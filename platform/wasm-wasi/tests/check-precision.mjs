import assert from "node:assert/strict";
import fs from "node:fs";

globalThis.goog = { define: (_name, value) => value, global: globalThis };
const { WASI } = await import("../browser/src/filesystem/preview2.js");
const { assertBrowserPrecision } = await import("../browser/src/utils/browser-reactor.js");
const { csoundTableCopyIn, csoundTableCopyOut, csoundGetTableArgs } =
  await import("../browser/src/modules/table.js");
const { csoundEvent } = await import("../browser/src/modules/control-events.js");
const { csoundGetParams } = await import("../browser/src/modules/attributes.js");
const { CS_MIDIDEVICE } = await import("../browser/src/structures.js");
const { sizeOfStruct } = await import("../browser/src/utils/native-sizes.js");
const { moduleABI, memory64Imports, nativeExports, nativeCallbacks } =
  await import("../browser/src/utils/wasm64.js");
const shared = process.argv[3] === "shared";
const module = await WebAssembly.compile(fs.readFileSync(process.argv[2]));
const abi = moduleABI(module);
const wide = abi?.pointerSize === 8;
const Samples = wide ? Float64Array : Float32Array;
const memory = new WebAssembly.Memory(
  wide
    ? { address: "i64", initial: 4096n, maximum: 131072n }
    : { initial: 4096, maximum: shared ? 4096 : 16384, shared },
);
const wasi = new WASI();
wasi.setMemory(memory);
const bridge = wide ? memory64Imports(module, memory, wasi) : undefined;
const imports = bridge ? bridge.imports : wasi.getImports(module);
imports.wasi = { "thread-spawn": () => -6 };
imports.env = {
  memory,
  __memory_base: new WebAssembly.Global({ value: wide ? "i64" : "i32" }, wide ? 0n : 0),
  __table_base: new WebAssembly.Global(
    { value: abi?.table64 ? "i64" : "i32" },
    abi?.table64 ? 1n : 1,
  ),
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
nativeCallbacks(imports, abi);
const raw = await WebAssembly.instantiate(module, imports);
bridge?.connect(raw);
const instance = { exports: nativeExports(raw.exports, abi) };
assertBrowserPrecision(instance.exports, wide ? 8 : 4);
wasi.start(instance);
const api = instance.exports;
const wasm = { exports: api, wasi };
const csound = api.csoundCreateWasi();
assert.equal(typeof csound, "number", "the JS API exposes exact numeric pointers");
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
  assert.equal(sizeOfStruct(CS_MIDIDEVICE, wasm), api.sizeOfMidiStruct());
  assert.equal(
    stringCall(
      "csoundSetOption",
      "-b512 -B2048 -m300 -t123.5 --sample-rate=48000.5 --control-rate=1500.25 " +
        "--nchnls=3 --nchnls_i=2 --0dbfs=2048 --midi-key=73 --midi-velocity=74 --fftlib=2",
    ),
    0,
  );
  const parameters = csoundGetParams(wasm)(csound);
  for (const [name, value] of Object.entries({
    buffer_frames: 512,
    hardware_buffer_frames: 2048,
    message_level: 300,
    tempo: 123.5,
    sample_rate_override: 48000.5,
    control_rate_override: 1500.25,
    nchnls_override: 3,
    nchnls_i_override: 2,
    e0dbfs_override: 2048,
    midi_key: 73,
    midi_velocity: 74,
    FFT_library: 2,
    use_cscore: 0,
    compute_weights: 0,
  }))
    assert.equal(parameters[name], value, `getParams ${name}`);
  assert.equal(stringCall("csoundSetOption", "--0dbfs=1024"), 0);
  assert.equal(csoundGetParams(wasm)(csound).e0dbfs_override, 1024);
  assert.equal(parameters.e0dbfs_override, 2048, "getParams returns a copy");
  api.csoundResetWasi(csound);
  const encode = (text) => new TextEncoder().encode(text);
  wasi.writeFile("/root.orc", encode("giRoot init 1"));
  wasi.mkdir("/project");
  wasi.writeFile("/project/relative.orc", encode("giRelative init 2"));
  assert.equal(wasi.chdir("/project"), 0);
  assert.equal(
    stringCall("csoundCompileOrc", '#include "/root.orc"\n#include "relative.orc"', 0),
    0,
  );
  assert.equal(wasi.chdir("/"), 0);
  api.csoundResetWasi(csound);
  assert.equal(stringCall("csoundSetOption", "-n -m0"), 0);
  assert.equal(
    stringCall(
      "csoundCompileOrc",
      `
sr=48000
ksmps=32
nchnls=1
0dbfs=1
giTable ftgen 1, 0, 8, -2, 0
instr 1
  a1 = p4
  out a1
endin`,
      0,
    ),
    0,
  );
  assert.equal(api.csoundStartWasi(csound), 0);
  const values = new Samples([0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8]);
  csoundTableCopyIn(wasm)(csound, 1, values);
  assert.deepEqual(csoundTableCopyOut(wasm)(csound, 1), values);
  assert(csoundGetTableArgs(wasm)(csound, 1) instanceof Samples);
  assert.equal(csoundGetTableArgs(wasm)(csound, 999), undefined);
  csoundEvent(wasm)(csound, 0, [1, 0, 1, 0.125]);
  // The asynchronous event enters the next control block.
  assert.equal(api.csoundPerformKsmps(csound), 0);
  assert.equal(api.csoundPerformKsmps(csound), 0);
  const samples = new Samples(memory.buffer, api.csoundGetSpout(csound), 32);
  assert.deepEqual(samples, new Samples(32).fill(0.125));
} finally {
  api.csoundDestroy(csound);
}
console.log(
  `${wide ? "Double wasm64" : "Float wasm32"} WASI ${shared ? "shared" : "baseline"} audio, tables, and events passed`,
);
