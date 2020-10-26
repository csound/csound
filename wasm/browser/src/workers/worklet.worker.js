import * as Comlink from 'comlink';
import { AUDIO_STATE, MAX_HARDWARE_BUFFER_SIZE } from '@root/constants';
import { instantiateAudioPacket } from '@root/workers/common.utils';
import { logWorklet } from '@root/logger';

const SAB_PERIODS = 3;
const VANILLA_PERIODS = 4;

const workerMessagePort = {
  ready: false,
  post: () => {},
  broadcastPlayState: () => {},
};

const audioFramePort = {
  requestFrames: () => {},
  ready: false,
};

const audioInputPort = {
  ready: false,
  transferInputFrames: undefined,
};

function processSharedArrayBuffer(inputs, outputs) {
  const isPerforming = Atomics.load(this.sharedArrayBuffer, AUDIO_STATE.IS_PERFORMING) === 1;
  const isPaused = Atomics.load(this.sharedArrayBuffer, AUDIO_STATE.IS_PAUSED) === 1;
  const isStopped = Atomics.load(this.sharedArrayBuffer, AUDIO_STATE.STOP) === 1;

  if (!this.sharedArrayBuffer || isPaused || !isPerforming || isStopped) {
    this.isPerformingLastTime = isPerforming;
    this.preProcessCount = 0;

    // Fix for that chrome 64 bug which doesn't 0 the arrays
    // https://github.com/csound/web-ide/issues/102#issuecomment-663894059
    (outputs[0] || []).forEach(array => array.fill(0));
    return true;
  }

  this.isPerformingLastTime = isPerforming;

  if (this.preProcessCount < SAB_PERIODS && this.isPerformingLastTime && isPerforming) {
    Atomics.store(this.sharedArrayBuffer, AUDIO_STATE.ATOMIC_NOFITY, 1);
    Atomics.notify(this.sharedArrayBuffer, AUDIO_STATE.ATOMIC_NOTIFY);
    this.preProcessCount += 1;
    return true;
  }

  const writeableInputChannels = inputs[0] || [];
  const writeableOutputChannels = outputs[0] || [];

  const hasWriteableInputChannels = writeableInputChannels.length > 0;
  const availableOutputBuffers = Atomics.load(this.sharedArrayBuffer, AUDIO_STATE.AVAIL_OUT_BUFS);

  if (availableOutputBuffers < this.softwareBufferSize * SAB_PERIODS) {
    Atomics.store(this.sharedArrayBuffer, AUDIO_STATE.ATOMIC_NOFITY, 1);
    Atomics.notify(this.sharedArrayBuffer, AUDIO_STATE.ATOMIC_NOTIFY);
  }

  const inputWriteIndex = Atomics.load(this.sharedArrayBuffer, AUDIO_STATE.INPUT_WRITE_INDEX);
  const outputReadIndex = Atomics.load(this.sharedArrayBuffer, AUDIO_STATE.OUTPUT_READ_INDEX);

  const nextInputWriteIndex = hasWriteableInputChannels
    ? (inputWriteIndex + writeableInputChannels[0].length) % this.hardwareBufferSize
    : 0;

  const nextOutputReadIndex =
    (outputReadIndex + writeableOutputChannels[0].length) % this.hardwareBufferSize;

  if (availableOutputBuffers > 0) {
    writeableOutputChannels.forEach((channelBuffer, channelIndex) => {
      channelBuffer.set(
        this.sabOutputChannels[channelIndex].subarray(
          outputReadIndex,
          nextOutputReadIndex < outputReadIndex ? this.hardwareBufferSize : nextOutputReadIndex
        )
      );
    });

    if (this.inputsCount > 0 && hasWriteableInputChannels && writeableInputChannels[0].length > 0) {
      writeableInputChannels.forEach((channelBuffer, channelIndex) => {
        this.sabInputChannels[channelIndex].set(channelBuffer, inputWriteIndex);
      });

      Atomics.store(this.sharedArrayBuffer, AUDIO_STATE.INPUT_WRITE_INDEX, nextInputWriteIndex);

      // increase availability of new input data
      Atomics.add(
        this.sharedArrayBuffer,
        AUDIO_STATE.AVAIL_IN_BUFS,
        writeableInputChannels[0].length
      );
    }

    Atomics.store(this.sharedArrayBuffer, AUDIO_STATE.OUTPUT_READ_INDEX, nextOutputReadIndex);

    // subtract the available output buffers, all channels are the same length
    Atomics.sub(
      this.sharedArrayBuffer,
      AUDIO_STATE.AVAIL_OUT_BUFS,
      writeableOutputChannels[0].length
    );
  } else {
    workerMessagePort.post('Buffer underrun');
  }

  return true;
}

