/*
   csound table module from <csound.h>
   https://csound.com/docs/api/group___t_a_b_l_e.html
*/

/**
 * Returns the length of a function table
 * (not including the guard point),
 * or -1 if the table does not exist.
 * @async
 * @function
 * @name tableLength
 * @memberof CsoundObj
 * @param {string} tableNum
 * @return {Promise.<number>}
 */
export const csoundTableLength = (wasm) => (csound, tableNum) =>
  wasm.exports.csoundTableLength(csound, tableNum);

csoundTableLength.toString = () => "tableLength = async (tableNum) => Number;";

/**
 * Returns the value of a slot in a function table.
 * The table number and index are assumed to be valid.
 * @async
 * @function
 * @name tableGet
 * @memberof CsoundObj
 * @param {string} tableNum
 * @param {string} tableIndex
 * @return {Promise.<number>}
 */
export const csoundTableGet = (wasm) => (csound, tableNum, tableIndex) =>
  wasm.exports.csoundTableGet(csound, tableNum, tableIndex);

csoundTableGet.toString = () => "tableGet = async (tableNum, tableIndex) => Number;";

/**
 * Sets the value of a slot in a function table.
 * The table number and index are assumed to be valid.
 * @async
 * @function
 * @name tableSet
 * @memberof CsoundObj
 * @param {string} tableNum
 * @param {string} tableIndex
 * @param {string} value
 * @return {Promise.<undefined>}
 */
export const csoundTableSet = (wasm) => (csound, tableNum, tableIndex, value) =>
  wasm.exports.csoundTableSet(csound, tableNum, tableIndex, value);

csoundTableSet.toString = () => "tableSet = async (tableNum, tableIndex, value) => undefined;";

/**
 * Copy the contents of an Array or TypedArray from javascript into a given csound function table.
 * The table number is assumed to be valid, and the table needs to have sufficient space
 * to receive all the array contents.
 * The table number and index are assumed to be valid.
 * @async
 * @function
 * @name tableCopyIn
 * @memberof CsoundObj
 * @param {string} tableNum
 * @param {string} tableIndex
 * @param {Array<number>|ArrayLike<number>} array
 * @return {Promise.<undefined>}
 */
export const csoundTableCopyIn = (wasm) => (csound, tableNum, array) => {
  const arrPtr = wasm.exports.allocFloatArray(array.length);
  const buffer = new Float64Array(wasm.exports.memory.buffer, arrPtr, array.length);
  buffer.set(array);
  wasm.exports.csoundTableCopyIn(csound, tableNum, arrPtr);
  wasm.exports.freeFloatArrayMem(arrPtr);
};

csoundTableCopyIn.toString = () => "tableCopyIn = async (tableNum, float64Array) => undefined;";

/**
 * Copies the contents of a function table from csound into Float64Array.
 * The function returns a Float64Array if the table exists, otherwise
 * it returns undefined.
 * @async
 * @function
 * @name tableCopyOut
 * @memberof CsoundObj
 * @param {string} tableNum
 * @return {Promise.<Float64Array|undefined>}
 */
export const csoundTableCopyOut = (wasm) => (csound, tableNum) => {
  const tableLength = wasm.exports.csoundTableLength(csound, tableNum);
  if (tableLength > 0) {
    const arrPtr = wasm.exports.allocFloatArray(tableLength);
    wasm.exports.csoundTableCopyOut(csound, tableNum, arrPtr);
    const { buffer } = wasm.exports.memory;
    const jsArr = new Float64Array(buffer, arrPtr, tableLength);
    wasm.exports.freeFloatArrayMem(arrPtr);
    return jsArr;
  }
};

csoundTableCopyOut.toString = () => "tableCopyOut = async (tableNum) => ?Float64Array;";

/**
 * @name getTable
 * @alias csoundTableCopyOut
 * @async
 * @function
 * @name tableCopyOut
 * @memberof CsoundObj
 * @param {string} tableNum
 * @return {Promise.<Float64Array|undefined>}
 */
export const csoundGetTable = csoundTableCopyOut;
csoundGetTable.toString = csoundTableCopyOut.toString;

/**
 * Copies the contents of a function table from csound into Float64Array.
 * The function returns a Float64Array if the table exists, otherwise
 * it returns undefined.
 * @async
 * @function
 * @name getTableArgs
 * @memberof CsoundObj
 * @param {string} tableNum
 * @return {Promise.<Float64Array|undefined>}
 */
export const csoundGetTableArgs = (wasm) => (csound, tableNum) => {
  const arrPtr = wasm.exports.allocFloatArray(1024);
  wasm.exports.csoundGetTableArgs(csound, arrPtr, tableNum);
  const { buffer } = wasm.exports.memory;
  const jsArr = new Float64Array(buffer, arrPtr, 1024);
  wasm.exports.freeFloatArrayMem(arrPtr);
  return jsArr;
};

csoundGetTableArgs.toString = () => "getTableArgs = async (tableNum) => ?Float64Array;";

// broken: https://github.com/csound/csound/issues/1422
/**
 * Checks if a given GEN number num is a named GEN if so,
 * it returns the string length (excluding terminating NULL char).
 * Otherwise it returns 0.
 * @async
 * @function
 * @name isNamedGEN
 * @memberof CsoundObj
 * @param {string} tableNum
 * @return {Promise.<number>}
 */
export const csoundIsNamedGEN = (wasm) => (csound, tableNum) =>
  wasm.exports.csoundIsNamedGEN(csound, tableNum);

csoundIsNamedGEN.toString = () => "isNamedGEN = async (tableNum) => number;";

// broken: https://github.com/csound/csound/issues/1422
/**
 * Gets the GEN name from a number num, if this is a named GEN.
 * If the table number doesn't represent a named GEN, it will
 * return undefined.
 * @async
 * @function
 * @name csoundGetNamedGEN
 * @memberof CsoundObj
 * @param {string} tableNum
 * @return {Promise.<string|undefined>}
 */
export const csoundGetNamedGEN = (wasm) => (csound, tableNum) => {
  const stringLength = wasm.exports.csoundIsNamedGEN(csound, tableNum);
  if (stringLength > 0) {
    const offset = wasm.exports.allocStringMem(stringLength);
    wasm.exports.csoundGetNamedGEN(csound, offset, tableNum, stringLength);
    const { buffer } = wasm.exports.memory;
    const stringBuffer = new Uint8Array(buffer, offset, stringLength);
    const result = decoder.decode(stringBuffer);
    return result;
  }
};

csoundGetNamedGEN.toString = () => "getNamedGEN = async (tableNum) => ?string;";
