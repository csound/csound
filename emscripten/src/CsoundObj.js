/*
 
 CsoundObj.js:

 Copyright (C) 2013 Edward Costello 
 This file is part of Csound Emscripten 

 The Csound Emscripten is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 Csound is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with Csound; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA

*/


CsoundObj = function()
{
	var that = this;

	//Wrap C functions
	var _new = cwrap('CsoundObj_new', 'number', null);
	var _compileCSD = cwrap('CsoundObj_compileCSD', null, ['number','string', 'number', 'number', 'number']);
	var _process = cwrap('CsoundObj_process', 'number', ['number', 'number', 'number', 'number']);
	var _getKsmps = cwrap('CsoundObj_getKsmps', 'number', 'number');
	var _getNchnls = cwrap('CsoundObj_getNchnls', 'number', 'number');
	var _getNchnlsInput = cwrap('CsoundObj_getNchnlsInput', 'number', 'number');
	var _setUsingAudioInput = cwrap('CsoundObj_setUsingAudioInput', null, ['number', 'number']);
	var _stop = cwrap('CsoundObj_stop', null, 'number');
	var _reset = cwrap('CsoundObj_reset', null, 'number');
	var _setControlChannel = cwrap('CsoundObj_setControlChannel', null, ['number', 'string', 'number']);
	var _compileOrc = cwrap('CsoundObj_compileOrc', 'number', ['number', 'string']);
	var _readScore = Module.cwrap('CsoundObj_readScore', 'number', ['number', 'string']);
	//Create instance of CsoundObj C structure
	var _self = _new();
	var csoundControlChannels = new Array();
	this.addControlChannel = function(channelName, initialValue)
	{
		var channel = {name:channelName, value:initialValue, updated:true};
		csoundControlChannels.push(channel);
		return channel;
	}

	this.setControlChannelValue = function(controlChannel, value)
	{
		controlChannel.value = value;
		controlChannel.updated = true;
	}

	this.getControlChannelValue = function(controlChannel)
	{
		controlChannel.updated = false;
		return controlChannel.value;
	}

	//Memory for callback references
	var csoundOutputBufferPointer;
	var csoundInputBufferPointer;
	var csoundInputBuffer
		//Set up audio
		var audioContext;

	if('webkitAudioContext' in window) {

		audioContext = new webkitAudioContext();

	}
	else if('AudioContext' in window){

		audioContext = new AudioContext();
	}

	var outputChannelCount;
	var inputChannelCount;
	var samplerate = audioContext.sampleRate;
	var framesPerCallback = 256;
	var krate = samplerate / framesPerCallback;
	var useAudioInput = false;
	var scriptProcessor;
	var microphoneNode;
	var allocateCsoundBuffer = function(framesPerCallback, outputChannelCount)
	{
		csoundBuffer = {};
		floatBuffer = new Float64Array(framesPerCallback * outputChannelCount);
		numBytes = floatBuffer.length * floatBuffer.BYTES_PER_ELEMENT;
		memoryPointer = Module._calloc(numBytes);

		csoundBuffer.heapBytes = new Uint8Array(Module.HEAPU8.buffer, memoryPointer, numBytes);
		//		csoundBuffer.heapBytes.set(new Uint8Array(floatBuffer.buffer));
		csoundBuffer.heapFloats = new Float64Array(csoundBuffer.heapBytes.buffer,
				csoundBuffer.heapBytes.byteOffset,
				floatBuffer.length);

		return csoundBuffer;
	}


	this.compileCSD = function(fileName)
	{       _reset(_self);
		_compileCSD(_self, fileName, samplerate, krate, framesPerCallback);
		outputChannelCount = _getNchnls(_self);
		inputChannelCount = _getNchnlsInput(_self);

		scriptProcessor = audioContext.createScriptProcessor(framesPerCallback, inputChannelCount, outputChannelCount);

		if (outputChannelCount > 0) {

			csoundOutputBuffer = allocateCsoundBuffer(framesPerCallback, outputChannelCount);
			csoundOutputBufferPointer = csoundOutputBuffer.heapBytes.byteOffset;
		}
		playingCSD = true;

		csoundInputBuffer = allocateCsoundBuffer(framesPerCallback, inputChannelCount);
		csoundInputBufferPointer = csoundInputBuffer.heapBytes.byteOffset;
	}

	this.compileOrc = function(orcString)
	{
		_compileOrc(_self, orcString);
		outputChannelCount = _getNchnls(_self);


		if (outputChannelCount > 0) {

			csoundOutputBuffer = allocateCsoundBuffer(framesPerCallback, outputChannelCount);
			csoundOutputBufferPointer = csoundOutputBuffer.heapBytes.byteOffset;
		}

		if (useAudioInput == true) {

			inputChannelCount = 1;
			csoundInputBuffer = allocateCsoundBuffer(framesPerCallback , inputChannelCount);
			csoundInputBufferPointer = csoundInputBuffer.heapBytes.byteOffset;
		}
		scriptProcessor = audioContext.createScriptProcessor(framesPerCallback, inputChannelCount, outputChannelCount);
	}

	this.readScore = function(scoreString)
	{
		_readScore(_self, scoreString);
	}

	this.enableAudioInput = function()
	{
		useAudioInput = true;
		navigator.getMedia = (navigator.getUserMedia ||
				navigator.webkitGetUserMedia ||
				navigator.mozGetUserMedia ||
				navigator.msGetUserMedia);

		if (navigator.getMedia == null) {

			console.log("This browser does not support audio input");
			return;
		}

		_setUsingAudioInput(_self, 1);
		navigator.getMedia({audio:true}, 

				function(stream) {

				microphoneNode = audioContext.createMediaStreamSource(stream);
				microphoneNode.connect(scriptProcessor);

				},

				function(err) {

				console.log("The following error occured: " + err);
				}
				);

	}

	this.setControlChannel = function(channelName, value)
	{
		_setControlChannel(_self, channelName, value);
	}

	this.startAudioCallback = function()
	{
		scriptProcessor.connect(audioContext.destination);
		if (useAudioInput == true) {

			microphoneNode.connect(scriptProcessor);
		}
		scriptProcessor.onaudioprocess = function(e)
		{
			process(e);
		}

		console.log("start");
	}

	this.stopAudioCallback = function()
	{
		if (useAudioInput == true) {

			microphoneNode.disconnect();
		}
		scriptProcessor.disconnect();
		scriptProcessor.onaudioprocess = null; //Needed for weird Firefox behaviour.
		console.log("stop");
	}

	var process = function(e)
	{
		if (useAudioInput == true) {

			for (i = 0; i < framesPerCallback; ++i) {

				csoundInputBuffer.heapFloats[i] = e.inputBuffer.getChannelData(0)[i];
			}	
		}

		for (controlChannel = 0; controlChannel < csoundControlChannels.length; ++controlChannel) {

			if (csoundControlChannels[controlChannel].updated == true) {

				_setControlChannel(_self,
						csoundControlChannels[controlChannel].name,
						csoundControlChannels[controlChannel].value);
			}
		}

		//Perform synthesis
		result = _process(_self, framesPerCallback, csoundInputBufferPointer, csoundOutputBufferPointer);
		if (result != 0) {

			that.stopAudioCallback();
		}
		for (i = 0; i < outputChannelCount; ++i) {

			for (j = 0; j < framesPerCallback; ++j) {

				e.outputBuffer.getChannelData(i)[j] = csoundOutputBuffer.heapFloats[j * outputChannelCount + i];
			}	
		}
	}
}
