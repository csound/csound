/* eslint-disable unicorn/require-post-message-target-origin */
/*
    worklet.singlethread.worker.js

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
import MessagePortState from "../utils/message-port-state";
import libcsoundFactory from "../libcsound";
import loadWasm from "../module";
import { clearArray } from "../utils/clear-array";
import { logSinglethreadWorkletWorker as log } from "../logger";

const singlethreadWorkerRender =
  ({ libraryCsound, workerMessagePort, setRenderSleep }) =>
  async (payload) => {
    const csound = payload["csound"];
    const kr = libraryCsound.csoundGetKr(csound);
    let lastResult = 0;
    let cnt = 0;

    while (workerMessagePort.workerState === "renderStarted" && lastResult === 0) {
      lastResult = libraryCsound.csoundPerformKsmps(csound);
      cnt += 1;

      if (lastResult === 0 && cnt % (kr * 2) === 0) {
        // this is immediately executed, but allows events to be picked up
        // we use the process loop instead of setTimeout(0)
        await new Promise((resolve) => {
          setRenderSleep(resolve);
        });
      }
    }

    workerMessagePort.broadcastPlayState("renderEnded");
  };

/** @template T */
class WorkletSinglethreadWorker extends AudioWorkletProcessor {
  static get parameterDescriptors() {
    return [];
  }

