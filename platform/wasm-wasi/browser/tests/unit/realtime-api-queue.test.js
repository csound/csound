import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import vm from "node:vm";
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
const context = { realtimeBackgroundOperations };
vm.runInNewContext(mainSource, context);

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
});
