import { TREE, ORCTOKEN, CS_MIDIDEVICE, CSOUND_PARAMS } from "../structures.js";

const valueOfToken = { ROOT: 0 };

const tokenOfValue = { 0: "ROOT" };

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
  jsStruct.reduce((accumulator, [_, primitive]) => {
    return sizeOfPrimitive[primitive]
      ? sizeOfPrimitive[primitive] + accumulator
      : sizeOfStruct({ TREE, ORCTOKEN }[primitive]) + accumulator;
  }, 0);

export const TREE_SIZE = sizeOfStruct(TREE);
export const ORCTOKEN_SIZE = sizeOfStruct(ORCTOKEN);
export const CSOUND_PARAMS_SIZE = sizeOfStruct(CSOUND_PARAMS);
export const CS_MIDIDEVICE_SIZE = sizeOfStruct(CS_MIDIDEVICE);

export const importValuesOfTokens = (wasm) => {
  valueOfToken.INSTR_TOKEN = wasm.exports.valueOf_INSTR_TOKEN();

  valueOfToken.NEWLINE = wasm.exports.valueOf_NEWLINE();
  valueOfToken.S_NEQ = wasm.exports.valueOf_S_NEQ();

  valueOfToken.S_AND = wasm.exports.valueOf_S_AND();
  valueOfToken.S_OR = wasm.exports.valueOf_S_OR();
  valueOfToken.S_LT = wasm.exports.valueOf_S_LT();
  valueOfToken.S_LE = wasm.exports.valueOf_S_LE();
  valueOfToken.S_EQ = wasm.exports.valueOf_S_EQ();
  valueOfToken.S_ADDIN = wasm.exports.valueOf_S_ADDIN();
  valueOfToken.S_SUBIN = wasm.exports.valueOf_S_SUBIN();
  valueOfToken.S_MULIN = wasm.exports.valueOf_S_MULIN();
  valueOfToken.S_DIVIN = wasm.exports.valueOf_S_DIVIN();
  valueOfToken.S_GT = wasm.exports.valueOf_S_GT();
  valueOfToken.S_GE = wasm.exports.valueOf_S_GE();
  valueOfToken.S_BITSHIFT_LEFT = wasm.exports.valueOf_S_BITSHIFT_LEFT();
  valueOfToken.S_BITSHIFT_RRIGHT = wasm.exports.valueOf_S_BITSHIFT_RRIGHT();
  valueOfToken.LABEL_TOKEN = wasm.exports.valueOf_LABEL_TOKEN();

  valueOfToken.IF_TOKEN = wasm.exports.valueOf_IF_TOKEN();
  valueOfToken.DECLARE_TOKEN = wasm.exports.valueOf_DECLARE_TOKEN();
  valueOfToken.UDO_TOKEN = wasm.exports.valueOf_UDO_TOKEN();
  valueOfToken.UDOSTART_DEFINITION = wasm.exports.valueOf_UDOSTART_DEFINITION();
  valueOfToken.UDOEND_TOKEN = wasm.exports.valueOf_UDOEND_TOKEN();
  valueOfToken.UDO_ANS_TOKEN = wasm.exports.valueOf_UDO_ANS_TOKEN();
  valueOfToken.UDO_ARGS_TOKEN = wasm.exports.valueOf_UDO_ARGS_TOKEN();
  valueOfToken.UDO_IDENT = wasm.exports.valueOf_UDO_IDENT();
  valueOfToken.VOID_TOKEN = wasm.exports.valueOf_VOID_TOKEN();
  valueOfToken.ERROR_TOKEN = wasm.exports.valueOf_ERROR_TOKEN();
  valueOfToken.T_OPCALL = wasm.exports.valueOf_T_OPCALL();
  valueOfToken.T_FUNCTION = wasm.exports.valueOf_T_FUNCTION();
  valueOfToken.T_ASSIGNMENT = wasm.exports.valueOf_T_ASSIGNMENT();
  valueOfToken.STRUCT_TOKEN = wasm.exports.valueOf_STRUCT_TOKEN();
  valueOfToken.INSTR_TOKEN = wasm.exports.valueOf_INSTR_TOKEN();

  valueOfToken.ENDIN_TOKEN = wasm.exports.valueOf_ENDIN_TOKEN();
  valueOfToken.GOTO_TOKEN = wasm.exports.valueOf_GOTO_TOKEN();
  valueOfToken.KGOTO_TOKEN = wasm.exports.valueOf_KGOTO_TOKEN();
  valueOfToken.IGOTO_TOKEN = wasm.exports.valueOf_IGOTO_TOKEN();
  valueOfToken.STRING_TOKEN = wasm.exports.valueOf_STRING_TOKEN();
  valueOfToken.T_IDENT = wasm.exports.valueOf_T_IDENT();
  valueOfToken.T_TYPED_IDENT = wasm.exports.valueOf_T_TYPED_IDENT();
  valueOfToken.T_PLUS_IDENT = wasm.exports.valueOf_T_PLUS_IDENT();
  valueOfToken.INTEGER_TOKEN = wasm.exports.valueOf_INTEGER_TOKEN();
  valueOfToken.NUMBER_TOKEN = wasm.exports.valueOf_NUMBER_TOKEN();
  valueOfToken.THEN_TOKEN = wasm.exports.valueOf_THEN_TOKEN();
  valueOfToken.ITHEN_TOKEN = wasm.exports.valueOf_ITHEN_TOKEN();
  valueOfToken.KTHEN_TOKEN = wasm.exports.valueOf_KTHEN_TOKEN();
  valueOfToken.ELSEIF_TOKEN = wasm.exports.valueOf_ELSEIF_TOKEN();

  valueOfToken.ELSE_TOKEN = wasm.exports.valueOf_ELSE_TOKEN();
  valueOfToken.ENDIF_TOKEN = wasm.exports.valueOf_ENDIF_TOKEN();
  valueOfToken.UNTIL_TOKEN = wasm.exports.valueOf_UNTIL_TOKEN();
  valueOfToken.WHILE_TOKEN = wasm.exports.valueOf_WHILE_TOKEN();
  valueOfToken.DO_TOKEN = wasm.exports.valueOf_DO_TOKEN();
  valueOfToken.OD_TOKEN = wasm.exports.valueOf_OD_TOKEN();
  valueOfToken.S_ELIPSIS = wasm.exports.valueOf_S_ELIPSIS();
  valueOfToken.T_ARRAY = wasm.exports.valueOf_T_ARRAY();

  valueOfToken.T_ARRAY_IDENT = wasm.exports.valueOf_T_ARRAY_IDENT();
  valueOfToken.T_DECLARE = wasm.exports.valueOf_T_DECLARE();
  valueOfToken.STRUCT_EXPR = wasm.exports.valueOf_STRUCT_EXPR();
  valueOfToken.T_MAPI = wasm.exports.valueOf_T_MAPI();
  valueOfToken.T_MAPK = wasm.exports.valueOf_T_MAPK();

  Object.entries(valueOfToken).reduce((accumulator, [k, v]) => {
    accumulator[v] = k;
    return accumulator;
  }, tokenOfValue);
};

