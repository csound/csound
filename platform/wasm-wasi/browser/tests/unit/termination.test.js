import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import vm from "node:vm";
import { IPCMessagePorts } from "../../src/mains/messages.main.js";
import * as Comlink from "../../src/utils/comlink.js";

class TestPort extends EventTarget {
  constructor() {
    super();
    this.closed = false;
    this.listeners = new Set();
  }
  addEventListener(type, listener) {
    super.addEventListener(type, listener);
    this.listeners.add(listener);
  }
  removeEventListener(type, listener) {
    super.removeEventListener(type, listener);
    this.listeners.delete(listener);
  }
  postMessage() {}
  start() {}
  close() {
    this.closed = true;
  }
}

// Run the real processor methods with only browser host objects replaced.
// Its source imports bundle-only worker assets, so it cannot be imported by Node.
const processorSource = readFileSync(
  new URL("../../src/workers/worklet.singlethread.worker.js", import.meta.url),
  "utf8",
).replace(/^import[\s\S]*?;\s*$/gm, "");

const makeProcessor = () => {
  let Processor;
  vm.runInNewContext(processorSource, {
    AudioWorkletProcessor: class {
      constructor() {
        this.port = new TestPort();
      }
    },
    Comlink: { expose() {} },
    MessagePortState: class {},
    clearArray: (array) => array.splice(0),
    createAudioFade: (frames) => ({ frames }),
    getAudioFadeRemainingFrames: (fade) => fade.frames,
    log: () => () => {},
    console,
    registerProcessor: (_name, type) => {
      Processor = type;
    },
  });
  const processor = new Processor({});
  const messagePort = new TestPort();
  const rtmidiPort = new TestPort();
  processor.initializeMessagePort({ messagePort, rtmidiPort });
  return { processor, messagePort, rtmidiPort };
};

describe("instance termination", () => {
  it("destroys the local Csound and drops its heap, API and transferred ports", () => {
    const { processor, messagePort, rtmidiPort } = makeProcessor();
    const calls = [];
    const memory = new WebAssembly.Memory({ initial: 1, maximum: 1, shared: true });
    processor.csound = 123;
    processor.wasm = { exports: { memory } };
    processor.wasi = { memory };
    processor.csoundInputBuffer = new Float64Array(memory.buffer, 0, 16);
    processor.csoundOutputBuffer = new Float64Array(memory.buffer, 128, 16);
    processor.libraryCsound = {
      csoundStop: (instance) => calls.push(["stop", instance]),
      csoundDestroy: (instance) => calls.push(["destroy", instance]),
    };
    processor.combined = new Map([["wasm", processor.wasm]]);
    processor.terminate();
    processor.terminate();
    assert.deepEqual(calls, [
      ["stop", 123],
      ["destroy", 123],
    ]);
    for (const key of [
      "wasm",
      "wasi",
      "libraryCsound",
      "combined",
      "csound",
      "rtmidiPort",
      "csoundInputBuffer",
      "csoundOutputBuffer",
    ])
      assert.equal(processor[key], undefined, key);
    assert.equal(messagePort.closed, true);
    assert.equal(rtmidiPort.closed, true);
    // The Comlink port must stay open long enough to return the terminate reply.
    assert.equal(processor.port.closed, false);
    assert.equal(processor.process([], [[]]), false);
  });

  it("closes every IPC channel, including channels never transferred to a worker", () => {
    const ports = new IPCMessagePorts();
    const all = Object.values(ports).filter((port) => typeof port?.close === "function");
    let closes = 0;
    for (const port of all) {
      const close = port.close.bind(port);
      port.close = () => {
        closes++;
        close();
      };
    }
    try {
      ports.close();
      ports.close();
      assert.equal(closes, all.length);
    } finally {
      for (const port of all) port.close();
    }
  });

  it("drops the local engine and ports even when native destruction throws", () => {
    const { processor, messagePort, rtmidiPort } = makeProcessor();
    processor.csound = 123;
    processor.wasm = {};
    processor.wasi = {};
    processor.combined = new Map();
    processor.libraryCsound = {
      csoundStop() {},
      csoundDestroy() {
        throw new Error("native destructor failed");
      },
    };
    assert.throws(() => processor.terminate(), /native destructor failed/);
    processor.terminate();
    assert.equal(processor.wasm, undefined);
    assert.equal(processor.wasi, undefined);
    assert.equal(processor.libraryCsound, undefined);
    assert.equal(processor.combined, undefined);
    assert.equal(messagePort.closed, true);
    assert.equal(rtmidiPort.closed, true);
  });

  it("releases dead worker proxies without leaving reply listeners or pending calls", async () => {
    const endpoint = new TestPort();
    const proxy = Comlink.wrap(endpoint);
    const pending = proxy["callUncloned"]("csoundGetSr", []);
    const rejected = assert.rejects(pending, /released/i);
    assert.equal(endpoint.listeners.size, 1);
    await proxy[Comlink.releaseProxy](false);
    await rejected;
    assert.equal(endpoint.listeners.size, 0);
    assert.throws(() => proxy["callUncloned"](), /released/i);
  });

  it("keeps ordinary Comlink replies and acknowledged releases working", async () => {
    const { port1, port2 } = new MessageChannel();
    Comlink.expose({ value: () => 42 }, port1);
    const proxy = Comlink.wrap(port2);
    try {
      assert.equal(await proxy["value"](), 42);
      await proxy[Comlink.releaseProxy]();
      assert.throws(() => proxy["value"](), /released/i);
    } finally {
      port1.close();
      port2.close();
    }
  });
});
