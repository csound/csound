import { pointerSize, readPointer, sampleArrayType } from "../utils/wasm64.js";
import { freeStringPtr, ptr2string, string2ptr } from "../utils/string-pointers.js";

/*
   Csound UGEN (Unit Generator) module
   API for creating and using Csound Opcodes as individual unit generators.

   All opaque types (UGEN_FACTORY*, UGEN*, UGEN_VAR*, UGEN_GRAPH*, UGEN_CONTEXT*)
   use the engine's native pointer width.
*/

/** @enum {number} */
export const UGEN_ARG_TYPE = {
  I: 0,
  K: 1,
  A: 2,
  S: 3,
  F: 4,
  UNKNOWN: 5,
};

/* ==== Factory API ==== */

/**
 * Creates a UGEN_FACTORY for listing and creating UGENs.
 * @function
 */
export const csoundUgenFactoryNew = (wasm) => (csound) =>
  wasm.exports["csoundUgenFactoryNew"](csound);

/**
 * Deletes a UGEN_FACTORY.
 * @function
 */
export const csoundUgenFactoryDelete = (wasm) => (factory) =>
  wasm.exports["csoundUgenFactoryDelete"](factory);

/* ==== Context API ==== */

/**
 * Creates a new UGEN_CONTEXT for instrument-like state management.
 * @function
 */
export const csoundUgenContextNew = (wasm) => (factory) =>
  wasm.exports["csoundUgenContextNew"](factory);

/**
 * Deletes a UGEN_CONTEXT.
 * @function
 */
export const csoundUgenContextDelete = (wasm) => (context) =>
  wasm.exports["csoundUgenContextDelete"](context);

/**
 * Associates a UGEN with a context.
 * Must be called before csoundUgenInit() if the opcode needs instrument-like state.
 * @function
 */
export const csoundUgenSetContext = (wasm) => (ugen, context) =>
  wasm.exports["csoundUgenSetContext"](ugen, context);

/* ==== UGEN Creation/Destruction ==== */

/**
 * Creates a new UGEN instance from opcode name and type strings.
 * @function
 */
export const csoundUgenNew = (wasm) => (factory, opName, outargTypes, inargTypes) => {
  const opNamePtr = string2ptr(wasm, opName);
  const outPtr = string2ptr(wasm, outargTypes);
  const inPtr = string2ptr(wasm, inargTypes);
  const result = wasm.exports["csoundUgenNew"](factory, opNamePtr, outPtr, inPtr);
  freeStringPtr(wasm, opNamePtr);
  freeStringPtr(wasm, outPtr);
  freeStringPtr(wasm, inPtr);
  return result;
};

/**
 * Deletes a UGEN and frees all associated resources.
 * @function
 */
export const csoundUgenDelete = (wasm) => (ugen) => wasm.exports["csoundUgenDelete"](ugen);

/* ==== UGEN_VAR: Typed Variable Handles ==== */

/**
 * Gets the output variable at the given index.
 * The returned pointer is owned by the UGEN.
 * @function
 */
export const csoundUgenGetOutVar = (wasm) => (ugen, index) =>
  wasm.exports["csoundUgenGetOutVar"](ugen, index);

/**
 * Gets the input variable at the given index.
 * The returned pointer is owned by the UGEN.
 * @function
 */
export const csoundUgenGetInVar = (wasm) => (ugen, index) =>
  wasm.exports["csoundUgenGetInVar"](ugen, index);

/**
 * Connects a UGEN_VAR to a UGEN's input at the given index.
 * This performs zero-copy pointer wiring.
 * @function
 */
export const csoundUgenSetInputVar = (wasm) => (ugen, inIdx, var_) =>
  wasm.exports["csoundUgenSetInputVar"](ugen, inIdx, var_);

/**
 * Creates a standalone UGEN_VAR of the given type.
 * Caller must free with csoundUgenVarDelete().
 * @function
 */
export const csoundUgenVarNew = (wasm) => (factory, type) =>
  wasm.exports["csoundUgenVarNew"](factory, type);

/**
 * Deletes a standalone UGEN_VAR.
 * Do NOT call on vars from csoundUgenGetOutVar/GetInVar.
 * @function
 */
export const csoundUgenVarDelete = (wasm) => (var_) => wasm.exports["csoundUgenVarDelete"](var_);

/* ==== UGEN_VAR: Query ==== */

/**
 * Gets the type of a UGEN_VAR.
 * @function
 */
export const csoundUgenVarGetType = (wasm) => (var_) => wasm.exports["csoundUgenVarGetType"](var_);

/**
 * Gets the size in bytes of a UGEN_VAR's data.
 * @function
 */
export const csoundUgenVarGetSize = (wasm) => (var_) => wasm.exports["csoundUgenVarGetSize"](var_);

/* ==== UGEN_VAR: Numeric Access (i/k) ==== */

