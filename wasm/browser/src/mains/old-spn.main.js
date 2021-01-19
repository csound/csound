import * as Comlink from "comlink";
import ScriptProcessorNodeWorker from "@root/workers/old-spn.worker";
import { logOldSpnMain as log } from "@root/logger";
import { messageEventHandler } from "@root/mains/messages.main";
import { WebkitAudioContext } from "@root/utils";
import { requestMidi } from "@utils/request-midi";
import { requestMicrophoneNode } from "@root/mains/io.utils";

// we reuse the spnWorker
// since it handles multiple
// audio Contexts via UID.
let spnWorker;
let proxyPort;

let UID = 0;

class ScriptProcessorNodeMainThread {
  constructor({ audioContext, audioContextIsProvided, autoConnect }) {
    this.autoConnect = autoConnect;
    this.audioContextIsProvided = audioContextIsProvided;

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
    log("ScriptProcessorNodeMainThread was constructed")();
  }

  async terminateInstance() {
    delete this.onPlayStateChange;
    if (window[`__csound_wasm_iframe_parent_${this.contextUid}Node`]) {
      window[`__csound_wasm_iframe_parent_${this.contextUid}Node`].disconnect();
      delete window[`__csound_wasm_iframe_parent_${this.contextUid}Node`].disconnect();
    }
    if (this.audioContext) {
      if (this.audioContext.state !== "closed") {
        try {
          await this.audioContext.close();
        } catch (error) {}
      }
      delete this.audioContext;
    }
    if (proxyPort) {
      proxyPort[Comlink.releaseProxy]();
      proxyPort = undefined;
    }
    if (this.iFrameElement) {
      this.iFrameElement.remove();
    }
    spnWorker = undefined;
    UID = 0;
    Object.keys(this).forEach((key) => delete this[key]);
  }

  async onPlayStateChange(newPlayState) {
    if (this.currentPlayState === newPlayState) {
      return;
    }

    switch (newPlayState) {
      case "realtimePerformanceStarted": {
        log("event received: realtimePerformanceStarted")();
        this.currentPlayState = newPlayState;
        await this.initialize();
        if (this.csoundWorkerMain.startPromiz) {
          // hacky SAB timing fix when starting
          // eventually, replace this spaghetti with
          // private/internal event emitters
          const startPromiz = this.csoundWorkerMain.startPromiz;
          setTimeout(() => {
            startPromiz();
          }, 0);
          delete this.csoundWorkerMain.startPromiz;
        }

        break;
      }
      case "realtimePerformanceEnded": {
        log("event received: realtimePerformanceEnded")();
        if (window[`__csound_wasm_iframe_parent_${this.contextUid}Node`]) {
          window[`__csound_wasm_iframe_parent_${this.contextUid}Node`].disconnect();
          delete window[`__csound_wasm_iframe_parent_${this.contextUid}Node`].disconnect();
        }
        break;
      }
      default: {
        break;
      }
    }
    this.currentPlayState = newPlayState;
    if (
      proxyPort &&
      (newPlayState !== "realtimePerformanceStarted" || newPlayState !== "renderStarted")
    ) {
      await proxyPort.setPlayState({
        contextUid: this.contextUid,
        newPlayState,
      });
    }
  }

  async initIframe() {
    // HACK FROM (but it works just fine when adding modern security models)
    // https://github.com/GoogleChromeLabs/audioworklet-polyfill/blob/274792e5e3d189e04c9496bed24129118539b4b5/src/realm.js#L18-L20
    if (typeof window === "undefined" || typeof window.document === "undefined") {
      throw new TypeError("Can only run SPN in Browser scope");
    }

    const parentScope = window.document;
    // eslint-disable-next-line unicorn/prevent-abbreviations
    const iFrameHtml = [
      `<!doctype html>`,
      `<html lang="en">`,
      `<head>`,
      `</head>`,
      `<body>`,
      `<script type="text/javascript" src="${ScriptProcessorNodeWorker()}"></script>`,
      `</body>`,
    ].join("\n");

    // eslint-disable-next-line unicorn/prevent-abbreviations
    const iFrameBlob = new Blob([iFrameHtml], { type: "text/html" });
    this.iFrameElement = document.createElement("iframe");

    this.iFrameElement.src = URL.createObjectURL(iFrameBlob);
    this.iFrameElement.sandbox.add("allow-scripts", "allow-same-origin");

    this.iFrameElement.style.cssText = "position:absolute;left:0;top:-999px;width:1px;height:1px;";

    // appending early to have access to contentWindow
    // eslint-disable-next-line unicorn/prevent-abbreviations
    const iFrameOnLoad = new Promise((resolve) => {
      // eslint-disable-next-line unicorn/prefer-add-event-listener
      this.iFrameElement.onload = () => {
        resolve();
      };
    });

    parentScope.body.append(this.iFrameElement);

    try {
      await iFrameOnLoad;
    } catch (error) {
      console.error(error);
    }

    spnWorker = this.iFrameElement.contentWindow;
  }

