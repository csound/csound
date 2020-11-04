import * as Comlink from "comlink";
import ScriptProcessorNodeWorker from "@root/workers/old-spn.worker";
import { logSPN } from "@root/logger";
import { IPCMessagePorts } from "@root/mains/messages.main";
// import {
//   audioWorkerAudioInputPort,
//   audioWorkerFrameRequestPort,
//   cleanupPorts,
//   emitInternalCsoundLogEvent,
//   workerMessagePortAudio,
// } from "@root/mains/messages.main";

const connectedMidiDevices = new Set();

class ScriptProcessorNodeMainThread {
  constructor() {
    this.ipcMessagePorts = new IPCMessagePorts();

    this.audioCtx = undefined;
    this.currentPlayState = undefined;
    this.csoundWorkerMain = undefined;

    // never default these, get it from
    // csound-worker before starting
    this.sampleRate = undefined;
    this.inputsCount = undefined;
    this.outputsCount = undefined;
    this.hardwareBufferSize = undefined;
    this.softwareBufferSize = undefined;

    this.initIframe = this.initIframe.bind(this);
    this.initialize = this.initialize.bind(this);
    this.onPlayStateChange = this.onPlayStateChange.bind(this);
    this.scriptProcessorNode = true;
    logSPN("ScriptProcessorNodeMainThread was constructed");
  }

  async onPlayStateChange(newPlayState) {
    this.currentPlayState = newPlayState;
    this.spnWorker && this.spnWorker.postMessage({ playStateChange: newPlayState }, "*");
    switch (newPlayState) {
      case "realtimePerformanceStarted": {
        logSPN("event received: realtimePerformanceStarted");
        try {
          await this.initialize();
        } catch (error) {
          console.error(error);
        }
        break;
      }
      case "realtimePerformanceEnded": {
        logSPN("event received: realtimePerformanceEnded");
        this.ipcMessagePorts.restart(this.csoundWorkerMain);
        this.currentPlayState = undefined;
        this.sampleRate = undefined;
        this.inputsCount = undefined;
        this.outputsCount = undefined;
        this.hardwareBufferSize = undefined;
        this.softwareBufferSize = undefined;
        break;
      }
      default: {
        break;
      }
    }
  }

  connectPorts() {
    logSPN("initializing MessagePort on worker threads");
    this.spnWorker.postMessage({ msg: "initMessagePort" }, "*", [
      this.ipcMessagePorts.workerMessagePortAudio,
    ]);
    this.spnWorker.postMessage({ msg: "initAudioInputPort" }, "*", [
      this.ipcMessagePorts.audioWorkerAudioInputPort,
    ]);
    this.spnWorker.postMessage({ msg: "initRequestPort" }, "*", [
      this.ipcMessagePorts.audioWorkerFrameRequestPort,
    ]);
    this.spnWorker.postMessage({ playStateChange: this.currentPlayState }, "*");
  }

  async initIframe() {
    // HACK FROM (but it works just fine when adding modern security models)
    // https://github.com/GoogleChromeLabs/audioworklet-polyfill/blob/274792e5e3d189e04c9496bed24129118539b4b5/src/realm.js#L18-L20
    if (typeof window === "undefined" || typeof window.document === "undefined") {
      throw "Can only run SPN in Browser scope";
    }

    const parentScope = window.document;
    const iFrameHtml = [
      `<!doctype html>`,
      `<html lang="en">`,
      `<head>`,
      `</head>`,
      `<body>`,
      `<script type="text/javascript" src="${ScriptProcessorNodeWorker()}"></script>`,
      `</body>`,
    ].join("\n");

    const iFrameBlob = new Blob([iFrameHtml], { type: "text/html" });
    const iFrame = document.createElement("iframe");

    iFrame.src = URL.createObjectURL(iFrameBlob);
    iFrame.sandbox.add("allow-scripts", "allow-same-origin");

    iFrame.style.cssText = "position:absolute;left:0;top:-999px;width:1px;height:1px;";

    // appending early to have access to contentWindow
    const iFrameOnLoad = new Promise((resolve) => {
      iFrame.onload = () => {
        resolve();
      };
    });

    parentScope.body.appendChild(iFrame);

    try {
      await iFrameOnLoad;
    } catch (error) {
      console.error(error);
    }

    const iFrameWin = iFrame.contentWindow;
    const iFrameDoc = iFrameWin.document;

    this.spnWorker = iFrameWin;
  }

  async initialize() {
    if (!this.spnWorker) {
      await this.initIframe();
      if (!this.spnWorker) {
        console.error("SPN FATAL: Couldn't create iFrame");
        return;
      }
    }

    this.connectPorts();

    this.spnWorker.postMessage(
      {
        msg: "makeSPNClass",
        argumentz: {
          hardwareBufferSize: 32768,
          softwareBufferSize: 2048,
          inputsCount: this.inputsCount,
          outputsCount: this.outputsCount,
          sampleRate: this.sampleRate,
        },
      },
      "*",
    );

    if (!this.csoundWorkerMain) {
      log.error(`fatal: worker not reachable from worklet-main thread`);
      return;
    }

    if (this.isRequestingMidi) {
      console.error("todo");
    }

    if (this.isRequestingInput) {
      console.error("todo");
    }
  }
}

export default ScriptProcessorNodeMainThread;
