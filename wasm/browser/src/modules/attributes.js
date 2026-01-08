import { freeStringPtr, string2ptr } from "../utils/string-pointers.js";

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
 * Returns the number of audio output and input channels
 * from Csound instance as an object {nchnls, nchnls_i}.
 * @function
 */
export const csoundGetChannels = (wasm) => (csound) => {
  const channels = wasm.exports["csoundGetChannels"](csound);
  // The return value is a 64-bit value with nchnls in lower 32 bits and nchnls_i in upper 32 bits
  const nchnls = channels & 0xFFFFFFFF;
  const nchnls_i = (channels >> 32) & 0xFFFFFFFF;
  return { nchnls, nchnls_i };
};

csoundGetChannels["toString"] = () => "getChannels = async () => {nchnls: Number, nchnls_i: Number};";

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
 * Get the current set of parameters
 * from a Csound instance.
 * Returns a pointer to the OPARMS structure.
 * Note: This returns the native OPARMS pointer, not a JavaScript object.
 * @function
 */
export const csoundGetParams = (wasm) => (csound) => {
  return wasm.exports["csoundGetParams"](csound);
};

csoundGetParams["toString"] = () => "getParams = async () => Number;";

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
