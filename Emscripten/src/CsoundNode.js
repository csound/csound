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


/* CsoundNode class to use for AudioWorklet */
class CsoundNode extends AudioWorkletNode {

  constructor(context, options) {
    options = options || {};
    options.numberOfInputs  = 1;
    options.numberOfOutputs = 2;
    options.channelCount = 2;

    super(context, 'Csound', options);

    this.msgCallback = (msg) => { console.log(msg); }

    this.port.start();
    this.port.onmessage = (event) => {
      let data = event.data;
      switch(data[0]) {
        case "log":
          this.msgCallback(data[1]);
          break;
          default:
          console.log('[CsoundNode] Invalid Message: "' + event.data);
      }

    };
  }

  writeToFS(filePath, blobData) {
    this.port.postMessage(["writeToFS", filePath, blobData]);
  }

  compileCSD(filePath) {
    this.port.postMessage(["compileCSD", filePath]);
  }

  compileOrc(orcString) {
    this.port.postMessage(["compileOrc", orcString]);
  }

  setOption(option) {
    this.port.postMessage(["setOption", option]);    
  }

  render(filePath) {
  }

  evaluateCode(codeString) {
    this.port.postMessage(["evalCode", codeString]);
  }

  readScore(scoreString) {
    this.port.postMessage(["readScore", scoreString]);
  }

  setControlChannel(channelName, value) {
    this.port.postMessage(["setControlChannel",
      channelName, value]);
  }

  setStringChannel(channelName, value) {
    this.port.postMessage(["setStringChannel",
      channelName, value]);
  }

  start() {
    this.port.postMessage(["start"]);
  }

  reset() {
    this.port.postMessage(["reset"]);
  }

  destroy() {
  }

  openAudioOut() {
  }

  closeAudioOut() {
  }

  play() {
    this.port.postMessage(["play"]);
  }

  stop() {
    this.port.postMessage(["stop"]);
  }

  setMessageCallback(msgCallback) {
    this.msgCallback = msgCallback;
  }
  
  midiMessage(byte1, byte2, byte3) {
    this.port.postMessage(["midiMessage", byte1, byte2, byte3]);
  }

}



class CsoundNodeFactory {

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

  static createNode(inputChannelCount=1, outputChannelCount=2) {
    return new CsoundNode(CSOUND_AUDIO_CONTEXT);
  }
}




