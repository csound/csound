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

import libcsound from "./libcsound";

// Global singleton variables
var CSOUND;

// Single-global message callback, needs reworking...
let messageCallback = (msg) => console.log(msg);
const printMessages = (t) => {
    messageCallback(t);
};

const CSMOD = {}
//CSMOD["ENVIRONMENT"] = "WEB";
CSMOD["print"] = printMessages;
CSMOD["printErr"] = printMessages;
CSMOD["locateFile"] = (f) => script_base + f;

/** This E6 class is used to setup scripts and
    allow the creation of new CsoundScriptProcessorNode objects
    *  @hideconstructor
    */
class CsoundScriptProcessorNodeFactory {
    /**
     * This static method is used to asynchronously setup scripts for
     *  ScriptProcessorNode Csound
     *
     * @param {string} script_base A string containing the base path to scripts
     */
    static initialize(audioContext) {
        return new Promise((resolve) => {


            libcsound(CSMOD).then(() => {
                // Cache cwrap functions into CSOUND global object
                CSOUND = {
                    new: CSMOD.cwrap("CsoundObj_new", ["number"], null),
                    compileCSD: CSMOD.cwrap(
                        "CsoundObj_compileCSD",
                        ["number"],
                        ["number", "string"]
                    ),
                    evaluateCode: CSMOD.cwrap(
                        "CsoundObj_evaluateCode",
                        ["number"],
                        ["number", "string"]
                    ),
                    readScore: CSMOD.cwrap(
                        "CsoundObj_readScore",
                        ["number"],
                        ["number", "string"]
                    ),
                    reset: CSMOD.cwrap("CsoundObj_reset", null, ["number"]),
                    getOutputBuffer: CSMOD.cwrap(
                        "CsoundObj_getOutputBuffer",
                        ["number"],
                        ["number"]
                    ),
                    getInputBuffer: CSMOD.cwrap(
                        "CsoundObj_getInputBuffer",
                        ["number"],
                        ["number"]
                    ),
                    getControlChannel: CSMOD.cwrap(
                        "CsoundObj_getControlChannel",
                        ["number"],
                        ["number", "string"]
                    ),
                    setControlChannel: CSMOD.cwrap(
                        "CsoundObj_setControlChannel",
                        null,
                        ["number", "string", "number"]
                    ),
                    getStringChannel: CSMOD.cwrap(
                        "CsoundObj_getStringChannel",
                        ["string"],
                        ["number", "string"]
                    ),
                    setStringChannel: CSMOD.cwrap(
                        "CsoundObj_setStringChannel",
                        null,
                        ["number", "string", "string"]
                    ),
                    getKsmps: CSMOD.cwrap(
                        "CsoundObj_getKsmps",
                        ["number"],
                        ["number"]
                    ),
                    performKsmps: CSMOD.cwrap(
                        "CsoundObj_performKsmps",
                        ["number"],
                        ["number"]
                    ),
                    render: CSMOD.cwrap("CsoundObj_render", null, ["number"]),
                    getInputChannelCount: CSMOD.cwrap(
                        "CsoundObj_getInputChannelCount",
                        ["number"],
                        ["number"]
                    ),
                    getOutputChannelCount: CSMOD.cwrap(
                        "CsoundObj_getOutputChannelCount",
                        ["number"],
                        ["number"]
                    ),
                    getTableLength: CSMOD.cwrap(
                        "CsoundObj_getTableLength",
                        ["number"],
                        ["number", "number"]
                    ),
                    getTable: CSMOD.cwrap(
                        "CsoundObj_getTable",
                        ["number"],
                        ["number", "number"]
                    ),
                    getZerodBFS: CSMOD.cwrap(
                        "CsoundObj_getZerodBFS",
                        ["number"],
                        ["number"]
                    ),
                    setMidiCallbacks: CSMOD.cwrap(
                        "CsoundObj_setMidiCallbacks",
                        null,
                        ["number"]
                    ),
                    pushMidiMessage: CSMOD.cwrap(
                        "CsoundObj_pushMidiMessage",
                        null,
                        ["number", "number", "number", "number"]
                    ),
                    setOutputChannelCallback: CSMOD.cwrap(
                        "CsoundObj_setOutputChannelCallback",
                        null,
                        ["number", "number"]
                    ),
                    compileOrc: CSMOD.cwrap("CsoundObj_compileOrc", "number", [
                        "number",
                        "string",
                    ]),
                    setOption: CSMOD.cwrap("CsoundObj_setOption", null, [
                        "number",
                        "string",
                    ]),
                    prepareRT: CSMOD.cwrap("CsoundObj_prepareRT", null, [
                        "number",
                    ]),
                    getScoreTime: CSMOD.cwrap("CsoundObj_getScoreTime", null, [
                        "number",
                    ]),
                    setTable: CSMOD.cwrap("CsoundObj_setTable", null, [
                        "number",
                        "number",
                        "number",
                        "number",
                    ]),
                    destroy: CSMOD.cwrap("CsoundObj_destroy", null, ["number"]),
                };

                resolve();
            });
        });
    }

