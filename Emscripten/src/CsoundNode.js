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

// Setup a single global AudioContext object if not already defined
var CSOUND_AUDIO_CONTEXT = CSOUND_AUDIO_CONTEXT || 
    (function() {

        try {
            var AudioContext = window.AudioContext || window.webkitAudioContext;
            return new AudioContext();      
        }
        catch(error) {

            console.log('Web Audio API is not supported in this browser');
        }
        return null;
    }());


/** This ES6 Class defines a Custom Node as an AudioWorkletNode 
 *  that holds a Csound engine.
 */
class CsoundNode extends AudioWorkletNode {

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
        
        super(context, 'Csound', options);

        this.msgCallback = (msg) => { console.log(msg); }

        this.port.start();
        this.channels =  {};
        this.channelCallbacks = {};
        this.stringChannels =  {};
        this.stringChannelCallbacks = {};
        this.table = {};
        this.tableCallbacks = {};
        this.port.onmessage = (event) => {
            let data = event.data;
            switch(data[0]) {
            case "log":
                this.msgCallback(data[1]);
                break;
            case "control":
                this.channels[data[1]] = data[2];
                if (typeof this.channelCallbacks[data[1]] != 'undefined')
                      this.channelCallbacks[data[1]](); 
                break;
            case "stringChannel":
                this.stringChannels[data[1]] = data[2];
                if (typeof this.stringChannelCallbacks[data[1]] != 'undefined')
                      this.stringChannelCallbacks[data[1]](); 
                break;
            case "table":
                this.table[data[1]] = data[2];
                if (typeof this.tableCallbacks[data[1]] != 'undefined')
                      this.tableCallbacks[data[1]](); 
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
        this.port.postMessage(["writeToFS", filePath, blobData]);
    }

    /** Compiles a CSD, which may be given as a filename in the
     *  WASM filesystem or a string containing the code
     *
     * @param {string} csd A string containing the CSD filename or the CSD code.
     */
    compileCSD(filePath) {
        this.port.postMessage(["compileCSD", filePath]);
    }

    /** Compiles Csound orchestra code.
     *
     * @param {string} orcString A string containing the orchestra code.
     */    
    compileOrc(orcString) {
        this.port.postMessage(["compileOrc", orcString]);
    }

    /** Sets a Csound engine option (flag)
     *  
     *
     * @param {string} option The Csound engine option to set. This should
     * not contain any whitespace.
     */
    setOption(option) {
        this.port.postMessage(["setOption", option]);    
    }

    render(filePath) {
    }

    /** Evaluates Csound orchestra code.
     *
     * @param {string} codeString A string containing the orchestra code.
     */   
    evaluateCode(codeString) {
        this.port.postMessage(["evalCode", codeString]);
    }

    /** Reads a numeric score string.
     *
     * @param {string} scoreString A string containing a numeric score.
     */    
    readScore(scoreString) {
        this.port.postMessage(["readScore", scoreString]);
    }

    /** Sets the value of a control channel in the software bus
     *
     * @param {string} channelName A string containing the channel name.
     * @param {number} value The value to be set.
     */ 
    setControlChannel(channelName, value) {
        this.port.postMessage(["setControlChannel",
                               channelName, value]);
    }

    /** Sets the value of a string channel in the software bus
     *
     * @param {string} channelName A string containing the channel name.
     * @param {string} stringValue The string to be set.
     */ 
    setStringChannel(channelName, value) {
        this.port.postMessage(["setStringChannel",
                               channelName, value]);
    }

    /** Request the data from a control channel 
     *
     * @param {string} channelName A string containing the channel name.
     * @param {function} callback An optional callback to be called when
     *  the requested data is available. This can be set once for all
     *  subsequent requests.
     */ 
    requestControlChannel(channelName, callback = null) {
        this.port.postMessage(["getControlChannel",
                               channelName]);
        if (callback !== null)
          this.channelCallbacks[channelName] = callback;
    }

    /** Request the data from a String channel 
     *
     * @param {string} channelName A string containing the channel name.
     * @param {function} callback An optional callback to be called when
     *  the requested data is available. This can be set once for all
     *  subsequent requests.
     */ 
    requestStringChannel(channelName, callback = null) {
        this.port.postMessage(["getStringChannel",
                               channelName]);
        if (callback !== null)
          this.stringChannelCallbacks[channelName] = callback;
    }

    /** Get the latest requested channel data 
     *
     * @param {string} channelName A string containing the channel name.
     * @returns {number} The latest channel value requested.
     */   
    getControlChannel(channelName) {
        return this.channels[channelName];
    }

    /** Get the latest requested string channel data 
     *
     * @param {string} channelName A string containing the channel name.
     * @returns {string} The latest channel value requested.
     */   
    getStringChannel(channelName) {
        return this.stringChannels[channelName];
    }

     /** Request the data from a Csound function table
     *
     * @param {number} number The function table number
     * @param {function} callback An optional callback to be called when
     *  the requested data is available. This can be set once for all
     *  subsequent requests.
     */ 
    requestTable(number, callback = null) {
        this.port.postMessage(["getTable", number]);
        if (callback !== null)
          this.tableCallbacks[number] = callback;
    }

    /** Get the requested table number
     *
     * @param {number} number The function table number
     * @returns {Float32Array} The table as a typed array.
     */   
    getTable(number) {
        return this.table[number];
    }

    /** Set a specific table position
     *
     * @param {number} number The function table number
     * @param {number} index The index of the position to be set
     * @param {number} value The value to set
     */ 
    setTableValue(number, index, value) {
        this.port.postMessage(["setTableAtIndex", number,
                              index, value]);
    }

    /** Set a table with data from an array
     *
     * @param {number} number The function table number
     * @param {Float32Array} table The source data for the table
     */   
    setTable(number, table) {
        this.port.postMessage(["setTable", number, table]);
    }
    
    /** Starts processing in this node
     */
    start() {
        this.port.postMessage(["start"]);
    }

    /** Resets the Csound engine.
     */
    reset() {
        this.port.postMessage(["reset"]);
    }

    destroy() {
    }

    /** Starts performance, same as start()
     */
    play() {
        this.port.postMessage(["play"]);
    }
    
    /** Stops (pauses) performance
     */
    stop() {
        this.port.postMessage(["stop"]);
    }

    /** Sets a callback to process Csound console messages.
     *
     * @param {function} msgCallback A callback to process messages 
     * with signature function(message), where message is a string
     * from Csound.
     */ 
    setMessageCallback(msgCallback) {
        this.msgCallback = msgCallback;
    }

    /** Sends a MIDI channel message to Csound
     *
     * @param {number} byte1 MIDI status byte
     * @param {number} byte2 MIDI data byte 1
     * @param {number} byte1 MIDI data byte 2
     *
     */
    midiMessage(byte1, byte2, byte3) {
        this.port.postMessage(["midiMessage", byte1, byte2, byte3]);
    }

}

/** This E6 class is used to setup scripts and
    allow the creation of new CsoundNode objects
   @hideconstructor
*/
class CsoundNodeFactory {

    /** 
     * This static method is used to asynchronously setup scripts for AudioWorklet Csound
     *
     * @param {string} script_base A string containing the base path to scripts
     */
    static importScripts(script_base='./') {
        let actx = CSOUND_AUDIO_CONTEXT;
        return new Promise( (resolve) => {
            actx.audioWorklet.addModule(script_base + 'CsoundProcessor.js').then(() => {
                resolve(); 
            })      
        }) 
    }

    /** 
     * This static method creates a new CsoundNode. 
     *  @param {number} InputChannelCount number of input channels
     *  @param {number} OutputChannelCount number of output channels
     *  @returns {object}
     */
    static createNode(inputChannelCount=1, outputChannelCount=2) {
        var options = {};
        options.numberOfInputs  = inputChannelCount;
        options.numberOfOutputs = 1;
        options.outputChannelCount = [ outputChannelCount ];
        return new CsoundNode(CSOUND_AUDIO_CONTEXT, options);
    }
}




