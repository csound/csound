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

import { TREE, ORCTOKEN, CS_MIDIDEVICE, CSOUND_PARAMS } from "../structures.js";

export const sizeOfPrimitive = {
  int: 4,
  uint64: 8,
  MYFLT: 8,
  char: 1,
  double: 8,
  ptr: 4,
  void: 8,
};

export const sizeOfStruct = (jsStruct) =>
  jsStruct
    ? jsStruct.reduce((accumulator, [_, primitive]) => {
        return sizeOfPrimitive[primitive]
          ? sizeOfPrimitive[primitive] + accumulator
          : sizeOfStruct({ TREE, ORCTOKEN }[primitive]) + accumulator;
      }, 0)
    : 0;

export const TREE_SIZE = sizeOfStruct(TREE);
export const ORCTOKEN_SIZE = sizeOfStruct(ORCTOKEN);
export const CSOUND_PARAMS_SIZE = sizeOfStruct(CSOUND_PARAMS);
export const CS_MIDIDEVICE_SIZE = sizeOfStruct(CS_MIDIDEVICE);
