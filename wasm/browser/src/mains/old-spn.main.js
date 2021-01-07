import * as Comlink from "comlink";
import ScriptProcessorNodeWorker from "@root/workers/old-spn.worker";
import { logSPN } from "@root/logger";
import { IPCMessagePorts } from "@root/mains/messages.main";

let UID = 0;

class ScriptProcessorNodeMainThread {
  constructor({ audioContext, audioContextIsProvided }) {
    this.ipcMessagePorts = new IPCMessagePorts();

    this.audioContext = audioContext;
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
    // hacky SAB timing fix when starting
    // eventually, replace this spaghetti with
    // private/internal event emitters
    if (this.csoundWorkerMain.startPromiz) {
      const startPromiz = this.csoundWorkerMain.startPromiz;
      setTimeout(() => {
        startPromiz();
      }, 0);
      delete this.csoundWorkerMain.startPromiz;
    }
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

    const contextUid = `audioWorklet${UID}`;
    UID += 1;

    const proxyPort = Comlink.wrap(Comlink.windowEndpoint(this.spnWorker));
    this.spnWorker[contextUid] = this.audioContext;
    await proxyPort.initialize(
      Comlink.transfer(
        {
          contextUid,
          hardwareBufferSize: 32768,
          softwareBufferSize: 2048,
          inputsCount: this.inputsCount,
          outputsCount: this.outputsCount,
          sampleRate: this.sampleRate,
          messagePort: this.ipcMessagePorts.workerMessagePortAudio,
          requestPort: this.ipcMessagePorts.audioWorkerFrameRequestPort,
        },
        [
          this.ipcMessagePorts.workerMessagePortAudio,
          this.ipcMessagePorts.audioWorkerFrameRequestPort,
        ],
      ),
    );
  }
}

export default ScriptProcessorNodeMainThread;
