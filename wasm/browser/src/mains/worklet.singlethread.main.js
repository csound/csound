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

let initialized = false;
const initializeModule = async (audioContext) => {

    console.log("Initialize Module");
    if(!initialized) {
        await audioContext.audioWorklet.addModule(WorkletWorker());
        initialized = true;
    }

    return true;
}

class SingleThreadAudioWorkletMainThread {
  constructor({audioContext, numberOfInputs = 1, numberOfOutputs = 2 }) {
      this.audioContext = audioContext;
      this.inputChannelCount = numberOfInputs;
      this.outputChannelCount = numberOfOutputs;
  }

  getNode() {
      return this.node;
  }

  async initialize() {
    await initializeModule(this.audioContext);

      console.log("Initializing Module");
      const options = {};
      options.numberOfInputs  = this.inputChannelCount;
      options.numberOfOutputs = 1;
      options.outputChannelCount = [ this.outputChannelCount ];

      console.log("Creating AudioWorkletNode");
      this.node = new AudioWorkletNode(this.audioContext, "Csound", options);

      try {
          logWorklet("wrapping Comlink proxy endpoint on the audioWorkletNode.port");
          this.workletProxy = Comlink.wrap(this.node.port);
      } catch (error) {
          log.error("COMLINK ERROR", error);
      }
    return this;
  }

}


