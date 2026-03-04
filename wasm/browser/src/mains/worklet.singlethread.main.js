/*
    worklet.singlethread.js

    Copyright (C) 2018 The Csound Developers

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

import * as Comlink from "../utils/comlink.js";
import { logSinglethreadWorkletMain as log } from "../logger.js";
import { csoundApiRename, fetchPlugins, makeProxyCallback } from "../utils.js";
import { messageEventHandler, IPCMessagePorts } from "./messages.main.js";
import { api as API } from "../libcsound.js";
import { PublicEventAPI } from "../events.js";
import { enableAudioInput } from "./io.utils.js";
import { requestMidi } from "../utils/request-midi.js";
import { EventPromises } from "../utils/event-promises.js";
import WorkletWorker from "../../dist/__compiled.worklet.singlethread.worker.inline.js";

const registeredContexts = new WeakSet();

const initializeModule = async (audioContext) => {
  if (registeredContexts.has(audioContext)) {
    log("Module already registered on this AudioContext, skipping addModule")();
    return true;
  }
  log("Initialize Module")();
  try {
    await audioContext.audioWorklet.addModule(WorkletWorker());
    registeredContexts.add(audioContext);
  } catch (error) {
    console.error("Error calling audioWorklet.addModule", error);
    return false;
  }
  return true;
};

/**
 * @unrestricted
 */
class SingleThreadAudioWorkletMainThread {
  constructor({
    audioContext,
    audioContextIsProvided = false,
    inputChannelCount = 1,
    outputChannelCount = 2,
  }) {
    /** @type {(WorkletSinglethreadProxy | undefined)} */
    this.workletProxy = undefined;
    this.node = undefined;
    this.csoundInstance = undefined;

    this.exportApi = {};
    this.ipcMessagePorts = new IPCMessagePorts();
    this.publicEvents = new PublicEventAPI(this);
    this.eventPromises = new EventPromises();

    this.audioContext = audioContext;
    this.audioContextIsProvided = audioContextIsProvided;
    this.inputChannelCount = inputChannelCount;
    this.outputChannelCount = outputChannelCount;

    this.messageCallbacks = [];

    /** @export */
    this.onPlayStateChange = this.onPlayStateChange.bind(this);
    this["handleMidiInput"] = this.handleMidiInput.bind(this);
    this.currentPlayState = undefined;
    this.midiPortStarted = false;
    this.needsResetBeforeCompileCsd = false;
  }

  async terminateInstance() {
    if (this.workletProxy) {
      try {
        await this.workletProxy["terminate"]();
      } catch {}
    }
    if (this.node) {
      this.node.disconnect();
      delete this.node;
    }
    if (this.audioContext) {
      if (!this.audioContextIsProvided && this.audioContext.state !== "closed") {
        await this.audioContext.close();
      }
      delete this.audioContext;
    }
    if (this.workletProxy) {
      this.workletProxy[Comlink.releaseProxy]();
      delete this.workletProxy;
    }
    if (this.publicEvents) {
      this.publicEvents.terminateInstance();
      delete this.publicEvents;
    }
  }

  async onPlayStateChange(newPlayState) {
    if (this.currentPlayState === newPlayState || !this.publicEvents) {
      return;
    }

    this.currentPlayState = newPlayState;

    switch (newPlayState) {
      case "realtimePerformanceStarted": {
        if (this.eventPromises.isWaitingToStart()) {
          log("Start promise resolved")();
          this.eventPromises.releaseStartPromise();
        }
        this.publicEvents.triggerRealtimePerformanceStarted();
        break;
      }

      case "realtimePerformanceEnded": {
        this.midiPortStarted = false;
        this.currentPlayState = undefined;
        this.publicEvents && this.publicEvents.triggerRealtimePerformanceEnded();
        this.eventPromises &&
          this.eventPromises.isWaitingToStop() &&
          this.eventPromises.releaseStopPromise();
        // just to be double sure that there's no hanging promise
        this.eventPromises && this.eventPromises.releaseStartPromise();
        break;
      }
      case "realtimePerformancePaused": {
        this.publicEvents.triggerRealtimePerformancePaused();
        break;
      }
      case "realtimePerformanceResumed": {
        this.publicEvents.triggerRealtimePerformanceResumed();
        break;
      }
      case "renderStarted": {
        if (this.eventPromises.isWaitingToStart()) {
          log("Start promise resolved")();
          this.publicEvents.triggerRenderStarted();
          this.eventPromises.releaseStartPromise();
        }
        break;
      }
      case "renderEnded": {
        this.currentPlayState = undefined;
        this.publicEvents.triggerRenderEnded();
        this.eventPromises &&
          this.eventPromises.isWaitingToStop() &&
          this.eventPromises.releaseStopPromise();
        // just to be double sure that there's no hanging promise
        this.eventPromises && this.eventPromises.releaseStartPromise();
        break;
      }

      default: {
        break;
      }
    }
  }

