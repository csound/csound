/* eslint-disable unicorn/require-post-message-target-origin */
import { expose } from "comlink/dist/esm/comlink.mjs";
import MessagePortState from "../utils/message-port-state";
import { AUDIO_STATE, RING_BUFFER_SIZE } from "../constants";
import { instantiateAudioPacket } from "./common.utils";
import { logWorkletWorker as log } from "../logger";

const VANILLA_INPUT_WRITE_BUFFER_LEN = 2048;

const activeNodes = new Map();

function processSharedArrayBuffer(inputs, outputs) {
  const isPerforming = Atomics.load(this.sharedArrayBuffer, AUDIO_STATE.IS_PERFORMING) === 1;
  const isPaused = Atomics.load(this.sharedArrayBuffer, AUDIO_STATE.IS_PAUSED) === 1;
  const isStopped = Atomics.load(this.sharedArrayBuffer, AUDIO_STATE.STOP) === 1;

  if (this.startPromiz) {
    this.startPromiz();
    delete this.startPromiz;
  }

  if (!this.sharedArrayBuffer || isPaused || !isPerforming || isStopped) {
    this.isPerformingLastTime = isPerforming;
    this.firstBufferReady = false;
    this.notifiedOnce = false;
    // this.preProcessCount = 0;

    // Fix for that chrome 64 bug which doesn't 0 the arrays
    // https://github.com/csound/web-ide/issues/102#issuecomment-663894059
    (outputs[0] || []).forEach((array) => array.fill(0));
    return true;
  }

  this.isPerformingLastTime = isPerforming;

  const writeableInputChannels = inputs && inputs[0];
  const writeableOutputChannels = outputs && outputs[0];
  const bufferLength = writeableOutputChannels[0].length;

  if (this.bufferLength !== bufferLength) {
    this.bufferLength = bufferLength;
    Atomics.store(this.sharedArrayBuffer, AUDIO_STATE.BUFFER_LEN, bufferLength);
  }

  const nextInputWriteIndex =
    writeableInputChannels && writeableInputChannels.length > 0
      ? (this.inputWriteIndex + bufferLength) % RING_BUFFER_SIZE
      : 0;

  const nextOutputReadIndex =
    writeableOutputChannels && writeableOutputChannels.length > 0
      ? (this.outputReadIndex + bufferLength) % RING_BUFFER_SIZE
      : 0;

  if (Atomics.load(this.sharedArrayBuffer, AUDIO_STATE.AVAIL_OUT_BUFS) >= bufferLength) {
    this.bufferUnderrunCount && (this.bufferUnderrunCount = 0);
    writeableOutputChannels.forEach((channelBuffer, channelIndex) => {
      // a simple set, where the len is the destination size and 2nd arg the offset

      channelBuffer.set(
        this.sabOutputChannels[channelIndex].subarray(
          this.outputReadIndex,
          nextOutputReadIndex < this.outputReadIndex ? RING_BUFFER_SIZE : nextOutputReadIndex,
        ),
      );
    });

    if (
      writeableInputChannels &&
      writeableInputChannels[0] &&
      writeableInputChannels[0].length > 0
    ) {
      writeableInputChannels.forEach((channelBuffer, channelIndex) => {
        this.sabInputChannels[channelIndex].set(channelBuffer, this.inputWriteIndex);
      });

      this.inputWriteIndex = nextInputWriteIndex;
      // Atomics.store(this.sharedArrayBuffer, AUDIO_STATE.INPUT_WRITE_INDEX, nextInputWriteIndex);

      // increase availability of new input data
      Atomics.add(
        this.sharedArrayBuffer,
        AUDIO_STATE.AVAIL_IN_BUFS,
        writeableInputChannels[0].length,
      );
    }

    this.outputReadIndex = nextOutputReadIndex;

    // subtract the available output buffers, all channels are the same length
    Atomics.sub(this.sharedArrayBuffer, AUDIO_STATE.AVAIL_OUT_BUFS, bufferLength);
    Atomics.store(this.sharedArrayBuffer, AUDIO_STATE.OUTPUT_READ_INDEX, this.outputReadIndex);
  } else {
    if (this.outputReadIndex > 4098) {
      console.log("buffer underrun");
    } else {
      // a not so pretty way to prevent buffer underrun messages
      // from being delivered before the first buffers are received
      return true;
    }

    this.bufferUnderrunCount += 1;
    if (this.bufferUnderrunCount === 100) {
      // 100 buffer Underruns in a row
      // means a fatal situation and browser
      // may crash
      this.workerMessagePort.post("FATAL: 100 buffers failed in a row");
      this.workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
      return false;
    }
  }

  return true;
}

