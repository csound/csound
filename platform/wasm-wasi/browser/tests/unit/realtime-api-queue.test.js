import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import vm from "node:vm";
import sinon from "sinon";
import { AUDIO_STATE, initialSharedState } from "../../src/constants.js";
import { stopSAB, wakeSAB } from "../../src/utils/sab-transport.js";
import {
  submitRealtimeApiCall,
  settleRealtimeApiCompletion,
} from "../../src/utils/realtime-api-queue.js";
import { realtimeBackgroundOperations } from "../../src/utils/realtime-api.js";
import { createWasiThreadEnvironment } from "../../src/utils/wasi-threads-imports.js";

const fixture = (capacity = 64) => {
  const memory = new WebAssembly.Memory({ initial: 1, maximum: 1, shared: true });
  const requests = [];
  const wasm = {
    wasi: { memory },
    exports: {
      csoundRealtimeApiCapacityWasi: () => capacity,
      csoundRealtimeApiBufferWasi: () => 128,
      csoundRealtimeApiSubmitWasi: (...args) => {
        requests.push(args);
        return 0;
      },
    },
  };
  return { wasm, memory, requests };
};

// Exercise the real main-thread stop methods without constructing browser hosts.
const mainSource = readFileSync(new URL("../../src/mains/sab.main.js", import.meta.url), "utf8")
  .replace(/^import[\s\S]*?;\s*$/gm, "")
  .replace(
    "export default SharedArrayBufferMainThread;",
    "globalThis.Main = SharedArrayBufferMainThread;",
  );
const context = {
  realtimeBackgroundOperations,
  AUDIO_STATE,
  stopSAB,
  wakeSAB,
  csoundApiRename: (key) => key,
  setTimeout: (...args) => setTimeout(...args),
  clearTimeout: (timer) => clearTimeout(timer),
};
vm.runInNewContext(mainSource, context);

const mainFixture = () => {
  const main = Object.create(context.Main.prototype);
  main.threaded = true;
  main.realtimeMode = true;
  main.callbackId = 0;
  main.callbackOrder = 0;
  main.callbackBuffer = {};
  main.performanceGeneration = 2;
  main.audioStatePointer = new Int32Array(new SharedArrayBuffer(initialSharedState.length * 4));
  main.eventPromises = {
    getStartRemainingMs: () => 0,
    releaseStartPromise() {},
    releasePausePromise() {},
    releaseResumePromise() {},
    releaseStopPromise() {},
  };
  main.rejectThreadFailure = () => {};
  main.publicEvents = { triggerMessage() {}, triggerRealtimePerformanceEnded() {} };
  main.terminateInstance = async () => {};
  return main;
};

