import * as Comlink from "comlink";
import { instantiateAudioPacket } from "@root/workers/common.utils";
import MessagePortState from "@utils/message-port-state";
import {newAudioContext} from "@utils/new-audio-context"
// https://github.com/xpl/ololog/issues/20
// import { logSPN } from '@root/logger';
import { range } from "ramda";

const getAudioContext = (contextUid) => {
  return window[contextUid] || window.parent["__csound_wasm_iframe_parent_" + contextUid] || newAudioContext();
}

const getAudioNode = (contextUid) => {
  return window[contextUid + "Node"] || window.parent["__csound_wasm_iframe_parent_" + contextUid + "Node"];
}

const setAudioNode = (contextUid, audioNode) => {
  if (window[contextUid]) {
    window[contextUid + "Node"] = audioNode;
  } else {
    window.parent["__csound_wasm_iframe_parent_" + contextUid + "Node"] = audioNode;
  }
}

const PERIODS = 4;
const spnInstances = new Map();

class CsoundScriptNodeProcessor {
  constructor({
    audioContext,
    contextUid,
    hardwareBufferSize,
    softwareBufferSize,
    inputsCount,
    outputsCount,
    sampleRate,
    audioContextIsProvided,
    autoConnect,
  }) {
    this.autoConnect = autoConnect;
    this.audioContextIsProvided = audioContextIsProvided;
    this.hardwareBufferSize = hardwareBufferSize;
    this.softwareBufferSize = softwareBufferSize;
    this.inputsCount = inputsCount;
    this.outputsCount = outputsCount;
    this.sampleRate = sampleRate;

    this.vanillaOutputChannels = [];
    this.vanillaInputChannels = [];
    this.vanillaOutputReadIndex = 0;
    this.vanillaInputReadIndex = 0;
    this.vanillaAvailableFrames = 0;
    this.pendingFrames = 0;

    this.vanillaInitialized = false;
    this.vanillaFirstTransferDone = false;
    this.vanillaInputChannels = instantiateAudioPacket(inputsCount, hardwareBufferSize);
    this.vanillaOutputChannels = instantiateAudioPacket(outputsCount, hardwareBufferSize);

    this.audioContext = audioContext;
    this.contextUid = contextUid;
    // this.nodeUid = `${contextUid}Node`;
    this.scriptNode = this.audioContext.createScriptProcessor(
      this.softwareBufferSize,
      inputsCount,
      outputsCount,
    );
    this.process = this.process.bind(this);
    const processor = this.process.bind(this);
    this.scriptNode.onaudioprocess = processor;

    if (this.autoConnect) {
      setAudioNode(contextUid, this.scriptNode.connect(this.audioContext.destination));
    } else {
      setAudioNode(contextUid, this.scriptNode.context.destination);
    }

    this.updateVanillaFrames = this.updateVanillaFrames.bind(this);
    this.initCallbacks = this.initCallbacks.bind(this);
  }

  initCallbacks({ workerMessagePort, transferInputFrames, requestPort }) {
    this.workerMessagePort = workerMessagePort;

    this.transferInputFrames = transferInputFrames;
    this.requestPort = requestPort;

    // Safari autoplay cancer :(
    if (this.audioContext.state === "suspended") {
      this.workerMessagePort.broadcastPlayState("realtimePerformancePaused");
      // this.workerMessagePort.vanillaWorkerState = "realtimePerformancePaused";
    }
  }

