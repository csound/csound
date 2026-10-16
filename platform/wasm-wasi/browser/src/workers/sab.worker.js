/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as Comlink from "../utils/comlink.js";
import { createRealtimeControlChannels } from "../utils/realtime-control-channels.js";
import { assertRealtimeCallAllowed } from "../utils/realtime-api.js";
import { waitForSABResume, waitForSABOutput } from "../utils/sab-transport.js";
import { drainMidiPackets } from "../utils/sab-midi-queue.js";
import { tryPushMidiMessage } from "../utils/wasm-midi.js";
import { sharedAudioChannels } from "../utils/sab-audio-channels.js";
import MessagePortState from "../utils/message-port-state";
import libcsoundFactory from "../libcsound.js";
import loadWasm from "../module.loader.js";
import loadRealtimeWasm from "../module.realtime.js";
import { superviseWasiPerformance } from "../utils/wasi-threads.js";
import { logSABWorker as log } from "../logger";
import { handleCsoundStart } from "../workers/common.utils";
import { AUDIO_STATE, RING_BUFFER_SIZE, initialSharedState } from "../constants";

/**
 * @type {Map<string, *>}
 */
let combined;
let pollPromise;
let unlockPromise;
let threadedMode = false;
let activeGeneration = -1;
let realtimeControls;

const callUncloned = async (k, arguments_) => {
  const caller = combined.get(k);
  const returnValue = caller && caller.apply({}, arguments_ || []);
  return returnValue;
};

const applyUserProvidedAudioParams = ({ audioStateBuffer, csound, libraryCsound }) => {
  if (!audioStateBuffer) {
    return;
  }
  const audioStatePointer = new Int32Array(audioStateBuffer);
  const userProvidedNchnls = Atomics.load(audioStatePointer, AUDIO_STATE.NCHNLS);
  const userProvidedNchnlsIn = Atomics.load(audioStatePointer, AUDIO_STATE.NCHNLS_I);
  const userProvidedSr = Atomics.load(audioStatePointer, AUDIO_STATE.SAMPLE_RATE);

  if (userProvidedNchnls > -1) {
    const result = libraryCsound.csoundSetOption(csound, "--nchnls=" + userProvidedNchnls);
    result !== 0 && console.error("csoundSetOption nchnls failed:", result);
  }
  if (userProvidedNchnlsIn > -1) {
    const result = libraryCsound.csoundSetOption(csound, "--nchnls_i=" + userProvidedNchnlsIn);
    result !== 0 && console.error("csoundSetOption nchnls_i failed:", result);
  }
  if (userProvidedSr > -1) {
    const result = libraryCsound.csoundSetOption(csound, "--sample-rate=" + userProvidedSr);
    result !== 0 && console.error("csoundSetOption sample-rate failed:", result);
  }
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
  async (argumentz) => {
    const audioStateBuffer = argumentz["audioStateBuffer"];
    const audioStreamIn = argumentz["audioStreamIn"];
    const audioStreamOut = argumentz["audioStreamOut"];
    const midiBuffer = argumentz["midiBuffer"];
    const csound = argumentz["csound"];
    const performanceGeneration = argumentz["performanceGeneration"];

    const audioStatePointer = new Int32Array(audioStateBuffer);

    // Preserve user-provided audio config across state reset.
    const userProvidedNchnls = Atomics.load(audioStatePointer, AUDIO_STATE.NCHNLS);
    const userProvidedNchnlsIn = Atomics.load(audioStatePointer, AUDIO_STATE.NCHNLS_I);
    const userProvidedSr = Atomics.load(audioStatePointer, AUDIO_STATE.SAMPLE_RATE);

    // In case of multiple performances, let's reset the sab state
    initialSharedState.forEach((value, index) => {
      // The main thread resets MIDI counters before start; it may already be
      // publishing new packets while this worker initializes the other state.
      if (index !== AUDIO_STATE.RTMIDI_READ && index !== AUDIO_STATE.RTMIDI_WRITE)
        Atomics.store(audioStatePointer, index, value);
    });
    if (userProvidedNchnls > -1) {
      Atomics.store(audioStatePointer, AUDIO_STATE.NCHNLS, userProvidedNchnls);
    }
    if (userProvidedNchnlsIn > -1) {
      Atomics.store(audioStatePointer, AUDIO_STATE.NCHNLS_I, userProvidedNchnlsIn);
    }
    if (userProvidedSr > -1) {
      Atomics.store(audioStatePointer, AUDIO_STATE.SAMPLE_RATE, userProvidedSr);
    }

    // Prompt for midi-input on demand
    const isRequestingRtMidiInput = libraryCsound["isRequestingRtMidiInput"](csound);

    // Prompt for microphone only on demand!
    const isExpectingInput =
      Atomics.load(audioStatePointer, AUDIO_STATE.NCHNLS_I) === 0 &&
      libraryCsound.isRequestingRtAudioInput(csound);

    const nchnls = libraryCsound.csoundGetNchnls(csound);

    const nchnlsInput =
      userProvidedNchnlsIn || isExpectingInput ? libraryCsound.csoundGetNchnlsInput(csound) : 0;

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
    const channelsOutput = sharedAudioChannels(audioStreamOut, nchnls);
    const channelsInput = sharedAudioChannels(audioStreamIn, nchnlsInput);

    workerMessagePort.broadcastPlayState("realtimePerformanceStarted", performanceGeneration);
    // Let's notify the audio-worker that performance has started
    Atomics.store(audioStatePointer, AUDIO_STATE.IS_PERFORMING, 1);

    let firstRound = true;
    let lastReturn = 0;
    let currentCsoundBufferPos = 0;
    let currentInputReadIndex = 0;
    let currentOutputWriteIndex = 0;

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
        Atomics.store(audioStatePointer, AUDIO_STATE.IS_PERFORMING, 0);
        log(`triggering realtimePerformanceEnded event`)();
        workerMessagePort.broadcastPlayState("realtimePerformanceEnded", performanceGeneration);
        log(`End of realtimePerformance loop!`)();
        releaseStop(performanceGeneration, csound);
        return true;
      } else {
        return false;
      }
    };

    let firstPerformKsmps = true;

    while (true) {
      if (firstRound) {
        firstRound = false;
        await new Promise((resolve) => {
          unlockPromise = resolve;
          workerMessagePort.broadcastSabUnlocked();
        });

        log(`Shared audio performance started`)();
      }

      if (Atomics.load(audioStatePointer, AUDIO_STATE.IS_PAUSED) === 1) {
        waitForSABResume(
          audioStatePointer,
          () => releasePause(performanceGeneration),
          () => releaseResumed(performanceGeneration),
        );
      }

      if (maybeStop()) {
        return;
      }

      if (isRequestingRtMidiInput) {
        drainMidiPackets(audioStatePointer, midiBuffer, (status, data1, data2) =>
          tryPushMidiMessage(wasm, csound, status, data1, data2),
        );
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
        waitForSABOutput(audioStatePointer, readIndex);
      }
    }
  };