  async csoundPause() {
    if (this.workletProxy !== undefined) {
      await this.workletProxy["pause"]();
    }
  }

  async csoundResume() {
    if (this.workletProxy !== undefined) {
      await this.workletProxy["resume"]();
    }
  }

  handleMidiInput({ data: payload }) {
    this.ipcMessagePorts.csoundMainRtMidiPort.postMessage &&
      this.ipcMessagePorts.csoundMainRtMidiPort.postMessage(payload);
  }

  async initialize({ wasmDataURI, withPlugins, autoConnect }) {
    if (withPlugins && withPlugins.length > 0) {
      withPlugins = await fetchPlugins(withPlugins);
    }

    await initializeModule(this.audioContext);

    this.node = new AudioWorkletNode(this.audioContext, "csound-singlethread-worklet-processor", {
      inputChannelCount: this.inputChannelCount ? [this.inputChannelCount] : 0,
      outputChannelCount: [this.outputChannelCount || 2],
    });

    if (autoConnect) {
      this.node.connect(this.audioContext.destination);
    }

    try {
      log("wrapping Comlink proxy endpoint on the audioWorkletNode.port")();
      this.workletProxy = Comlink.wrap(this.node.port, undefined);
    } catch (error) {
      console.error("COMLINK ERROR", error);
    }

    /** @nocollapse */
    const initializeMessagePortPayload = {};

    initializeMessagePortPayload["messagePort"] = this.ipcMessagePorts.workerMessagePort;
    initializeMessagePortPayload["rtmidiPort"] = this.ipcMessagePorts.csoundWorkerRtMidiPort;

    await this.workletProxy["initializeMessagePort"](
      Comlink.transfer(initializeMessagePortPayload, [
        this.ipcMessagePorts.workerMessagePort,
        this.ipcMessagePorts.csoundWorkerRtMidiPort,
      ]),
    );
    this.ipcMessagePorts.mainMessagePort.addEventListener("message", messageEventHandler(this));
    this.ipcMessagePorts.mainMessagePort.start();

    await this.workletProxy["initialize"](wasmDataURI(), withPlugins);
    const csoundInstance = await makeProxyCallback(
      this.workletProxy,
      undefined,
      "csoundCreate",
      this.currentPlayState,
    )();
    this.csoundInstance = csoundInstance;
    await makeProxyCallback(
      this.workletProxy,
      csoundInstance,
      "csoundInitialize",
      this.currentPlayState,
    )(0);

    this.exportApi["pause"] = this.csoundPause.bind(this);
    this.exportApi["resume"] = this.csoundResume.bind(this);
    this.exportApi["terminateInstance"] = this.terminateInstance.bind(this);

    this.exportApi["getAudioContext"] = async () => this.audioContext;
    /** @suppress {checkTypes} */
    this.exportApi["getNode"] = async () => this.node;
    /** @suppress {checkTypes} */
    this.exportApi["enableAudioInput"] = enableAudioInput;
    this.exportApi["name"] = "Csound: Audio Worklet, Single-threaded";
    this.exportApi = this.publicEvents.decorateAPI(this.exportApi);
    // the default message listener
    this.exportApi["addListener"]("message", console.log);

    for (const apiK of Object.keys(API)) {
      const reference = API[apiK];
      const proxyCallback = makeProxyCallback(
        this.workletProxy,
        csoundInstance,
        apiK,
        this.currentPlayState,
      );
      switch (apiK) {
        case "csoundCreate": {
          break;
        }

        case "csoundStart": {
          const csoundStart = async function () {
            this.eventPromises.createStartPromise();
            const isRequestingInput = await this.workletProxy["isRequestingInput"]();
            const isRequestingRealtimeOutput = await this.workletProxy[
              "isRequestingRealtimeOutput"
            ]();

            if (isRequestingRealtimeOutput) {
              if (isRequestingInput) {
                this.exportApi["enableAudioInput"]();
              }

              const isRequestingMidi = await this.exportApi["_isRequestingRtMidiInput"](
                csoundInstance,
              );

              if (isRequestingMidi) {
                requestMidi({
                  onMidiMessage: this["handleMidiInput"],
                });
              }

              const startResult = await proxyCallback({ csound: csoundInstance });
              this.publicEvents.triggerOnAudioNodeCreated(this.node);
              await this.eventPromises.waitForStart();
              if (startResult === 0) {
                this.needsResetBeforeCompileCsd = true;
              }
              return startResult;
            } else {
              // because worklet worker can't return while rendering
              proxyCallback({ csound: csoundInstance });
              this.publicEvents.triggerOnAudioNodeCreated(this.node);
              await this.eventPromises.waitForStart();
              this.needsResetBeforeCompileCsd = true;
              return 0;
            }
          };

          csoundStart["toString"] = () => reference["toString"]();
          this.exportApi["start"] = csoundStart.bind(this);
          break;
        }
        case "csoundStop": {
          const csoundStop = async () => {
            if (this.eventPromises.isWaitingToStop()) {
              log("already waiting to stop, doing nothing")();
              return -1;
            } else {
              this.eventPromises.createStopPromise();
              const stopResult = await proxyCallback();
              await this.eventPromises.waitForStop();
              return stopResult;
            }
          };
          csoundStop["toString"] = () => reference["toString"]();
          this.exportApi.stop = csoundStop.bind(this);
          break;
        }

        case "csoundCompileCSD": {
          const csoundCompileCSD = async (...arguments_) => {
            // Starting a new CSD after any previous run must reset engine state first.
            if (
              this.needsResetBeforeCompileCsd &&
              (this.currentPlayState === undefined || this.currentPlayState === "renderEnded")
            ) {
              await this.workletProxy["csoundReset"](csoundInstance);
              this.needsResetBeforeCompileCsd = false;
            }
            return proxyCallback(...arguments_);
          };
          csoundCompileCSD["toString"] = () => reference["toString"]();
          this.exportApi[csoundApiRename(apiK)] = csoundCompileCSD.bind(this);
          break;
        }

        case "csoundReset": {
          const csoundReset = async (...arguments_) => {
            const resetResult = await proxyCallback(...arguments_);
            this.needsResetBeforeCompileCsd = false;
            return resetResult;
          };
          csoundReset["toString"] = () => reference["toString"]();
          this.exportApi[csoundApiRename(apiK)] = csoundReset.bind(this);
          break;
        }

        case "fs": {
          this.exportApi["fs"] = {};
          Object.keys(reference).forEach((method) => {
            const proxyFsCallback = makeProxyCallback(
              this.workletProxy,
              csoundInstance,
              method,
              this.currentPlayState,
            );
            proxyFsCallback["toString"] = () => reference[method]["toString"]();
            this.exportApi["fs"][method] = proxyFsCallback;
          });
          break;
        }

        default: {
          proxyCallback["toString"] = () => reference["toString"]();
          this.exportApi[csoundApiRename(apiK)] = proxyCallback;
          break;
        }
      }
    }

    return this.exportApi;
  }
}

export default SingleThreadAudioWorkletMainThread;