function processVanillaBuffers(inputs, outputs) {
  if (!this.vanillaInitialized || !audioFramePort.ready) {
    if (audioFramePort.requestFrames && !this.vanillaInitialized) {
      // this minimizes startup glitches
      const firstTransferSize = this.softwareBufferSize * 4;
      audioFramePort.requestFrames({
        readIndex: 0,
        numFrames: firstTransferSize,
      });
      this.pendingFrames += firstTransferSize;
      this.vanillaInitialized = true;
      return true;
    } else if (!this.vanillaFirstTransferDone) {
      return true;
    }
  }

  const writeableInputChannels = inputs[0] || [];
  const writeableOutputChannels = outputs[0] || [];
  const hasWriteableInputChannels = writeableInputChannels.length > 0;

  const nextOutputReadIndex =
    (this.vanillaOutputReadIndex + writeableOutputChannels[0].length) % this.hardwareBufferSize;

  const nextInputReadIndex = hasWriteableInputChannels
    ? (this.vanillaInputReadIndex + writeableInputChannels[0].length) % this.hardwareBufferSize
    : 0;

  if (this.vanillaAvailableFrames >= writeableOutputChannels[0].length) {
    writeableOutputChannels.forEach((channelBuffer, channelIndex) => {
      channelBuffer.set(
        this.vanillaOutputChannels[channelIndex].subarray(
          this.vanillaOutputReadIndex,
          nextOutputReadIndex < this.vanillaOutputReadIndex
            ? this.hardwareBufferSize
            : nextOutputReadIndex
        )
      );
    });

    if (this.inputsCount > 0 && hasWriteableInputChannels && writeableInputChannels[0].length > 0) {
      const inputBufferLength = this.softwareBufferSize * VANILLA_PERIODS;
      writeableInputChannels.forEach((channelBuffer, channelIndex) => {
        this.vanillaInputChannels[channelIndex].set(channelBuffer, this.vanillaInputReadIndex);
      });
      if (nextInputReadIndex % inputBufferLength === 0) {
        const packet = [];
        const pastBufferBegin =
          (nextInputReadIndex === 0 ? this.hardwareBufferSize : nextInputReadIndex) -
          inputBufferLength;
        const thisBufferEnd =
          nextInputReadIndex === 0 ? this.hardwareBufferSize : nextInputReadIndex;
        this.vanillaInputChannels.forEach(channelBuffer => {
          packet.push(channelBuffer.subarray(pastBufferBegin, thisBufferEnd));
        });
        audioInputPort.transferInputFrames(packet);
      }
    }

    this.vanillaOutputReadIndex = nextOutputReadIndex;
    this.vanillaInputReadIndex = nextInputReadIndex;
    this.vanillaAvailableFrames -= writeableOutputChannels[0].length;
    this.bufferUnderrunCount = 0;
  } else {
    // minimize noise
    if (this.bufferUnderrunCount > 1 && this.bufferUnderrunCount < 12) {
      workerMessagePort.post('Buffer underrun');
      this.bufferUnderrunCount += 1;
    }

    if (this.bufferUnderrunCount === 100) {
      // 100 buffer Underruns in a row
      // means a fatal situation and browser
      // may crash
      workerMessagePort.post('FATAL: 100 buffers failed in a row');
      workerMessagePort.broadcastPlayState('realtimePerformanceEnded');
    }
  }

  if (
    this.vanillaAvailableFrames < this.softwareBufferSize * VANILLA_PERIODS &&
    this.pendingFrames < this.softwareBufferSize * VANILLA_PERIODS * 2
  ) {
    const futureOutputReadIndex =
      (this.vanillaAvailableFrames + nextOutputReadIndex + this.pendingFrames) %
      this.hardwareBufferSize;

    audioFramePort.requestFrames({
      readIndex:
        futureOutputReadIndex < this.hardwareBufferSize
          ? futureOutputReadIndex
          : futureOutputReadIndex + 1,
      numFrames: this.softwareBufferSize * VANILLA_PERIODS
    });
    this.pendingFrames += this.softwareBufferSize * VANILLA_PERIODS;
  }

  return true;
}

