/* eslint-env mocha */

import assert from "node:assert/strict";
import { AUDIO_STATE, initialSharedState } from "../../src/constants.js";

let handleCsoundStart;
let originalGoogDescriptor;

describe("worker performance mode", () => {
  before(async () => {
    originalGoogDescriptor = Object.getOwnPropertyDescriptor(globalThis, "goog");
    Object.defineProperty(globalThis, "goog", {
      configurable: true,
      writable: true,
      value: { define: () => true },
    });
    ({ handleCsoundStart } = await import("../../src/workers/common.utils.js"));
  });

  after(() => {
    if (originalGoogDescriptor) {
      Object.defineProperty(globalThis, "goog", originalGoogDescriptor);
    } else {
      delete globalThis.goog;
    }
  });

  for (const shared of [false, true]) {
    it(`announces an offline render only when ready (SAB: ${shared})`, async () => {
      const audioState = shared
        ? new Int32Array(new SharedArrayBuffer(initialSharedState.length * 4))
        : undefined;
      audioState?.set(initialSharedState);
      const events = [];
      const libraryCsound = {
        csoundShouldDaemonize: () => 0,
        csoundStart: () => 0,
        csoundGetOutputName: () => "recording.wav",
        isRequestingRtMidiInput: () => 0,
        isRequestingRtAudioInput: () => 0,
      };
      const workerMessagePort = {
        broadcastPlayState: (state, generation) => {
          // The main thread may handle this message before the renderer runs.
          events.push([
            state,
            generation,
            audioState ? Atomics.load(audioState, AUDIO_STATE.IS_RENDERING) : undefined,
          ]);
        },
      };
      const start = handleCsoundStart(
        workerMessagePort,
        libraryCsound,
        {},
        () => events.push("realtime"),
        () => events.push("render"),
      );

      assert.equal(
        start({ csound: 1, performanceGeneration: 7, audioStateBuffer: audioState?.buffer }),
        0,
      );
      await new Promise((resolve) => setTimeout(resolve, 0));

      assert.deepEqual(events, [["renderStarted", 7, shared ? 1 : undefined], "render"]);
    });
  }

  it("uses the real-time path for microphone input without DAC output", async () => {
    let realtimeStarted = false;
    let renderStarted = false;
    const audioState = new Int32Array(new SharedArrayBuffer(initialSharedState.length * 4));
    audioState.set(initialSharedState);
    const libraryCsound = {
      csoundShouldDaemonize: () => 0,
      csoundStart: () => 0,
      csoundGetOutputName: () => "recording.wav",
      isRequestingRtMidiInput: () => 0,
      isRequestingRtAudioInput: () => 1,
    };
    const workerMessagePort = {
      broadcastPlayState: () => {
        renderStarted = true;
      },
    };
    const start = handleCsoundStart(
      workerMessagePort,
      libraryCsound,
      {},
      () => {
        realtimeStarted = true;
      },
      () => {},
    );

    assert.equal(start({ csound: 1, audioStateBuffer: audioState.buffer }), 0);
    await new Promise((resolve) => setTimeout(resolve, 0));

    assert.equal(realtimeStarted, true);
    assert.equal(renderStarted, false);
    assert.equal(Atomics.load(audioState, AUDIO_STATE.IS_RENDERING), 0);
  });
});
