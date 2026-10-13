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

// using statically built wasm-bin
// we can create a minimal wrapper
// for calling csound without any audio
// bindings

import wasmDataURI from "../dist/__csound_wasm_static_tools.inline.js";
import libcsoundFactory from "../src/libcsound";
import loadWasm from "../src/module";
import { csoundApiRename } from "../src/utils";
import fs from "../src/filesystem/worker-fs";

const fsMethods = Object.keys(fs);

let arrayBuffer;

const makeCsoundCallback =
  (csoundInstance, apiCallback) =>
  async (...arguments_) => {
    return await apiCallback.apply({}, [csoundInstance, ...arguments_]);
  };

const Csound = async ({ logCallback } = {}) => {
  if (!arrayBuffer) {
    arrayBuffer = wasmDataURI();
  }
  // Use the same per-instance log buffer and WASI setup as the audio bindings.
  const messagePort = {
    post: ({ log }) =>
      typeof log === "string" && typeof logCallback === "function" && logCallback(log),
  };
  const [instance, wasi] = await loadWasm({ wasmDataURI: arrayBuffer, messagePort });
  instance.wasi = wasi;

  const libcsound = libcsoundFactory(instance);

  const csoundInstance = libcsound.csoundCreate(0);

  const csoundApi = Object.keys(libcsound).reduce((accumulator, apiName) => {
    if (!libcsound[apiName]) {
      return accumulator;
    }

    if (fsMethods.includes(apiName)) {
      accumulator.fs = accumulator.fs || {};
      const reference = libcsound[apiName];
      const callback = async (...arguments_) =>
        makeCsoundCallback(csoundInstance, libcsound[apiName]).apply({}, arguments_);
      callback.toString = reference.toString;
      accumulator.fs[apiName] = callback;
    } else {
      const renamedApiName = csoundApiRename(apiName);
      accumulator[renamedApiName] = (...arguments_) => {
        return makeCsoundCallback(csoundInstance, libcsound[apiName]).apply({}, arguments_);
      };
      accumulator[renamedApiName].toString = libcsound[apiName].toString;
    }

    return accumulator;
  }, {});

  return csoundApi;
};

goog.exportSymbol("__Csound__", Csound);
