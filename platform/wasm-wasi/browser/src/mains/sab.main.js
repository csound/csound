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
import { api as API } from "../libcsound";
import { messageEventHandler, IPCMessagePorts } from "./messages.main";
import {
  AUDIO_STATE,
  MAX_CHANNELS,
  RING_BUFFER_SIZE,
  MIDI_BUFFER_PAYLOAD_SIZE,
  MIDI_BUFFER_SIZE,
  initialSharedState,
} from "../constants";
import { logSABMain as log } from "../logger";
import { csoundApiRename, fetchPlugins, makeProxyCallback, stopableStates } from "../utils";
import { EventPromises } from "../utils/event-promises";
import { assertRealtimeCallAllowed, realtimeBackgroundOperations } from "../utils/realtime-api.js";
import { settleRealtimeApiCompletion } from "../utils/realtime-api-queue.js";
import { stopSAB, wakeSAB } from "../utils/sab-transport.js";
import { enqueueMidiPacket, resetMidiQueue } from "../utils/sab-midi-queue.js";
import { SABCompletionCoordinator } from "../utils/sab-completion-coordinator.js";
import { PublicEventAPI } from "../events";
import { enableAudioInputInWorker } from "./io.utils.js";
import SABWorker from "../../dist/__compiled.sab.worker.inline.js";
import WasiThreadWorker from "../../dist/__compiled.wasi-thread.worker.inline.js";

class SharedArrayBufferMainThread {
  constructor({
    audioContext,
    audioWorker,
    audioContextIsProvided,
    inputChannelCount,
    outputChannelCount,
  }) {
    this.csoundInstance = undefined;
    this.threaded = false;
    this.realtimeMode = false;
    this.threadWorkers = [];
    this.threadFailure = undefined;
    this.threadError = undefined;
    /** @type {function(!Error):void} */
    this.rejectThreadFailure = () => {};
    this.currentPlayState = undefined;
    this.wasmTransformerDataURI = undefined;
    /**
     * @type {SABMainProxy}
     * @suppress {checkTypes}
     */
    this.proxyPort = undefined;
    this.csoundWorker = undefined;

    this.hasSharedArrayBuffer = true;
    this.ipcMessagePorts = new IPCMessagePorts();
    this.eventPromises = new EventPromises();
    this.publicEvents = new PublicEventAPI(this);
    audioWorker.ipcMessagePorts = this.ipcMessagePorts;

    this.audioContextIsProvided = audioContextIsProvided;
    this.audioWorker = audioWorker;
    this.audioWorker.onPlayStateChange = this.audioWorker.onPlayStateChange.bind(audioWorker);
    this.currentDerivedPlayState = "stop";
    this.exportApi = {};

    this.callbackId = 0;
    this.callbackOrder = 0;
    this.callbackBuffer = {};
    // End state and stop release arrive on different ports. A generation keeps
    // late messages from an earlier run from completing the current one.
    this.performanceGeneration = 0;
    this.pendingPerformanceEnd = undefined;
    this.performanceCompletion = new SABCompletionCoordinator((performanceEndState) =>
      this.finishPerformanceEnd(performanceEndState),
    );

    this.audioStateBuffer = new SharedArrayBuffer(
      initialSharedState.length * Int32Array.BYTES_PER_ELEMENT,
    );

    this.audioStatePointer = new Int32Array(this.audioStateBuffer);

    // Always extract sample rate from audioContext to ensure Csound matches it
    if (audioContext) {
      Atomics.store(this.audioStatePointer, AUDIO_STATE.SAMPLE_RATE, audioContext.sampleRate);
    }

    if (inputChannelCount) {
      Atomics.store(this.audioStatePointer, AUDIO_STATE.NCHNLS_I, inputChannelCount);
    }

    if (outputChannelCount) {
      Atomics.store(this.audioStatePointer, AUDIO_STATE.NCHNLS, outputChannelCount);
    }

    this.audioStreamIn = new SharedArrayBuffer(
      MAX_CHANNELS * RING_BUFFER_SIZE * Float32Array.BYTES_PER_ELEMENT,
    );
    this.audioStreamOut = new SharedArrayBuffer(
      MAX_CHANNELS * RING_BUFFER_SIZE * Float32Array.BYTES_PER_ELEMENT,
    );

    this.midiBufferSAB = new SharedArrayBuffer(
      MIDI_BUFFER_SIZE * MIDI_BUFFER_PAYLOAD_SIZE * Int32Array.BYTES_PER_ELEMENT,
    );

    this.midiBuffer = new Int32Array(this.midiBufferSAB);

    this.onPlayStateChange = this.onPlayStateChange.bind(this);
    this.prepareRealtimePerformance = this.prepareRealtimePerformance.bind(this);

    log(`SharedArrayBufferMainThread got constructed`)();
  }