/**
 * Sets a scalar (i-rate or k-rate) value on a UGEN_VAR.
 * @function
 */
export const csoundUgenVarSetValue = (wasm) => (var_, value) =>
  wasm.exports["csoundUgenVarSetValue"](var_, value);

/**
 * Gets a scalar (i-rate or k-rate) value from a UGEN_VAR.
 * @function
 */
export const csoundUgenVarGetValue = (wasm) => (var_) =>
  wasm.exports["csoundUgenVarGetValue"](var_);

/* ==== UGEN_VAR: Data Access (generic) ==== */

/**
 * Gets a raw pointer to the var's data buffer.
 * Use with wasm memory to create typed array views.
 * @function
 */
export const csoundUgenVarGetData = (wasm) => (var_) => wasm.exports["csoundUgenVarGetData"](var_);

/**
 * Legacy alias for the raw sample pointer.
 * Use csoundUgenVarGetData and a Float64Array with the browser build.
 * @function
 */
export const csoundUgenVarGetDataAsFloat64Array = (wasm) => (var_) =>
  wasm.exports["csoundUgenVarGetDataAsFloat64Array"](var_);

/**
 * Gets the ksmps (block size) for a UGEN_VAR.
 * For a-rate: number of samples in the audio buffer.
 * For i/k: returns 1.
 * @function
 */
export const csoundUgenVarGetKsmps = (wasm) => (var_) =>
  wasm.exports["csoundUgenVarGetKsmps"](var_);

/* ==== UGEN_VAR: String Access ==== */

/**
 * Sets a string value on a UGEN_VAR of type S.
 * @function
 */
export const csoundUgenVarSetString = (wasm) => (var_, str) => {
  const strPtr = string2ptr(wasm, str);
  const result = wasm.exports["csoundUgenVarSetString"](var_, strPtr);
  freeStringPtr(wasm, strPtr);
  return result;
};

/**
 * Gets the string value from a UGEN_VAR of type S.
 * @function
 */
export const csoundUgenVarGetString = (wasm) => (var_) => {
  const ptr = wasm.exports["csoundUgenVarGetString"](var_);
  return ptr === 0 ? null : ptr2string(wasm, ptr);
};

/* ==== UGEN convenience: scalar access by index ==== */

/**
 * Convenience: set a scalar value on input argument at the given index.
 * @function
 */
export const csoundUgenSetValue = (wasm) => (ugen, index, value) =>
  wasm.exports["csoundUgenSetValue"](ugen, index, value);

/**
 * Convenience: get a scalar value from output argument at the given index.
 * @function
 */
export const csoundUgenGetValue = (wasm) => (ugen, index) =>
  wasm.exports["csoundUgenGetValue"](ugen, index);

/* ==== UGEN convenience: string access by index ==== */

/**
 * Convenience: set a string on input argument at the given index.
 * @function
 */
export const csoundUgenSetString = (wasm) => (ugen, index, str) => {
  const strPtr = string2ptr(wasm, str);
  const result = wasm.exports["csoundUgenSetString"](ugen, index, strPtr);
  freeStringPtr(wasm, strPtr);
  return result;
};

/**
 * Convenience: get a string from output argument at the given index.
 * @function
 */
export const csoundUgenGetString = (wasm) => (ugen, index) => {
  const ptr = wasm.exports["csoundUgenGetString"](ugen, index);
  return ptr === 0 ? null : ptr2string(wasm, ptr);
};

/* ==== Argument Query ==== */

/**
 * Gets the number of input arguments.
 * @function
 */
export const csoundUgenGetInCount = (wasm) => (ugen) => wasm.exports["csoundUgenGetInCount"](ugen);

/**
 * Gets the number of output arguments.
 * @function
 */
export const csoundUgenGetOutCount = (wasm) => (ugen) =>
  wasm.exports["csoundUgenGetOutCount"](ugen);

/**
 * Gets the argument type for input argument at index.
 * @function
 */
export const csoundUgenGetInType = (wasm) => (ugen, index) =>
  wasm.exports["csoundUgenGetInType"](ugen, index);

/**
 * Gets the argument type for output argument at index.
 * @function
 */
export const csoundUgenGetOutType = (wasm) => (ugen, index) =>
  wasm.exports["csoundUgenGetOutType"](ugen, index);

/* ==== Init/Perform ==== */

/**
 * Runs the init-pass for the opcode instance.
 * @function
 */
export const csoundUgenInit = (wasm) => (ugen) => wasm.exports["csoundUgenInit"](ugen);

/**
 * Runs the perf-pass for the opcode instance.
 * @function
 */
export const csoundUgenPerform = (wasm) => (ugen) => wasm.exports["csoundUgenPerform"](ugen);

/* ==== Opcode Listing API ==== */