class CsoundWorkletProcessor extends AudioWorkletProcessor {
  constructor({
    processorOptions: {
      hardwareBufferSize,
      softwareBufferSize,
      inputsCount,
      outputsCount,
      sampleRate,
      maybeSharedArrayBuffer,
      maybeSharedArrayBufferAudioIn,
      maybeSharedArrayBufferAudioOut,
    },
  }) {
    super();
    this.currentPlayState = undefined;
    this.pause = this.pause.bind(this);
    this.resume = this.resume.bind(this);
    this.isPaused = false;

    this.sampleRate = sampleRate;
    this.inputsCount = inputsCount;
    this.outputsCount = outputsCount;
    this.hardwareBufferSize = hardwareBufferSize;
    this.softwareBufferSize = softwareBufferSize;
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
          new Float64Array(
            this.audioStreamIn,
            MAX_HARDWARE_BUFFER_SIZE * channelIndex,
            MAX_HARDWARE_BUFFER_SIZE
          )
        );
      }

      for (let channelIndex = 0; channelIndex < outputsCount; ++channelIndex) {
        this.sabOutputChannels.push(
          new Float64Array(
            this.audioStreamOut,
            MAX_HARDWARE_BUFFER_SIZE * channelIndex,
            MAX_HARDWARE_BUFFER_SIZE
          )
        );
      }
      this.actualProcess = processSharedArrayBuffer.bind(this);
    } else {
      // Bit more agressive buffering with vanilla
      this.hardwareBufferSize = MAX_HARDWARE_BUFFER_SIZE;
      this.vanillaOutputChannels = [];
      this.vanillaInputChannels = [];
      this.vanillaOutputReadIndex = 0;
      this.vanillaInputReadIndex = 0;
      this.vanillaAvailableFrames = 0;
      this.pendingFrames = 0;

      this.vanillaInitialized = false;
      this.vanillaFirstTransferDone = false;
      this.vanillaInputChannels = instantiateAudioPacket(inputsCount, MAX_HARDWARE_BUFFER_SIZE);
      this.vanillaOutputChannels = instantiateAudioPacket(outputsCount, MAX_HARDWARE_BUFFER_SIZE);

      this.actualProcess = processVanillaBuffers.bind(this);
      const updateVanillaFrames = this.updateVanillaFrames.bind(this);
      this.vanillaMessageHandler = this.vanillaMessageHandler.bind(this);
      const messageHandlerCallback = this.vanillaMessageHandler(updateVanillaFrames).bind(this);
      this.port.addEventListener('message', messageHandlerCallback);
      this.port.start();
    }

    Comlink.expose(this, this.port);
    logWorklet(`Worker thread was constructed`);
    logWorklet(
      JSON.stringify({
        sr: this.sampleRate,
        nchnls_i: this.inputsCount,
        nchnls: this.outputsCount,
        _B: this.hardwareBufferSize,
        _b: this.softwareBufferSize,
      })
    );
  }

  updateVanillaFrames({ audioPacket, numFrames, readIndex }) {
    // aways dec pending Frames even for empty ones
    this.pendingFrames -= numFrames;
    if (audioPacket) {
      for (let channelIndex = 0; channelIndex < this.outputsCount; ++channelIndex) {
        let hasLeftover = false;
        let framesLeft = numFrames;
        const nextReadIndex = readIndex % this.hardwareBufferSize;
        if (nextReadIndex < readIndex) {
          hasLeftover = true;
          framesLeft = this.hardwareBufferSize - readIndex;
        }

        this.vanillaOutputChannels[channelIndex].set(
          audioPacket[channelIndex].subarray(0, framesLeft),
          readIndex
        );

        if (hasLeftover) {
          this.vanillaOutputChannels[channelIndex].set(
            audioPacket[channelIndex].subarray(framesLeft)
          );
        }
      }
      this.vanillaAvailableFrames += numFrames;
      if (!this.vanillaFirstTransferDone) {
        this.vanillaFirstTransferDone = true;
      }
    }
  }

  vanillaMessageHandler(updateVanillaFrames) {
    logWorklet(`vanillaMessageHandler was assigned`);
    return event => {
      if (event.data.msg === 'initMessagePort') {
        logWorklet(`initMessagePort in worker`);
        const port = event.ports[0];
        workerMessagePort.post = log => port.postMessage({ log });
        workerMessagePort.broadcastPlayState = playStateChange =>
          port.postMessage({ playStateChange });
        workerMessagePort.ready = true;
      } else if (event.data.msg === 'initRequestPort') {
        logWorklet(`initRequestPort in worker`);
        const requestPort = event.ports[0];
        requestPort.addEventListener('message', requestPortEvent => {
          const { audioPacket, readIndex, numFrames } = requestPortEvent.data;
          updateVanillaFrames({ audioPacket, numFrames, readIndex });
        });
        audioFramePort.requestFrames = arguments_ => requestPort.postMessage(arguments_);

        if (!audioFramePort.ready) {
          requestPort.start();
          audioFramePort.ready = true;
        }
      } else if (event.data.msg === 'initAudioInputPort') {
        logWorklet(`initAudioInputPort in worker`);
        const inputPort = event.ports[0];
        audioInputPort.transferInputFrames = frames => inputPort.postMessage(frames);
      }
    };
  }

  pause() {
    if (!this.isPaused) {
      this.isPaused = true;
    }
  }

  resume() {
    if (this.isPaused) {
      this.isPaused = false;
    }
  }

  process(inputs, outputs) {
    if (this.isPaused) {
      return true;
    } else {
      return this.actualProcess(inputs, outputs);
    }
  }
}

registerProcessor('csound-worklet-processor', CsoundWorkletProcessor);
