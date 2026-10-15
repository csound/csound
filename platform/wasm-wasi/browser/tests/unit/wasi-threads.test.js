import assert from "node:assert/strict";
import { Worker, MessageChannel } from "node:worker_threads";
import {
  createWasiThreadHost,
  superviseWasiPerformance,
  WASI_THREAD_IDLE,
  WASI_THREAD_STRIDE,
} from "../../src/utils/wasi-threads.js";
import {
  createWasiClockImports,
  createWasiThreadImports,
} from "../../src/utils/wasi-threads-imports.js";

const memory = () => new WebAssembly.Memory({ initial: 1, maximum: 2, shared: true });
const string = (value) => [value.length, ...new TextEncoder().encode(value)];
const section = (id, bytes) => [id, bytes.length, ...bytes];
// A real worker instance stores its assigned tid at the shared argument pointer.
// Passing -1 traps, allowing the UI-side failure path to run while the host waits.
const threadModule = () =>
  new WebAssembly.Module(
    new Uint8Array([
      0,
      97,
      115,
      109,
      1,
      0,
      0,
      0,
      ...section(1, [1, 0x60, 2, 0x7f, 0x7f, 0]),
      ...section(2, [1, ...string("env"), ...string("memory"), 2, 3, 1, 2]),
      ...section(3, [1, 0]),
      ...section(7, [1, ...string("wasi_thread_start"), 0, 0]),
      ...section(
        10,
        [
          1, 19, 0, 0x20, 1, 0x41, 0x7f, 0x46, 0x04, 0x40, 0x00, 0x0b, 0x20, 1, 0x20, 0, 0xfe, 0x17,
          2, 0, 0x0b,
        ],
      ),
    ]),
  );
const tick = () => new Promise((resolve) => setTimeout(resolve, 5));
async function until(predicate) {
  const deadline = Date.now() + 3000;
  while (!predicate()) {
    if (Date.now() > deadline) throw new Error("Worker did not make progress");
    await tick();
  }
}

class FakeWorker extends EventTarget {
  postMessage(data) {
    this.data = data;
  }
  terminate() {
    this.terminated = true;
  }
  ready() {
    const control = new Int32Array(this.data.control);
    Atomics.store(control, 1 + this.data.index * WASI_THREAD_STRIDE, WASI_THREAD_IDLE);
    this.dispatchEvent(new MessageEvent("message", { data: { type: "ready" } }));
  }
}

describe("WASI thread host", () => {
  it("does not admit threads before every worker is ready or above pool capacity", async () => {
    const workers = [];
    const shared = memory();
    const host = createWasiThreadHost({
      module: threadModule(),
      memory: shared,
      poolSize: 2,
      createWorker: () => {
        const worker = new FakeWorker();
        workers.push(worker);
        return worker;
      },
    });
    try {
      assert.equal(host.threadSpawn(128), -6);
      workers[0].ready();
      assert.equal(host.threadSpawn(128), -6);
      workers[1].ready();
      await host.ready;
      assert.equal(host.threadSpawn(128), 2);
      host.threadSpawn(132, 64);
      assert.equal(new DataView(shared.buffer).getUint32(64, true), 0);
      assert.equal(new DataView(shared.buffer).getUint32(68, true), 3);
      assert.equal(host.threadSpawn(136), -6);
      Atomics.store(host.control, 1, WASI_THREAD_IDLE);
      assert.equal(host.threadSpawn(136), 4);
    } finally {
      host.dispose();
    }
    assert.equal(host.threadSpawn(140), -6);
    assert.ok(workers.every((worker) => worker.terminated));
  });

  it("rejects readiness and releases the whole pool on a startup error", async () => {
    const workers = [];
    let failure;
    const host = createWasiThreadHost({
      module: threadModule(),
      memory: memory(),
      poolSize: 2,
      onError: (error) => {
        failure = error;
      },
      createWorker: () => {
        const worker = new FakeWorker();
        workers.push(worker);
        return worker;
      },
    });
    workers[0].dispatchEvent(
      new MessageEvent("message", { data: { type: "fatal", message: "bad import" } }),
    );
    await assert.rejects(host.ready, /bad import/);
    assert.match(failure.message, /bad import/);
    assert.ok(workers.every((worker) => worker.terminated));
    assert.equal(Atomics.load(host.control, 0), 2);
  });

  it("bounds startup when a worker never replies", async () => {
    const worker = new FakeWorker();
    const host = createWasiThreadHost({
      module: threadModule(),
      memory: memory(),
      poolSize: 1,
      readyTimeout: 10,
      createWorker: () => worker,
    });
    await assert.rejects(host.ready, /did not become ready/);
    assert.equal(worker.terminated, true);
    assert.equal(host.threadSpawn(128), -6);
  });

  it("runs real child instances and reports a trap directly to their UI owner", async () => {
    const workers = [];
    const ports = [];
    let fatal;
    const shared = memory();
    const host = createWasiThreadHost({
      module: threadModule(),
      memory: shared,
      poolSize: 2,
      createWorker: () => {
        const worker = new Worker(new URL("./fixtures/wasi-thread-worker.mjs", import.meta.url));
        workers.push(worker);
        worker.on("message", (data) => {
          if (data.type === "wasi-thread-fatal") fatal = data.message;
        });
        const { port1, port2 } = new MessageChannel();
        ports.push(port2);
        worker.postMessage({ type: "connect", port: port1 }, [port1]);
        return {
          addEventListener: (...args) => port2.addEventListener(...args),
          postMessage: (...args) => port2.postMessage(...args),
          start: () => port2.start(),
          terminate: () => {},
        };
      },
    });
    try {
      await host.ready;
      const words = new Int32Array(shared.buffer);
      const first = host.threadSpawn(128);
      await until(() => Atomics.load(words, 32) === first);
      const second = host.threadSpawn(132);
      await until(() => Atomics.load(words, 33) === second);
      assert.notEqual(first, second);
      await until(() => Atomics.load(host.control, 1) === WASI_THREAD_IDLE);
      host.threadSpawn(-1);
      await until(() => fatal !== undefined);
      assert.match(fatal, /unreachable/);
      assert.equal(Atomics.load(host.control, 0), 2);
      assert.equal(host.threadSpawn(136), -6);
    } finally {
      host.dispose();
      for (const port of ports) port.close();
      await Promise.all(workers.map((worker) => worker.terminate()));
    }
  });
});

