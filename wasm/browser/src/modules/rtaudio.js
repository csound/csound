/*
   Realtime Audio I/O module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

/**
 * Returns the address of the Csound audio input working buffer (spin).
 * Enables external software to write audio into Csound before calling csoundPerformKsmps.
 * @function
 */
export const csoundGetSpin = (wasm /* WasmInst */) => (csound /* CsoundInst */) =>
  wasm.exports["csoundGetSpin"](csound);

csoundGetSpin["toString"] = () => "getSpin = async (csound) => Number;";

/**
 * Returns the address of the Csound audio output working buffer (spout).
 * Enables external software to read audio from Csound after calling csoundPerformKsmps.
 * @function
 */
export const csoundGetSpout = (wasm /* WasmInst */) => (csound /* CsoundInst */) =>
  wasm.exports["csoundGetSpout"](csound);

csoundGetSpout["toString"] = () => "getSpout = async () => Number;";

