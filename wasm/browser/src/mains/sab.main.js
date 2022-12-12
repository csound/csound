import * as Comlink from "comlink/dist/esm/comlink.mjs";
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
import { isEmpty } from "rambda/dist/rambda.mjs";
import { csoundApiRename, fetchPlugins, makeProxyCallback, stopableStates } from "../utils";
import { EventPromises } from "../utils/event-promises";
import { PublicEventAPI } from "../events";
import SABWorker from "../../dist/__compiled.sab.worker.inline.js";

class SharedArrayBufferMainThread {
  constructor({
    audioContext,
    audioWorker,
    audioContextIsProvided,
    inputChannelCount,
    outputChannelCount,
  }) {
    this.hasSharedArrayBuffer = true;
    this.ipcMessagePorts = new IPCMessagePorts();
    this.eventPromises = new EventPromises();
    this.publicEvents = new PublicEventAPI(this);
    audioWorker.ipcMessagePorts = this.ipcMessagePorts;

    this.audioContextIsProvided = audioContextIsProvided;
    this.audioWorker = audioWorker;
    this.audioWorker.onPlayStateChange = this.audioWorker.onPlayStateChange.bind(audioWorker);
    this.csoundInstance = undefined;
    this.currentPlayState = undefined;
    this.currentDerivedPlayState = "stop";
    this.exportApi = {};

    this.callbackId = 0;
    this.callbackBuffer = {};

    this.audioStateBuffer = new SharedArrayBuffer(
      initialSharedState.length * Int32Array.BYTES_PER_ELEMENT,
    );

    this.audioStatePointer = new Int32Array(this.audioStateBuffer);

    if (audioContextIsProvided) {
      Atomics.store(this.audioStatePointer, AUDIO_STATE.SAMPLE_RATE, audioContext.sampleRate);
    }

    if (inputChannelCount) {
      Atomics.store(this.audioStatePointer, AUDIO_STATE.NCHNLS_I, inputChannelCount);
    }

    if (outputChannelCount) {
      Atomics.store(this.audioStatePointer, AUDIO_STATE.NCHNLS, outputChannelCount);
    }

    this.audioStreamIn = new SharedArrayBuffer(
      MAX_CHANNELS * RING_BUFFER_SIZE * Float64Array.BYTES_PER_ELEMENT,
    );
    this.audioStreamOut = new SharedArrayBuffer(
      MAX_CHANNELS * RING_BUFFER_SIZE * Float64Array.BYTES_PER_ELEMENT,
    );

    this.midiBufferSAB = new SharedArrayBuffer(
      MIDI_BUFFER_SIZE * MIDI_BUFFER_PAYLOAD_SIZE * Int32Array.BYTES_PER_ELEMENT,
    );

    this.midiBuffer = new Int32Array(this.midiBufferSAB);

    this.onPlayStateChange = this.onPlayStateChange.bind(this);
    this.prepareRealtimePerformance = this.prepareRealtimePerformance.bind(this);

    log(`SharedArrayBufferMainThread got constructed`)();
  }

  async terminateInstance() {
    if (this.csoundWorker) {
      this.csoundWorker.terminate();
      delete this.csoundWorker;
    }
    if (this.audioWorker && this.audioWorker.terminateInstance) {
      await this.audioWorker.terminateInstance();
      delete this.audioWorker.terminateInstance;
    }
    if (this.proxyPort) {
      this.proxyPort[Comlink.releaseProxy]();
      delete this.proxyPort;
    }
    if (this.publicEvents) {
      this.publicEvents.terminateInstance();
    }
    Object.keys(this.exportApi).forEach((key) => delete this.exportApi[key]);
    Object.keys(this).forEach((key) => delete this[key]);
  }

  get api() {
    return this.exportApi;
  }

