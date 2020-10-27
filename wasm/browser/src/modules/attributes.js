/*
   csound attribute module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

/* eslint-disable unicorn/prevent-abbreviations */

import { freeStringPtr, sizeofStruct, string2ptr, structBuffer2Object } from "@root/utils";
import { CSOUND_PARAMS } from "@root/structures";
import { curry } from "ramda";

/**
 * Returns the sample rate from Csound instance
 * @callback csoundGetSr
 * @param {Csound} csound
 * @return {number}
 */
/**
 * @param {Object} wasm
 * @return {csoundGetSr}
 */
export const csoundGetSr = curry((wasm, csound) => wasm.exports.csoundGetSr(csound));

csoundGetSr.toString = () => "csoundGetSr = async (csound) => Number;";

/**
 * Returns the control rate from Csound instance
 * @callback csoundGetKr
 * @param {Csound} csound
 * @return {number}
 */
/**
 * @param {Object} wasm
 * @return {csoundGetKr}
 */
export const csoundGetKr = curry((wasm, csound) => wasm.exports.csoundGetKr(csound));

csoundGetKr.toString = () => "csoundGetKr = async (csound) => Number;";

/**
 * Returns the ksmps value (kr/sr) from Csound instance
 * @callback csoundGetKsmps
 * @param {Csound} csound
 * @return {number}
 */
/**
 * @param {Object} wasm
 * @return {csoundGetKsmps}
 */
export const csoundGetKsmps = curry((wasm, csound) => wasm.exports.csoundGetKsmps(csound));

csoundGetKsmps.toString = () => "csoundGetKsmps = async (csound) => Number;";

/**
 * Returns the number of output channels from Csound instance
 * @callback csoundGetNchnls
 * @param {Csound} csound
 * @return {number}
 */
/**
 * @param {Object} wasm
 * @return {csoundGetNchnls}
 */
export const csoundGetNchnls = curry((wasm, csound) => wasm.exports.csoundGetNchnls(csound));

csoundGetNchnls.toString = () => "csoundGetNchnls = async (csound) => Number;";

/**
 * Returns the number of input channels from Csound instance
 * @callback csoundGetNchnlsInput
 * @param {Csound} csound
 * @return {number}
 */
/**
 * @param {Object} wasm
 * @return {csoundGetNchnlsInput}
 */
export const csoundGetNchnlsInput = curry((wasm, csound) =>
  wasm.exports.csoundGetNchnlsInput(csound),
);

csoundGetNchnlsInput.toString = () => "csoundGetNchnlsInput = async (csound) => Number;";

/**
 * Returns the value of csoundGet0dBFS
 * @callback csoundGet0dBFS
 * @param {Csound} csound
 * @return {number}
 */
/**
 * @param {Object} wasm
 * @return {csoundGet0dBFS}
 */
export const csoundGet0dBFS = curry((wasm, csound) => wasm.exports.csoundGet0dBFS(csound));

csoundGet0dBFS.toString = () => "csoundGet0dBFS = async (csound) => Number;";

/**
 * Returns the A4 frequency reference
 * @callback csoundGetA4
 * @param {Csound} csound
 * @return {number}
 */
/**
 * @param {Object} wasm
 * @return {csoundGetA4}
 */
export const csoundGetA4 = curry((wasm, csound) => wasm.exports.csoundGetA4(csound));

csoundGetA4.toString = () => "csoundGetA4 = async (csound) => Number;";

/**
 * Return the current performance time in samples
 * @callback csoundGetCurrentTimeSamples
 * @param {Csound} csound
 * @return {number}
 */
/**
 * @param {Object} wasm
 * @return {csoundGetCurrentTimeSamples}
 */
export const csoundGetCurrentTimeSamples = curry((wasm, csound) =>
  wasm.exports.csoundGetCurrentTimeSamples(csound),
);

csoundGetCurrentTimeSamples.toString = () =>
  "csoundGetCurrentTimeSamples = async (csound) => Number;";

