// Check the real C reactor without starting its background workers.
import fs from "node:fs";
import assert from "node:assert/strict";

// Source files use Closure constants; the standalone check uses their defaults.
globalThis.goog = { define: (_name, value) => value, global: globalThis };
const { WASI } = await import("../../browser/src/filesystem/wasi.js");
const module = await WebAssembly.compile(fs.readFileSync(process.argv[2]));
const maximum =
  Number(fs.readFileSync(new URL("../../src/shared-memory-max.txt", import.meta.url), "utf8")) /
  65536;
const { assertBrowserReactor } = await import("../../browser/src/utils/browser-reactor.js");
assertBrowserReactor(module);
const memory = new WebAssembly.Memory({ initial: maximum, maximum, shared: true });
const wasi = new WASI();
wasi.setMemory(memory);
let hostSupportsThreads = true;
let threadSpawns = 0;
const imports = wasi.getImports(module);
imports.wasi = {
  "thread-spawn": () => {
    threadSpawns++;
    return -6;
  },
};
imports.env = {
  memory,
  csoundWasiJsSupportsThreads: () => Number(hostSupportsThreads),
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
const compileCsd = () =>
  stringCall(
    "csoundCompileCSD",
    `<CsoundSynthesizer>
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
</CsoundSynthesizer>`,
    1,
    0,
  );
assert.equal(compileCsd(), 0);
assert.equal(api.csoundIsRealtime(csound), 1);
api.csoundResetWasi(csound);
hostSupportsThreads = false;
assert.equal(compileCsd(), 0);
assert.equal(api.csoundIsRealtime(csound), 0);
api.csoundResetWasi(csound);
hostSupportsThreads = true;

// Exercise overload lookup in the fixed-memory reactor, including the
// empty signatures that previously read before the required-type array.
const signatureCsd = fs
  .readFileSync(new URL("../../../../tests/commandline/test_opcode_empty_signatures.csd", import.meta.url), "utf8")
  .replace(/<CsTest>[\s\S]*?<\/CsTest>/, "");
api.csoundCreateMessageBuffer(csound, 0);
assert.equal(stringCall("csoundCompileCSD", signatureCsd, 1, 0), 0);
assert.equal(api.csoundStartWasi(csound), 0);
let signatureStatus = 0;
for (let block = 0; block < 100 && signatureStatus === 0; block++) {
  signatureStatus = api.csoundPerformKsmps(csound);
}
assert(signatureStatus > 0, "signature score did not finish");
assert.equal(api.csoundCleanupWasi(csound), 0);
let signatureMessages = "";
while (api.csoundGetMessageCnt(csound) > 0) {
  const pointer = api.csoundGetFirstMessage(csound);
  const bytes = new Uint8Array(memory.buffer);
  signatureMessages += new TextDecoder().decode(bytes.subarray(pointer, bytes.indexOf(0, pointer)));
  api.csoundPopFirstMessage(csound);
}
assert.match(signatureMessages, /Total assertions: 3/);
assert.match(signatureMessages, /Failed: 0/);
api.csoundDestroyMessageBuffer(csound);
api.csoundResetWasi(csound);

// Global declarations run inside start; pre-start API writes also survive it.
// Live access will use only these stable addresses, not the channel hash table.
const { createRealtimeControlChannels } =
  await import("../../browser/src/utils/realtime-control-channels.js");
const runtime = { exports: api, wasi: { memory } };
assert.equal(
  compile(
    'chn_k "gain", 3\nchn_k "input", 1\nchn_k "output", 2\nchn_S "text", 3\nchn_a "audio", 3',
  ),
  0,
);
function channelType(name) {
  const bytes = new TextEncoder().encode(name);
  const namePointer = api.allocStringMem(bytes.length);
  const valuePointer = api.allocByteMem(4);
  assert(namePointer && valuePointer);
  try {
    new Uint8Array(memory.buffer, namePointer, bytes.length).set(bytes);
    // Type zero returns the existing type without changing its direction.
    return api.csoundGetChannelPtr(csound, valuePointer, namePointer, 0);
  } finally {
    api.freeStringMem(namePointer);
    api.freeByteMem(valuePointer);
  }
}
stringCall("csoundSetControlChannel", "preset", 0.25);
assert.equal(stringCall("csoundSetOption", "-n -m0"), 0);
assert.equal(api.csoundStartWasi(csound), 0);
const declaredTypes = ["input", "output", "gain"].map(channelType);
assert.deepEqual(declaredTypes, [17, 33, 49]); // control + input/output flags
const controls = createRealtimeControlChannels(runtime, csound);
assert.deepEqual(["input", "output", "gain"].map(channelType), declaredTypes);
controls.set("input", 0.375);
assert.equal(controls.get("input"), 0.375);
controls.set("output", 0.625);
assert.equal(controls.get("output"), 0.625);
assert.deepEqual(["input", "output", "gain"].map(channelType), declaredTypes);
controls.set("gain", 0.5);
assert.equal(stringCall("csoundGetControlChannelWasi", "gain"), 0.5);
assert.equal(controls.get("preset"), 0.25);
stringCall("csoundSetControlChannel", "gain", 0.75);
assert.equal(controls.get("gain"), 0.75);
assert.equal(api.csoundCleanupWasi(csound), 0);
assert.equal(api.csoundCleanupWasi(csound), 0);
assert.equal(stringCall("csoundGetControlChannelWasi", "gain"), 0.75);
for (const name of ["text", "audio", "unknown"])
  assert.throws(() => controls.get(name), /before start/);
api.csoundResetWasi(csound);
assert.throws(() => createRealtimeControlChannels(runtime, csound).get("gain"), /before start/);

assert.notEqual(compile("instr 1\nsyntax nonsense @@@\nendin"), 0);
assert.equal(
  compile("sr=48000\nksmps=32\nnchnls=1\n0dbfs=1\ninstr 1\na1 oscili 0.1,440\nout a1\nendin"),
  0,
);

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

function fillMidiQueue(target = csound) {
  for (let index = 0; index < 1024; ++index)
    assert.equal(api.tryPushMidiMessage(target, 144, index % 128, 100), 1);
  assert.equal(api.tryPushMidiMessage(target, 128, 60, 0), 0);
}

fillMidiQueue();
const second = api.csoundCreateWasi();
assert(second);
assert.equal(api.tryPushMidiMessage(csound, 128, 60, 0), 0);
fillMidiQueue(second);
api.csoundResetWasi(second);
assert.equal(api.tryPushMidiMessage(csound, 128, 60, 0), 0);
fillMidiQueue(second);
api.csoundDestroy(second);
assert.equal(api.tryPushMidiMessage(csound, 128, 60, 0), 0);
api.csoundResetWasi(csound);
fillMidiQueue();
api.csoundDestroy(csound);
csound = api.csoundCreateWasi();
assert(csound);
fillMidiQueue();
api.csoundDestroy(csound);
console.log("WASI browser runtime mode, recovery, and MIDI ownership/reset checks passed");
