import { encoder, uint2String } from "./text-encoders.js";
import { trimNull } from "./trim-null.js";

/**
 * @param {WasmInst} wasm
 * @param {number} ptr
 */
export const freeStringPtr = (wasm, ptr) => {
  wasm.exports.freeStringMem(ptr);
};

export const ptr2string = (wasm, stringPtr) => {
  const { buffer } = wasm.wasi.memory;
  const intArray = new Uint8Array(buffer, stringPtr);
  const result = uint2String(intArray);
  return trimNull(result);
};

export const string2ptr = (wasm, string) => {
  if (typeof string !== "string") {
    console.error("Expected string but got", typeof string);
    return;
  }

  const stringBuf = encoder.encode(string);
  const offset = wasm.exports.allocStringMem(stringBuf.length);
  const { buffer } = wasm.wasi.memory;
  const outBuf = new Uint8Array(buffer, offset, stringBuf.length + 1);
  outBuf.set(stringBuf);
  return offset;
};
