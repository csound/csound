import * as Comlink from "comlink/dist/esm/comlink.mjs";
import { logVANMain as log } from "../logger";
import { api as API } from "../libcsound";
import { isEmpty } from "rambda/dist/rambda.mjs";
import { csoundApiRename, fetchPlugins, makeProxyCallback, stopableStates } from "../utils";
import { IPCMessagePorts, messageEventHandler } from "./messages.main";
import { EventPromises } from "../utils/event-promises";
import { PublicEventAPI } from "../events";
import VanillaWorker from "../../dist/__compiled.vanilla.worker.inline.js";

class VanillaWorkerMainThread {
  constructor({
    audioContext,
    audioWorker,
    audioContextIsProvided,
    inputChannelCount,
    outputChannelCount,
  }) {
    this.ipcMessagePorts = new IPCMessagePorts();
    this.eventPromises = new EventPromises();
    this.publicEvents = new PublicEventAPI(this);

    audioWorker.ipcMessagePorts = this.ipcMessagePorts;
    audioWorker.csoundWorkerMain = this;
    audioWorker.publicEvents = this.publicEvents;

    this.audioWorker = audioWorker;
    this.audioContextIsProvided = audioContextIsProvided;

    if (audioContextIsProvided) {
      this.sampleRate = audioContext.sampleRate;
    }
    if (inputChannelCount) {
      this.inputChannelCount = inputChannelCount;
    }

    if (outputChannelCount) {
      this.outputChannelCount = outputChannelCount;
    }

    this.exportApi = {};
    this.csoundInstance = undefined;
    this.currentPlayState = undefined;
    // this.messageCallbacks = [];
    this.midiPortStarted = false;
    this.onPlayStateChange = this.onPlayStateChange.bind(this);
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
    const inputName = await this.exportApi.getInputName(this.csoundInstance);
    this.audioWorker.isRequestingInput = inputName.includes("adc");
    this.audioWorker.isRequestingMidi = await this.exportApi._isRequestingRtMidiInput(
      this.csoundInstance,
    );
    this.audioWorker.outputsCount = await this.exportApi.getNchnls(this.csoundInstance);
    // TODO fix upstream: await this.exportApi.csoundGetNchnlsInput(this.csound);

    this.audioWorker.inputsCount = this.audioWorker.isRequestingInput ? 1 : 0;
    // if (this.audioWorker.scriptProcessorNode) {
    //   this.audioWorker.softwareBufferSize *= 2;
    // }

    log(`vars for rtPerf set`)();
  }

  async onPlayStateChange(newPlayState) {
    if (!this.publicEvents) {
      // prevent error after termination
      return;
    }
    this.currentPlayState = newPlayState;

    switch (newPlayState) {
      case "realtimePerformanceStarted": {
        log(`event: realtimePerformanceStarted from worker, now preparingRT..`)();
        await this.prepareRealtimePerformance();
        break;
      }

      case "realtimePerformanceEnded": {
        log(`event: realtimePerformanceEnded`)();
        // a noop if the stop promise already exists
        this.eventPromises.createStopPromise();
        this.midiPortStarted = false;
        this.publicEvents.triggerRealtimePerformanceEnded(this);
        await this.eventPromises.releaseStopPromise();
        break;
      }

      case "renderStarted": {
        await this.eventPromises.releaseStartPromise();
        this.publicEvents.triggerRenderStarted(this);
        break;
      }
      case "renderEnded": {
        log(`event: renderEnded received, beginning cleanup`)();
        this.publicEvents.triggerRenderEnded(this);
        await this.eventPromises.releaseStopPromise();
        break;
      }

      default: {
        break;
      }
    }

    // forward the message from worker to the audioWorker
    if (!this.audioWorker.ipcMessagePorts) {
      this.audioWorker.ipcMessagePorts = this.ipcMessagePorts;
    }
    await this.audioWorker.onPlayStateChange(newPlayState);
  }

  async csoundPause() {
    if (this.eventPromises.isWaiting("pause")) {
      return -1;
    } else {
      this.eventPromises.createPausePromise();

      this.audioWorker && this.audioWorker.workletProxy !== undefined
        ? await this.audioWorker.workletProxy.pause()
        : await this.audioWorker.onPlayStateChange("realtimePerformancePaused");

      await this.eventPromises.waitForPause();
      return 0;
    }
  }

  async csoundResume() {
    if (this.eventPromises.isWaiting("resume")) {
      return -1;
    } else {
      this.eventPromises.createResumePromise();
      this.audioWorker && this.audioWorker.workletProxy !== undefined
        ? await this.audioWorker.workletProxy.resume()
        : await this.audioWorker.onPlayStateChange("realtimePerformanceResumed");

      await this.eventPromises.waitForResume();
      return 0;
    }
  }

