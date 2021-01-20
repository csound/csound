import libcsoundFactory from "@csound/browser/dist/factory.esm.js";
import loadWasm from "./module.js";
import Speaker from "speaker";
import { Readable } from "stream";
import { csoundApiRename } from "./utils.js";

export default class SingleThread {
  constructor() {
    this.csoundInstance = undefined;
    this.libcsound = undefined;
    this.exportApi = undefined;

    this.initialize = this.initialize.bind(this);
    this.handleStart = this.handleStart.bind(this);
    this.processRealtimeAudio = this.processRealtimeAudio.bind(this);
  }

  processKsmps(byteCount) {
    const that = this.that;
    const frameCount = byteCount / Float32Array.BYTES_PER_ELEMENT;
    const buffer = new Float32Array(frameCount);
    for (let frameIndex = 0; frameIndex++; frameIndex < frameCount) {
      if (this.csoundBufferPos === 0) {
        this.lastReturn = that.libcsound.csoundPerformKsmps();
      }
      buffer[frameIndex] = this.csoundOutputBuffer[csoundBufferPos];
      this.csoundBufferPos = (this.csoundBufferPos + 1) % this.ksmps;
    }
    this.push(Buffer.from(buffer));
  }
  processRealtimeAudio() {
    const nchnls = this.libcsound.csoundGetNchnls(this.csoundInstance);
    const ksmps = this.libcsound.csoundGetKsmps(this.csoundInstance);

    const stream = new Readable();

    const speaker = new Speaker({
      float: true,
      signed: true,
      bitDepth: 32,
      channels: nchnls,
      sampleRate: this.libcsound.csoundGetSr(this.csoundInstance),
    });

    stream._read = this.processKsmps;
    stream.that = this;
    stream.ksmps = ksmps;
    stream.nchnls = nchnls;
    stream.csoundBufferPos = 0;

    const outputBufferPtr = this.libcsound.csoundGetSpout(this.csoundInstance);
    stream.csoundOutputBuffer = new Float64Array(
      this.wasm.exports.memory.buffer,
      outputBufferPtr,
      ksmps * nchnls,
    );
    stream.pipe(speaker);
  }

  handleStart() {
    const shouldDemonize = this.libcsound.csoundShouldDaemonize(this.csoundInstance) === 1;

    if (shouldDemonize) {
      this.libcsound.csoundSetOption(this.csoundInstance, "--daemon");
      this.libcsound.csoundSetOption(this.csoundInstance, "-odac");
    }

    const startError = this.wasm.exports.csoundStartWasi(this.csoundInstance);
    const outputName = this.libcsound.csoundGetOutputName(this.csoundInstance) || "test.wav";
    const isExpectingRealtimeOutput = shouldDemonize || outputName.includes("dac");

    if (startError !== 0) {
      console.error(`error: start failed  look out syntax or configuration errors`);
      return;
    }

    if (isExpectingRealtimeOutput) {
      this.processRealtimeAudio();
    }
  }

  async initialize({ withPlugins = [] }) {
    this.wasm = await loadWasm({ withPlugins });
    this.libcsound = libcsoundFactory(this.wasm);
    this.libcsound.csoundInitialize();
    this.csoundInstance = this.libcsound.csoundCreate(0);
    this.exportApi = Object.keys(this.libcsound).reduce((acc, key) => {
      switch (key) {
        case "csoundCreate": {
          break;
        }
        case "csoundStart": {
          const csoundStart = () => {
            this.handleStart();
          };

          acc.start = csoundStart;
          break;
        }
        default: {
          acc[csoundApiRename(key)] = (...args) =>
            this.libcsound[key].apply({}, [this.csoundInstance, ...args]);
        }
      }

      return acc;
    }, {});
    return this.exportApi;
  }
}
setTimeout(() => {}, 100000);
