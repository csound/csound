#!/usr/bin/env node
"use strict";

// Baseline bug reproductions for Csound commit
// 9d6f33d345e5691a70de776973e76415357b50a0.
//
// These assertions confirm the defects described in the study. They are NOT a
// regression suite whose success means the queues work correctly. Once a defect
// changes, update or retire its reproduction. These probes do not run browser
// audio or measure latency, throughput, or the frequency of a racing schedule.
//
// Supply browser sources from the baseline commit; current sources fix several
// of these defects and have their own regression tests:
//   node browser-queues.cjs /path/to/platform/wasm-wasi/browser/src

const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");

assert.equal(process.argv.length, 3,
  "Usage: node browser-queues.cjs /path/to/baseline/platform/wasm-wasi/browser/src");
const sourceDirectory = path.resolve(process.argv[2]);

function readSource(relativePath) {
  return fs.readFileSync(path.join(sourceDirectory, relativePath), "utf8");
}

function sourceChanged(label) {
  return `Baseline source changed: ${label}. Update or retire this reproduction.`;
}

function extractBetween(source, start, end, label) {
  const startIndex = source.indexOf(start);
  assert.notEqual(startIndex, -1, sourceChanged(`${label}: missing start marker`));
  assert.equal(source.indexOf(start, startIndex + start.length), -1,
    sourceChanged(`${label}: ambiguous start marker`));
  const endIndex = source.indexOf(end, startIndex + start.length);
  assert.notEqual(endIndex, -1, sourceChanged(`${label}: missing end marker`));
  return source.slice(startIndex, endIndex);
}

const withoutWhitespace = (source) => source.replace(/\s+/g, "");

function requireFragment(source, fragment, label) {
  const normalizedSource = withoutWhitespace(source);
  const normalizedFragment = withoutWhitespace(fragment);
  const index = normalizedSource.indexOf(normalizedFragment);
  assert.notEqual(index, -1, sourceChanged(label));
  assert.equal(normalizedSource.indexOf(normalizedFragment, index + normalizedFragment.length), -1,
    sourceChanged(`${label}: ambiguous fragment`));
}

const constantsSource = readSource("constants.js");
const {
  AUDIO_STATE,
  MIDI_BUFFER_SIZE,
  MIDI_BUFFER_PAYLOAD_SIZE,
  RING_BUFFER_SIZE,
  MAX_CHANNELS,
  initialSharedState,
} = new Function(
  constantsSource.replaceAll("export const ", "const ") +
    "; return { AUDIO_STATE, MIDI_BUFFER_SIZE, MIDI_BUFFER_PAYLOAD_SIZE," +
    " RING_BUFFER_SIZE, MAX_CHANNELS, initialSharedState };",
)();

assert.equal(MIDI_BUFFER_SIZE, 1024, sourceChanged("MIDI capacity"));
assert.equal(MIDI_BUFFER_PAYLOAD_SIZE, 3, sourceChanged("MIDI payload size"));
assert.equal(RING_BUFFER_SIZE, 16384, sourceChanged("audio ring capacity"));
assert.equal(MAX_CHANNELS, 32, sourceChanged("audio channel capacity"));

const mainSource = readSource("mains/sab.main.js");
const workerSource = readSource("workers/sab.worker.js");
const workletSource = readSource("workers/worklet.worker.js");

// Execute the enqueue method from the source, without loading its browser imports.
const enqueueMethod = extractBetween(
  mainSource,
  "  handleMidiInput(",
  "  async csoundPause(",
  "SAB MIDI enqueue method",
);
const enqueue = new Function(
  "AUDIO_STATE", "MIDI_BUFFER_SIZE", "MIDI_BUFFER_PAYLOAD_SIZE",
  `return ({ ${enqueueMethod} }).handleMidiInput;`,
)(AUDIO_STATE, MIDI_BUFFER_SIZE, MIDI_BUFFER_PAYLOAD_SIZE);

function makeMidiHost() {
  return {
    audioStatePointer: new Int32Array(new SharedArrayBuffer(
      initialSharedState.length * Int32Array.BYTES_PER_ELEMENT,
    )),
    midiBuffer: new Int32Array(new SharedArrayBuffer(
      MIDI_BUFFER_SIZE * MIDI_BUFFER_PAYLOAD_SIZE * Int32Array.BYTES_PER_ELEMENT,
    )),
  };
}

