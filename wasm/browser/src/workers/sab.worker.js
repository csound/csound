import { expose } from "comlink/dist/esm/comlink.mjs";
import MessagePortState from "../utils/message-port-state";
import libcsoundFactory from "../libcsound.js";
import loadWasm from "../module";
import { logSABWorker as log } from "../logger";
import { handleCsoundStart } from "../workers/common.utils";
import {
  AUDIO_STATE,
  RING_BUFFER_SIZE,
  MIDI_BUFFER_SIZE,
  MIDI_BUFFER_PAYLOAD_SIZE,
  initialSharedState,
} from "../constants";
import { assoc, pipe } from "rambda/dist/rambda.mjs";

/**
 * @type {Map.<LibcsoundUncloned>}
 */
let combined;
let pollPromise;
let unlockPromise;

const callUncloned = async (k, arguments_) => {
  const caller = combined.get(k);
  const returnValue = caller && caller.apply({}, arguments_ || []);
  return returnValue;
};

const sabCreateRealtimeAudioThread =
  ({
    libraryCsound,
    callbacksRequest,
    releaseStop,
    releasePause,
    releaseResumed,
    wasm,
    wasi,
    workerMessagePort,
  }) =>
  async ({ audioStateBuffer, audioStreamIn, audioStreamOut, midiBuffer, csound }) => {
    const audioStatePointer = new Int32Array(audioStateBuffer);

    // In case of multiple performances, let's reset the sab state
    initialSharedState.forEach((value, index) => {
      Atomics.store(audioStatePointer, index, value);
    });

    // Prompt for midi-input on demand
    const isRequestingRtMidiInput = libraryCsound._isRequestingRtMidiInput(csound);

    // Prompt for microphone only on demand!
    const isExpectingInput =
      Atomics.load(audioStatePointer, AUDIO_STATE.NCHNLS_I) === 0 &&
      libraryCsound.csoundGetInputName(csound).includes("adc");

    // Store Csound AudioParams for upcoming performance
    const userProvidedNchnls = Atomics.load(audioStatePointer, AUDIO_STATE.NCHNLS);
    const userProvidedNchnlsIn = Atomics.load(audioStatePointer, AUDIO_STATE.NCHNLS_I);
    const userProvidedSr = Atomics.load(audioStatePointer, AUDIO_STATE.SAMPLE_RATE);

    userProvidedNchnls > -1 &&
      libraryCsound.csoundSetOption(csound, `--nchnls=${userProvidedNchnls}`);
    userProvidedNchnlsIn > -1 &&
      libraryCsound.csoundSetOption(csound, `--nchnls_i=${userProvidedNchnlsIn}`);
    userProvidedSr > -1 && libraryCsound.csoundSetOption(csound, `--sr=${userProvidedSr}`);

    const nchnls = libraryCsound.csoundGetNchnls(csound);

    const nchnlsInput =
      userProvidedNchnlsIn || isExpectingInput ? libraryCsound.csoundGetNchnlsInput(csound) : 0;
    const sampleRate =
      Atomics.load(audioStatePointer, AUDIO_STATE.SAMPLE_RATE) || libraryCsound.csoundGetSr(csound);

    // a final merge of user configuration and csound options
    Atomics.store(audioStatePointer, AUDIO_STATE.NCHNLS, nchnls);
    Atomics.store(audioStatePointer, AUDIO_STATE.NCHNLS_I, nchnlsInput);
    Atomics.store(audioStatePointer, AUDIO_STATE.IS_REQUESTING_MIC, isExpectingInput ? 1 : 0);
    Atomics.store(audioStatePointer, AUDIO_STATE.SAMPLE_RATE, libraryCsound.csoundGetSr(csound));
    Atomics.store(audioStatePointer, AUDIO_STATE.IS_REQUESTING_RTMIDI, isRequestingRtMidiInput);

    const ksmps = libraryCsound.csoundGetKsmps(csound);
    Atomics.store(audioStatePointer, AUDIO_STATE.KSMPS, ksmps);

    const zeroDecibelFullScale = libraryCsound.csoundGet0dBFS(csound);

    // Get the Worklet channels
    const channelsOutput = [];
    const channelsInput = [];

    for (let channelIndex = 0; channelIndex < nchnls; ++channelIndex) {
      channelsOutput.push(
        new Float64Array(audioStreamOut, RING_BUFFER_SIZE * channelIndex, RING_BUFFER_SIZE),
      );
    }

    for (let channelIndex = 0; channelIndex < nchnlsInput; ++channelIndex) {
      channelsInput.push(
        new Float64Array(audioStreamIn, RING_BUFFER_SIZE * channelIndex, RING_BUFFER_SIZE),
      );
    }

    workerMessagePort.broadcastPlayState("realtimePerformanceStarted");
    // Let's notify the audio-worker that performance has started
    Atomics.store(audioStatePointer, AUDIO_STATE.IS_PERFORMING, 1);

    log(`Atomic.wait started (thread is now locked)\n`)();

    let firstRound = true;
    let lastReturn = 0;
    let currentCsoundBufferPos = 0;
    let currentInputReadIndex = 0;
    let currentOutputWriteIndex = 0;
    let waitResult;

    const maybeStop = (forceStop = false) => {
      if (
        Atomics.load(audioStatePointer, AUDIO_STATE.STOP) === 1 ||
        Atomics.load(audioStatePointer, AUDIO_STATE.IS_PERFORMING) !== 1 ||
        lastReturn !== 0 ||
        forceStop
      ) {
        if (lastReturn === 0) {
          log(`calling csoundStop and one performKsmps to trigger endof logs`)();
          // Trigger "performance ended" logs
          libraryCsound.csoundStop(csound);
          libraryCsound.csoundPerformKsmps(csound);
        }
        log(`triggering realtimePerformanceEnded event`)();
        workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
        log(`End of realtimePerformance loop!`)();
        releaseStop();
        return true;
      } else {
        return false;
      }
    };

    let firstPerformKsmps = true;

    while (
      !firstPerformKsmps ||
      (waitResult = Atomics.wait(audioStatePointer, AUDIO_STATE.CSOUND_LOCK, 1, 10 * 1000))
    ) {
      if (waitResult === "timed-out") {
        maybeStop(true);
        return;
      }

      if (firstRound) {
        firstRound = false;
        await new Promise((resolve) => {
          unlockPromise = resolve;
          workerMessagePort.broadcastSabUnlocked();
        });

        log(`Atomic.wait unlocked, performance started`)();
      }

      if (Atomics.load(audioStatePointer, AUDIO_STATE.IS_PAUSED) === 1) {
        await new Promise((resolve) => setTimeout(resolve, 0));
        releasePause();
        await new Promise((resolve) => setTimeout(resolve, 0));
        Atomics.wait(audioStatePointer, AUDIO_STATE.IS_PAUSED, 0);
        await new Promise((resolve) => setTimeout(resolve, 0));
        releaseResumed();
        await new Promise((resolve) => setTimeout(resolve, 0));
      }

      if (maybeStop()) {
        return;
      }

      if (isRequestingRtMidiInput) {
        const availableMidiEvents = Atomics.load(
          audioStatePointer,
          AUDIO_STATE.AVAIL_RTMIDI_EVENTS,
        );
        if (availableMidiEvents > 0) {
          const rtmidiBufferIndex = Atomics.load(audioStatePointer, AUDIO_STATE.RTMIDI_INDEX);
          let absIndex = rtmidiBufferIndex;
          for (let index = 0; index < availableMidiEvents; index++) {
            absIndex = (rtmidiBufferIndex + MIDI_BUFFER_PAYLOAD_SIZE * index) % MIDI_BUFFER_SIZE;
            const status = Atomics.load(midiBuffer, absIndex);
            const data1 = Atomics.load(midiBuffer, absIndex + 1);
            const data2 = Atomics.load(midiBuffer, absIndex + 2);
            libraryCsound.csoundPushMidiMessage(csound, status, data1, data2);
          }

          Atomics.store(
            audioStatePointer,
            AUDIO_STATE.RTMIDI_INDEX,
            (absIndex + 1) % MIDI_BUFFER_SIZE,
          );
          Atomics.sub(audioStatePointer, AUDIO_STATE.AVAIL_RTMIDI_EVENTS, availableMidiEvents);
        }
      }

      const bufferLength = Atomics.load(audioStatePointer, AUDIO_STATE.BUFFER_LEN);
      const availableInputFrames = Atomics.load(audioStatePointer, AUDIO_STATE.AVAIL_IN_BUFS);

      const hasInput = availableInputFrames >= bufferLength;
      const inputBufferPtr = libraryCsound.csoundGetSpin(csound);
      const outputBufferPtr = libraryCsound.csoundGetSpout(csound);

      const csoundInputBuffer =
        hasInput && new Float64Array(wasm.wasi.memory.buffer, inputBufferPtr, ksmps * nchnlsInput);

      const csoundOutputBuffer = new Float64Array(
        wasm.wasi.memory.buffer,
        outputBufferPtr,
        ksmps * nchnls,
      );

      const framesRequested = Atomics.load(audioStatePointer, AUDIO_STATE.FRAMES_REQUESTED);

      for (let index = 0; index < framesRequested; index++) {
        if (currentCsoundBufferPos === 0) {
          if (lastReturn === 0) {
            lastReturn = libraryCsound.csoundPerformKsmps(csound);
            !firstPerformKsmps && Atomics.add(audioStatePointer, AUDIO_STATE.AVAIL_OUT_BUFS, ksmps);
            firstPerformKsmps = false;
          } else if (lastReturn !== 0) {
            Atomics.store(audioStatePointer, AUDIO_STATE.IS_PERFORMING, 0);
            maybeStop(true);
            return;
          }
        }

        channelsOutput.forEach((channel, channelIndex) => {
          channel[currentOutputWriteIndex] =
            (csoundOutputBuffer[currentCsoundBufferPos * nchnls + channelIndex] || 0) /
            zeroDecibelFullScale;
        });

        if (hasInput) {
          channelsInput.forEach((channel, channelIndex) => {
            csoundInputBuffer[currentCsoundBufferPos * nchnlsInput + channelIndex] =
              (channel[currentInputReadIndex] || 0) * zeroDecibelFullScale;
          });

          currentInputReadIndex = hasInput && (currentInputReadIndex + 1) % RING_BUFFER_SIZE;
        }

        currentOutputWriteIndex = (currentOutputWriteIndex + 1) % RING_BUFFER_SIZE;
        currentCsoundBufferPos = (currentCsoundBufferPos + 1) % ksmps;
      }

      hasInput && Atomics.sub(audioStatePointer, AUDIO_STATE.AVAIL_IN_BUFS, framesRequested);

      if (
        Atomics.compareExchange(audioStatePointer, AUDIO_STATE.HAS_PENDING_CALLBACKS, 1, 0) === 1
      ) {
        await new Promise((resolve) => {
          pollPromise = resolve;
          callbacksRequest();
        });
      }

      if (maybeStop()) {
        return;
      }

      const readIndex = Atomics.load(audioStatePointer, AUDIO_STATE.OUTPUT_READ_INDEX);

      const ioDelay =
        currentOutputWriteIndex < readIndex
          ? currentOutputWriteIndex + RING_BUFFER_SIZE - readIndex
          : currentOutputWriteIndex - readIndex;

      // 2048 is only an indicator of MAX latency
      const nextFramesRequested = Math.max(2048 - ioDelay, 0);

      Atomics.store(audioStatePointer, AUDIO_STATE.FRAMES_REQUESTED, nextFramesRequested);

      if (nextFramesRequested === 0) {
        await new Promise((resolve) => setTimeout(resolve, 1000 * (bufferLength / sampleRate)));
      }
    }
  };

