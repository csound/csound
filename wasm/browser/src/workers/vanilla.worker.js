import * as Comlink from "comlink";
import { writeToFs, lsFs, llFs, readFromFs, rmrfFs, workerMessagePort } from "@root/filesystem";
import { logVAN } from "@root/logger";
import { MAX_HARDWARE_BUFFER_SIZE } from "@root/constants.js";
import { handleCsoundStart, instantiateAudioPacket } from "@root/workers/common.utils";
import libcsoundFactory from "@root/libcsound";
import loadWasm from "@root/module";
import { assoc, pipe } from "ramda";

let wasm, combined, libraryCsound;

let audioProcessCallback = () => {};

const audioInputs = {
  availableFrames: 0,
  buffers: [],
  inputReadIndex: 0,
  inputWriteIndex: 0,
  port: undefined,
};

let csoundWorkerFrameRequestPort;

let rtmidiPort;
let rtmidiQueue = [];

const createAudioInputBuffers = (inputsCount) => {
  for (let channelIndex = 0; channelIndex < inputsCount; ++channelIndex) {
    audioInputs.buffers.push(new Float64Array(MAX_HARDWARE_BUFFER_SIZE));
  }
};

const generateAudioFrames = (arguments_) => {
  if (workerMessagePort.vanillaWorkerState !== "realtimePerformanceEnded") {
    return audioProcessCallback(arguments_);
  }
};

const createRealtimeAudioThread = ({ csound }) => {
  if (!wasm || !libraryCsound) {
    workerMessagePort.post("error: csound wasn't initialized before starting");
    return -1;
  }

  // Prompt for midi-input on demand
  // const isRequestingRtMidiInput = libraryCsound._isRequestingRtMidiInput(csound);

  // Prompt for microphone only on demand!
  const isExpectingInput = libraryCsound.csoundGetInputName(csound).includes("adc");

  // Store Csound AudioParams for upcoming performance
  const nchnls = libraryCsound.csoundGetNchnls(csound);
  const nchnlsInput = isExpectingInput ? libraryCsound.csoundGetNchnlsInput(csound) : 0;
  const zeroDecibelFullScale = libraryCsound.csoundGet0dBFS(csound);

  workerMessagePort.broadcastPlayState("realtimePerformanceStarted");

  const { buffer } = wasm.exports.memory;
  const inputBufferPtr = libraryCsound.csoundGetSpin(csound);
  const outputBufferPtr = libraryCsound.csoundGetSpout(csound);
  const ksmps = libraryCsound.csoundGetKsmps(csound);

  let csoundInputBuffer = new Float64Array(buffer, inputBufferPtr, ksmps * nchnlsInput);

  let csoundOutputBuffer = new Float64Array(buffer, outputBufferPtr, ksmps * nchnls);

  let lastPerformance = 0;

  audioProcessCallback = ({ readIndex, numFrames }) => {
    const outputAudioPacket = instantiateAudioPacket(nchnls, numFrames);
    const hasInput = audioInputs.buffers.length > 0 && audioInputs.availableFrames >= numFrames;

    if (rtmidiQueue.length > 0) {
      rtmidiQueue.forEach((event) => {
        try {
          libraryCsound.csoundPushMidiMessage(csound, event[0], event[1], event[2]);
        } catch (error) {
          console.error(error);
        }
      });
      rtmidiQueue = [];
    }

    for (let i = 0; i < numFrames; i++) {
      const currentCsoundBufferPos = i % ksmps;

      if (currentCsoundBufferPos === 0 && lastPerformance === 0) {
        lastPerformance = libraryCsound.csoundPerformKsmps(csound);
        if (lastPerformance !== 0) {
          workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
          audioProcessCallback = () => {};
          rtmidiQueue = [];
          rtmidiPort = undefined;
          audioInputs.port = undefined;
          csoundWorkerFrameRequestPort = undefined;
          return { framesLeft: i };
        }
      }

      // MEMGROW KILLS REFERENCES!
      // https://github.com/emscripten-core/emscripten/issues/6747#issuecomment-400081465
      if (csoundInputBuffer.length === 0) {
        csoundInputBuffer = new Float64Array(
          wasm.exports.memory.buffer,
          libraryCsound.csoundGetSpin(csound),
          ksmps * nchnlsInput,
        );
      }
      if (csoundOutputBuffer.length === 0) {
        csoundOutputBuffer = new Float64Array(
          wasm.exports.memory.buffer,
          libraryCsound.csoundGetSpout(csound),
          ksmps * nchnls,
        );
      }

      outputAudioPacket.forEach((channel, channelIndex) => {
        if (csoundOutputBuffer.length > 0) {
          channel[i] =
            (csoundOutputBuffer[currentCsoundBufferPos * nchnls + channelIndex] || 0) /
            zeroDecibelFullScale;
        }
      });

      if (hasInput) {
        for (let ii = 0; ii < nchnlsInput; ii++) {
          csoundInputBuffer[currentCsoundBufferPos * nchnlsInput + ii] =
            (audioInputs.buffers[ii][i + (audioInputs.inputReadIndex % MAX_HARDWARE_BUFFER_SIZE)] ||
              0) * zeroDecibelFullScale;
        }
      }
    }

    if (hasInput) {
      audioInputs.availableFrames -= numFrames;
      audioInputs.inputReadIndex += numFrames % MAX_HARDWARE_BUFFER_SIZE;
    }

    return { audioPacket: outputAudioPacket, framesLeft: 0 };
  };
};

