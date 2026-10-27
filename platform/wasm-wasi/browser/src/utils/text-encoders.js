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

import { TextDecoderPoly, TextEncoderPoly } from "./utf8.js";

/** @define {boolean} */
const WITH_TEXT_ENCODER_POLYFILL = goog.define("WITH_TEXT_ENCODER_POLYFILL", false);

export const decoder = WITH_TEXT_ENCODER_POLYFILL
  ? new TextDecoderPoly()
  : new TextDecoder("utf-8");

export const encoder = WITH_TEXT_ENCODER_POLYFILL ? new TextEncoderPoly() : new TextEncoder("utf8");

export const uint2String = (uint) =>
  decoder.decode(
    uint.buffer instanceof ArrayBuffer
      ? uint
      : // TextDecoder rejects shared buffers, including local Wasm heaps on
        // pages where the browser hides the SharedArrayBuffer constructor.
        // eslint-disable-next-line unicorn/prefer-spread
        uint.slice(),
  );
