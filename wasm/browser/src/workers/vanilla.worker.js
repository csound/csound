/* eslint-disable unicorn/require-post-message-target-origin */
import { expose } from "comlink/dist/esm/comlink.mjs";
import MessagePortState from "../utils/message-port-state";
// import { initFS, getWorkerFs, syncWorkerFs } from "../filesystem/worker-fs";
import { logVANWorker as log } from "../logger";
import { RING_BUFFER_SIZE } from "../constants.js";
import { handleCsoundStart, instantiateAudioPacket, renderFunction } from "./common.utils";
import libcsoundFactory from "../libcsound";
import loadWasm from "../module";
import { clearArray } from "../utils/clear-array";
import { assoc, pipe } from "rambda/dist/rambda.mjs";

let combined;
let audioProcessCallback = () => {};
const rtmidiQueue = [];

const createAudioInputBuffers = (audioInputs, inputsCount) => {
  for (let channelIndex = 0; channelIndex < inputsCount; ++channelIndex) {
    audioInputs.buffers.push(new Float64Array(RING_BUFFER_SIZE));
  }
};

const generateAudioFrames = (arguments_, workerMessagePort) => {
  if (workerMessagePort.vanillaWorkerState !== "realtimePerformanceEnded") {
    return audioProcessCallback(arguments_);
  }
};

const createRealtimeAudioThread =
  ({
    libraryCsound,
    wasm,
    wasi,
    workerMessagePort,
    audioInputs,
    inputChannelCount,
    outputChannelCount,
    sampleRate,
  }) =>
  ({ csound }) => {
    // Prompt for midi-input on demand
    // const isRequestingRtMidiInput = libraryCsound._isRequestingRtMidiInput(csound);

    // Prompt for microphone only on demand!
    const isExpectingInput = libraryCsound.csoundGetInputName(csound).includes("adc");

    // Store Csound AudioParams for upcoming performance
    sampleRate && libraryCsound.csoundSetOption(csound, `--sr=${sampleRate}`);
    outputChannelCount && libraryCsound.csoundSetOption(csound, `--nchnls=${outputChannelCount}`);
    inputChannelCount && libraryCsound.csoundSetOption(csound, `--nchnls_i=${inputChannelCount}`);
    const nchnls = libraryCsound.csoundGetNchnls(csound);
    const nchnlsInput =
      inputChannelCount > 0
        ? inputChannelCount
        : isExpectingInput
        ? libraryCsound.csoundGetNchnlsInput(csound)
        : 0;

    const zeroDecibelFullScale = libraryCsound.csoundGet0dBFS(csound);

    // const { buffer } = wasm.wasi.memory;
    const inputBufferPtr = libraryCsound.csoundGetSpin(csound);
    const outputBufferPtr = libraryCsound.csoundGetSpout(csound);
    const ksmps = libraryCsound.csoundGetKsmps(csound);

    let csoundInputBuffer = new Float64Array(
      wasm.wasi.memory.buffer,
      inputBufferPtr,
      ksmps * nchnlsInput,
    );

    let csoundOutputBuffer = new Float64Array(
      wasm.wasi.memory.buffer,
      outputBufferPtr,
      ksmps * nchnls,
    );

    let lastPerformance = 0;
    let currentCsoundBufferPos = 0;
    workerMessagePort.broadcastPlayState("realtimePerformanceStarted");

    audioProcessCallback = ({ numFrames /** number */ }) => {
      const outputAudioPacket = instantiateAudioPacket(nchnls, numFrames);
      const hasInput = audioInputs.buffers.length > 0 && audioInputs.availableFrames >= numFrames;

      if (rtmidiQueue.length > 0) {
        rtmidiQueue.forEach((event) => {
          libraryCsound.csoundPushMidiMessage(csound, event[0], event[1], event[2]);
        });
        clearArray(rtmidiQueue);
      }

      for (let index = 0; index < numFrames; index++) {
        currentCsoundBufferPos = (currentCsoundBufferPos + 1) % ksmps;
        if (workerMessagePort.vanillaWorkerState === "realtimePerformanceEnded") {
          if (lastPerformance === 0) {
            libraryCsound.csoundStop(csound);
            lastPerformance = libraryCsound.csoundPerformKsmps(csound);
          }
          workerMessagePort.broadcastPlayState("realtimePerformanceEnded");

          audioProcessCallback = () => {};
          clearArray(rtmidiQueue);
          audioInputs.port = undefined;
          return { framesLeft: index };
        }
        if (currentCsoundBufferPos === 0 && lastPerformance === 0) {
          lastPerformance = libraryCsound.csoundPerformKsmps(csound);
          if (lastPerformance !== 0) {
            workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
            audioProcessCallback = () => {};
            clearArray(rtmidiQueue);
            audioInputs.port = undefined;
            return { framesLeft: index };
          }
        }

        // MEMGROW KILLS REFERENCES!
        // https://github.com/emscripten-core/emscripten/issues/6747#issuecomment-400081465
        if (csoundInputBuffer.length === 0) {
          csoundInputBuffer = new Float64Array(
            wasm.wasi.memory.buffer,
            libraryCsound.csoundGetSpin(csound),
            ksmps * nchnlsInput,
          );
        }
        if (csoundOutputBuffer.length === 0) {
          csoundOutputBuffer = new Float64Array(
            wasm.wasi.memory.buffer,
            libraryCsound.csoundGetSpout(csound),
            ksmps * nchnls,
          );
        }

        outputAudioPacket.forEach((channel, channelIndex) => {
          if (csoundOutputBuffer.length > 0) {
            channel[index] =
              (csoundOutputBuffer[currentCsoundBufferPos * nchnls + channelIndex] || 0) /
              zeroDecibelFullScale;
          }
        });

        if (hasInput) {
          for (let ii = 0; ii < nchnlsInput; ii++) {
            csoundInputBuffer[currentCsoundBufferPos * nchnlsInput + ii] =
              (audioInputs.buffers[ii][index + (audioInputs.inputReadIndex % RING_BUFFER_SIZE)] ||
                0) * zeroDecibelFullScale;
          }
        }
      }

      if (hasInput) {
        audioInputs.availableFrames -= numFrames;
        audioInputs.inputReadIndex += numFrames % RING_BUFFER_SIZE;
      }

      return { audioPacket: outputAudioPacket, framesLeft: 0 };
    };
  };

