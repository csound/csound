export const sizeOfPrimitive = {
  int: 4,
  MYFLT: 4,
  char: 1,
};

export const sizeofStruct = (jsStruct) => {
  const result = jsStruct.reduce((total, [_, primitive, ...rest]) => {
    return (total +=
      primitive === "char" ? sizeOfPrimitive[primitive] * rest[0] : sizeOfPrimitive[primitive]);
  }, 0);
  return result;
};
