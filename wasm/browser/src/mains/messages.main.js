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

const iterableMessageChannel = () => {
  const { port1, port2 } = new MessageChannel();
  return [port1, port2];
};

const safelyClosePorts = ([p1, p2]) => {
  if (typeof p1.close !== "undefined") {
    try {
      p1.close();
      // eslint-disable unicorn/prefer-optional-catch-binding
    } catch (_) {}
  }
  if (typeof p2.close !== "undefined") {
    try {
      p2.close();
      // eslint-disable unicorn/prefer-optional-catch-binding
    } catch (_) {}
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

    // Methods
    this.restartMessagePortAudio = this.restartMessagePortAudio.bind(this);
    this.restartWorkerAudioInputPort = this.restartWorkerAudioInputPort.bind(this);
    this.restartWorkerFrameRequestPort = this.restartWorkerFrameRequestPort.bind(this);
    this.restartAudioInputPorts = this.restartAudioInputPorts.bind(this);
    this.restartRtMidiPorts = this.restartRtMidiPorts.bind(this);
  }

  restartMessagePortAudio() {
    safelyClosePorts([this.mainMessagePortAudio, this.workerMessagePortAudio]);
    [this.mainMessagePortAudio, this.workerMessagePortAudio] = iterableMessageChannel();
  }

  restartWorkerAudioInputPort() {
    safelyClosePorts([this.csoundWorkerAudioInputPort, this.audioWorkerAudioInputPort]);
    [this.csoundWorkerAudioInputPort, this.audioWorkerAudioInputPort] = iterableMessageChannel();
  }

  restartWorkerFrameRequestPort() {
    safelyClosePorts([this.csoundWorkerFrameRequestPort, this.audioWorkerFrameRequestPort]);
    [
      this.csoundWorkerFrameRequestPort,
      this.audioWorkerFrameRequestPort,
    ] = iterableMessageChannel();
  }

  restartAudioInputPorts() {
    safelyClosePorts([this.csoundWorkerAudioInputPort, this.audioWorkerAudioInputPort]);
    [this.csoundWorkerAudioInputPort, this.audioWorkerAudioInputPort] = iterableMessageChannel();
  }

  restartRtMidiPorts() {
    safelyClosePorts([this.csoundWorkerRtMidiPort, this.csoundMainRtMidiPort]);
    [this.csoundWorkerRtMidiPort, this.csoundMainRtMidiPort] = iterableMessageChannel();
  }

  restart(csoundWorkerMain) {
    this.restartMessagePortAudio();
    this.restartWorkerAudioInputPort();
    this.restartWorkerFrameRequestPort();
    this.restartAudioInputPorts();
    this.restartRtMidiPorts();

    this.mainMessagePort.addEventListener("message", messageEventHandler(csoundWorkerMain));
    this.mainMessagePortAudio.addEventListener("message", messageEventHandler(csoundWorkerMain));

    this.mainMessagePort.start();
    this.mainMessagePortAudio.start();
  }
}
