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
	var _compileCSD = cwrap('CsoundObj_compileCSD', null, ['number', 'string', 'number']);
	var _evaluateCode = cwrap('CsoundObj_evaluateCode', ['number'], ['number', 'string']);
	var _readScore = cwrap('CsoundObj_readScore', ['number'], ['number', 'string']);
	var _reset = cwrap('CsoundObj_reset', null, ['number']);
	var _getOutputBuffer = cwrap('CsoundObj_getOutputBuffer', ['number'], ['number']);
	var _getInputBuffer = cwrap('CsoundObj_getInputBuffer', ['number'], ['number']);
	var _getControlChannel = cwrap('CsoundObj_getControlChannel', ['number'], ['number', 'string']);
	var _setControlChannel = cwrap('CsoundObj_setControlChannel', null, ['number', 'string', 'number']);
	var _getKsmps = cwrap('CsoundObj_getKsmps', ['number'], ['number']);
	var _performKsmps = cwrap('CsoundObj_performKsmps', ['number'], ['number']);
	var _render = cwrap('CsoundObj_render', null, ['number']);
	var _getInputChannelCount = cwrap('CsoundObj_getInputChannelCount', ['number'], ['number']);
	var _getOutputChannelCount = cwrap('CsoundObj_getOutputChannelCount', ['number'], ['number']);
	var _getZerodBFS = cwrap('CsoundObj_getZerodBFS', ['number'], ['number']);
	var _setMidiCallbacks = cwrap('CsoundObj_setMidiCallbacks', null, ['number']);
	var _pushMidiMessage = cwrap('CsoundObj_pushMidiMessage', null, ['number', 'number', 'number', 'number']);
	var bufferSize = 256;
	var _self = _new();

	var getAudioContext = function() {

		try {
			window.AudioContext = window.AudioContext || window.webkitAudioContext;
			return new AudioContext();	
		}
		catch(error) {

			alert('Web Audio API is not supported in this browser');
		}
	};

	var microphoneNode = null;

	this.disableAudioInput = function (){

		microphoneNode = null;
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

		_compileCSD(_self, filePath, samplerate);
		compiled = true;
	};

	this.render = function(filePath) {

		that.reset();
		that.compileCSD(filePath);
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



	var getAudioProcessNode = function() {

		var inputChannelCount = _getInputChannelCount(_self);
		var outputChannelCount = _getOutputChannelCount(_self);
		var audioProcessNode = audioContext.createScriptProcessor(bufferSize, inputChannelCount, outputChannelCount);
		audioProcessNode.inputCount = inputChannelCount;
		audioProcessNode.outputCount = outputChannelCount;
		return audioProcessNode;
	};

	this.start = function() {

		var audioProcessNode = getAudioProcessNode();

		var ksmps = _getKsmps(_self);
		var inputChannelCount = audioProcessNode.inputCount;
		var outputChannelCount = audioProcessNode.outputCount;
		var outputPointer = _getOutputBuffer(_self);	
		var csoundOutputBuffer = new Float32Array(Module.HEAP8.buffer, outputPointer, ksmps * outputChannelCount);
		var contextOutputBuffer;

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
		var inputPointer = _getInputBuffer(_self);	
		var csoundInputBuffer = new Float32Array(Module.HEAP8.buffer, inputPointer, ksmps * inputChannelCount);
		var zerodBFS = _getZerodBFS(_self);
		audioProcessNode.onaudioprocess = function(e) {

			if (microphoneNode !== null) {

				for (var i = 0; i < inputChannelCount; ++i) {

					contextInputBuffer = e.inputBuffer.getChannelData(i);
					for (var j = 0; j < ksmps; ++j) {

						csoundInputBuffer[j + ksmps * i] = contextInputBuffer[j] * zerodBFS;
					}
				}
			}

			var result = _performKsmps(_self);

			if (result != 0) {

				compiled = false;
				that.stop();	
			}

			for (var i = 0; i < outputChannelCount; ++i) {

				contextOutputBuffer = e.outputBuffer.getChannelData(i);

				for (var j = 0; j < ksmps; ++j) {

					contextOutputBuffer[j] = csoundOutputBuffer[j * outputChannelCount + i] / zerodBFS;
				}	
			}

		};

		that.stop = function() {

			if (microphoneNode !== null) {

				microphoneNode.disconnect();
			}

			audioProcessNode.disconnect();
			audioProcessNode.onaudioprocess = null;
		};
	};

	this.reset = function() {

		_reset(_self);		
		compiled = false;
	};
};