console.log("BASELINE BUG REPRODUCTIONS — success confirms known defects, not correct queues.");
console.log(`Browser source: ${sourceDirectory}`);

// Reproduction 1: payload starts wrap after 1,024 words, not 1,024 events.
{
  const host = makeMidiHost();
  for (let index = 0; index < 343; index += 1) {
    enqueue.call(host, { data: [144, index % 128, 100] });
  }
  const firstPayload = Array.from(host.midiBuffer.slice(0, 3));
  assert.equal(host.midiBuffer.length, 3072);
  assert.equal(Atomics.load(host.audioStatePointer, AUDIO_STATE.AVAIL_RTMIDI_EVENTS), 343);
  assert.deepEqual(firstPayload, [144, 0, 144], "Reproduce first-payload corruption");
  console.log("1. After 343 enqueues: first payload [144, 0, 144]; correct payload [144, 0, 100].");
}

// Reproduction 2 models one possible interleaving, not a timed race. Guard the
// full source consumer block before reproducing its snapshot and publication.
const consumerBlock = extractBetween(
  workerSource,
  "      if (isRequestingRtMidiInput) {",
  "      const bufferLength =",
  "SAB MIDI consumer block",
);
const expectedConsumerBlock = `
  if (isRequestingRtMidiInput) {
    const availableMidiEvents = Atomics.load(
      audioStatePointer,
      AUDIO_STATE.AVAIL_RTMIDI_EVENTS,
    );
    if (availableMidiEvents > 0) {
      const rtmidiBufferIndex = Atomics.load(audioStatePointer, AUDIO_STATE.RTMIDI_INDEX);
      let absIndex = rtmidiBufferIndex;
      for (let index = 0; index < availableMidiEvents; index++) {
        absIndex = (rtmidiBufferIndex + MIDI_BUFFER_PAYLOAD_SIZE * index) % MIDI_BUFFER_SIZE;
        const status = Atomics.load(midiBuffer, absIndex);
        const data1 = Atomics.load(midiBuffer, absIndex + 1);
        const data2 = Atomics.load(midiBuffer, absIndex + 2);
        libraryCsound["csoundPushMidiMessage"](csound, status, data1, data2);
      }
      Atomics.store(
        audioStatePointer,
        AUDIO_STATE.RTMIDI_INDEX,
        (absIndex + 1) % MIDI_BUFFER_SIZE,
      );
      Atomics.sub(audioStatePointer, AUDIO_STATE.AVAIL_RTMIDI_EVENTS, availableMidiEvents);
    }
  }
`;
assert.equal(withoutWhitespace(consumerBlock), withoutWhitespace(expectedConsumerBlock),
  sourceChanged("manual MIDI consumer model"));
{
  const host = makeMidiHost();
  enqueue.call(host, { data: [144, 60, 100] });
  const snapshotCount = Atomics.load(host.audioStatePointer, AUDIO_STATE.AVAIL_RTMIDI_EVENTS);
  const snapshotIndex = Atomics.load(host.audioStatePointer, AUDIO_STATE.RTMIDI_INDEX);
  assert.equal(snapshotCount, 1);
  assert.equal(snapshotIndex, 0);

  // Producer appends after the consumer snapshot, before the consumer commits.
  enqueue.call(host, { data: [144, 61, 101] });
  assert.deepEqual(Array.from(host.midiBuffer.slice(0, 3)), [144, 60, 100]);
  assert.deepEqual(Array.from(host.midiBuffer.slice(3, 6)), [144, 61, 101]);
  const lastReadIndex = (snapshotIndex + MIDI_BUFFER_PAYLOAD_SIZE * (snapshotCount - 1))
    % MIDI_BUFFER_SIZE;
  Atomics.store(host.audioStatePointer, AUDIO_STATE.RTMIDI_INDEX,
    (lastReadIndex + 1) % MIDI_BUFFER_SIZE);
  Atomics.sub(host.audioStatePointer, AUDIO_STATE.AVAIL_RTMIDI_EVENTS, snapshotCount);

  const nextIndex = Atomics.load(host.audioStatePointer, AUDIO_STATE.RTMIDI_INDEX);
  assert.equal(nextIndex, 1);
  assert.equal(Atomics.load(host.audioStatePointer, AUDIO_STATE.AVAIL_RTMIDI_EVENTS), 1);
  assert.deepEqual(Array.from(host.midiBuffer.slice(nextIndex, nextIndex + 3)), [60, 100, 144],
    "Reproduce the misaligned next MIDI read");
  console.log("2. Append during drain: next payload [60, 100, 144]; correct payload [144, 61, 101].");
}

