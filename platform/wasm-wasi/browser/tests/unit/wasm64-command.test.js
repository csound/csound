import assert from "node:assert/strict";
import { WASI } from "../../src/filesystem/preview2.js";
import { instantiateCommand } from "../../src/utils/wasm64.js";

const encode = (text) => [...new TextEncoder().encode(text)];
const leb = (value) => {
  const bytes = [];
  do {
    const byte = value & 127;
    value >>>= 7;
    bytes.push(byte | (value ? 128 : 0));
  } while (value);
  return bytes;
};
const name = (text) => [...leb(encode(text).length), ...encode(text)];
const section = (id, bytes) => [id, ...leb(bytes.length), ...bytes];
const custom = (key, bytes) => section(0, [...name(key), ...bytes]);
const binary = (...sections) => new Uint8Array([0, 97, 115, 109, 1, 0, 0, 0, ...sections.flat()]);

// A memory64 command and its memory32 adapter import the same engine memory.
// Both export a constant allocator; run returns success without allocating.
function commandModule(maximum) {
  const memoryImport = section(2, [
    1,
    ...name("env"),
    ...name("memory"),
    2,
    ...(maximum === undefined ? [4, 1] : [5, 1, ...leb(maximum)]),
  ]);
  const bridge = binary(
    section(1, [1, 0x60, 4, 0x7f, 0x7f, 0x7f, 0x7f, 1, 0x7f]),
    memoryImport,
    section(3, [1, 0]),
    section(4, [1, 0x70, 0, 1]),
    section(5, [1, 0, 1]),
    section(7, [
      3,
      ...name("memory"),
      2,
      1,
      ...name("engine-functions"),
      1,
      0,
      ...name("cabi_realloc"),
      0,
      0,
    ]),
    section(10, [1, 5, 0, 0x41, 0x80, 8, 0x0b]),
  );
  const run = "wasi:cli/run@0.2.12#run";
  const abi = {
    pointerSize: 8,
    memoryMinimum: 1,
    memoryMaximum: maximum ?? null,
    exports: {
      cabi_realloc: [["i64", "i64", "i64", "i64"], ["i64"]],
      [run]: [[], ["i32"]],
    },
  };
  return new WebAssembly.Module(
    binary(
      section(1, [2, 0x60, 4, 0x7e, 0x7e, 0x7e, 0x7e, 1, 0x7e, 0x60, 0, 1, 0x7f]),
      memoryImport,
      section(3, [2, 0, 1]),
      section(7, [2, ...name("cabi_realloc"), 0, 0, ...name(run), 0, 1]),
      section(10, [2, 5, 0, 0x42, 0x80, 8, 0x0b, 4, 0, 0x41, 0, 0x0b]),
      custom("csound.abi", encode(JSON.stringify(abi))),
      custom("csound.wasi-bridge", [...bridge]),
    ),
  );
}

describe("wasm64 command memory", () => {
  it("runs bounded commands and enforces their memory limit", () => {
    const wasi = new WASI({ args: ["bounded"] });
    assert.equal(wasi.startCommand(instantiateCommand(commandModule(2), wasi)), 0);
    assert.equal(wasi.memory.grow(1n), 1n);
    assert.throws(() => wasi.memory.grow(1n), RangeError);
  });

  it("runs commands without a declared memory limit", () => {
    const wasi = new WASI({ args: ["unbounded"] });
    assert.equal(wasi.startCommand(instantiateCommand(commandModule(), wasi)), 0);
    assert.equal(wasi.memory.grow(2n), 1n);
  });
});