  async initialize() {
    if (!spnWorker) {
      await this.initIframe();
      if (!spnWorker) {
        console.error("SPN FATAL: Couldn't create iFrame");
        return;
      }
    }

    const contextUid = `audioWorklet${UID}`;
    this.contextUid = contextUid;
    UID += 1;

    if (!proxyPort) {
      proxyPort = Comlink.wrap(Comlink.windowEndpoint(spnWorker));
    }

    if (!this.audioContext) {
      if (this.audioContextIsProvided) {
        console.error(`fatal: the provided AudioContext was undefined`);
      }
      this.audioContext = new (WebkitAudioContext())({ sampleRate: this.sampleRate });
    }
    if (this.audioContext.state === "closed") {
      if (this.audioContextIsProvided) {
        console.error(`fatal: the provided AudioContext was closed, falling back new AudioContext`);
      }
      this.audioContext = new (WebkitAudioContext())({ sampleRate: this.sampleRate });
    }

    if (this.sampleRate !== this.audioContext.sampleRate) {
      this.audioContext = new (WebkitAudioContext())({ sampleRate: this.sampleRate });
      // if this.audioContextIsProvided is true
      // it should already be picked
      if (this.audioContextIsProvided) {
        console.error("Internal error: sample rate was ignored from provided audioContext");
      }
    }

    // just set it both on parent and iframe
    // since 1 works on linux and other one on mac
    // leaking globals indeed
    spnWorker[contextUid] = this.audioContext;
    window[`__csound_wasm_iframe_parent_${contextUid}`] = this.audioContext;
    const { port1: mainMessagePort, port2: workerMessagePort } = new MessageChannel();

    let liveInput;
    if (this.isRequestingInput) {
      await new Promise((resolve) => {
        const microphoneCallback = (stream) => {
          if (stream) {
            liveInput = this.audioContext.createMediaStreamSource(stream);
          }
          resolve();
        };
        requestMicrophoneNode(microphoneCallback);
      });
    }

    await proxyPort.initialize(
      Comlink.transfer(
        {
          contextUid,
          hardwareBufferSize: 32768,
          softwareBufferSize: 2048,
          inputsCount: this.inputsCount,
          outputsCount: this.outputsCount,
          sampleRate: this.sampleRate,
          audioInputPort: this.ipcMessagePorts.audioWorkerAudioInputPort,
          messagePort: workerMessagePort,
          requestPort: this.ipcMessagePorts.audioWorkerFrameRequestPort,
          audioContextIsProvided: this.audioContextIsProvided,
          autoConnect: this.autoConnect,
          initialPlayState: this.currentPlayState,
        },
        [
          this.ipcMessagePorts.audioWorkerAudioInputPort,
          workerMessagePort,
          this.ipcMessagePorts.audioWorkerFrameRequestPort,
        ],
      ),
    );
    mainMessagePort.addEventListener("message", messageEventHandler(this));
    mainMessagePort.start();

    if (this.csoundWorkerMain && this.csoundWorkerMain.publicEvents) {
      const audioNode =
        spnWorker[`${contextUid}Node`] || window[`__csound_wasm_iframe_parent_${contextUid}Node`];
      audioNode && liveInput.connect(audioNode);

      if (
        audioNode &&
        this.csoundWorkerMain &&
        this.csoundWorkerMain.publicEvents &&
        this.csoundWorkerMain.publicEvents.triggerOnAudioNodeCreated
      ) {
        this.csoundWorkerMain.publicEvents.triggerOnAudioNodeCreated(audioNode);
      }
    }
    if (this.isRequestingMidi && this.csoundWorkerMain && this.csoundWorkerMain.handleMidiInput) {
      log("requesting for web-midi connection");
      requestMidi({
        onMidiMessage: this.csoundWorkerMain.handleMidiInput.bind(this.csoundWorkerMain),
      });
    }
  }
}

export default ScriptProcessorNodeMainThread;