// Reproduction 3 executes the exact refill decision from the source. It models
// ten decisions after startup with no available output and no packet replies.
{
  const refill = extractBetween(workletSource, "  // 2048 is max buffer", "\n  return true;",
    "vanilla audio refill block");
  const requestFrames = new Function("RING_BUFFER_SIZE", "nextOutputReadIndex", refill);
  const requests = [];
  const worklet = {
    vanillaAvailableFrames: 0,
    pendingFrames: 0,
    audioFramePort: { requestFrames(payload) { requests.push(payload); } },
  };
  for (let index = 0; index < 10; index += 1) {
    requestFrames.call(worklet, RING_BUFFER_SIZE, 0);
  }
  assert.deepEqual(requests.map(({ numFrames }) => numFrames), Array(10).fill(2048));
  assert.deepEqual(requests.map(({ readIndex }) => readIndex),
    [0, 2048, 4096, 6144, 8192, 10240, 12288, 14336, 0, 2048]);
  assert.equal(worklet.pendingFrames, 20480);
  console.log("3. Ten refill decisions without replies: 20,480 pending frames; ring capacity 16,384.");
}

// Reproduction 4 models channel views. Guard all four source constructors so
// this model cannot silently keep asserting an offset defect after a fix.
for (const direction of ["In", "Out"]) {
  requireFragment(workerSource,
    `new Float64Array(audioStream${direction}, RING_BUFFER_SIZE * channelIndex, RING_BUFFER_SIZE)`,
    `worker audio ${direction} channel stride`);
  requireFragment(workletSource,
    `new Float64Array(this.audioStream${direction}, RING_BUFFER_SIZE * channelIndex, RING_BUFFER_SIZE)`,
    `worklet audio ${direction} channel stride`);
}
{
  const audioMemory = new SharedArrayBuffer(
    MAX_CHANNELS * RING_BUFFER_SIZE * Float64Array.BYTES_PER_ELEMENT,
  );
  const channel0 = new Float64Array(audioMemory, 0, RING_BUFFER_SIZE);
  const channel1 = new Float64Array(audioMemory, RING_BUFFER_SIZE, RING_BUFFER_SIZE);
  assert.equal(channel1.byteOffset, 16384);
  assert.equal(RING_BUFFER_SIZE / Float64Array.BYTES_PER_ELEMENT, 2048);
  channel1[0] = 123;
  assert.equal(channel0[2048], 123, "Reproduce overlapping channel views");
  assert.equal(channel0[0], 0);
  console.log("4. Channel 1 frame 0 aliases channel 0 frame 2,048; channel windows overlap.");
}

// Separate Wasm contract check, not a Csound defect reproduction. This tiny
// module imports env.memory with an unshared minimum/maximum of one page.
{
  const moduleBytes = Uint8Array.from([
    0, 97, 115, 109, 1, 0, 0, 0,
    2, 16, 1, 3, 101, 110, 118, 6, 109, 101, 109, 111, 114, 121, 2, 1, 1, 1,
  ]);
  const module = new WebAssembly.Module(moduleBytes);
  new WebAssembly.Instance(module, {
    env: { memory: new WebAssembly.Memory({ initial: 1, maximum: 1 }) },
  });
  assert.throws(() => new WebAssembly.Instance(module, {
    env: { memory: new WebAssembly.Memory({ initial: 1, maximum: 1, shared: true }) },
  }), WebAssembly.LinkError);
  console.log("Wasm contract: an unshared memory import rejects shared memory.");
}

console.log("Buffer durations below are arithmetic only, not measured audio latency:");
console.table([128, 2048, 4096, 8192, 16384].map((frames) => ({
  frames,
  "ms at 44.1 kHz": (1000 * frames / 44100).toFixed(2),
  "ms at 48 kHz": (1000 * frames / 48000).toFixed(2),
})));