  updateVanillaFrames({ audioPacket, numFrames, readIndex }) {
    // aways dec pending Frames even for empty ones
    this.pendingFrames -= numFrames;
    if (audioPacket) {
      for (let channelIndex = 0; channelIndex < this.outputsCount; ++channelIndex) {
        let hasLeftover = false;
        let framesLeft = numFrames;
        const nextReadIndex = (readIndex + numFrames) % this.hardwareBufferSize;
        if (nextReadIndex < readIndex) {
          hasLeftover = true;
          framesLeft = this.hardwareBufferSize - readIndex;
        }

        this.vanillaOutputChannels[channelIndex].set(
          audioPacket[channelIndex].subarray(0, framesLeft),
          readIndex,
        );

        if (hasLeftover) {
          this.vanillaOutputChannels[channelIndex].set(
            audioPacket[channelIndex].subarray(framesLeft),
          );
        }
      }
      this.vanillaAvailableFrames += numFrames;
    }
    if (!this.vanillaFirstTransferDone) {
      this.vanillaFirstTransferDone = true;
      this.workerMessagePort.broadcastPlayState("realtimePerformanceStarted");
    }
  }

  // try to do the same here as in Vanilla+Worklet
  process({ inputBuffer, outputBuffer }) {
    if (this.workerMessagePort.vanillaWorkerState === "realtimePerformanceEnded") {
      setPlayState({ contextUid: this.contextUid, newPlayState: "realtimePerformanceEnded" });
      return true;
    }

    if (!this.vanillaInitialized) {
      // this minimizes startup glitches
      const firstTransferSize = this.softwareBufferSize * PERIODS;

      this.requestPort.postMessage({
        readIndex: 0,
        numFrames: firstTransferSize,
      });

      this.pendingFrames += firstTransferSize;
      this.vanillaInitialized = true;
      return true;
    }

    if (!this.vanillaFirstTransferDone) {
      return true;
    }

    const writeableInputChannels = range(0, this.inputsCount).map((index) =>
      inputBuffer.getChannelData(index),
    );
    const writeableOutputChannels = range(0, this.outputsCount).map((index) =>
      outputBuffer.getChannelData(index),
    );

    const hasWriteableInputChannels = writeableInputChannels.length > 0;

    const nextOutputReadIndex =
      (this.vanillaOutputReadIndex + writeableOutputChannels[0].length) % this.hardwareBufferSize;

    const nextInputReadIndex = hasWriteableInputChannels
      ? (this.vanillaInputReadIndex + writeableInputChannels[0].length) % this.hardwareBufferSize
      : 0;

    if (
      this.workerMessagePort.vanillaWorkerState !== "realtimePerformanceStarted" &&
      this.workerMessagePort.vanillaWorkerState !== "realtimePerformanceResumed"
    ) {
      writeableOutputChannels.forEach((channelBuffer) => {
        channelBuffer.fill(0);
      });
      return true;
    } else if (this.vanillaAvailableFrames >= writeableOutputChannels[0].length) {
      writeableOutputChannels.forEach((channelBuffer, channelIndex) => {
        channelBuffer.set(
          this.vanillaOutputChannels[channelIndex].subarray(
            this.vanillaOutputReadIndex,
            nextOutputReadIndex < this.vanillaOutputReadIndex
              ? this.hardwareBufferSize
              : nextOutputReadIndex,
          ),
        );
      });

      if (
        this.inputsCount > 0 &&
        hasWriteableInputChannels &&
        writeableInputChannels[0].length > 0
      ) {
        const inputBufferLength = this.softwareBufferSize * PERIODS;
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
          this.vanillaInputChannels.forEach((channelBuffer) => {
            packet.push(channelBuffer.subarray(pastBufferBegin, thisBufferEnd));
          });
          this.transferInputFrames(packet);
        }
      }

      this.vanillaOutputReadIndex = nextOutputReadIndex;
      this.vanillaInputReadIndex = nextInputReadIndex;
      this.vanillaAvailableFrames -= writeableOutputChannels[0].length;
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
      }
    }

    if (
      this.vanillaAvailableFrames < this.softwareBufferSize * PERIODS &&
      this.pendingFrames < this.softwareBufferSize * PERIODS
    ) {
      const futureOutputReadIndex =
        (this.vanillaAvailableFrames + nextOutputReadIndex + this.pendingFrames) %
        this.hardwareBufferSize;

      this.requestPort.postMessage.call(this.requestPort, {
        readIndex: futureOutputReadIndex,
        numFrames: this.softwareBufferSize * PERIODS,
      });
      this.pendingFrames += this.softwareBufferSize * PERIODS;
    }

    return true;
  }
}
const initAudioInputPort = ({ audioInputPort }) => (frames) => audioInputPort.postMessage(frames);