export const readOrctoken = (wasm, dataView, offset) => {
  const type = tokenOfValue[wasm.exports.orctokenGetType(offset)];
  // const lexemePtr = wasm.exports.orctokenGetLexeme(offset);
  const value = wasm.exports.orctokenGetValue(offset);
  const fvalue = wasm.exports.orctokenGetFvalue(offset);
  const optype = wasm.exports.orctokenGetOptype(offset);
  const nextPtr = wasm.exports.orctokenGetNext(offset);

  const node = { type, value, fvalue, optype };

  // if (lexemePtr > 0) {
  // }

  if (nextPtr > 0) {
    node.next = readOrctoken(wasm, dataView, nextPtr);
  }

  return node;
};

export const readTree = (wasm, dataView, offset) => {
  const type = tokenOfValue[wasm.exports.treeStructGetType(offset)];
  const valuePtr = wasm.exports.treeStructGetValuePtr(offset);
  const rate = wasm.exports.treeStructGetRate(offset);
  const length_ = wasm.exports.treeStructGetLen(offset);
  const line = wasm.exports.treeStructGetLine(offset);
  const locn = wasm.exports.treeStructGetLocn(offset);
  const leftPtr = wasm.exports.treeStructGetLeftPtr(offset);
  const rightPtr = wasm.exports.treeStructGetRightPtr(offset);
  const nextPtr = wasm.exports.treeStructGetNextPtr(offset);

  const node = { type, rate, len: length_, line, locn };

  if (valuePtr > 0) {
    node.value = readOrctoken(wasm, dataView, valuePtr);
  }

  if (leftPtr > 0) {
    node.left = readTree(wasm, dataView, leftPtr);
  }

  if (rightPtr > 0) {
    node.right = readTree(wasm, dataView, rightPtr);
  }

  if (nextPtr > 0) {
    node.next = readTree(wasm, dataView, nextPtr);
  }

  return node;
};
