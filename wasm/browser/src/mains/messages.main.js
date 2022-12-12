// exec log-event: msg => cb(msg)
export const messageEventHandler = (worker) => (event) => {
  if (event.data.log) {
    if (worker && worker.publicEvents && worker.publicEvents.triggerMessage) {
      worker.publicEvents.triggerMessage(event.data.log);
    } else {
      // in case of errors, this can happen
      // in which case, it's good to see the log
      console.log(
        typeof event.data.log === "object" && typeof event.data.log.log === "string"
          ? event.data.log.log
          : event.data.log,
      );
    }
  } else if (event.data.playStateChange && worker && worker.onPlayStateChange) {
    worker.onPlayStateChange(event.data.playStateChange);
  }
};

const iterableMessageChannel = () => {
  const { port1, port2 } = new MessageChannel();
  return [port1, port2];
};

const safelyClosePorts = ([p1, p2]) => {
  if (p1.close !== undefined) {
    try {
      p1.close();
      // eslint-disable unicorn/prefer-optional-catch-binding
    } catch (_) {}
  }
  if (p2.close !== undefined) {
    try {
      p2.close();
      // eslint-disable unicorn/prefer-optional-catch-binding
    } catch (_) {}
  }
};

export class IPCMessagePorts {
  constructor() {
    const { port1: mainMessagePort, port2: workerMessagePort } = new MessageChannel();
    this.mainMessagePort = mainMessagePort;
    this.workerMessagePort = workerMessagePort;
    const { port1: mainMessagePortAudio, port2: workerMessagePortAudio } = new MessageChannel();
    this.mainMessagePortAudio = mainMessagePortAudio;
    this.workerMessagePortAudio = workerMessagePortAudio;
    const { port1: csoundWorkerFrameRequestPort, port2: audioWorkerFrameRequestPort } =
      new MessageChannel();
    this.csoundWorkerFrameRequestPort = csoundWorkerFrameRequestPort;
    this.audioWorkerFrameRequestPort = audioWorkerFrameRequestPort;
    const { port1: csoundWorkerAudioInputPort, port2: audioWorkerAudioInputPort } =
      new MessageChannel();
    this.csoundWorkerAudioInputPort = csoundWorkerAudioInputPort;
    this.audioWorkerAudioInputPort = audioWorkerAudioInputPort;
    const { port1: csoundWorkerRtMidiPort, port2: csoundMainRtMidiPort } = new MessageChannel();
    this.csoundWorkerRtMidiPort = csoundWorkerRtMidiPort;
    this.csoundMainRtMidiPort = csoundMainRtMidiPort;

    const { port1: sabWorkerCallbackReply, port2: sabMainCallbackReply } = new MessageChannel();
    this.sabWorkerCallbackReply = sabWorkerCallbackReply;
    this.sabMainCallbackReply = sabMainCallbackReply;

    // old-spn worker-port
    const { port1: mainMessagePort2, port2: workerMessagePort2 } = new MessageChannel();
    this.mainMessagePort2 = mainMessagePort2;
    this.workerMessagePort2 = workerMessagePort2;

    this.restartAudioWorkerPorts = this.restartAudioWorkerPorts.bind(this);
  }

  restartAudioWorkerPorts() {
    safelyClosePorts([this.csoundWorkerAudioInputPort, this.audioWorkerAudioInputPort]);
    [this.csoundWorkerAudioInputPort, this.audioWorkerAudioInputPort] = iterableMessageChannel();

    safelyClosePorts([this.mainMessagePortAudio, this.workerMessagePortAudio]);
    [this.mainMessagePortAudio, this.workerMessagePortAudio] = iterableMessageChannel();

    safelyClosePorts([this.csoundWorkerFrameRequestPort, this.audioWorkerFrameRequestPort]);
    [this.csoundWorkerFrameRequestPort, this.audioWorkerFrameRequestPort] =
      iterableMessageChannel();

    safelyClosePorts([this.mainMessagePort2, this.workerMessagePort2]);
    [this.mainMessagePort2, this.workerMessagePort2] = iterableMessageChannel();
  }
}
