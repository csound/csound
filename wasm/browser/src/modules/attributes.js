import { freeStringPtr, string2ptr } from "../utils/string-pointers.js";
import { structBufferToObject } from "../utils/structure-buffer-to-object.js";
import { sizeOfStruct } from "../utils/native-sizes.js";
import { CSOUND_PARAMS } from "../structures.js";

/*
   csound attribute module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

/**
 * Returns the sample rate from Csound instance
 * @function
 */
export const csoundGetSr = (wasm) => (csound) => wasm.exports["csoundGetSr"](csound);

csoundGetSr["toString"] = () => "getSr = async () => Number;";

/**
 * Sets or gets the system (hardware) sample rate.
 * If val > 0, sets the system sr. Returns the stored system sr.
 * @function
 */
export const csoundSystemSr = (wasm) => (csound, val) => wasm.exports["csoundSystemSr"](csound, val);

csoundSystemSr["toString"] = () => "systemSr = async (val) => Number;";

/**
 * Returns the control rate from Csound instance
 * @function
 */
export const csoundGetKr = (wasm) => (csound) => wasm.exports["csoundGetKr"](csound);

csoundGetKr["toString"] = () => "getKr = async () => Number;";

/**
 * Returns the ksmps value (kr/sr) from Csound instance
 * @function
 */
export const csoundGetKsmps = (wasm) => (csound) => wasm.exports["csoundGetKsmps"](csound);

csoundGetKsmps["toString"] = () => "getKsmps = async () => Number;";

/**
 * Returns the number of output channels from Csound instance
 * @function
 */
export const csoundGetChannels = (wasm) => (csound, mode) => wasm.exports["csoundGetChannels"](csound, mode = 0);

csoundGetChannels["toString"] = () => "getChannels = async (mode) => Number;";

/**
 * Returns the value of csoundGet0dBFS
 * @function
 */
export const csoundGet0dBFS = (wasm) => (csound) => wasm.exports["csoundGet0dBFS"](csound);

csoundGet0dBFS["toString"] = () => "get0dBFS = async () => Number;";

/**
 * Returns the A4 frequency reference
 * @function
 */
export const csoundGetA4 = (wasm) => (csound) => wasm.exports["csoundGetA4"](csound);

csoundGetA4["toString"] = () => "getA4 = async () => Number;";

/**
 * Return the current performance time in samples
 * @function
 */
export const csoundGetCurrentTimeSamples = (wasm) => (csound) =>
  wasm.exports["csoundGetCurrentTimeSamples"](csound);

csoundGetCurrentTimeSamples["toString"] = () => "getCurrentTimeSamples = async () => Number;";

/**
 * Return the size of MYFLT in number of bytes
 * @function
 */
export const csoundGetSizeOfMYFLT = (wasm) => (csound) =>
  wasm.exports["csoundGetSizeOfMYFLT"](csound);

csoundGetSizeOfMYFLT["toString"] = () => "getSizeOfMYFLT = async () => Number;";

// TODO (do these make any sense in wasm?)
// csoundGetHostData
// csoundSetHostData

/**
 * Set a single csound option (flag),
 * no spaces are allowed in the string.
 * @function
 */
export const csoundSetOption = (wasm) => (csound, option) => {
  const stringPtr = string2ptr(wasm, option);
  const result = wasm.exports["csoundSetOption"](csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundSetOption["toString"] = () => "setOption = async (option) => Number;";

/**
 * Returns whether Csound is set to print debug messages
 * sent through the DebugMsg() internal API function.
 * Anything different to 0 means true.
 * @function
 */
export const csoundGetDebug = (wasm) => (csound) => wasm.exports["csoundGetDebug"](csound);

csoundGetDebug["toString"] = () => "getDebug = async () => Number;";

/**
 * Return the size of MYFLT in number of bytes
 * @function
 */
export const csoundSetDebug = (wasm) => (csound, debug) => {
  wasm.exports["csoundSetDebug"](csound, debug);
};

csoundSetDebug["toString"] = () => "setDebug = async (number) => undefined;";
