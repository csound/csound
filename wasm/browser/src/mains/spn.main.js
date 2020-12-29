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

import libcsoundFactory from "@root/libcsound";
import loadWasm from "@root/module";
import { isEmpty } from "ramda";
import { csoundApiRename, fetchPlugins, makeSingleThreadCallback } from "@root/utils";

class ScriptProcessorNodeSingleThread {
  constructor({ audioContext, numberOfInputs = 1, numberOfOutputs = 2 }) {
    this.audioContext = audioContext;
    this.onaudioprocess = this.onaudioprocess.bind(this);
    this.currentPlayState = undefined;
    this.start = this.start.bind(this);
    this.wasm = undefined;
    this.csoundInstance = undefined;
    this.csoundApi = undefined;
    this.exportApi = {};
    this.spn = audioContext.createScriptProcessor(0, numberOfInputs, numberOfOutputs);
    this.spn.connect(audioContext.destination);
    this.spn.audioContext = audioContext;
    this.spn.inputCount = numberOfInputs;
    this.spn.outputCount = numberOfOutputs;
    this.spn.onaudioprocess = this.onaudioprocess;
    this.node = this.spn;
    this.exportApi.getNode = async () => this.spn;
    this.numberOfInputs = numberOfInputs;
    this.numberOfOutputs = numberOfOutputs;
    this.sampleRate = audioContext.sampleRate;

    // imports from original csound-wasm
    this.started = false;
  }

  async pause() {}

  async resume() {}

  async setMessageCallback() {}

  async start() {
    if (!this.csoundApi) {
      console.error("starting csound failed because csound instance wasn't created");
      return undefined;
    }

    if (this.currentPlayState !== "realtimePerformanceStarted") {
      const ksmps = this.csoundApi.csoundGetKsmps(this.csoundInstance);
      this.ksmps = ksmps;
      this.cnt = ksmps;

      this.nchnls = this.csoundApi.csoundGetNchnls(this.csoundInstance);
      this.nchnls_i = this.csoundApi.csoundGetNchnlsInput(this.csoundInstance);

      const outputPointer = this.csoundApi.csoundGetSpout(this.csoundInstance);
      this.csoundOutputBuffer = new Float64Array(
        this.wasm.exports.memory.buffer,
        outputPointer,
        ksmps * this.nchnls,
      );

      const inputPointer = this.csoundApi.csoundGetSpin(this.csoundInstance);
      this.csoundInputBuffer = new Float64Array(
        this.wasm.exports.memory.buffer,
        inputPointer,
        ksmps * this.nchnls_i,
      );
      this.zerodBFS = this.csoundApi.csoundGet0dBFS(this.csoundInstance);
      this.started = true;
    }
    // TODO FIRE THE EVENT
    this.currentPlayState = "realtimePerformanceStarted";
    return this.csoundApi.csoundStart(this.csoundInstance);
  }

