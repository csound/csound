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

export let { port1: mainMessagePort, port2: workerMessagePort } = new MessageChannel();

export let { port1: mainMessagePortAudio, port2: workerMessagePortAudio } = new MessageChannel();

export let {
  port1: csoundWorkerFrameRequestPort,
  port2: audioWorkerFrameRequestPort,
} = new MessageChannel();

export let {
  port1: csoundWorkerAudioInputPort,
  port2: audioWorkerAudioInputPort,
} = new MessageChannel();

export let { port1: csoundWorkerRtMidiPort, port2: csoundMainRtMidiPort } = new MessageChannel();

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

export const restartMessagePortAudio = () => {
  safelyClosePorts([mainMessagePortAudio, workerMessagePortAudio]);
  [mainMessagePortAudio, workerMessagePortAudio] = iterableMessageChannel();
};

export const restartWorkerAudioInputPort = () => {
  safelyClosePorts([csoundWorkerAudioInputPort, audioWorkerAudioInputPort]);
  [csoundWorkerAudioInputPort, audioWorkerAudioInputPort] = iterableMessageChannel();
};

export const restartWorkerFrameRequestPort = () => {
  safelyClosePorts([csoundWorkerFrameRequestPort, audioWorkerFrameRequestPort]);
  [csoundWorkerFrameRequestPort, audioWorkerFrameRequestPort] = iterableMessageChannel();
};

export const cleanupPorts = (csoundWorkerMain) => {
  logVAN(`cleanupPorts`);

  safelyClosePorts([mainMessagePort, workerMessagePort]);
  [mainMessagePort, workerMessagePort] = iterableMessageChannel();

  safelyClosePorts([mainMessagePortAudio, workerMessagePortAudio]);
  [mainMessagePortAudio, workerMessagePortAudio] = iterableMessageChannel();

  safelyClosePorts([csoundWorkerFrameRequestPort, audioWorkerFrameRequestPort]);
  [csoundWorkerFrameRequestPort, audioWorkerFrameRequestPort] = iterableMessageChannel();

  safelyClosePorts([csoundWorkerAudioInputPort, audioWorkerAudioInputPort]);
  [csoundWorkerAudioInputPort, audioWorkerAudioInputPort] = iterableMessageChannel();

  safelyClosePorts([csoundWorkerRtMidiPort, csoundMainRtMidiPort]);
  [csoundWorkerRtMidiPort, csoundMainRtMidiPort] = iterableMessageChannel();

  mainMessagePort.addEventListener("message", messageEventHandler(csoundWorkerMain));
  mainMessagePortAudio.addEventListener("message", messageEventHandler(csoundWorkerMain));

  mainMessagePort.start();
  mainMessagePortAudio.start();

  // csoundWorkerMain.csoundWorker.postMessage({ msg: 'initRequestPort' }, [
  //   csoundWorkerFrameRequestPort,
  // ]);
  // csoundWorkerMain.csoundWorker.postMessage({ msg: 'initAudioInputPort' }, [
  //   csoundWorkerAudioInputPort,
  // ]);
  // csoundWorkerMain.csoundWorker.postMessage({ msg: 'initRtMidiEventPort' }, [
  //   csoundWorkerRtMidiPort,
  // ]);
};
