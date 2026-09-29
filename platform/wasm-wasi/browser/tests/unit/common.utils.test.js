/* eslint-env mocha */

import assert from "node:assert/strict";

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

  it("uses the real-time path for microphone input without DAC output", async () => {
    let realtimeStarted = false;
    let renderStarted = false;
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

    assert.equal(start({ csound: 1 }), 0);
    await new Promise((resolve) => setTimeout(resolve, 0));

    assert.equal(realtimeStarted, true);
    assert.equal(renderStarted, false);
  });
});
