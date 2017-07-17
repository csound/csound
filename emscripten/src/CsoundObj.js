/*
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
    var _new = cwrap('CsoundObj_new', ['number'], null);
    var _compileCSD = cwrap('CsoundObj_compileCSD', null, ['number', 'string']);
    var _evaluateCode = cwrap('CsoundObj_evaluateCode', ['number'], ['number', 'string']);
    var _readScore = cwrap('CsoundObj_readScore', ['number'], ['number', 'string']);
    var _reset = cwrap('CsoundObj_reset', null, ['number']);
    var _getOutputBuffer = cwrap('CsoundObj_getOutputBuffer', ['number'], ['number']);
    var _getInputBuffer = cwrap('CsoundObj_getInputBuffer', ['number'], ['number']);
    var _getControlChannel = cwrap('CsoundObj_getControlChannel', ['number'], ['number', 'string']);
    var _setControlChannel = cwrap('CsoundObj_setControlChannel', null, ['number', 'string', 'number']);
    var _setStringChannel = cwrap('CsoundObj_setStringChannel', null, ['number', 'string', 'string']);
    var _getKsmps = cwrap('CsoundObj_getKsmps', ['number'], ['number']);
    var _performKsmps = cwrap('CsoundObj_performKsmps', ['number'], ['number']);
    var _render = cwrap('CsoundObj_render', null, ['number']);
    var _getInputChannelCount = cwrap('CsoundObj_getInputChannelCount', ['number'], ['number']);
    var _getOutputChannelCount = cwrap('CsoundObj_getOutputChannelCount', ['number'], ['number']);
    var _getTableLength = cwrap('CsoundObj_getTableLength', ['number'], ['number', 'number']);
    var _getTable = cwrap('CsoundObj_getTable', ['number'], ['number', 'number']);
    var _getZerodBFS = cwrap('CsoundObj_getZerodBFS', ['number'], ['number']);
    var _setMidiCallbacks = cwrap('CsoundObj_setMidiCallbacks', null, ['number']);
    var _pushMidiMessage = cwrap('CsoundObj_pushMidiMessage', null, ['number', 'number', 'number', 'number']);
    var _setOutputChannelCallback = cwrap('CsoundObj_setOutputChannelCallback', null, ['number', 'number']);
    var _compileOrc = cwrap('CsoundObj_compileOrc', 'number', ['number', 'string']);
    var _prepareRT = cwrap('CsoundObj_prepareRT', null, ['number']);
    var _getScoreTime = cwrap('CsoundObj_getScoreTime', null, ['number']);
    var _setTable = cwrap('CsoundObj_setTable', null, ['number', 'number', 'number', 'number']);
    var _openAudioOut = cwrap('CsoundObj_openAudioOut', null, ['number']);
    var _closeAudioOut = cwrap('CsoundObj_closeAudioOut', null, ['number']);
    var _play = cwrap('CsoundObj_play', null, ['number']);
    var _paude = cwrap('CsoundObj_pause', null, ['number']);
    var bufferSize;
    var _self = _new();
    var _destroy = cwrap('CsoundObj_destroy', null, ['number']);
    var running;

    var getAudioContext = function() {

	try {
	    window.AudioContext = window.AudioContext || window.webkitAudioContext;
	    return new AudioContext();	
	}
	catch(error) {

	    alert('Web Audio API is not supported in this browser');
	}
    };

    this.setMidiCallbacks = function() {
	_setMidiCallbacks(_self);
    }
    
    var microphoneNode = null;

    this.disableAudioInput = function (){
	microphoneNode = null;
    }

    this.midiMessage = function(byte1, byte2, byte3) {
        _pushMidiMessage(_self, byte1, byte2, byte3);
    }
    
    this.enableAudioInput = function(audioInputCallback) {

	window.navigator = window.navigator || {};
	navigator.getUserMedia = navigator.getUserMedia||navigator.webkitGetUserMedia ||navigator.mozGetUserMedia||null;

	if (navigator.getUserMedia === null) {

	    Module['print']("Audio Input not supported in this browser");
	    audioInputCallback(false);
	}	
	else{
	    function onSuccess(stream) {

		microphoneNode = audioContext.createMediaStreamSource(stream);	
		audioInputCallback(true);
	    };
	    function onFailure(error) {

		microphoneNode = null;	
		audioInputCallback(false);
		Module['print']("Could not initialise audio input, error:" +  error); 
	    };
	    navigator.getUserMedia({audio:true}, onSuccess, onFailure);
	}		

    };

    this.enableMidiInput = function(midiInputCallback) {

	var handleMidiInput = function(event) {

	    _pushMidiMessage(_self, event.data[0], event.data[1], event.data[2]);	
	};

	var midiSuccess = function(midiInterface) {

	    var inputs = midiInterface.inputs.values();

	    for (var input = inputs.next(); input && !input.done; input = inputs.next() ){

		input = input.value;
		input.onmidimessage = handleMidiInput;
	    }
	    _setMidiCallbacks(_self);
	    midiInputCallback(true);
	};

	var midiFail = function(error) {

	    Module['print']("MIDI failed to start, error:" + error);
	    midiInputCallback(false);
	};
	if (navigator.requestMIDIAccess) {

	    navigator.requestMIDIAccess().then(midiSuccess, midiFail);
	}
	else {

	    Module['print']("MIDI not supported in this browser");
	    midiInputCallback(false);
	}	
    };


    var audioContext = getAudioContext();
    var samplerate = audioContext.sampleRate;
    var compiled = false;

    this.compileCSD = function(filePath) {
        _prepareRT(_self);
	_compileCSD(_self, filePath);
	compiled = true;
    };

    this.compileOrc = function(orcString)
    {
	_prepareRT(_self);
	_compileOrc(_self, orcString);
	compiled = true;
	
    };

    this.render = function(filePath) {
	
	_compileCSD(_self, filePath);
	_render(_self);
	compiled = false;
    }

    this.evaluateCode = function(codeString) {

	_evaluateCode(_self, codeString);
    };

    this.readScore = function(scoreString) {

	_readScore(_self, scoreString);
    };

    this.setControlChannel = function(channelName, value) {

	_setControlChannel(_self, channelName, value);
    };

    this.getControlChannel = function(channelName) {

	return _getControlChannel(_self, channelName);
    };

    this.setStringChannel = function(channelName, value) {

	_setStringChannel(_self, channelName, value);
    };

    this.setOutputChannelCallback = function(callback) {
	
        function csoundCallback(csoundPtr, stringPtr, valuePtr, typePtr) {
	    
            var string = Pointer_stringify(stringPtr);
            var value = getValue(valuePtr, 'float');
            callback(string, value);
        };
        var functionPointer = Runtime.addFunction(csoundCallback);
        _setOutputChannelCallback(_self, functionPointer);
    };

    this.getTable = function(tableNumber) {

	var tableLength = _getTableLength(_self, tableNumber);
	var tablePointer = _getTable(_self, tableNumber);	
	var table = new Float32Array(Module.HEAP8.buffer, tablePointer, tableLength);
	return table;
    };

    this.setTable  = function(num, index, val) {
        _setTable(_self, num, index, val);
    }

    this.getScoreTime = function() {
        _getScoreTime(_self);
    }
    
    var getAudioProcessNode = function() {

	var inputChannelCount = _getInputChannelCount(_self);
	var outputChannelCount = _getOutputChannelCount(_self);
	var audioProcessNode = audioContext.createScriptProcessor(0, inputChannelCount, outputChannelCount);
	bufferSize = audioProcessNode.bufferSize;
	// console.error("bufferSize = " + bufferSize);
	audioProcessNode.inputCount = inputChannelCount;
	audioProcessNode.outputCount = outputChannelCount;
	return audioProcessNode;
    };

    
    this.start = function() {
	if(that.running == true)  return;
	var ksmps = _getKsmps(_self);
	// console.error("ksmps = " + ksmps);
	var audioProcessNode = getAudioProcessNode();
	var inputChannelCount = audioProcessNode.inputCount;
	var outputChannelCount = audioProcessNode.outputCount;
	var outputPointer = _getOutputBuffer(_self);	
	var csoundOutputBuffer = new Float32Array(Module.HEAP8.buffer, outputPointer, ksmps * outputChannelCount);
	var inputPointer = _getInputBuffer(_self);	
	var csoundInputBuffer = new Float32Array(Module.HEAP8.buffer, inputPointer, ksmps * inputChannelCount);
	var zerodBFS = _getZerodBFS(_self);
	var offset = ksmps;
	that.running = true;
	var processBuffers = function (e, sample_count, src_offset, dst_offset) {
	    var i, j;
	    var contextOutputBuffer;
	    var contextInputBuffer;
	    if (microphoneNode !== null) {
		for (i = 0; i < inputChannelCount; ++i) {
		    contextInputBuffer = e.inputBuffer.getChannelData(i);
		    for (j = 0; j < sample_count; ++j) {
			csoundInputBuffer[src_offset + j] = contextInputBuffer[(dst_offset + j) * inputChannelCount + i] * zerodBFS;
		    }
		}
	    }
	    for (i = 0; i < outputChannelCount; ++i) {
		contextOutputBuffer = e.outputBuffer.getChannelData(i);
		for (j = 0; j < sample_count; ++j) {
		    contextOutputBuffer[dst_offset + j] = csoundOutputBuffer[(src_offset + j) * outputChannelCount + i] / zerodBFS;
		}	
	    }
	};

	if (microphoneNode !== null) {

	    if (inputChannelCount >= microphoneNode.numberOfInputs) {

		microphoneNode.connect(audioProcessNode);
	    }
	    else {

		alert("Insufficient number of Csound inputs: nchnls_i, not starting");
		return;
	    }
	}

	audioProcessNode.connect(audioContext.destination);
	audioProcessNode.onaudioprocess = function(e) {
	    var idx = 0;
	    var sample_count;

	    sample_count = ksmps - offset;
	    //console.error("1: sample_count = " + sample_count);
	    if (sample_count > 0) {
		processBuffers(e, sample_count, offset, 0);
		idx += sample_count;
	    }
		while (idx < bufferSize) {		
		    var result = _performKsmps(_self);
		    if (result != 0) {
			compiled = false;
			that.stop();	
		    }
		    if (isNaN(csoundOutputBuffer[0])) {
			console.error("NaN! outputPointer = " + outputPointer);
		    }
		    sample_count = Math.min(ksmps, bufferSize - idx);
		    //console.error("2: sample_count = " + sample_count);
		    processBuffers(e, sample_count, 0, idx);
		    idx += sample_count;
		}
		offset = sample_count;
	    //console.error("3: offset = " + offset);
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
	_reset(_self);		
	compiled = false;
    };

    this.destroy = function() {
        _destroy(_self);
    };

    this.openAudioOut = function() {
        _openAudioOut(_self);
    }

    this.closeAudioOut = function() {
        _closeAudioOut(_self);
    }

    this.play = function() {
        _play(_self);
     }

     this.stop = function() {
        _openAudioOut(_self);
    }
};  

