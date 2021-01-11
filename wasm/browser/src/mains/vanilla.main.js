import * as Comlink from "comlink";
import { logVAN } from "@root/logger";
import { api as API } from "@root/libcsound";
import VanillaWorker from "@root/workers/vanilla.worker";
import {
  DEFAULT_HARDWARE_BUFFER_SIZE,
  DEFAULT_SOFTWARE_BUFFER_SIZE,
  MAX_CHANNELS,
  MAX_HARDWARE_BUFFER_SIZE,
  MIDI_BUFFER_PAYLOAD_SIZE,
  MIDI_BUFFER_SIZE,
} from "@root/constants.js";
import { isEmpty } from "ramda";
import { csoundApiRename, fetchPlugins, makeProxyCallback, stopableStates } from "@root/utils";
import { IPCMessagePorts, messageEventHandler } from "@root/mains/messages.main";
import { PublicEventAPI } from "@root/events";

class VanillaWorkerMainThread {
  constructor({ audioWorker, wasmDataURI, audioContextIsProvided }) {
    this.ipcMessagePorts = new IPCMessagePorts();
    this.publicEvents = new PublicEventAPI(this);
    audioWorker.ipcMessagePorts = this.ipcMessagePorts;

    audioWorker.csoundWorkerMain = this;
    this.audioWorker = audioWorker;
    this.audioContextIsProvided = audioContextIsProvided;
    this.wasmDataURI = wasmDataURI;
    this.exportApi = {};
    this.csoundInstance = undefined;
    this.currentPlayState = undefined;
    // this.messageCallbacks = [];
    this.midiPortStarted = false;
    this.onPlayStateChange = this.onPlayStateChange.bind(this);
    this.startPromiz = undefined;
  }

  get api() {
    return this.exportApi;
  }

  handleMidiInput({ data: payload }) {
    this.ipcMessagePorts.csoundMainRtMidiPort.postMessage &&
      this.ipcMessagePorts.csoundMainRtMidiPort.postMessage(payload);
  }

  async prepareRealtimePerformance() {
    if (!this.csoundInstance) {
      console.error(`fatal error: csound instance not found?`);
      return;
    }

    this.audioWorker.sampleRate = await this.exportApi.getSr(this.csoundInstance);
    this.audioWorker.isRequestingInput = (
      await this.exportApi.getInputName(this.csoundInstance)
    ).includes("adc");
    this.audioWorker.isRequestingMidi = await this.exportApi._isRequestingRtMidiInput(
      this.csoundInstance,
    );
    this.audioWorker.outputsCount = await this.exportApi.getNchnls(this.csoundInstance);
    // TODO fix upstream: await this.exportApi.csoundGetNchnlsInput(this.csound);
    this.audioWorker.inputsCount = this.audioWorker.isRequestingInput ? 1 : 0;
    this.audioWorker.hardwareBufferSize = DEFAULT_HARDWARE_BUFFER_SIZE;
    this.audioWorker.softwareBufferSize = DEFAULT_SOFTWARE_BUFFER_SIZE;
    if (this.audioWorker.scriptProcessorNode) {
      this.audioWorker.softwareBufferSize *= 2;
    }

    logVAN(`vars for rtPerf set`);
  }

