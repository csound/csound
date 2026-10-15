import { AUDIO_STATE, MIDI_BUFFER_SIZE, MIDI_BUFFER_PAYLOAD_SIZE } from "../constants.js";

// Only the producer resets this pair, before a render worker starts consuming.
export const resetMidiQueue = (state) => {
  Atomics.store(state, AUDIO_STATE.RTMIDI_READ, 0);
  Atomics.store(state, AUDIO_STATE.RTMIDI_WRITE, 0);
};

// One main-thread producer and one render-worker consumer. Counters count
// packets, including across uint32 wrap; only the owner advances its counter.
export const enqueueMidiPacket = (state, buffer, status, data1 = 0, data2 = 0) => {
  const write = Atomics.load(state, AUDIO_STATE.RTMIDI_WRITE) >>> 0;
  const read = Atomics.load(state, AUDIO_STATE.RTMIDI_READ) >>> 0;
  if ((write - read) >>> 0 >= MIDI_BUFFER_SIZE) return false;
  const offset = (write % MIDI_BUFFER_SIZE) * MIDI_BUFFER_PAYLOAD_SIZE;
  buffer[offset] = status;
  buffer[offset + 1] = data1;
  buffer[offset + 2] = data2;
  // Publish only after all three words are ready.
  Atomics.store(state, AUDIO_STATE.RTMIDI_WRITE, write + 1);
  return true;
};

export const drainMidiPackets = (state, buffer, consume) => {
  let read = Atomics.load(state, AUDIO_STATE.RTMIDI_READ) >>> 0;
  const end = Atomics.load(state, AUDIO_STATE.RTMIDI_WRITE) >>> 0;
  const count = (end - read) >>> 0;
  // Snapshot admission so messages appended during this drain wait for the next.
  let consumed = 0;
  for (; consumed < count; ++consumed) {
    const offset = (read % MIDI_BUFFER_SIZE) * MIDI_BUFFER_PAYLOAD_SIZE;
    if (consume(buffer[offset], buffer[offset + 1], buffer[offset + 2]) === false) break;
    read = (read + 1) >>> 0;
    // The producer can reuse this slot only after the callback consumed it.
    Atomics.store(state, AUDIO_STATE.RTMIDI_READ, read);
  }
  return consumed;
};
