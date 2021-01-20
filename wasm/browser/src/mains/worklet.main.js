import * as Comlink from "comlink";
import WorkletWorker from "@root/workers/worklet.worker";
import { logWorkletMain as log } from "@root/logger";
import { WebkitAudioContext } from "@root/utils";
// import { requestMicrophoneNode } from "./io.utils";
import { requestMidi } from "@utils/request-midi";

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
    this.ksmps = undefined;
    this.sampleRate = undefined;
    this.inputsCount = undefined;
    this.outputsCount = undefined;
    this.hardwareBufferSize = undefined;
    this.softwareBufferSize = undefined;

    this.initialize = this.initialize.bind(this);
    this.onPlayStateChange = this.onPlayStateChange.bind(this);
    this.terminateInstance = this.terminateInstance.bind(this);
    this.createWorkletNode = this.createWorkletNode.bind(this);
    log("AudioWorkletMainThread was constructed")();
  }

  async terminateInstance() {
    if (this.audioWorkletNode) {
      this.audioWorkletNode.disconnect();
      delete this.audioWorkletNode;
    }
    if (this.audioContext) {
      if (this.audioContext.state !== "closed") {
        try {
          await this.audioContext.close();
        } catch {}
      }
      delete this.audioContext;
    }
    if (this.workletProxy) {
      this.workletProxy[Comlink.releaseProxy]();
      delete this.workletProxy;
    }
  }

  createWorkletNode(audioContext, inputsCount, contextUid) {
    const audioNode = new AudioWorkletNode(audioContext, "csound-worklet-processor", {
      inputChannelCount: inputsCount ? [inputsCount] : 0,
      outputChannelCount: [this.outputsCount || 2],
      processorOptions: {
        contextUid,
        isRequestingInput: this.isRequestingInput,
        inputsCount,
        outputsCount: this.outputsCount,
        ksmps: this.ksmps,
        maybeSharedArrayBuffer:
          this.csoundWorkerMain.hasSharedArrayBuffer && this.csoundWorkerMain.audioStatePointer,
        maybeSharedArrayBufferAudioIn:
          this.csoundWorkerMain.hasSharedArrayBuffer && this.csoundWorkerMain.audioStreamIn,
        maybeSharedArrayBufferAudioOut:
          this.csoundWorkerMain.hasSharedArrayBuffer && this.csoundWorkerMain.audioStreamOut,
      },
    });
    this.csoundWorkerMain.publicEvents.triggerOnAudioNodeCreated(audioNode);
    return audioNode;
  }

  async onPlayStateChange(newPlayState) {
    this.currentPlayState = newPlayState;

    switch (newPlayState) {
      case "realtimePerformanceStarted": {
        log("event received: realtimePerformanceStarted")();
        await this.initialize();
        break;
      }
      case "realtimePerformanceEnded": {
        log(
          "event received: realtimePerformanceEnded" + !this.csoundWorkerMain.hasSharedArrayBuffer
            ? ` cleaning up ports`
            : "",
        )();
        if (
          !this.audioContextIsProvided &&
          this.autoConnect &&
          this.audioContext &&
          this.audioContext.state !== "closed"
        ) {
          try {
            await this.audioContext.close();
          } catch {}
        }

        if (this.autoConnect && this.audioWorkletNode) {
          this.audioWorkletNode.disconnect();
          delete this.audioWorkletNode;
        }
        if (this.workletProxy) {
          this.workletProxy[Comlink.releaseProxy]();
          delete this.workletProxy;
        }

        if (this.workletWorkerUrl) {
          (window.URL || window.webkitURL).revokeObjectURL(this.workletWorkerUrl);
        }

        this.audioWorkletNode && delete this.audioWorkletNode;
        this.currentPlayState = undefined;
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

    if (
      this.csoundWorkerMain &&
      this.csoundWorkerMain.eventPromises &&
      !this.csoundWorkerMain.hasSharedArrayBuffer
    ) {
      await this.csoundWorkerMain.eventPromises.releaseStartPromises();
    }
  }

  async initialize() {
    if (!this.audioContext) {
      if (this.audioContextIsProvided) {
        console.error(`fatal: the provided AudioContext was undefined`);
      }
      this.audioContext = new (WebkitAudioContext())({ sampleRate: this.sampleRate });
    }
    if (this.audioContext.state === "closed") {
      if (this.audioContextIsProvided) {
        console.error(`fatal: the provided AudioContext was closed, falling back new AudioContext`);
      }
      this.audioContext = new (WebkitAudioContext())({ sampleRate: this.sampleRate });
    }

    if (this.sampleRate !== this.audioContext.sampleRate) {
      this.audioContext = new (WebkitAudioContext())({ sampleRate: this.sampleRate });
      // if this.audioContextIsProvided is true
      // it should already be picked
      if (this.audioContextIsProvided) {
        console.error("Internal error: sample rate was ignored from provided audioContext");
      }
    }

    this.workletWorkerUrl = WorkletWorker();
    await this.audioContext.audioWorklet.addModule(this.workletWorkerUrl);

    log("WorkletWorker module added")();

    if (!this.csoundWorkerMain) {
      console.error(`fatal: worker not reachable from worklet-main thread`);
      return;
    }

    const contextUid = `audioWorklet${UID}`;
    UID += 1;

    if (this.isRequestingMidi) {
      log("requesting for web-midi connection");
      requestMidi({
        onMidiMessage: this.csoundWorkerMain.handleMidiInput.bind(this.csoundWorkerMain),
      });
    }

    let microphonePromise;

    if (this.isRequestingInput) {
      let resolveMicrophonePromise;
      microphonePromise = new Promise((resolve) => {
        resolveMicrophonePromise = resolve;
      });
      const getUserMedia =
        typeof navigator.mediaDevices !== "undefined"
          ? navigator.mediaDevices.getUserMedia
          : navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia;

      const microphoneCallback = (stream) => {
        if (stream) {
          const liveInput = this.audioContext.createMediaStreamSource(stream);
          this.inputsCount = liveInput.channelCount;
          const newNode = this.createWorkletNode(
            this.audioContext,
            liveInput.channelCount,
            contextUid,
          );
          this.audioWorkletNode = newNode;
          if (this.autoConnect) {
            liveInput.connect(newNode).connect(this.audioContext.destination);
          }
        } else {
          // Continue as before if user cancels
          this.inputsCount = 0;
          const newNode = this.createWorkletNode(this.audioContext, 0, contextUid);
          this.audioWorkletNode = newNode;
          if (this.autoConnect) {
            this.audioWorkletNode.connect(this.audioContext.destination);
          }
        }
        resolveMicrophonePromise && resolveMicrophonePromise();
      };

      log("requesting microphone access")();
      typeof navigator.mediaDevices !== "undefined"
        ? getUserMedia
            .call(navigator.mediaDevices, {
              audio: { echoCancellation: false, sampleSize: 32 },
            })
            .then(microphoneCallback)
            .catch(console.error)
        : getUserMedia.call(
            navigator,
            {
              audio: {
                optional: [{ echoCancellation: false, sampleSize: 32 }],
              },
            },
            microphoneCallback,
            console.error,
          );
    } else {
      const newNode = this.createWorkletNode(this.audioContext, 0, contextUid);
      this.audioWorkletNode = newNode;

      log("connecting Node to AudioContext destination")();
      if (this.autoConnect) {
        this.audioWorkletNode.connect(this.audioContext.destination);
      }
    }

    microphonePromise && (await microphonePromise);
    this.workletProxy = Comlink.wrap(this.audioWorkletNode.port);
    await this.workletProxy.initialize(
      Comlink.transfer(
        {
          contextUid,
          messagePort: this.ipcMessagePorts.workerMessagePortAudio,
          requestPort: this.ipcMessagePorts.audioWorkerFrameRequestPort,
          inputPort: this.ipcMessagePorts.audioWorkerAudioInputPort,
        },
        [
          this.ipcMessagePorts.workerMessagePortAudio,
          this.ipcMessagePorts.audioWorkerFrameRequestPort,
          this.ipcMessagePorts.audioWorkerAudioInputPort,
        ],
      ),
    );

    log("initialization finished in main")();
  }
}

export default AudioWorkletMainThread;
