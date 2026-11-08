import assert from "node:assert/strict";

let handleCsoundStart;
let originalGoogDescriptor;
describe("pre-render setup", () => {
  before(async () => {
    originalGoogDescriptor = Object.getOwnPropertyDescriptor(globalThis, "goog");
    Object.defineProperty(globalThis, "goog", {
      configurable: true,
      value: { define: (_name, value) => value },
    });
    ({ handleCsoundStart } = await import("../../src/workers/common.utils.js"));
  });
  after(() => {
    if (originalGoogDescriptor) Object.defineProperty(globalThis, "goog", originalGoogDescriptor);
    else delete globalThis.goog;
  });
  const library = (calls) => ({
    csoundShouldDaemonize: () => 0,
    csoundStart: () => {
      calls.push("start");
      return 0;
    },
    csoundGetOutputName: () => "dac",
    isRequestingRtMidiInput: () => 0,
    isRequestingRtAudioInput: () => 1,
    csoundReset: () => {
      calls.push("reset");
    },
  });
  it("runs setup after global init and before scheduling render", async () => {
    const calls = [];
    const rendered = new Promise((resolve) => {
      assert.equal(
        handleCsoundStart(
          {},
          library(calls),
          {},
          () => {
            calls.push("render");
            resolve();
          },
          undefined,
          () => {
            calls.push("snapshot");
          },
        )({ csound: 42 }),
        0,
      );
    });
    assert.deepEqual(calls, ["start", "snapshot"]);
    await rendered;
    assert.deepEqual(calls, ["start", "snapshot", "render"]);
  });
  it("joins idle workers and schedules no render when setup fails", async () => {
    const calls = [];
    const failure = new Error("snapshot failed");
    assert.throws(
      () =>
        handleCsoundStart(
          {},
          library(calls),
          {},
          () => {
            calls.push("render");
          },
          undefined,
          () => {
            throw failure;
          },
        )({ csound: 42 }),
      failure,
    );
    await new Promise((resolve) => setTimeout(resolve, 0));
    assert.deepEqual(calls, ["start", "reset"]);
  });
});
