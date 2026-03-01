import { encoder, uint2String } from "./text-encoders.js";
import { trimNull } from "./trim-null.js";

/**
 * @param {WasmInst} wasm
 * @param {number} ptr
 */
export const freeStringPtr = (wasm, ptr) => {
  wasm["exports"]["freeStringMem"](ptr);
};

export const ptr2string = (wasm, stringPtr) => {
  const { buffer } = wasm.wasi.memory;
  const bytes = new Uint8Array(buffer, stringPtr);
  // Find the null terminator so we only decode the actual string,
  // not the entire remaining wasm memory buffer.
  let len = 0;
  while (bytes[len] !== 0) len++;
  if (len === 0) return "";
  const bounded = new Uint8Array(buffer, stringPtr, len);
  return uint2String(bounded);
};

export const string2ptr = (wasm, string) => {
  if (typeof string !== "string") {
    console.error("Expected string but got", typeof string);
    return;
  }

  const stringBuf = encoder.encode(string);
  const offset = wasm["exports"]["allocStringMem"](stringBuf.length);
  const { buffer } = wasm.wasi.memory;
  const outBuf = new Uint8Array(buffer, offset, stringBuf.length + 1);
  outBuf.set(stringBuf);
  return offset;
};
