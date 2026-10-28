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

import { pointerSize } from "./wasm64.js";

/** Read C primitive widths from the engine bound to this API. */
export const sizeOfPrimitive = (wasm) => ({
  "int": 4,
  "uint64": 8,
  "cs_float": wasm.exports["csoundGetSizeOfCsFloat"](),
  "cs_double": wasm.exports["csoundGetSizeOfCsDouble"](),
  "char": 1,
  "double": 8,
  "ptr": pointerSize(wasm),
});

const alignUp = (offset, alignment) => Math.ceil(offset / alignment) * alignment;

/** Lay out C fields, including array lengths, field alignment and tail padding. */
export const structLayout = (jsStruct, wasm) => {
  const sizes = sizeOfPrimitive(wasm);
  let offset = 0;
  let alignment = 1;
  const fields = jsStruct.map(([name, primitive, count = 1]) => {
    const width = sizes[primitive];
    if (!width) throw new TypeError(`Unknown C primitive: ${primitive}`);
    alignment = Math.max(alignment, width);
    offset = alignUp(offset, width);
    const field = { name, primitive, offset, size: width * count };
    offset += field.size;
    return field;
  });
  return { fields, size: alignUp(offset, alignment) };
};

export const sizeOfStruct = (jsStruct, wasm) => structLayout(jsStruct, wasm).size;
