/* eslint-env mocha */
import assert from "node:assert/strict";
import {
  AUDIO_STATE,
  MIDI_BUFFER_SIZE,
  MIDI_BUFFER_PAYLOAD_SIZE,
  initialSharedState,
} from "../../src/constants.js";
import {
  enqueueMidiPacket,
  drainMidiPackets,
  resetMidiQueue,
} from "../../src/utils/sab-midi-queue.js";
import { tryPushMidiMessage } from "../../src/utils/wasm-midi.js";

const queue = (position = 0) => {
  const state = new Int32Array(new SharedArrayBuffer(initialSharedState.length * 4));
  state.set(initialSharedState);
  Atomics.store(state, AUDIO_STATE.RTMIDI_READ, position);
  Atomics.store(state, AUDIO_STATE.RTMIDI_WRITE, position);
  const buffer = new Int32Array(
    new SharedArrayBuffer(MIDI_BUFFER_SIZE * MIDI_BUFFER_PAYLOAD_SIZE * 4),
  );
  return { state, buffer };
};

const read = ({ state, buffer }) => {
  const packets = [];
  assert.equal(
    drainMidiPackets(state, buffer, (...packet) => packets.push(packet)),
    packets.length,
  );
  return packets;
};

describe("SAB MIDI packets", () => {
  it("discards the old run before accepting new startup packets", () => {
    const q = queue(-3);
    enqueueMidiPacket(q.state, q.buffer, 144, 60, 100);
    resetMidiQueue(q.state);
    assert.deepEqual(read(q), []);
    assert.equal(enqueueMidiPacket(q.state, q.buffer, 128, 60, 0), true);
    assert.deepEqual(read(q), [[128, 60, 0]]);
  });

  it("keeps all 1024 complete packets and rejects a full queue without overwriting", () => {
    const q = queue();
    const expected = [];
    for (let index = 0; index < MIDI_BUFFER_SIZE; ++index) {
      const packet = [144, index % 128, 100];
      assert.equal(enqueueMidiPacket(q.state, q.buffer, ...packet), true);
      expected.push(packet);
    }
    const snapshot = q.buffer.slice();
    assert.equal(enqueueMidiPacket(q.state, q.buffer, 128, 60, 0), false);
    assert.deepEqual(q.buffer, snapshot);
    assert.deepEqual(read(q), expected);
    assert.deepEqual(read(q), []);
    assert.equal(enqueueMidiPacket(q.state, q.buffer, 128, 60, 0), true);
    assert.deepEqual(read(q), [[128, 60, 0]]);
  });

  it("keeps a packet appended during a drain for the next drain", () => {
    const q = queue();
    enqueueMidiPacket(q.state, q.buffer, 144, 60, 100);
    const first = [];
    assert.equal(
      drainMidiPackets(q.state, q.buffer, (...packet) => {
        first.push(packet);
        assert.equal(enqueueMidiPacket(q.state, q.buffer, 144, 61, 101), true);
      }),
      1,
    );
    assert.deepEqual(first, [[144, 60, 100]]);
    assert.deepEqual(read(q), [[144, 61, 101]]);
  });

  it("retains full/empty and packet order across uint32 and ring wrap", () => {
    const q = queue(-3);
    for (let round = 0; round < 3; ++round) {
      const expected = [];
      for (let index = 0; index < MIDI_BUFFER_SIZE; ++index) {
        const packet = [144 + round, index % 128, 100];
        assert.equal(enqueueMidiPacket(q.state, q.buffer, ...packet), true);
        expected.push(packet);
      }
      assert.equal(enqueueMidiPacket(q.state, q.buffer, 255), false);
      assert.deepEqual(read(q), expected);
    }
    assert.equal(Atomics.load(q.state, AUDIO_STATE.RTMIDI_READ), 3 * MIDI_BUFFER_SIZE - 3);
    assert.equal(Atomics.load(q.state, AUDIO_STATE.RTMIDI_WRITE), 3 * MIDI_BUFFER_SIZE - 3);
  });

  it("does not release a full slot before its callback consumes it", () => {
    const q = queue();
    for (let index = 0; index < MIDI_BUFFER_SIZE; ++index)
      enqueueMidiPacket(q.state, q.buffer, 144, index % 128, 100);
    let count = 0;
    drainMidiPackets(q.state, q.buffer, (status, note, velocity) => {
      if (count === 0) assert.equal(enqueueMidiPacket(q.state, q.buffer, 128, 60, 0), false);
      if (count === 1) assert.equal(enqueueMidiPacket(q.state, q.buffer, 128, 60, 0), true);
      assert.deepEqual([status, note, velocity], [144, count++ % 128, 100]);
    });
    assert.equal(count, MIDI_BUFFER_SIZE);
    assert.deepEqual(read(q), [[128, 60, 0]]);
  });

  it("retains the next packet when the C queue is full", () => {
    const q = queue();
    enqueueMidiPacket(q.state, q.buffer, 144, 60, 100);
    enqueueMidiPacket(q.state, q.buffer, 128, 60, 0);
    const accepted = [];
    const wasm = {
      exports: {
        tryPushMidiMessage: (_, ...packet) => {
          if (accepted.length) return 0;
          accepted.push(packet);
          return 1;
        },
      },
    };
    assert.equal(
      drainMidiPackets(q.state, q.buffer, (...packet) => tryPushMidiMessage(wasm, 1, ...packet)),
      1,
    );
    assert.deepEqual(accepted, [[144, 60, 100]]);
    assert.deepEqual(read(q), [[128, 60, 0]]);
  });

  it("keeps the legacy void MIDI shim usable", () => {
    const calls = [];
    const wasm = { exports: { pushMidiMessage: (...args) => calls.push(args) } };
    assert.equal(tryPushMidiMessage(wasm, 1, 144, 60, 100), true);
    assert.deepEqual(calls, [[1, 144, 60, 100]]);
  });
});