describe("background realtime API", () => {
  it("copies UTF-8 strings with terminators into the bounded native slot", () => {
    const { wasm, memory, requests } = fixture();
    submitRealtimeApiCall(wasm, 10, "csoundSetStringChannel", ["code", "π"], 11, 12);
    assert.deepEqual(requests, [[10, 4, 11, 12, 8, 0]]);
    assert.deepEqual(
      [...new Uint8Array(memory.buffer, 128, 8)],
      [99, 111, 100, 101, 0, 207, 128, 0],
    );
    submitRealtimeApiCall(wasm, 10, "csoundSetControlChannel", ["gain", 0.25], 12, 12);
    assert.deepEqual(requests[1], [10, 6, 12, 12, 5, 0.25]);
  });

  it("rejects oversized UTF-8 and full queues before writing memory", () => {
    const { wasm, memory, requests } = fixture(6);
    const bytes = new Uint8Array(memory.buffer);
    bytes.fill(0xa5);
    assert.throws(
      () => submitRealtimeApiCall(wasm, 1, "csoundEvalCode", ["πππ"], 1, 1),
      /capacity/,
    );
    assert.throws(
      () => submitRealtimeApiCall(wasm, 1, "csoundEvalCode", ["123456"], 1, 1),
      /capacity/,
    );
    wasm.exports.csoundRealtimeApiBufferWasi = () => 0;
    assert.throws(
      () => submitRealtimeApiCall(wasm, 1, "csoundEvalCode", ["0"], 1, 1),
      /queue is full/,
    );
    assert.equal(requests.length, 0);
    assert.ok(bytes.every((byte) => byte === 0xa5));
  });

  it("rejects a full queue before encoding text on the render worker", () => {
    const { wasm } = fixture(1048576);
    wasm.exports.csoundRealtimeApiBufferWasi = () => 0;
    const encode = TextEncoder.prototype.encode;
    try {
      TextEncoder.prototype.encode = () => assert.fail("full queue must not encode source");
      assert.throws(
        () => submitRealtimeApiCall(wasm, 1, "csoundEvalCode", [" ".repeat(1000000)], 1, 1),
        /queue is full/,
      );
    } finally {
      TextEncoder.prototype.encode = encode;
    }
  });

  it("reports native admission errors", () => {
    const { wasm } = fixture();
    wasm.exports.csoundRealtimeApiSubmitWasi = () => -1;
    assert.throws(() => submitRealtimeApiCall(wasm, 1, "csoundEvalCode", ["0"], 1, 1), /rejected/);
  });

  it("copies native results before the worker frees them, without log truncation", () => {
    const { memory } = fixture();
    const result = "π".repeat(10000);
    const bytes = new TextEncoder().encode(result);
    new Uint8Array(memory.buffer, 128, bytes.length).set(bytes);
    const messages = [];
    const env = createWasiThreadEnvironment({ memory, postMessage: (data) => messages.push(data) });
    env.csoundWasiApiComplete(3, 7, 0, 0, 128);
    new Uint8Array(memory.buffer, 128, bytes.length).fill(0);
    assert.deepEqual(messages, [
      { type: "wasi-api-completion", id: 3, generation: 7, status: 0, answer: result },
    ]);
  });

  it("ignores stale replies and preserves numeric, string, and void results", () => {
    for (const [api, answer, expected] of [
      ["csoundEvalCode", 42, 42],
      ["csoundCompileOrc", -1, -1],
      ["csoundGetStringChannel", "hello", "hello"],
      ["csoundSetStringChannel", 0, undefined],
      ["csoundSetControlChannel", 0, undefined],
    ]) {
      const results = [];
      const callbacks = {
        1: { apiKey: api, generation: 2, resolveCallback: (value) => results.push(value) },
      };
      assert.equal(
        settleRealtimeApiCompletion(callbacks, { id: 1, generation: 1, status: 0, answer }),
        false,
      );
      assert.equal(results.length, 0);
      assert.equal(
        settleRealtimeApiCompletion(callbacks, { id: 1, generation: 2, status: 0, answer }),
        true,
      );
      assert.deepEqual(results, [expected]);
      assert.equal(
        settleRealtimeApiCompletion(callbacks, { id: 1, generation: 2, status: 0, answer }),
        false,
      );
    }
  });

  it("settles accepted API calls before exposing stop across reordered ports", () => {
    const main = Object.create(context.Main.prototype);
    const calls = [];
    main.realtimeMode = true;
    main.callbackBuffer = {
      1: {
        apiKey: "csoundEvalCode",
        generation: 2,
        inFlight: true,
        resolveCallback: (value) => calls.push(["eval", value]),
      },
      2: {
        apiKey: "csoundEvalCode",
        generation: 2,
        inFlight: true,
        rejectCallback: (error) => calls.push(["cancel", error.message]),
      },
      3: {
        apiKey: "csoundEvalCode",
        generation: 2,
        inFlight: false,
        rejectCallback: () => calls.push(["unsent"]),
      },
    };
    main.publicEvents = { triggerRealtimePerformanceEnded: () => calls.push(["ended"]) };
    main.eventPromises = {
      releasePausePromise() {},
      releaseResumePromise() {},
      releaseStopPromise: () => calls.push(["stop"]),
    };
    main.finishPerformanceEnd("realtimePerformanceEnded");
    assert.deepEqual(calls, [["unsent"]]);
    settleRealtimeApiCompletion(main.callbackBuffer, {
      id: 1,
      generation: 2,
      status: 0,
      answer: 42,
    });
    main.maybeFinishPerformanceEnd();
    assert.deepEqual(calls, [["unsent"], ["eval", 42]]);
    settleRealtimeApiCompletion(main.callbackBuffer, { id: 2, generation: 2, status: -2 });
    main.maybeFinishPerformanceEnd();
    assert.deepEqual(calls, [
      ["unsent"],
      ["eval", 42],
      ["cancel", "Performance stopped before the call ran"],
      ["ended"],
      ["stop"],
    ]);
    main.maybeFinishPerformanceEnd();
    assert.equal(calls.length, 5);
  });

  it("keeps a long compile pending until completion rather than timing out", async () => {
    const clock = sinon.useFakeTimers();
    try {
      const main = mainFixture();
      let completed = false;
      const result = main.enqueueCallback("csoundCompileOrc", [1, "instr LongEdit\nendin"]);
      result.then(
        () => (completed = true),
        () => (completed = true),
      );
      await clock.tickAsync(60000);
      assert.equal(completed, false);
      assert.equal(Object.keys(main.callbackBuffer).length, 1);
      settleRealtimeApiCompletion(main.callbackBuffer, {
        id: 0,
        generation: 2,
        status: 0,
        answer: 0,
      });
      assert.equal(await result, 0);
      assert.equal(clock.countTimers(), 0);
    } finally {
      clock.restore();
    }
  });

  it("rejects long background calls on worker failure or before dispatch at stop", async () => {
    for (const terminate of [false, true]) {
      const main = mainFixture();
      const result = main.enqueueCallback("csoundEvalCode", [1, "return 42"]);
      const rejected = assert.rejects(result, /worker failed|Performance ended/);
      if (terminate) main.failThreadedInstance(new Error("worker failed"));
      else main.finishPerformanceEnd("realtimePerformanceEnded");
      await rejected;
      assert.equal(Object.keys(main.callbackBuffer).length, 0);
    }
  });

  it("retains transport deadlines for short calls and nonthreaded performance", async () => {
    const clock = sinon.useFakeTimers();
    try {
      for (const [threaded, api] of [
        [true, "csoundGetKr"],
        [false, "csoundCompileOrc"],
      ]) {
        const main = mainFixture();
        main.threaded = threaded;
        const result = main.enqueueCallback(api, [1]);
        const rejected = assert.rejects(result, /Worker timed out/);
        await clock.tickAsync(10000);
        await rejected;
        assert.equal(Object.keys(main.callbackBuffer).length, 0);
      }
    } finally {
      clock.restore();
    }
  });

  it("preserves enqueue order through id rollover and reordered port replies", async () => {
    const main = mainFixture();
    for (let round = 0; round < 100; round++) {
      main.callbackId = 0xffff_fffe;
      const results = Array.from({ length: 40 }, (_, index) =>
        main.enqueueCallback("csoundEvalCode", [1, `return ${index}`]),
      );
      const first = main.takeCallbacks();
      const second = main.takeCallbacks();
      const dispatched = [...first, ...second];
      assert.equal(first.length, 32);
      assert.equal(second.length, 8);
      assert.deepEqual(
        dispatched.map((call) => call.argumentz[1]),
        Array.from({ length: 40 }, (_, index) => `return ${index}`),
      );
      assert.equal(new Set(dispatched.map((call) => call.id)).size, 40);
      // Force rollover while high and low IDs are still in flight.
      main.callbackId = 0xffff_fffe;
      const next = main.enqueueCallback("csoundEvalCode", [1, "return 40"]);
      const extra = main.takeCallbacks();
      assert.equal(extra.length, 1);
      assert.equal(extra[0].id, "38");
      // Replies from both ports may overtake one another; each keeps its call.
      const env = createWasiThreadEnvironment({
        memory: new WebAssembly.Memory({ initial: 1 }),
        postMessage: (payload) => settleRealtimeApiCompletion(main.callbackBuffer, payload),
      });
      for (const [index, call] of [...dispatched, ...extra].entries()) {
        if (index % 2) continue;
        // Wasm i32 imports pass the high unsigned IDs as negative JS numbers.
        env.csoundWasiApiComplete(Number(call.id) | 0, 2, 0, index, 0);
      }
      main.settleCallbackReplies(
        dispatched.flatMap((call, index) =>
          index % 2 ? [{ id: call.id, generation: 2, answer: index }] : [],
        ),
      );
      assert.deepEqual(
        await Promise.all([...results, next]),
        Array.from({ length: 41 }, (_, index) => index),
      );
      assert.equal(Object.keys(main.callbackBuffer).length, 0);
    }
  });

  it("ignores replies from a previous performance when an id is reused", async () => {
    const main = mainFixture();
    const result = main.enqueueCallback("csoundEvalCode", [1, "return 42"]);
    main.takeCallbacks();
    main.settleCallbackReplies([{ id: 0, generation: 1, answer: -1 }]);
    assert.equal(
      settleRealtimeApiCompletion(main.callbackBuffer, {
        id: 0,
        generation: 1,
        status: 0,
        answer: -1,
      }),
      false,
    );
    assert.equal(Object.keys(main.callbackBuffer).length, 1);
    main.settleCallbackReplies([{ id: 0, generation: 2, answer: 42 }]);
    assert.equal(await result, 42);
    assert.equal(
      settleRealtimeApiCompletion(main.callbackBuffer, {
        id: 0,
        generation: 2,
        status: 0,
        answer: -1,
      }),
      false,
    );
  });

  it("settles nonthreaded replies with the same generation check", async () => {
    const main = mainFixture();
    main.threaded = false;
    const result = main.enqueueCallback("csoundCompileOrc", [1, "instr Note\nendin"]);
    const [request] = main.takeCallbacks();
    assert.equal(request.generation, main.performanceGeneration);
    main.settleCallbackReplies([{ ...request, answer: 0 }]);
    assert.equal(await result, 0);
    assert.equal(Object.keys(main.callbackBuffer).length, 0);
  });
});