  async initialize({ wasmDataURI, withPlugins }) {
    if (!this.plugins && withPlugins && !isEmpty(withPlugins)) {
      withPlugins = await fetchPlugins(withPlugins);
    }

    if (!this.wasm) {
      [this.wasm, this.plugins] = await loadWasm(wasmDataURI, withPlugins);
    }

    // libcsound
    const csoundApi = libcsoundFactory(this.wasm);
    this.csoundApi = csoundApi;
    const csoundInstance = await csoundApi.csoundCreate(0);
    this.csoundInstance = csoundInstance;

    // this.plugins.forEach((plugin) => {
    //   console.log(plugin);
    //   console.log("INSTANCE??", this.wasm.exports.memory, plugin.exports.memory);
    //   plugin.exports.wasm_init(csoundInstance);
    // });
    // CSOUND.setMidiCallbacks(cs); // FIXME

    csoundApi.csoundSetOption(csoundInstance, "-odac");
    csoundApi.csoundSetOption(csoundInstance, "-iadc");
    // csoundApi.csoundSetOption(csoundInstance, "-M0");
    // csoundApi.csoundSetOption(csoundInstance, "-+rtaudio=null");
    // csoundApi.csoundSetOption(csoundInstance, "-+rtmidi=null");
    // csoundApi.prepareRT(cs);
    // var sampleRate = context.sampleRate;
    csoundApi.csoundSetOption(csoundInstance, "--sample-rate=" + this.sampleRate);

    // FIXME: don't hardcode nchnls and instead read what csound actually gets and map to mono or stereo out
    csoundApi.csoundSetOption(csoundInstance, "--nchnls=" + this.numberOfOutputs);
    csoundApi.csoundSetOption(csoundInstance, "--nchnls_i=" + this.numberOfInputs);

    // csoundObj
    Object.keys(csoundApi).reduce((acc, apiName) => {
      const renamedApiName = csoundApiRename(apiName);
      acc[renamedApiName] = makeSingleThreadCallback(csoundInstance, csoundApi[apiName]);
      return acc;
    }, this.exportApi);

    this.exportApi.pause = this.pause.bind(this);
    this.exportApi.resume = this.resume.bind(this);
    this.exportApi.setMessageCallback = this.setMessageCallback.bind(this);
    this.exportApi.start = this.start.bind(this);
    this.exportApi.getAudioContext = async () => this.audioContext;
    this.exportApi.name = "Csound: ScriptProcessor Node, Single-threaded";
    return this.exportApi;
  }

  onaudioprocess(e) {
    if (
      this.csoundOutputBuffer === null ||
      this.currentPlayState !== "realtimePerformanceStarted"
    ) {
      const output = e.outputBuffer;
      const bufferLen = output.getChannelData(0).length;

      for (let i = 0; i < bufferLen; i++) {
        for (let channel = 0; channel < output.numberOfChannels; channel++) {
          const outputChannel = output.getChannelData(channel);
          outputChannel[i] = 0;
        }
      }
      return;
    }

    const input = e.inputBuffer;
    const output = e.outputBuffer;

    const bufferLen = output.getChannelData(0).length;

    let csOut = this.csoundOutputBuffer;
    let csIn = this.csoundInputBuffer;

    const ksmps = this.ksmps;
    const zerodBFS = this.zerodBFS;

    const nchnls = this.nchnls;
    const nchnls_i = this.nchnls_i;

    let cnt = this.cnt || 0;
    let result = this.result || 0;

    for (let i = 0; i < bufferLen; i++, cnt++) {
      if (cnt == ksmps && result == 0) {
        // if we need more samples from Csound
        result = this.csoundApi.csoundPerformKsmps(this.csoundInstance);
        cnt = 0;
        if (result != 0) {
          // this.running = false;
          // this.started = false;
          // this.firePlayStateChange();
          // TODO fire event
          this.currentPlayState = "realtimePerformanceEnded";
        }
      }

      /* Check if MEMGROWTH occured from csoundPerformKsmps or otherwise. If so,
      rest output ant input buffers to new pointer locations. */
      if (csOut.length === 0) {
        csOut = this.csoundOutputBuffer = new Float64Array(
          this.wasm.exports.memory.buffer,
          this.csoundApi.csoundGetSpout(this.csoundInstance),
          ksmps * nchnls,
        );
      }

      if (csIn.length === 0) {
        csIn = this.csoundInputBuffer = new Float64Array(
          this.wasm.exports.memory.buffer,
          this.csoundApi.csoundGetSpin(this.csoundInstance),
          ksmps * nchnls_i,
        );
      }

      for (let channel = 0; channel < input.numberOfChannels; channel++) {
        const inputChannel = input.getChannelData(channel);
        csIn[cnt * nchnls_i + channel] = inputChannel[i] * zerodBFS;
      }
      for (let channel = 0; channel < output.numberOfChannels; channel++) {
        const outputChannel = output.getChannelData(channel);
        if (result == 0) outputChannel[i] = csOut[cnt * nchnls + channel] / zerodBFS;
        else outputChannel[i] = 0;
      }
    }

    this.cnt = cnt;
    this.result = result;
  }
}

export default ScriptProcessorNodeSingleThread;
