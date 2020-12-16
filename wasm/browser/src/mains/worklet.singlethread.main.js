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
import { logWorklet } from "@root/logger";
import { csoundApiRename, makeProxyCallback } from "@root/utils";
import { api as API } from "@root/libcsound";

let initialized = false;
const initializeModule = async (audioContext) => {
  console.log("Initialize Module");
//   if (!initialized) {
    await audioContext.audioWorklet.addModule(WorkletWorker());
    initialized = true;
//   }
  return true;
};

class SingleThreadAudioWorkletMainThread {
  constructor({ audioContext, numberOfInputs = 1, numberOfOutputs = 2 }) {
    this.exportApi = {};
    this.audioContext = audioContext;
    this.inputChannelCount = numberOfInputs;
    this.outputChannelCount = numberOfOutputs;
    this.onPlayStateChange = this.onPlayStateChange.bind(this);
    this.currentPlayState = undefined;
  }
  async onPlayStateChange(newPlayState) {
    this.currentPlayState = newPlayState;
  }

  async csoundPause() {
    if (typeof this.workletProxy !== "undefined") {
      await this.workletProxy.pause();
    }
    this.onPlayStateChange("realtimePerformancePaused");
  }

  async csoundResume() {
    if (typeof this.workletProxy !== "undefined") {
      await this.workletProxy.resume();
    }
    this.onPlayStateChange("realtimePerformanceResumed");
  }

  async initialize(wasmDataURI) {
    await initializeModule(this.audioContext);

    // if (!this.wasm) {
    //   this.wasm = await loadWasm(wasmDataURI);
    // }
    // libcsound
    // const csoundApi = libcsoundFactory(this.wasm);
    // this.worker = WorkletWorker;
    // console.log("Initializing Module");
    // const options = {};
    // options.numberOfInputs = this.inputChannelCount;
    // options.numberOfOutputs = 1;
    // options.outputChannelCount = [this.outputChannelCount];
    // await this.audioContext.audioWorklet.addModule(WorkletWorker());

    // console.log("moduleinitialized");
    // const createWorkletNode = (audoContext, inputsCount) => {
    //   return
    // };

    // this.node = createWorkletNode(this.audoContext, this.inputChannelCount);
    this.node = new AudioWorkletNode(this.audioContext, "csound-singlethread-worklet-processor", {
      inputChannelCount: this.inputChannelCount ? [this.inputChannelCount] : 0,
      outputChannelCount: [this.outputChannelCount || 2],
    });

    try {
      console.log("wrapping Comlink proxy endpoint on the audioWorkletNode.port");
      this.workletProxy = Comlink.wrap(this.node.port);
    } catch (error) {
      console.log("COMLINK ERROR", error);
    }

    await this.workletProxy.initialize(wasmDataURI);
    const csoundInstance = await makeProxyCallback(this.workletProxy, undefined, "csoundCreate")();
    this.csoundInstance = csoundInstance;
    await makeProxyCallback(this.workletProxy, csoundInstance, "csoundInitialize")(0);
    this.exportApi.pause = this.csoundPause.bind(this);
    this.exportApi.resume = this.csoundResume.bind(this);
    this.exportApi.writeToFs = makeProxyCallback(this.workletProxy, csoundInstance, "writeToFs");
    this.exportApi.readFromFs = makeProxyCallback(this.workletProxy, csoundInstance, "readFromFs");
    this.exportApi.llFs = makeProxyCallback(this.workletProxy, csoundInstance, "llFs");
    this.exportApi.lsFs = makeProxyCallback(this.workletProxy, csoundInstance, "lsFs");
    this.exportApi.rmrfFs = makeProxyCallback(this.workletProxy, csoundInstance, "rmrfFs");
    this.exportApi.getAudioContext = async () => this.audioContext;
    this.exportApi.getNode = async () => this.node;

    for (const apiK of Object.keys(API)) {
      const reference = API[apiK];
      const proxyCallback = makeProxyCallback(this.workletProxy, csoundInstance, apiK);
      proxyCallback.toString = () => reference.toString();
      this.exportApi[csoundApiRename(apiK)] = proxyCallback;
    }
    // csoundObj
    // Object.keys(csoundApi).reduce((acc, apiName) => {
    //   const renamedApiName = csoundApiRename(apiName);
    //   acc[renamedApiName] = makeSingleThreadCallback(csoundInstance, csoundApi[apiName]);
    //   return acc;
    // }, this.exportApi);

    return this.exportApi;
  }
}

export default SingleThreadAudioWorkletMainThread;
