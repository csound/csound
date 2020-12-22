// eslint-disable-next-line no-unused-vars
import * as Comlink from "comlink";
import VanillaWorkerMainThread from "@root/mains/vanilla.main";
import unmuteIosAudio from "unmute-ios-audio";
import SharedArrayBufferMainThread from "@root/mains/sab.main";
import AudioWorkletMainThread from "@root/mains/worklet.main";
import ScriptProcessorNodeMainThread from "@root/mains/old-spn.main";
import ScriptProcessorNodeSingleThread from "@root/mains/spn.main";
import SingleThreadAudioWorkletMainThread from "@root/mains/worklet.singlethread.main";
import wasmDataURI from "@csound/wasm/lib/libcsound.wasm.zlib";
import log, { logSAB, logWorklet, logVAN } from "@root/logger";
import {
  areWorkletsSupported,
  isSabSupported,
  isScriptProcessorNodeSupported,
  WebkitAudioContext,
} from "@root/utils";

/**
 * CsoundObj API.
 * @namespace CsoundObj
 */
/**
 * The default entry for @csound/wasm/browser module.
 * If loaded successfully, it returns CsoundObj,
 * otherwise undefined.
 * @async
 * @return {Promise.<CsoundObj|undefined>}
 */
export async function Csound({
  audioContext = new (WebkitAudioContext())(),
  useWorker = false,
} = {}) {
  unmuteIosAudio();

  const workletSupport = areWorkletsSupported();
  const spnSupport = isScriptProcessorNodeSupported();

  // SingleThread implementations
  if (!useWorker) {
    if (workletSupport) {
      console.log("Single Thread AudioWorklet", audioContext);
      const instance = new SingleThreadAudioWorkletMainThread({ audioContext });
      return await instance.initialize(wasmDataURI);
    } else if (spnSupport) {
      console.log("Single Thread ScriptProcessorNode");
      const instance = new ScriptProcessorNodeSingleThread({ audioContext });
      return await instance.initialize(wasmDataURI);
    } else {
      log.error("No detectable WebAudioAPI in current environment");
      return undefined;
    }
  }

  if (workletSupport) {
    logWorklet(`support detected`);
  } else if (spnSupport) {
    logVAN(`support detected`);
  } else {
    log.warning(`No WebAudio Support detected`);
  }

  let audioWorker;
  let csoundWasmApi;

  if (workletSupport) {
    audioWorker = new AudioWorkletMainThread({ audioContext });
  } else if (spnSupport) {
    audioWorker = new ScriptProcessorNodeMainThread({ audioContext });
  }

  if (!audioWorker) {
    log.error("No detectable WebAudioAPI in current environment");
    return undefined;
  }

  const hasSABSupport = isSabSupported();

  if (!hasSABSupport) {
    log.warning(`SharedArrayBuffers not found, falling back to Vanilla concurrency`);
  } else {
    logSAB(`using SharedArrayBuffers`);
  }

  const worker =
    hasSABSupport && workletSupport
      ? new SharedArrayBufferMainThread(audioWorker, wasmDataURI)
      : new VanillaWorkerMainThread(audioWorker, wasmDataURI);

  if (worker) {
    log(`starting Csound thread initialization via WebWorker`);
    await worker.initialize();
    csoundWasmApi = worker.api;
  } else {
    log.error("No detectable WebAssembly support in current environment");
    return undefined;
  }

  return csoundWasmApi;
}

export default Csound;
