var CsoundObj = function(printLog) {

	var that = this;
	var _new = cwrap('CsoundObj_new', ['number'], ['number', 'number']);
	var _compileCSD = cwrap('CsoundObj_compileCSD', null, ['number', 'string', 'number']);
	var _process = cwrap('CsoundObj_process', ['number'], ['number', 'number', 'number']);
	var _reset = cwrap('CsoundObj_reset', null, ['number']);
	var _getInputChannelCount = cwrap('CsoundObj_getInputChannelCount', ['number'], ['number']);
	var _getOutputChannelCount = cwrap('CsoundObj_getOutputChannelCount', ['number'], ['number']);
	var bufferSize = 256.;
	var _self = _new(bufferSize, printLog);

	var getAudioContext = function() {

		try {
			window.AudioContext = window.AudioContext || window.webkitAudioContext;
			return new AudioContext();	
		}
		catch(error) {

			alert('Web Audio API is not supported in this browser');
		}
	};

	var audioContext = getAudioContext();
	var samplerate = audioContext.sampleRate;

	this.compileCSD = function(filePath) {

		_compileCSD(_self, filePath, 44100);
	};

	var getAudioProcessNode = function() {

		var inputChannelCount = _getInputChannelCount(_self);
		var outputChannelCount = _getOutputChannelCount(_self);

		var audioProcessNode = audioContext.createScriptProcessor(bufferSize, inputChannelCount, outputChannelCount);
		audioProcessNode.inputCount = inputChannelCount;
		audioProcessNode.outputCount = outputChannelCount;
		return audioProcessNode;
	};

	var inputBuffer;
	var outputBuffer;

	var allocateIOBuffer = function(channelCount) {

		var buffer = {};	
		var floatArray = new Float64Array(bufferSize * channelCount);
		var numBytes = floatArray.length * floatArray.BYTES_PER_ELEMENT;


		buffer.memoryPointer = Module._calloc(numBytes);

		buffer.heapBytes = new Uint8Array(Module.HEAPU8.buffer, buffer.memoryPointer, numBytes);
		buffer.heapFloats = new Float64Array(buffer.heapBytes.buffer, buffer.heapBytes.byteOffset, floatArray.length);
		return buffer;
	};

	var audioProcessNode;


	this.start = function() {

		audioProcessNode = getAudioProcessNode();
		inputBuffer = allocateIOBuffer(audioProcessNode.inputCount);
		outputBuffer = allocateIOBuffer(audioProcessNode.outputCount);
		var outputChannelCount = audioProcessNode.outputCount;
		audioProcessNode.onaudioprocess = function(e) {

			console.log("processing");

			result = _process(_self, inputBuffer.heapBytes.byteOffset, outputBuffer.heapBytes.byteOffset);

			if (result != 0) {
				audioProcessNode.disconnect();
				audioProcessNode.onaudioprocess = null;

				_reset(_self);
			}

			for (i = 0; i < outputChannelCount; ++i) {

				for (j = 0; j < bufferSize; ++j) {

					e.outputBuffer.getChannelData(i)[j] = outputBuffer.heapFloats[j * outputChannelCount + i];
				}	
			}

		}

		audioProcessNode.connect(audioContext.destination);
	};

	this.stop = function() {

		console.log("stop");
		audioProcessNode.disconnect();
		audioProcessNode.onaudioprocess = null;
		//Module._free(outputBuffer.heapBytes.byteOffset);
		//Module._free(inputBuffer.heapBytes.byteOffset);
	}
};