describe("WASI thread imports", () => {
  it("uses the AudioWorklet sample clock when performance is absent", () => {
    const previousPerformance = Object.getOwnPropertyDescriptor(globalThis, "performance");
    const previousTime = Object.getOwnPropertyDescriptor(globalThis, "currentTime");
    const shared = memory();
    try {
      delete globalThis.performance;
      Object.defineProperty(globalThis, "currentTime", { value: 1.5, configurable: true });
      const clocks = createWasiClockImports({ memory: shared });
      assert.equal(clocks.clock_time_get(1, BigInt(0), 16), 0);
      assert.equal(new DataView(shared.buffer).getBigUint64(16, true), BigInt(1500000000));
    } finally {
      Object.defineProperty(globalThis, "performance", previousPerformance);
      if (previousTime) Object.defineProperty(globalThis, "currentTime", previousTime);
      else delete globalThis.currentTime;
    }
  });
  it("implements nanosleep clock events and rejects filesystem polling", () => {
    const shared = memory();
    const control = new Int32Array(new SharedArrayBuffer(16));
    const clocks = createWasiClockImports({ memory: shared, control });
    const view = new DataView(shared.buffer);
    view.setBigUint64(64, 123n, true);
    view.setBigUint64(88, 1000000n, true);
    assert.equal(clocks.poll_oneoff(64, 128, 1, 32), 0);
    assert.equal(view.getBigUint64(128, true), 123n);
    assert.equal(view.getUint16(136, true), 0);
    assert.equal(view.getUint32(32, true), 1);
    view.setUint8(72, 1);
    assert.equal(clocks.poll_oneoff(64, 128, 1, 32), 58);
    view.setUint8(72, 0);
    Atomics.store(control, 0, 1);
    assert.throws(() => clocks.poll_oneoff(64, 128, 1, 32), /stopped/);
  });

  it("uses the parent clock origin and returns elapsed absolute timers immediately", () => {
    const shared = memory();
    const control = new Int32Array(new SharedArrayBuffer(16));
    const clocks = createWasiClockImports({
      memory: shared,
      control,
      clockOrigin: performance.timeOrigin - 1000,
    });
    assert.equal(clocks.clock_time_get(1, 0n, 32), 0);
    const view = new DataView(shared.buffer);
    assert.ok(view.getBigUint64(32, true) >= 1000000000n);
    view.setUint32(80, 1, true);
    view.setUint16(104, 1, true);
    assert.equal(clocks.poll_oneoff(64, 128, 1, 32), 0);
    assert.equal(view.getUint32(32, true), 1);
  });

  it("denies nested spawn and file syscalls without creating descriptor copies", () => {
    const module = new WebAssembly.Module(
      new Uint8Array([
        0,
        97,
        115,
        109,
        1,
        0,
        0,
        0,
        ...section(1, [1, 0x60, 0, 1, 0x7f]),
        ...section(2, [1, ...string("wasi_snapshot_preview1"), ...string("fd_read"), 0, 0]),
      ]),
    );
    const shared = memory();
    const imports = createWasiThreadImports({
      module,
      memory: shared,
      control: new Int32Array(new SharedArrayBuffer(16)),
      index: 0,
      postMessage: () => {},
    });
    assert.equal(imports.wasi_snapshot_preview1.fd_read(), 52);
    assert.equal(imports.wasi["thread-spawn"](128), -6);
    imports.wasi["thread-spawn"](128, 64);
    assert.equal(new DataView(shared.buffer).getUint32(64, true), 1);
    assert.equal(new DataView(shared.buffer).getUint32(68, true), 6);
  });
});

describe("WASI performance supervision", () => {
  it("forwards a deferred render trap to the UI supervisor", async () => {
    const trap = new WebAssembly.RuntimeError("render trap");
    const failures = [];
    const render = superviseWasiPerformance(
      async () => {
        await Promise.resolve();
        throw trap;
      },
      (error) => failures.push(error),
    );
    await render({});
    assert.deepEqual(failures, [trap]);
  });

  it("leaves a completed render alone", async () => {
    let rendered;
    const render = superviseWasiPerformance(
      async (payload) => {
        rendered = payload;
      },
      () => assert.fail("no fatal error"),
    );
    await render(42);
    assert.equal(rendered, 42);
  });
});
