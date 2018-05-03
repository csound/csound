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


// Global singleton variables
var AudioWorkletGlobalScope = AudioWorkletGlobalScope || {};
var CSOUND;

class CsoundNodeFactory {


    // Utility function to load a script and set callback
    static loadScript(src, callback) {
	var script = document.createElement('script');
	script.src = src;
	script.onload = callback;
	document.head.appendChild(script);
    }

    static importScripts(script_base='./') {
	return new Promise((resolve) => {
	    CsoundNodeFactory.loadScript(script_base + 'libcsound.js', () => {
		AudioWorkletGlobalScope.WAM = {}
		let WAM = AudioWorkletGlobalScope.WAM;

		WAM["ENVIRONMENT"] = "WEB";
		WAM["print"] = (t) => console.log(t);
		WAM["printErr"] = (t) => console.log(t);
		WAM["locateFile"] = (f) => script_base + f;

		AudioWorkletGlobalScope.libcsound(WAM).then(() => {

		    // Cache cwrap functions into CSOUND global object 
		    CSOUND = {
			new: WAM.cwrap('CsoundObj_new', ['number'], null),
			compileCSD: WAM.cwrap('CsoundObj_compileCSD', ['number'], ['number', 'string']),
			evaluateCode: WAM.cwrap('CsoundObj_evaluateCode', ['number'], ['number', 'string']),
			readScore: WAM.cwrap('CsoundObj_readScore', ['number'], ['number', 'string']),
			reset: WAM.cwrap('CsoundObj_reset', null, ['number']),
			getOutputBuffer: WAM.cwrap('CsoundObj_getOutputBuffer', ['number'], ['number']),
			getInputBuffer: WAM.cwrap('CsoundObj_getInputBuffer', ['number'], ['number']),
			getControlChannel: WAM.cwrap('CsoundObj_getControlChannel', ['number'], ['number', 'string']),
			setControlChannel: WAM.cwrap('CsoundObj_setControlChannel', null, ['number', 'string', 'number']),
			setStringChannel: WAM.cwrap('CsoundObj_setStringChannel', null, ['number', 'string', 'string']),
			getKsmps: WAM.cwrap('CsoundObj_getKsmps', ['number'], ['number']),
			performKsmps: WAM.cwrap('CsoundObj_performKsmps', ['number'], ['number']),
			render: WAM.cwrap('CsoundObj_render', null, ['number']),
			getInputChannelCount: WAM.cwrap('CsoundObj_getInputChannelCount', ['number'], ['number']),
			getOutputChannelCount: WAM.cwrap('CsoundObj_getOutputChannelCount', ['number'], ['number']),
			getTableLength: WAM.cwrap('CsoundObj_getTableLength', ['number'], ['number', 'number']),
			getTable: WAM.cwrap('CsoundObj_getTable', ['number'], ['number', 'number']),
			getZerodBFS: WAM.cwrap('CsoundObj_getZerodBFS', ['number'], ['number']),
			setMidiCallbacks: WAM.cwrap('CsoundObj_setMidiCallbacks', null, ['number']),
			pushMidiMessage: WAM.cwrap('CsoundObj_pushMidiMessage', null, ['number', 'number', 'number', 'number']),
			setOutputChannelCallback: WAM.cwrap('CsoundObj_setOutputChannelCallback', null, ['number', 'number']),
			compileOrc: WAM.cwrap('CsoundObj_compileOrc', 'number', ['number', 'string']),
			setOption: WAM.cwrap('CsoundObj_setOption', null, ['number', 'string']),
			prepareRT: WAM.cwrap('CsoundObj_prepareRT', null, ['number']),
			getScoreTime: WAM.cwrap('CsoundObj_getScoreTime', null, ['number']),
			setTable: WAM.cwrap('CsoundObj_setTable', null, ['number', 'number', 'number', 'number']),
			openAudioOut: WAM.cwrap('CsoundObj_openAudioOut', null, ['number']),
			closeAudioOut: WAM.cwrap('CsoundObj_closeAudioOut', null, ['number']),
			destroy: WAM.cwrap('CsoundObj_destroy', null, ['number'])
		    }

		    resolve();
		}) 
	    }) 
	});
    }