function processVanillaBuffers(inputs, outputs) {
  if (!this.vanillaInitialized) {
    // this minimizes startup glitches
    const firstTransferSize = 8192;
    this.audioFramePort.requestFrames({
      readIndex: 0,
      numFrames: firstTransferSize,
    });
    this.pendingFrames += firstTransferSize;
    this.vanillaInitialized = true;
    if (this.startPromiz) {
      this.startPromiz();
      delete this.startPromiz;
    }
    return true;
  }

  if (!this.vanillaFirstTransferDone) {
    ((outputs && outputs[0]) || []).forEach((array) => array.fill(0));
    return true;
  }

  const writeableInputChannels = inputs && inputs[0];
  const writeableOutputChannels = outputs && outputs[0];
  const bufferLength = writeableOutputChannels ? writeableOutputChannels[0].length : 0;

  const nextOutputReadIndex =
    writeableOutputChannels && writeableOutputChannels.length > 0
      ? (this.vanillaOutputReadIndex + writeableOutputChannels[0].length) % RING_BUFFER_SIZE
      : 0;

  const nextInputReadIndex =
    writeableInputChannels && writeableInputChannels.length > 0
      ? (this.vanillaInputReadIndex + writeableInputChannels[0].length) % RING_BUFFER_SIZE
      : 0;

  if (bufferLength && this.vanillaAvailableFrames >= bufferLength) {
    writeableOutputChannels.forEach((channelBuffer, channelIndex) => {
      channelBuffer.set(
        this.vanillaOutputChannels[channelIndex].subarray(
          this.vanillaOutputReadIndex,
          nextOutputReadIndex < this.vanillaOutputReadIndex
            ? RING_BUFFER_SIZE
            : nextOutputReadIndex,
        ),
      );
    });

    if (writeableInputChannels && writeableInputChannels.length > 0) {
      writeableInputChannels.forEach((channelBuffer, channelIndex) => {
        this.vanillaInputChannels[channelIndex].set(channelBuffer, this.vanillaInputReadIndex);
      });
      if (nextInputReadIndex % VANILLA_INPUT_WRITE_BUFFER_LEN === 0) {
        const packet = [];
        const pastBufferBegin =
          (nextInputReadIndex === 0 ? RING_BUFFER_SIZE : nextInputReadIndex) -
          VANILLA_INPUT_WRITE_BUFFER_LEN;
        const thisBufferEnd = nextInputReadIndex === 0 ? RING_BUFFER_SIZE : nextInputReadIndex;
        this.vanillaInputChannels.forEach((channelBuffer) => {
          packet.push(channelBuffer.subarray(pastBufferBegin, thisBufferEnd));
        });
        this.audioInputPort.transferInputFrames(packet);
      }
    }

    this.vanillaOutputReadIndex = nextOutputReadIndex;
    this.vanillaInputReadIndex = nextInputReadIndex;
    this.vanillaAvailableFrames -= bufferLength;
    this.bufferUnderrunCount = 0;
  } else {
    // minimize noise
    if (this.bufferUnderrunCount > 1 && this.bufferUnderrunCount < 12) {
      this.workerMessagePort.post("Buffer underrun");
      this.bufferUnderrunCount += 1;
    }

    if (this.bufferUnderrunCount === 100) {
      // 100 buffer Underruns in a row
      // means a fatal situation and browser
      // may crash
      this.workerMessagePort.post("FATAL: 100 buffers failed in a row");
      this.workerMessagePort.broadcastPlayState("realtimePerformanceEnded");
      return false;
    }
  }

  // 2048 is max buffer
  const framesRequest = 2048 - this.vanillaAvailableFrames;
  if (framesRequest > 0) {
    const futureOutputReadIndex =
      (this.vanillaAvailableFrames + nextOutputReadIndex + this.pendingFrames) % RING_BUFFER_SIZE;

    this.audioFramePort.requestFrames({
      readIndex: futureOutputReadIndex,
      numFrames: framesRequest,
    });
    this.pendingFrames += framesRequest;
  }

  return true;
}

