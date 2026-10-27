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

/*
   csound table module from <csound.h>
   https://csound.com/docs/api/group___t_a_b_l_e.html
*/

/**
 * Returns the length of a function table
 * (not including the guard point),
 * or -1 if the table does not exist.
 * @function
 */
export const csoundTableLength = (wasm) => (csound /* CsoundInst */, tableNumber /* string */) =>
  wasm.exports["csoundTableLength"](csound, tableNumber);

csoundTableLength.toString = () => "tableLength = async (tableNum) => Number;";

/**
 * Copy the contents of an Array or TypedArray from javascript into a given csound function table.
 * The table number is assumed to be valid, and the table needs to have sufficient space
 * to receive all the array contents.
 * The table number and index are assumed to be valid.
 */
export const csoundTableCopyIn =
  (wasm) => (csound /* CsoundInst */, tableNumber /* string */, array /* ArrayLike<number> */) => {
    const sourceLength = array.length;
    const length = wasm.exports["csoundTableLength"](csound, tableNumber);
    if (length < 0) return;
    if (sourceLength < length) throw new RangeError("The source must fill the Csound table");
    // The C API also reads the guard point after the table's last sample.
    const arrayPtr = wasm.exports["allocFloatArray"](length + 1);
    try {
      const buffer = new Float32Array(wasm.wasi.memory.buffer, arrayPtr, length + 1);
      for (let index = 0; index < length; index++) buffer[index] = array[index];
      buffer[length] = sourceLength > length ? array[length] : array[0];
      wasm.exports["csoundTableCopyIn"](csound, tableNumber, arrayPtr, 0);
    } finally {
      wasm.exports["freeFloatArrayMem"](arrayPtr);
    }
  };

csoundTableCopyIn.toString = () => "tableCopyIn = async (tableNum, float32Array) => undefined;";

/**
 * Copies the contents of a function table from csound into Float32Array.
 * The function returns a Float32Array if the table exists, otherwise
 * it returns undefined.
 * @function
 */
export const csoundTableCopyOut = (wasm) => (csound /* CsoundInst */, tableNumber /* string */) => {
  const tableLength = wasm.exports["csoundTableLength"](csound, tableNumber);
  if (tableLength > 0) {
    const arrayPtr = wasm.exports["allocFloatArray"](tableLength);
    wasm.exports["csoundTableCopyOut"](csound, tableNumber, arrayPtr);
    const { buffer } = wasm.wasi.memory;
    const freeableArray = new Float32Array(buffer, arrayPtr, tableLength);
    const clonedArray = new Float32Array(freeableArray.length);
    /* eslint-disable-next-line unicorn/no-for-loop */
    for (let i = 0; i < freeableArray.length; i++) {
      clonedArray[i] = freeableArray[i];
    }
    wasm.exports["freeFloatArrayMem"](arrayPtr);
    return clonedArray;
  }
};

csoundTableCopyOut.toString = () => "tableCopyOut = async (tableNum) => ?Float32Array;";

/**
 * @name getTable
 * @alias csoundTableCopyOut
 * @async
 * @function
 */
export const csoundGetTable = csoundTableCopyOut;
csoundGetTable.toString = csoundTableCopyOut.toString;

/**
 * Copies the contents of a function table from csound into Float32Array.
 * The function returns a Float32Array if the table exists, otherwise
 * it returns undefined.
 */
export const csoundGetTableArgs = (wasm) => (csound /* CsoundInst */, tableNumber /* string */) => {
  const pointerSlot = wasm.exports["allocByteMem"](4);
  try {
    const length = wasm.exports["csoundGetTableArgs"](csound, pointerSlot, tableNumber);
    if (length < 0) return;
    const { buffer } = wasm.wasi.memory;
    const pointer = new Uint32Array(buffer, pointerSlot, 1)[0];
    return new Float32Array(buffer, pointer, length).slice();
  } finally {
    wasm.exports["freeByteMem"](pointerSlot);
  }
};

csoundGetTableArgs["toString"] = () => "getTableArgs = async (tableNum) => ?Float32Array;";