/**
 * Lists all available opcodes.
 * Returns a JS array of opcode info objects.
 * @function
 */
export const csoundUgenListOpcodes = (wasm) => (factory) => {
  // Allocate space for the list pointer and count
  const width = pointerSize(wasm);
  const listPtrPtr = wasm.exports["allocStringMem"](width);
  const countPtr = wasm.exports["allocStringMem"](4);

  const result = wasm.exports["csoundUgenListOpcodes"](factory, listPtrPtr, countPtr);

  if (result !== 0) {
    wasm.exports["freeStringMem"](listPtrPtr);
    wasm.exports["freeStringMem"](countPtr);
    return [];
  }

  const memory = wasm.exports["memory"];
  const view = new DataView(memory.buffer);
  const listPtr = readPointer(wasm, view, listPtrPtr);
  const count = view.getInt32(countPtr, true);

  // Four pointer-sized fields followed by flags and alignment padding.
  const STRUCT_SIZE = width * 5;

  const opcodes = [];
  for (let i = 0; i < count; i++) {
    const base = listPtr + i * STRUCT_SIZE;
    const opnamePtr = readPointer(wasm, view, base);
    const outypesPtr = readPointer(wasm, view, base + width);
    const intypesPtr = readPointer(wasm, view, base + 2 * width);

    opcodes.push({
      "opname": opnamePtr ? ptr2string(wasm, opnamePtr) : "",
      "outypes": outypesPtr ? ptr2string(wasm, outypesPtr) : "",
      "intypes": intypesPtr ? ptr2string(wasm, intypesPtr) : "",
    });
  }

  // Free the list
  wasm.exports["csoundUgenFreeOpcodeList"](factory, listPtr);
  wasm.exports["freeStringMem"](listPtrPtr);
  wasm.exports["freeStringMem"](countPtr);

  return opcodes;
};

/**
 * Checks whether a specific opcode entry exists.
 * @function
 */
export const csoundUgenFindOpcode = (wasm) => (factory, opname, outargTypes, inargTypes) => {
  const opnamePtr = string2ptr(wasm, opname);
  const outPtr = string2ptr(wasm, outargTypes);
  const inPtr = string2ptr(wasm, inargTypes);
  const result = wasm.exports["csoundUgenFindOpcode"](factory, opnamePtr, outPtr, inPtr);
  freeStringPtr(wasm, opnamePtr);
  freeStringPtr(wasm, outPtr);
  freeStringPtr(wasm, inPtr);
  return result;
};

/* ==== UGen Graph API ==== */

/**
 * Creates a new empty UGen graph.
 * @function
 */
export const csoundUgenGraphNew = (wasm) => (factory) =>
  wasm.exports["csoundUgenGraphNew"](factory);

/**
 * Adds a UGEN to the graph.
 * @function
 */
export const csoundUgenGraphAdd = (wasm) => (graph, ugen) =>
  wasm.exports["csoundUgenGraphAdd"](graph, ugen);

/**
 * Initializes all UGENs in graph order.
 * @function
 */
export const csoundUgenGraphInit = (wasm) => (graph) => wasm.exports["csoundUgenGraphInit"](graph);

/**
 * Performs one ksmps block for all UGENs in graph order.
 * @function
 */
export const csoundUgenGraphPerform = (wasm) => (graph) =>
  wasm.exports["csoundUgenGraphPerform"](graph);

/**
 * Deletes a UGen graph (does NOT delete individual UGENs).
 * @function
 */
export const csoundUgenGraphDelete = (wasm) => (graph) =>
  wasm.exports["csoundUgenGraphDelete"](graph);

/**
 * Deletes a UGen graph AND all UGENs contained in it.
 * @function
 */
export const csoundUgenGraphDeleteAll = (wasm) => (graph) =>
  wasm.exports["csoundUgenGraphDeleteAll"](graph);

/* ==== High-level JS helpers ==== */

/**
 * Returns float samples, converting double engine buffers to a copy.
 * For a-rate vars, returns ksmps samples. For i/k, returns 1 sample.
 * @function
 */
const sampleView = (wasm, variable) => {
  const pointer = wasm.exports["csoundUgenVarGetData"](variable);
  const count = wasm.exports["csoundUgenVarGetKsmps"](variable);
  const Samples = sampleArrayType(wasm);
  return pointer && count
    ? new Samples(wasm.exports["memory"].buffer, pointer, count)
    : new Samples(0);
};

export const csoundUgenVarGetFloat32Array = (wasm) => (variable) => {
  const view = sampleView(wasm, variable);
  return view instanceof Float32Array ? view : Float32Array.from(view);
};

/** Returns a live view for double engines, or a converted copy for float engines. */
export const csoundUgenVarGetFloat64Array = (wasm) => (variable) => {
  const view = sampleView(wasm, variable);
  return view instanceof Float64Array ? view : Float64Array.from(view);
};
