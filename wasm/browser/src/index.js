/* eslint-disable no-unused-vars */

import wasmDataURI from "../dist/__csound_wasm.inline.js";
import VanillaWorkerMainThread from "./mains/vanilla.main";
import unmuteIosAudio from "unmute-ios-audio/index.js";
import SharedArrayBufferMainThread from "./mains/sab.main";
import AudioWorkletMainThread from "./mains/worklet.main";

import SingleThreadAudioWorkletMainThread from "./mains/worklet.singlethread.main";
import { logIndex as log } from "./logger";
import {
  areWorkletsSupported,
  isSafari,
  isSabSupported,

  WebkitAudioContext,
} from "./utils";

unmuteIosAudio();

/**
 * CsoundObj API.
 * @async
 * @export
 * @expose
 * @noalias
 * @param {CsoundExportArguments} argumentz
 * @return {Promise.<CsoundObj|undefined>}
 */
const Csound = async function ({
  audioContext,
  inputChannelCount,
  outputChannelCount,
  autoConnect = true,
  withPlugins = [],
  useWorker = false,
  useSAB = true,

} = {}) {
  const audioContextIsProvided =
    audioContext && WebkitAudioContext() && audioContext instanceof WebkitAudioContext();

  if (!audioContextIsProvided) {
    // default to creating an audio context for SingleThread
    audioContext = audioContext || new (WebkitAudioContext())({ latencyHint: "interactive" });
  }

  if (isSafari()) {
    audioContext.resume();
  }

  const workletSupport = areWorkletsSupported();

  // SingleThread implementations
  if (!useWorker) {
    if (workletSupport) {
      log("Single Thread AudioWorklet")();
      const instance = new SingleThreadAudioWorkletMainThread({
        audioContext,
        audioContextIsProvided,
        inputChannelCount: inputChannelCount || 2,
        outputChannelCount: outputChannelCount || 2,
      });
      return instance.initialize({ wasmDataURI, withPlugins, autoConnect });
    } else {
      console.error("No detectable WebAudioAPI in current environment");
      return;
    }
  }

  if (workletSupport) {
    // closure-compiler keepme
    log(`worklet support detected`)();
  } else {
    console.error(`No WebAudio Support detected`);
  }

  let audioWorker;
  let csoundWasmApi;

  if (workletSupport) {
    audioWorker = new AudioWorkletMainThread({ audioContext, audioContextIsProvided, autoConnect });
  }

  if (!audioWorker) {
    console.error("No detectable WebAudioAPI in current environment");
    return;
  }

  const hasSABSupport = isSabSupported();

  if (hasSABSupport) {
    useSAB && log(`using SharedArrayBuffers`)();
  } else {
    log(`SharedArrayBuffers not found, falling back to Vanilla concurrency`)();
  }

  const worker =
    hasSABSupport && workletSupport && useSAB
      ? new SharedArrayBufferMainThread({
          audioWorker,
          audioContext,
          audioContextIsProvided,
          inputChannelCount,
          outputChannelCount,
        })
      : new VanillaWorkerMainThread({
          audioWorker,
          audioContext,
          audioContextIsProvided,
          inputChannelCount,
          outputChannelCount,
        });

  if (worker) {
    log(`starting Csound thread initialization via WebWorker`)();
    await worker.initialize({ wasmDataURI, withPlugins });
    csoundWasmApi = worker.api;
  } else {
    console.error("No detectable WebAssembly support in current environment");
    return;
  }

  return Object.freeze(csoundWasmApi);
};

goog.exportSymbol("__Csound__", Csound);
