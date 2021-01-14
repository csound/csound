import * as Comlink from "comlink";
import MessagePortState from "@utils/message-port-state";
import { initFS, writeToFs, lsFs, llFs, readFromFs, rmrfFs } from "@root/filesystem";
import { logVANWorker as log } from "@root/logger";
import { MAX_HARDWARE_BUFFER_SIZE } from "@root/constants.js";
import { handleCsoundStart, instantiateAudioPacket } from "@root/workers/common.utils";
import libcsoundFactory from "@root/libcsound";
import loadWasm from "@root/module";
import { assoc, pipe } from "ramda";

let combined;
let audioProcessCallback = () => {};
let rtmidiQueue = [];

const createAudioInputBuffers = (audioInputs, inputsCount) => {
  for (let channelIndex = 0; channelIndex < inputsCount; ++channelIndex) {
    audioInputs.buffers.push(new Float64Array(MAX_HARDWARE_BUFFER_SIZE));
  }
};

const generateAudioFrames = (arguments_, workerMessagePort) => {
  if (workerMessagePort.vanillaWorkerState !== "realtimePerformanceEnded") {
    return audioProcessCallback(arguments_);
  }
};

const createRealtimeAudioThread = ({
  libraryCsound,
  wasm,
  wasmFs,
  workerMessagePort,
  audioInputs,
  watcherStdOut,
  watcherStdErr,
}) => ({ csound }) => {
  if (!watcherStdOut && !watcherStdErr) {
    [watcherStdOut, watcherStdErr] = initFS(wasmFs, workerMessagePort);
  }
  const closeWatchers = () => {
    watcherStdOut && watcherStdOut.close();
    watcherStdOut = undefined;
    watcherStdErr && watcherStdErr.close();
    watcherStdErr = undefined;
  };
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

  audioProcessCallback = ({ numFrames }) => {
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
      if (workerMessagePort.vanillaWorkerState === "realtimePerformanceEnded") {
        if (lastPerformance === 0) {
          libraryCsound.csoundStop(csoundInstance);
          lastPerformance = libraryCsound.csoundPerformKsmps(csound);
        }
        workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
        audioProcessCallback = () => {};
        rtmidiQueue = [];
        audioInputs.port = undefined;
        closeWatchers();
        return { framesLeft: i };
      }
      if (currentCsoundBufferPos === 0 && lastPerformance === 0) {
        lastPerformance = libraryCsound.csoundPerformKsmps(csound);
        if (lastPerformance !== 0) {
          workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
          audioProcessCallback = () => {};
          rtmidiQueue = [];
          audioInputs.port = undefined;
          closeWatchers();
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

const renderFn = ({
  libraryCsound,
  workerMessagePort,
  wasmFs,
  watcherStdOut,
  watcherStdErr,
}) => async ({ csound }) => {
  let endResolve;
  if (!watcherStdOut && !watcherStdErr) {
    [watcherStdOut, watcherStdErr] = initFS(wasmFs, workerMessagePort);
  }

  const endPromise = new Promise((resolve) => {
    endResolve = resolve;
  });
  const performKsmps = () => {
    if (
      workerMessagePort.vanillaWorkerState === "renderStarted" &&
      libraryCsound.csoundPerformKsmps(csound) === 0
    ) {
      // this is immediately executed, but allows events to be picked up
      setTimeout(performKsmps, 0);
    } else {
      workerMessagePort.broadcastPlayState("renderEnded");
      endResolve();
    }
  };
  performKsmps();
  await endPromise;
  watcherStdOut && watcherStdOut.close();
  watcherStdOut = undefined;
  watcherStdErr && watcherStdErr.close();
  watcherStdErr = undefined;
};

const initialize = async ({
  wasmDataURI,
  withPlugins = [],
  messagePort,
  requestPort,
  audioInputPort,
  rtmidiPort,
}) => {
  log(`initializing wasm and exposing csoundAPI functions from worker to main`)();
  const workerMessagePort = initMessagePort({ port: messagePort });

  const audioInputs = initAudioInputPort({ port: audioInputPort });
  initRequestPort({
    csoundWorkerFrameRequestPort: requestPort,
    workerMessagePort,
  });
  initRtMidiEventPort({ rtmidiPort });
  const [wasm, wasmFs] = await loadWasm({
    wasmDataURI,
    withPlugins,
    messagePort: workerMessagePort,
  });

  let [watcherStdOut, watcherStdErr] = initFS(wasmFs, workerMessagePort);

  const libraryCsound = libcsoundFactory(wasm);

  const startHandler = handleCsoundStart(
    workerMessagePort,
    libraryCsound,
    createRealtimeAudioThread({
      libraryCsound,
      wasm,
      wasmFs,
      audioInputs,
      workerMessagePort,
      watcherStdOut,
      watcherStdErr,
    }),
    renderFn({ libraryCsound, workerMessagePort, watcherStdOut, watcherStdErr, wasmFs }),
  );

  const allAPI = pipe(
    assoc("writeToFs", writeToFs(wasmFs)),
    assoc("readFromFs", readFromFs(wasmFs)),
    assoc("lsFs", lsFs(wasmFs)),
    assoc("llFs", llFs(wasmFs)),
    assoc("rmrfFs", rmrfFs(wasmFs)),
    assoc("csoundStart", startHandler),
    assoc("wasm", wasm),
  )(libraryCsound);
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

Comlink.expose({
  initialize,
  callUncloned,
});
