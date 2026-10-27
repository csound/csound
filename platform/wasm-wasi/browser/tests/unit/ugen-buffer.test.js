import assert from "node:assert/strict";

describe("float UGen sample views", () => {
  let floatView, doubleCopy;
  before(async () => {
    const previous = Object.getOwnPropertyDescriptor(globalThis, "goog");
    globalThis.goog = { define: (_name, value) => value };
    try {
      ({ csoundUgenVarGetFloat32Array: floatView, csoundUgenVarGetFloat64Array: doubleCopy } =
        await import("../../src/modules/ugen.js"));
    } finally {
      if (previous) Object.defineProperty(globalThis, "goog", previous);
      else delete globalThis.goog;
    }
  });

  it("writes through a four-byte-aligned view and keeps legacy copies separate", () => {
    const memory = new WebAssembly.Memory({ initial: 1 });
    const native = new Float32Array(memory.buffer, 4, 3);
    native.set([0.125, 0.25, 0.5]);
    const wasm = {
      exports: {
        memory,
        csoundUgenVarGetData: () => 4,
        csoundUgenVarGetKsmps: () => 3,
      },
    };
    const view = floatView(wasm)(1);
    const copy = doubleCopy(wasm)(1);
    assert(view instanceof Float32Array);
    assert(copy instanceof Float64Array);
    view[1] = 0.75;
    assert.equal(native[1], 0.75);
    assert.deepEqual([...copy], [0.125, 0.25, 0.5]);
  });
});