  failThreadedInstance(error) {
    if (this.threadError) return;
    this.threadError = error;
    this.pendingPerformanceEnd = undefined;
    stopSAB(this.audioStatePointer);
    this.rejectThreadFailure(error);
    for (const payload of Object.values(this.callbackBuffer)) payload["rejectCallback"](error);
    this.callbackBuffer = {};
    try {
      this.publicEvents.triggerMessage({ log: error.message });
    } finally {
      // Application listeners must not keep failed native threads alive.
      this.eventPromises.releaseStartPromise();
      this.eventPromises.releaseStopPromise();
      this.eventPromises.releasePausePromise();
      this.eventPromises.releaseResumePromise();
      this.terminateInstance().catch((cleanupError) => console.error(cleanupError));
    }
  }

  guardThreadedCall(promise) {
    return this.threadFailure ? Promise.race([promise, this.threadFailure]) : promise;
  }

  async refreshRealtimeMode() {
    if (!this.threaded) return;
    const active =
      stopableStates.has(this.currentPlayState) || this.eventPromises.isWaitingToStart();
    this.realtimeMode = Boolean(
      await (active
        ? this.enqueueCallback("csoundIsRealtime", [this.csoundInstance])
        : this.guardThreadedCall(
            this.proxyPort["callUncloned"]("csoundIsRealtime", [this.csoundInstance]),
          )),
    );
  }

  enqueueCallback(apiKey, arguments_) {
    if (this.threaded) {
      if (Atomics.load(this.audioStatePointer, AUDIO_STATE.IS_PAUSED) === 1)
        throw new Error("Resume performance before calling engine functions");
      if (Object.keys(this.callbackBuffer).length >= 256)
        throw new Error("Threaded control queue is full");
    }
    while (Object.hasOwn(this.callbackBuffer, String(this.callbackId)))
      this.callbackId = (this.callbackId + 1) >>> 0;
    const callbackId = this.callbackId;
    this.callbackId = (this.callbackId + 1) >>> 0;
    if (Object.keys(this.callbackBuffer).length === 0) this.callbackOrder = 0;
    const promise = new Promise((resolve, reject) => {
      // Compilation/init may take arbitrarily long on the shared host. Do not
      // gate this on cached realtimeMode: options/start replies can lag native
      // admission. Completion, stop cancellation, or worker failure settles it.
      const timeout =
        this.threaded && realtimeBackgroundOperations.has(apiKey)
          ? undefined
          : setTimeout(() => {
              delete this.callbackBuffer[callbackId];
              reject(
                new Error(
                  `Worker timed out during ${csoundApiRename(apiKey)}(); dispatched work may have run`,
                ),
              );
              this.maybeFinishPerformanceEnd();
            }, 10000 + this.eventPromises.getStartRemainingMs());
      this.callbackBuffer[callbackId] = {
        "resolveCallback": (answer) => {
          clearTimeout(timeout);
          resolve(answer);
        },
        "rejectCallback": (error) => {
          clearTimeout(timeout);
          reject(error);
        },
        // eslint-disable-next-line object-shorthand -- Keep wire keys across bundles.
        "apiKey": apiKey,
        "order": this.callbackOrder++,
        "generation": this.performanceGeneration,
        "argumentz": arguments_,
      };
    });
    Atomics.compareExchange(this.audioStatePointer, AUDIO_STATE.HAS_PENDING_CALLBACKS, 0, 1);
    wakeSAB(this.audioStatePointer);
    return this.guardThreadedCall(promise);
  }

  takeCallbacks() {
    return (
      Object.entries(this.callbackBuffer)
        .filter(([_id, payload]) => !payload["inFlight"])
        // Integer object keys sort by value, which reorders edits at ID rollover.
        .sort((left, right) => left[1]["order"] - right[1]["order"])
        .slice(0, this.threaded ? 32 : undefined)
        .map(([id, payload]) => {
          payload["inFlight"] = this.threaded;
          const request = {};
          request["id"] = id;
          request["apiKey"] = payload["apiKey"];
          request["argumentz"] = payload["argumentz"];
          request["generation"] = payload["generation"];
          return request;
        })
    );
  }

  settleCallbackReplies(replies) {
    for (const payload of replies) {
      const pending = this.callbackBuffer[payload["id"]];
      if (!pending || pending["generation"] !== payload["generation"]) continue;
      if (payload["error"]) pending["rejectCallback"](new Error(payload["error"]));
      else pending["resolveCallback"](payload["answer"]);
      delete this.callbackBuffer[payload["id"]];
    }
    this.maybeFinishPerformanceEnd();
    if (Object.values(this.callbackBuffer).some((entry) => !entry["inFlight"])) {
      Atomics.store(this.audioStatePointer, AUDIO_STATE.HAS_PENDING_CALLBACKS, 1);
      wakeSAB(this.audioStatePointer);
    }
  }