  async initialize({ wasmDataURI, withPlugins }) {
    const wasmBytes = wasmDataURI();
    if (typeof this.audioWorker.initIframe === "function") {
      await this.audioWorker.initIframe();
    }

    if (withPlugins && !isEmpty(withPlugins)) {
      withPlugins = await fetchPlugins(withPlugins);
    }
    log(`vanilla.main: initialize`)();
    this.csoundWorker = this.csoundWorker || new Worker(VanillaWorker());
    this.ipcMessagePorts.mainMessagePort.addEventListener("message", messageEventHandler(this));
    this.ipcMessagePorts.mainMessagePort2.addEventListener("message", messageEventHandler(this));
    this.ipcMessagePorts.mainMessagePort.start();

    const proxyPort = Comlink.wrap(this.csoundWorker);
    this.proxyPort = proxyPort;

    this.csoundInstance = await proxyPort.initialize(
      Comlink.transfer(
        {
          wasmDataURI: wasmBytes,
          messagePort: this.ipcMessagePorts.workerMessagePort,
          requestPort: this.ipcMessagePorts.csoundWorkerFrameRequestPort,
          audioInputPort: this.ipcMessagePorts.csoundWorkerAudioInputPort,
          rtmidiPort: this.ipcMessagePorts.csoundWorkerRtMidiPort,
          // these values are only set if the user provided them
          // during init or by passing audioContext
          sampleRate: this.sampleRate,
          inputChannelCount: this.inputChannelCount,
          outputChannelCount: this.outputChannelCount,
          withPlugins,
        },
        [
          wasmBytes,
          this.ipcMessagePorts.workerMessagePort,
          this.ipcMessagePorts.csoundWorkerFrameRequestPort,
          this.ipcMessagePorts.csoundWorkerAudioInputPort,
          this.ipcMessagePorts.csoundWorkerRtMidiPort,
        ],
      ),
    );

    this.exportApi.pause = this.csoundPause.bind(this);
    this.exportApi.resume = this.csoundResume.bind(this);
    this.exportApi.terminateInstance = this.terminateInstance.bind(this);

    this.exportApi.getAudioContext = async () => this.audioWorker.audioContext;

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

    this.exportApi = this.publicEvents.decorateAPI(this.exportApi);
    this.exportApi.enableAudioInput = () =>
      console.warn(
        `enableAudioInput was ignored: please use -iadc option before calling start with useWorker=true`,
      );

    // the default message listener
    this.exportApi.addListener("message", console.log);

    for (const apiK of Object.keys(API)) {
      const reference = API[apiK];
      const proxyCallback = makeProxyCallback(
        proxyPort,
        this.csoundInstance,
        apiK,
        this.currentPlayState,
      );

      switch (apiK) {
        case "csoundCreate": {
          break;
        }

        case "csoundStart": {
          const csoundStart = async function () {
            if (this.eventPromises.isWaiting("start")) {
              return -1;
            } else {
              this.eventPromises.createStartPromise();

              const startResult = await proxyCallback({
                csound: this.csoundInstance,
              });
              await this.eventPromises.waitForStart();

              return startResult;
            }
          };

          csoundStart.toString = () => reference.toString();
          this.exportApi.start = csoundStart.bind(this);
          break;
        }

        case "csoundStop": {
          const csoundStop = async function () {
            if (this.eventPromises.isWaiting("stop")) {
              return -1;
            } else {
              this.eventPromises.createStopPromise();
              this.ipcMessagePorts.mainMessagePort.postMessage({
                newPlayState:
                  this.currentPlayState === "renderStarted"
                    ? "renderEnded"
                    : "realtimePerformanceEnded",
              });

              await this.eventPromises.waitForStop();
              return 0;
            }
          };
          this.exportApi.stop = csoundStop.bind(this);
          csoundStop.toString = reference.toString;
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
              const resetResult = await proxyCallback([]);
              if (!this.audioContextIsProvided) {
                await this.audioWorker.terminateInstance();
                delete this.audioWorker.audioContext;
              }

              this.ipcMessagePorts.restartAudioWorkerPorts();

              return resetResult;
            }
          };
          this.exportApi.reset = csoundReset.bind(this);
          csoundReset.toString = reference.toString;
          break;
        }

        case "fs": {
          this.exportApi.fs = {};
          Object.keys(reference).forEach((method) => {
            const proxyFsCallback = makeProxyCallback(
              proxyPort,
              this.csoundInstance,
              method,
              this.currentPlayState,
            );
            proxyFsCallback.toString = reference[method].toString;
            this.exportApi.fs[method] = proxyFsCallback;
          });
          break;
        }

        default: {
          proxyCallback.toString = reference.toString;
          this.exportApi[csoundApiRename(apiK)] = proxyCallback;
          break;
        }
      }
    }
    log(`exportAPI generated`)();
  }
}

export default VanillaWorkerMainThread;
