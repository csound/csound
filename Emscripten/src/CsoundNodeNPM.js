/*
    CsoundNode.js

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

/** This ES6 Class defines a Custom Node as an AudioWorkletNode
 *  that holds a Csound engine.
 */

import libcsound from "./libcsound-combined.js.txt";

export class CsoundNode {
    /**
     *
     * @constructor
     * @param {AudioContext} context AudioContext in which this node will run
     * @param {object} options Configuration options, holding numberOfInputs,
     *   numberOfOutputs
     * @returns {object} A new CsoundNode
     */
    constructor(context, options) {
        options = options || {};
        options.numberOfInputs = 1;
        options.numberOfOutputs = 2;
        options.channelCount = 2;

        this.awn = new window.AudioWorkletNode(context, "Csound", options);

        this.awn.msgCallback = msg => {
            console.log(msg);
        };

        this.awn.port.start();
        this.awn.channel = {};
        this.awn.channelCallback = {};
        this.awn.table = {};
        this.awn.tableCallback = {};
        this.awn.port.onmessage = event => {
            let data = event.data;
            switch (data[0]) {
                case "log":
                    this.awn.msgCallback(data[1]);
                    break;
                case "control":
                    this.awn.channel[data[1]] = data[2];
                    if (typeof this.awn.channelCallback[data[1]] != "undefined")
                        this.awn.channelCallback[data[1]]();
                    break;
                case "table":
                    this.awn.table[data[1]] = data[2];
                    if (typeof this.awn.tableCallback[data[1]] != "undefined")
                        this.awn.tableCallback[data[1]]();
                    break;
                default:
                    console.log('[CsoundNode] Invalid Message: "' + event.data);
            }
        };
    }

    /** Writes data to a file in the WASM filesystem for
     *  use with csound.
     *
     * @param {string} filePath A string containing the path to write to.
     * @param {blob}   blobData The data to write to file.
     */

    writeToFS(filePath, blobData) {
        this.awn.port.postMessage(["writeToFS", filePath, blobData]);
    }

    /** Compiles a CSD, which may be given as a filename in the
     *  WASM filesystem or a string containing the code
     *
     * @param {string} csd A string containing the CSD filename or the CSD code.
     */
    compileCSD(filePath) {
        this.awn.port.postMessage(["compileCSD", filePath]);
    }

    /** Compiles Csound orchestra code.
     *
     * @param {string} orcString A string containing the orchestra code.
     */

    compileOrc(orcString) {
        this.awn.port.postMessage(["compileOrc", orcString]);
    }

    /** Sets a Csound engine option (flag)
     *
     *
     * @param {string} option The Csound engine option to set. This should
     * not contain any whitespace.
     */
    setOption(option) {
        this.awn.port.postMessage(["setOption", option]);
    }

    render(filePath) {}

    /** Evaluates Csound orchestra code.
     *
     * @param {string} codeString A string containing the orchestra code.
     */

    evaluateCode(codeString) {
        this.awn.port.postMessage(["evalCode", codeString]);
    }

    /** Reads a numeric score string.
     *
     * @param {string} scoreString A string containing a numeric score.
     */

    readScore(scoreString) {
        this.awn.port.postMessage(["readScore", scoreString]);
    }

    /** Sets the value of a control channel in the software bus
     *
     * @param {string} channelName A string containing the channel name.
     * @param {number} value The value to be set.
     */

    setControlChannel(channelName, value) {
        this.awn.port.postMessage(["setControlChannel", channelName, value]);
    }

    /** Sets the value of a string channel in the software bus
     *
     * @param {string} channelName A string containing the channel name.
     * @param {string} stringValue The string to be set.
     */

    setStringChannel(channelName, value) {
        this.awn.port.postMessage(["setStringChannel", channelName, value]);
    }

    /** Request the data from a control channel
     *
     * @param {string} channelName A string containing the channel name.
     * @param {function} callback An optional callback to be called when
     *  the requested data is available. This can be set once for all
     *  subsequent requests.
     */

    requestControlChannel(channelName, callback = null) {
        this.awn.port.postMessage(["getControlChannel", channelName]);
        if (callback !== null) this.awn.channelCallback[channelName] = callback;
    }

    /** Get the latest requested channel data
     *
     * @param {string} channelName A string containing the channel name.
     * @returns {(number|string)} The latest channel value requested.
     */

    getChannel(channelName) {
        return this.awn.channel[channelName];
    }

    /** Request the data from a Csound function table
     *
     * @param {number} number The function table number
     * @param {function} callback An optional callback to be called when
     *  the requested data is available. This can be set once for all
     *  subsequent requests.
     */

    requestTable(number, callback = null) {
        this.awn.port.postMessage(["getTable", number]);
        if (callback !== null) this.awn.tableCallback[number] = callback;
    }

    /** Get the requested table number
     *
     * @param {number} number The function table number
     * @returns {Float32Array} The table as a typed array.
     */

    getTable(number) {
        return this.awn.table[number];
    }

    /** Set a specific table position
     *
     * @param {number} number The function table number
     * @param {number} index The index of the position to be set
     * @param {number} value The value to set
     */

    setTableValue(number, index, value) {
        this.awn.port.postMessage(["setTableAtIndex", number, index, value]);
    }

    /** Set a table with data from an array
     *
     * @param {number} number The function table number
     * @param {Float32Array} table The source data for the table
     */

    setTable(number, table) {
        this.awn.port.postMessage(["setTable", number, table]);
    }

    /** Starts processing in this node
     */
    start() {
        this.awn.port.postMessage(["start"]);
    }

    /** Resets the Csound engine.
     */
    reset() {
        this.awn.port.postMessage(["reset"]);
    }

    destroy() {}

    /** Starts performance, same as start()
     */
    play() {
        this.awn.port.postMessage(["play"]);
    }

    /** Stops (pauses) performance
     */
    stop() {
        this.awn.port.postMessage(["stop"]);
    }

    /** Sets a callback to process Csound console messages.
     *
     * @param {function} msgCallback A callback to process messages
     * with signature function(message), where message is a string
     * from Csound.
     */

    setMessageCallback(msgCallback) {
        this.awn.msgCallback = msgCallback;
    }

    /** Sends a MIDI channel message to Csound
     *
     * @param {number} byte1 MIDI status byte
     * @param {number} byte2 MIDI data byte 1
     * @param {number} byte1 MIDI data byte 2
     *
     */
    midiMessage(byte1, byte2, byte3) {
        this.awn.port.postMessage(["midiMessage", byte1, byte2, byte3]);
    }
}

/** This E6 class is used to setup scripts and
    allow the creation of new CsoundNode objects
   @hideconstructor
*/
export class CsoundNodeFactory {
    /**
     * This static method is used to asynchronously setup scripts for AudioWorklet Csound
     *
     * @param {string} script_base A string containing the base path to scripts
     */
    static importScripts(audioContext, script_base = "./") {
        const blob = new Blob([libcsound], { type: "application/javascript" });
        const blobURL = URL.createObjectURL(blob);

        return new Promise(resolve => {
            audioContext.audioWorklet
                .addModule(blobURL)
                .then(() => {
                    resolve();
                })
                .catch(e => {
                    console.log(e);
                });
        });
    }
}
