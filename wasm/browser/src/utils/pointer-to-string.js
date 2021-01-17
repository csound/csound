import { uint2String } from "@utils/text-encoders";
import { trimNull } from "@utils/trim-null";

export const ptr2string = (wasm, stringPtr) => {
  const { buffer } = wasm.exports.memory;
  const intArray = new Uint8Array(buffer, stringPtr);
  const result = uint2String(intArray);
  return trimNull(result);
};
