import { TREE, ORCTOKEN, CS_MIDIDEVICE, CSOUND_PARAMS } from "../structures.js";

export const sizeOfPrimitive = {
  int: 4,
  uint64: 8,
  MYFLT: 8,
  char: 1,
  double: 8,
  ptr: 4,
  void: 8,
};

export const sizeOfStruct = (jsStruct) =>
  jsStruct
    ? jsStruct.reduce((accumulator, [_, primitive]) => {
        return sizeOfPrimitive[primitive]
          ? sizeOfPrimitive[primitive] + accumulator
          : sizeOfStruct({ TREE, ORCTOKEN }[primitive]) + accumulator;
      }, 0)
    : 0;

export const TREE_SIZE = sizeOfStruct(TREE);
export const ORCTOKEN_SIZE = sizeOfStruct(ORCTOKEN);
export const CSOUND_PARAMS_SIZE = sizeOfStruct(CSOUND_PARAMS);
export const CS_MIDIDEVICE_SIZE = sizeOfStruct(CS_MIDIDEVICE);
