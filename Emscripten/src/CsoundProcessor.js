/*
    CsoundProcessor.js

    Copyright (C) 2018 Steven Yi, Victor Lazzarini

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

import libcsound from './libcsound-worklet.js'

const CSMOD = {} 

let printCallbacks = [];
let printMessages = (t) => {
  for(let i = 0; i < printCallbacks.length; i++) {
    printCallbacks[i](t);
  }
};

CSMOD["ENVIRONMENT"] = "WEB";
CSMOD["print"] = printMessages;
CSMOD["printErr"] = printMessages;


// INITIALIAZE WASM
libcsound(CSMOD);

// SETUP FS

let FS = CSMOD["FS"];
let pointerStringify = CSMOD["Pointer_stringify"];



// Get cwrap-ed functions
const Csound = {

    new: CSMOD.cwrap('CsoundObj_new', ['number'], null),
    compileCSD: CSMOD.cwrap('CsoundObj_compileCSD', ['number'], ['number', 'string']),
    evaluateCode: CSMOD.cwrap('CsoundObj_evaluateCode', ['number'], ['number', 'string']),
    readScore: CSMOD.cwrap('CsoundObj_readScore', ['number'], ['number', 'string']),

    reset: CSMOD.cwrap('CsoundObj_reset', null, ['number']),
    getOutputBuffer: CSMOD.cwrap('CsoundObj_getOutputBuffer', ['number'], ['number']),
    getInputBuffer: CSMOD.cwrap('CsoundObj_getInputBuffer', ['number'], ['number']),
    getControlChannel: CSMOD.cwrap('CsoundObj_getControlChannel', ['number'], ['number', 'string']),
    setControlChannel: CSMOD.cwrap('CsoundObj_setControlChannel', null, ['number', 'string', 'number']),
    getStringChannel: CSMOD.cwrap('CsoundObj_getStringChannel', ['string'], ['number', 'string']),
    setStringChannel: CSMOD.cwrap('CsoundObj_setStringChannel', null, ['number', 'string', 'string']),
    getKsmps: CSMOD.cwrap('CsoundObj_getKsmps', ['number'], ['number']),
    performKsmps: CSMOD.cwrap('CsoundObj_performKsmps', ['number'], ['number']),

    render: CSMOD.cwrap('CsoundObj_render', null, ['number']),
    getInputChannelCount: CSMOD.cwrap('CsoundObj_getInputChannelCount', ['number'], ['number']),
    getOutputChannelCount: CSMOD.cwrap('CsoundObj_getOutputChannelCount', ['number'], ['number']),
    getTableLength: CSMOD.cwrap('CsoundObj_getTableLength', ['number'], ['number', 'number']),
    getTable: CSMOD.cwrap('CsoundObj_getTable', ['number'], ['number', 'number']),
    getZerodBFS: CSMOD.cwrap('CsoundObj_getZerodBFS', ['number'], ['number']),
    compileOrc: CSMOD.cwrap('CsoundObj_compileOrc', 'number', ['number', 'string']),
    setOption: CSMOD.cwrap('CsoundObj_setOption', null, ['number', 'string']),
    prepareRT: CSMOD.cwrap('CsoundObj_prepareRT', null, ['number']),
    getScoreTime: CSMOD.cwrap('CsoundObj_getScoreTime', null, ['number']),
    setTable: CSMOD.cwrap('CsoundObj_setTable', null, ['number', 'number', 'number', 'number']),
    pushMidiMessage: CSMOD.cwrap('CsoundObj_pushMidiMessage', null, ['number', 'number', 'number', 'number']),
    setMidiCallbacks: CSMOD.cwrap('CsoundObj_setMidiCallbacks', null, ['number']),

}

class CsoundProcessor extends AudioWorkletProcessor {

    static get parameterDescriptors() {
        return [];
    }

    constructor(options) {
        super(options);
        let p = this.port;
        printCallbacks.push((t) => {
            p.postMessage(["log", t]);
        });

        let csObj = Csound.new();
        this.csObj = csObj;
        // engine status
        this.status = 0;
        this.running = false;
        this.started = false;
        this.sampleRate = sampleRate;  

        Csound.setMidiCallbacks(csObj);
        Csound.setOption(csObj, "-odac");
        Csound.setOption(csObj, "-iadc");
        Csound.setOption(csObj, "-M0");
        Csound.setOption(csObj, "-+rtaudio=null");
        Csound.setOption(csObj, "-+rtmidi=null");
        Csound.setOption(csObj, "--sample-rate="+sampleRate);  
        Csound.prepareRT(csObj);
        this.nchnls = options.outputChannelCount[0];
        this.nchnls_i = options.numberOfInputs;
        Csound.setOption(csObj, "--nchnls=" + this.nchnls);
        Csound.setOption(csObj, "--nchnls_i=" + this.nchnls_i); 

        this.port.onmessage = this.handleMessage.bind(this);
        this.port.start();
    }


    process(inputs, outputs, parameters) {
        if (this.csoundOutputBuffer == null ||
            this.running == false) {
             let output =  outputs[0];
                let bufferLen = output[0].length;
                for (let i = 0; i < bufferLen; i++) {
                    for (let channel = 0; channel < output.numberOfChannels; channel++) {
                        let outputChannel = output[channel];
                        outputChannel[i] = 0;
                    }
                }
            return true;
        }

        let input = inputs[0];
        let output = outputs[0];

        let bufferLen = output[0].length;

        let csOut = this.csoundOutputBuffer;
        let csIn = this.csoundInputBuffer;
        let ksmps = this.ksmps;
        let zerodBFS = this.zerodBFS;

        let cnt = this.cnt;
        let nchnls = this.nchnls;
        let nchnls_i = this.nchnls_i;
        let status = this.status;

        for (let i = 0; i < bufferLen; i++, cnt++) {
            if(cnt == ksmps && status == 0) {
                // if we need more samples from Csound
                status = Csound.performKsmps(this.csObj);
                cnt = 0;
            }

            for (let channel = 0; channel < input.length; channel++) {
                let inputChannel = input[channel];
                csIn[cnt*nchnls_i + channel] = inputChannel[i] * zerodBFS;
            }
            for (let channel = 0; channel < output.length; channel++) {
                let outputChannel = output[channel];
                if(status == 0)
                    outputChannel[i] = csOut[cnt*nchnls + channel] / zerodBFS;
                else
                    outputChannel[i] = 0;
            } 
        }

        this.cnt = cnt;
        this.status = status;

        return true;
    }

    start() {
        if(this.started == false) {
            let csObj = this.csObj;
            let ksmps = Csound.getKsmps(csObj);
            this.ksmps = ksmps;
            this.cnt = ksmps;
            
            let outputPointer = Csound.getOutputBuffer(csObj);
            this.csoundOutputBuffer = new Float32Array(CSMOD.HEAP8.buffer, outputPointer, ksmps * this.nchnls);
            let inputPointer = Csound.getInputBuffer(csObj);
            this.csoundInputBuffer = new Float32Array(CSMOD.HEAP8.buffer, inputPointer, ksmps * this.nchnls_i);
            this.zerodBFS = Csound.getZerodBFS(csObj);
            this.started = true;
        }
        this.running = true;
    }

    compileOrc(orcString) {
        Csound.compileOrc(this.csObj, orcString);
    }

    handleMessage(event) {
        let data = event.data;
        let p = this.port;

        switch (data[0]) {
        case "compileCSD":
            this.status = Csound.compileCSD(this.csObj, data[1]);
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
        case "stop":
            this.running = false;
            break;
        case "play":
            this.start();
            break;
        case "setOption":
            Csound.setOption(this.csObj, data[1]);
            break;
        case "reset":
            let csObj = this.csObj;
            this.started = false;
            this.running = false;
            Csound.reset(csObj);
            Csound.setMidiCallbacks(csObj);
            Csound.setOption(csObj, "-odac");
            Csound.setOption(csObj, "-iadc");
            Csound.setOption(csObj, "-M0");
            Csound.setOption(csObj, "-+rtaudio=null");
            Csound.setOption(csObj, "-+rtmidi=null");
            Csound.setOption(csObj, "--sample-rate="+this.sampleRate);  
            Csound.prepareRT(csObj);
            //this.nchnls = options.numberOfOutputs;
            //this.nchnls_i = options.numberOfInputs;
            Csound.setOption(csObj, "--nchnls=" + this.nchnls);
            Csound.setOption(csObj, "--nchnls_i=" + this.nchnls_i);
            this.csoundOutputBuffer = null; 
            this.ksmps = null; 
            this.zerodBFS = null; 
            break;
        case "writeToFS":
            let name = data[1];
            let blobData = data[2];
            let buf = new Uint8Array(blobData)
            let stream = FS.open(name, 'w+');
            FS.write(stream, buf, 0, buf.length, 0);
            FS.close(stream);

            break;
        case "midiMessage":
            let byte1 = data[1];
            let byte2 = data[2];
            let byte3 = data[3];
            Csound.pushMidiMessage(this.csObj, byte1, byte2, byte3);
            break;
        case "getControlChannel":
            let channel = data[1];
            let value = Csound.getControlChannel(this.csObj, channel);
            p.postMessage(["control", channel, value]);
            break;
        case "getStringChannel":
            let schannel = data[1];
            let svalue = Csound.getStringChannel(this.csObj, schannel);
            svalue = pointerStringify(svalue);
            p.postMessage(["stringChannel", schannel, svalue]);
            break;
        case "getTable":
            let buffer = Csound.getTable(this.csObj, data[1]);
            let len = Csound.getTableLength(this.csObj, data[1]);
            let src = new Float32Array(CSMOD.HEAP8.buffer, buffer, len);
            let table = new Float32Array(src);
            p.postMessage(["table", data[1], table]);
            break;
        case "setTableAtIndex":
            Csound.setTable(this.csObj, data[1], data[2], data[3]);
            break;
        case "setTable":
            let cstable = new Float32Array(data[2]);
            for(let i = 0; i < cstable.length; i++)
                Csound.setTable(this.csObj, data[1], i, cstable[i]);
            break;         
        default:
            console.log('[CsoundAudioProcessor] Invalid Message: "' + event.data);
        }
    }
}

registerProcessor('Csound', CsoundProcessor);
