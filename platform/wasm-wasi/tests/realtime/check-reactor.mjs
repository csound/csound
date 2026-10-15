// Check the real C reactor without starting its background workers.
import fs from "node:fs";
import assert from "node:assert/strict";

// Source files use Closure constants; the standalone check uses their defaults.
globalThis.goog = { define: (_name, value) => value, global: globalThis };
const { WASI } = await import("../../browser/src/filesystem/wasi.js");
const module = await WebAssembly.compile(fs.readFileSync(process.argv[2]));
const memory = new WebAssembly.Memory({ initial: 256, maximum: 4096, shared: true });
const wasi = new WASI();
wasi.setMemory(memory);
let hostSupportsThreads = true;
let threadSpawns = 0;
const imports = wasi.getImports(module);
imports.wasi = { "thread-spawn": () => { threadSpawns++; return -6; } };
imports.env = {
  memory,
  csoundWasiJsSupportsThreads: () => Number(hostSupportsThreads),
  csoundLoadModules: () => 0,
  csoundLoadExternals: () => 0,
  csoundWasiJsMessageCallback() {},
  csoundWasiJsSystem: () => -1,
  csoundWasiJsReadlineCallback() {},
  csoundWasiJsDebugCallback() {},
  printDebugCallback() {},
};
const instance = await WebAssembly.instantiate(module, imports);
wasi.start(instance);
const api = instance.exports;
let csound = api.csoundCreateWasi();
assert(csound);
assert.equal(api.csoundSupportsRealtime(), 1);
assert.equal(api.csoundIsRealtime(csound), 0);

function stringCall(name, source, ...arguments_) {
  const bytes = new TextEncoder().encode(source);
  const pointer = api.allocStringMem(bytes.length);
  assert(pointer);
  new Uint8Array(memory.buffer, pointer, bytes.length).set(bytes);
  const status = api[name](csound, pointer, ...arguments_);
  api.freeStringMem(pointer);
  return status;
}

const compile = (source) => stringCall("csoundCompileOrc", source, 0);
hostSupportsThreads = false;
assert.equal(api.csoundSupportsRealtime(), 0);
assert.equal(stringCall("csoundSetOption", "--realtime"), 0);
assert.equal(api.csoundIsRealtime(csound), 0);
hostSupportsThreads = true;
assert.equal(api.csoundSupportsRealtime(), 1);
assert.equal(stringCall("csoundSetOption", "--realtime"), 0);
assert.equal(api.csoundIsRealtime(csound), 1);
api.csoundResetWasi(csound);
assert.equal(api.csoundIsRealtime(csound), 0);
const compileCsd = () => stringCall("csoundCompileCSD", `<CsoundSynthesizer>
<CsOptions>
-n -d --realtime
</CsOptions>
<CsInstruments>
sr=48000
ksmps=32
nchnls=1
0dbfs=1
instr 1
endin
</CsInstruments>
<CsScore>
f0 1
</CsScore>
</CsoundSynthesizer>`, 1, 0);
assert.equal(compileCsd(), 0);
assert.equal(api.csoundIsRealtime(csound), 1);
api.csoundResetWasi(csound);
hostSupportsThreads = false;
assert.equal(compileCsd(), 0);
assert.equal(api.csoundIsRealtime(csound), 0);
api.csoundResetWasi(csound);
hostSupportsThreads = true;

assert.notEqual(compile("instr 1\nsyntax nonsense @@@\nendin"), 0);
assert.equal(compile("sr=48000\nksmps=32\nnchnls=1\n0dbfs=1\ninstr 1\na1 oscili 0.1,440\nout a1\nendin"), 0);

// Check raw start as well as the WASI wrapper: a preset native flag cannot
// start background workers on a host which has no pool.
assert.equal(stringCall("csoundSetOption", "-n"), 0);
assert.equal(stringCall("csoundSetOption", "--realtime"), 0);
assert.equal(api.csoundIsRealtime(csound), 1);
hostSupportsThreads = false;
assert.equal(api.csoundStart(csound), 0);
assert.equal(api.csoundIsRealtime(csound), 0);
assert.equal(threadSpawns, 0);
api.csoundResetWasi(csound);

function fillMidiQueue() {
  for (let index = 0; index < 1024; ++index)
    assert.equal(api.tryPushMidiMessage(csound, 144, index % 128, 100), 1);
  assert.equal(api.tryPushMidiMessage(csound, 128, 60, 0), 0);
}

fillMidiQueue();
api.csoundResetWasi(csound);
fillMidiQueue();
api.csoundDestroy(csound);
csound = api.csoundCreateWasi();
assert(csound);
fillMidiQueue();
api.csoundDestroy(csound);
console.log("WASI browser runtime mode, recovery, and MIDI reset checks passed");
