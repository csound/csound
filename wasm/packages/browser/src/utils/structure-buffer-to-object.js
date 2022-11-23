import { sizeOfPrimitive } from "./native-sizes.js";
import { uint2String } from "./text-encoders.js";
import { trimNull } from "./trim-null.js";

export const structBufferToObject = (jsStruct, buffer) => {
  const [result] = jsStruct.reduce(
    ([parameters, offset], [parameterName, primitive, ...rest]) => {
      const currentSize =
        primitive === "char" ? sizeOfPrimitive[primitive] * rest[0] : sizeOfPrimitive[primitive];
      const currentValue =
        primitive === "char"
          ? trimNull(uint2String(buffer.subarray(offset, currentSize))) || ""
          : buffer[offset];
      parameters[parameterName] = currentValue;
      return [parameters, offset + currentSize];
    },
    [{}, 0],
  );
  return result;
};
