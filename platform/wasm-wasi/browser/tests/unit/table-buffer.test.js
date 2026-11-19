import assert from "node:assert/strict";
import {
  csoundGetTableArgs,
  csoundTableCopyIn,
  csoundTableCopyOut,
} from "../../src/modules/table.js";

describe("table transfer buffers", () => {
  it("throws if allocation fails before calling any native table transfer", () => {
    const memory = new WebAssembly.Memory({ initial: 1 });
    const wasm = {
      wasi: { memory },
      exports: {
        csoundTableLength: () => 2,
        csoundGetSizeOfMYFLT: () => 4,
        allocFloatArray: () => 0,
        allocByteMem: () => 0,
      },
    };
    assert.throws(() => csoundTableCopyIn(wasm)(1, 1, [0.25, 0.5]), /Could not allocate/);
    assert.throws(() => csoundTableCopyOut(wasm)(1, 1), /Could not allocate/);
    assert.throws(() => csoundGetTableArgs(wasm)(1, 1), /Could not allocate/);
  });

  it("copies samples synchronously and frees the temporary buffer", () => {
    const memory = new WebAssembly.Memory({ initial: 1 });
    const native = new Float64Array(memory.buffer, 64, 2);
    const wasm = {
      wasi: { memory },
      exports: {
        csoundTableLength: () => 2,
        csoundGetSizeOfMYFLT: () => 8,
        allocFloatArray: () => 64,
        csoundTableCopyOut: (csound, table, pointer, async) => {
          assert.deepEqual([csound, table, pointer, async], [1, 2, 64, 0]);
          native.set([0.25, 0.5]);
        },
        freeFloatArrayMem: (pointer) => {
          assert.equal(pointer, 64);
          native.fill(0);
        },
      },
    };
    assert.deepEqual(csoundTableCopyOut(wasm)(1, 2), new Float64Array([0.25, 0.5]));
    assert.deepEqual(native, new Float64Array(2));
  });
});