  handleMidiInput({ data: [status, data1, data2] }) {
    const currentQueueLength = Atomics.load(
      this.audioStatePointer,
      AUDIO_STATE.AVAIL_RTMIDI_EVENTS,
    );
    const rtmidiBufferIndex = Atomics.load(this.audioStatePointer, AUDIO_STATE.RTMIDI_INDEX);
    const nextIndex =
      (currentQueueLength * MIDI_BUFFER_PAYLOAD_SIZE + rtmidiBufferIndex) % MIDI_BUFFER_SIZE;

    Atomics.store(this.midiBuffer, nextIndex, status);
    Atomics.store(this.midiBuffer, nextIndex + 1, data1);
    Atomics.store(this.midiBuffer, nextIndex + 2, data2);
    Atomics.add(this.audioStatePointer, AUDIO_STATE.AVAIL_RTMIDI_EVENTS, 1);
  }

  async csoundPause() {
    if (this.eventPromises.isWaiting("pause")) {
      return -1;
    } else {
      this.eventPromises.createPausePromise();

      Atomics.store(this.audioStatePointer, AUDIO_STATE.IS_PAUSED, 1);
      await this.eventPromises.waitForPause();
      this.onPlayStateChange("realtimePerformancePaused");
      return 0;
    }
  }

  async csoundResume() {
    if (
      Atomics.load(this.audioStatePointer, AUDIO_STATE.IS_PAUSED) === 1 &&
      Atomics.load(this.audioStatePointer, AUDIO_STATE.STOP) !== 1 &&
      Atomics.load(this.audioStatePointer, AUDIO_STATE.IS_PERFORMING) === 1
    ) {
      Atomics.store(this.audioStatePointer, AUDIO_STATE.IS_PAUSED, 0);
      Atomics.notify(this.audioStatePointer, AUDIO_STATE.IS_PAUSED);
      this.onPlayStateChange("realtimePerformanceResumed");
    }
  }

