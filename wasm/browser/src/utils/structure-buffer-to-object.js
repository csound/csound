goog.provide("csound.utils.structure_buffer_to_object");

goog.require("csound.utils.native_sizes");
goog.require("csound.utils.text_encoders");
goog.require("csound.utils.trim_null");

const structBufferToObject = (jsStruct, buffer) => {
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

csound.utils.struct_buffer_to_object = { structBufferToObject };
