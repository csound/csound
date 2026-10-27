import assert from "node:assert/strict";
import fs from "node:fs";

globalThis.goog = { define: (_name, value) => value, global: globalThis };
const { WASI } = await import("../browser/src/filesystem/preview2.js");
const { assertBrowserPrecision } = await import("../browser/src/utils/browser-reactor.js");
const { csoundTableCopyIn, csoundTableCopyOut, csoundGetTableArgs } =
  await import("../browser/src/modules/table.js");
const { csoundEvent } = await import("../browser/src/modules/control-events.js");
const shared = process.argv[3] === "shared";
const module = await WebAssembly.compile(fs.readFileSync(process.argv[2]));
const memory = new WebAssembly.Memory({ initial: 4096, maximum: shared ? 4096 : 16384, shared });
const wasi = new WASI();
wasi.setMemory(memory);
const imports = wasi.getImports(module);
imports.wasi = { "thread-spawn": () => -6 };
imports.env = {
  memory,
  __memory_base: new WebAssembly.Global({ value: "i32" }, 0),
  __table_base: new WebAssembly.Global({ value: "i32" }, 1),
  csoundWasiJsSupportsThreads: () => 0,
  csoundLoadModules: () => 0,
  csoundLoadExternals: () => 0,
  csoundWasiJsMessageCallback() {},
  csoundWasiApiComplete() {},
  csoundWasiJsSystem: () => -1,
  csoundWasiJsReadlineCallback() {},
  csoundWasiJsDebugCallback() {},
  printDebugCallback() {},
};
const instance = await WebAssembly.instantiate(module, imports);
assertBrowserPrecision(instance.exports);
wasi.start(instance);
const api = instance.exports;
const wasm = { exports: api, wasi };
const csound = api.csoundCreateWasi();
assert.equal(typeof csound, "number", "wasm32 pointers remain 32-bit values");
assert(csound);
function stringCall(name, text, ...args) {
  const bytes = new TextEncoder().encode(text);
  const pointer = api.allocStringMem(bytes.length);
  assert(pointer);
  try {
    new Uint8Array(memory.buffer, pointer, bytes.length).set(bytes);
    return api[name](csound, pointer, ...args);
  } finally {
    api.freeStringMem(pointer);
  }
}
try {
  const encode = (text) => new TextEncoder().encode(text);
  wasi.writeFile("/root.orc", encode("giRoot init 1"));
  wasi.mkdir("/project");
  wasi.writeFile("/project/relative.orc", encode("giRelative init 2"));
  assert.equal(wasi.chdir("/project"), 0);
  assert.equal(stringCall("csoundCompileOrc", '#include "/root.orc"\n#include "relative.orc"', 0), 0);
  assert.equal(wasi.chdir("/"), 0);
  api.csoundResetWasi(csound);
  assert.equal(stringCall("csoundSetOption", "-n -m0"), 0);
  assert.equal(stringCall("csoundCompileOrc", `
sr=48000
ksmps=32
nchnls=1
0dbfs=1
giTable ftgen 1, 0, 8, -2, 0
instr 1
  a1 = p4
  out a1
endin`, 0), 0);
  assert.equal(api.csoundStartWasi(csound), 0);
  const values = new Float32Array([0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8]);
  csoundTableCopyIn(wasm)(csound, 1, values);
  assert.deepEqual(csoundTableCopyOut(wasm)(csound, 1), values);
  assert(csoundGetTableArgs(wasm)(csound, 1) instanceof Float32Array);
  assert.equal(csoundGetTableArgs(wasm)(csound, 999), undefined);
  csoundEvent(wasm)(csound, 0, [1, 0, 1, 0.125]);
  // The asynchronous event enters the next control block.
  assert.equal(api.csoundPerformKsmps(csound), 0);
  assert.equal(api.csoundPerformKsmps(csound), 0);
  const samples = new Float32Array(memory.buffer, api.csoundGetSpout(csound), 32);
  assert.deepEqual(samples, new Float32Array(32).fill(0.125));
} finally {
  api.csoundDestroy(csound);
}
console.log(`Float WASI ${shared ? "shared" : "baseline"} audio, tables, and events passed`);