  async terminateInstance() {
    this.pendingPerformanceEnd = undefined;
    this.ipcMessagePorts.close();
    if (this.threaded) {
      stopSAB(this.audioStatePointer);
      if (!this.threadError) {
        this.threadError = new Error("Threaded Csound instance terminated");
        this.rejectThreadFailure(this.threadError);
      }
      for (const payload of Object.values(this.callbackBuffer))
        payload["rejectCallback"](this.threadError);
      this.callbackBuffer = {};
      this.eventPromises.releaseStartPromise();
      this.eventPromises.releaseStopPromise();
      this.eventPromises.releasePausePromise();
      this.eventPromises.releaseResumePromise();
    }
    if (this.threadWorkers) {
      for (const worker of this.threadWorkers) worker.terminate();
      this.threadWorkers = [];
    }
    if (this.csoundWorker) {
      this.csoundWorker.terminate();
      delete this.csoundWorker;
    }
    if (this.proxyPort) {
      this.proxyPort[Comlink.releaseProxy](false);
      delete this.proxyPort;
    }
    if (this.audioWorker && this.audioWorker.terminateInstance) {
      await this.audioWorker.terminateInstance();
      delete this.audioWorker.terminateInstance;
    }
    if (this.publicEvents) {
      this.publicEvents.terminateInstance();
    }
  }

  get api() {
    return this.exportApi;
  }

  handleMidiInput({ data: [status, data1, data2] }) {
    return enqueueMidiPacket(this.audioStatePointer, this.midiBuffer, status, data1, data2);
  }

  async csoundPause() {
    if (this.threadError) throw this.threadError;
    if (
      this.eventPromises.isWaiting("pause") ||
      Atomics.load(this.audioStatePointer, AUDIO_STATE.STOP) === 1 ||
      (Atomics.load(this.audioStatePointer, AUDIO_STATE.IS_PERFORMING) !== 1 &&
        Atomics.load(this.audioStatePointer, AUDIO_STATE.IS_RENDERING) !== 1)
    )
      return -1;
    if (Atomics.load(this.audioStatePointer, AUDIO_STATE.IS_PAUSED) === 1) return 0;
    this.eventPromises.createPausePromise();
    Atomics.store(this.audioStatePointer, AUDIO_STATE.IS_PAUSED, 1);
    wakeSAB(this.audioStatePointer);
    await this.guardThreadedCall(this.eventPromises.waitForPause());
    // Only the worker acknowledgement publishes the transition.
    return 0;
  }

  async csoundResume() {
    if (this.threadError) throw this.threadError;
    if (this.eventPromises.isWaiting("resume")) return -1;
    if (
      Atomics.load(this.audioStatePointer, AUDIO_STATE.IS_PAUSED) === 1 &&
      Atomics.load(this.audioStatePointer, AUDIO_STATE.STOP) !== 1 &&
      (Atomics.load(this.audioStatePointer, AUDIO_STATE.IS_PERFORMING) === 1 ||
        Atomics.load(this.audioStatePointer, AUDIO_STATE.IS_RENDERING) === 1)
    ) {
      this.eventPromises.createResumePromise();
      Atomics.store(this.audioStatePointer, AUDIO_STATE.IS_PAUSED, 0);
      Atomics.notify(this.audioStatePointer, AUDIO_STATE.IS_PAUSED);
      await this.guardThreadedCall(this.eventPromises.waitForResume());
    }
    return 0;
  }

  markPerformanceEndState(playState, performanceGeneration) {
    this.performanceCompletion.markEnd(playState, performanceGeneration);
  }

  markStopReleaseReceived(performanceGeneration) {
    if (!this.performanceCompletion.canAccept(performanceGeneration)) {
      return;
    }
    // Either completion signal can arrive first. Establish the stop barrier
    // before recording this one so a new run cannot reset partial state.
    this.eventPromises.createStopPromise();
    this.performanceCompletion.markStopReleased(performanceGeneration);
  }

