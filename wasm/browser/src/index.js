// eslint-disable-next-line no-unused-vars
import * as Comlink from "comlink";
import VanillaWorkerMainThread from "@root/mains/vanilla.main";
import unmuteIosAudio from "unmute-ios-audio";
import SharedArrayBufferMainThread from "@root/mains/sab.main";
import AudioWorkletMainThread from "@root/mains/worklet.main";
import ScriptProcessorNodeMainThread from "@root/mains/old-spn.main";
import wasmDataURI from "@csound/wasm/lib/libcsound.wasm.zlib";
import log, { logSAB, logWorklet, logVAN } from "@root/logger";
import { areWorkletsSupportet, isSabSupported, isScriptProcessorNodeSupported } from "@root/utils";

let audioWorker, csoundWasmApi;

/**
 * The default entry for libcsound es7 module
 * @async
 * @return {Promise.<Object>}
 */
export async function Csound() {
  // prevent multiple initializations
  if (csoundWasmApi) {
    return csoundWasmApi;
  } else {
    unmuteIosAudio();
  }
  const workletSupport = areWorkletsSupportet();
  const spnSupport = isScriptProcessorNodeSupported();

  if (workletSupport) {
    logWorklet(`support detected`);
  } else if (spnSupport) {
    logVAN(`support detected`);
  } else {
    log.warn(`No WebAudio Support detected`);
  }

  if (workletSupport) {
    audioWorker = new AudioWorkletMainThread();
  } else if (spnSupport) {
    audioWorker = new ScriptProcessorNodeMainThread();
  }

  if (!audioWorker) {
    log.error("No detectable WebAudioAPI in current environment");
    return undefined;
  }

  const hasSABSupport = isSabSupported();

  if (!hasSABSupport) {
    log.warn(`SharedArrayBuffers not found, falling back to Vanilla concurrency`);
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
