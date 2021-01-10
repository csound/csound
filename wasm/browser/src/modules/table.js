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
 * @return {Promise.<number>}
 */
export const csoundTableSet = (wasm) => (csound, tableNum, tableIndex, value) =>
  wasm.exports.csoundTableSet(csound, tableNum, tableIndex, value);

csoundTableSet.toString = () => "tableSet = async (tableNum, tableIndex, value) => Number;";

/**
 * Copy the contents of a Float64Array from javascript into a given csound function table.
 * The table number is assumed to be valid, and the table needs to have sufficient space
 * to receive all the array contents.
 * The table number and index are assumed to be valid.
 * @async
 * @function
 * @name tableCopyIn
 * @memberof CsoundObj
 * @param {string} tableNum
 * @param {string} tableIndex
 * @param {string} value
 * @return {Promise.<undefined>}
 */
export const csoundTableCopyIn = (wasm) => (csound, tableNum, float64Array) => {
  const arrPtr = wasm.exports.allocFloatArray(float64Array.length);
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

// PUBLIC void 	csoundTableCopyOut (CSOUND *csound, int table, MYFLT *dest)
// PUBLIC void 	csoundTableCopyOutAsync (CSOUND *csound, int table, MYFLT *dest)

// PUBLIC int 	csoundGetTable (CSOUND *, MYFLT **tablePtr, int tableNum)
// PUBLIC int 	csoundGetTableArgs (CSOUND *csound, MYFLT **argsPtr, int tableNum)
// PUBLIC int 	csoundIsNamedGEN (CSOUND *csound, int num)
// PUBLIC void 	csoundGetNamedGEN (CSOUND *csound, int num, char *name, int len)