  finishPerformanceEnd(performanceEndState) {
    // Both completion signals have arrived, including any earlier callback
    // replies. Send calls the worker did not pick up before it finished.
    for (const [id, payload] of Object.entries(this.callbackBuffer)) {
      if (this.realtimeMode) {
        // Native stop joined the API worker, but its direct replies use another
        // port. Wait for those replies before publishing stop/reset completion.
        if (payload["inFlight"] && realtimeBackgroundOperations.has(payload["apiKey"])) continue;
        payload["rejectCallback"](new Error("Performance ended before the call completed"));
      } else {
        this.proxyPort["callUncloned"](payload["apiKey"], payload["argumentz"]).then(
          payload["resolveCallback"],
          payload["rejectCallback"],
        );
      }
      delete this.callbackBuffer[id];
    }
    this.pendingPerformanceEnd = performanceEndState;
    this.maybeFinishPerformanceEnd();
  }

  maybeFinishPerformanceEnd() {
    if (!this.pendingPerformanceEnd || Object.keys(this.callbackBuffer).length > 0) return;
    const performanceEndState = this.pendingPerformanceEnd;
    this.pendingPerformanceEnd = undefined;

    // A score can finish before acknowledging pause or resume. Settle both
    // here too: releasing stop clears their timeout timers.
    if (this.eventPromises) {
      this.eventPromises.releasePausePromise();
      this.eventPromises.releaseResumePromise();
    }

    // Logs and play-state changes share one ordered port. releaseStop uses a
    // second port, so wait for both before exposing completion to callers.
    if (performanceEndState === "renderEnded") {
      this.publicEvents && this.publicEvents.triggerRenderEnded();
    } else {
      this.publicEvents && this.publicEvents.triggerRealtimePerformanceEnded();
    }
    this.eventPromises && this.eventPromises.releaseStopPromise();
  }

  async onPlayStateChange(newPlayState, performanceGeneration) {
    if (this === undefined) {
      console.log("Failed to announce playstatechange", newPlayState);
      return;
    }
    if (!this.performanceCompletion.canAccept(performanceGeneration)) {
      return;
    }
    this.currentPlayState = newPlayState;
    if (!this.publicEvents || !newPlayState) {
      // prevent late timers from calling terminated fn
      return;
    }
    let performanceEndState;
    let userProvidedSampleRate;
    let userProvidedNchnls;
    let userProvidedNchnlsInput;
    switch (newPlayState) {
      case "realtimePerformanceStarted": {
        log(
          `event: realtimePerformanceStarted received,` +
            ` proceeding to call prepareRealtimePerformance`,
        )();
        try {
          await this.prepareRealtimePerformance();
        } catch (error) {
          console.error(error);
        }
        break;
      }
      case "realtimePerformanceEnded": {
        this.eventPromises.createStopPromise();

        log(`event: realtimePerformanceEnded received, beginning cleanup`)();
        // Preserve audio configuration while re-initializing SAB runtime state.
        userProvidedSampleRate = Atomics.load(this.audioStatePointer, AUDIO_STATE.SAMPLE_RATE);
        userProvidedNchnls = Atomics.load(this.audioStatePointer, AUDIO_STATE.NCHNLS);
        userProvidedNchnlsInput = Atomics.load(this.audioStatePointer, AUDIO_STATE.NCHNLS_I);
        performanceEndState = newPlayState;
        break;
      }
      case "renderStarted": {
        this.publicEvents.triggerRenderStarted();
        this.eventPromises.releaseStartPromise();
        break;
      }
      case "renderEnded": {
        this.eventPromises.createStopPromise();
        log(`event: renderEnded received, beginning cleanup`)();
        performanceEndState = newPlayState;
        break;
      }
      default: {
        break;
      }
    }

    // forward the message from worker to the audioWorker
    try {
      await this.audioWorker.onPlayStateChange(newPlayState, performanceGeneration);
    } catch (error) {
      if (this.threaded) {
        this.failThreadedInstance(error);
        return;
      }
      console.error(error);
    }
    if (performanceEndState === "realtimePerformanceEnded") {
      initialSharedState.forEach((value, index) => {
        Atomics.store(this.audioStatePointer, index, value);
      });
      if (userProvidedSampleRate > -1) {
        Atomics.store(this.audioStatePointer, AUDIO_STATE.SAMPLE_RATE, userProvidedSampleRate);
      }
      if (userProvidedNchnls > -1) {
        Atomics.store(this.audioStatePointer, AUDIO_STATE.NCHNLS, userProvidedNchnls);
      }
      if (userProvidedNchnlsInput > -1) {
        Atomics.store(this.audioStatePointer, AUDIO_STATE.NCHNLS_I, userProvidedNchnlsInput);
      }
    }
    if (performanceEndState) {
      this.markPerformanceEndState(performanceEndState, performanceGeneration);
    }
  }

