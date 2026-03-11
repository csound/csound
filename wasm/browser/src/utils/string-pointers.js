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

import { encoder, uint2String } from "./text-encoders.js";
import { trimNull } from "./trim-null.js";

/**
 * @param {WasmInst} wasm
 * @param {number} ptr
 */
export const freeStringPtr = (wasm, ptr) => {
  wasm["exports"]["freeStringMem"](ptr);
};

export const ptr2string = (wasm, stringPtr) => {
  const { buffer } = wasm.wasi.memory;
  const bytes = new Uint8Array(buffer, stringPtr);
  // Find the null terminator so we only decode the actual string,
  // not the entire remaining wasm memory buffer.
  let len = 0;
  while (bytes[len] !== 0) len++;
  if (len === 0) return "";
  const bounded = new Uint8Array(buffer, stringPtr, len);
  return uint2String(bounded);
};

export const string2ptr = (wasm, string) => {
  if (typeof string !== "string") {
    console.error("Expected string but got", typeof string);
    return;
  }

  const stringBuf = encoder.encode(string);
  const offset = wasm["exports"]["allocStringMem"](stringBuf.length);
  const { buffer } = wasm.wasi.memory;
  const outBuf = new Uint8Array(buffer, offset, stringBuf.length + 1);
  outBuf.set(stringBuf);
  return offset;
};
