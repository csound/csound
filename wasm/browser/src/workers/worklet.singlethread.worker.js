/*
    worklet.singlethread.worker.js

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

import * as Comlink from "comlink";
import { writeToFs, lsFs, llFs, readFromFs, rmrfFs, workerMessagePort } from "@root/filesystem";
import libcsoundFactory from "@root/libcsound";
import loadWasm from "@root/module";
import { assoc, pipe } from "ramda";
import { logWorklet } from "@root/logger";

// const workerMessagePort = {
//   ready: false,
//   post: () => {},
//   broadcastPlayState: () => {},
// };

let wasm;
let libraryCsound;
let combined;

const callUncloned = async (k, arguments_) => {
  // console.log("calling " + k);
  const caller = combined.get(k);
  const ret = caller && caller.apply({}, arguments_ || []);
  return ret;
};

// const handleCsoundStart = (workerMessagePort, libraryCsound) => (...arguments_) => {
//   const csound = arguments_[0];
//   const startError = libraryCsound.csoundStart(csound);
//   const outputName = libraryCsound.csoundGetOutputName(csound) || "test.wav";
//   // logWorklet(
//   //   `handleCsoundStart: actual csoundStart result ${startError}, outputName: ${outputName}`,
//   // );
//   // if (startError !== 0) {
//   //   workerMessagePort.post(
//   //     `error: csoundStart failed while trying to render ${outputName},` +
//   //       " look out for errors in options and syntax",
//   //   );
//   // }

//   return startError;
// };

class WorkletSinglethreadWorker extends AudioWorkletProcessor {
  static get parameterDescriptors() {
    return [];
  }

  constructor(options) {
    super(options);
    this.options = options;
    this.initialize = this.initialize.bind(this);
    // Comlink.expose(this.initialize, this.port);
    this.callUncloned = () => console.error("Csound worklet thread is still uninitialized!");
    this.port.start();
    Comlink.expose(this, this.port);

    workerMessagePort.post = (log) => this.port.postMessage({ log });
    workerMessagePort.broadcastPlayState = (playStateChange) => {
      workerMessagePort.vanillaWorkerState = playStateChange;
      this.port.postMessage({ playStateChange });
    };
    workerMessagePort.ready = true;

    // this.port.addEventListener("message", (event) => {
    //   console.log(event.data);
    // });
    // this.port.addEventListener("message", (event) => {
    //   if (event.data.msg === "initMessagePort") {
    //     const port = event.ports[0];
    //     workerMessagePort.post = (log) => port.postMessage({ log });
    //     workerMessagePort.broadcastPlayState = (playStateChange) => {
    //       workerMessagePort.vanillaWorkerState = playStateChange;
    //       port.postMessage({ playStateChange });
    //     };
    //     workerMessagePort.ready = true;
    //   }
    // });
  }

  async initialize(wasmDataURI, withPlugins) {
    let resolver;
    const waiter = new Promise((res) => {
      resolver = res;
    });
    loadWasm(wasmDataURI, withPlugins).then((wasm) => {
      this.wasm = wasm;
      libraryCsound = libcsoundFactory(wasm);
      this.callUncloned = callUncloned;
      let cs = (this.csound = libraryCsound.csoundCreate(0));

      this.result = 0;
      this.running = false;
      this.started = false;
      this.sampleRate = sampleRate;

      this.resetCsound(false);

      // const startHandler = handleCsoundStart(this.port, libraryCsound);
      const csoundCreate = (v) => {
        // console.log("Calling csoundCreate");
        return this.csound;
      };
      const allAPI = pipe(
        assoc("writeToFs", writeToFs),
        assoc("readFromFs", readFromFs),
        assoc("lsFs", lsFs),
        assoc("llFs", llFs),
        assoc("rmrfFs", rmrfFs),
        assoc("csoundCreate", csoundCreate),
        assoc("csoundReset", (cs) => this.resetCsound(true)),
        // assoc("csoundStart", startHandler),
        assoc("csoundStart", this.start.bind(this)),
        assoc("wasm", wasm),
      )(libraryCsound);
      combined = new Map(Object.entries(allAPI));
      resolver();
    });

    await waiter;
  }

  async resetCsound(callReset) {
    this.running = false;
    this.started = false;
    this.result = 0;

    let cs = this.csound;

    if (callReset) {
      libraryCsound.csoundReset(cs);
    }

    libraryCsound.csoundSetMidiCallbacks(cs);
    libraryCsound.csoundSetOption(cs, "-odac");
    libraryCsound.csoundSetOption(cs, "-iadc");
    // libraryCsound.csoundSetOption(cs, "-M0");
    // libraryCsound.csoundSetOption(cs, "-+rtaudio=null");
    // libraryCsound.csoundSetOption(cs, "-+rtmidi=null");
    libraryCsound.csoundSetOption(cs, "--sample-rate=" + this.sampleRate);
    this.nchnls = this.options.outputChannelCount[0];
    this.nchnls_i = this.options.numberOfInputs;
    libraryCsound.csoundSetOption(cs, "--nchnls=" + this.nchnls);
    libraryCsound.csoundSetOption(cs, "--nchnls_i=" + this.nchnls_i);
    this.csoundOutputBuffer = null;
  }

  process(inputs, outputs, parameters) {
    if (this.csoundOutputBuffer == null || this.running == false) {
      let output = outputs[0];
      let bufferLen = output[0].length;
      for (let i = 0; i < bufferLen; i++) {
        for (let channel = 0; channel < output.numberOfChannels; channel++) {
          let outputChannel = output[channel];
          outputChannel[i] = 0;
        }
      }
      return true;
    }

    let input = inputs[0];
    let output = outputs[0];

    let bufferLen = output[0].length;

    let csOut = this.csoundOutputBuffer;
    let csIn = this.csoundInputBuffer;
    let ksmps = this.ksmps;
    let zerodBFS = this.zerodBFS;

    let cnt = this.cnt;
    let nchnls = this.nchnls;
    let nchnls_i = this.nchnls_i;
    let result = this.result;

    for (let i = 0; i < bufferLen; i++, cnt++) {
      if (cnt >= ksmps && result == 0) {
        // if we need more samples from Csound
        result = libraryCsound.csoundPerformKsmps(this.csound);
        cnt = 0;

        if (result != 0) {
          this.running = false;
          this.started = false;
          libraryCsound.csoundCleanup(this.csound);
          // this.firePlayStateChange();
        }
      }

      /* Check if MEMGROWTH occured from csoundPerformKsmps or otherwise. If so,
      rest output ant input buffers to new pointer locations. */
      if (csOut.length === 0) {
        csOut = this.csoundOutputBuffer = new Float64Array(
          this.wasm.exports.memory.buffer,
          libraryCsound.csoundGetSpout(this.csound),
          ksmps * nchnls,
        );
      }

      if (csIn.length === 0) {
        csIn = this.csoundInputBuffer = new Float64Array(
          this.wasm.exports.memory.buffer,
          libraryCsound.csoundGetSpin(this.csound),
          ksmps * nchnls_i,
        );
      }

      for (let channel = 0; channel < input.length; channel++) {
        let inputChannel = input[channel];
        csIn[cnt * nchnls_i + channel] = inputChannel[i] * zerodBFS;
      }
      for (let channel = 0; channel < output.length; channel++) {
        let outputChannel = output[channel];
        if (result == 0) outputChannel[i] = csOut[cnt * nchnls + channel] / zerodBFS;
        else outputChannel[i] = 0;
      }
    }

    this.cnt = cnt;
    this.result = result;

    return true;
  }

  start() {
    let retVal = -1;
    if (this.started == false) {
      let cs = this.csound;
      let ksmps = libraryCsound.csoundGetKsmps(cs);
      this.ksmps = ksmps;
      this.cnt = ksmps;

      this.zerodBFS = libraryCsound.csoundGet0dBFS(cs);
      retVal = libraryCsound.csoundStart(cs);

      this.csoundOutputBuffer = new Float64Array(
        this.wasm.exports.memory.buffer,
        libraryCsound.csoundGetSpout(cs),
        ksmps * this.nchnls,
      );
      this.csoundInputBuffer = new Float64Array(
        this.wasm.exports.memory.buffer,
        libraryCsound.csoundGetSpin(cs),
        ksmps * this.nchnls_i,
      );

      this.started = true;
    }
    this.running = true;
    // this.firePlayStateChange();

    return retVal;
  }
}

registerProcessor("csound-singlethread-worklet-processor", WorkletSinglethreadWorker);
