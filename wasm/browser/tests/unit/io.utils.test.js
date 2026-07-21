/* eslint-env mocha */

import assert from "node:assert/strict";

import { enableAudioInput, requestMicrophoneNode } from "../../src/mains/io.utils.js";

const originalNavigator = Object.getOwnPropertyDescriptor(globalThis, "navigator");

const setNavigator = (value) => {
  Object.defineProperty(globalThis, "navigator", {
    configurable: true,
    value,
  });
};

describe("microphone input", () => {
  afterEach(() => {
    if (originalNavigator) {
      Object.defineProperty(globalThis, "navigator", originalNavigator);
    } else {
      delete globalThis.navigator;
    }
  });

  it("rejects with a clear error when browser input is unavailable", async () => {
    setNavigator({});

    await assert.rejects(requestMicrophoneNode(), {
      message:
        "Microphone input is unavailable. Use HTTPS, localhost, or a loopback address and allow microphone access.",
    });
  });

  it("requests modern browser audio input", async () => {
    const stream = { id: "microphone" };
    const mediaDevices = {
      getUserMedia: async (constraints) => {
        assert.deepEqual(constraints, {
          audio: { echoCancellation: false, sampleSize: 32 },
        });
        return stream;
      },
    };
    setNavigator({ mediaDevices });

    assert.equal(await requestMicrophoneNode(), stream);
  });

  it("passes browser permission errors to the caller", async () => {
    const permissionError = new Error("Permission denied");
    permissionError.name = "NotAllowedError";
    setNavigator({
      mediaDevices: {
        getUserMedia: async () => {
          throw permissionError;
        },
      },
    });

    await assert.rejects(requestMicrophoneNode(), (error) => error === permissionError);
  });

  it("supports the legacy callback API", async () => {
    const stream = { id: "legacy-microphone" };
    setNavigator({
      getUserMedia: (constraints, resolve) => {
        assert.equal(constraints.audio.optional[0].echoCancellation, false);
        resolve(stream);
      },
    });

    assert.equal(await requestMicrophoneNode(), stream);
  });

  it("resolves enableAudioInput only after connecting the stream", async () => {
    let grantPermission;
    const stream = { id: "microphone" };
    setNavigator({
      mediaDevices: {
        getUserMedia: () =>
          new Promise((resolve) => {
            grantPermission = resolve;
          }),
      },
    });

    const node = {};
    let connectedNode;
    const liveInput = {
      channelCount: 2,
      connect: (value) => {
        connectedNode = value;
      },
    };
    const csound = {
      getAudioContext: async () => ({
        createMediaStreamSource: (value) => {
          assert.equal(value, stream);
          return liveInput;
        },
      }),
      getNode: async () => node,
    };

    let resolved = false;
    const enabled = enableAudioInput.call(csound).then(() => {
      resolved = true;
    });
    await Promise.resolve();
    assert.equal(resolved, false);

    grantPermission(stream);
    await enabled;

    assert.equal(csound.inputsCount, 2);
    assert.equal(connectedNode, node);
  });
});
