goog.provide("csound.utils.string_pointers");
goog.require("csound.utils.trim_null");
goog.require("csound.utils.text_encoders");

const freeStringPtr = (wasm, ptr) => {
  wasm.exports.freeStringMem(ptr);
};

const ptr2string = (wasm, stringPtr) => {
  const { buffer } = wasm.exports.memory;
  const intArray = new Uint8Array(buffer, stringPtr);
  const result = uint2String(intArray);
  return trimNull(result);
};

const string2ptr = (wasm, string) => {
  if (typeof string !== "string") {
    console.error("Expected string but got", typeof string);
    return;
  }

  const stringBuf = encoder.encode(string);
  const offset = wasm.exports.allocStringMem(stringBuf.length);
  const { buffer } = wasm.exports.memory;
  const outBuf = new Uint8Array(buffer, offset, stringBuf.length + 1);
  outBuf.set(stringBuf);
  return offset;
};

csound.utils.text_encoders = {
  freeStringPtr,
  ptr2string,
  string2ptr,
};
