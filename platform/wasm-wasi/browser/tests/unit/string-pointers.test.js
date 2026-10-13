/* eslint-env mocha */

import assert from "node:assert/strict";

let string2ptr;
let originalGoogDescriptor;

describe("WASM string allocation", () => {
  before(async () => {
    originalGoogDescriptor = Object.getOwnPropertyDescriptor(globalThis, "goog");
    Object.defineProperty(globalThis, "goog", {
      configurable: true,
      value: { define: (_name, value) => value },
    });
    ({ string2ptr } = await import("../../src/utils/string-pointers.js"));
  });

  after(() => {
    if (originalGoogDescriptor) {
      Object.defineProperty(globalThis, "goog", originalGoogDescriptor);
    } else {
      delete globalThis.goog;
    }
  });

  it("leaves memory untouched when allocation fails", () => {
    const memory = new WebAssembly.Memory({ initial: 1 });
    const bytes = new Uint8Array(memory.buffer);
    bytes.fill(42);
    const wasm = {
      exports: { allocStringMem: () => 0 },
      wasi: { memory },
    };
    assert.throws(() => string2ptr(wasm, "hello"), /Could not allocate/);
    assert.ok(bytes.every((byte) => byte === 42));
  });

  it("copies UTF-8 text after allocation grows memory", () => {
    const memory = new WebAssembly.Memory({ initial: 1 });
    const wasm = {
      exports: {
        allocStringMem: (length) => {
          assert.equal(length, 3); // UTF-8 bytes for the euro sign.
          memory.grow(1);
          return 65536;
        },
      },
      wasi: { memory },
    };
    assert.equal(string2ptr(wasm, "€"), 65536);
    assert.deepEqual(Array.from(new Uint8Array(memory.buffer, 65536, 4)), [226, 130, 172, 0]);
  });
});
