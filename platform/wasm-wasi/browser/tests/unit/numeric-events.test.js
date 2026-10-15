/* eslint-env mocha */
import assert from "node:assert/strict";

let event;
let originalGoogDescriptor;

describe("numeric score events", () => {
  before(async () => {
    originalGoogDescriptor = Object.getOwnPropertyDescriptor(globalThis, "goog");
    Object.defineProperty(globalThis, "goog", {
      configurable: true,
      value: { define: (_name, value) => value },
    });
    ({ csoundEvent: event } = await import("../../src/modules/control-events.js"));
  });
  after(() => {
    if (originalGoogDescriptor) Object.defineProperty(globalThis, "goog", originalGoogDescriptor);
    else delete globalThis.goog;
  });

  const fixture = (nativeEvent) => {
    const memory = new WebAssembly.Memory({ initial: 1, maximum: 1, shared: true });
    const allocations = [];
    const freed = [];
    return {
      allocations,
      freed,
      memory,
      wasm: {
        wasi: { memory },
        exports: {
          allocFloatArray: (length) => {
            allocations.push(length);
            return 64;
          },
          freeFloatArrayMem: (pointer) => {
            freed.push(pointer);
          },
          csoundEvent: (...args) => nativeEvent(memory, ...args),
        },
      },
    };
  };

  it("copies numeric p-fields into shared memory and requests async admission", () => {
    const fields = [1, 0, 0.125, 660];
    const state = fixture((memory, csound, type, pointer, length, async) => {
      assert.deepEqual([csound, type, length, async], [123, 0, 4, 1]);
      assert.deepEqual([...new Float64Array(memory.buffer, pointer, length)], fields);
    });
    event(state.wasm)(123, 0, fields);
    assert.deepEqual(state.allocations, [4]);
    assert.deepEqual(state.freed, [64]);
  });

  it("rejects invalid or oversized events before allocating", () => {
    const state = fixture(() => assert.fail("native event must not run"));
    for (const [type, fields] of [
      [3, []],
      [0, [1, 0]],
      [0, [1, 0, NaN]],
      [0, [1, 0, Infinity]],
      [0, Array(3)],
      [0, [1, , 0.25]],
      [0, Array(65).fill(1)],
      [0, "i1 0 1"],
    ]) {
      assert.throws(() => event(state.wasm)(123, type, fields), TypeError);
    }
    assert.deepEqual(state.allocations, []);
  });

  it("frees copied p-fields if native admission throws", () => {
    const failure = new Error("native failure");
    const state = fixture(() => {
      throw failure;
    });
    assert.throws(() => event(state.wasm)(123, 0, [1, 0, 1]), failure);
    assert.deepEqual(state.freed, [64]);
  });
});
