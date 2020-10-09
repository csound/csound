/*
    CsoundObj.js

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

import CsoundScriptProcessorNodeFactory from "./CsoundScriptProcessorNode";
import CsoundNodeFactory from "./CsoundNode";

/** This ES6 Class provides an interface to the Csound
 * engine running on a node (an AudioWorkletNode where available,
 * ScriptProcessorNode elsewhere)
 * This class is designed to be compatible with
 * the previous ScriptProcessorNode-based CsoundObj
 */
class CsoundObj {
    /** Create a CsoundObj
     * @constructor
     */
    constructor() {
        this.audioContext = CsoundObj.CSOUND_AUDIO_CONTEXT;

        // exposes node as property, user may access to set port onMessage callback
        // or we can add a setOnMessage(cb) method on CsoundObj...
        this.node = CsoundObj.createNode();
        this.node.connect(this.audioContext.destination);
        this.microphoneNode = null;
    }

    /** Returns the underlying Csound node
        running the Csound engine.
    */
    getNode() {
        return this.node;
    }

    /** Writes data to a file in the WASM filesystem for
     *  use with csound.
     *
     * @param {string} filePath A string containing the path to write to.
     * @param {blob}   blobData The data to write to file.
     */
    writeToFS(filePath, blobData) {
        this.node.writeToFS(filePath, blobData);
    }

    /** Unlink file from WASM filesystem (i.e. remove).
     *
     * @param {string} filePath A string containing the path to unlink.
     */
    unlinkFromFS(filePath) {
        this.node.unlinkFromFS(filePath);
    }

    /** Compiles a CSD, which may be given as a filename in the
     *  WASM filesystem or a string containing the code
     *
     * @param {string} csd A string containing the CSD filename or the CSD code.
     */
    compileCSD(csd) {
        this.node.compileCSD(csd);
    }

    /** Compiles Csound orchestra code.
     *
     * @param {string} orcString A string containing the orchestra code.
     */
    compileOrc(orcString) {
        this.node.compileOrc(orcString);
    }

    /** Sets a Csound engine option (flag)
     *
     *
     * @param {string} option The Csound engine option to set. This should
     * not contain any whitespace.
     */
    setOption(option) {
        this.node.setOption(option);
    }

    render(filePath) {}

    /** Evaluates Csound orchestra code.
     *
     * @param {string} codeString A string containing the orchestra code.
     */

    evaluateCode(codeString) {
        this.node.evaluateCode(codeString);
    }

    /** Reads a numeric score string.
     *
     * @param {string} scoreString A string containing a numeric score.
     */

    readScore(scoreString) {
        this.node.readScore(scoreString);
    }

    /** Sets the value of a control channel in the software bus
     *
     * @param {string} channelName A string containing the channel name.
     * @param {number} value The value to be set.
     */

    setControlChannel(channelName, value) {
        this.node.setControlChannel(channelName, value);
    }

    /** Sets the value of a string channel in the software bus
     *
     * @param {string} channelName A string containing the channel name.
     * @param {string} stringValue The string to be set.
     */

    setStringChannel(channelName, stringValue) {
        this.node.setStringChannel(channelName, stringValue);
    }

    /** Request the data from a control channel
     *
     * @param {string} channelName A string containing the channel name.
     * @param {function} callback An optional callback to be called when
     *  the requested data is available. This can be set once for all
     *  subsequent requests.
     */

    requestControlChannel(channelName, callback = null) {
        this.node.requestControlChannel(channelName, callback);
    }

    /** Request the string data from a control channel
     *
     * @param {string} channelName A string containing the channel name.
     * @param {function} callback An optional callback to be called when
     *  the requested data is available. This can be set once for all
     *  subsequent requests.
     */

    requestStringChannel(channelName, callback = null) {
        this.node.requestStringChannel(channelName, callback);
    }

    /** Get the latest requested control channel data
     *
     * @param {string} channelName A string containing the channel name.
     * @returns {(number)} The latest channel value requested.
     */

    getControlChannel(channelName) {
        return this.node.getControlChannel(channelName);
    }

    /** Get the latest requested string channel data
     *
     * @param {string} channelName A string containing the channel name.
     * @returns {(number|string)} The latest channel value requested.
     */

    getStringChannel(channelName) {
        return this.node.getStringChannel(channelName);
    }

    /** Request the data from a Csound function table
     *
     * @param {number} number The function table number
     * @param {function} callback An optional callback to be called when
     *  the requested data is available. This can be set once for all
     *  subsequent requests.
     */

    requestTable(number, callback = null) {
        this.node.requestTable(number, callback);
    }

    /** Get the requested table number
     *
     * @param {number} number The function table number
     * @returns {Float32Array} The table as a typed array.
     */

    getTable(number) {
        return this.node.getTable(number);
    }

    /** Set a specific table position
     *
     * @param {number} number The function table number
     * @param {number} index The index of the position to be set
     * @param {number} value The value to set
     */

    setTableValue(number, index, value) {
        this.node.setTableValue(number, index, value);
    }

    /** Set a table with data from an array
     *
     * @param {number} number The function table number
     * @param {Float32Array} table The source data for the table
     */

    setTable(number, table) {
        this.node.setTable(number, table);
    }

    /** Starts the node containing the Csound engine.
     */
    start() {
        if (this.microphoneNode != null) {
            this.microphoneNode.connect(this.node);
        }
        this.node.start();
    }

    /** Resets the Csound engine.
     */
    reset() {
        this.node.reset();
    }

    destroy() {}

    /** Starts performance, same as start()
     */
    play() {
        this.node.play();
    }

    /** Stops (pauses) performance
     */
    stop() {
        this.node.stop();
    }

