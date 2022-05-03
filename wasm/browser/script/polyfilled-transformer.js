import { inflate } from "pako";
import wasmTransformerDataURI from "@wasmer/wasm-transformer/lib/wasm-transformer.wasm";
let wasm = null;
let cachegetInt32Memory = null;

function getInt32Memory() {
  if (cachegetInt32Memory === null || cachegetInt32Memory.buffer !== wasm.wasi.memory.buffer) {
    cachegetInt32Memory = new Int32Array(wasm.wasi.memory.buffer);
  }
  return cachegetInt32Memory;
}

let cachedTextDecoder = new TextDecoder("utf-8", { ignoreBOM: true, fatal: true });

let cachegetUint8Memory = null;
function getUint8Memory() {
  if (cachegetUint8Memory === null || cachegetUint8Memory.buffer !== wasm.wasi.memory.buffer) {
    cachegetUint8Memory = new Uint8Array(wasm.wasi.memory.buffer);
  }
  return cachegetUint8Memory;
}

function getStringFromWasm(ptr, len) {
  return cachedTextDecoder.decode(getUint8Memory().subarray(ptr, ptr + len));
}
/**
 * get the versioon of the package
 * @returns {string}
 */
export function version() {
  const retptr = 8;
  const ret = wasm.exports.version(retptr);
  const memi32 = getInt32Memory();
  const v0 = getStringFromWasm(memi32[retptr / 4 + 0], memi32[retptr / 4 + 1]).slice();
  wasm.exports.__wbindgen_free(memi32[retptr / 4 + 0], memi32[retptr / 4 + 1] * 1);
  return v0;
}

let WASM_VECTOR_LEN = 0;

function passArray8ToWasm(arg) {
  const ptr = wasm.exports.__wbindgen_malloc(arg.length * 1);
  getUint8Memory().set(arg, ptr / 1);
  WASM_VECTOR_LEN = arg.length;
  return ptr;
}

function getArrayU8FromWasm(ptr, len) {
  return getUint8Memory().subarray(ptr / 1, ptr / 1 + len);
}
/**
 * i64 lowering that can be done by the browser
 * @param {Uint8Array} wasm_binary
 * @returns {Uint8Array}
 */
export async function lowerI64Imports(wasm_binary) {
  if (!wasm) {
    wasm = await init(wasmTransformerDataURI);
  }
  const retptr = 8;
  const ret = wasm.exports.lowerI64Imports(retptr, passArray8ToWasm(wasm_binary), WASM_VECTOR_LEN);
  const memi32 = getInt32Memory();
  const v0 = getArrayU8FromWasm(memi32[retptr / 4 + 0], memi32[retptr / 4 + 1]).slice();
  wasm.exports.__wbindgen_free(memi32[retptr / 4 + 0], memi32[retptr / 4 + 1] * 1);
  return v0;
}

async function init(module) {
  const imports = {};
  imports.wbg = {};
  imports.wbg.__wbindgen_throw = function (arg0, arg1) {
    throw new Error(getStringFromWasm(arg0, arg1));
  };

  const compiled = await WebAssembly.compile(module);
  wasm = await WebAssembly.instantiate(compiled, imports);
  return wasm;
}

export default init;