const initMessagePort = ({ port }) => {
  const workerMessagePort = new MessagePortState();
  workerMessagePort.post = (messageLog) => port.postMessage({ log: messageLog });
  workerMessagePort.broadcastPlayState = (playStateChange) => port.postMessage({ playStateChange });
  workerMessagePort.broadcastSabUnlocked = () => port.postMessage({ sabWorker: "unlocked" });
  workerMessagePort.ready = true;
  return workerMessagePort;
};

const initCallbackReplyPort = ({ port }) => {
  port.addEventListener("message", (event) => {
    if (event.data && event.data.unlock) {
      const unlockPromise_ = unlockPromise;
      unlockPromise = undefined;
      unlockPromise_ && unlockPromise_();
    } else {
      const callbacks = event.data;
      const answers = callbacks.reduce((accumulator, { id, argumentz, apiKey }) => {
        try {
          const caller = combined.get(apiKey);
          const answer = caller && caller.apply({}, argumentz || []);
          accumulator.push({ id, answer });
        } catch (error) {
          throw new Error(error);
        }
        return accumulator;
      }, []);
      port.postMessage(answers);
      const pollPromise_ = pollPromise;
      pollPromise = undefined;
      pollPromise_ && pollPromise_(callbacks);
    }
  });
  port.start();
};