    /** Sets a callback to process Csound console messages.
     *
     * @param {function} msgCallback A callback to process messages
     * with signature function(message), where message is a string
     * from Csound.
     */

    setMessageCallback(msgCallback) {
        this.node.setMessageCallback(msgCallback);
    }

    /** Sends a MIDI channel message to Csound
     *
     * @param {number} byte1 MIDI status byte
     * @param {number} byte2 MIDI data byte 1
     * @param {number} byte1 MIDI data byte 2
     *
     */
    midiMessage(byte1, byte2, byte3) {
        this.node.midiMessage(byte1, byte2, byte3);
    }

    /** Enables microphone (external audio) input in browser
     *
     * @param {function} audioInputCallback A callback with a signature
     * function(result), with result set to true in the event of success
     * or false if the microphone cannot be enabled
     */

    enableAudioInput(audioInputCallback) {
        const getUserMedia =
            typeof navigator.mediaDevices !== "undefined"
                ? navigator.mediaDevices.getUserMedia
                : navigator.getUserMedia ||
                  navigator.webkitGetUserMedia ||
                  navigator.mozGetUserMedia;

        if (getUserMedia === null) {
            console.log("Audio Input not supported in this browser");
            audioInputCallback(false);
        } else {
            let onSuccess = (stream) => {
                this.microphoneNode = this.audioContext.createMediaStreamSource(
                    stream
                );
                audioInputCallback(true);
            };

            let onFailure = (error) => {
                this.microphoneNode = null;
                console.log("Could not initialise audio input, error:" + error);
                audioInputCallback(false);
            };


            typeof navigator.mediaDevices !== "undefined"
                ? getUserMedia
                      .call(navigator.mediaDevices, {
                          audio: { echoCancellation: false, sampleSize: 32 }
                      })
                      .then(onSuccess)
                      .catch(onFailure)
                : getUserMedia.call(
                      navigator,
                      {
                          audio: {
                              optional: [
                                  { echoCancellation: false, sampleSize: 32 }
                              ]
                          }
                      },
                      onSuccess,
                      onFailure
                  );
        }
    }

    enableMidiInput(midiInputCallback) {
        const handleMidiInput = (evt) => {
            this.midiMessage(evt.data[0], evt.data[1], evt.data[2]);
        };
        const midiSuccess = function (midiInterface) {
            const inputs = midiInterface.inputs.values();

            for (
                let input = inputs.next();
                input && !input.done;
                input = inputs.next()
            ) {
                input = input.value;
                input.onmidimessage = handleMidiInput;
            }
            if (midiInputCallback) {
                midiInputCallback(true);
            }
        };

        const midiFail = function (error) {
            console.log("MIDI failed to start, error:" + error);
            if (midiInputCallback) {
                midiInputCallback(false);
            }
        };

        if (navigator.requestMIDIAccess) {
            navigator.requestMIDIAccess().then(midiSuccess, midiFail);
        } else {
            console.log("MIDI not supported in this browser");
            if (midiInputCallback) {
                midiInputCallback(false);
            }
        }
    }

    /** Returns the current play state of Csound. Results are either
     * "playing", "paused", or "stopped".
     */

    getPlayState() {
        return this.node.getPlayState();
    }

    /** Add a listener callback for play state listening. Must be a function
     * of type (csoundObj:CsoundObj):void.
     */

    addPlayStateListener(listener) {
        // CsoundObj will wrap the listener so that it will use itself as the
        // as the listener's argument
        this.node.addPlayStateListener((cs) => listener(this));
    }

    /** Remove a listener callback for play state listening. Must be the same
     * function as passed in with addPlayStateListener.
     */

    removePlayStateListener(listener) {
        this.node.removePlayStateListener(listener);
    }

    /**
     * This static method is used to asynchronously setup the Csound system.
     *
     * @param {AudioContext} audioContext Optional AudioContext object to use for node creation, 
     * if none passed in, CsoundObj will default to creating one 
     */
    static initialize(audioContext = null) {
        const createAudioContext = () => {
            try {
                var AudioContext =
                    window.AudioContext || window.webkitAudioContext;
                return new AudioContext();
            } catch (error) {
                console.log("Web Audio API is not supported in this browser");
            }
            return null;
        };
        const ctx = audioContext || createAudioContext();

        CsoundObj.CSOUND_AUDIO_CONTEXT = ctx;

        return new Promise((resolve) => {
            let factory;
            if (
                typeof AudioWorkletNode !== "undefined" &&
                ctx.audioWorklet !== undefined
            ) {
                console.log("Using WASM + AudioWorklet Csound implementation");
                factory = CsoundNodeFactory;
            } else {
                console.log(
                    "Using WASM + ScriptProcessorNode Csound implementation"
                );
                factory = CsoundScriptProcessorNodeFactory;
            }

            CsoundObj.factory = factory;

            factory.initialize(ctx).then(() => {
                resolve();
            });
        });
    }

    /**
     * This static method creates a new Csound Engine node unattached
     * to a CsoundObj object. It can be used in scenarios where
     * CsoundObj is not needed (ie. WebAudio API programming)
     *
     *  @param {number} InputChannelCount number of input channels
     *  @param {number} OutputChannelCount number of output channels
     *  @return A new Csound Engine Node (CsoundNode or CsoundScriptProcessorNode)
     */
    static createNode(
        inputChannelCount = 1,
        outputChannelCount = 2
    ) {
        return CsoundObj.factory.createNode(
            CsoundObj.CSOUND_AUDIO_CONTEXT,
            inputChannelCount,
            outputChannelCount
        );
    }
}

export default CsoundObj;
