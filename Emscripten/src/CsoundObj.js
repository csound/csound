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
var CSOUND_NODE_SCRIPT;


/* SETUP NODE TYPE */
if(typeof AudioWorkletNode !== 'undefined' &&
  CSOUND_AUDIO_CONTEXT.audioWorklet !== null) {

  console.log("Using WASM + AudioWorklet Csound implementation");
  CSOUND_NODE_SCRIPT = 'CsoundNode.js';

} else {

  console.log("Using WASM + ScriptProcessorNode Csound implementation");
  CSOUND_NODE_SCRIPT = 'CsoundScriptProcessorNode.js';
}


// Utility function to load a script and set callback
const csound_load_script = function(src, callback) {
	var script = document.createElement('script');
	script.src = src;
	script.onload = callback;
	document.head.appendChild(script);
}


/* CsoundObj 
 * This ES6 Class is designed to be compatible with
 * the previous ScriptProcessorNode-based CsoundObj
 */

class CsoundObj {
  constructor() {
    this.audioContext = CSOUND_AUDIO_CONTEXT;

    // exposes node as property, user may access to set port onMessage callback
    // or we can add a setOnMessage(cb) method on CsoundObj...
    this.node = CsoundNodeFactory.createNode();
    this.node.connect(this.audioContext.destination);
    this.microphoneNode = null;
  }

  getNode() {
    return this.node;
  }

  // methods delegate to node
  writeToFS(filePath, blobData) {
    this.node.writeToFS(filePath, blobData);
  }

  compileCSD(filePath) {
    this.node.compileCSD(filePath);
  }

  compileOrc(orcString) {
    this.node.compileOrc(orcString);
  }

  setOption(option) {
    this.node.setOption(option);  
  }

  render(filePath) {
  }

  evaluateCode(codeString) {
    this.node.evaluateCode(codeString);
  }

  readScore(scoreString) {
    this.node.readScore(scoreString);
  }

  setControlChannel(channelName, value) {
    this.node.setControlChannel(channelName, value);
  }

  setStringChannel(channelName, value) {
    this.node.setStringChannel(channelName, value);
  }

  start() {
    if(this.microphoneNode != null) {
      this.microphoneNode.connect(this.node);
    }
    this.node.start();
  }

  reset() {
    this.node.reset();
  }

  destroy() {
  }

  play() {
    this.node.play();
  }

  stop() {
    this.node.stop();
  }

  setMessageCallback(msgCallback) {
    this.node.setMessageCallback(msgCallback);
  }


  enableAudioInput(audioInputCallback) {

    navigator.getUserMedia = navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia || null;
    let that = this;

    if (navigator.getUserMedia === null) {
      //Module['print']("Audio Input not supported in this browser");
      audioInputCallback(false);
    } else {
      let onSuccess = function(stream) {
        that.microphoneNode = CSOUND_AUDIO_CONTEXT.createMediaStreamSource(stream);
        audioInputCallback(true);
      };

      let onFailure = function(error) {
        that.microphoneNode = null;
        audioInputCallback(false);
        //Module['print']("Could not initialise audio input, error:" + error);
      };
      navigator.getUserMedia({
        audio: true, 
        video: false
      }, onSuccess, onFailure);
    }
  }

  /** Use to asynchronously setup AudioWorklet */
  static importScripts(script_base='./') {
    return new Promise((resolve) => {

      csound_load_script(script_base + CSOUND_NODE_SCRIPT, () => {
        CsoundNodeFactory.importScripts(script_base).then(() => {
          resolve();
        })
      })
    }) 
  }

  /** Node creation */
  static createNode() {
    return CsoundNodeFactory.createNode();
  }

}

