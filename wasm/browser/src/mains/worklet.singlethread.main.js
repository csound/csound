/*
    worklet.singlethread.js

    Copyright (C) 2018 Steven Yi, Victor Lazzarini

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

import * as Comlink from "comlink/dist/esm/comlink.mjs";
import { logSinglethreadWorkletMain as log } from "../logger.js";
import { csoundApiRename, fetchPlugins, makeProxyCallback } from "../utils.js";
import { messageEventHandler, IPCMessagePorts } from "./messages.main.js";
import { api as API } from "../libcsound.js";
import { PublicEventAPI } from "../events.js";
import { enableAudioInput } from "./io.utils.js";
import { requestMidi } from "../utils/request-midi.js";
import { EventPromises } from "../utils/event-promises.js";
import WorkletWorker from "../../dist/__compiled.worklet.singlethread.worker.inline.js";

const initializeModule = async (audioContext) => {
  log("Initialize Module")();
  try {
    await audioContext.audioWorklet.addModule(WorkletWorker());
  } catch (error) {
    console.error("Error calling audioWorklet.addModule", error);
    return false;
  }
  return true;
};

class SingleThreadAudioWorkletMainThread {
  constructor({ audioContext, inputChannelCount = 1, outputChannelCount = 2 }) {
    this.exportApi = {};
    this.ipcMessagePorts = new IPCMessagePorts();
    this.publicEvents = new PublicEventAPI(this);
    this.eventPromises = new EventPromises();

    this.audioContext = audioContext;
    this.inputChannelCount = inputChannelCount;
    this.outputChannelCount = outputChannelCount;

    this.messageCallbacks = [];
    this.onPlayStateChange = this.onPlayStateChange.bind(this);
    this.currentPlayState = undefined;
  }

  async terminateInstance() {
    if (this.node) {
      this.node.disconnect();
      delete this.node;
    }
    if (this.audioContext) {
      if (this.audioContext.state !== "closed") {
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
    Object.keys(this.exportApi).forEach((key) => delete this.exportApi[key]);
    Object.keys(this).forEach((key) => delete this[key]);
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
        this.publicEvents.triggerRealtimePerformanceStarted(this);
        break;
      }

      case "realtimePerformanceEnded": {
        this.midiPortStarted = false;
        this.currentPlayState = undefined;
        this.publicEvents && this.publicEvents.triggerRealtimePerformanceEnded(this);
        this.eventPromises &&
          this.eventPromises.isWaitingToStop() &&
          this.eventPromises.releaseStopPromise();
        // just to be double sure that there's no hanging promise
        this.eventPromises && this.eventPromises.releaseStartPromise();
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
        if (this.eventPromises.isWaitingToStart()) {
          log("Start promise resolved")();
          this.publicEvents.triggerRenderStarted(this);
          this.eventPromises.releaseStartPromise();
        }
        break;
      }
      case "renderEnded": {
        this.publicEvents.triggerRenderEnded(this);
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
      await this.workletProxy.pause();
    }
  }

  async csoundResume() {
    if (this.workletProxy !== undefined) {
      await this.workletProxy.resume();
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
      this.workletProxy = Comlink.wrap(this.node.port);
    } catch (error) {
      console.error("COMLINK ERROR", error);
    }

    await this.workletProxy.initializeMessagePort(
      Comlink.transfer(
        {
          messagePort: this.ipcMessagePorts.workerMessagePort,
          rtmidiPort: this.ipcMessagePorts.csoundWorkerRtMidiPort,
        },
        [this.ipcMessagePorts.workerMessagePort, this.ipcMessagePorts.csoundWorkerRtMidiPort],
      ),
    );
    this.ipcMessagePorts.mainMessagePort.addEventListener("message", messageEventHandler(this));
    this.ipcMessagePorts.mainMessagePort.start();

    await this.workletProxy.initialize(wasmDataURI(), withPlugins);
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

    this.exportApi.pause = this.csoundPause.bind(this);
    this.exportApi.resume = this.csoundResume.bind(this);
    this.exportApi.terminateInstance = this.terminateInstance.bind(this);

    this.exportApi.getAudioContext = async () => this.audioContext;
    this.exportApi.getNode = async () => this.node;
    this.exportApi.enableAudioInput = enableAudioInput.bind(this.exportApi);

    this.exportApi.name = "Csound: Audio Worklet, Single-threaded";
    this.exportApi = this.publicEvents.decorateAPI(this.exportApi);
    // the default message listener
    this.exportApi.addListener("message", console.log);

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
            const isRequestingInput = await this.workletProxy.isRequestingInput();
            const isRequestingRealtimeOutput = await this.workletProxy.isRequestingRealtimeOutput();

            if (isRequestingRealtimeOutput) {
              if (isRequestingInput) {
                this.exportApi.enableAudioInput();
              }

              const isRequestingMidi = await this.exportApi._isRequestingRtMidiInput(
                csoundInstance,
              );

              if (isRequestingMidi) {
                requestMidi({
                  onMidiMessage: this.handleMidiInput.bind(this),
                });
              }

              const startResult = await proxyCallback({ csound: csoundInstance });
              this.publicEvents.triggerOnAudioNodeCreated(this.node);
              await this.eventPromises.waitForStart();
              return startResult;
            } else {
              // because worklet worker can't return while rendering
              proxyCallback({ csound: csoundInstance });
              this.publicEvents.triggerOnAudioNodeCreated(this.node);
              await this.eventPromises.waitForStart();
              return 0;
            }
          };

          csoundStart.toString = () => reference.toString();
          this.exportApi.start = csoundStart.bind(this);
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
          csoundStop.toString = () => reference.toString();
          this.exportApi.stop = csoundStop.bind(this);
          break;
        }

        case "fs": {
          this.exportApi.fs = {};
          Object.keys(reference).forEach((method) => {
            const proxyFsCallback = makeProxyCallback(
              this.workletProxy,
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
          proxyCallback.toString = () => reference.toString();
          this.exportApi[csoundApiRename(apiK)] = proxyCallback;
          break;
        }
      }
    }

    return this.exportApi;
  }
}

export default SingleThreadAudioWorkletMainThread;
