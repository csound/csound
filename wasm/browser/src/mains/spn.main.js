/*
    CsoundScriptProcessor.js

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

import libcsoundFactory from "../libcsound";
import loadWasm from "../module";
import MessagePortState from "../utils/message-port-state";
import { isEmpty } from "rambda/dist/rambda.mjs";
import { csoundApiRename, fetchPlugins, makeSingleThreadCallback } from "../utils";
import { messageEventHandler } from "./messages.main";
import { PublicEventAPI } from "../events";
import { EventPromises } from "../utils/event-promises";
import { requestMidi } from "../utils/request-midi";

class ScriptProcessorNodeSingleThread {
  constructor({ audioContext, inputChannelCount = 1, outputChannelCount = 2 }) {
    this.publicEvents = new PublicEventAPI(this);
    this.eventPromises = new EventPromises();
    this.audioContext = audioContext;
    this.onaudioprocess = this.onaudioprocess.bind(this);
    this.currentPlayState = undefined;
    this.onPlayStateChange = this.onPlayStateChange.bind(this);
    this.start = this.start.bind(this);
    this.stop = this.stop.bind(this);
    this.pause = this.pause.bind(this);
    this.resume = this.resume.bind(this);
    this.wasm = undefined;
    this.csoundInstance = undefined;
    this.csoundApi = undefined;
    this.exportApi = {};
    this.spn = audioContext.createScriptProcessor(0, inputChannelCount, outputChannelCount);
    this.spn.audioContext = audioContext;
    this.spn.inputChannelCount = inputChannelCount;
    this.spn.outputChannelCount = outputChannelCount;
    this.spn.onaudioprocess = this.onaudioprocess;
    this.node = this.spn;
    this.exportApi.getNode = async () => this.spn;
    this.sampleRate = audioContext.sampleRate;
    // this is the only actual single-thread usecase
    // so we get away with just forwarding it as if it's form
    // a message port
    this.messagePort = new MessagePortState();
    this.messagePort.post = (log) => messageEventHandler(this)({ data: { log } });
    this.messagePort.ready = true;

    // imports from original csound-wasm
    this.running = false;
    this.started = false;
  }

  async terminateInstance() {
    if (this.spn) {
      this.spn.disconnect();
      delete this.spn;
    }
    if (this.audioContext) {
      if (this.audioContext.state !== "closed") {
        await this.audioContext.close();
      }
      delete this.audioContext;
    }
    if (this.publicEvents) {
      this.publicEvents.terminateInstance();
      delete this.publicEvents;
    }
    Object.keys(this.exportApi).forEach((key) => delete this.exportApi[key]);
    Object.keys(this).forEach((key) => delete this[key]);
  }

  async onPlayStateChange(newPlayState) {
    if (!this.publicEvents || this.currentPlayState === newPlayState) {
      return;
    }
    this.currentPlayState = newPlayState;
    switch (newPlayState) {
      case "realtimePerformanceStarted": {
        this.publicEvents.triggerRealtimePerformanceStarted(this);
        break;
      }

      case "realtimePerformanceEnded": {
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

  async pause() {
    if (this.started && this.running) {
      this.running = false;
      this.onPlayStateChange("realtimePerformancePaused");
    }
  }

  async resume() {
    if (this.started && !this.running) {
      this.running = true;
      this.onPlayStateChange("realtimePerformanceResumed");
    }
  }

  async stop() {
    if (this.started) {
      this.eventPromises.createStopPromise();
      const stopResult = this.csoundApi.csoundStop(this.csoundInstance);
      await this.eventPromises.waitForStop();
      if (this.watcherStdOut) {
        this.watcherStdOut.close();
        delete this.watcherStdOut;
      }

      if (this.watcherStdErr) {
        this.watcherStdErr.close();
        delete this.watcherStdErr;
      }

      delete this.csoundInputBuffer;
      delete this.csoundOutputBuffer;
      delete this.currentPlayState;
      return stopResult;
    }
  }

  async start() {
    if (!this.csoundApi) {
      console.error("starting csound failed because csound instance wasn't created");
      return;
    }

    const outputName = this.csoundApi.csoundGetOutputName(this.csoundInstance) || "test.wav";
    const isExpectingRealtimeOutput = outputName.includes("dac");

    if (isExpectingRealtimeOutput && this.currentPlayState !== "realtimePerformanceStarted") {
      this.result = 0;
      this.csoundApi.csoundSetOption(this.csoundInstance, "--sample-rate=" + this.sampleRate);
      this.nchnls = -1;
      this.nchnls_i = -1;

      const ksmps = this.csoundApi.csoundGetKsmps(this.csoundInstance);
      this.ksmps = ksmps;
      this.cnt = ksmps;

      this.nchnls = this.csoundApi.csoundGetNchnls(this.csoundInstance);
      this.nchnls_i = this.csoundApi.csoundGetNchnlsInput(this.csoundInstance);

      const outputPointer = this.csoundApi.csoundGetSpout(this.csoundInstance);
      this.csoundOutputBuffer = new Float64Array(
        this.wasm.wasi.memory.buffer,
        outputPointer,
        ksmps * this.nchnls,
      );

      const inputPointer = this.csoundApi.csoundGetSpin(this.csoundInstance);
      this.csoundInputBuffer = new Float64Array(
        this.wasm.wasi.memory.buffer,
        inputPointer,
        ksmps * this.nchnls_i,
      );
      this.zerodBFS = this.csoundApi.csoundGet0dBFS(this.csoundInstance);

      this.publicEvents.triggerOnAudioNodeCreated(this.spn);
      this.eventPromises.createStartPromise();

      const startResult = this.csoundApi.csoundStart(this.csoundInstance);
      if (this.csoundApi._isRequestingRtMidiInput(this.csoundInstance)) {
        requestMidi({
          onMidiMessage: ({ data: event }) =>
            this.csoundApi.csoundPushMidiMessage(this.csoundInstance, event[0], event[1], event[2]),
        });
      }
      this.running = true;
      await this.eventPromises.waitForStart();
      return startResult;
    } else if (!isExpectingRealtimeOutput && this.currentPlayState !== "renderStarted") {
      const startResult = this.csoundApi.csoundStart(this.csoundInstance);
      this.onPlayStateChange("renderStarted");

      setTimeout(() => {
        let lastResult = 0;
        while (lastResult === 0 && this.csoundApi && this.csoundInstance) {
          lastResult = this.csoundApi.csoundPerformKsmps(this.csoundInstance);
        }

        this.onPlayStateChange && this.onPlayStateChange("renderEnded");
      }, 0);

      return startResult;
    }
  }

  async initialize({ wasmDataURI, withPlugins, autoConnect }) {
    if (!this.plugins && withPlugins && !isEmpty(withPlugins)) {
      withPlugins = await fetchPlugins(withPlugins);
    }

    if (!this.wasm) {
      const [wasm, wasi] = await loadWasm({
        wasmDataURI: wasmDataURI(),
        withPlugins,
        messagePort: this.messagePort,
      });
      this.wasm = wasm;
      this.wasi = wasi;
      wasm.wasi = wasi;
    }

    // libcsound
    const csoundApi = libcsoundFactory(this.wasm);
    this.csoundApi = csoundApi;
    const csoundInstance = await csoundApi.csoundCreate(0);
    this.csoundInstance = csoundInstance;

    if (autoConnect) {
      this.spn.connect(this.audioContext.destination);
    }

    this.resetCsound(false);

    // csoundObj
    Object.keys(csoundApi).reduce((accumulator, apiName) => {
      if (["mkdir", "readdir", "writeFile"].includes(apiName)) {
        accumulator.fs = accumulator.fs || {};
        const reference = csoundApi[apiName];
        const callback = async (...arguments_) =>
          makeSingleThreadCallback(this.wasm, csoundApi[apiName]).apply({}, arguments_);
        callback.toString = reference.toString;
        accumulator.fs[apiName] = callback;
      } else {
        const renamedApiName = csoundApiRename(apiName);
        accumulator[renamedApiName] = (...arguments_) => {
          return makeSingleThreadCallback(csoundInstance, csoundApi[apiName]).apply({}, arguments_);
        };
        accumulator[renamedApiName].toString = csoundApi[apiName].toString;
      }

      return accumulator;
    }, this.exportApi);

    this.exportApi.pause = this.pause.bind(this);
    this.exportApi.resume = this.resume.bind(this);
    this.exportApi.start = this.start.bind(this);
    this.exportApi.stop = this.stop.bind(this);
    this.exportApi.terminateInstance = this.terminateInstance.bind(this);
    this.exportApi.getAudioContext = async () => this.audioContext;
    this.exportApi.name = "Csound: ScriptProcessor Node, Single-threaded";
    // this.exportApi.fs = persistentFilesystem;

    this.exportApi = this.publicEvents.decorateAPI(this.exportApi);

    this.exportApi.reset = () => this.resetCsound(true);
    // the default message listener
    this.exportApi.addListener("message", console.log);
    return this.exportApi;
  }

  async resetCsound(callReset) {
    if (
      callReset &&
      this.currentPlayState !== "realtimePerformanceEnded" &&
      this.currentPlayState !== "realtimePerformanceStarted"
    ) {
      // reset can't be called until performance has started or ended!
      return -1;
    }
    if (this.currentPlayState === "realtimePerformanceStarted") {
      this.onPlayStateChange("realtimePerformanceEnded");
    }

    this.running = false;
    this.started = false;
    this.result = 0;

    const cs = this.csoundInstance;
    const libraryCsound = this.csoundApi;

    if (callReset) {
      libraryCsound.csoundReset(cs);
    }

    libraryCsound.csoundSetOption(cs, "--sample-rate=" + this.sampleRate);
    this.nchnls = -1;
    this.nchnls_i = -1;
    delete this.csoundOutputBuffer;
    delete this.csoundInputBuffer;
  }

  onaudioprocess(event) {
    if (!this.csoundApi || ["renderStarted", "renderEnded"].includes(this.currentPlayState)) {
      return;
    }
    if (this.csoundOutputBuffer === null || this.running === false) {
      const output = event.outputBuffer;
      const channelData = output.getChannelData(0);

      if (channelData) {
        const bufferLength = channelData.length;

        for (let index = 0; index < bufferLength; index++) {
          for (let channel = 0; channel < output.numberOfChannels; channel++) {
            const outputChannel = output.getChannelData(channel);
            outputChannel[index] = 0;
          }
        }
      }
    }

    if (this.running && !this.started) {
      this.started = true;
      this.onPlayStateChange("realtimePerformanceStarted");
      this.eventPromises && this.eventPromises.releaseStartPromise();
    }

    const input = event.inputBuffer;
    const output = event.outputBuffer;

    const bufferLength = output.getChannelData(0).length;

    let csOut = this.csoundOutputBuffer;
    let csIn = this.csoundInputBuffer;

    const ksmps = this.ksmps;
    const zerodBFS = this.zerodBFS;

    const nchnls = this.nchnls;
    const nchnlsIn = this.nchnls_i;

    let cnt = this.cnt || 0;
    let result = this.result || 0;

    for (let index = 0; index < bufferLength; index++, cnt++) {
      if (cnt === ksmps && result === 0) {
        // if we need more samples from Csound
        result = this.csoundApi.csoundPerformKsmps(this.csoundInstance);
        cnt = 0;
        if (result !== 0) {
          this.running = false;
          this.started = false;
          this.onPlayStateChange("realtimePerformanceEnded");
          this.eventPromises && this.eventPromises.releaseStopPromise();
        }
      }

      /* Check if MEMGROWTH occured from csoundPerformKsmps or otherwise. If so,
      rest output ant input buffers to new pointer locations. */
      if (!csOut || csOut.length === 0) {
        csOut = this.csoundOutputBuffer = new Float64Array(
          this.wasm.wasi.memory.buffer,
          this.csoundApi.csoundGetSpout(this.csoundInstance),
          ksmps * nchnls,
        );
      }

      if (!csIn || csIn.length === 0) {
        csIn = this.csoundInputBuffer = new Float64Array(
          this.wasm.wasi.memory.buffer,
          this.csoundApi.csoundGetSpin(this.csoundInstance),
          ksmps * nchnlsIn,
        );
      }

      // handle 1->1, 1->2, 2->1, 2->2 input channel count mixing and nchnls_i
      const inputChanMax = Math.min(this.nchnls_i, input.numberOfChannels);
      for (let channel = 0; channel < inputChanMax; channel++) {
        const inputChannel = input.getChannelData(channel);
        csIn[cnt * nchnlsIn + channel] = inputChannel[index] * zerodBFS;
      }

      // Output Channel mixing matches behavior of:
      // https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API/Basic_concepts_behind_Web_Audio_API#Up-mixing_and_down-mixing

      // handle 1->1, 1->2, 2->1, 2->2 output channel count mixing and nchnls
      if (this.nchnls === output.numberOfChannels) {
        for (let channel = 0; channel < output.numberOfChannels; channel++) {
          const outputChannel = output.getChannelData(channel);
          outputChannel[index] = result === 0 ? csOut[cnt * nchnls + channel] / zerodBFS : 0;
        }
      } else if (this.nchnls === 2 && output.numberOfChannels === 1) {
        const outputChannel = output.getChannelData(0);
        if (result === 0) {
          const left = csOut[cnt * nchnls] / zerodBFS;
          const right = csOut[cnt * nchnls + 1] / zerodBFS;
          outputChannel[index] = 0.5 * (left + right);
        } else {
          outputChannel[index] = 0;
        }
      } else if (this.nchnls === 1 && output.numberOfChannels === 2) {
        const outChan0 = output.getChannelData(0);
        const outChan1 = output.getChannelData(1);

        if (result === 0) {
          const value = csOut[cnt * nchnls] / zerodBFS;
          outChan0[index] = value;
          outChan1[index] = value;
        } else {
          outChan0[index] = 0;
          outChan1[index] = 0;
        }
      } else {
        // FIXME: we do not support other cases at this time
      }

      // for (let channel = 0; channel < input.numberOfChannels; channel++) {
      //   const inputChannel = input.getChannelData(channel);
      //   csIn[cnt * nchnls_i + channel] = inputChannel[i] * zerodBFS;
      // }
      // for (let channel = 0; channel < output.numberOfChannels; channel++) {
      //   const outputChannel = output.getChannelData(channel);
      //   if (result == 0) outputChannel[i] = csOut[cnt * nchnls + channel] / zerodBFS;
      //   else outputChannel[i] = 0;
      // }
    }

    this.cnt = cnt;
    this.result = result;
  }
}

export default ScriptProcessorNodeSingleThread;
