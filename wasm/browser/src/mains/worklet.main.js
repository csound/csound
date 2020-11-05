import * as Comlink from "comlink";
import WorkletWorker from "@root/workers/worklet.worker";
import log, { logWorklet } from "@root/logger";

const connectedMidiDevices = new Set();

class AudioWorkletMainThread {
  constructor() {
    this.ipcMessagePorts = undefined;
    this.audioCtx = undefined;
    this.audioWorkletNode = undefined;
    this.currentPlayState = undefined;
    this.csoundWorkerMain = undefined;
    this.workletProxy = undefined;

    // never default these, get it from
    // csound-worker before starting
    this.sampleRate = undefined;
    this.inputsCount = undefined;
    this.outputsCount = undefined;
    this.hardwareBufferSize = undefined;
    this.softwareBufferSize = undefined;

    this.initialize = this.initialize.bind(this);
    this.connectPorts = this.connectPorts.bind(this);
    this.onPlayStateChange = this.onPlayStateChange.bind(this);
    logWorklet("AudioWorkletMainThread was constructed");
  }

  async onPlayStateChange(newPlayState) {
    this.currentPlayState = newPlayState;
    switch (newPlayState) {
      case "realtimePerformanceStarted": {
        logWorklet("event received: realtimePerformanceStarted");
        await this.initialize();
        break;
      }
      case "realtimePerformanceEnded": {
        logWorklet(
          "event received: realtimePerformanceEnded" + !this.csoundWorkerMain.hasSharedArrayBuffer
            ? ` cleaning up ports`
            : "",
        );
        this.audioCtx.close();
        this.audioWorkletNode.disconnect();
        delete this.audioWorkletNode;
        this.audioCtx = undefined;
        this.currentPlayState = undefined;
        this.workletProxy = undefined;
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
  }

  // SAB bypasses this mechanism!
  connectPorts() {
    logWorklet("initializing MessagePort on worker threads");

    this.audioWorkletNode.port.postMessage({ msg: "initMessagePort" }, [
      this.ipcMessagePorts.workerMessagePortAudio,
    ]);

    this.audioWorkletNode.port.postMessage({ msg: "initAudioInputPort" }, [
      this.ipcMessagePorts.audioWorkerAudioInputPort,
    ]);

    this.audioWorkletNode.port.postMessage({ msg: "initRequestPort" }, [
      this.ipcMessagePorts.audioWorkerFrameRequestPort,
    ]);

    try {
      logWorklet("wrapping Comlink proxy endpoint on the audioWorkletNode.port");
      this.workletProxy = Comlink.wrap(this.audioWorkletNode.port);
    } catch (error) {
      log.error("COMLINK ERROR", error);
    }
  }

  async initialize() {
    const newAudioContext = new AudioContext({
      latencyHint: "interactive",
      sampleRate: this.sampleRate,
    });

    this.audioCtx = newAudioContext;

    logWorklet("new AudioContext");
    await newAudioContext.audioWorklet.addModule(WorkletWorker());
    logWorklet("WorkletWorker module added");

    if (!this.csoundWorkerMain) {
      log.error(`fatal: worker not reachable from worklet-main thread`);
      return;
    }

    const createWorkletNode = (audoContext, inputsCount) => {
      return new AudioWorkletNode(audoContext, "csound-worklet-processor", {
        inputChannelCount: inputsCount ? [inputsCount] : 0,
        outputChannelCount: [this.outputsCount || 2],
        processorOptions: {
          hardwareBufferSize: this.hardwareBufferSize,
          softwareBufferSize: this.softwareBufferSize,
          isRequestingInput: this.isRequestingInput,
          inputsCount,
          outputsCount: this.outputsCount,
          sampleRate: this.sampleRate,
          maybeSharedArrayBuffer:
            this.csoundWorkerMain.hasSharedArrayBuffer && this.csoundWorkerMain.audioStatePointer,
          maybeSharedArrayBufferAudioIn:
            this.csoundWorkerMain.hasSharedArrayBuffer && this.csoundWorkerMain.audioStreamIn,
          maybeSharedArrayBufferAudioOut:
            this.csoundWorkerMain.hasSharedArrayBuffer && this.csoundWorkerMain.audioStreamOut,
        },
      });
    };

    if (this.isRequestingMidi) {
      emitInternalCsoundLogEvent(this.csoundWorkerMain, "requesting for web-midi connection");
      if (navigator && navigator.requestMIDIAccess) {
        try {
          const midiDevices = await navigator.requestMIDIAccess();
          if (midiDevices.inputs) {
            const midiInputs = midiDevices.inputs.values();
            for (let input = midiInputs.next(); input && !input.done; input = midiInputs.next()) {
              emitInternalCsoundLogEvent(
                this.csoundWorkerMain,
                `Connecting midi-input: ${input.value.name || "unkown"}`,
              );
              if (!connectedMidiDevices.has(input.value.name || "unkown")) {
                input.value.onmidimessage = this.csoundWorkerMain.handleMidiInput.bind(
                  this.csoundWorkerMain,
                );
                connectedMidiDevices.add(input.value.name || "unkown");
              }
            }
          } else {
            emitInternalCsoundLogEvent(this.csoundWorkerMain, "no midi-device detected");
          }
        } catch (error) {
          emitInternalCsoundLogEvent(
            this.csoundWorkerMain,
            "error while connecting web-midi: " + error,
          );
        }
      } else {
        emitInternalCsoundLogEvent(
          this.csoundWorkerMain,
          "no web-midi support found, midi-input will not work!",
        );
      }
    }

    if (this.isRequestingInput) {
      const getUserMedia =
        typeof navigator.mediaDevices !== "undefined"
          ? navigator.mediaDevices.getUserMedia
          : navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia;

      const microphoneCallback = (stream) => {
        if (stream) {
          const liveInput = newAudioContext.createMediaStreamSource(stream);
          this.inputsCount = liveInput.channelCount;
          const newNode = createWorkletNode(newAudioContext, liveInput.channelCount);
          this.audioWorkletNode = newNode;
          liveInput.connect(newNode).connect(newAudioContext.destination);
        } else {
          // Continue as before if user cancels
          this.inputsCount = 0;
          const newNode = createWorkletNode(newAudioContext, 0);
          this.audioWorkletNode = newNode;
          this.audioWorkletNode.connect(newAudioContext.destination);
        }
        !this.csoundWorkerMain.hasSharedArrayBuffer && this.connectPorts();
      };

      logWorklet("requesting microphone access");
      typeof navigator.mediaDevices !== "undefined"
        ? getUserMedia
            .call(navigator.mediaDevices, {
              audio: { echoCancellation: false, sampleSize: 32 },
            })
            .then(microphoneCallback)
            .catch(log.error)
        : getUserMedia.call(
            navigator,
            {
              audio: {
                optional: [{ echoCancellation: false, sampleSize: 32 }],
              },
            },
            microphoneCallback,
            log.error,
          );
    } else {
      const newNode = createWorkletNode(newAudioContext, 0);
      this.audioWorkletNode = newNode;
      logWorklet("connecting Node to AudioContext destination");
      this.audioWorkletNode.connect(newAudioContext.destination);
      !this.csoundWorkerMain.hasSharedArrayBuffer && this.connectPorts();
    }
  }
}

export default AudioWorkletMainThread;