// /** This ES6 Class defines a Custom Node as an AudioWorkletNode
//  *  that holds a Csound engine.
//  */
// WorkletSinglethreadMain = class extends AudioWorkletNode {
//
//     /**
//      *
//      * @constructor
//      * @param {AudioContext} context AudioContext in which this node will run
//      * @param {object} options Configuration options, holding numberOfInputs,
//      *   numberOfOutputs
//      * @returns {object} A new WorkletSinglethreadMain
//      */
//     constructor(context, options) {
//         options = options || {};
//
//         super(context, 'Csound', options);
//
//         this.msgCallback = (msg) => { console.log(msg); }
//
//         this.port.start();
//         this.channels =  {};
//         this.promiseBuffer = {};
//         this.channelCallbacks = {};
//         this.stringChannels =  {};
//         this.stringChannelCallbacks = {};
//         this.table = {};
//         this.tableCallbacks = {};
//         this.playState = "stopped";
//         this.playStateListeners = new Set();
//         this.port.onmessage = (event) => {
//             let data = event.data;
//             switch(data[0]) {
//             case "log":
//                 this.msgCallback(data[1]);
//                 break;
//             case "evalCodePromise":
//                 if (typeof this.promiseBuffer[data[1]] === "function") {
//                     this.promiseBuffer[data[1]](data[2]);
//                     delete this.promiseBuffer[data[1]];
//                 }
//                 break;
//             case "compileCSDPromise":
//                 if (typeof this.promiseBuffer[data[1]] === "function") {
//                     this.promiseBuffer[data[1]](data[2]);
//                     delete this.promiseBuffer[data[1]];
//                 }
//                 break;
//             case "setCurrentDirFSDone":
//                 if (typeof this.promiseBuffer[data[1]] === "function") {
//                     this.promiseBuffer[data[1]](true);
//                     delete this.promiseBuffer[data[1]];
//                 }
//                 break;
//             case "control":
//                 this.channels[data[1]] = data[2];
//                 if (typeof this.channelCallbacks[data[1]] != 'undefined')
//                       this.channelCallbacks[data[1]]();
//                 break;
//             case "stringChannel":
//                 this.stringChannels[data[1]] = data[2];
//                 if (typeof this.stringChannelCallbacks[data[1]] != 'undefined')
//                       this.stringChannelCallbacks[data[1]]();
//                 break;
//             case "table":
//                 this.table[data[1]] = data[2];
//                 if (typeof this.tableCallbacks[data[1]] != 'undefined')
//                       this.tableCallbacks[data[1]]();
//                break;
//             case "playState":
//                 this.playState = data[1];
//                 this.firePlayStateChange();
//                 break;
//             default:
//                 console.log('[WorkletSinglethreadMain] Invalid Message: "' + event.data);
//             }
//         };
//     }
//
//     /** Calls FS.chdir and changes the current directory root
//      * @param {string} a path to set cwd to
//      */
//     setCurrentDirFS(dirPath) {
//         return new Promise(resolve => {
//             const promiseId = gensym();
//             this.promiseBuffer[promiseId] = resolve;
//             this.port.postMessage(["setCurrentDirFS", promiseId, dirPath]);
//         });
//     }
//
//     /** Writes data to a file in the WASM filesystem for
//      *  use with csound.
//      *
//      * @param {string} filePath A string containing the path to write to.
//      * @param {blob}   blobData The data to write to file.
//      */
//     writeToFS(filePath, blobData) {
//         this.port.postMessage(["writeToFS", filePath, blobData]);
//     }
//
//     /**
//      *
//      * Unlink file from WASM filesystem (i.e. remove).
//      *
//      * @param {string} filePath A string containing the path to write to.
//      * @memberof CsoundMixin
//      */
//     unlinkFromFS(filePath) {
//         this.port.postMessage(["unlinkFromFS", filePath]);
//     }
//
//     /** Compiles a CSD, which may be given as a filename in the
//      *  WASM filesystem or a string containing the code
//      *
//      * @param {string} csd A string containing the CSD filename or the CSD code.
//      */
//     compileCSD(filePath) {
//         this.port.postMessage(["compileCSD", filePath]);
//     }
//
//     compileCSDPromise(filePath) {
//         return new Promise((resolve, reject) => {
//             const promiseId = gensym();
//             this.promiseBuffer[promiseId] = resolve;
//             this.port.postMessage(["compileCSDPromise", promiseId, filePath]);
//         });
//     }
//
//     /** Compiles Csound orchestra code.
//      *
//      * @param {string} orcString A string containing the orchestra code.
//      */
//     compileOrc(orcString) {
//         this.port.postMessage(["compileOrc", orcString]);
//     }
//
//     /** Sets a Csound engine option (flag)
//      *
//      *
//      * @param {string} option The Csound engine option to set. This should
//      * not contain any whitespace.
//      */
//     setOption(option) {
//         this.port.postMessage(["setOption", option]);
//     }
//
//     render(filePath) {
//     }
//
//     /** Evaluates Csound orchestra code.
//      *
//      * @param {string} codeString A string containing the orchestra code.
//      */
//     evaluateCode(codeString) {
//         this.port.postMessage(["evalCode", codeString]);
//     }
//
//     evaluateCodePromise(codeString) {
//         return new Promise((resolve, reject) => {
//             const promiseId = gensym();
//             this.promiseBuffer[promiseId] = resolve;
//             this.port.postMessage(["evalCodePromise", promiseId, codeString]);
//         });
//     }
//
//     /** Reads a numeric score string.
//      *
//      * @param {string} scoreString A string containing a numeric score.
//      */
//     readScore(scoreString) {
//         this.port.postMessage(["readScore", scoreString]);
//     }
//
//     /** Sets the value of a control channel in the software bus
//      *
//      * @param {string} channelName A string containing the channel name.
//      * @param {number} value The value to be set.
//      */
//     setControlChannel(channelName, value) {
//         this.port.postMessage(["setControlChannel",
//                                channelName, value]);
//     }
//
//     /** Sets the value of a string channel in the software bus
//      *
//      * @param {string} channelName A string containing the channel name.
//      * @param {string} stringValue The string to be set.
//      */
//     setStringChannel(channelName, value) {
//         this.port.postMessage(["setStringChannel",
//                                channelName, value]);
//     }
//
//     /** Request the data from a control channel
//      *
//      * @param {string} channelName A string containing the channel name.
//      * @param {function} callback An optional callback to be called when
//      *  the requested data is available. This can be set once for all
//      *  subsequent requests.
//      */
//     requestControlChannel(channelName, callback = null) {
//         this.port.postMessage(["getControlChannel",
//                                channelName]);
//         if (callback !== null)
//           this.channelCallbacks[channelName] = callback;
//     }
//
//     /** Request the data from a String channel
//      *
//      * @param {string} channelName A string containing the channel name.
//      * @param {function} callback An optional callback to be called when
//      *  the requested data is available. This can be set once for all
//      *  subsequent requests.
//      */
//     requestStringChannel(channelName, callback = null) {
//         this.port.postMessage(["getStringChannel",
//                                channelName]);
//         if (callback !== null)
//           this.stringChannelCallbacks[channelName] = callback;
//     }
//
//     /** Get the latest requested channel data
//      *
//      * @param {string} channelName A string containing the channel name.
//      * @returns {number} The latest channel value requested.
//      */
//     getControlChannel(channelName) {
//         return this.channels[channelName];
//     }
//
//     /** Get the latest requested string channel data
//      *
//      * @param {string} channelName A string containing the channel name.
//      * @returns {string} The latest channel value requested.
//      */
//     getStringChannel(channelName) {
//         return this.stringChannels[channelName];
//     }
//
//      /** Request the data from a Csound function table
//      *
//      * @param {number} number The function table number
//      * @param {function} callback An optional callback to be called when
//      *  the requested data is available. This can be set once for all
//      *  subsequent requests.
//      */
//     requestTable(number, callback = null) {
//         this.port.postMessage(["getTable", number]);
//         if (callback !== null)
//           this.tableCallbacks[number] = callback;
//     }
//
//     /** Get the requested table number
//      *
//      * @param {number} number The function table number
//      * @returns {Float32Array} The table as a typed array.
//      */
//     getTable(number) {
//         return this.table[number];
//     }
//
//     /** Set a specific table position
//      *
//      * @param {number} number The function table number
//      * @param {number} index The index of the position to be set
//      * @param {number} value The value to set
//      */
//     setTableValue(number, index, value) {
//         this.port.postMessage(["setTableAtIndex", number,
//                               index, value]);
//     }
//
//     /** Set a table with data from an array
//      *
//      * @param {number} number The function table number
//      * @param {Float32Array} table The source data for the table
//      */
//     setTable(number, table) {
//         this.port.postMessage(["setTable", number, table]);
//     }
//
//     /** Starts processing in this node
//      */
//     start() {
//         this.port.postMessage(["start"]);
//     }
//
//     /** Resets the Csound engine.
//      */
//     reset() {
//         this.port.postMessage(["reset"]);
//     }
//
//     /** Prints information about the end of a performance, and closes audio
//      * and MIDI devices.
//      * Note: after calling csoundCleanup(), the operation of the perform
//      * functions is undefined. */
//     cleanup() {
//         this.port.postMessage(["cleanup"]);
//     }
//
//     destroy() {
//     }
//
//     pause() {
//         this.port.postMessage(["pause"]);
//     }
//
//     /** Resumes a paused performance
//      */
//     resume() {
//         this.port.postMessage(["resume"]);
//     }
//
//     /** Starts performance, same as start()
//      */
//     play() {
//         this.port.postMessage(["play"]);
//     }
//
//     /** Stops (pauses) performance
//      */
//     stop() {
//         this.port.postMessage(["stop"]);
//     }
//
//     /** Sets a callback to process Csound console messages.
//      *
//      * @param {function} msgCallback A callback to process messages
//      * with signature function(message), where message is a string
//      * from Csound.
//      */
//     setMessageCallback(msgCallback) {
//         this.msgCallback = msgCallback;
//     }
//
//     /** Sends a MIDI channel message to Csound
//      *
//      * @param {number} byte1 MIDI status byte
//      * @param {number} byte2 MIDI data byte 1
//      * @param {number} byte1 MIDI data byte 2
//      *
//      */
//     midiMessage(byte1, byte2, byte3) {
//         this.port.postMessage(["midiMessage", byte1, byte2, byte3]);
//     }
//
//     /** Returns the current play state of Csound. Results are either
//      * "playing", "paused", or "stopped".
//      */
//     getPlayState() {
//        return this.playState;
//     }
//
//
//     /** Add a listener callback for play state listening. Must be a function
//      * of type (csoundObj:CsoundObj):void.
//      */
//     addPlayStateListener(listener) {
//         this.playStateListeners.add(listener);
//     }
//
//     /** Remove a listener callback for play state listening. Must be the same
//      * function as passed in with addPlayStateListener.
//      */
//     removePlayStateListener(listener) {
//         this.playStateListeners.delete(listener);
//     }
//
//     firePlayStateChange() {
//         // uses CsoundObj's wrapper function
//         this.playStateListeners.forEach(v => v());
//     }
// }


export default SingleThreadAudioWorkletMainThread;

