import { uint2String } from "../utils/text-encoders.js";
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
    const arrayPtr = wasm.exports["allocFloatArray"](array.length);
    const buffer = new Float64Array(wasm.wasi.memory.buffer, arrayPtr, array.length);
    buffer.set(array);
    wasm.exports["csoundTableCopyIn"](csound, tableNumber, arrayPtr);
    wasm.exports["freeFloatArrayMem"](arrayPtr);
  };

csoundTableCopyIn.toString = () => "tableCopyIn = async (tableNum, float64Array) => undefined;";

/**
 * Copies the contents of a function table from csound into Float64Array.
 * The function returns a Float64Array if the table exists, otherwise
 * it returns undefined.
 * @function
 */
export const csoundTableCopyOut = (wasm) => (csound /* CsoundInst */, tableNumber /* string */) => {
  const tableLength = wasm.exports["csoundTableLength"](csound, tableNumber);
  if (tableLength > 0) {
    const arrayPtr = wasm.exports["allocFloatArray"](tableLength);
    wasm.exports["csoundTableCopyOut"](csound, tableNumber, arrayPtr);
    const { buffer } = wasm.wasi.memory;
    const freeableArray = new Float64Array(buffer, arrayPtr, tableLength);
    const clonedArray = new Float64Array(freeableArray.length);
    /* eslint-disable-next-line unicorn/no-for-loop */
    for (let i = 0; i < freeableArray.length; i++) {
      clonedArray[i] = freeableArray[i];
    }
    wasm.exports["freeFloatArrayMem"](arrayPtr);
    return clonedArray;
  }
};

csoundTableCopyOut.toString = () => "tableCopyOut = async (tableNum) => ?Float64Array;";

/**
 * @name getTable
 * @alias csoundTableCopyOut
 * @async
 * @function
 */
export const csoundGetTable = csoundTableCopyOut;
csoundGetTable.toString = csoundTableCopyOut.toString;

/**
 * Copies the contents of a function table from csound into Float64Array.
 * The function returns a Float64Array if the table exists, otherwise
 * it returns undefined.
 */
export const csoundGetTableArgs = (wasm) => (csound /* CsoundInst */, tableNumber /* string */) => {
  const arrayPtr = wasm.exports["allocFloatArray"](1024);
  wasm.exports["csoundGetTableArgs"](csound, arrayPtr, tableNumber);
  const { buffer } = wasm.wasi.memory;
  const jsArray = new Float64Array(buffer, arrayPtr, 1024);
  wasm.exports["freeFloatArrayMem"](arrayPtr);
  return jsArray;
};

csoundGetTableArgs["toString"] = () => "getTableArgs = async (tableNum) => ?Float64Array;";
