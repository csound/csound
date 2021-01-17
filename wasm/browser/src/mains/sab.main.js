import * as Comlink from "comlink";
import { api as API } from "@root/libcsound";
import { messageEventHandler, IPCMessagePorts } from "@root/mains/messages.main";
import SABWorker from "@root/workers/sab.worker";
import {
  AUDIO_STATE,
  MAX_CHANNELS,
  MAX_HARDWARE_BUFFER_SIZE,
  MIDI_BUFFER_PAYLOAD_SIZE,
  MIDI_BUFFER_SIZE,
  initialSharedState,
} from "@root/constants";
import { logSABMain as log } from "@root/logger";
import { isEmpty } from "ramda";
import { csoundApiRename, fetchPlugins, makeProxyCallback, stopableStates } from "@root/utils";
import { PublicEventAPI } from "@root/events";

class SharedArrayBufferMainThread {
  constructor({ audioWorker, wasmDataURI, audioContextIsProvided }) {
    this.hasSharedArrayBuffer = true;
    this.ipcMessagePorts = new IPCMessagePorts();
    this.publicEvents = new PublicEventAPI(this);
    audioWorker.ipcMessagePorts = this.ipcMessagePorts;

    this.audioWorker = audioWorker;
    this.audioContextIsProvided = audioContextIsProvided;
    this.csoundInstance = undefined;
    this.wasmDataURI = wasmDataURI;
    this.currentPlayState = undefined;
    this.currentDerivedPlayState = "stop";
    this.exportApi = {};

    this.callbackId = 0;
    this.callbackBuffer = {};

    this.startPromiz = undefined;
    this.stopPromiz = undefined;

    this.audioStateBuffer = new SharedArrayBuffer(
      initialSharedState.length * Int32Array.BYTES_PER_ELEMENT,
    );

    this.audioStatePointer = new Int32Array(this.audioStateBuffer);

    this.audioStreamIn = new SharedArrayBuffer(
      MAX_CHANNELS * MAX_HARDWARE_BUFFER_SIZE * Float64Array.BYTES_PER_ELEMENT,
    );
    this.audioStreamOut = new SharedArrayBuffer(
      MAX_CHANNELS * MAX_HARDWARE_BUFFER_SIZE * Float64Array.BYTES_PER_ELEMENT,
    );

    this.midiBufferSAB = new SharedArrayBuffer(
      MIDI_BUFFER_SIZE * MIDI_BUFFER_PAYLOAD_SIZE * Int32Array.BYTES_PER_ELEMENT,
    );

    this.midiBuffer = new Int32Array(this.midiBufferSAB);

    this.onPlayStateChange = this.onPlayStateChange.bind(this);
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
    if (
      Atomics.load(this.audioStatePointer, AUDIO_STATE.IS_PAUSED) !== 1 &&
      Atomics.load(this.audioStatePointer, AUDIO_STATE.STOP) !== 1 &&
      Atomics.load(this.audioStatePointer, AUDIO_STATE.IS_PERFORMING) === 1
    ) {
      Atomics.store(this.audioStatePointer, AUDIO_STATE.IS_PAUSED, 1);
      this.onPlayStateChange("realtimePerformancePaused");
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
    this.currentPlayState = newPlayState;
    if (!this.publicEvents) {
      // prevent late timers from calling terminated fn
      return;
    }
    switch (newPlayState) {
      case "realtimePerformanceStarted": {
        log(
          `event: realtimePerformanceStarted received,` +
            ` proceeding to call prepareRealtimePerformance`,
        )();
        await this.prepareRealtimePerformance();
        this.publicEvents.triggerRealtimePerformanceStarted(this);
        break;
      }
      case "realtimePerformanceEnded": {
        log(`event: realtimePerformanceEnded received, beginning cleanup`)();
        if (this.stopPromiz) {
          this.stopPromiz();
          delete this.stopPromiz;
        }
        this.publicEvents.triggerRealtimePerformanceEnded(this);
        // re-initialize SAB
        initialSharedState.forEach((value, index) => {
          Atomics.store(this.audioStatePointer, index, value);
        });
        break;
      }
      case "realtimePerformancePaused": {
        this.publicEvents.triggerRealtimePerformancePaused(this);
        break;
      }
      case "realtimePerformanceResumed": {
        this.publicEvents.triggerRealtimePerformanceResumed(this);
        break;
      }
      case "renderStarted": {
        this.publicEvents.triggerRenderStarted(this);
        break;
      }
      case "renderEnded": {
        if (this.stopPromiz) {
          this.stopPromiz();
          delete this.stopPromiz;
        }
        this.publicEvents.triggerRenderEnded(this);
        log(`event: renderEnded received, beginning cleanup`)();
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

    if (this.startPromiz && newPlayState !== "realtimePerformanceStarted") {
      // either we are rendering or something went wrong with the start
      // otherwise the audioWorker resolves this
      this.startPromiz();
      delete this.startPromiz;
    }
  }

  async prepareRealtimePerformance() {
    log(`prepareRealtimePerformance`)();
    const outputsCount = Atomics.load(this.audioStatePointer, AUDIO_STATE.NCHNLS);
    const inputCount = Atomics.load(this.audioStatePointer, AUDIO_STATE.NCHNLS_I);

    this.audioWorker.isRequestingInput = inputCount > 0;
    this.audioWorker.isRequestingMidi = Atomics.load(
      this.audioStatePointer,
      AUDIO_STATE.IS_REQUESTING_RTMIDI,
    );

    const sampleRate = Atomics.load(this.audioStatePointer, AUDIO_STATE.SAMPLE_RATE);

    const hardwareBufferSize = Atomics.load(this.audioStatePointer, AUDIO_STATE.HW_BUFFER_SIZE);

    const softwareBufferSize = Atomics.load(this.audioStatePointer, AUDIO_STATE.SW_BUFFER_SIZE);

    this.audioWorker.sampleRate = sampleRate;
    this.audioWorker.inputCount = inputCount;
    this.audioWorker.outputsCount = outputsCount;
    this.audioWorker.hardwareBufferSize = hardwareBufferSize;
    this.audioWorker.softwareBufferSize = softwareBufferSize;
  }

  async initialize({ withPlugins }) {
    if (withPlugins && !isEmpty(withPlugins)) {
      withPlugins = await fetchPlugins(withPlugins);
    }

    log(`initialization: instantiate the SABWorker Thread`)();
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
      if (event.data === "poll") {
        this.ipcMessagePorts.sabMainCallbackReply.postMessage(
          Object.keys(this.callbackBuffer).map((id) => ({
            id,
            apiKey: this.callbackBuffer[id].apiKey,
            argumentz: this.callbackBuffer[id].argumentz,
          })),
        );
      } else {
        event.data.forEach(({ id, answer }) => {
          this.callbackBuffer[id].resolveCallback(answer);
          delete this.callbackBuffer[id];
        });
      }
    });
    this.ipcMessagePorts.sabMainCallbackReply.start();

    const proxyPort = Comlink.wrap(csoundWorker);
    this.proxyPort = proxyPort;
    const csoundInstance = await proxyPort.initialize(
      Comlink.transfer(
        {
          wasmDataURI: this.wasmDataURI,
          messagePort: this.ipcMessagePorts.workerMessagePort,
          callbackPort: this.ipcMessagePorts.sabWorkerCallbackReply,
          withPlugins,
        },
        [this.ipcMessagePorts.workerMessagePort, this.ipcMessagePorts.sabWorkerCallbackReply],
      ),
    );
    this.csoundInstance = csoundInstance;

    this.ipcMessagePorts.mainMessagePort.start();
    this.ipcMessagePorts.mainMessagePortAudio.start();

    log(`A proxy port from SABMain to SABWorker established`)();

    this.exportApi.pause = this.csoundPause.bind(this);
    this.exportApi.resume = this.csoundResume.bind(this);
    this.exportApi.terminateInstance = this.terminateInstance.bind(this);

    this.exportApi.writeToFs = makeProxyCallback(proxyPort, csoundInstance, "writeToFs");
    this.exportApi.readFromFs = makeProxyCallback(proxyPort, csoundInstance, "readFromFs");
    this.exportApi.llFs = makeProxyCallback(proxyPort, csoundInstance, "llFs");
    this.exportApi.lsFs = makeProxyCallback(proxyPort, csoundInstance, "lsFs");
    this.exportApi.rmrfFs = makeProxyCallback(proxyPort, csoundInstance, "rmrfFs");

    this.exportApi.getNode = async () => {
      const maybeNode = this.audioWorker.audioWorkletNode;
      return maybeNode;
    };

    this.exportApi.getAudioContext = async () => this.audioWorker.audioContext;

    this.exportApi = this.publicEvents.decorateAPI(this.exportApi);

    // the default message listener
    this.exportApi.addListener("message", console.log);

    for (const apiKey of Object.keys(API)) {
      const proxyCallback = makeProxyCallback(proxyPort, csoundInstance, apiKey);
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

            const startPromise = new Promise((resolve) => {
              this.startPromiz = resolve;
            });

            const startResult = await proxyCallback({
              audioStateBuffer,
              audioStreamIn,
              audioStreamOut,
              midiBuffer,
              csound: csoundInstance,
            });

            await startPromise;
            return startResult;
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

            if (stopableStates.has(this.currentPlayState)) {
              log("Marking SAB's state to STOP")();
              const stopPromise = new Promise((resolve) => {
                this.stopPromiz = resolve;
              });
              Atomics.store(this.audioStatePointer, AUDIO_STATE.STOP, 1);
              log("Marking that performance is not running anymore (stops the audio too)")();
              Atomics.store(this.audioStatePointer, AUDIO_STATE.IS_PERFORMING, 0);

              // A potential case where the thread is locked because of pause
              if (this.currentPlayState === "realtimePerformancePaused") {
                Atomics.store(this.audioStatePointer, AUDIO_STATE.IS_PAUSED, 0);
                Atomics.notify(this.audioStatePointer, AUDIO_STATE.IS_PAUSED);
              }
              if (this.currentPlayState !== "renderStarted") {
                Atomics.store(this.audioStatePointer, AUDIO_STATE.ATOMIC_NOFITY, 1);
                Atomics.notify(this.audioStatePointer, AUDIO_STATE.ATOMIC_NOTIFY);
              }
              await stopPromise;
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
            if (stopableStates.has(this.currentPlayState)) {
              await this.exportApi.stop();
            }
            const resetResult = await proxyCallback([]);
            this.audioStateBuffer = new SharedArrayBuffer(
              initialSharedState.length * Int32Array.BYTES_PER_ELEMENT,
            );
            this.audioStatePointer = new Int32Array(this.audioStateBuffer);
            return resetResult;
          };
          this.exportApi.reset = csoundReset.bind(this);
          csoundReset.toString = () => reference.toString();
          break;
        }

        default: {
          const bufferWrappedCallback = async (...arguments_) => {
            if (
              this.currentPlayState === "realtimePerformanceStarted" ||
              this.currentPlayState === "renderStarted"
            ) {
              // avoiding deadlock by sending the IPC callback
              // while thread is unlocked
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
                const resolveCallback = (...answer) => {
                  clearTimeout(timeout);
                  if (answer.length > 0) {
                    resolve.apply(answer);
                  } else {
                    resolve();
                  }
                };
                this.callbackBuffer[callbackId] = {
                  resolveCallback,
                  apiKey,
                  argumentz: [csoundInstance, ...arguments_],
                };
              });
              Atomics.compareExchange(audioStatePointer, AUDIO_STATE.HAS_PENDING_CALLBACKS, 0, 1);
              return await returnPromise;
            } else if (
              this.currentPlayState === "realtimePerformanceEnded" ||
              this.currentPlayState === "renderEnded"
            ) {
              console.error(`${csoundApiRename(apiKey)} was called after perfomance ended`);
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