  async onPlayStateChange(newPlayState) {
    if (this === undefined) {
      console.log("Failed to announce playstatechange", newPlayState);
      return;
    }
    this.currentPlayState = newPlayState;
    if (!this.publicEvents || !newPlayState) {
      // prevent late timers from calling terminated fn
      return;
    }
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

        // flush out events sent during the time which the worker was stopping
        Object.values(this.callbackBuffer).forEach(({ argumentz, apiKey, resolveCallback }) =>
          this.proxyPort.callUncloned(apiKey, argumentz).then(resolveCallback),
        );
        this.callbackBuffer = {};
        log(`event: realtimePerformanceEnded received, beginning cleanup`)();
        // re-initialize SAB
        initialSharedState.forEach((value, index) => {
          Atomics.store(this.audioStatePointer, index, value);
        });
        break;
      }
      case "renderStarted": {
        this.publicEvents.triggerRenderStarted(this);
        this.eventPromises.releaseStartPromise();
        break;
      }
      case "renderEnded": {
        log(`event: renderEnded received, beginning cleanup`)();
        this.publicEvents.triggerRenderEnded(this);
        this.eventPromises && this.eventPromises.releaseStopPromise();
        break;
      }
      default: {
        break;
      }
    }

    // forward the message from worker to the audioWorker
    try {
      await this.audioWorker.onPlayStateChange(newPlayState);
    } catch (error) {
      console.error(error);
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
    this.audioWorker.isRequestingMidi = Atomics.load(
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

  async initialize({ wasmDataURI, withPlugins }) {
    if (withPlugins && !isEmpty(withPlugins)) {
      withPlugins = await fetchPlugins(withPlugins);
    }

    log(`initialization: instantiate the SABWorker Thread`)();
    // clearFsLastmods();
    const csoundWorker = new Worker(SABWorker());
    this.csoundWorker = csoundWorker;
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
      switch (event.data) {
        case "poll": {
          this.ipcMessagePorts &&
            this.ipcMessagePorts.sabMainCallbackReply.postMessage(
              Object.keys(this.callbackBuffer).map((id) => ({
                id,
                apiKey: this.callbackBuffer[id].apiKey,
                argumentz: this.callbackBuffer[id].argumentz,
              })),
            );
          break;
        }
        case "releaseStop": {
          this.onPlayStateChange(
            this.currentPlayState === "renderStarted" ? "renderEnded" : "realtimePerformanceEnded",
          );
          this.publicEvents && this.publicEvents.triggerRealtimePerformanceEnded(this);
          this.eventPromises && this.eventPromises.releaseStopPromise();
          break;
        }
        case "releasePause": {
          this.publicEvents.triggerRealtimePerformancePaused(this);
          this.eventPromises.releasePausePromise();
          break;
        }
        case "releaseResumed": {
          this.publicEvents.triggerRealtimePerformanceResumed(this);
          this.eventPromises.releaseResumePromise();
          break;
        }
        default: {
          event.data.forEach(({ id, answer }) => {
            this.callbackBuffer[id].resolveCallback(answer);
            delete this.callbackBuffer[id];
          });
        }
      }
    });
    this.ipcMessagePorts.sabMainCallbackReply.start();

    const proxyPort = Comlink.wrap(csoundWorker);
    const wasmBytes = wasmDataURI();
    this.proxyPort = proxyPort;
    const csoundInstance = await proxyPort.initialize(
      Comlink.transfer(
        {
          wasmDataURI: wasmBytes,
          wasmTransformerDataURI: this.wasmTransformerDataURI,
          messagePort: this.ipcMessagePorts.workerMessagePort,
          callbackPort: this.ipcMessagePorts.sabWorkerCallbackReply,
          withPlugins,
        },
        [
          wasmBytes,
          this.ipcMessagePorts.workerMessagePort,
          this.ipcMessagePorts.sabWorkerCallbackReply,
        ],
      ),
    );
    this.csoundInstance = csoundInstance;

    this.ipcMessagePorts.mainMessagePort.start();
    this.ipcMessagePorts.mainMessagePortAudio.start();

    log(`A proxy port from SABMain to SABWorker established`)();

    this.exportApi.pause = this.csoundPause.bind(this);
    this.exportApi.resume = this.csoundResume.bind(this);
    this.exportApi.terminateInstance = this.terminateInstance.bind(this);
    // this.exportApi.fs = this.fs;

    this.exportApi.enableAudioInput = () =>
      console.warn(
        `enableAudioInput was ignored: please use -iadc option before calling start with useWorker=true`,
      );

    this.exportApi.getNode = async () => {
      const maybeNode = this.audioWorker.audioWorkletNode;
      if (maybeNode) {
        return maybeNode;
      } else {
        const node = await new Promise((resolve) => {
          this.exportApi.once("onAudioNodeCreated", resolve);
        });
        return node;
      }
    };

    this.exportApi.getAudioContext = async () => this.audioWorker.audioContext;

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
            if (!csoundInstance || typeof csoundInstance !== "number") {
              console.error("starting csound failed because csound instance wasn't created");
              return -1;
            }
            if (this.eventPromises.isWaiting("start")) {
              return -1;
            } else {
              this.eventPromises.createStartPromise();

              const startResult = await proxyCallback({
                audioStateBuffer,
                audioStreamIn,
                audioStreamOut,
                midiBuffer,
                csound: csoundInstance,
              });

              await this.eventPromises.waitForStart();

              this.ipcMessagePorts &&
                this.ipcMessagePorts.sabMainCallbackReply.postMessage({ unlock: true });

              return startResult;
            }
          };

          csoundStart.toString = () => reference.toString();
          this.exportApi.start = csoundStart.bind(this);
          break;
        }
        case "csoundStop": {
          const csoundStop = async () => {
            log(
              [
                "Checking if it's safe to call stop:",
                stopableStates.has(this.currentPlayState),
                "currentPlayState is",
                this.currentPlayState,
              ].join("\n"),
            )();
            if (this.eventPromises.isWaiting("stop")) {
              log("already waiting to stop, doing nothing")();
              return -1;
            } else if (stopableStates.has(this.currentPlayState)) {
              log("Marking SAB's state to STOP")();

              this.eventPromises.createStopPromise();

              Atomics.store(this.audioStatePointer, AUDIO_STATE.STOP, 1);
              log("Marking that performance is not running anymore (stops the audio too)")();
              Atomics.store(this.audioStatePointer, AUDIO_STATE.IS_PERFORMING, 0);

              // A potential case where the thread is locked because of pause
              if (this.currentPlayState === "realtimePerformancePaused") {
                Atomics.store(this.audioStatePointer, AUDIO_STATE.IS_PAUSED, 0);
                Atomics.notify(this.audioStatePointer, AUDIO_STATE.IS_PAUSED);
              }
              if (this.currentPlayState !== "renderStarted") {
                !Atomics.compareExchange(this.audioStatePointer, AUDIO_STATE.CSOUND_LOCK, 0, 1) &&
                  Atomics.notify(this.audioStatePointer, AUDIO_STATE.CSOUND_LOCK);
              }
              await this.eventPromises.waitForStop();
              return 0;
            } else {
              return -1;
            }
          };
          this.exportApi.stop = csoundStop.bind(this);
          csoundStop.toString = () => reference.toString();
          break;
        }

        case "csoundReset": {
          const csoundReset = async () => {
            // no start = noReset
            if (!this.currentPlayState) {
              return;
            }

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
              const resetResult = await proxyCallback([]);
              return resetResult;
            }
          };
          this.exportApi.reset = csoundReset.bind(this);
          csoundReset.toString = () => reference.toString();
          break;
        }
        case "csoundPushMidiMessage": {
          const midiMessage = async (status = 0, data1 = 0, data2 = 0) => {
            this.handleMidiInput({ data: [status, data1, data2] });
          };
          this.exportApi.midiMessage = midiMessage.bind(this);
          midiMessage.toString = () => reference.toString();
          break;
        }

        case "fs": {
          this.exportApi.fs = {};
          Object.keys(reference).forEach((method) => {
            const proxyFsCallback = makeProxyCallback(
              proxyPort,
              csoundInstance,
              method,
              this.currentPlayState,
            );
            proxyFsCallback.toString = () => reference[method].toString();
            this.exportApi.fs[method] = proxyFsCallback;
          });
          break;
        }

        default: {
          // avoiding deadlock by sending the IPC callback
          // while thread is unlocked
          const bufferWrappedCallback = async (...arguments_) => {
            if (
              this.currentPlayState === "realtimePerformanceStarted" ||
              this.currentPlayState === "renderStarted" ||
              this.eventPromises.isWaitingToStart()
              // startPromiz indicates that startup is in progress
              // and any events send before it's resolved are swallowed
            ) {
              const callbackId = this.callbackId;
              this.callbackId += 1;
              const returnPromise = new Promise((resolve, reject) => {
                const timeout = setTimeout(
                  () =>
                    reject(
                      new Error(`Worker timed out so ${csoundApiRename(apiKey)}() wasn't called!`),
                    ),
                  10000,
                );
                const resolveCallback = (answer) => {
                  clearTimeout(timeout);
                  resolve(answer);
                };
                this.callbackBuffer[callbackId] = {
                  resolveCallback,
                  apiKey,
                  argumentz: [csoundInstance, ...arguments_],
                };
              });
              Atomics.compareExchange(audioStatePointer, AUDIO_STATE.HAS_PENDING_CALLBACKS, 0, 1);
              return await returnPromise;
            } else {
              return await proxyCallback.apply(undefined, arguments_);
            }
          };
          bufferWrappedCallback.toString = () => reference.toString();
          this.exportApi[csoundApiRename(apiKey)] = bufferWrappedCallback;
          break;
        }
      }
    }
    log(`PUBLIC API Generated and stored`)();
  }
}

export default SharedArrayBufferMainThread;
