/* eslint-env mocha */

import assert from "node:assert/strict";

import {
  applyAudioFade,
  createAudioFade,
  fillAudioFade,
  getAudioFadeRemainingFrames,
} from "../../src/utils/audio-fade.js";

describe("audio fade", () => {
  it("fades every channel once per frame", () => {
    const fade = createAudioFade(4);
    const channels = [
      new Float64Array([1, 1]),
      new Float64Array([1, 1]),
    ];

    applyAudioFade(fade, channels);

    assert.deepEqual([...channels[0]], [1, 2 / 3]);
    assert.deepEqual([...channels[1]], [1, 2 / 3]);
    assert.equal(getAudioFadeRemainingFrames(fade), 2);
  });

  it("continues the ramp across output buffers", () => {
    const fade = createAudioFade(4);
    const first = [new Float64Array([1, 1])];
    const second = [new Float64Array([1, 1])];

    applyAudioFade(fade, first);
    applyAudioFade(fade, second);

    assert.deepEqual([...first[0]], [1, 2 / 3]);
    assert.deepEqual([...second[0]], [1 / 3, 0]);
    assert.equal(getAudioFadeRemainingFrames(fade), 0);
  });

  it("fades a held final sample to silence", () => {
    const fade = createAudioFade(3);
    const output = [new Float64Array(3)];

    fillAudioFade(fade, output, [1]);

    assert.deepEqual([...output[0]], [1, 0.5, 0]);
  });
});
