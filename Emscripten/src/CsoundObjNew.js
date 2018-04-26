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


// Setup a single global AudioContext object
const CSOUND_AUDIO_CONTEXT = (function() {

	try {
	    var AudioContext = window.AudioContext || window.webkitAudioContext;
	    return new AudioContext();	
	}
	catch(error) {

	    console.log('Web Audio API is not supported in this browser');
	}
  return null;
}());


var CsoundObj;
var AudioWorkletGlobalScope = AudioWorkletGlobalScope || {};

/******************************************/
/* AUDIO WORKLET CSOUNDOBJ IMPLEMENTATION */ 
/******************************************/
if(typeof AudioWorkletNode !== 'undefined' &&
  CSOUND_AUDIO_CONTEXT.audioWorklet != null) {

  console.log("Using WASM + AudioWorklet Csound implementation");

  /* CsoundNode class to use for AudioWorklet */
  class CsoundNode extends AudioWorkletNode {

    constructor(context, options) {
      options = options || {};
      options.numberOfInputs  = 1;
      options.numberOfOutputs = 2;
      options.channelCount = 2;

      super(context, 'Csound', options);

      this.port.start();
    }

  }


  /* CsoundObj 
   * This ES6 Class is designed to be compatible with
   * the previous ScriptProcessorNode-based CsoundObj
   */

   CsoundObj = class {
    constructor() {
      this.audioContext = CSOUND_AUDIO_CONTEXT;

      // exposes node as property, user may access to set port onMessage callback
      // or we can add a setOnMessage(cb) method on CsoundObj...
      this.node = new CsoundNode(this.audioContext);
      this.node.connect(this.audioContext.destination);
    }

    compileCSD(filePath) {
      // not sure what to do about file path...
      // need to see what can be accessed in
      // worklet scope
      this.node.port.postMessage(["compileCSD", filePath]);
    }

    compileOrc(orcString) {
      this.node.port.postMessage(["compileOrc", orcString]);
    }

    setOption(option) {
      this.node.port.postMessage(["setOption", option]);    
    }

    render(filePath) {
    }

    evaluateCode(codeString) {
      this.node.port.postMessage(["evalCode", codeString]);
    }

    readScore(scoreString) {
      this.node.port.postMessage(["readScore", scoreString]);
    }

    setControlChannel(channelName, value) {
      this.node.port.postMessage(["setControlChannel",
        channelName, value]);
    }

    setStringChannel(channelName, value) {
      this.node.port.postMessage(["setStringChannel",
        channelName, value]);
    }

    start() {
      this.node.port.postMessage(["start"]);
    }

    reset() {
      this.node.port.postMessage(["reset"]);
    }

    destroy() {
    }

    openAudioOut() {
    }

    closeAudioOut() {
    }

    play() {
    }

    stop() {
    }


    /** Use to asynchronously setup AudioWorklet */
    static importScripts(script_base='./') {
      let actx = CSOUND_AUDIO_CONTEXT;
      return new Promise( (resolve) => {
        actx.audioWorklet.addModule(script_base + 'libcsound-worklet.wasm.js').then(() => {
        actx.audioWorklet.addModule(script_base + 'libcsound-worklet.js').then(() => {
        actx.audioWorklet.addModule(script_base + 'CsoundProcessor.js').then(() => {
          resolve(); 
        }) }) })      
      }) 
    }
  }


/**************************************************/
/* SCRIPT PROCESSOR NODE CSOUNDOBJ IMPLEMENTATION */ 
/**************************************************/
} else { 

  console.log("Using WASM + ScriptProcessorNode Csound implementation");

  // Create global var here 

  CsoundObj = function() { 


    var WAM = AudioWorkletGlobalScope.WAM;
    var Module = WAM;

    var that = this;
    var _new = WAM.cwrap('CsoundObj_new', ['number'], null);
    var _compileCSD = WAM.cwrap('CsoundObj_compileCSD', null, ['number', 'string']);
    var _evaluateCode = WAM.cwrap('CsoundObj_evaluateCode', ['number'], ['number', 'string']);
    var _readScore = WAM.cwrap('CsoundObj_readScore', ['number'], ['number', 'string']);
    var _reset = WAM.cwrap('CsoundObj_reset', null, ['number']);
    var _getOutputBuffer = WAM.cwrap('CsoundObj_getOutputBuffer', ['number'], ['number']);
    var _getInputBuffer = WAM.cwrap('CsoundObj_getInputBuffer', ['number'], ['number']);
    var _getControlChannel = WAM.cwrap('CsoundObj_getControlChannel', ['number'], ['number', 'string']);
    var _setControlChannel = WAM.cwrap('CsoundObj_setControlChannel', null, ['number', 'string', 'number']);
    var _setStringChannel = WAM.cwrap('CsoundObj_setStringChannel', null, ['number', 'string', 'string']);
    var _getKsmps = WAM.cwrap('CsoundObj_getKsmps', ['number'], ['number']);
    var _performKsmps = WAM.cwrap('CsoundObj_performKsmps', ['number'], ['number']);
    var _render = WAM.cwrap('CsoundObj_render', null, ['number']);
    var _getInputChannelCount = WAM.cwrap('CsoundObj_getInputChannelCount', ['number'], ['number']);
    var _getOutputChannelCount = WAM.cwrap('CsoundObj_getOutputChannelCount', ['number'], ['number']);
    var _getTableLength = WAM.cwrap('CsoundObj_getTableLength', ['number'], ['number', 'number']);
    var _getTable = WAM.cwrap('CsoundObj_getTable', ['number'], ['number', 'number']);
    var _getZerodBFS = WAM.cwrap('CsoundObj_getZerodBFS', ['number'], ['number']);
    var _setMidiCallbacks = WAM.cwrap('CsoundObj_setMidiCallbacks', null, ['number']);
    var _pushMidiMessage = WAM.cwrap('CsoundObj_pushMidiMessage', null, ['number', 'number', 'number', 'number']);
    var _setOutputChannelCallback = WAM.cwrap('CsoundObj_setOutputChannelCallback', null, ['number', 'number']);
    var _compileOrc = WAM.cwrap('CsoundObj_compileOrc', 'number', ['number', 'string']);
    var _setOption = WAM.cwrap('CsoundObj_setOption', null, ['number', 'string']);
    var _prepareRT = WAM.cwrap('CsoundObj_prepareRT', null, ['number']);
    var _getScoreTime = WAM.cwrap('CsoundObj_getScoreTime', null, ['number']);
    var _setTable = WAM.cwrap('CsoundObj_setTable', null, ['number', 'number', 'number', 'number']);
    var _openAudioOut = WAM.cwrap('CsoundObj_openAudioOut', null, ['number']);
    var _closeAudioOut = WAM.cwrap('CsoundObj_closeAudioOut', null, ['number']);
    var _play = WAM.cwrap('CsoundObj_play', null, ['number']);
    var _pause = WAM.cwrap('CsoundObj_pause', null, ['number']);
    var bufferSize;
    var _self = _new();
    var _destroy = WAM.cwrap('CsoundObj_destroy', null, ['number']);
    var running;

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

	//window.navigator = window.navigator || {};
	//navigator.getUserMedia = navigator.getUserMedia||navigator.webkitGetUserMedia ||navigator.mozGetUserMedia||null;

	//if (navigator.getUserMedia === null) {

	//    Module['print']("Audio Input not supported in this browser");
	//    audioInputCallback(false);
	//}	
	//else{
	//    function onSuccess(stream) {

	//  microphoneNode = audioContext.createMediaStreamSource(stream);	
	//  audioInputCallback(true);
	//    };
	//    function onFailure(error) {

	//  microphoneNode = null;	
	//  audioInputCallback(false);
	//  Module['print']("Could not initialise audio input, error:" +  error); 
	//    };
	//    navigator.getUserMedia({audio:true}, onSuccess, onFailure);
	//}		

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


    var audioContext = CSOUND_AUDIO_CONTEXT;
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

    this.setOption = function(option)
    {
	_setOption(_self, option);
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

		console("Insufficient number of Csound inputs: nchnls_i, not starting");
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
     
    /**
     * This is a stub to enable cross-platform compatibility of the core Csound API
     * across WASM, csound.node, CsoundQt, and Csound for Android.
     */
    this.perform = function() {
        console.log("CsoundObj.perform()...");
        return 0;
    }

    this.stop = function() {
        _openAudioOut(_self);
    }

    }

  CsoundObj.loadScript = function (src, callback) {
    var script = document.createElement('script');
    script.src = src;
    //script.async = ;
    script.onload = callback;
    document.head.appendChild(script);
  }


  CsoundObj.importScripts = function(script_base='./') {
    return new Promise((resolve) => {

      //CsoundObj.loadScript(script_base + 'libcsound.base64.js', () => {
      CsoundObj.loadScript(script_base + 'libcsound.js', () => {
        AudioWorkletGlobalScope.WAM = {}
        let WAM = AudioWorkletGlobalScope.WAM;
        console.log(AudioWorkletGlobalScope);

        WAM["ENVIRONMENT"] = "WEB";
        WAM["print"] = (t) => console.log(t);
        WAM["printErr"] = (t) => console.log(t);
        WAM["locateFile"] = (f) => script_base + f;

        AudioWorkletGlobalScope.libcsound(WAM).then(() => {
          resolve();
        }) 
      }) 
    });

    //}) 

  }
}