const callUncloned = async (k, arguments_) => {
  const caller = combined.get(k);
  return caller && caller.apply({}, arguments_ || []);
};

const initMessagePort = ({ port }) => {
  log(`initMessagePort`)();
  const workerMessagePort = new MessagePortState();
  workerMessagePort.port = port;
  workerMessagePort.post = (messageLog) => port.postMessage({ log: messageLog });
  workerMessagePort.broadcastPlayState = (playStateChange) => {
    workerMessagePort.vanillaWorkerState = playStateChange;
    port.postMessage({ playStateChange });
  };
  workerMessagePort.ready = true;
  return workerMessagePort;
};

const initRequestPort = ({ csoundWorkerFrameRequestPort, workerMessagePort }) => {
  log(`initRequestPort`)();
  csoundWorkerFrameRequestPort.addEventListener("message", (requestEvent) => {
    const { framesLeft = 0, audioPacket } =
      generateAudioFrames(requestEvent.data, workerMessagePort) || {};
    csoundWorkerFrameRequestPort.postMessage({
      numFrames: requestEvent.data.numFrames - framesLeft,
      audioPacket,
      ...requestEvent.data,
    });
  });
  csoundWorkerFrameRequestPort.start();
  return csoundWorkerFrameRequestPort;
};

const initAudioInputPort = ({ port }) => {
  log(`initAudioInputPort`)();
  const audioInputs = {
    availableFrames: 0,
    buffers: [],
    inputReadIndex: 0,
    inputWriteIndex: 0,
    port,
  };
  audioInputs.port.addEventListener("message", ({ data: pkgs }) => {
    if (audioInputs.buffers.length === 0) {
      createAudioInputBuffers(audioInputs, pkgs.length);
    }
    audioInputs.buffers.forEach((buf, index) => {
      buf.set(pkgs[index], audioInputs.inputWriteIndex);
    });
    audioInputs.inputWriteIndex += pkgs[0].length;
    audioInputs.availableFrames += pkgs[0].length;
    if (audioInputs.inputWriteIndex >= RING_BUFFER_SIZE) {
      audioInputs.inputWriteIndex = 0;
    }
  });
  audioInputs.port.start();
  return audioInputs;
};

const initRtMidiEventPort = ({ rtmidiPort }) => {
  log(`initRtMidiEventPort`)();
  rtmidiPort.addEventListener("message", ({ data: payload }) => {
    rtmidiQueue.push(payload);
  });
  rtmidiPort.start();
  return rtmidiPort;
};

const initialize = async ({
  audioInputPort,
  inputChannelCount,
  messagePort,
  outputChannelCount,
  requestPort,
  rtmidiPort,
  sampleRate,
  wasmDataURI,
  wasmTransformerDataURI,
  withPlugins = [],
}) => {
  log(`initializing wasm and exposing csoundAPI functions from worker to main`)();
  const workerMessagePort = initMessagePort({ port: messagePort });

  const audioInputs = initAudioInputPort({ port: audioInputPort });
  initRequestPort({
    csoundWorkerFrameRequestPort: requestPort,
    workerMessagePort,
  });
  initRtMidiEventPort({ rtmidiPort });

  const [wasm, wasi] = await loadWasm({
    wasmDataURI,
    wasmTransformerDataURI,
    withPlugins,
    messagePort: workerMessagePort,
  });

  wasm.wasi = wasi;

  const libraryCsound = libcsoundFactory(wasm);

  const startHandler = (_, arguments_) =>
    handleCsoundStart(
      workerMessagePort,
      libraryCsound,
      wasi,
      createRealtimeAudioThread({
        audioInputs,
        inputChannelCount,
        libraryCsound,
        outputChannelCount,
        wasm,
        wasi,
        workerMessagePort,
      }),
      renderFunction({
        inputChannelCount,
        libraryCsound,
        outputChannelCount,
        wasm,
        workerMessagePort,
      }),
    )(arguments_);

  const allAPI = pipe(assoc("csoundStart", startHandler), assoc("wasm", wasm))(libraryCsound);
  combined = new Map(Object.entries(allAPI));

  libraryCsound.csoundInitialize(0);
  const csoundInstance = libraryCsound.csoundCreate();

  workerMessagePort.port.addEventListener("message", (event) => {
    if (event.data && event.data.newPlayState) {
      if (event.data.newPlayState === "realtimePerformanceEnded") {
        libraryCsound.csoundStop(csoundInstance);
        if (workerMessagePort.vanillaWorkerState !== "realtimePerformanceEnded") {
          libraryCsound.csoundPerformKsmps(csoundInstance);
        }
        // ping-pong for better timing of events:
        // the event is only sent from main but state isn't stored
        // until it arrived back
        workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
      }
      workerMessagePort.vanillaWorkerState = event.data.newPlayState;
    }
  });

  workerMessagePort.port.start();

  return csoundInstance;
};

expose({
  initialize,
  callUncloned,
});
