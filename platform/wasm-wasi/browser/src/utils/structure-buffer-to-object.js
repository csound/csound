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

import { sizeOfPrimitive } from "./native-sizes.js";
import { uint2String } from "./text-encoders.js";
import { trimNull } from "./trim-null.js";

export const structBufferToObject = (jsStruct, buffer) => {
  const [result] = jsStruct.reduce(
    ([parameters, offset], [parameterName, primitive, ...rest]) => {
      const currentSize =
        primitive === "char" ? sizeOfPrimitive[primitive] * rest[0] : sizeOfPrimitive[primitive];
      const currentValue =
        primitive === "char"
          ? trimNull(uint2String(buffer.subarray(offset, currentSize))) || ""
          : buffer[offset];
      parameters[parameterName] = currentValue;
      return [parameters, offset + currentSize];
    },
    [{}, 0],
  );
  return result;
};