const renderFunction =
  ({
    libraryCsound,
    callbacksRequest,
    releaseStop,
    releasePause,
    releaseResumed,
    wasi,
    workerMessagePort,
  }) =>
  async ({ audioStateBuffer, csound }) => {
    const audioStatePointer = new Int32Array(audioStateBuffer);
    Atomics.store(audioStatePointer, AUDIO_STATE.IS_RENDERING, 1);
    workerMessagePort.broadcastSabUnlocked();

    while (
      Atomics.load(audioStatePointer, AUDIO_STATE.STOP) !== 1 &&
      libraryCsound.csoundPerformKsmps(csound) === 0
    ) {
      if (Atomics.load(audioStatePointer, AUDIO_STATE.IS_PAUSED) === 1) {
        releasePause();
        // eslint-disable-next-line no-unused-expressions
        Atomics.wait(audioStatePointer, AUDIO_STATE.IS_PAUSED, 0);
        releaseResumed();
      }
      if (
        Atomics.compareExchange(audioStatePointer, AUDIO_STATE.HAS_PENDING_CALLBACKS, 1, 0) === 1
      ) {
        await new Promise((resolve) => {
          pollPromise = resolve;
          callbacksRequest();
        });
      }
    }
    Atomics.store(audioStatePointer, AUDIO_STATE.IS_RENDERING, 0);
    workerMessagePort.broadcastPlayState("renderEnded");
    releaseStop();
  };