const callUncloned = async (k, arguments_) => {
  const caller = combined.get(k);
  return caller && caller.apply({}, arguments_ || []);
};

addEventListener("message", (event) => {
  if (event.data.msg === "initMessagePort") {
    logVAN(`initMessagePort`);
    const port = event.ports[0];
    workerMessagePort.post = (log) => port.postMessage({ log });
    workerMessagePort.broadcastPlayState = (playStateChange) => {
      workerMessagePort.vanillaWorkerState = playStateChange;
      port.postMessage({ playStateChange });
    };
    workerMessagePort.ready = true;
  } else if (event.data.msg === "initRequestPort") {
    logVAN(`initRequestPort`);
    csoundWorkerFrameRequestPort = event.ports[0];
    csoundWorkerFrameRequestPort.addEventListener("message", (requestEvent) => {
      const { framesLeft = 0, audioPacket } = generateAudioFrames(requestEvent.data) || {};
      csoundWorkerFrameRequestPort &&
        csoundWorkerFrameRequestPort.postMessage({
          numFrames: requestEvent.data.numFrames - framesLeft,
          audioPacket,
          ...requestEvent.data,
        });
    });
    csoundWorkerFrameRequestPort.start();
  } else if (event.data.msg === "initAudioInputPort") {
    logVAN(`initAudioInputPort`);
    audioInputs.port = event.ports[0];
    audioInputs.port.addEventListener("message", ({ data: pkgs }) => {
      if (audioInputs.buffers.length === 0) {
        createAudioInputBuffers(pkgs.length);
      }
      audioInputs.buffers.forEach((buf, i) => {
        buf.set(pkgs[i], audioInputs.inputWriteIndex);
      });
      audioInputs.inputWriteIndex += pkgs[0].length;
      audioInputs.availableFrames += pkgs[0].length;
      if (audioInputs.inputWriteIndex >= MAX_HARDWARE_BUFFER_SIZE) {
        audioInputs.inputWriteIndex = 0;
      }
    });
    audioInputs.port.start();
  } else if (event.data.msg === "initRtMidiEventPort") {
    logVAN(`initRtMidiEventPort`);
    rtmidiPort = event.ports[0];
    rtmidiPort.addEventListener("message", ({ data: payload }) => {
      rtmidiQueue.push(payload);
    });
    rtmidiPort.start();
  } else if (event.data.playStateChange) {
    logVAN(`playStateChange`, event.data.playStateChange.playStateChange);
    workerMessagePort.vanillaWorkerState = event.data.playStateChange.playStateChange;
  }
});

const initialize = async (wasmDataURI, withPlugins = []) => {
  logVAN(`initializing wasm and exposing csoundAPI functions from worker to main`);
  wasm = await loadWasm(wasmDataURI);
  libraryCsound = libraryCsound || libcsoundFactory(wasm);
  const startHandler = handleCsoundStart(
    workerMessagePort,
    libraryCsound,
    createRealtimeAudioThread,
    plugins,
  );
  const allAPI = pipe(
    assoc("writeToFs", writeToFs),
    assoc("readFromFs", readFromFs),
    assoc("lsFs", lsFs),
    assoc("llFs", llFs),
    assoc("rmrfFs", rmrfFs),
    assoc("csoundStart", startHandler),
    assoc("wasm", wasm),
  )(libraryCsound);
  combined = new Map(Object.entries(allAPI));

  libraryCsound.csoundInitialize(0);
  const csoundInstance = libraryCsound.csoundCreate();
  plugins.forEach((plugin) => plugin.exports.init(csoundInstance));

  return csoundInstance;
};

Comlink.expose({
  initialize,
  callUncloned,
});
