/**
 * libcsound — Lightweight in-process Csound constructor.
 *
 * Returns a synchronous, C-library-like API object.
 * No AudioContext or AudioWorklet dependency required.
 * Each call to libcsound() creates an independent wasm instance.
 *
 * Usage:
 *   import { Csound, libcsound } from "@csound/browser";
 *   const cs = await libcsound();
 *   const csound = cs.csoundCreate();
 *   cs.csoundSetOption(csound, "-d");
 *   cs.csoundSetOption(csound, "--nchnls=2");
 *   cs.csoundSetOption(csound, "--0dbfs=1");
 *   cs.csoundCompileOrc(csound, "instr 1\n  out vco2(0.5, 440)\nendin");
 *   cs.csoundStart(csound);
 *   cs.csoundPerformKsmps(csound);
 *
 *   // UGEN API:
 *   const factory = cs.csoundUgenFactoryNew(csound);
 *   const osc = cs.csoundUgenNew(factory, "vco2", "a", "kk");
 *   cs.csoundUgenSetValue(osc, 0, 0.5);  // amplitude
 *   cs.csoundUgenSetValue(osc, 1, 440);  // frequency
 *   cs.csoundUgenInit(osc);
 *   cs.csoundUgenPerform(osc);
 *   const samples = cs.csoundUgenVarGetFloat64Array(
 *     cs.csoundUgenGetOutVar(osc, 0)
 *   );
 */

import wasmDataURI from "../dist/__csound_wasm.inline.js";
import loadWasm from "./module";
import libcsoundFactory from "./libcsound";

/**
 * Creates a new, independent Csound wasm instance with full API access.
 * The returned object exposes synchronous functions matching the C API.
 * @function
 */
const libcsound = async function ({ withPlugins = [] } = {}) {
  // Minimal message port that prints to console
  const messagePort = {
    post: (msg) => {
      if (msg && msg.log) {
        console.log(msg.log);
      }
    },
  };

  const [wasm, wasi] = await loadWasm({
    wasmDataURI: wasmDataURI(),
    withPlugins,
    messagePort,
  });

  wasm.wasi = wasi;

  // Get all bound API functions from the existing libcsound factory
  const api = libcsoundFactory(wasm);

  // Expose wasm internals for advanced use (memory access, etc.)
  api["wasm"] = wasm;

  // Convenience: expose memory for Float64Array views
  api["getMemory"] = () => wasm.exports["memory"];

  return api;
};

export default libcsound;
