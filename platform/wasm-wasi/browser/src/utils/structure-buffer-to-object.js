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

import { structLayout } from "./native-sizes.js";
import { readPointer } from "./wasm64.js";
import { uint2String } from "./text-encoders.js";
import { trimNull } from "./trim-null.js";

/** Copy a native struct into JS values using its engine's little-endian ABI. */
export const structBufferToObject = (jsStruct, buffer, wasm) => {
  const view = new DataView(buffer.buffer, buffer.byteOffset, buffer.byteLength);
  const result = {};
  for (const { name, primitive, offset, size } of structLayout(jsStruct, wasm).fields) {
    let value;
    switch (primitive) {
      case "char": {
        value = trimNull(uint2String(buffer.subarray(offset, offset + size)));
        break;
      }
      case "int": {
        value = view.getInt32(offset, true);
        break;
      }
      case "uint64": {
        value = view.getBigUint64(offset, true);
        break;
      }
      case "ptr": {
        value = readPointer(wasm, view, offset);
        break;
      }
      default: {
        value = size === 4 ? view.getFloat32(offset, true) : view.getFloat64(offset, true);
      }
    }
    result[name] = value;
  }
  return result;
};