const initMessagePort = ({ port }) => {
  const workerMessagePort = new MessagePortState();
  workerMessagePort.post = (messageLog) => port.postMessage({ log: messageLog });
  workerMessagePort.broadcastPlayState = (playStateChange, performanceGeneration) => {
    const payload = {};
    payload["playStateChange"] = playStateChange;
    if (performanceGeneration !== undefined) {
      payload["performanceGeneration"] = performanceGeneration;
    }
    port.postMessage(payload);
  };
  workerMessagePort.broadcastSabUnlocked = () => port.postMessage({ sabWorker: "unlocked" });
  workerMessagePort.ready = true;
  return workerMessagePort;
};

const initCallbackReplyPort = (response) => {
  const port = response["port"];
  port.addEventListener("message", (event) => {
    if (event.data && event.data["unlock"]) {
      const unlockPromise_ = unlockPromise;
      unlockPromise = undefined;
      unlockPromise_ && unlockPromise_();
    } else {
      const callbacks = event.data;
      const answers = callbacks.reduce((accumulator, callerPayload) => {
        const id = callerPayload["id"];
        const argumentz = callerPayload["argumentz"];
        const apiKey = callerPayload["apiKey"];
        const caller = combined.get(apiKey);
        const answerPayload = {};
        answerPayload["id"] = id;
        try {
          if (threadedMode && callerPayload["generation"] !== activeGeneration)
            throw new Error("Call belongs to an ended performance");
          // Options and start can overlap on the UI. Check the native mode
          // here too, before a call admitted under an older mode can execute.
          const isRealtime = /** @type {function(number): number} */ (
            combined.get("csoundIsRealtime")
          );
          const realtime = threadedMode && isRealtime(argumentz[0]);
          if (realtime) assertRealtimeCallAllowed(apiKey, argumentz.slice(1));
          if (
            realtime &&
            (apiKey === "csoundGetControlChannel" || apiKey === "csoundSetControlChannel") &&
            !realtimeControls
          )
            throw new Error("Control channels are unavailable for this performance phase");
          if (realtime && apiKey === "csoundGetControlChannel")
            answerPayload["answer"] = realtimeControls.get(argumentz[1]);
          else if (realtime && apiKey === "csoundSetControlChannel")
            answerPayload["answer"] = realtimeControls.set(argumentz[1], argumentz[2]);
          else answerPayload["answer"] = caller && caller.apply({}, argumentz || []);
        } catch (error) {
          answerPayload["error"] = error instanceof Error ? error.message : String(error);
        }
        accumulator.push(answerPayload);
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
  async (argumentz) => {
    const audioStateBuffer = argumentz["audioStateBuffer"];
    const csound = argumentz["csound"];
    const performanceGeneration = argumentz["performanceGeneration"];
    const audioStatePointer = new Int32Array(audioStateBuffer);
    workerMessagePort.broadcastSabUnlocked();

    while (
      Atomics.load(audioStatePointer, AUDIO_STATE.STOP) !== 1 &&
      libraryCsound["csoundPerformKsmps"](csound) === 0
    ) {
      if (Atomics.load(audioStatePointer, AUDIO_STATE.IS_PAUSED) === 1) {
        waitForSABResume(
          audioStatePointer,
          () => releasePause(performanceGeneration),
          () => releaseResumed(performanceGeneration),
        );
      }
      if (Atomics.load(audioStatePointer, AUDIO_STATE.STOP) === 1) break;
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
    workerMessagePort.broadcastPlayState("renderEnded", performanceGeneration);
    releaseStop(performanceGeneration, csound);
  };

/** @export */
const initialize = async (payload) => {
  const wasmDataURI = payload["wasmDataURI"];
  const withPlugins = payload["withPlugins"] || [];
  const messagePort = payload["messagePort"];
  const callbackPort = payload["callbackPort"];

  log(`initializing SABWorker and WASM`)();
  const workerMessagePort = initMessagePort({ port: messagePort });
  const callbacksRequest = () => callbackPort.postMessage("poll");
  const releaseStop = (performanceGeneration, csound) => {
    activeGeneration = -1;
    realtimeControls = undefined;
    // Score end only stops queue admission. Join init and close native I/O
    // before the second completion signal releases pending host calls.
    if (threadedMode && libraryCsound.csoundIsRealtime(csound))
      wasm.exports["csoundCleanupWasi"](csound);
    const payload = {};
    payload["type"] = "releaseStop";
    payload["performanceGeneration"] = performanceGeneration;
    callbackPort.postMessage(payload);
  };
  const releaseTransport = (type, performanceGeneration) => {
    // Keep wire keys stable across separately compiled worker/main bundles.
    const payload = {};
    payload["type"] = type;
    payload["performanceGeneration"] = performanceGeneration;
    callbackPort.postMessage(payload);
  };
  const releasePause = (performanceGeneration) =>
    releaseTransport("releasePause", performanceGeneration);
  const releaseResumed = (performanceGeneration) =>
    releaseTransport("releaseResumed", performanceGeneration);

  initCallbackReplyPort({ port: callbackPort });

  const threaded = payload["threaded"] === true;
  threadedMode = threaded;
  const [wasm, wasi] = threaded
    ? await loadRealtimeWasm({
        wasmBytes: wasmDataURI,
        ports: payload["threadPorts"],
        mayBlock: true,
        messagePort: workerMessagePort,
        withPlugins,
      })
    : await loadWasm({ wasmDataURI, withPlugins, messagePort: workerMessagePort, mayBlock: true });

  wasm.wasi = wasi;

  const libraryCsound = libcsoundFactory(wasm);

  const supervise = (perform) =>
    threaded
      ? superviseWasiPerformance(perform, (error) => {
          globalThis.postMessage({
            "type": "wasi-thread-fatal",
            "message": error instanceof Error ? error.message : String(error),
          });
        })
      : perform;

  const startHandler = async (_, arguments_) => {
    // Mirror every plugin before native start can launch an init thread.
    if (threaded) await wasm["prepareThreads"](arguments_["csound"]);
    realtimeControls = undefined;
    activeGeneration = arguments_["performanceGeneration"];
    applyUserProvidedAudioParams({
      audioStateBuffer: arguments_ && arguments_["audioStateBuffer"],
      csound: arguments_ && arguments_["csound"],
      libraryCsound,
    });

    const status = handleCsoundStart(
      workerMessagePort,
      libraryCsound,
      wasi,
      supervise(
        sabCreateRealtimeAudioThread({
          libraryCsound,
          callbacksRequest,
          wasm,
          workerMessagePort,
          releaseStop,
          releasePause,
          releaseResumed,
        }),
      ),
      supervise(
        renderFunction({
          libraryCsound,
          callbacksRequest,
          workerMessagePort,
          wasi,
          releaseStop,
          releasePause,
          releaseResumed,
        }),
      ),
      () => {
        // Global chn_k declarations run inside csoundStart. Snapshot after
        // those inits, before performKsmps can feed the background init queue.
        if (threaded && libraryCsound.csoundIsRealtime(arguments_["csound"]))
          realtimeControls = createRealtimeControlChannels(wasm, arguments_["csound"]);
      },
    )(arguments_);
    if (status !== 0) realtimeControls = undefined;
    return threaded
      ? {
          // eslint-disable-next-line object-shorthand -- Keep wire keys across bundles.
          "status": status,
          "realtimeMode": Boolean(libraryCsound.csoundIsRealtime(arguments_["csound"])),
        }
      : status;
  };

  const allAPI = { ...libraryCsound, csoundStart: startHandler, wasm };
  combined = new Map(Object.entries(allAPI));

  libraryCsound["csoundInitialize"](0);
  const csoundInstance = libraryCsound["csoundCreate"]();

  const installProbe = wasm.exports["csoundWasiInstallThreadProbe"];
  if (threaded && typeof installProbe === "function") {
    const pointer = installProbe(csoundInstance);
    return {
      // eslint-disable-next-line object-shorthand -- Preserve keys across worker bundles.
      "csoundInstance": csoundInstance,
      "threadProbe": new Int32Array(wasi.memory.buffer, pointer, 4),
    };
  }
  return csoundInstance;
};

export const sabWorker = { initialize, callUncloned };

Comlink.expose({ initialize: sabWorker.initialize, callUncloned: sabWorker.callUncloned });