  async onPlayStateChange(newPlayState) {
    this.currentPlayState = newPlayState;

    switch (newPlayState) {
      case "realtimePerformanceStarted": {
        logVAN(`event: realtimePerformanceStarted from worker, now preparingRT..`);
        await this.prepareRealtimePerformance();
        this.publicEvents.triggerRealtimePerformanceStarted(this);
        break;
      }

      case "realtimePerformanceEnded": {
        logVAN(`event: realtimePerformanceEnded`);
        if (this.stopPromiz) {
          this.stopPromiz();
          delete this.stopPromiz;
        }
        this.midiPortStarted = false;
        this.currentPlayState = undefined;
        this.publicEvents.triggerRealtimePerformanceEnded(this);
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
              if (this.startPromiz) {
          this.startPromiz();
          delete this.startPromiz;
              };
        this.publicEvents.triggerRenderStarted(this);
        break;
      }
      case "renderEnded": {
        logVAN(`event: renderEnded received, beginning cleanup`);
        if (this.stopPromiz) {
          this.stopPromiz();
          delete this.stopPromiz;
        }
        this.publicEvents.triggerRenderEnded(this);
        break;
      }

      default: {
        break;
      }
    }

    // forward the message from worker to the audioWorker
    try {
      if (!this.audioWorker) {
        console.error(`fatal error: audioWorker not initialized!`);
      } else {
        await this.audioWorker.onPlayStateChange(newPlayState);
      }
    } catch (error) {
      console.error(`Csound thread crashed while receiving an IPC message: ${error}`);
    }
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

  async csoundPause() {
    if (this.audioWorker && typeof this.audioWorker.workletProxy !== "undefined") {
      await this.audioWorker.workletProxy.pause();
    }
    this.onPlayStateChange("realtimePerformancePaused");
  }

  async csoundResume() {
    if (this.audioWorker && typeof this.audioWorker.workletProxy !== "undefined") {
      await this.audioWorker.workletProxy.resume();
    }
    this.onPlayStateChange("realtimePerformanceResumed");
  }

  async initialize({ withPlugins }) {
    if (typeof this.audioWorker.initIframe === "function") {
      await this.audioWorker.initIframe();
    }

    if (withPlugins && !isEmpty(withPlugins)) {
      withPlugins = await fetchPlugins(withPlugins);
    }
    logVAN(`vanilla.main: initialize`);
    this.csoundWorker = this.csoundWorker || new Worker(VanillaWorker());

    this.ipcMessagePorts.mainMessagePort.addEventListener("message", messageEventHandler(this));
    this.ipcMessagePorts.mainMessagePort.start();

    const proxyPort = Comlink.wrap(this.csoundWorker);
    this.proxyPort = proxyPort;
    const csoundInstance = await proxyPort.initialize(
      Comlink.transfer(
        {
          wasmDataURI: this.wasmDataURI,
          messagePort: this.ipcMessagePorts.workerMessagePort,
          requestPort: this.ipcMessagePorts.csoundWorkerFrameRequestPort,
          audioInputPort: this.ipcMessagePorts.csoundWorkerAudioInputPort,
          rtmidiPort: this.ipcMessagePorts.csoundWorkerRtMidiPort,
          withPlugins,
        },
        [
          this.ipcMessagePorts.workerMessagePort,
          this.ipcMessagePorts.csoundWorkerFrameRequestPort,
          this.ipcMessagePorts.csoundWorkerAudioInputPort,
          this.ipcMessagePorts.csoundWorkerRtMidiPort,
        ],
      ),
    );

    this.csoundInstance = csoundInstance;

    this.exportApi.setMessageCallback = this.setMessageCallback.bind(this);
    this.exportApi.addMessageCallback = this.addMessageCallback.bind(this);

    this.exportApi.pause = this.csoundPause.bind(this);
    this.exportApi.resume = this.csoundResume.bind(this);
    this.exportApi.writeToFs = makeProxyCallback(proxyPort, csoundInstance, "writeToFs");
    this.exportApi.readFromFs = makeProxyCallback(proxyPort, csoundInstance, "readFromFs");
    this.exportApi.llFs = makeProxyCallback(proxyPort, csoundInstance, "llFs");
    this.exportApi.lsFs = makeProxyCallback(proxyPort, csoundInstance, "lsFs");
    this.exportApi.rmrfFs = makeProxyCallback(proxyPort, csoundInstance, "rmrfFs");
    this.exportApi.getAudioContext = async () => this.audioWorker.audioContext;
    this.exportApi.getNode = async () => this.audioWorker.audioWorkletNode;
    this.exportApi = this.publicEvents.decorateAPI(this.exportApi);

    for (const apiK of Object.keys(API)) {
      const reference = API[apiK];
      const proxyCallback = makeProxyCallback(proxyPort, csoundInstance, apiK);
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
            const startPromise = new Promise((resolve, reject) => {
              this.startPromiz = resolve;
              setTimeout(() => {
                if (typeof this.startPromiz === "function") {
                  reject(`a call to start() timed out`);
                  delete this.startPromiz;
                  return -1;
                }
                // 10 second timeout
              }, 10 * 60 * 1000);
            });
            const startResult = await proxyCallback({
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
          const csoundStop = async function () {
            const stopPromise = new Promise((resolve) => {
              this.stopPromiz = resolve;
            });
            this.ipcMessagePorts.mainMessagePort.postMessage({
              newPlayState:
                this.currentPlayState === "renderStarted"
                  ? "renderEnded"
                  : "realtimePerformanceEnded",
            });
            await stopPromise;
            return 0;
          };
          this.exportApi.stop = csoundStop.bind(this);
          csoundStop.toString = () => reference.toString();
          break;
        }

        case "csoundReset": {
          const csoundReset = async () => {
            if (stopableStates.has(this.currentPlayState)) {
              await this.exportApi.stop();
            }
            const resetResult = await proxyCallback([]);
            return resetResult;
          };
          this.exportApi.reset = csoundReset.bind(this);
          csoundReset.toString = () => reference.toString();
          break;
        }

        default: {
          proxyCallback.toString = () => reference.toString();
          this.exportApi[csoundApiRename(apiK)] = proxyCallback;
          break;
        }
      }
    }
    logVAN(`exportAPI generated`);
  }
}

export default VanillaWorkerMainThread;
