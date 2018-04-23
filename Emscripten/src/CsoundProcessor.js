'use strict';

const WAM = AudioWorkletGlobalScope.WAM;

WAM["ENVIRONMENT"] = "WEB";
WAM["print"] = (t) => console.log(t);
WAM["printErr"] = (t) => console.log(t);


// INITIALIAZE WASM
AudioWorkletGlobalScope.libcsound(WAM);

// Get cwrap-ed functions
const Csound = {

  new: WAM.cwrap('CsoundObj_new', ['number'], null),
  compileCSD: WAM.cwrap('CsoundObj_compileCSD', null, ['number', 'string']),
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
  compileOrc: WAM.cwrap('CsoundObj_compileOrc', 'number', ['number', 'string']),
  setOption: WAM.cwrap('CsoundObj_setOption', null, ['number', 'string']),
  prepareRT: WAM.cwrap('CsoundObj_prepareRT', null, ['number']),
  getScoreTime: WAM.cwrap('CsoundObj_getScoreTime', null, ['number']),
  setTable: WAM.cwrap('CsoundObj_setTable', null, ['number', 'number', 'number', 'number']),
  play: WAM.cwrap('CsoundObj_play', null, ['number']),
  pause: WAM.cwrap('CsoundObj_pause', null, ['number']),

}
//var _openAudioOut = WAM.cwrap('CsoundObj_openAudioOut', null, ['number']);
//var _closeAudioOut = WAM.cwrap('CsoundObj_closeAudioOut', null, ['number']);
//


class CsoundProcessor extends AudioWorkletProcessor {

  static get parameterDescriptors() {
    return [];
  }

  constructor(options) {
    super(options);

    let csObj = Csound.new();
    this.csObj = csObj;
    Csound.setOption(this.csObj, "-odac");
    Csound.setOption(this.csObj, "-+rtaudio=null");

    this.port.onmessage = this.handleMessage.bind(this);
  }


  process(inputs, outputs, parameters) {
    if (this.csoundOutputBuffer == null) {
      return true;
    }

    let input = inputs[0];
    let output = outputs[0];

    let bufferLen = output[0].length;

    let csOut = this.csoundOutputBuffer;
    let ksmps = this.ksmps;
    let zerodBFS = this.zerodBFS;

    let cnt = this.cnt;
    let nchnls = this.nchnls;

    for (let i = 0; i < bufferLen; i++, cnt++) {
      if(cnt == ksmps) {
        // if we need more samples from Csound
        Csound.performKsmps(this.csObj);
        cnt = 0;
      }

      // de-interleave 
      for (let channel = 0; channel < output.length; channel++) {
        let outputChannel = output[channel];
        outputChannel[i] = csOut[cnt*nchnls + channel] / zerodBFS;
      } 
    }

    this.cnt = cnt;

    return true;
  }

  start() {
    let csObj = this.csObj;
    let ksmps = Csound.getKsmps(csObj);
    this.ksmps = ksmps;
    this.cnt = ksmps;
    let outputChannelCount = 2;
    let inputChannelCount = 1;

    //hardcode for now, but has to match Node's input/output settings
    this.nchnls = 2;
    this.nchnls_i = 1;

    Csound.prepareRT(csObj);
    Csound.play(csObj);


    let outputPointer = Csound.getOutputBuffer(csObj);
    this.csoundOutputBuffer = new Float32Array(WAM.HEAP8.buffer, outputPointer, ksmps * outputChannelCount);
    let inputPointer = Csound.getInputBuffer(csObj);
    this.csoundInputBuffer = new Float32Array(WAM.HEAP8.buffer, inputPointer, ksmps * inputChannelCount);
    this.zerodBFS = Csound.getZerodBFS(csObj);

  }

  compileOrc(orcString) {
    Csound.compileOrc(this.csObj, orcString);
  }

  handleMessage(event) {
    let data = event.data;

    switch (data[0]) {
      case "compileCSD":
        Csound.compileCSD(this.csObj, data[1]);
        break;
      case "compileOrc":
        Csound.compileOrc(this.csObj, data[1]);
        break;
      case "evalCode":
        Csound.evaluateCode(this.csObj, data[1]);
        break;
      case "readScore":
        Csound.readScore(this.csObj, data[1]);
        break;
      case "setControlChannel":
        Csound.setControlChannel(this.csObj,
          data[1], data[2]);
        break;
      case "setStringChannel":
        Csound.setStringChannel(this.csObj,
          data[1], data[2]);
        break;
      case "start":
        this.start();
        break;
      case "setOption":
        Csound.setOption(this.csObj, data[1]);
        break;
      case "reset":
        Csound.reset(this.csObj);
        Csound.setOption(this.csObj, "-odac");
        Csound.setOption(this.csObj, "-+rtaudio=null");
        this.csoundOutputBuffer = null; 
        this.ksmps = null; 
        this.zerodBFS = null; 
        break;
      default:
        console.log('[CsoundAudioProcessor] Invalid Message: "' + event.data);
    }
  }
}

registerProcessor('Csound', CsoundProcessor);
