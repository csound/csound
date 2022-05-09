/* eslint-disable unicorn/prevent-abbreviations */
import { freeStringPtr, string2ptr } from "../utils/string-pointers.js";
import { structBufferToObject } from "../utils/structure-buffer-to-object.js";
import { sizeofStruct } from "../utils/native-sizes.js";
import { CSOUND_PARAMS } from "../structures.js";

/*
   csound attribute module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

/**
 * Returns the sample rate from Csound instance
 * @async
 * @function
 * @name getSr
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGetSr = (wasm) => (csound) => wasm.exports.csoundGetSr(csound);

csoundGetSr.toString = () => "getSr = async () => Number;";

/**
 * Returns the control rate from Csound instance
 * @async
 * @function
 * @name getKr
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGetKr = (wasm) => (csound) => wasm.exports.csoundGetKr(csound);

csoundGetKr.toString = () => "getKr = async () => Number;";

/**
 * Returns the ksmps value (kr/sr) from Csound instance
 * @async
 * @function
 * @name getKsmps
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGetKsmps = (wasm) => (csound) => wasm.exports.csoundGetKsmps(csound);

csoundGetKsmps.toString = () => "getKsmps = async () => Number;";

/**
 * Returns the number of output channels from Csound instance
 * @async
 * @function
 * @name getNchnls
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGetNchnls = (wasm) => (csound) => wasm.exports.csoundGetNchnls(csound);

csoundGetNchnls.toString = () => "getNchnls = async () => Number;";

/**
 * Returns the number of input channels from Csound instance
 * @async
 * @function
 * @name getNchnlsInput
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGetNchnlsInput = (wasm) => (csound) => wasm.exports.csoundGetNchnlsInput(csound);

csoundGetNchnlsInput.toString = () => "getNchnlsInput = async () => Number;";

/**
 * Returns the value of csoundGet0dBFS
 * @async
 * @function
 * @name get0dBFS
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGet0dBFS = (wasm) => (csound) => wasm.exports.csoundGet0dBFS(csound);

csoundGet0dBFS.toString = () => "get0dBFS = async () => Number;";

/**
 * Returns the A4 frequency reference
 * @async
 * @function
 * @name getA4
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGetA4 = (wasm) => (csound) => wasm.exports.csoundGetA4(csound);

csoundGetA4.toString = () => "getA4 = async () => Number;";

/**
 * Return the current performance time in samples
 * @async
 * @function
 * @name getCurrentTimeSamples
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGetCurrentTimeSamples = (wasm) => (csound) =>
  wasm.exports.csoundGetCurrentTimeSamples(csound);

csoundGetCurrentTimeSamples.toString = () => "getCurrentTimeSamples = async () => Number;";

/**
 * Return the size of MYFLT in number of bytes
 * @async
 * @function
 * @name getSizeOfMYFLT
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGetSizeOfMYFLT = (wasm) => (csound) => wasm.exports.csoundGetSizeOfMYFLT(csound);

csoundGetSizeOfMYFLT.toString = () => "getSizeOfMYFLT = async () => Number;";

// TODO (do these make any sense in wasm?)
// csoundGetHostData
// csoundSetHostData

/**
 * Set a single csound option (flag),
 * no spaces are allowed in the string.
 * @async
 * @function
 * @name setOption
 * @memberof CsoundObj
 * @param {string} option
 * @return {Promise.<number>}
 */
export const csoundSetOption = (wasm) => (csound, option) => {
  const stringPtr = string2ptr(wasm, option);
  const result = wasm.exports.csoundSetOption(csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundSetOption.toString = () => "setOption = async (option) => Number;";

/**
 * Configure Csound with a given set of
 * parameters defined in the CSOUND_PARAMS structure.
 * These parameters are the part of the OPARMS struct
 * that are configurable through command line flags.
 * The CSOUND_PARAMS structure can be obtained using
 * csoundGetParams().
 * These options should only be changed before
 * performance has started.
 * @async
 * @function
 * @name setParams
 * @memberof CsoundObj
 * @param {CSOUND_PARAMS} csoundParams - csoundParams object
 * @return {Promise.<undefined>}
 */
export const csoundSetParams = (wasm) => (csound, csoundParameters) => {
  wasm.exports.csoundSetParams(csound, csoundParameters);
};

csoundSetParams.toString = () => "setParams = async (csoundParams) => undefined;";

/**
 * Get the current set of parameters
 * from a Csound instance
 * in a CSOUND_PARAMS structure.
 * @async
 * @function
 * @name getParams
 * @memberof CsoundObj
 * @return {Promise.<CSOUND_PARAMS>} - CSOUND_PARAMS object
 */
export const csoundGetParams = (wasm) => (csound) => {
  const { buffer } = wasm.wasi.memory;
  const structLength = sizeofStruct(CSOUND_PARAMS);
  const structOffset = wasm.exports.allocCsoundParamsStruct();
  const structBuffer = new Uint8Array(buffer, structOffset, structLength);
  wasm.exports.csoundGetParams(csound, structOffset);
  const currentCsoundParameters = structBufferToObject(CSOUND_PARAMS, structBuffer);
  wasm.exports.freeCsoundParams(structOffset);
  return currentCsoundParameters;
};

csoundGetParams.toString = () => "getParams = async () => CSOUND_PARAMS;";

/**
 * Returns whether Csound is set to print debug messages
 * sent through the DebugMsg() internal API function.
 * Anything different to 0 means true.
 * @async
 * @function
 * @name getDebug
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGetDebug = (wasm) => (csound) => wasm.exports.csoundGetDebug(csound);

csoundGetDebug.toString = () => "getDebug = async () => Number;";

/**
 * Return the size of MYFLT in number of bytes
 * @async
 * @function
 * @name setDebug
 * @memberof CsoundObj
 * @param {number} debug
 * @return {Promise.<undefined>}
 */
export const csoundSetDebug = (wasm) => (csound, debug) => {
  wasm.exports.csoundSetDebug(csound, debug);
};

csoundSetDebug.toString = () => "setDebug = async (number) => undefined;";
