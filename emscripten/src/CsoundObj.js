/**
 * C S O U N D
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 */
var CsoundObj = function() {
    var that = this;
    // All exports defined in CsoundObj.c, please keep in alphabetical order:
    var _closeAudioOut = cwrap('CsoundObj_closeAudioOut', null, ['number']);
    var _compileCSD = cwrap('CsoundObj_compileCSD', null, ['number', 'string']);
    var _compileOrc = cwrap('CsoundObj_compileOrc', 'number', ['number', 'string']);
    var _destroy = cwrap('CsoundObj_destroy', null, ['number']);
    var _evaluateCode = cwrap('CsoundObj_evaluateCode', 'number', ['number', 'string']);
    var _getControlChannel = cwrap('CsoundObj_getControlChannel', 'number', ['number', 'string']);
    var _getInputBuffer = cwrap('CsoundObj_getInputBuffer', 'number', ['number']);
    var _getInputChannelCount = cwrap('CsoundObj_getInputChannelCount', 'number', ['number']);
    var _getKsmps = cwrap('CsoundObj_getKsmps', 'number', ['number']);
    var _getOutputBuffer = cwrap('CsoundObj_getOutputBuffer', 'number', ['number']);
    var _getOutputChannelCount = cwrap('CsoundObj_getOutputChannelCount', 'number', ['number']);
    var _getTable = cwrap('CsoundObj_getTable', 'number', ['number', 'number']);
    var _getTableLength = cwrap('CsoundObj_getTableLength', 'number', ['number', 'number']);
    var _getZerodBFS = cwrap('CsoundObj_getZerodBFS', 'number', ['number']);
    var _new = cwrap('CsoundObj_new', 'number');
    var _openAudioOut = cwrap('CsoundObj_openAudioOut', null, ['number']);
    var _pause = cwrap('CsoundObj_pause', null, ['number']);
    var _performKsmps = cwrap('CsoundObj_performKsmps', 'number', ['number']);
    var _play = cwrap('CsoundObj_play', null, ['number']);
    var _prepareRT = cwrap('CsoundObj_prepareRT', null, ['number']);
    var _pushMidiMessage = cwrap('CsoundObj_pushMidiMessage', null, ['number', 'number', 'number', 'number']);
    var _readScore = cwrap('CsoundObj_readScore', 'number', ['number', 'string']);
    var _render = cwrap('CsoundObj_render', null, ['number']);
    var _reset = cwrap('CsoundObj_reset', null, ['number']);
    var _setControlChannel = cwrap('CsoundObj_setControlChannel', null, ['number', 'string', 'number']);
    var _setMidiCallbacks = cwrap('CsoundObj_setMidiCallbacks', null, ['number']);
    var _setOption = cwrap('CsoundObj_setOption', null, ['number', 'string']);
    var _setOutputChannelCallback = cwrap('CsoundObj_setOutputChannelCallback', null, ['number', 'number']);
    var _getScoreTime = cwrap('CsoundObj_getScoreTime', null, ['number']);
    var _setStringChannel = cwrap('CsoundObj_setStringChannel', null, ['number', 'string', 'string']);
    var _setTable = cwrap('CsoundObj_setTable', null, ['number', 'number', 'number', 'number']);
    // End of exports.
    var csound_obj_ = _new();
    var bufferFrameCount;
    var running;

    var getAudioContext = function() {
        try {
            window.AudioContext = window.AudioContext || window.webkitAudioContext;
            return new AudioContext();
        } catch (error) {
            alert('Web Audio API is not supported in this browser');
        }
    };

    this.setMidiCallbacks = function() {
        _setMidiCallbacks(csound_obj_);
    };

    var microphoneNode = null;

    this.disableAudioInput = function() {
        microphoneNode = null;
    };

    this.midiMessage = function(byte1, byte2, byte3) {
        _pushMidiMessage(csound_obj_, byte1, byte2, byte3);
    };

    this.enableAudioInput = function(audioInputCallback) {
        window.navigator = window.navigator || {};
        navigator.getUserMedia = navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia || null;
        if (navigator.getUserMedia === null) {
            Module['print']("Audio Input not supported in this browser");
            audioInputCallback(false);
        } else {
            function onSuccess(stream) {
                microphoneNode = audioContext.createMediaStreamSource(stream);
                audioInputCallback(true);
            };
            function onFailure(error) {
                microphoneNode = null;
                audioInputCallback(false);
                Module['print']("Could not initialise audio input, error:" + error);
            };
            navigator.getUserMedia({
                audio: true
            }, onSuccess, onFailure);
        }
    };

    this.enableMidiInput = function(midiInputCallback) {
        var handleMidiInput = function(event) {
            _pushMidiMessage(csound_obj_, event.data[0], event.data[1], event.data[2]);
        };
        var midiSuccess = function(midiInterface) {
            var inputs = midiInterface.inputs.values();
            for (var input = inputs.next(); input && !input.done; input = inputs.next()) {
                input = input.value;
                input.onmidimessage = handleMidiInput;
            }
            _setMidiCallbacks(csound_obj_);
            midiInputCallback(true);
        };
        var midiFail = function(error) {
            Module['print']("MIDI failed to start, error:" + error);
            midiInputCallback(false);
        };
        if (navigator.requestMIDIAccess) {
            navigator.requestMIDIAccess().then(midiSuccess, midiFail);
        } else {
            Module['print']("MIDI not supported in this browser");
            midiInputCallback(false);
        }
    };

    var audioContext = getAudioContext();
    var samplerate = audioContext.sampleRate;
    var compiled = false;

    this.compileCSD = function(filePath) {
        _prepareRT(csound_obj_);
        _compileCSD(csound_obj_, filePath);
        compiled = true;
    };

    this.compileOrc = function(orcString) {
        _prepareRT(csound_obj_);
        _compileOrc(csound_obj_, orcString);
        compiled = true;
    };

    this.setOption = function(option) {
        _setOption(csound_obj_, option);
    };

    this.render = function(filePath) {
        _compileCSD(csound_obj_, filePath);
        _render(csound_obj_);
        compiled = false;
    };

    this.evaluateCode = function(codeString) {
        _evaluateCode(csound_obj_, codeString);
    };

    this.readScore = function(scoreString) {
        _readScore(csound_obj_, scoreString);
    };

    this.setControlChannel = function(channelName, value) {
        _setControlChannel(csound_obj_, channelName, value);
    };

    this.getControlChannel = function(channelName) {
        return _getControlChannel(csound_obj_, channelName);
    };

    this.setStringChannel = function(channelName, value) {
        _setStringChannel(csound_obj_, channelName, value);
    };

    this.setOutputChannelCallback = function(callback) {
        function csoundCallback(csoundPtr, stringPtr, valuePtr, typePtr) {
            var string = Pointer_stringify(stringPtr);
            var value = getValue(valuePtr, 'float');
            callback(string, value);
        };
        var functionPointer = Runtime.addFunction(csoundCallback);
        _setOutputChannelCallback(csound_obj_, functionPointer);
    };

    this.getTable = function(tableNumber) {
        var tableLength = _getTableLength(csound_obj_, tableNumber);
        var tablePointer = _getTable(csound_obj_, tableNumber);
        var table = new Float32Array(Module.HEAP8.buffer, tablePointer, tableLength);
        return table;
    };

    this.setTable = function(num, index, val) {
        _setTable(csound_obj_, num, index, val);
    };

    this.getScoreTime = function() {
        _getScoreTime(csound_obj_);
    };

    var getAudioProcessNode = function() {
        var inputChannelCount = _getInputChannelCount(csound_obj_);
        var outputChannelCount = _getOutputChannelCount(csound_obj_);
        var audioProcessNode = audioContext.createScriptProcessor(0, inputChannelCount, outputChannelCount);
        bufferFrameCount = audioProcessNode.bufferSize;
        console.info("audioProcessNode.bufferSize (WebAudio frames per buffer): " +  bufferFrameCount);
        audioProcessNode.inputCount = inputChannelCount;
        audioProcessNode.outputCount = outputChannelCount;
        return audioProcessNode;
    };

    //~ this.start = function() {
        //~ if (that.running == true) {
            //~ return;
        //~ }
        //~ var ksmps = _getKsmps(csound_obj_);
        //~ // console.error("ksmps = " + ksmps);
        //~ var audioProcessNode = getAudioProcessNode();
        //~ var inputChannelCount = audioProcessNode.inputCount;
        //~ var outputChannelCount = audioProcessNode.outputCount;
        //~ var spout = _getOutputBuffer(csound_obj_);
        //~ var spoutBuffer = new Float32Array(Module.HEAP8.buffer, spout, ksmps * outputChannelCount);
        //~ var spin = _getInputBuffer(csound_obj_);
        //~ var spinBuffer = new Float32Array(Module.HEAP8.buffer, spin, ksmps * inputChannelCount);
        //~ var zerodBFS = _getZerodBFS(csound_obj_);
        //~ var offset = ksmps;
        //~ that.running = true;
        //~ var processBuffers = function(e, sample_count, src_offset, dst_offset) {
            //~ var i, j;
            //~ var contextOutputBuffer;
            //~ var contextInputBuffer;
            //~ if (microphoneNode !== null) {
                //~ for (i = 0; i < inputChannelCount; ++i) {
                    //~ contextInputBuffer = e.inputBuffer.getChannelData(i);
                    //~ for (j = 0; j < sample_count; ++j) {
                        //~ spinBuffer[src_offset + j] = contextInputBuffer[(dst_offset + j) * inputChannelCount + i] * zerodBFS;
                    //~ }
                //~ }
            //~ }
            //~ for (i = 0; i < outputChannelCount; ++i) {
                //~ contextOutputBuffer = e.outputBuffer.getChannelData(i);
                //~ for (j = 0; j < sample_count; ++j) {
                    //~ contextOutputBuffer[dst_offset + j] = spoutBuffer[(src_offset + j) * outputChannelCount + i] / zerodBFS;
                //~ }
            //~ }
        //~ };
        //~ if (microphoneNode !== null) {
            //~ if (inputChannelCount >= microphoneNode.numberOfInputs) {
                //~ microphoneNode.connect(audioProcessNode);
            //~ } else {
                //~ alert("Csound nchnls_i does not match microphoneNode.numberOfInputs.");
                //~ return;
            //~ }
        //~ }
        //~ audioProcessNode.connect(audioContext.destination);
        //~ audioProcessNode.onaudioprocess = function(e) {
            //~ var idx = 0;
            //~ var sample_count;
            //~ sample_count = ksmps - offset;
            //~ //console.error("1: sample_count = " + sample_count);
            //~ if (sample_count > 0) {
                //~ processBuffers(e, sample_count, offset, 0);
                //~ idx += sample_count;
            //~ }
            //~ while (idx < bufferFrameCount) {
                //~ var result = _performKsmps(csound_obj_);
                //~ if (result != 0) {
                    //~ compiled = false;
                    //~ that.stop();
                //~ }
                //~ if (isNaN(spoutBuffer[0])) {
                    //~ console.error("NaN! spout = " + spout);
                //~ }
                //~ sample_count = Math.min(ksmps, bufferFrameCount - idx);
                //~ //console.error("2: sample_count = " + sample_count);
                //~ processBuffers(e, sample_count, 0, idx);
                //~ idx += sample_count;
            //~ }
            //~ offset = sample_count;
            //~ //console.error("3: offset = " + offset);
        //~ };
        //~ that.stop = function() {
            //~ if (microphoneNode !== null) {
                //~ microphoneNode.disconnect();
            //~ }
            //~ audioProcessNode.disconnect();
            //~ audioProcessNode.onaudioprocess = null;
            //~ that.running = false;
        //~ };
    //~ };

    this.start = function() {
        if (that.running == true) {
            return;
        }
        var ksmps = _getKsmps(csound_obj_);
        var audioProcessNode = getAudioProcessNode();
        var inputChannelN = audioProcessNode.inputCount;
        var outputChannelN = audioProcessNode.outputCount;
        var spin = _getInputBuffer(csound_obj_);
        var spinBuffer = new Float32Array(Module.HEAP8.buffer, spin, ksmps * inputChannelN);
        var spout = _getOutputBuffer(csound_obj_);
        var spoutBuffer = new Float32Array(Module.HEAP8.buffer, spout, ksmps * outputChannelN);
        var zerodBFS = _getZerodBFS(csound_obj_);
        that.running = true;
        var csoundFrameI = 0;
        if (microphoneNode !== null) {
            if (inputChannelN >= microphoneNode.numberOfInputs) {
                microphoneNode.connect(audioProcessNode);
            } else {
                alert("Csound nchnls_i does not match microphoneNode.numberOfInputs.");
                return;
            }
        }
        audioProcessNode.connect(audioContext.destination);
        audioProcessNode.onaudioprocess = function(audioProcessEvent) {
            var inputBuffer = audioProcessEvent.inputBuffer;
            var outputBuffer = audioProcessEvent.outputBuffer;
            var hostFrameN = outputBuffer.length;
            var result = 0;
            for (var hostFrameI = 0; hostFrameI < hostFrameN; hostFrameI++) {
                for (var inputChannelI = 0; inputChannelI < inputChannelN; inputChannelI++) {
                    var inputChannelBuffer = inputBuffer.getChannelData(inputChannelI);
                    spinBuffer[(csoundFrameI * inputChannelN) + inputChannelI] = inputChannelBuffer[hostFrameI] * zerodBFS;
                }
                for (var outputChannelI = 0; outputChannelI < outputChannelN; outputChannelI++) {
                    var outputChannelBuffer = outputBuffer.getChannelData(outputChannelI);
                    outputChannelBuffer[hostFrameI] = spoutBuffer[(csoundFrameI * outputChannelN) + outputChannelI] / zerodBFS;
                    spoutBuffer[(csoundFrameI * outputChannelN) + outputChannelI] = 0.0;
                }
                csoundFrameI++
                if (csoundFrameI === ksmps) {
                    csoundFrameI = 0;
                    result = _performKsmps(csound_obj_);
                    if (result !== 0) {
                        compiled = false;
                        that.stop();
                    }
                }
            }
        };
        that.stop = function() {
            if (microphoneNode !== null) {
                microphoneNode.disconnect();
            }
            audioProcessNode.disconnect();
            audioProcessNode.onaudioprocess = null;
            that.running = false;
        };
    };

    this.reset = function() {
        _reset(csound_obj_);
        compiled = false;
    };

    this.destroy = function() {
        _destroy(csound_obj_);
    };

    this.openAudioOut = function() {
        _openAudioOut(csound_obj_);
    };

    this.closeAudioOut = function() {
        _closeAudioOut(csound_obj_);
    };

    this.play = function() {
        _play(csound_obj_);
    };

    /**
     * This is a stub to enable cross-platform compatibility of the core Csound API
     * across WASM, csound.node, CsoundQt, and Csound for Android.
     */
    this.perform = function() {
        console.log("CsoundObj.perform()...");
        return 0;
    };

    this.stop = function() {
        _openAudioOut(csound_obj_);
    };
};