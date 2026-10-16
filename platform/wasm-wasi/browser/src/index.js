/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import wasmDataURI from "../dist/__csound_wasm.inline.js";
import { sharedEngine } from "../dist/__csound_engine.inline.js";
import { canUseSharedWasm, canUseThreadedWasm } from "./utils/wasi-thread-capabilities.js";
import unmuteIosAudio from "unmute-ios-audio/index.js";
import SharedArrayBufferMainThread from "./mains/sab.main";
import AudioWorkletMainThread from "./mains/worklet.main";
import VanillaWorkerMainThread from "./mains/vanilla.main";
import SingleThreadAudioWorkletMainThread from "./mains/worklet.singlethread.main";
import { areWorkletsSupported, isSabSupported, isSafari, WebkitAudioContext } from "./utils";
import libcsound from "./libcsound-entry";

let audioUnmuted = false;

/**
 * CsoundObj API.
 * @async
 * @export
 * @expose
 * @noalias
 * @param {CsoundExportArguments=} options
 * @return {Promise.<CsoundObj>}
 */
const Csound = async function (options = {}) {
  const {
    audioContext: requestedContext,
    inputChannelCount,
    outputChannelCount,
    autoConnect = true,
    withPlugins = [],
    useSAB = true,
  } = options;
  // Keep the default usable without isolation, but honor an explicit worker request.
  const threaded =
    sharedEngine &&
    options.useWorker !== false &&
    useSAB &&
    (options.useWorker === true || canUseThreadedWasm());
  const useWorker = threaded || Boolean(options.useWorker);
  if (threaded && !canUseThreadedWasm())
    throw new Error(
      "Csound requires cross-origin isolation, SharedArrayBuffer, AudioWorklet, and " +
        "WebAssembly threads with standard exception handling. " +
        "Set useWorker: false for a local engine without background init.",
    );
  if (sharedEngine && !threaded && !canUseSharedWasm())
    throw new Error(
      "Csound requires WebAssembly shared memory, atomics, and standard exception handling.",
    );
  if (!areWorkletsSupported()) throw new Error("Csound requires AudioWorklet support.");
  const audioContextIsProvided =
    requestedContext && WebkitAudioContext() && requestedContext instanceof WebkitAudioContext();
  if (!audioUnmuted) {
    unmuteIosAudio();
    audioUnmuted = true;
  }
  const audioContext =
    requestedContext || new (WebkitAudioContext())({ latencyHint: "interactive" });
  if (isSafari()) audioContext.resume();
  if (!useWorker) {
    const instance = new SingleThreadAudioWorkletMainThread({
      audioContext,
      audioContextIsProvided,
      inputChannelCount: inputChannelCount || 2,
      outputChannelCount: outputChannelCount || 2,
    });
    try {
      return await instance.initialize({ wasmDataURI, withPlugins, autoConnect });
    } catch (error) {
      await instance.terminateInstance().catch(() => {});
      if (!audioContextIsProvided && audioContext.state !== "closed")
        await audioContext.close().catch(() => {});
      throw error;
    }
  }
  const audioWorker = new AudioWorkletMainThread({
    audioContext,
    audioContextIsProvided,
    autoConnect,
  });
  const WorkerMain =
    threaded || (useSAB && isSabSupported())
      ? SharedArrayBufferMainThread
      : VanillaWorkerMainThread;
  const worker = new WorkerMain({
    audioWorker,
    audioContext,
    audioContextIsProvided,
    inputChannelCount,
    outputChannelCount,
  });
  try {
    await worker.initialize({
      wasmDataURI,
      withPlugins,
      threadedBytes: threaded ? wasmDataURI() : undefined,
    });
  } catch (error) {
    // Cleanup must not hide a bad reactor or worker startup error.
    await worker.terminateInstance().catch(() => {});
    if (!audioContextIsProvided && audioContext.state !== "closed")
      await audioContext.close().catch(() => {});
    throw error;
  }
  return Object.freeze(worker.api);
};

goog.exportSymbol("__Csound__", Csound);
goog.exportSymbol("__libcsound__", libcsound);
