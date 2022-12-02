/* eslint-disable unicorn/require-post-message-target-origin */
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

import * as Comlink from "comlink/dist/esm/comlink.mjs";
import MessagePortState from "../utils/message-port-state";
import libcsoundFactory from "../libcsound";
import loadWasm from "../module";
import { assoc, pipe } from "rambda/dist/rambda.mjs";
import { clearArray } from "../utils/clear-array";
import { logSinglethreadWorkletWorker as log } from "../logger";
import { renderFunction } from "./common.utils";

let libraryCsound;
let combined;
const rtmidiQueue = [];

const callUncloned = async (k, arguments_) => {
  const caller = combined.get(k);
  const returnValue = caller && caller.apply({}, arguments_ || []);
  return returnValue;
};

class WorkletSinglethreadWorker extends AudioWorkletProcessor {
  static get parameterDescriptors() {
    return [];
  }

  constructor(options) {
    super(options);
    // eslint-disable-next-line no-undef
    this.sampleRate = sampleRate;
    this.options = options;
    this.initialize = this.initialize.bind(this);
    this.pause = this.pause.bind(this);
    this.process = this.process.bind(this);
    this.resume = this.resume.bind(this);
    this.start = this.start.bind(this);
    this.needsStartNotification = false;
    this.isPaused = false;
    this.callUncloned = () => console.error("Csound worklet thread is still uninitialized!");
    this.port.start();
    Comlink.expose(this, this.port);
    this.workerMessagePort = new MessagePortState();

    this.initializeMessagePort = ({ messagePort, rtmidiPort }) => {
      this.workerMessagePort.post = (messageLog) => messagePort.postMessage({ log: messageLog });
      this.workerMessagePort.broadcastPlayState = (playStateChange) => {
        if (this.workerMessagePort.workerState !== playStateChange) {
          this.workerMessagePort.workerState = playStateChange;
        }
        messagePort.postMessage({ playStateChange });
      };
      this.workerMessagePort.ready = true;
      log(`initRtMidiEventPort`)();
      this.rtmidiPort = rtmidiPort;
      this.rtmidiPort.addEventListener("message", ({ data: payload }) => {
        rtmidiQueue.push(payload);
      });
      this.rtmidiPort.start();
    };
  }

  async initialize(wasmDataURI, withPlugins) {
    log("initializing worklet.singlethread.worker")();

    let resolver;
    const waiter = new Promise((resolve) => {
      resolver = resolve;
    });

    loadWasm({
      wasmDataURI,
      withPlugins,
      messagePort: this.workerMessagePort,
    }).then(([wasm, wasi]) => {
      this.wasm = wasm;
      this.wasi = wasi;
      wasm.wasi = wasi;

      libraryCsound = libcsoundFactory(wasm);
      this.callUncloned = callUncloned;
      this.csound = libraryCsound.csoundCreate(0);
      this.result = 0;
      this.running = false;
      this.isRendering = false;
      this.started = false;
      this.resetCsound(false);

      const csoundCreate = async (v) => {
        return this.csound;
      };

      const allAPI = pipe(
        assoc("csoundCreate", csoundCreate),
        assoc("csoundReset", this.resetCsound.bind(this)),
        assoc("csoundStart", this.start.bind(this)),
        assoc("csoundStop", this.stop.bind(this)),
        assoc("wasm", wasm),
      )(libraryCsound);

      combined = new Map(Object.entries(allAPI));
      log("wasm initialized and api generated")();
      resolver();
    });
    log("waiting on wasm initialization to complete")();
    await waiter;
  }

  async resetCsound(callReset) {
    // no start = no reset
    if (callReset && !this.workerMessagePort) {
      return -1;
    }
    if (
      callReset &&
      this.workerMessagePort.workerState !== "realtimePerformanceEnded" &&
      this.workerMessagePort.workerState !== "realtimePerformanceStarted"
    ) {
      // reset can't be called until performance has started or ended!
      return -1;
    }
    if (callReset && this.workerMessagePort.workerState === "realtimePerformanceStarted") {
      this.workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
    }

    this.running = false;
    this.started = false;
    this.result = 0;

    const cs = this.csound;

    if (callReset) {
      libraryCsound.csoundReset(cs);
    }

    libraryCsound.csoundSetMidiCallbacks(cs);
    this.sampleRate && libraryCsound.csoundSetOption(cs, "--sample-rate=" + this.sampleRate);
    this.nchnls = -1;
    this.nchnls_i = -1;
    delete this.csoundOutputBuffer;
  }

