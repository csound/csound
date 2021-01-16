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

import WorkletWorker from "@root/workers/worklet.singlethread.worker";
import * as Comlink from "comlink";
import { logSinglethreadWorkletMain as log } from "@root/logger";
import { csoundApiRename, fetchPlugins, makeProxyCallback } from "@root/utils";
import { messageEventHandler, IPCMessagePorts } from "@root/mains/messages.main";
import { api as API } from "@root/libcsound";
import { PublicEventAPI } from "@root/events";
import { enableAudioInput } from "./io.utils";
import { requestMidi } from "@utils/request-midi";

let initialized = false;
const initializeModule = async (audioContext) => {
  log("Initialize Module")();
  //   if (!initialized) {
  await audioContext.audioWorklet.addModule(WorkletWorker());
  initialized = true;
  //   }
  return true;
};

class SingleThreadAudioWorkletMainThread {
  constructor({ audioContext, inputChannelCount = 1, outputChannelCount = 2 }) {
    this.exportApi = {};
    this.ipcMessagePorts = new IPCMessagePorts();
    this.publicEvents = new PublicEventAPI(this);
    this.exportApi = this.publicEvents.decorateAPI(this.exportApi);
    // the default message listener
    this.exportApi.addListener("message", console.log);
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
        if (this.startPromiz) {
          log("Start promise resolved")();
          this.startPromiz();
          delete this.startPromiz;
        }
        this.publicEvents.triggerRealtimePerformanceStarted(this);
        break;
      }

      case "realtimePerformanceEnded": {
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
        this.publicEvents.triggerRenderStarted(this);
        break;
      }
      case "renderEnded": {
        this.publicEvents.triggerRenderEnded(this);
        break;
      }

      default: {
        break;
      }
    }
  }

  async csoundPause() {
    if (typeof this.workletProxy !== "undefined") {
      await this.workletProxy.pause();
    }
  }

  async csoundResume() {
    if (typeof this.workletProxy !== "undefined") {
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

    await this.workletProxy.initialize(wasmDataURI, withPlugins);
    const csoundInstance = await makeProxyCallback(this.workletProxy, undefined, "csoundCreate")();
    this.csoundInstance = csoundInstance;
    await makeProxyCallback(this.workletProxy, csoundInstance, "csoundInitialize")(0);
    this.exportApi.pause = this.csoundPause.bind(this);
    this.exportApi.resume = this.csoundResume.bind(this);
    this.exportApi.terminateInstance = this.terminateInstance.bind(this);
    this.exportApi.writeToFs = makeProxyCallback(this.workletProxy, csoundInstance, "writeToFs");
    this.exportApi.readFromFs = makeProxyCallback(this.workletProxy, csoundInstance, "readFromFs");
    this.exportApi.llFs = makeProxyCallback(this.workletProxy, csoundInstance, "llFs");
    this.exportApi.lsFs = makeProxyCallback(this.workletProxy, csoundInstance, "lsFs");
    this.exportApi.rmrfFs = makeProxyCallback(this.workletProxy, csoundInstance, "rmrfFs");
    this.exportApi.getAudioContext = async () => this.audioContext;
    this.exportApi.getNode = async () => this.node;
    this.exportApi.enableAudioInput = enableAudioInput.bind(this.exportApi);

    this.exportApi.name = "Csound: Audio Worklet, Single-threaded";

    for (const apiK of Object.keys(API)) {
      const reference = API[apiK];
      const proxyCallback = makeProxyCallback(this.workletProxy, csoundInstance, apiK);
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
            const startPromise = new Promise((resolve) => {
              this.startPromiz = resolve;
            });

            const startResult = await proxyCallback({
              csound: csoundInstance,
            });
            if (await this.exportApi._isRequestingRtMidiInput(csoundInstance)) {
              requestMidi({
                onMidiMessage: this.handleMidiInput.bind(this),
              });
            }
            this.publicEvents.triggerOnAudioNodeCreated(this.node);
            await startPromise;
            return startResult;
          };

          csoundStart.toString = () => reference.toString();
          this.exportApi.start = csoundStart.bind(this);
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
