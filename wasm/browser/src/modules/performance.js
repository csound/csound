import { freeStringPtr, string2ptr } from "../utils/string-pointers.js";

/*
   csound performance module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

/**
 * Parses a csound orchestra string
 * @function
 */
export const csoundParseOrc = (wasm) => (csound, orc) => {
  const stringPtr = string2ptr(wasm, orc);
  const resultPtr = wasm.exports["csoundParseOrc"](csound, stringPtr);

  // const { buffer } = wasm.wasi.memory;
  // const dataView = new DataView(buffer);

  freeStringPtr(wasm, stringPtr);

  return resultPtr;
};

csoundParseOrc["toString"] = () => "parseOrc = async (orchestra) => Object;";

/**
 * Compiles AST tree
 * @function
 */
export const csoundCompileTree = (wasm) => (csound, tree) =>
  wasm.exports["csoundCompileTree"](csound, tree);

csoundCompileTree["toString"] = () => "compileTree = async (tree) => Number;";

// TODO
// csoundDeleteTree (CSOUND *csound, TREE *tree)

/**
 * Compiles a csound orchestra string
 * @function
 */
export const csoundCompileOrc = (wasm) => (csound, orc) => {
  const stringPtr = string2ptr(wasm, orc);
  const result = wasm.exports["csoundCompileOrc"](csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundCompileOrc["toString"] = () => "compileOrc = async (orchestra) => Number;";

/**
 * Compiles a csound orchestra string
 * @function
 */
export const csoundEvalCode = (wasm) => (csound, orc) => {
  const stringPtr = string2ptr(wasm, orc);
  const result = wasm.exports["csoundEvalCode"](csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundEvalCode["toString"] = () => "csoundEvalCode = async (orchestra) => Number;";

// TODO
// csoundInitializeCscore (CSOUND *, FILE *insco, FILE *outsco)

// TODO
// csoundCompileArgs (CSOUND *, int argc, const char **argv)

/**
 * Prepares Csound for performance
 * @function
 */
export const csoundStart = (wasm) => (csound) => wasm.exports["csoundStartWasi"](csound);

csoundStart["toString"] = () => "start = async () => Number;";

// TODO
// csoundCompile (CSOUND *, int argc, const char **argv)



/**
 * Compiles a CSD string but does not perform it.
 * @function
 */
export const csoundCompileCSD = (wasm) => (csound, csd, mode = 1) => {
  const stringPtr = string2ptr(wasm, csd);
  const result = wasm.exports["csoundCompileCSD"](csound, stringPtr, mode, 0);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundCompileCSD["toString"] = () => "compileCSD = async (csoundDocument) => Number;";

/**
 * Performs(plays) 1 ksmps worth of sample(s)
 * @function
 */
export const csoundPerformKsmps = (wasm) => (csound) =>
  wasm.exports["csoundPerformKsmpsWasi"](csound);

csoundPerformKsmps["toString"] = () => "performKsmps = async (csound) => Number;";

/**
 * Prints information about the end of a performance,
 * and closes audio and MIDI devices.
 * @function
 */
export const csoundReset = (wasm) => (csound) => wasm.exports["csoundResetWasi"](csound);

csoundReset["toString"] = () => "reset = async () => Number;";
