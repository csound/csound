import * as Comlink from "comlink";
import WorkletWorker from "@root/workers/worklet.worker";
import log, { logWorklet } from "@root/logger";
import { WebkitAudioContext } from "@root/utils";

const connectedMidiDevices = new Set();
let UID = 0;

class AudioWorkletMainThread {
  constructor({ audioContext, audioContextIsProvided, autoConnect }) {
    this.autoConnect = autoConnect;
    this.audioContextIsProvided = audioContextIsProvided;
    this.ipcMessagePorts = undefined;
    this.audioContext = audioContext;
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
        if (!this.audioContextIsProvided && this.autoConnect) {
          this.audioContext.close();
        }
        if (this.autoConnect) {
          this.audioWorkletNode.disconnect();
        }
        delete this.audioWorkletNode;
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

    // hacky SAB timing fix when starting
    // eventually, replace this spaghetti with
    // private/internal event emitters
    if (this.csoundWorkerMain.startPromiz) {
      const startPromiz = this.csoundWorkerMain.startPromiz;
      setTimeout(() => {
        startPromiz();
      }, 0);
      delete this.csoundWorkerMain.startPromiz;
    }
  }

  async initialize() {
    if (!this.audioContext) {
      if (this.audioContextIsProvided) {
        log.error(`fatal: the provided AudioContext was undefined`);
      }
      this.audioContext = new (WebkitAudioContext())();
    }
    if (this.audioContext.state === "closed") {
      if (this.audioContextIsProvided) {
        log.error(`fatal: the provided AudioContext was closed, falling back new AudioContext`);
      }
      this.audioContext = new (WebkitAudioContext())();
    }

    await this.audioContext.audioWorklet.addModule(WorkletWorker());
    logWorklet("WorkletWorker module added");

    if (!this.csoundWorkerMain) {
      log.error(`fatal: worker not reachable from worklet-main thread`);
      return;
    }

    const contextUid = `audioWorklet${UID}`;
    UID += 1;

    const createWorkletNode = (audioContext, inputsCount) => {
      return new AudioWorkletNode(audioContext, "csound-worklet-processor", {
        inputChannelCount: inputsCount ? [inputsCount] : 0,
        outputChannelCount: [this.outputsCount || 2],
        processorOptions: {
          contextUid,
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
          const liveInput = this.audioContext.createMediaStreamSource(stream);
          this.inputsCount = liveInput.channelCount;
          const newNode = createWorkletNode(this.audioContext, liveInput.channelCount);
          this.audioWorkletNode = newNode;
          if (this.autoConnect) {
            liveInput.connect(newNode).connect(this.audioContext.destination);
          }
        } else {
          // Continue as before if user cancels
          this.inputsCount = 0;
          const newNode = createWorkletNode(this.audioContext, 0);
          this.audioWorkletNode = newNode;
          if (this.autoConnect) {
            this.audioWorkletNode.connect(this.audioContext.destination);
          }
        }
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
      const newNode = createWorkletNode(this.audioContext, 0);
      this.audioWorkletNode = newNode;
      logWorklet("connecting Node to AudioContext destination");
      if (this.autoConnect) {
        this.audioWorkletNode.connect(this.audioContext.destination);
      }
    }

    this.workletProxy = Comlink.wrap(this.audioWorkletNode.port);
    await this.workletProxy.initialize(
      Comlink.transfer(
        {
          contextUid,
          inputPort: this.ipcMessagePorts.audioWorkerAudioInputPort,
          messagePort: this.ipcMessagePorts.workerMessagePortAudio,
          requestPort: this.ipcMessagePorts.audioWorkerFrameRequestPort,
        },
        [
          this.ipcMessagePorts.audioWorkerAudioInputPort,
          this.ipcMessagePorts.workerMessagePortAudio,
          this.ipcMessagePorts.audioWorkerFrameRequestPort,
        ],
      ),
    );
  }
}

export default AudioWorkletMainThread;