const initMessagePort = ({ port, initialPlayState }) => {
  const workerMessagePort = new MessagePortState();
  workerMessagePort.post = (log) => port.postMessage({ log });
  workerMessagePort.broadcastPlayState = (playStateChange) => port.postMessage({ playStateChange });
  workerMessagePort.ready = true;
  workerMessagePort.vanillaWorkerState = initialPlayState;
  return workerMessagePort;
};

const initRequestPort = ({ requestPort, spnClassInstance }) => {
  requestPort.addEventListener("message", (requestPortEvent) => {
    const { audioPacket, readIndex, numFrames } = requestPortEvent.data;
    spnClassInstance.updateVanillaFrames({ audioPacket, numFrames, readIndex });
  });
  requestPort.start();
  return requestPort;
};

const setPlayState = ({ contextUid, newPlayState }) => {
  const spnClassInstance = spnInstances.get(contextUid);
  if (!spnClassInstance) {
    return;
  }

  // unclear I know, but it's just here to imperatively
  // resume with audioplay policy in mind
  if (newPlayState === "resume") {
    spnClassInstance.audioContext.state === "suspended" && spnClassInstance.audioContext.resume();
    if (spnClassInstance.audioContext.state === "running") {
      spnClassInstance.workerMessagePort.broadcastPlayState("realtimePerformanceResumed");
    }
    return;
  }
  if (!spnClassInstance.workerMessagePort) {
    // perhaps we are rendering, so this is just ignored
    return;
  }
  if (
    newPlayState === "realtimePerformanceEnded" &&
    spnClassInstance.workerMessagePort.vanillaWorkerState !== "realtimePerformanceEnded"
  ) {
    // ping-pong
    spnClassInstance.workerMessagePort.broadcastPlayState("realtimePerformanceEnded");

    if (window[`${contextUid}Node`]) {
      if (spnClassInstance.autoConnect) {
        window[`${contextUid}Node`].disconnect();
      }
      window[`${contextUid}Node`] = undefined;
    }
    if (window[contextUid]) {
      if (spnClassInstance.autoConnect && !spnClassInstance.audioContextIsProvided) {
        window[contextUid].close();
      }
    }
    spnInstances.delete(contextUid);
    window[contextUid] = undefined;
  } else if (newPlayState === "realtimePerformanceResumed") {
    spnClassInstance.audioContext.state === "suspended" && spnClassInstance.audioContext.resume();
  }
  spnClassInstance.workerMessagePort.vanillaWorkerState = newPlayState;
};

const initialize = async ({
  contextUid,
  hardwareBufferSize,
  softwareBufferSize,
  inputsCount,
  outputsCount,
  sampleRate,
  audioInputPort,
  messagePort,
  requestPort,
  audioContextIsProvided,
  autoConnect,
  initialPlayState,
}) => {
  let audioContext = getAudioContext(contextUid);

  const spnClassInstance = new CsoundScriptNodeProcessor({
    audioContext,
    contextUid,
    hardwareBufferSize,
    softwareBufferSize,
    inputsCount,
    outputsCount,
    sampleRate,
    audioContextIsProvided,
    autoConnect,
  });

  const workerMessagePort = initMessagePort({ port: messagePort, initialPlayState });
  const transferInputFrames = initAudioInputPort({ audioInputPort, spnClassInstance });
  initRequestPort({ requestPort, spnClassInstance });
  spnClassInstance.initCallbacks({ workerMessagePort, transferInputFrames, requestPort });
  spnInstances.set(contextUid, spnClassInstance);
};

Comlink.expose({ initialize, setPlayState }, Comlink.windowEndpoint(window.parent));
