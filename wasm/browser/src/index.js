/* eslint-disable no-unused-vars */

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

import libcsoundEntry from "./libcsound-entry";

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
goog.exportSymbol("__libcsound__", libcsoundEntry);
