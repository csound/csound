/* eslint-disable unicorn/prevent-abbreviations */
import { freeStringPtr, string2ptr } from "../utils/string-pointers.js";

/**
 * Append 'value' to environment variable 'name'
 * added for internal usage of csound-wasm, feel
 * free to use as well ;)
 */
export const csoundAppendEnv = (wasm) => (csound, variable, value) => {
  const varStringPtr = string2ptr(wasm, variable);
  const valueStringPtr = string2ptr(wasm, value);
  const res = wasm.exports.csoundAppendEnv(csound, varStringPtr, valueStringPtr);
  freeStringPtr(wasm, varStringPtr);
  freeStringPtr(wasm, valueStringPtr);
  return res;
};

csoundAppendEnv.toString = () => "appendEnv = async (csound, variable, value) => Number;";

// deliberately no jsdocs because this is internal only
export const csoundShouldDaemonize = (wasm) => (csound) =>
  wasm.exports.csoundShouldDaemonize(csound);