  async prepareRealtimePerformance() {
    log(`prepareRealtimePerformance`)();
    const outputsCount = Atomics.load(this.audioStatePointer, AUDIO_STATE.NCHNLS);
    const inputCount = Atomics.load(this.audioStatePointer, AUDIO_STATE.NCHNLS_I);

    this.audioWorker.isRequestingInput = Atomics.load(
      this.audioStatePointer,
      AUDIO_STATE.IS_REQUESTING_MIC,
    );
    this.audioWorker["isRequestingMidi"] = Atomics.load(
      this.audioStatePointer,
      AUDIO_STATE.IS_REQUESTING_RTMIDI,
    );

    const ksmps = Atomics.load(this.audioStatePointer, AUDIO_STATE.KSMPS);
    const sampleRate = Atomics.load(this.audioStatePointer, AUDIO_STATE.SAMPLE_RATE);

    this.audioWorker.ksmps = ksmps;
    this.audioWorker.sampleRate = sampleRate;
    this.audioWorker.inputCount = inputCount;
    this.audioWorker.outputsCount = outputsCount;
  }

  async initialize({ wasmDataURI, withPlugins, threadedBytes }) {
    this.threaded = Boolean(threadedBytes);
    this.threadWorkers = [];
    const threadPorts = [];
    if (this.threaded) {
      this.eventPromises = new EventPromises((event) =>
        this.failThreadedInstance(
          new Error(`Threaded ${event} timed out; the instance was terminated`),
        ),
      );
      this.threadFailure = new Promise((_resolve, reject) => {
        this.rejectThreadFailure = reject;
      });
      // Some callers only listen for messages. Keep terminal failure observed
      // even when there is no outstanding API call to race it against.
      this.threadFailure.catch(() => {});
      const onThreadMessage = messageEventHandler(this);
      for (let index = 0; index < 4; index++) {
        const url = WasiThreadWorker();
        const worker = new Worker(url);
        URL.revokeObjectURL(url);
        this.threadWorkers.push(worker);
        worker.addEventListener("error", (event) =>
          this.failThreadedInstance(new Error(event.message)),
        );
        worker.addEventListener("message", ({ data }) => {
          switch (data["type"]) {
            case "wasi-thread-fatal": {
              this.failThreadedInstance(new Error(data["message"]));
              break;
            }
            case "wasi-api-completion": {
              settleRealtimeApiCompletion(this.callbackBuffer, data);
              this.maybeFinishPerformanceEnd();
              break;
            }
            case "wasi-thread-message": {
              onThreadMessage({ data: { log: data["message"] } });
              break;
            }
            default: {
              break;
            }
          }
        });
        const channel = new MessageChannel();
        worker.postMessage({ type: "connect", port: channel.port1 }, [channel.port1]);
        threadPorts.push(channel.port2);
      }
    }
    if (withPlugins && withPlugins.length > 0) {
      withPlugins = await fetchPlugins(withPlugins);
    }

    log(`initialization: instantiate the SABWorker Thread`)();
    const csoundWorker = new Worker(SABWorker());
    this.csoundWorker = csoundWorker;
    if (this.threaded) {
      csoundWorker.addEventListener("error", (event) =>
        this.failThreadedInstance(new Error(event.message)),
      );
      csoundWorker.addEventListener("message", ({ data }) => {
        if (data["type"] === "wasi-thread-fatal")
          this.failThreadedInstance(new Error(data["message"]));
      });
    }
    const audioStateBuffer = this.audioStateBuffer;
    const audioStatePointer = this.audioStatePointer;
    const audioStreamIn = this.audioStreamIn;
    const audioStreamOut = this.audioStreamOut;
    const midiBuffer = this.midiBuffer;

    log(`providing the audioWorker a pointer to SABMain's instance`)();
    this.audioWorker.csoundWorkerMain = this;

    // both audio worker and csound worker use 1 handler
    // simplifies flow of data (csound main.worker is always first to receive)
    log(`adding message eventListeners for mainMessagePort and mainMessagePortAudio`)();
    this.ipcMessagePorts.mainMessagePort.addEventListener("message", messageEventHandler(this));
    this.ipcMessagePorts.mainMessagePort.start();
    this.ipcMessagePorts.mainMessagePortAudio.addEventListener(
      "message",
      messageEventHandler(this),
    );

    this.ipcMessagePorts.mainMessagePortAudio.start();
    log(`(postMessage) making a message channel from SABMain to SABWorker via workerMessagePort`)();

    this.ipcMessagePorts.sabMainCallbackReply.addEventListener("message", (event) => {
      if (event.data && event.data["type"] === "releaseStop") {
        this.markStopReleaseReceived(event.data["performanceGeneration"]);
        return;
      }
      const type = event.data && event.data["type"];
      if (type === "releasePause" || type === "releaseResumed") {
        if (
          this.performanceCompletion.canAccept(event.data["performanceGeneration"]) &&
          !this.eventPromises.isWaitingToStop() &&
          Atomics.load(this.audioStatePointer, AUDIO_STATE.STOP) !== 1
        ) {
          this.onPlayStateChange(
            type === "releasePause" ? "realtimePerformancePaused" : "realtimePerformanceResumed",
            event.data["performanceGeneration"],
          );
        }
        return;
      }
      switch (event.data) {
        case "poll": {
          if (this.ipcMessagePorts && this.ipcMessagePorts.sabMainCallbackReply) {
            this.ipcMessagePorts.sabMainCallbackReply.postMessage(this.takeCallbacks());
          }

          break;
        }
        case "releaseStop": {
          // Untagged releases cannot be attributed to a performance. Current
          // workers always send the object payload handled above.
          break;
        }
        case "releasePause":
        case "releaseResumed": {
          // Ignore untagged releases from workers without generation metadata.
          break;
        }
        default: {
          this.settleCallbackReplies(event.data);
        }
      }
    });

    this.ipcMessagePorts.sabMainCallbackReply.start();

    const proxyPort = Comlink.wrap(csoundWorker, undefined);
    const wasmBytes = threadedBytes || wasmDataURI();
    this.proxyPort = proxyPort;

    const initializePayload = {};
    initializePayload["wasmDataURI"] = wasmBytes;
    initializePayload["wasmTransformerDataURI"] = this.wasmTransformerDataURI;
    initializePayload["messagePort"] = this.ipcMessagePorts.workerMessagePort;
    initializePayload["callbackPort"] = this.ipcMessagePorts.sabWorkerCallbackReply;
    initializePayload["withPlugins"] = withPlugins;
    initializePayload["threaded"] = this.threaded;
    initializePayload["threadPorts"] = threadPorts;

    const initialized = await this.guardThreadedCall(
      proxyPort["initialize"](
        Comlink.transfer(initializePayload, [
          wasmBytes,
          this.ipcMessagePorts.workerMessagePort,
          this.ipcMessagePorts.sabWorkerCallbackReply,
          ...threadPorts,
        ]),
      ),
    );
    const csoundInstance =
      typeof initialized === "number" ? initialized : initialized["csoundInstance"];
    if (typeof initialized === "object" && initialized["threadProbe"])
      this.exportApi["__threadProbe"] = initialized["threadProbe"];
    this.csoundInstance = csoundInstance;

    // Ensure Csound aligns with requested audio setup before user compilation/start.
    const userProvidedSampleRate = Atomics.load(audioStatePointer, AUDIO_STATE.SAMPLE_RATE);
    const userProvidedNchnls = Atomics.load(audioStatePointer, AUDIO_STATE.NCHNLS);
    const userProvidedNchnlsInput = Atomics.load(audioStatePointer, AUDIO_STATE.NCHNLS_I);

    if (userProvidedSampleRate > -1) {
      await this.guardThreadedCall(
        proxyPort["callUncloned"]("csoundSetOption", [
          csoundInstance,
          "--sample-rate=" + userProvidedSampleRate,
        ]),
      );
    }
    if (userProvidedNchnls > -1) {
      await this.guardThreadedCall(
        proxyPort["callUncloned"]("csoundSetOption", [
          csoundInstance,
          "--nchnls=" + userProvidedNchnls,
        ]),
      );
    }
    if (userProvidedNchnlsInput > -1) {
      await this.guardThreadedCall(
        proxyPort["callUncloned"]("csoundSetOption", [
          csoundInstance,
          "--nchnls_i=" + userProvidedNchnlsInput,
        ]),
      );
    }

    this.ipcMessagePorts.mainMessagePort.start();
    this.ipcMessagePorts.mainMessagePortAudio.start();

    log(`A proxy port from SABMain to SABWorker established`)();

    this.exportApi["pause"] = this.csoundPause.bind(this);
    this.exportApi["resume"] = this.csoundResume.bind(this);
    this.exportApi["terminateInstance"] = this.terminateInstance.bind(this);
    this.exportApi["enableAudioInput"] = enableAudioInputInWorker.bind(this);

    this.exportApi["name"] = "Csound: Audio Worklet, Shared-Array Buffer";

    this.exportApi["getNode"] = async () => {
      if (this.threadError) throw this.threadError;
      const maybeNode = this.audioWorker.audioWorkletNode;
      if (maybeNode) {
        return maybeNode;
      } else {
        const node = await this.guardThreadedCall(
          new Promise((resolve) => {
            this.exportApi.once("onAudioNodeCreated", resolve);
          }),
        );
        return node;
      }
    };

    this.exportApi["getAudioContext"] = async () => this.audioWorker.audioContext;

    this.exportApi = this.publicEvents.decorateAPI(this.exportApi);

    // the default message listener
    this.exportApi.addListener("message", console.log);

    for (const apiKey of Object.keys(API)) {
      const proxyCallback = makeProxyCallback(
        proxyPort,
        csoundInstance,
        apiKey,
        this.currentPlayState,
      );
      const reference = API[apiKey];

      switch (apiKey) {
        case "csoundCreate": {
          break;
        }
        case "csoundStart": {
          const csoundStart = async function () {
            if (this.threadError) throw this.threadError;
            // Natural end can close audio while native cleanup still joins
            // init. Do not replace that performance's completion generation.
            if (this.eventPromises.isWaitingToStop())
              await this.guardThreadedCall(this.eventPromises.waitForStop());
            if (!csoundInstance || typeof csoundInstance !== "number") {
              console.error("starting csound failed because csound instance wasn't created");
              return -1;
            }
            if (
              (this.threaded && stopableStates.has(this.currentPlayState)) ||
              this.eventPromises.isWaiting("start")
            ) {
              return -1;
            } else {
              this.performanceGeneration = (this.performanceGeneration + 1) >>> 0;
              this.performanceCompletion.begin(this.performanceGeneration);
              // The first pooled start links plugins in every child. Allow
              // its 30-second readiness deadline to report a setup failure.
              this.eventPromises.createStartPromise(this.threaded ? 35000 : 2000);

              // Reset on the producer thread before dispatch. The worker must
              // not tear this pair while MIDI arrives during startup.
              resetMidiQueue(audioStatePointer);
              const startPayload = {};
              startPayload["audioStateBuffer"] = audioStateBuffer;
              startPayload["audioStreamIn"] = audioStreamIn;
              startPayload["audioStreamOut"] = audioStreamOut;
              startPayload["midiBuffer"] = midiBuffer;
              startPayload["csound"] = csoundInstance;
              startPayload["performanceGeneration"] = this.performanceGeneration;

              let startReply;
              try {
                startReply = await this.guardThreadedCall(proxyCallback(startPayload));
              } catch (error) {
                // Plugin preparation can fail before native start. Settle the
                // queued calls now instead of leaving them behind a start wait.
                for (const pending of Object.values(this.callbackBuffer))
                  pending["rejectCallback"](error);
                this.callbackBuffer = {};
                this.eventPromises.releaseStartPromise();
                throw error;
              }
              const startResult = this.threaded ? startReply["status"] : startReply;
              if (this.threaded) this.realtimeMode = startReply["realtimeMode"];
              if (startResult !== 0) {
                if (this.threaded) {
                  for (const pending of Object.values(this.callbackBuffer))
                    pending["rejectCallback"](new Error("Threaded performance did not start"));
                  this.callbackBuffer = {};
                }
                this.eventPromises.releaseStartPromise();
                return startResult;
              }

              await this.guardThreadedCall(this.eventPromises.waitForStart());

              // Worker startup resets the shared state. Republish calls that
              // arrived during startup before letting the render loop proceed.
              if (Object.keys(this.callbackBuffer).length > 0)
                Atomics.store(audioStatePointer, AUDIO_STATE.HAS_PENDING_CALLBACKS, 1);
              this.ipcMessagePorts &&
                this.ipcMessagePorts.sabMainCallbackReply.postMessage({ unlock: true });

              return startResult;
            }
          };

          csoundStart["toString"] = () => reference["toString"]();
          this.exportApi.start = csoundStart.bind(this);
          break;
        }
        case "csoundStop": {
          const csoundStop = async () => {
            if (this.threadError) throw this.threadError;
            log(
              [
                "Checking if it's safe to call stop:",
                stopableStates.has(this.currentPlayState),
                "currentPlayState is",
                this.currentPlayState,
              ].join("\n"),
            )();
            // Natural completion may have already started cleanup.
            if (this.eventPromises.isWaitingToStop()) {
              await this.guardThreadedCall(this.eventPromises.waitForStop());
              return 0;
            }
            if (this.eventPromises.isWaitingToStart()) {
              log("cannot stop while starting")();
              return -1;
            } else if (stopableStates.has(this.currentPlayState)) {
              log("Marking SAB's state to STOP")();

              stopSAB(this.audioStatePointer);
              // Stop takes precedence over an in-flight pause/resume. Settle
              // those callers without publishing a false transport event.
              this.eventPromises.releasePausePromise();
              this.eventPromises.releaseResumePromise();
              this.eventPromises.createStopPromise();
              await this.guardThreadedCall(this.eventPromises.waitForStop());
              return 0;
            } else {
              return -1;
            }
          };
          this.exportApi.stop = csoundStop.bind(this);
          csoundStop["toString"] = () => reference["toString"]();
          break;
        }

        case "csoundReset": {
          const csoundReset = async () => {
            if (this.threadError) throw this.threadError;
            // Thread/plugin preparation yields before native start. A reset
            // must wait until that start has either completed or failed.
            if (this.eventPromises.isWaitingToStart())
              await this.guardThreadedCall(this.eventPromises.waitForStart());
            if (!this.currentPlayState) {
              if (!this.threaded) return;
              // Reset options even before the first start, without replacing
              // audio ports that the worker has not received yet.
              const result = await this.guardThreadedCall(proxyCallback([]));
              await this.refreshRealtimeMode();
              return result;
            }

            await this.guardThreadedCall(this.eventPromises.waitForStop());
            if (this.eventPromises.isWaiting("reset")) {
              return -1;
            } else {
              if (stopableStates.has(this.currentPlayState)) {
                await this.exportApi.stop();
              }
              this.ipcMessagePorts.restartAudioWorkerPorts();
              if (!this.audioContextIsProvided) {
                await this.audioWorker.terminateInstance();
                delete this.audioWorker.audioContext;
              }
              const resetResult = await this.guardThreadedCall(proxyCallback([]));
              await this.refreshRealtimeMode();
              return resetResult;
            }
          };
          this.exportApi.reset = csoundReset.bind(this);
          csoundReset["toString"] = () => reference["toString"]();
          break;
        }
        case "csoundPushMidiMessage": {
          const midiMessage = async (status = 0, data1 = 0, data2 = 0) => {
            if (this.threadError) throw this.threadError;
            if (!this.handleMidiInput({ data: [status, data1, data2] }))
              throw new Error("MIDI queue is full");
          };
          this.exportApi["midiMessage"] = midiMessage.bind(this);
          midiMessage["toString"] = () => reference["toString"]();
          break;
        }

        case "fs": {
          this.exportApi["fs"] = {};
          Object.keys(reference).forEach((method) => {
            const proxyFsCallback = makeProxyCallback(
              proxyPort,
              csoundInstance,
              method,
              this.currentPlayState,
            );
            proxyFsCallback["toString"] = () => reference[method]["toString"]();
            this.exportApi["fs"][method] = async (...args) => {
              if (this.threadError) throw this.threadError;
              const active =
                stopableStates.has(this.currentPlayState) || this.eventPromises.isWaitingToStart();
              if (this.realtimeMode && active)
                throw new Error("Filesystem calls require stopped realtime performance");
              return this.threaded && active
                ? this.enqueueCallback(method, [csoundInstance, ...args])
                : this.guardThreadedCall(proxyFsCallback(...args));
            };
          });
          break;
        }

        default: {
          // avoiding deadlock by sending the IPC callback
          // while thread is unlocked
          const bufferWrappedCallback = async (...arguments_) => {
            if (this.threadError) throw this.threadError;
            const active =
              stopableStates.has(this.currentPlayState) || this.eventPromises.isWaitingToStart();
            if (this.realtimeMode && active) assertRealtimeCallAllowed(apiKey, arguments_);
            if (
              (this.threaded && active) ||
              this.currentPlayState === "realtimePerformanceStarted" ||
              this.currentPlayState === "realtimePerformanceResumed" ||
              this.currentPlayState === "renderStarted" ||
              this.eventPromises.isWaitingToStart()
              // startPromiz indicates that startup is in progress
              // and any events send before it's resolved are swallowed
            ) {
              return this.enqueueCallback(apiKey, [csoundInstance, ...arguments_]);
            } else {
              const result = await this.guardThreadedCall(
                proxyCallback.apply(undefined, arguments_),
              );
              // CSD options and rejected setOption calls make requested text
              // unreliable. Read the setting that the native engine accepted.
              if (
                [
                  "csoundSetOption",
                  "csoundCompileCSD",
                  "csoundCompileOrc",
                  "csoundSetParams",
                ].includes(apiKey)
              )
                await this.refreshRealtimeMode();
              return result;
            }
          };
          bufferWrappedCallback["toString"] = () => reference["toString"]();
          this.exportApi[csoundApiRename(apiKey)] = bufferWrappedCallback;
          break;
        }
      }
    }

    this.exportApi["enableDebugCallback"] = async () => {
      const fn = this.exportApi["setDebugCallbackWasi"];
      return typeof fn === "function" ? await fn() : -1;
    };

    log(`PUBLIC API Generated and stored`)();
  }
}

export default SharedArrayBufferMainThread;
