import assert from "node:assert/strict";
import { withWasiThreadStack } from "../../src/utils/wasi-thread-stack.js";

const string = (value) => [value.length, ...new TextEncoder().encode(value)];
const section = (id, bytes) => [id, bytes.length, ...bytes];
// A Wasm initializer with a real stack store, unlike the stack-free linker TLS
// stub. A compiler may emit this shape for a plugin's start or TLS function.
const initializer = new WebAssembly.Module(
  new Uint8Array([
    0,
    97,
    115,
    109,
    1,
    0,
    0,
    0,
    ...section(1, [1, 0x60, 0, 0]),
    ...section(2, [
      2,
      ...string("env"),
      ...string("memory"),
      2,
      0,
      1,
      ...string("env"),
      ...string("__stack_pointer"),
      3,
      0x7f,
      1,
    ]),
    ...section(3, [1, 0]),
    ...section(7, [1, ...string("initialize"), 0, 0]),
    ...section(10, [1, 12, 0, 0x23, 0, 0x41, 16, 0x6b, 0x41, 42, 0x36, 2, 0, 0x0b]),
  ]),
);

describe("child plugin setup stack", () => {
  it("keeps Wasm stack stores away from the parent's live stack", () => {
    const memory = new WebAssembly.Memory({ initial: 1 });
    const stack = new WebAssembly.Global({ value: "i32", mutable: true }, 1024);
    const bytes = new Int32Array(memory.buffer);
    bytes[(1024 - 16) / 4] = 99;
    const instance = new WebAssembly.Instance(initializer, {
      env: { memory, __stack_pointer: stack },
    });
    withWasiThreadStack({ __stack_pointer: stack }, 4096, () => instance.exports.initialize());
    assert.equal(bytes[(1024 - 16) / 4], 99);
    assert.equal(bytes[(4096 - 16) / 4], 42);
    assert.equal(stack.value, 1024);
    assert.throws(
      () =>
        withWasiThreadStack({ __stack_pointer: stack }, 4096, () => {
          throw new Error("plugin failed");
        }),
      /plugin failed/,
    );
    assert.equal(stack.value, 1024);
  });
});