class CsoundWorkletProcessor extends AudioWorkletProcessor {
  constructor({
    processorOptions: {
      contextUid,
      inputsCount,
      outputsCount,
      ksmps,
      // sampleRate,
      maybeSharedArrayBuffer,
      maybeSharedArrayBufferAudioIn,
      maybeSharedArrayBufferAudioOut,
    },
  }) {
    super();
    const nodeUid = `${contextUid}Node`;
    activeNodes.set(nodeUid, this);
    this.messagePortsReady = false;
    this.currentPlayState = undefined;
    this.pause = this.pause.bind(this);
    this.resume = this.resume.bind(this);
    this.isPaused = false;
    // this.sampleRate = sampleRate;
    this.ksmps = ksmps;
    this.inputsCount = inputsCount;
    this.outputsCount = outputsCount;
    this.inputWriteIndex = 0;
    this.outputReadIndex = 0;
    this.bufferUnderrunCount = 0;

    // NON-SAB PROCESS
    this.isPerformingLastTime = false;
    this.preProcessCount = 0;

    if (maybeSharedArrayBuffer) {
      this.sharedArrayBuffer = maybeSharedArrayBuffer;
      this.audioStreamIn = maybeSharedArrayBufferAudioIn;
      this.audioStreamOut = maybeSharedArrayBufferAudioOut;
      this.sabOutputChannels = [];
      this.sabInputChannels = [];

      for (let channelIndex = 0; channelIndex < inputsCount; ++channelIndex) {
        this.sabInputChannels.push(
          new Float64Array(this.audioStreamIn, RING_BUFFER_SIZE * channelIndex, RING_BUFFER_SIZE),
        );
      }

      for (let channelIndex = 0; channelIndex < outputsCount; ++channelIndex) {
        this.sabOutputChannels.push(
          new Float64Array(this.audioStreamOut, RING_BUFFER_SIZE * channelIndex, RING_BUFFER_SIZE),
        );
      }
      this.actualProcess = processSharedArrayBuffer.bind(this);
    } else {
      this.vanillaOutputChannels = [];
      this.vanillaInputChannels = [];
      this.vanillaOutputReadIndex = 0;
      this.vanillaInputReadIndex = 0;
      this.vanillaAvailableFrames = 0;
      this.pendingFrames = 0;

      this.vanillaInitialized = false;
      this.vanillaFirstTransferDone = false;
      this.minBufferSize = 4096;
      this.vanillaInputChannels = instantiateAudioPacket(inputsCount, RING_BUFFER_SIZE);
      this.vanillaOutputChannels = instantiateAudioPacket(outputsCount, RING_BUFFER_SIZE);

      this.actualProcess = processVanillaBuffers.bind(this);
      this.updateVanillaFrames = this.updateVanillaFrames.bind(this);
    }
    expose({ initialize, pause: this.pause, resume: this.resume }, this.port);
    log(`Worker thread was constructed`)();
  }

