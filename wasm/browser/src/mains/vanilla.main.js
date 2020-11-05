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
import { csoundApiRename, makeProxyCallback } from "@root/utils";
import { IPCMessagePorts, messageEventHandler } from "@root/mains/messages.main";

class VanillaWorkerMainThread {
  constructor(audioWorker, wasmDataURI) {
    this.ipcMessagePorts = new IPCMessagePorts();
    audioWorker.ipcMessagePorts = this.ipcMessagePorts;

    this.audioStreamIn = new Float64Array(
      MAX_CHANNELS * MAX_HARDWARE_BUFFER_SIZE * Float64Array.BYTES_PER_ELEMENT,
    );

    this.audioStreamOut = new Float64Array(
      MAX_CHANNELS * MAX_HARDWARE_BUFFER_SIZE * Float64Array.BYTES_PER_ELEMENT,
    );

    this.midiBuffer = new Int32Array(
      MIDI_BUFFER_SIZE * MIDI_BUFFER_PAYLOAD_SIZE * Int32Array.BYTES_PER_ELEMENT,
    );

    audioWorker.csoundWorkerMain = this;
    this.audioWorker = audioWorker;
    this.wasmDataURI = wasmDataURI;
    this.exportApi = {};
    this.csoundInstance = undefined;
    this.currentPlayState = undefined;
    this.messageCallbacks = [];
    this.csoundPlayStateChangeCallbacks = [];
    this.midiPortStarted = false;
    this.onPlayStateChange = this.onPlayStateChange.bind(this);
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
        logVAN(`event realtimePerformanceStarted from worker, now preparingRT..`);
        await this.prepareRealtimePerformance();
        break;
      }

      case "realtimePerformanceEnded": {
        logVAN(`realtimePerformanceEnded`);
        this.midiPortStarted = false;
        this.csound = undefined;
        this.currentPlayState = undefined;
        this.ipcMessagePorts.restart(this);
        break;
      }

      case "renderEnded": {
        logVAN(`event: renderEnded received, beginning cleanup`);
        this.ipcMessagePorts.restart(this);
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
        this.audioWorker.onPlayStateChange(newPlayState);
      }
    } catch (error) {
      console.error(`Csound thread crashed while receiving an IPC message: ${error}`);
    }

    this.csoundPlayStateChangeCallbacks.forEach((callback) => {
      try {
        callback(newPlayState);
      } catch (error) {
        console.error(error);
      }
    });
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

  async initialize() {
    logVAN(`vanilla.main: initialize`);
    this.csoundWorker = this.csoundWorker || new Worker(VanillaWorker());
    const audioStreamIn = this.audioStreamIn;
    const audioStreamOut = this.audioStreamOut;
    const midiBuffer = this.midiBuffer;

    logVAN(`mainMessagePort mainMessagePortAudio ports connected to event-listeners`);
    this.ipcMessagePorts.mainMessagePort.addEventListener("message", messageEventHandler(this));
    this.ipcMessagePorts.mainMessagePortAudio.addEventListener(
      "message",
      messageEventHandler(this),
    );

    this.ipcMessagePorts.mainMessagePort.start();
    this.ipcMessagePorts.mainMessagePortAudio.start();
    logVAN(`mainMessagePort- mainMessagePortAudio .start()`);

    const proxyPort = Comlink.wrap(this.csoundWorker);
    this.proxyPort = proxyPort;
    await proxyPort.initialize(this.wasmDataURI);
    this.exportApi.setMessageCallback = this.setMessageCallback.bind(this);
    this.exportApi.addMessageCallback = this.addMessageCallback.bind(this);
    this.exportApi.setCsoundPlayStateChangeCallback = this.setCsoundPlayStateChangeCallback.bind(
      this,
    );
    this.exportApi.addCsoundPlayStateChangeCallback = this.addCsoundPlayStateChangeCallback.bind(
      this,
    );

    const csoundInstance = await makeProxyCallback(proxyPort, undefined, "csoundCreate")();
    await makeProxyCallback(proxyPort, csoundInstance, "csoundInitialize")(0);
    this.csoundInstance = csoundInstance;

    this.exportApi.pause = this.csoundPause.bind(this);
    this.exportApi.resume = this.csoundResume.bind(this);
    this.exportApi.writeToFs = makeProxyCallback(proxyPort, csoundInstance, "writeToFs");
    this.exportApi.readFromFs = makeProxyCallback(proxyPort, csoundInstance, "readFromFs");
    this.exportApi.llFs = makeProxyCallback(proxyPort, csoundInstance, "llFs");
    this.exportApi.lsFs = makeProxyCallback(proxyPort, csoundInstance, "lsFs");
    this.exportApi.rmrfFs = makeProxyCallback(proxyPort, csoundInstance, "rmrfFs");
    this.exportApi.getAudioContext = async () => this.audioWorker.audioCtx;

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
            this.csoundWorker.postMessage({ msg: "initMessagePort" }, [
              this.ipcMessagePorts.workerMessagePort,
            ]);
            this.csoundWorker.postMessage({ msg: "initRequestPort" }, [
              this.ipcMessagePorts.csoundWorkerFrameRequestPort,
            ]);
            this.csoundWorker.postMessage({ msg: "initAudioInputPort" }, [
              this.ipcMessagePorts.csoundWorkerAudioInputPort,
            ]);
            this.csoundWorker.postMessage({ msg: "initRtMidiEventPort" }, [
              this.ipcMessagePorts.csoundWorkerRtMidiPort,
            ]);
            logVAN(`4x message-ports sent to the worker`);
            return await proxyCallback({
              audioStreamIn,
              audioStreamOut,
              midiBuffer,
              csound: csoundInstance,
            });
          };

          csoundStart.toString = () => reference.toString();
          this.exportApi.start = csoundStart.bind(this);
          break;
        }

        case "csoundStop": {
          const brodcastTheEnd = async () =>
            await this.onPlayStateChange("realtimePerformanceEnded");
          const csoundStop = async function (csound) {
            if (!csound || typeof csound !== "number") {
              console.error("csoundStop expects first parameter to be instance of Csound");
              return -1;
            }

            const stopResult = await proxyCallback(csound);
            if (this.currentPlayState === "realtimePerformancePaused") {
              try {
                await proxyPort.callUncloned("csoundPerformKsmps", [csound]);
              } catch (_) {}
              try {
                await brodcastTheEnd();
              } catch (_) {}
            }
            if (this.currentPlayState !== "realtimePerformanceEnded") {
              await brodcastTheEnd();
            }
            return stopResult;
          };
          this.exportApi.stop = csoundStop.bind(this);
          csoundStop.toString = () => reference.toString();
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
