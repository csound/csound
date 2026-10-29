/* eslint-env mocha */
import assert from "node:assert/strict";
import { MAX_CHANNELS, RING_BUFFER_SIZE } from "../../src/constants.js";
import { sharedAudioChannels } from "../../src/utils/sab-audio-channels.js";

describe("SAB audio channel storage", () => {
  it("keeps every channel separate across the full ring capacity", () => {
    const buffer = new SharedArrayBuffer(
      MAX_CHANNELS * RING_BUFFER_SIZE * Float32Array.BYTES_PER_ELEMENT,
    );
    const writer = sharedAudioChannels(buffer, MAX_CHANNELS);
    const reader = sharedAudioChannels(buffer, MAX_CHANNELS);
    for (let channel = 0; channel < MAX_CHANNELS; ++channel) writer[channel].fill(channel + 0.5);
    for (let channel = 0; channel < MAX_CHANNELS; ++channel) {
      for (let frame = 0; frame < RING_BUFFER_SIZE; ++frame)
        assert.equal(reader[channel][frame], channel + 0.5);
    }
    // The former byte offset made channel 1 frame 0 alias channel 0 frame 2048.
    writer[1][0] = -1;
    assert.equal(reader[1][0], -1);
    assert.equal(reader[0][RING_BUFFER_SIZE / Float32Array.BYTES_PER_ELEMENT], 0.5);
  });

  it("uses separate input and output rings and accepts no input channels", () => {
    const bytes = 2 * RING_BUFFER_SIZE * Float32Array.BYTES_PER_ELEMENT;
    const input = sharedAudioChannels(new SharedArrayBuffer(bytes), 2);
    const output = sharedAudioChannels(new SharedArrayBuffer(bytes), 2);
    input[0][0] = 0.25;
    output[1][RING_BUFFER_SIZE - 1] = 0.75;
    assert.equal(output[0][0], 0);
    assert.equal(input[1][RING_BUFFER_SIZE - 1], 0);
    assert.deepEqual(sharedAudioChannels(new SharedArrayBuffer(bytes), 0), []);
  });
});
