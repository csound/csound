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

    this.port.addEventListener("message", (event) => {
      if (event.data.msg === "initMessagePort") {
        const port = event.ports[0];
        workerMessagePort.post = (log) => port.postMessage({ log });
        workerMessagePort.broadcastPlayState = (playStateChange) => {
          workerMessagePort.vanillaWorkerState = playStateChange;
          port.postMessage({ playStateChange });
        };
        workerMessagePort.ready = true;
      }
    });
  }

  async initialize(wasmDataURI) {
    wasm = this.wasm = await loadWasm(wasmDataURI);
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
    // console.log("AAA", combined);
  }

  async resetCsound(callReset) {
    let cs = this.csound;

    if(callReset) {
      libraryCsound.csoundReset(cs);
    }

    libraryCsound.csoundSetMidiCallbacks(cs);
    libraryCsound.csoundSetOption(cs, "-odac");
    libraryCsound.csoundSetOption(cs, "-iadc");
    libraryCsound.csoundSetOption(cs, "-M0");
    libraryCsound.csoundSetOption(cs, "-+rtaudio=null");
    libraryCsound.csoundSetOption(cs, "-+rtmidi=null");
    libraryCsound.csoundSetOption(cs, "--sample-rate=" + this.sampleRate);
    this.nchnls = this.options.outputChannelCount[0];
    this.nchnls_i = this.options.numberOfInputs;
    libraryCsound.csoundSetOption(cs, "--nchnls=" + this.nchnls);
    libraryCsound.csoundSetOption(cs, "--nchnls_i=" + this.nchnls_i);
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

      if (cnt == ksmps && result == 0) {
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

      let outputPointer = libraryCsound.csoundGetSpout(cs);
      this.csoundOutputBuffer = new Float64Array(
        this.wasm.exports.memory.buffer,
        outputPointer,
        ksmps * this.nchnls,
      );
      let inputPointer = libraryCsound.csoundGetSpin(cs);
      this.csoundInputBuffer = new Float64Array(
        this.wasm.exports.memory.buffer,
        inputPointer,
        ksmps * this.nchnls_i,
      );
      this.zerodBFS = libraryCsound.csoundGet0dBFS(cs);
      retVal = libraryCsound.csoundStart(cs);
      this.started = true;
    }
    this.running = true;
    // this.firePlayStateChange();

    return retVal;
  }

  // compileOrc(orcString) {
  //     Csound.compileOrc(this.csObj, orcString);
  // }
  //
  // getPlayState() {
  //     if(this.running) {
  //         return "playing";
  //     } else if(this.started) {
  //         return "paused"
  //     }
  //     return "stopped";
  // }

  // firePlayStateChange() {
  //     this.port.postMessage(["playState", this.getPlayState()]);
  // }
  //
  //
  // handleMessage(event) {
  //     let data = event.data;
  //     let p = this.port;
  //
  //     switch (data[0]) {
  //     case "compileCSD":
  //         this.result = Csound.compileCSD(this.csObj, data[1]);
  //         break;
  //     case "compileCSDPromise":
  //         p.postMessage([
  //             "compileCSDPromise",
  //             data[1],
  //             Csound.compileCSD(this.csObj, data[2])
  //         ]);
  //         break;
  //     case "compileOrc":
  //         Csound.compileOrc(this.csObj, data[1]);
  //         break;
  //     case "evalCode":
  //         Csound.evaluateCode(this.csObj, data[1]);
  //         break;
  //     case "evalCodePromise":
  //         p.postMessage([
  //             "evalCodePromise",
  //             data[1],
  //             Csound.evaluateCode(this.csObj, data[2])
  //         ]);
  //         break;
  //     case "readScore":
  //         Csound.readScore(this.csObj, data[1]);
  //         break;
  //     case "setControlChannel":
  //         Csound.setControlChannel(this.csObj,
  //                                  data[1], data[2]);
  //         break;
  //     case "setStringChannel":
  //         Csound.setStringChannel(this.csObj,
  //                                 data[1], data[2]);
  //         break;
  //     case "start":
  //         this.start();
  //         break;
  //     case "stop":
  //         this.running = false;
  //         this.started = false;
  //         this.firePlayStateChange();
  //         break;
  //     case "play":
  //         this.start();
  //         break;
  //     case "pause":
  //         this.running = false;
  //         this.started = true;
  //         this.firePlayStateChange();
  //         break;
  //     case "resume":
  //         this.running = true;
  //         this.started = true;
  //         this.firePlayStateChange();
  //         break;
  //     case "setOption":
  //         Csound.setOption(this.csObj, data[1]);
  //         break;
  //     case "reset":
  //         let csObj = this.csObj;
  //         this.started = false;
  //         this.running = false;
  //         Csound.reset(csObj);
  //         Csound.setMidiCallbacks(csObj);
  //         Csound.setOption(csObj, "-odac");
  //         Csound.setOption(csObj, "-iadc");
  //         Csound.setOption(csObj, "-M0");
  //         Csound.setOption(csObj, "-+rtaudio=null");
  //         Csound.setOption(csObj, "-+rtmidi=null");
  //         Csound.setOption(csObj, "--sample-rate="+this.sampleRate);
  //         Csound.prepareRT(csObj);
  //         //this.nchnls = options.numberOfOutputs;
  //         //this.nchnls_i = options.numberOfInputs;
  //         Csound.setOption(csObj, "--nchnls=" + this.nchnls);
  //         Csound.setOption(csObj, "--nchnls_i=" + this.nchnls_i);
  //         this.csoundOutputBuffer = null;
  //         this.ksmps = null;
  //         this.zerodBFS = null;
  //         this.firePlayStateChange();
  //         break;
  //     case "cleanup":
  //         Csound.cleanup(this.csObj);
  //         break;
  //     case "setCurrentDirFS":
  //         let dirPath = data[2];
  //         if (!dirExists(dirPath)) {
  //             mkdirRecursive(dirPath);
  //         }
  //         FS.chdir(ensureRootPrefix(dirPath));
  //         p.postMessage(["setCurrentDirFSDone", data[1]]);
  //         break;
  //     case "writeToFS":
  //         let name = data[1];
  //         let blobData = data[2];
  //         let buf = new Uint8Array(blobData)
  //         let stream = FS.open(name, 'w+');
  //         FS.write(stream, buf, 0, buf.length, 0);
  //         FS.close(stream);
  //
  //         break;
  //     case "unlinkFromFS":
  //         let filePath = data[1];
  //         FS.unlink(filePath);
  //         break;
  //     case "midiMessage":
  //         let byte1 = data[1];
  //         let byte2 = data[2];
  //         let byte3 = data[3];
  //         Csound.pushMidiMessage(this.csObj, byte1, byte2, byte3);
  //         break;
  //     case "getControlChannel":
  //         let channel = data[1];
  //         let value = Csound.getControlChannel(this.csObj, channel);
  //         p.postMessage(["control", channel, value]);
  //         break;
  //     case "getStringChannel":
  //         let schannel = data[1];
  //         let svalue = Csound.getStringChannel(this.csObj, schannel);
  //         svalue = pointerStringify(svalue);
  //         p.postMessage(["stringChannel", schannel, svalue]);
  //         break;
  //     case "getTable":
  //         let buffer = Csound.getTable(this.csObj, data[1]);
  //         let len = Csound.getTableLength(this.csObj, data[1]);
  //         let src = new Float32Array(CSMOD.HEAP8.buffer, buffer, len);
  //         let table = new Float32Array(src);
  //         p.postMessage(["table", data[1], table]);
  //         break;
  //     case "setTableAtIndex":
  //         Csound.setTable(this.csObj, data[1], data[2], data[3]);
  //         break;
  //     case "setTable":
  //         let cstable = new Float32Array(data[2]);
  //         for(let i = 0; i < cstable.length; i++)
  //             Csound.setTable(this.csObj, data[1], i, cstable[i]);
  //         break;
  //     default:
  //         console.log('[CsoundAudioProcessor] Invalid Message: "' + event.data);
  //     }
  // }
}

registerProcessor("csound-singlethread-worklet-processor", WorkletSinglethreadWorker);