const initialize = async ({ wasmDataURI, withPlugins = [], messagePort, callbackPort }) => {
  log(`initializing SABWorker and WASM`)();
  const workerMessagePort = initMessagePort({ port: messagePort });
  const callbacksRequest = () => callbackPort.postMessage("poll");
  const releaseStop = () => callbackPort.postMessage("releaseStop");
  const releasePause = () => callbackPort.postMessage("releasePause");
  const releaseResumed = () => callbackPort.postMessage("releaseResumed");

  initCallbackReplyPort({ port: callbackPort });

  const [wasm, wasi] = await loadWasm({
    wasmDataURI,
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
      sabCreateRealtimeAudioThread({
        libraryCsound,
        callbacksRequest,
        wasm,
        workerMessagePort,
        releaseStop,
        releasePause,
        releaseResumed,
      }),
      renderFunction({
        libraryCsound,
        callbacksRequest,
        workerMessagePort,
        wasi,
        releaseStop,
        releasePause,
        releaseResumed,
      }),
    )(arguments_);

  const allAPI = pipe(assoc("csoundStart", startHandler), assoc("wasm", wasm))(libraryCsound);
  combined = new Map(Object.entries(allAPI));

  libraryCsound.csoundInitialize(0);
  const csoundInstance = libraryCsound.csoundCreate();

  return csoundInstance;
};

export const sabWorker = { initialize, callUncloned };

expose({ initialize: sabWorker.initialize, callUncloned: sabWorker.callUncloned });
