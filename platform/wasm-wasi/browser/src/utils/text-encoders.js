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

/** @define {boolean} */
const WITH_TEXT_ENCODER_POLYFILL = goog.define("WITH_TEXT_ENCODER_POLYFILL", false);

/** @constructor */
function TextEncoderPoly() {
  this.encoding = "utf8";
  return this;
}

TextEncoderPoly.prototype.encode = function (string_) {
  if (typeof string_ !== "string") {
    throw new TypeError("passed argument must be of type string " + string_ + " " + typeof string_);
  }
  const binstr = unescape(encodeURIComponent(string_));
  const array = new Uint8Array(binstr.length);
  [...binstr].forEach(function (char, index) {
    array[index] = char.codePointAt(0);
  });
  return array;
};

/** @constructor */
function TextDecoderPoly() {
  this.encoding = "utf8";
  this.ignoreBOM = false;

  this.trimNull = (a) => {
    const c = a.indexOf("\0");
    return c === -1 ? a : a.slice(0, Math.max(0, c));
  };

  this.decode = function (view, options) {
    if (view === undefined) {
      return "";
    }

    const stream = options !== undefined && "stream" in options && options.stream;
    if (typeof stream !== "boolean") {
      throw new TypeError("stream option must be boolean");
    }

    if (ArrayBuffer.isView(view)) {
      const array = new Uint8Array(view.buffer, view.byteOffset, view.byteLength);
      // AudioWorklet has no TextDecoder. Decode UTF-8 rather than treating
      // each byte as a character; prompts and string channels use UTF-8.
      let text = "";
      for (let index = 0; index < array.length;) {
        const first = array[index++];
        if (first < 0x80) {
          text += String.fromCodePoint(first);
          continue;
        }
        const count =
          first >= 0xc2 && first <= 0xdf
            ? 1
            : first >= 0xe0 && first <= 0xef
              ? 2
              : first >= 0xf0 && first <= 0xf4
                ? 3
                : 0;
        if (count === 0) {
          text += "\u{FFFD}";
          continue;
        }
        let codepoint = first & (0x7f >> count);
        let consumed = 0;
        for (; consumed < count && index < array.length; consumed++) {
          const byte = array[index];
          const lower =
            consumed === 0 && first === 0xe0
              ? 0xa0
              : consumed === 0 && first === 0xf0
                ? 0x90
                : 0x80;
          const upper =
            consumed === 0 && first === 0xed
              ? 0x9f
              : consumed === 0 && first === 0xf4
                ? 0x8f
                : 0xbf;
          if (byte < lower || byte > upper) break;
          codepoint = (codepoint << 6) | (byte & 0x3f);
          index++;
        }
        text += consumed === count ? String.fromCodePoint(codepoint) : "\u{FFFD}";
      }
      return this.trimNull(text);
    } else {
      throw new TypeError("passed argument must be an array buffer view");
    }
  };
}

export const decoder = WITH_TEXT_ENCODER_POLYFILL
  ? new TextDecoderPoly()
  : new TextDecoder("utf-8");

export const encoder = WITH_TEXT_ENCODER_POLYFILL ? new TextEncoderPoly() : new TextEncoder("utf8");

export const uint2String = (uint) => decoder.decode(uint);