  stop() {
    if (this.csound) {
      libraryCsound.csoundStop(this.csound);
    }
    this.workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
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
    if (!this.isRendering && (this.isPaused || !this.csoundOutputBuffer || !this.running)) {
      const output = outputs[0];
      const bufferLength = output[0].length;
      for (let index = 0; index < bufferLength; index++) {
        for (let channel = 0; channel < output.numberOfChannels; channel++) {
          const outputChannel = output[channel];
          outputChannel[index] = 0;
        }
      }
      return true;
    }

    // if we are starting, we need to bordcast it
    // this late in order to avoid timing issues
    if (this.needsStartNotification) {
      this.needsStartNotification = false;
      this.workerMessagePort.broadcastPlayState("realtimePerformanceStarted");
    }

    if (rtmidiQueue.length > 0) {
      rtmidiQueue.forEach((event) => {
        libraryCsound.csoundPushMidiMessage(this.csound, event[0], event[1], event[2]);
      });
      clearArray(rtmidiQueue);
    }

    const input = inputs[0];
    const output = outputs[0];

    const bufferLength = output[0].length;

    let csOut = this.csoundOutputBuffer;
    let csIn = this.csoundInputBuffer;
    const ksmps = this.ksmps;
    const zerodBFS = this.zerodBFS;

    let cnt = this.cnt;
    const nchnls = this.nchnls;
    const nchnlsIn = this.nchnls_i;
    let result = this.result;

    for (let index = 0; index < bufferLength; index++, cnt++) {
      if (cnt >= ksmps && result === 0) {
        // if we need more samples from Csound
        result = libraryCsound.csoundPerformKsmps(this.csound);
        cnt = 0;

        if (result !== 0) {
          this.running = false;
          this.started = false;
          libraryCsound.csoundCleanup(this.csound);
          this.workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
        }
      }

      /* Check if MEMGROWTH occured from csoundPerformKsmps or otherwise. If so,
      rest output ant input buffers to new pointer locations. */
      if (!csOut || csOut.length === 0) {
        csOut = this.csoundOutputBuffer = new Float64Array(
          this.wasm.wasi.memory.buffer,
          libraryCsound.csoundGetSpout(this.csound),
          ksmps * nchnls,
        );
      }

      if (!csIn || csIn.length === 0) {
        csIn = this.csoundInputBuffer = new Float64Array(
          this.wasm.wasi.memory.buffer,
          libraryCsound.csoundGetSpin(this.csound),
          ksmps * nchnlsIn,
        );
      }

      // handle 1->1, 1->2, 2->1, 2->2 input channel count mixing and nchnls_i
      const inputChanMax = Math.min(this.nchnls_i, input.length);
      for (let channel = 0; channel < inputChanMax; channel++) {
        const inputChannel = input[channel];
        csIn[cnt * nchnlsIn + channel] = inputChannel[index] * zerodBFS;
      }

      // Channel mixing matches behavior of:
      // https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API/Basic_concepts_behind_Web_Audio_API#Up-mixing_and_down-mixing

      // handle 1->1, 1->2, 2->1, 2->2 output channel count mixing and nchnls
      if (this.nchnls === output.length) {
        for (const [channel, outputChannel] of output.entries()) {
          outputChannel[index] = result === 0 ? csOut[cnt * nchnls + channel] / zerodBFS : 0;
        }
      } else if (this.nchnls === 2 && output.length === 1) {
        const outputChannel = output[0];
        if (result === 0) {
          const left = csOut[cnt * nchnls] / zerodBFS;
          const right = csOut[cnt * nchnls + 1] / zerodBFS;
          outputChannel[index] = 0.5 * (left + right);
        } else {
          outputChannel[index] = 0;
        }
      } else if (this.nchnls === 1 && output.length === 2) {
        const outChan0 = output[0];
        const outChan1 = output[1];

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
    }

    this.cnt = cnt;
    this.result = result;

    return true;
  }

  async isRequestingInput() {
    const cs = this.csound;
    const inputName = libraryCsound.csoundGetInputName(cs) || "";
    return inputName.includes("adc");
  }

  async isRequestingRealtimeOutput() {
    const cs = this.csound;
    const outputName = libraryCsound.csoundGetOutputName(cs) || "";
    return outputName.includes("dac");
  }

  async start() {
    let returnValueValue = -1;
    if (this.started) {
      log("worklet was asked to start but it already has!")();
    } else {
      log("worklet thread is starting..")();
      const cs = this.csound;
      const ksmps = libraryCsound.csoundGetKsmps(cs);
      this.ksmps = ksmps;
      this.cnt = ksmps;
      this.nchnls = libraryCsound.csoundGetNchnls(cs);
      this.nchnls_i = libraryCsound.csoundGetNchnlsInput(cs);

      this.zerodBFS = libraryCsound.csoundGet0dBFS(cs);

      returnValueValue = libraryCsound.csoundStart(cs);

      if (returnValueValue !== 0) {
        return returnValueValue;
      }

      const isExpectingRealtimeOutput = await this.isRequestingRealtimeOutput();

      if (isExpectingRealtimeOutput) {
        this.csoundOutputBuffer = new Float64Array(
          this.wasm.wasi.memory.buffer,
          libraryCsound.csoundGetSpout(cs),
          ksmps * this.nchnls,
        );
        this.csoundInputBuffer = new Float64Array(
          this.wasm.wasi.memory.buffer,
          libraryCsound.csoundGetSpin(cs),
          ksmps * this.nchnls_i,
        );
        log("csoundStart called with {} return val", returnValueValue)();
        this.started = true;
        this.needsStartNotification = true;
      } else {
        this.workerMessagePort.broadcastPlayState("renderStarted");
        this.isRendering = true;
        renderFunction({
          libraryCsound,
          workerMessagePort: this.workerMessagePort,
          wasi: this.wasi,
        })({ csound: cs })
          .then(() => {
            this.workerMessagePort.broadcastPlayState("renderEnded");
            this.isRendering = false;
          })
          .catch(() => {
            this.workerMessagePort.broadcastPlayState("renderEnded");
            this.isRendering = false;
          });

        return 0;
      }
    }
    this.running = true;
    return returnValueValue;
  }
}

registerProcessor("csound-singlethread-worklet-processor", WorkletSinglethreadWorker);
