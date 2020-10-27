import { freeStringPtr, string2ptr } from "@root/utils";

/* eslint-disable unicorn/prevent-abbreviations */

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

csoundAppendEnv.toString = () => "csoundAppendEnv = async (csound, variable, value) => Number;";

/**
 * Internal function for setting-up browser-fs
 */
export const setupWasmBrowserFS = (wasm) => () => {
  wasm.exports.setupWasmBrowserFS();
};
