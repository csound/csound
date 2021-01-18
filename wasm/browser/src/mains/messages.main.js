// exec log-event: msg => cb(msg)
export const messageEventHandler = (worker) => (event) => {
  if (event.data.log) {
    if (worker && worker.publicEvents && worker.publicEvents.triggerMessage) {
      worker.publicEvents.triggerMessage(event.data.log);
    } else {
      // in case of errors, this can happen
      // in which case, it's good to see the log
      console.log(event.data.log);
    }
  } else if (event.data.sabWorker && worker && worker.hasSharedArrayBuffer) {
    if (event.data.sabWorker === "unlocked" && typeof worker.startPromiz === "function") {
      worker.startPromiz();
    }
  } else if (event.data.playStateChange && worker && worker.onPlayStateChange) {
    worker.onPlayStateChange(event.data.playStateChange);
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
    const {
      port1: csoundWorkerFrameRequestPort,
      port2: audioWorkerFrameRequestPort,
    } = new MessageChannel();
    this.csoundWorkerFrameRequestPort = csoundWorkerFrameRequestPort;
    this.audioWorkerFrameRequestPort = audioWorkerFrameRequestPort;
    const {
      port1: csoundWorkerAudioInputPort,
      port2: audioWorkerAudioInputPort,
    } = new MessageChannel();
    this.csoundWorkerAudioInputPort = csoundWorkerAudioInputPort;
    this.audioWorkerAudioInputPort = audioWorkerAudioInputPort;
    const { port1: csoundWorkerRtMidiPort, port2: csoundMainRtMidiPort } = new MessageChannel();
    this.csoundWorkerRtMidiPort = csoundWorkerRtMidiPort;
    this.csoundMainRtMidiPort = csoundMainRtMidiPort;

    const { port1: sabWorkerCallbackReply, port2: sabMainCallbackReply } = new MessageChannel();
    this.sabWorkerCallbackReply = sabWorkerCallbackReply;
    this.sabMainCallbackReply = sabMainCallbackReply;
  }
}