    /**
     * This static method creates a new CsoundScriptProcessorNode.
     *  @param {number} inputChannelCount Number of input channels
     *  @param {number} outputChannelCount Number of output channels
     *  @returns {object} A new CsoundScriptProcessorNode
     */
    static createNode(audioContext, inputChannelCount = 1, outputChannelCount = 2) {
        var options = {};
        options.numberOfInputs = inputChannelCount;
        options.numberOfOutputs = outputChannelCount;
        return new CsoundScriptProcessorNode(audioContext, options);
    }
}

/**  @classdesc A ScriptProcessorNode class containing a Csound engine
 *   @class CsoundScriptProcessorNode
 *   @mixes CsoundMixin
 *   @param {AudioContext} context AudioContext in which this node will run
 *   @param {object} options Configuration options, holding numberOfInputs,
 *   numberOfOutputs
 *   @returns {object} A new CsoundScriptProcessorNode
 */
const CsoundScriptProcessorNode = function (context, options) {
    var spn = context.createScriptProcessor(
        0,
        options.numberOfInputs,
        options.numberOfOutputs
    );
    spn.audioContext = context;
    spn.inputCount = options.numberOfInputs;
    spn.outputCount = options.numberOfOutputs;

    let cs = CSOUND.new();
    CSOUND.setMidiCallbacks(cs);
    CSOUND.setOption(cs, "-odac");
    CSOUND.setOption(cs, "-iadc");
    CSOUND.setOption(cs, "-M0");
    CSOUND.setOption(cs, "-+rtaudio=null");
    CSOUND.setOption(cs, "-+rtmidi=null");
    CSOUND.prepareRT(cs);
    var sampleRate = context.sampleRate;
    CSOUND.setOption(cs, "--sample-rate=" + sampleRate);
    CSOUND.setOption(cs, "--nchnls=" + this.nchnls);
    CSOUND.setOption(cs, "--nchnls_i=" + this.nchnls_i);

    /**
     *   @mixin CsoundMixin
     *   @ignore
     */
    let CsoundMixin = {
        csound: cs,
        compiled: false,
        csoundOutputBuffer: null,
        csoundInputBuffer: null,
        zerodBFS: 1.0,
        offset: 32,
        ksmps: 32,
        running: false,
        started: false,
        cnt: 0,
        result: 0,
        nchnls_i: options.numberOfInputs,
        nchnls: options.numberOfOutputs,
        channel: {},
        channelCallback: {},
        stringChannels: {},
        stringChannelCallbacks: {},
        table: {},
        tableCallback: {},
        playState: "stopped",
        playStateListeners: new Set(),

        /**
         *
         *  Writes data to a file in the WASM filesystem for
         *  use with csound.
         *
         * @param {string} filePath A string containing the path to write to.
         * @param {blob}   blobData The data to write to file.
         * @memberof CsoundMixin
         */

        writeToFS(filePath, blobData) {
            let FS = CSMOD["FS"];
            let stream = FS.open(filePath, "w+");
            let buf = new Uint8Array(blobData);
            FS.write(stream, buf, 0, buf.length, 0);
            FS.close(stream);
        },

        /**
         *
         * Unlink file from WASM filesystem (i.e. remove).
         *
         * @param {string} filePath A string containing the path to write to.
         * @memberof CsoundMixin
         */
        unlinkFromFS(filePath) {
            let FS = CSMOD["FS"];
            FS.unlink(filePath);
        },

        /** Compiles a CSD, which may be given as a filename in the
         *  WASM filesystem or a string containing the code
         *
         * @param {string} csd A string containing the CSD filename or the CSD code.
         * @memberof CsoundMixin
         */

        compileCSD(csd) {
            this.result = CSOUND.compileCSD(this.csound, csd);
        },

        /** Compiles Csound orchestra code.
         *
         * @param {string} orcString A string containing the orchestra code.
         * @memberof CsoundMixin
         */

        compileOrc(orcString) {
            CSOUND.compileOrc(this.csound, orcString);
        },

        /** Sets a Csound engine option (flag)
         *
         *
         * @param {string} option The Csound engine option to set. This should
         * not contain any whitespace.
         * @memberof CsoundMixin
         */

        setOption(option) {
            CSOUND.setOption(this.csound, option);
        },

        /** Renders a CSD, which may be given as a filename in the
         *  WASM filesystem or a string containing the code. This is used for
         *  disk rendering only.
         * @param {string} csd A string containing the CSD filename or the CSD code.
         * @memberof CsoundMixin
         */

        render(csd) {
            CSOUND.compileCSD(this.csound, csd);
            CSOUND.render(this.csound);
        },

        /** Evaluates Csound orchestra code.
         *
         * @param {string} codeString A string containing the orchestra code.
         * @memberof CsoundMixin
         */

        evaluateCode(codeString) {
            CSOUND.evaluateCode(this.csound, codeString);
        },
        /** Reads a numeric score string.
         *
         * @param {string} scoreString A string containing a numeric score.
         * @memberof CsoundMixin
         */

        readScore(scoreString) {
            CSOUND.readScore(this.csound, scoreString);
        },

        /** Sets the value of a control channel in the software bus
         *
         * @param {string} channelName A string containing the channel name.
         * @param {number} value The value to be set.
         * @memberof CsoundMixin
         */

        setControlChannel(channelName, value) {
            CSOUND.setControlChannel(this.csound, channelName, value);
        },

        /** Sets the value of a string channel in the software bus
         *
         * @param {string} channelName A string containing the channel name.
         * @param {string} stringValue The string to be set.
         * @memberof CsoundMixin
         */

        setStringChannel(channelName, value) {
            CSOUND.setStringChannel(this.csound, channelName, value);
        },

        /** Request the data from a control channel
         *
         * @param {string} channelName A string containing the channel name.
         * @param {function} callback An optional callback to be called when
         *  the requested data is available. This can be set once for all
         *  subsequent requests.
         */

        requestControlChannel(channelName, callback = null) {
            this.channel[channelName] = CSOUND.getControlChannel(
                this.csound,
                channelName
            );
            if (callback !== null) this.channelCallback[channelName] = callback;
            if (typeof this.channelCallback[channelName] !== "undefined")
                this.channelCallback[channelName]();
        },

        /** Request the data from a string channel
         *
         * @param {string} channelName A string containing the channel name.
         * @param {function} callback An optional callback to be called when
         *  the requested data is available. This can be set once for all
         *  subsequent requests.
         */

        requestStringChannel(channelName, callback = null) {
            let pointerStringify = CSMOD["Pointer_stringify"];
            let svalue = CSOUND.getStringChannel(this.csound, channelName);
            this.stringChannels[channelName] = pointerStringify(svalue);
            if (callback !== null)
                this.stringChannelCallbacks[channelName] = callback;
            if (typeof this.stringChannelCallbacks[channelName] !== "undefined")
                this.stringChannelCallbacks[channelName]();
        },

        /** Get the latest requested channel data
         *
         * @param {string} channelName A string containing the channel name.
         * @returns {(number|string)} The latest channel value requested.
         */

        getControlChannel(channelName) {
            return this.channel[channelName];
        },

        /** Get the latest requested string channel data
         *
         * @param {string} channelName A string containing the channel name.
         * @returns {string} The latest channel value requested.
         */

        getStringChannel(channelName) {
            return this.stringChannels[channelName];
        },

        /** Request the data from a Csound function table
         *
         * @param {number} number The function table number
         * @param {function} callback An optional callback to be called when
         *  the requested data is available. This can be set once for all
         *  subsequent requests.
         */

        requestTable(number, callback = null) {
            let buffer = CSOUND.getTable(this.csound, number);
            let len = CSOUND.getTableLength(this.csound, number);
            let src = new Float32Array(CSMOD.HEAP8.buffer, buffer, len);
            this.table[number] = new Float32Array(src);
            if (callback !== null) this.tableCallback[number] = callback;
            if (typeof this.tableCallback[number] != "undefined")
                this.tableCallback[number]();
        },

        /** Get the requested table number
         *
         * @param {number} number The function table number
         * @returns {Float32Array} The table as a typed array.
         */

        getTable(number) {
            return this.table[number];
        },

        /** Set a specific table position
         *
         * @param {number} number The function table number
         * @param {number} index The index of the position to be set
         * @param {number} value The value to set
         */

        setTableValue(number, index, value) {
            CSOUND.setTable(this.csound, number, index, value);
        },

        /** Set a table with data from an array
         *
         * @param {number} number The function table number
         * @param {Float32Array} table The source data for the table
         */

        setTable(number, table) {
            for (let i = 0; i < table.length; i++)
                CSOUND.setTable(this.csound, number, i, table[i]);
        },

        /** Starts processing in this node
         *  @memberof CsoundMixin
         */

        start() {
            if (this.started == false) {
                let ksmps = CSOUND.getKsmps(this.csound);
                this.ksmps = ksmps;
                this.cnt = ksmps;

                let outputPointer = CSOUND.getOutputBuffer(this.csound);
                this.csoundOutputBuffer = new Float32Array(
                    CSMOD.HEAP8.buffer,
                    outputPointer,
                    ksmps * this.nchnls
                );
                let inputPointer = CSOUND.getInputBuffer(this.csound);
                this.csoundInputBuffer = new Float32Array(
                    CSMOD.HEAP8.buffer,
                    inputPointer,
                    ksmps * this.nchnls_i
                );
                this.zerodBFS = CSOUND.getZerodBFS(this.csound);
                this.started = true;
            }
            this.running = true;
            this.firePlayStateChange();
        },

        /** Resets the Csound engine.
         *  @memberof CsoundMixin
         */

        reset() {
            this.started = false;
            this.running = false;
            CSOUND.reset(this.csound);
            CSOUND.setMidiCallbacks(this.csound);
            CSOUND.setOption(this.csound, "-odac");
            CSOUND.setOption(this.csound, "-iadc");
            CSOUND.setOption(this.csound, "-M0");
            CSOUND.setOption(this.csound, "-+rtaudio=null");
            CSOUND.setOption(this.csound, "-+rtmidi=null");
            var sampleRate = this.audioContext.sampleRate;
            CSOUND.setOption(this.csound, "--sample-rate=" + sampleRate);
            CSOUND.prepareRT(this.csound);
            CSOUND.setOption(this.csound, "--nchnls=" + this.nchnls);
            CSOUND.setOption(this.csound, "--nchnls_i=" + this.nchnls_i);
            this.csoundOutputBuffer = null;
            this.ksmps = null;
            this.zerodBFS = null;

            this.firePlayStateChange();
        },

        destroy() {
            CSOUND.destroy(this.csound);
        },

        /** Starts performance, same as start()
         * @memberof CsoundMixin
         */

        play() {
            this.start(this.csound);
        },

        /** Stops (pauses) performance
         *   @memberof CsoundMixin
         */

        stop() {
            this.running = false;
            this.firePlayStateChange();
        },

        /** Sets a callback to process Csound console messages.
         *
         * @param {function} msgCallback A callback to process messages
         * with signature function(message), where message is a string
         * from Csound.
         * @memberof CsoundMixin
         */

        setMessageCallback(msgCallback) {
            messageCallback = msgCallback;
        },

        /** Sends a MIDI channel message to Csound
         *
         * @param {number} byte1 MIDI status byte
         * @param {number} byte2 MIDI data byte 1
         * @param {number} byte1 MIDI data byte 2
         *
         * @memberof CsoundMixin
         */

        midiMessage(byte1, byte2, byte3) {
            CSOUND.pushMidiMessage(this.csound, byte1, byte2, byte3);
        },

        /** Returns the current play state of Csound. Results are either
         * "playing", "paused", or "stopped".
         */

        getPlayState() {
            if (this.running) {
                return "playing";
            } else if (this.started) {
                return "paused";
            }
            return "stopped";
        },

        /** Add a listener callback for play state listening. Must be a function
         * of type (csoundObj:CsoundObj):void.
         */

        addPlayStateListener(listener) {
            this.playStateListeners.add(listener);
        },

        /** Remove a listener callback for play state listening. Must be the same
         * function as passed in with addPlayStateListener.
         */

        removePlayStateListener(listener) {
            this.playStateListeners.delete(listener);
        },

        firePlayStateChange() {
            // uses CsoundObj's wrapper function
            this.playStateListeners.forEach((v) => v());
        },

        onaudioprocess(e) {
            if (this.csoundOutputBuffer == null || this.running == false) {
                let output = e.outputBuffer;
                let bufferLen = output.getChannelData(0).length;
                for (let i = 0; i < bufferLen; i++) {
                    for (
                        let channel = 0;
                        channel < output.numberOfChannels;
                        channel++
                    ) {
                        let outputChannel = output.getChannelData(channel);
                        outputChannel[i] = 0;
                    }
                }
                return;
            }

            let input = e.inputBuffer;
            let output = e.outputBuffer;

            let bufferLen = output.getChannelData(0).length;

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
                    result = CSOUND.performKsmps(this.csound);
                    cnt = 0;
                    if (result != 0) {
                        this.running = false;
                        this.started = false;
                        this.firePlayStateChange();
                    }
                }

                for (
                    let channel = 0;
                    channel < input.numberOfChannels;
                    channel++
                ) {
                    let inputChannel = input.getChannelData(channel);
                    csIn[cnt * nchnls_i + channel] = inputChannel[i] * zerodBFS;
                }
                for (
                    let channel = 0;
                    channel < output.numberOfChannels;
                    channel++
                ) {
                    let outputChannel = output.getChannelData(channel);
                    if (result == 0)
                        outputChannel[i] =
                            csOut[cnt * nchnls + channel] / zerodBFS;
                    else outputChannel[i] = 0;
                }
            }

            this.cnt = cnt;
            this.result = result;
        },
    };

    return Object.assign(spn, CsoundMixin);
};

export default CsoundScriptProcessorNodeFactory;