  constructor(options) {
    super(options);
    this.wasi = undefined;
    this.wasm = undefined;
    this.csoundInputBuffer = undefined;
    this.csoundOutputBuffer = undefined;
    /** @type {(number | undefined)} */
    this.zerodBFS = undefined;
    /** @type {(number | undefined)} */
    this.nchnls = undefined;
    /** @type {(number | undefined)} */
    this.nchnls_i = undefined;
    /** @type {(number | undefined)} */
    this.cnt = undefined;
    /** @type {(number | undefined)} */
    this.ksmps = undefined;
    /** @type {(number | undefined)} */
    this.csound = undefined;
    /** @type {(number | undefined)} */
    this.result = undefined;

    this.rtmidiPort = undefined;
    this.renderSleep = undefined;
    this.libraryCsound = undefined;
    this.combined = undefined;
    this.rtmidiQueue = [];

    /** @suppress {checkTypes} */
    this.sampleRate = globalThis.sampleRate;
    this.options = options;
    /** @export */
    this.initialize = this.initialize.bind(this);
    /** @export */
    this.pause = this.pause.bind(this);
    /** @export */
    this.stop = this.stop.bind(this);
    /** @export */
    this.terminate = this.terminate.bind(this);
    /** @export */
    this.process = this.process.bind(this);
    /** @export */
    this.resume = this.resume.bind(this);
    /** @export */
    this.start = this.start.bind(this);
    /** @export */
    this.isRequestingInput = this.isRequestingInput.bind(this);
    /** @export */
    this.isRequestingRealtimeOutput = this.isRequestingRealtimeOutput.bind(this);
    this.needsStartNotification = false;
    this.isRendering = false;
    this.isPaused = false;
    this.running = false;
    this.started = false;
    this.isTerminated = false;
    /** @export */
    this.callUncloned = async (k, arguments_) => {
      const caller = this.combined && this.combined.get(k);
      if (!caller) {
        console.error("Csound worklet thread is still uninitialized!");
        return undefined;
      }
      const returnValue = caller.apply({}, arguments_ || []);
      return returnValue;
    };
    this.port.start();
    Comlink.expose(this, this.port);
    this.workerMessagePort = new MessagePortState();

    /** @export */
    this.initializeMessagePort = ({ messagePort, rtmidiPort }) => {
      this.workerMessagePort.post = (messageLog) => messagePort.postMessage({ log: messageLog });
      this.workerMessagePort.broadcastPlayState = (playStateChange) => {
        if (this.workerMessagePort.workerState !== playStateChange) {
          this.workerMessagePort.workerState = playStateChange;
        }
        const dispatch = {};
        dispatch["playStateChange"] = playStateChange;
        messagePort.postMessage(dispatch);
      };
      this.workerMessagePort.ready = true;
      log(`initRtMidiEventPort`)();
      this.rtmidiPort = rtmidiPort;
      this.rtmidiPort.addEventListener("message", ({ data: payload }) => {
        this.rtmidiQueue.push(payload);
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

      this.libraryCsound = libcsoundFactory(wasm);
      this.csound = this.libraryCsound.csoundCreate(0);
      this.result = 0;
      this.running = false;
      this.isRendering = false;
      this.started = false;
      this.isTerminated = false;
      this.resetCsound(false);

      const csoundCreate = async (v) => {
        return this.csound;
      };

      const allAPI = {
        ...this.libraryCsound,
        csoundCreate,
        csoundReset: this.resetCsound.bind(this),
        csoundStop: this.stop.bind(this),
        csoundStart: this.start.bind(this),
        wasm,
      };

      this.combined = new Map(Object.entries(allAPI));
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
      this.libraryCsound.csoundReset(cs);
    }

    this.libraryCsound.csoundSetMidiCallbacks(cs);
    if (this.sampleRate) {
      const result = this.libraryCsound.csoundSetOption(cs, "--sample-rate=" + this.sampleRate);
      result !== 0 && console.error("csoundSetOption sample-rate failed:", result);
    }
    this.nchnls = -1;
    this.nchnls_i = -1;
    delete this.csoundOutputBuffer;
    delete this.csoundInputBuffer;
  }

  stop() {
    // Ensure process() cannot keep advancing DSP after a manual stop.
    this.running = false;
    this.started = false;
    this.result = 0;
    this.needsStartNotification = false;
    this.isRendering = false;
    delete this.csoundOutputBuffer;
    delete this.csoundInputBuffer;

    if (this.csound) {
      this.libraryCsound.csoundStop(this.csound);
    }
    this.workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
  }

  terminate() {
    if (this.isTerminated) {
      return;
    }
    this.isTerminated = true;
    const resolveRenderSleep = this.renderSleep;
    this.renderSleep = undefined;
    if (typeof resolveRenderSleep === "function") {
      resolveRenderSleep();
    }
    clearArray(this.rtmidiQueue);
    this.stop();
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
    if (typeof this.renderSleep === "function") {
      const resolve = this.renderSleep;
      this.renderSleep = undefined;
      resolve();
    }

    if (this.isTerminated) {
      (outputs[0] || []).forEach((array) => array.fill(0));
      return false;
    }

    if (this.isRendering || this.isPaused || !this.csoundOutputBuffer || !this.running) {
      const output = outputs[0];
      const bufferLength = output[0].length;
      for (let index = 0; index < bufferLength; index++) {
        for (let channel = 0; channel < this.nchnls; channel++) {
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

    if (this.rtmidiQueue.length > 0) {
      this.rtmidiQueue.forEach((event) => {
        this.libraryCsound["csoundPushMidiMessage"](this.csound, event[0], event[1], event[2]);
      });
      clearArray(this.rtmidiQueue);
    }

    const input = inputs[0];
    const output = outputs[0];

    const bufferLength = output[0].length;

    let csOut = this.csoundOutputBuffer;
    let csIn = this.csoundInputBuffer;
    const ksmps = this.ksmps;
    /** @type {number} */
    const zerodBFS = this.zerodBFS || 1;

    let cnt = this.cnt;
    const nchnls = this.nchnls;
    const nchnlsIn = this.nchnls_i;
    let result = this.result;

    for (let index = 0; index < bufferLength; index++, cnt++) {
      if (cnt >= ksmps && result === 0) {
        // if we need more samples from Csound
        result = this.libraryCsound.csoundPerformKsmps(this.csound);
        cnt = 0;

        if (result !== 0) {
          this.running = false;
          this.started = false;
          this.workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
        }
      }

      /* Check if MEMGROWTH occured from csoundPerformKsmps or otherwise. If so,
      rest output ant input buffers to new pointer locations. */
      if (!csOut || csOut.length === 0) {
        csOut = this.csoundOutputBuffer = new Float64Array(
          this.wasm.wasi.memory.buffer,
          this.libraryCsound.csoundGetSpout(this.csound),
          ksmps * nchnls,
        );
      }

      if (!csIn || csIn.length === 0) {
        csIn = this.csoundInputBuffer = new Float64Array(
          this.wasm.wasi.memory.buffer,
          this.libraryCsound.csoundGetSpin(this.csound),
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
          /** @suppress {checkTypes} */
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

  isRequestingInput() {
    const cs = this.csound;
    const inputName = this.libraryCsound.csoundGetInputName(cs) || "";
    return inputName.includes("adc");
  }

  isRequestingRealtimeOutput() {
    const cs = this.csound;
    const outputName = this.libraryCsound.csoundGetOutputName(cs) || "";
    return outputName.includes("dac");
  }

  async start() {
    let returnValueValue = -1;
    if (this.started) {
      log("worklet was asked to start but it already has!")();
    } else {
      log("worklet thread is starting..")();
      const cs = this.csound;
      const ksmps = this.libraryCsound.csoundGetKsmps(cs);
      this.ksmps = ksmps;
      this.cnt = ksmps;
      this.nchnls = this.libraryCsound.csoundGetNchnls(cs);
      this.nchnls_i = this.libraryCsound.csoundGetNchnlsInput(cs);

      this.zerodBFS = this.libraryCsound.csoundGet0dBFS(cs);

      returnValueValue = this.libraryCsound.csoundStart(cs);

      if (returnValueValue !== 0) {
        return returnValueValue;
      }

      const isExpectingRealtimeOutput = this.isRequestingRealtimeOutput();

      if (isExpectingRealtimeOutput) {
        this.csoundOutputBuffer = new Float64Array(
          this.wasm.wasi.memory.buffer,
          this.libraryCsound.csoundGetSpout(cs),
          ksmps * this.nchnls,
        );
        this.csoundInputBuffer = new Float64Array(
          this.wasm.wasi.memory.buffer,
          this.libraryCsound.csoundGetSpin(cs),
          ksmps * this.nchnls_i,
        );
        log("csoundStart called with {} return val", returnValueValue)();
        this.started = true;
        this.needsStartNotification = true;
      } else {
        this.workerMessagePort.broadcastPlayState("renderStarted");
        this.isRendering = true;

        singlethreadWorkerRender({
          libraryCsound: this.libraryCsound,
          workerMessagePort: this.workerMessagePort,
          setRenderSleep: (resolve) => {
            this.renderSleep = resolve;
          },
        })({ csound: cs })
          .then(() => {
            this.workerMessagePort.broadcastPlayState("renderEnded");
            this.isRendering = false;
          })
          .catch((error) => {
            console.error(error);
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
