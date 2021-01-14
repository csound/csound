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
import MessagePortState from "@utils/message-port-state";
import { writeToFs, lsFs, llFs, readFromFs, rmrfFs } from "@root/filesystem";
import libcsoundFactory from "@root/libcsound";
import loadWasm from "@root/module";
import { assoc, pipe } from "ramda";
import { logSinglethreadWorkletWorker as log } from "@root/logger";

let libraryCsound;
let combined;

const callUncloned = async (k, arguments_) => {
  const caller = combined.get(k);
  const ret = caller && caller.apply({}, arguments_ || []);
  return ret;
};

class WorkletSinglethreadWorker extends AudioWorkletProcessor {
  static get parameterDescriptors() {
    return [];
  }

  constructor(options) {
    super(options);
    this.options = options;
    this.initialize = this.initialize.bind(this);
    this.pause = this.pause.bind(this);
    this.resume = this.resume.bind(this);
    this.isPaused = false;
    this.callUncloned = () => console.error("Csound worklet thread is still uninitialized!");
    this.port.start();
    Comlink.expose(this, this.port);
    this.workerMessagePort = new MessagePortState();
    this.initializeMessagePort = ({ messagePort }) => {
      this.workerMessagePort.post = (messageLog) => messagePort.postMessage({ messageLog });
      this.workerMessagePort.broadcastPlayState = (playStateChange) => {
        if (this.workerMessagePort.workerState !== playStateChange) {
          this.workerMessagePort.workerState = playStateChange;
        }
        messagePort.postMessage({ playStateChange });
      };
      this.workerMessagePort.ready = true;
    };
  }

  async initialize(wasmDataURI, withPlugins) {
    log("initializing worklet.singlethread.worker")();
    let resolver;
    const waiter = new Promise((res) => {
      resolver = res;
    });
    loadWasm({ wasmDataURI, withPlugins, messagePort: this.workerMessagePort }).then(
      ([wasm, wasmFs]) => {
        this.wasm = wasm;
        this.wasmFs = wasmFs;

        libraryCsound = libcsoundFactory(wasm);
        this.callUncloned = callUncloned;
        this.csound = libraryCsound.csoundCreate(0);
        this.result = 0;
        this.running = false;
        this.started = false;
        this.sampleRate = sampleRate;
        this.resetCsound(false);

        const csoundCreate = (v) => {
          return this.csound;
        };
        const allAPI = pipe(
          assoc("writeToFs", writeToFs(wasmFs)),
          assoc("readFromFs", readFromFs(wasmFs)),
          assoc("lsFs", lsFs(wasmFs)),
          assoc("llFs", llFs(wasmFs)),
          assoc("rmrfFs", rmrfFs(wasmFs)),
          assoc("csoundCreate", csoundCreate),
          assoc("csoundReset", this.resetCsound.bind(true)),
          assoc("csoundStart", this.start.bind(this)),
          assoc("csoundStop", this.stop.bind(this)),
          assoc("wasm", wasm),
        )(libraryCsound);
        combined = new Map(Object.entries(allAPI));
        resolver();
      },
    );

    await waiter;
  }

  async resetCsound(callReset) {
    if (
      this.workerMessagePort.workerState !== "realtimePerformanceEnded" &&
      this.workerMessagePort.workerState !== "realtimePerformanceStarted"
    ) {
      // reset can't be called until performance has started or ended!
      return -1;
    }
    if (this.workerMessagePort.workerState === "realtimePerformanceStarted") {
      this.workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
    }

    this.running = false;
    this.started = false;
    this.result = 0;

    let cs = this.csound;

    if (callReset) {
      libraryCsound.csoundReset(cs);
    }

    libraryCsound.csoundSetMidiCallbacks(cs);
    libraryCsound.csoundSetOption(cs, "--sample-rate=" + this.sampleRate);
    this.nchnls = -1;
    this.nchnls_i = -1;
    this.csoundOutputBuffer = null;
  }

  stop() {
    this.workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
    if (this.csound) {
      libraryCsound.csoundStop(this.csound);
    }
  }

  pause() {
    if (!this.isPaused) {
      this.workerMessagePort.broadcastPlayState("realtimePerformancePaused");
      this.isPaused = true;
    }
  }

  resume() {
    if (this.isPaused) {
      this.workerMessagePort.broadcastPlayState("realtimePerformanceResumed");
      this.isPaused = false;
    }
  }

  process(inputs, outputs) {
    if (this.isPaused || this.csoundOutputBuffer == null || this.running == false) {
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

      // handle 1->1, 1->2, 2->1, 2->2 input channel count mixing and nchnls_i
      const inputChanMax = Math.min(this.nchnls_i, input.length);
      for (let channel = 0; channel < inputChanMax; channel++) {
        let inputChannel = input[channel];
        csIn[cnt * nchnls_i + channel] = inputChannel[i] * zerodBFS;
      }

      // Channel mixing matches behavior of:
      // https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API/Basic_concepts_behind_Web_Audio_API#Up-mixing_and_down-mixing

      // handle 1->1, 1->2, 2->1, 2->2 output channel count mixing and nchnls
      if (this.nchnls == output.length) {
        for (let channel = 0; channel < output.length; channel++) {
          const outputChannel = output[channel];
          if (result == 0) outputChannel[i] = csOut[cnt * nchnls + channel] / zerodBFS;
          else outputChannel[i] = 0;
        }
      } else if (this.nchnls == 2 && output.length == 1) {
        const outputChannel = output[0];
        if (result == 0) {
          const left = csOut[cnt * nchnls] / zerodBFS;
          const right = csOut[cnt * nchnls + 1] / zerodBFS;
          outputChannel[i] = 0.5 * (left + right);
        } else {
          outputChannel[i] = 0;
        }
      } else if (this.nchnls == 1 && output.length == 2) {
        const outChan0 = output[0];
        const outChan1 = output[1];

        if (result == 0) {
          const val = csOut[cnt * nchnls] / zerodBFS;
          outChan0[i] = val;
          outChan1[i] = val;
        } else {
          outChan0[i] = 0;
          outChan1[i] = 0;
        }
      } else {
        // FIXME: we do not support other cases at this time
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
      this.nchnls = libraryCsound.csoundGetNchnls(cs);
      this.nchnls_i = libraryCsound.csoundGetNchnlsInput(cs);

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
      this.workerMessagePort.broadcastPlayState("realtimePerformanceStarted");
    }
    this.running = true;
    // this.firePlayStateChange();

    return retVal;
  }
}

registerProcessor("csound-singlethread-worklet-processor", WorkletSinglethreadWorker);
