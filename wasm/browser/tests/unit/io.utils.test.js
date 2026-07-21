/* eslint-env mocha */

import assert from "node:assert/strict";

import {
  enableAudioInput,
  enableAudioInputInWorker,
  releaseMicrophoneStream,
  requestMicrophoneNode,
  requestMicrophoneStream,
} from "../../src/mains/io.utils.js";

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
    const owner = {
      exportApi: Object.freeze({
        setOption: async (option) => {
          assert.equal(option, "-iadc");
          return 0;
        },
      }),
      audioContext: {
        createMediaStreamSource: (value) => {
          assert.equal(value, stream);
          return liveInput;
        },
      },
      node,
    };

    let resolved = false;
    const enabled = enableAudioInput.call(owner).then(() => {
      resolved = true;
    });
    while (!grantPermission) {
      await Promise.resolve();
    }
    assert.equal(resolved, false);

    grantPermission(stream);
    await enabled;

    assert.equal(owner.inputsCount, 2);
    assert.equal(connectedNode, node);
  });

  it("releases the microphone source and media tracks", () => {
    let disconnected = false;
    let stopped = false;
    const owner = {
      microphoneInput: {
        disconnect: () => {
          disconnected = true;
        },
      },
      microphonePromise: Promise.resolve(),
      microphoneStream: {
        getTracks: () => [
          {
            stop: () => {
              stopped = true;
            },
          },
        ],
      },
    };

    releaseMicrophoneStream(owner);

    assert.equal(disconnected, true);
    assert.equal(stopped, true);
    assert.equal(owner.microphoneInput, undefined);
    assert.equal(owner.microphonePromise, undefined);
    assert.equal(owner.microphoneStream, undefined);
  });

  it("shares an in-flight microphone request", async () => {
    let grantPermission;
    let requestCount = 0;
    const stream = { id: "microphone" };
    setNavigator({
      mediaDevices: {
        getUserMedia: () => {
          requestCount += 1;
          return new Promise((resolve) => {
            grantPermission = resolve;
          });
        },
      },
    });

    const owner = {};
    const firstRequest = requestMicrophoneStream.call(owner);
    const secondRequest = requestMicrophoneStream.call(owner);
    grantPermission(stream);

    assert.equal(await firstRequest, stream);
    assert.equal(await secondRequest, stream);
    assert.equal(await requestMicrophoneStream.call(owner), stream);
    assert.equal(requestCount, 1);
  });

  it("enables input before worker mode starts", async () => {
    const calls = [];
    const main = {
      currentPlayState: undefined,
      exportApi: {
        setOption: async (option) => {
          calls.push(["setOption", option]);
          return 0;
        },
      },
      audioWorker: {
        requestMicrophoneInput: async () => {
          calls.push(["requestMicrophoneInput"]);
        },
      },
    };

    await enableAudioInputInWorker.call(main);

    assert.deepEqual(calls, [
      ["setOption", "-iadc"],
      ["requestMicrophoneInput"],
    ]);
  });

  it("rejects worker input changes after start", async () => {
    await assert.rejects(
      enableAudioInputInWorker.call({ currentPlayState: "realtimePerformanceStarted" }),
      { message: "enableAudioInput() must be called before start() in worker mode" },
    );
  });

  it("passes worker microphone permission errors to the caller", async () => {
    const permissionError = new Error("Permission denied");
    const main = {
      currentPlayState: undefined,
      exportApi: { setOption: async () => 0 },
      audioWorker: {
        requestMicrophoneInput: async () => {
          throw permissionError;
        },
      },
    };

    await assert.rejects(
      enableAudioInputInWorker.call(main),
      (error) => error === permissionError,
    );
  });
});
