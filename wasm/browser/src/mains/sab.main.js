import * as Comlink from "comlink";
import { api as API } from "@root/libcsound";
import { messageEventHandler, IPCMessagePorts } from "@root/mains/messages.main";
import { makeSABPerfCallback } from "@root/sab.main.utils";
import SABWorker from "@root/workers/sab.worker";
import {
  AUDIO_STATE,
  CALLBACK_DATA_BUFFER_SIZE,
  DATA_TYPE,
  MAX_CHANNELS,
  MAX_HARDWARE_BUFFER_SIZE,
  MIDI_BUFFER_PAYLOAD_SIZE,
  MIDI_BUFFER_SIZE,
  initialSharedState,
} from "@root/constants";
import { logSAB } from "@root/logger";
import { csoundApiRename, makeProxyCallback, stopableStates } from "@root/utils";

class SharedArrayBufferMainThread {
  constructor(audioWorker, wasmDataURI) {
    this.hasSharedArrayBuffer = true;
    this.ipcMessagePorts = new IPCMessagePorts();
    audioWorker.ipcMessagePorts = this.ipcMessagePorts;

    this.audioWorker = audioWorker;
    this.csoundInstance = undefined;
    this.wasmDataURI = wasmDataURI;
    this.currentPlayState = undefined;
    this.exportApi = {};
    this.messageCallbacks = [];
    this.csoundPlayStateChangeCallbacks = [];

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

    this.callbackBufferSAB = new SharedArrayBuffer(1024 * Int32Array.BYTES_PER_ELEMENT);

    this.callbackBuffer = new Int32Array(this.callbackBufferSAB);

    this.callbackStringDataBufferSAB = new SharedArrayBuffer(
      CALLBACK_DATA_BUFFER_SIZE * Int8Array.BYTES_PER_ELEMENT,
    );

    this.callbackStringDataBuffer = new Uint8Array(this.callbackStringDataBufferSAB);

    this.callbackFloatArrayDataBufferSAB = new SharedArrayBuffer(
      CALLBACK_DATA_BUFFER_SIZE * Float64Array.BYTES_PER_ELEMENT,
    );

    this.callbackFloatArrayDataBuffer = new Float64Array(this.callbackFloatArrayDataBufferSAB);

    this.onPlayStateChange = this.onPlayStateChange.bind(this);
    logSAB(`SharedArrayBufferMainThread got constructed`);
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

  async addMessageCallback(callback) {
    if (typeof callback === "function") {
      this.messageCallbacks.push(callback);
    } else {
      console.error(`Can't assign ${typeof callback} as a message callback`);
    }
  }

  async setMessageCallback(callback) {
    if (typeof callback === "function") {
      this.messageCallbacks = [callback];
    } else {
      console.error(`Can't assign ${typeof callback} as a message callback`);
    }
  }

  // User-land hook to csound's play-state changes
  async setCsoundPlayStateChangeCallback(callback) {
    if (typeof callback !== "function") {
      console.error(`Can't assign ${typeof callback} as a playstate change callback`);
    } else {
      this.csoundPlayStateChangeCallbacks = [callback];
    }
  }

  async addCsoundPlayStateChangeCallback(callback) {
    if (typeof callback !== "function") {
      console.error(`Can't assign ${typeof callback} as a playstate change callback`);
    } else {
      this.csoundPlayStateChangeCallbacks.push(callback);
    }
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

    switch (newPlayState) {
      case "realtimePerformanceStarted": {
        logSAB(
          `event: realtimePerformanceStarted received,` +
            ` proceeding to call prepareRealtimePerformance`,
        );
        await this.prepareRealtimePerformance();
        break;
      }
      case "realtimePerformanceEnded": {
        logSAB(`event: realtimePerformanceEnded received, beginning cleanup`);

        // re-initialize SAB
        initialSharedState.forEach((value, index) => {
          Atomics.store(this.audioStatePointer, index, value);
        });
        break;
      }

      case "renderEnded": {
        logSAB(`event: renderEnded received, beginning cleanup`);
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

    this.csoundPlayStateChangeCallbacks.forEach((callback) => {
      try {
        callback(newPlayState);
      } catch (error) {
        console.error(error);
      }
    });
  }

  async prepareRealtimePerformance() {
    logSAB(`prepareRealtimePerformance`);
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

  async initialize() {
    logSAB(`initialization: instantiate the SABWorker Thread`);
    const csoundWorker = new Worker(SABWorker());
    this.csoundWorker = csoundWorker;
    const audioStateBuffer = this.audioStateBuffer;
    const audioStatePointer = this.audioStatePointer;
    const audioStreamIn = this.audioStreamIn;
    const audioStreamOut = this.audioStreamOut;
    const midiBuffer = this.midiBuffer;
    const callbackBuffer = this.callbackBuffer;
    const callbackStringDataBuffer = this.callbackStringDataBuffer;
    const callbackFloatArrayDataBuffer = this.callbackFloatArrayDataBuffer;

    // This will sadly create circular structure
    // that's still mostly harmless.
    logSAB(`providing the audioWorker a pointer to SABMain's instance`);
    this.audioWorker.csoundWorkerMain = this;

    // both audio worker and csound worker use 1 handler
    // simplifies flow of data (csound main.worker is always first to receive)
    logSAB(`adding message eventListeners for mainMessagePort and mainMessagePortAudio`);
    this.ipcMessagePorts.mainMessagePort.addEventListener("message", messageEventHandler(this));
    this.ipcMessagePorts.mainMessagePortAudio.addEventListener(
      "message",
      messageEventHandler(this),
    );
    logSAB(
      `(postMessage) making a message channel from SABMain to SABWorker via workerMessagePort`,
    );
    csoundWorker.postMessage({ msg: "initMessagePort" }, [this.ipcMessagePorts.workerMessagePort]);

    // we send callbacks to the worker in SAB, but receive these return values as message events
    let returnQueue = {};
    this.ipcMessagePorts.sabMainCallbackReply.addEventListener("message", (event) => {
      const { uid, value } = event.data;
      const promize = returnQueue[uid];
      promize && promize(value);
    });
    csoundWorker.postMessage({ msg: "initCallbackReplyPort" }, [
      this.ipcMessagePorts.sabWorkerCallbackReply,
    ]);

    this.ipcMessagePorts.mainMessagePort.start();
    this.ipcMessagePorts.mainMessagePortAudio.start();

    const proxyPort = Comlink.wrap(csoundWorker);
    await proxyPort.initialize(this.wasmDataURI);
    logSAB(`A proxy port from SABMain to SABWorker established`);

    this.exportApi.setMessageCallback = this.setMessageCallback.bind(this);
    this.exportApi.addMessageCallback = this.addMessageCallback.bind(this);
    this.exportApi.setCsoundPlayStateChangeCallback = this.setCsoundPlayStateChangeCallback.bind(
      this,
    );
    this.exportApi.addCsoundPlayStateChangeCallback = this.addCsoundPlayStateChangeCallback.bind(
      this,
    );

    this.exportApi.pause = this.csoundPause.bind(this);
    this.exportApi.resume = this.csoundResume.bind(this);
    const csoundInstance = await makeProxyCallback(proxyPort, undefined, "csoundCreate")();
    await makeProxyCallback(proxyPort, csoundInstance, "csoundInitialize")(0);

    this.csoundInstance = csoundInstance;
    this.exportApi.writeToFs = makeProxyCallback(proxyPort, csoundInstance, "writeToFs");
    this.exportApi.readFromFs = makeProxyCallback(proxyPort, csoundInstance, "readFromFs");
    this.exportApi.llFs = makeProxyCallback(proxyPort, csoundInstance, "llFs");
    this.exportApi.lsFs = makeProxyCallback(proxyPort, csoundInstance, "lsFs");
    this.exportApi.rmrfFs = makeProxyCallback(proxyPort, csoundInstance, "rmrfFs");

    this.exportApi.getAudioContext = async () => this.audioWorker.audioContext;

    for (const apiK of Object.keys(API)) {
      const proxyCallback = makeProxyCallback(proxyPort, csoundInstance, apiK);
      const reference = API[apiK];

      switch (apiK) {
        case "csoundCreate": {
          break;
        }
        case "csoundStart": {
          const csoundStart = async function () {
            if (!csoundInstance || typeof csoundInstance !== "number") {
              console.error("starting csound failed because csound instance wasn't created");
              return -1;
            }
            return await proxyCallback({
              audioStateBuffer,
              audioStreamIn,
              audioStreamOut,
              midiBuffer,
              callbackBuffer,
              callbackStringDataBuffer,
              callbackFloatArrayDataBuffer,
              csound: csoundInstance,
            });
          };

          csoundStart.toString = () => reference.toString();
          this.exportApi.start = csoundStart.bind(this);
          break;
        }

        case "csoundStop": {
          const csoundStop = async (csound) => {
            logSAB(
              "Checking if it's safe to call stop:",
              stopableStates.has(this.currentPlayState),
            );
            if (stopableStates.has(this.currentPlayState)) {
              logSAB("Marking SAB's state to STOP");
              Atomics.store(this.audioStatePointer, AUDIO_STATE.STOP, 1);
              logSAB("Marking that performance is not running anymore (stops the audio too)");
              Atomics.store(this.audioStatePointer, AUDIO_STATE.IS_PERFORMING, 0);
              x;
              // Double check if the thread didn't defenitely get the STOP message
              setTimeout(() => {
                logSAB("Double checking if SAB stopped");
                if (this.currentPlayState !== "realtimePerformanceEnded") {
                  logSAB("stopping didn't cause the correct event to be triggered");
                  if (Atomics.load(this.audioStatePointer, AUDIO_STATE.STOP) === 0) {
                    logSAB(
                      "stopped state got reset to 0 (could be fatal, but also race condition)",
                    );
                    Atomics.store(this.audioStatePointer, AUDIO_STATE.STOP, 1);
                  }
                  logSAB("making a second Atomic notify to SAB, pray for the best");
                  Atomics.notify(this.audioStatePointer, AUDIO_STATE.ATOMIC_NOTIFY);
                }
              }, 1000);

              // A potential case where the thread is locked because of pause
              if (this.currentPlayState === "realtimePerformancePaused") {
                Atomics.store(this.audioStatePointer, AUDIO_STATE.IS_PAUSED, 0);
                Atomics.notify(this.audioStatePointer, AUDIO_STATE.IS_PAUSED);
              }
            }
          };
          this.exportApi.stop = csoundStop.bind(this);
          csoundStop.toString = () => reference.toString();
          break;
        }

        default: {
          const perfCallback = makeSABPerfCallback({
            apiK,
            audioStatePointer,
            callbackBuffer,
            callbackStringDataBuffer,
            callbackFloatArrayDataBuffer,
            returnQueue,
          });
          const bufferWrappedCallback = async (...args) => {
            if (this.currentPlayState === "realtimePerformanceStarted") {
              return perfCallback(args);
            } else {
              return await proxyCallback.apply(undefined, args);
            }
          };
          bufferWrappedCallback.toString = () => reference.toString();
          this.exportApi[csoundApiRename(apiK)] = bufferWrappedCallback;
          break;
        }
      }
    }
    logSAB(`PUBLIC API Generated and stored`);
  }
}

export default SharedArrayBufferMainThread;