    static createNode(inputChannelCount=1, outputChannelCount=2) {
	var spn = CSOUND_AUDIO_CONTEXT.createScriptProcessor(0, inputChannelCount, outputChannelCount);
	//bufferSize = audioProcessNode.bufferSize;
	// console.error("bufferSize = " + bufferSize);
	spn.inputCount = inputChannelCount;
	spn.outputCount = outputChannelCount;

	let cs = CSOUND.new();
	CSOUND.setMidiCallbacks(cs);
	CSOUND.setOption(cs, "-odac");
	CSOUND.setOption(cs, "-iadc");
	CSOUND.setOption(cs, "-M0");
	CSOUND.setOption(cs, "-+rtaudio=null");
	CSOUND.setOption(cs, "-+rtmidi=null");
	CSOUND.prepareRT(cs);
	var sampleRate = CSOUND_AUDIO_CONTEXT.sampleRate;
	CSOUND.setOption(cs, "--sample-rate="+sampleRate);
	CSOUND.setOption(cs, "--nchnls=" + this.nchnls);
	CSOUND.setOption(cs, "--nchnls_i=" + this.nchnls_i); 

	let CsoundMixin = {
	    csound: cs,
	    compiled: false,
	    msgCallback: (t) => console.log(t),
	    csoundRunning: false,
	    csoundOutputBuffer: null,
	    csoundInputBuffer: null,
	    zerodBFS: 1.0,
	    offset: 32,
	    ksmps: 32,
	    running: false,
	    started: false,
	    cnt: 0,
	    res: 0,
	    nchnls_i: inputChannelCount, 
	    nchnls: outputChannelCount, 	

	    writeToFS(filePath, blobData) {
		let FS = WAM["FS"];
		let stream = FS.open(filePath, 'w+');
		let buf = new Uint8Array(blobData)
		FS.write(stream, buf, 0, buf.length, 0);
		FS.close(stream);
	    },

	    compileCSD(filePath) {
		CSOUND.prepareRT(this.csound);
		CSOUND.compileCSD(this.csound, filePath);
		this.compiled = true;
	    },

	    compileOrc(orcString) {
		CSOUND.prepareRT(this.csound);
		CSOUND.compileOrc(this.csound, orcString);
		this.compiled = true;
	    },

	    setOption(option) {
		CSOUND.setOption(this.csound, option);
	    },

	    render(filePath) {
		CSOUND.compileCSD(this.csound, filePath);
		CSOUND.render(this.csound);
		this.compiled = false;
	    },

	    evaluateCode(codeString) {
		CSOUND.evaluateCode(this.csound, codeString);
	    },

	    readScore(scoreString) {
		CSOUND.readScore(this.csound, scoreString);
	    },

	    setControlChannel(channelName, value) {
		CSOUND.setControlChannel(this.csound, channelName, value);
	    },

	    setStringChannel(channelName, value) {
		CSOUND.setStringChannel(this.csound, channelName, value); 
	    },

	    start() {
		if(this.started == false) {
		    let ksmps = CSOUND.getKsmps(this.csound);
		    this.ksmps = ksmps;
		    this.cnt = ksmps;

		    let outputPointer = CSOUND.getOutputBuffer(this.csound);
		    this.csoundOutputBuffer = new Float32Array(WAM.HEAP8.buffer, outputPointer, ksmps * outputChannelCount);
		    let inputPointer = CSOUND.getInputBuffer(this.csound);
		    this.csoundInputBuffer = new Float32Array(WAM.HEAP8.buffer, inputPointer, ksmps * inputChannelCount);
		    this.zerodBFS = CSOUND.getZerodBFS(this.csound);
		    this.started = true;
		}
		this.running = true;

	    },

	    reset() {
		CSOUND.reset(this.csound);
		this.compiled = false;
	    },

	    destroy() {
		CSOUND.destroy(this.csound);
	    },

	    play() {
		CSOUND.play(this.csound); 
	    },

	    setMessageCallback(msgCallback) {
		this.msgCallBack = msgCallback;
	    },

	    midiMessage(byte1, byte2, byte3) {
		CSOUND.pushMidiMessage(this.csound, byte1, byte2, byte3);
	    },

	    onaudioprocess(e) {
		if (this.csoundOutputBuffer == null ||
		    this.running == false) {
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
		let res = this.res;

		for (let i = 0; i < bufferLen; i++, cnt++) {
		    if(cnt >= ksmps && res == 0) {
			// if we need more samples from Csound
			res = CSOUND.performKsmps(this.csound);
			cnt = 0;
		    }

		    for (let channel = 0; channel < input.numberOfChannels; channel++) {
			let inputChannel = input.getChannelData(channel);
			csIn[cnt*nchnls_i + channel] =  inputChannel[i] * zerodBFS;
		    }
		    for (let channel = 0; channel < output.numberOfChannels; channel++) {
			let outputChannel = output.getChannelData(channel);
			if(res == 0)
			    outputChannel[i] = csOut[cnt*nchnls + channel] / zerodBFS;
			else
			    outputChannel[i] = 0;
		    } 
		}

		this.cnt = cnt;
		this.res = res;
	    }
	}


	let WAM = AudioWorkletGlobalScope.WAM;
	WAM["print"] = (t) => spn.msgCallback(t);
	WAM["printErr"] = (t) => spn.msgCallback(t);

	return Object.assign(spn,CsoundMixin);
    }
}