/**
 * Return the size of MYFLT in number of bytes
 * @callback csoundGetSizeOfMYFLT
 * @param {Csound} csound
 * @return {number}
 */
/**
 * @param {Object} wasm
 * @return {csoundGetSizeOfMYFLT}
 */
export const csoundGetSizeOfMYFLT = curry((wasm, csound) =>
  wasm.exports.csoundGetSizeOfMYFLT(csound),
);

csoundGetSizeOfMYFLT.toString = () => "csoundGetSizeOfMYFLT = async (csound) => Number;";

// TODO (do these make any sense in wasm?)
// csoundGetHostData
// csoundSetHostData

/**
 * Set a single csound option (flag),
 * no spaces are allowed in the string.
 * @callback csoundSetOption
 * @param {Csound} csound
 * @return {number}
 */
/**
 * @param {Object} wasm
 * @return {csoundSetOption}
 */
export const csoundSetOption = curry((wasm, csound, option) => {
  const stringPtr = string2ptr(wasm, option);
  const result = wasm.exports.csoundSetOption(csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
});

csoundSetOption.toString = () => "csoundSetOption = async (csound, option) => Number;";

/**
 * Configure Csound with a given set of
 * parameters defined in the CSOUND_PARAMS structure.
 * These parameters are the part of the OPARMS struct
 * that are configurable through command line flags.
 * The CSOUND_PARAMS structure can be obtained using
 * csoundGetParams().
 * These options should only be changed before
 * performance has started.
 * @callback csoundSetParams
 * @param {Csound} csound
 * @param {Object} csoundParams
 * @return {undefined}
 */
/**
 * @param {Object} wasm
 * @return {csoundSetParams}
 */
export const csoundSetParams = curry((wasm, csound, csoundParameters) => {
  wasm.exports.csoundSetParams(csound, csoundParameters);
});

csoundSetParams.toString = () => "csoundSetParams = async (csound, csoundParams) => undefined;";

/**
 * Get the current set of parameters
 * from a Csound instance
 * in a CSOUND_PARAMS structure.
 * @callback csoundGetParams
 * @param {Csound} csound
 * @return {Object} - CSOUND_PARAMS object
 */
/**
 * @param {Object} wasm
 * @return {csoundGetParams}
 */
export const csoundGetParams = curry((wasm, csound) => {
  const { buffer } = wasm.exports.memory;
  const structLength = sizeofStruct(CSOUND_PARAMS);
  const structOffset = wasm.exports.allocCsoundParamsStruct();
  const structBuffer = new Uint8Array(buffer, structOffset, structLength);
  wasm.exports.csoundGetParams(csound, structOffset);
  const currentCsoundParameters = structBuffer2Object(CSOUND_PARAMS, structBuffer);
  wasm.exports.freeCsoundParams(structOffset);
  return currentCsoundParameters;
});

csoundGetParams.toString = () => "csoundGetParams = async (csound) => CSOUND_PARAMS;";

/**
 * Returns whether Csound is set to print debug messages
 * sent through the DebugMsg() internal API function.
 * Anything different to 0 means true.
 * @callback csoundGetDebug
 * @param {Csound} csound
 * @return {number}
 */
/**
 * @param {Object} wasm
 * @return {csoundGetDebug}
 */
export const csoundGetDebug = curry((wasm, csound) => wasm.exports.csoundGetDebug(csound));

csoundGetDebug.toString = () => "csoundGetDebug = async (csound) => Number;";

/**
 * Return the size of MYFLT in number of bytes
 * @callback csoundSetDebug
 * @param {Csound} csound
 * @param {number} debug
 * @return {undefined}
 */
/**
 * @param {Object} wasm
 * @return {csoundSetDebug}
 */
export const csoundSetDebug = curry((wasm, csound, debug) => {
  wasm.exports.csoundSetDebug(csound, debug);
});

csoundSetDebug.toString = () => "csoundSetDebug = async (csound, number) => undefined;";
