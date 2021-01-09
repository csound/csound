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
  audioContext,
  inputChannelCount = 2,
  outputChannelCount = 2,
  autoConnect = true,
  withPlugins = [],
  useWorker = false,
  useSAB = true,
  useSPN = false,
} = {}) {
  unmuteIosAudio();

  let audioContextIsProvided =
    audioContext && WebkitAudioContext() && audioContext instanceof WebkitAudioContext();

  if (!audioContextIsProvided) {
    // default to creating an audio context for SingleThread
    audioContext = audioContext || new (WebkitAudioContext())({ latencyHint: "interactive" });
  }
  const workletSupport = areWorkletsSupported();
  const spnSupport = isScriptProcessorNodeSupported();

  // SingleThread implementations
  if (!useWorker) {
    if (workletSupport && !useSPN) {
      console.log("Single Thread AudioWorklet", audioContext);
      const instance = new SingleThreadAudioWorkletMainThread({
        audioContext,
        inputChannelCount,
        outputChannelCount,
      });
      // const instance = await createSingleThreadAudioWorkletAPI({audioContext, withPlugins});
      //return instance;
      return instance.initialize({ wasmDataURI, withPlugins, autoConnect });
    } else if (spnSupport) {
      console.log("Single Thread ScriptProcessorNode");
      const instance = new ScriptProcessorNodeSingleThread({
        audioContext,
        inputChannelCount,
        outputChannelCount,
      });
      return await instance.initialize({ wasmDataURI, withPlugins, autoConnect });
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

  if (workletSupport && !useSPN) {
    audioWorker = new AudioWorkletMainThread({ audioContext, audioContextIsProvided });
  } else if (spnSupport) {
    audioWorker = new ScriptProcessorNodeMainThread({ audioContext, audioContextIsProvided });
  }

  if (!audioWorker) {
    log.error("No detectable WebAudioAPI in current environment");
    return undefined;
  }

  const hasSABSupport = isSabSupported();

  if (!hasSABSupport) {
    log.warning(`SharedArrayBuffers not found, falling back to Vanilla concurrency`);
  } else {
    useSAB && logSAB(`using SharedArrayBuffers`);
  }

  const worker =
    hasSABSupport && workletSupport && useSAB
      ? new SharedArrayBufferMainThread({ audioWorker, wasmDataURI, audioContextIsProvided })
      : new VanillaWorkerMainThread({ audioWorker, wasmDataURI, audioContextIsProvided });

  if (worker) {
    log(`starting Csound thread initialization via WebWorker`);
    await worker.initialize({ withPlugins });
    csoundWasmApi = worker.api;
  } else {
    log.error("No detectable WebAssembly support in current environment");
    return undefined;
  }

  return csoundWasmApi;
}

export default Csound;