  initCallbacks({ workerMessagePort, audioInputPort, audioFramePort, startPromiz }) {
    log(`initCallbacks in worker`)();
    if (workerMessagePort) {
      this.workerMessagePort = workerMessagePort;
    }

    if (audioInputPort) {
      this.audioInputPort = audioInputPort;
    }
    if (audioFramePort) {
      this.audioFramePort = audioFramePort;
    }
    this.messagePortsReady = true;
    this.startPromiz = startPromiz;
  }

  updateVanillaFrames({ audioPacket, numFrames, readIndex }) {
    // aways dec pending Frames even for empty ones
    this.pendingFrames -= numFrames;
    if (audioPacket) {
      for (let channelIndex = 0; channelIndex < this.outputsCount; ++channelIndex) {
        let hasLeftover = false;
        let framesLeft;
        const nextReadIndex = (readIndex + numFrames) % RING_BUFFER_SIZE;
        if (nextReadIndex < readIndex) {
          hasLeftover = true;
          framesLeft = RING_BUFFER_SIZE - readIndex;
        }

        if (hasLeftover) {
          this.vanillaOutputChannels[channelIndex].set(
            audioPacket[channelIndex].subarray(0, framesLeft),
            readIndex,
          );
          this.vanillaOutputChannels[channelIndex].set(
            audioPacket[channelIndex].subarray(framesLeft),
            0,
          );
        } else {
          this.vanillaOutputChannels[channelIndex].set(audioPacket[channelIndex], readIndex);
        }
      }
      this.vanillaAvailableFrames += numFrames;
      if (!this.vanillaFirstTransferDone) {
        this.vanillaFirstTransferDone = true;
      }
    }
  }

  pause() {
    this.isPaused = true;
    this.workerMessagePort.broadcastPlayState("realtimePerformancePaused");
  }

  resume() {
    this.isPaused = false;
    this.workerMessagePort.broadcastPlayState("realtimePerformanceResumed");
  }

  process(inputs, outputs) {
    return this.isPaused || !this.messagePortsReady ? true : this.actualProcess(inputs, outputs);
  }
}

function initMessagePort({ port }) {
  log(`initMessagePort in worker`)();
  const workerMessagePort = new MessagePortState();
  workerMessagePort.post = (logMessage) => port.postMessage({ log: logMessage });
  workerMessagePort.broadcastPlayState = (playStateChange) => port.postMessage({ playStateChange });

  workerMessagePort.ready = true;
  return workerMessagePort;
}

function initRequestPort({ requestPort, audioNode }) {
  log(`initRequestPort in worker`)();
  requestPort.addEventListener("message", (requestPortEvent) => {
    const { audioPacket, readIndex, numFrames } = requestPortEvent.data;
    audioNode.updateVanillaFrames({ audioPacket, numFrames, readIndex });
  });
  const requestFrames = (arguments_) => requestPort.postMessage(arguments_);

  requestPort.start();
  return {
    requestFrames,
    ready: true,
  };
}

function initAudioInputPort({ inputPort }) {
  log(`initAudioInputPort in worker`)();
  return {
    ready: false,
    transferInputFrames: (frames) => inputPort.postMessage(frames),
  };
}

const initialize = async ({ contextUid, inputPort, messagePort, requestPort }) => {
  const nodeUid = `${contextUid}Node`;
  const audioNode = activeNodes.get(nodeUid);
  const workerMessagePort = initMessagePort({ port: messagePort });

  const audioInputPort = initAudioInputPort({ inputPort });
  const audioFramePort = initRequestPort({ requestPort, audioNode });
  let startPromiz;
  const startPromise = new Promise((resolve) => {
    startPromiz = resolve;
  });
  audioNode.initCallbacks({ workerMessagePort, audioInputPort, audioFramePort, startPromiz });
  await startPromise;
};

registerProcessor("csound-worklet-processor", CsoundWorkletProcessor);
