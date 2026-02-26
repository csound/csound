/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
export const csoundStart = (wasm) => (csound) => {
  const result = wasm.exports["csoundStartWasi"](csound);
  return result;
};

csoundStart["toString"] = () => "start = async () => Number;";

// TODO
// csoundCompile (CSOUND *, int argc, const char **argv)

/**
 * Compiles a CSD string but does not perform it.
 * @function
 */
export const csoundCompileCSD =
  (wasm) =>
  (csound, csd, mode = 1) => {
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
export const csoundPerformKsmps = (wasm) => (csound) => wasm.exports["csoundPerformKsmps"](csound);

csoundPerformKsmps["toString"] = () => "performKsmps = async (csound) => Number;";

/**
 * Dummy function to enable stop mechanism
 * @function
 */
export const csoundStop = (wasm) => (csound) => {};

csoundStop["toString"] = () => "stop = async () => undefined;";

/**
 * Prints information about the end of a performance,
 * and closes audio and MIDI devices.
 * @function
 */
export const csoundReset = (wasm) => (csound) => wasm.exports["csoundResetWasi"](csound);

csoundReset["toString"] = () => "reset = async () => Number;";
