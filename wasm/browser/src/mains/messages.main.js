import { logVAN } from "@root/logger";

const loggerPool = new Set();

// debug mode: console.log always all messages
if (!__PROD__) {
  loggerPool.add(console.log);
}

// exec log-event: msg => cb(msg)
export const messageEventHandler = (worker) => (event) => {
  if (event.data.log) {
    loggerPool.forEach((callback) => callback(event.data.log));
    (worker.messageCallbacks || []).forEach((callback) => callback(event.data.log));
  } else {
    worker.onPlayStateChange(event.data.playStateChange);
  }
};

export const emitInternalCsoundLogEvent = (worker, message) => {
  if (typeof message === "string") {
    loggerPool.forEach((callback) => callback(message));
    if (worker) {
      (worker.messageCallbacks || []).forEach((callback) => callback(message));
    }
  }
};

export class IPCMessagePorts {
  constructor() {
    let { port1: mainMessagePort, port2: workerMessagePort } = new MessageChannel();
    this.mainMessagePort = mainMessagePort;
    this.workerMessagePort = workerMessagePort;
    let { port1: mainMessagePortAudio, port2: workerMessagePortAudio } = new MessageChannel();
    this.mainMessagePortAudio = mainMessagePortAudio;
    this.workerMessagePortAudio = workerMessagePortAudio;
    let {
      port1: csoundWorkerFrameRequestPort,
      port2: audioWorkerFrameRequestPort,
    } = new MessageChannel();
    this.csoundWorkerFrameRequestPort = csoundWorkerFrameRequestPort;
    this.audioWorkerFrameRequestPort = audioWorkerFrameRequestPort;
    let {
      port1: csoundWorkerAudioInputPort,
      port2: audioWorkerAudioInputPort,
    } = new MessageChannel();
    this.csoundWorkerAudioInputPort = csoundWorkerAudioInputPort;
    this.audioWorkerAudioInputPort = audioWorkerAudioInputPort;
    let { port1: csoundWorkerRtMidiPort, port2: csoundMainRtMidiPort } = new MessageChannel();
    this.csoundWorkerRtMidiPort = csoundWorkerRtMidiPort;
    this.csoundMainRtMidiPort = csoundMainRtMidiPort;

    let { port1: sabWorkerCallbackReply, port2: sabMainCallbackReply } = new MessageChannel();
    this.sabWorkerCallbackReply = sabWorkerCallbackReply;
    this.sabMainCallbackReply = sabMainCallbackReply;
  }
}
