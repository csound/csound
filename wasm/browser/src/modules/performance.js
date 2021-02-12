/*
   csound performance module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

import { freeStringPtr, string2ptr } from "@utils/string-pointers";

/**
 * Parses a csound orchestra string
 * @async
 * @function
 * @name CsoundObj#parseOrc
 * @param {string} orc
 * @return {Promise.<object>}
 */
export const csoundParseOrc = (wasm) => (csound, orc) => wasm.exports.csoundParseOrc(csound, orc);

csoundParseOrc.toString = () => "parseOrc = async (orchestra) => Object;";

/**
 * Compiles AST tree
 * @async
 * @function
 * @name CsoundObj#compileTree
 * @param {object} tree
 * @return {Promise.<number>}
 */
export const csoundCompileTree = (wasm) => (csound, tree) =>
  wasm.exports.csoundCompileTree(csound, tree);

csoundCompileTree.toString = () => "compileTree = async (tree) => Number;";

// TODO
// csoundDeleteTree (CSOUND *csound, TREE *tree)

/**
 * Compiles a csound orchestra string
 * @async
 * @function
 * @name CsoundObj#compileOrc
 * @param {string} orc
 * @return {Promise.<number>}
 */
export const csoundCompileOrc = (wasm) => (csound, orc) => {
  const stringPtr = string2ptr(wasm, orc);
  const result = wasm.exports.csoundCompileOrc(csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundCompileOrc.toString = () => "compileOrc = async (orchestra) => Number;";

/**
 * Compiles a csound orchestra string
 * @async
 * @function
 * @name CsoundObj#evalCode
 * @param {string} orc
 * @return {Promise.<number>}
 */
export const csoundEvalCode = (wasm) => (csound, orc) => {
  const stringPtr = string2ptr(wasm, orc);
  const result = wasm.exports.csoundEvalCode(csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundEvalCode.toString = () => "csoundEvalCode = async (orchestra) => Number;";

// TODO
// csoundInitializeCscore (CSOUND *, FILE *insco, FILE *outsco)

// TODO
// csoundCompileArgs (CSOUND *, int argc, const char **argv)

/**
 * Prepares Csound for performance
 * @async
 * @function
 * @name CsoundObj#start
 * @return {Promise.<number>}
 */
export const csoundStart = (wasm) => (csound) => wasm.exports.csoundStartWasi(csound);

csoundStart.toString = () => "start = async () => Number;";

// TODO
// csoundCompile (CSOUND *, int argc, const char **argv)

/**
 * Compiles a Csound input file but does not perform it.
 * @async
 * @function
 * @name CsoundObj#compileCsd
 * @param {string} path
 * @return {Promise.<number>}
 */
export const csoundCompileCsd = (wasm) => (csound, path) => {
  const sandboxedPath = /^\//i.test(path) ? `/sandbox${path}` : `/sandbox/${path}`;
  const stringPtr = string2ptr(wasm, sandboxedPath);
  const result = wasm.exports.csoundCompileCsd(csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundCompileCsd.toString = () => "compileCsd = async (path) => Number;";

/**
 * Compiles a CSD string but does not perform it.
 * @async
 * @function
 * @name CsoundObj#compileCsdText
 * @param {string} orc
 * @return {Promise.<number>}
 */
export const csoundCompileCsdText = (wasm) => (csound, orc) => {
  const stringPtr = string2ptr(wasm, orc);
  const result = wasm.exports.csoundCompileCsdText(csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundCompileCsdText.toString = () => "compileCsdText = async (csoundDocument) => Number;";

/**
 * Performs(plays) audio until end is reached
 * @async
 * @function
 * @name CsoundObj#perform
 * @return {Promise.<number>}
 */
export const csoundPerform = (wasm) => (csound) => wasm.exports.csoundPerform(csound);

csoundPerform.toString = () => "perform = async () => Number;";

/**
 * Performs(plays) 1 ksmps worth of sample(s)
 * @async
 * @function
 * @name CsoundObj#performKsmps
 * @return {Promise.<number>}
 */
export const csoundPerformKsmps = (wasm) => (csound) => wasm.exports.csoundPerformKsmpsWasi(csound);

csoundPerformKsmps.toString = () => "performKsmps = async (csound) => Number;";

/**
 * Performs(plays) 1 buffer worth of audio
 * @async
 * @function
 * @name CsoundObj#performBuffer
 * @return {Promise.<number>}
 */
export const csoundPerformBuffer = (wasm) => (csound) => wasm.exports.csoundPerformBuffer(csound);

csoundPerformBuffer.toString = () => "performBuffer = async (csound) => Number;";

/**
 * Stops a csoundPerform
 * @async
 * @function
 * @name CsoundObj#stop
 * @return {Promise.<undefined>}
 */
export const csoundStop = (wasm) => (csound) => wasm.exports.csoundStop(csound);

csoundStop.toString = () => "stop = async () => undefined;";

/**
 * Prints information about the end of a performance,
 * and closes audio and MIDI devices.
 * @async
 * @function
 * @name CsoundObj#cleanup
 * @return {Promise.<number>}
 */
export const csoundCleanup = (wasm) => (csound) => wasm.exports.csoundCleanup(csound);

csoundCleanup.toString = () => "cleanup = async () => Number;";

/**
 * Prints information about the end of a performance,
 * and closes audio and MIDI devices.
 * @async
 * @function
 * @name CsoundObj#reset
 * @return {Promise.<number>}
 */
export const csoundReset = (wasm) => (csound) => wasm.exports.csoundResetWasi(csound);

csoundReset.toString = () => "reset = async () => Number;";
