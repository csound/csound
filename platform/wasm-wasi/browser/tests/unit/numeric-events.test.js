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

  const fixture = (nativeEvent, precision = 8) => {
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
          csoundGetSizeOfMYFLT: () => precision,
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

  it("submits short end events with valid storage even for no p-fields", () => {
    for (const fields of [[], [0], [0, 0.25]]) {
      const state = fixture((memory, csound, type, pointer, length, async) => {
        assert.deepEqual([type, length, async], [2, fields.length, 1]);
        assert.deepEqual([...new Float64Array(memory.buffer, pointer, length)], fields);
      });
      event(state.wasm)(123, 2, fields);
      assert.deepEqual(state.allocations, [Math.max(1, fields.length)]);
      assert.deepEqual(state.freed, [64]);
    }
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

  it("rejects an incompatible sample width before allocating or calling Csound", () => {
    for (const precision of [4, 16]) {
      const state = fixture(() => assert.fail("native event must not run"), precision);
      new Uint8Array(state.memory.buffer).fill(0xa5);
      for (const [type, fields] of [
        [0, [1, 0, 0.1]],
        [2, []],
      ])
        assert.throws(() => event(state.wasm)(123, type, fields), /double-precision Csound module/);
      assert.deepEqual(state.allocations, []);
      assert.deepEqual(state.freed, []);
      assert.ok(new Uint8Array(state.memory.buffer).every((byte) => byte === 0xa5));
    }
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
