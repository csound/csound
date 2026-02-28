/** API Functions for creating instances of Csound Opcodes as
 * individual unit generators.
 *
 * Based on the design from:
 *   "Extending Aura with Csound Opcodes"
 *   Steven Yi, Victor Lazzarini, Roger Dannenberg, John ffitch
 *   ICMC/SMC 2014
 *
 * Workflow:
 *
 * - User creates a CSOUND instance
 * - User creates a UGEN_FACTORY
 * - User lists available opcodes
 * - User creates UGENs via the factory
 * - User gets UGEN_VAR handles for inputs/outputs
 * - User sets values on vars and/or connects vars between UGENs
 * - User calls csoundUgenInit / csoundUgenPerform (or graph equivalents) to
 *   run the processing
 *
 * All struct types are opaque; internal details are in H/ugen_internal.h
 * (for library implementation and white-box tests only).
 */

#pragma once

#include "csound.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif

/** Maximum number of var-arg slots expanded for a single var-arg parameter */
#define UGEN_MAX_VAR_ARGS 8

/* Opaque types – defined in H/ugen_internal.h */
typedef struct UGEN UGEN;
typedef struct UGEN_FACTORY UGEN_FACTORY;
typedef struct UGEN_CONTEXT UGEN_CONTEXT;
typedef struct UGEN_GRAPH UGEN_GRAPH;
typedef struct UGEN_VAR UGEN_VAR;

/**
 * Argument type enum returned by the query API.
 * Maps to Csound's internal CS_TYPE system without exposing it.
 */
typedef enum {
  UGEN_ARG_TYPE_I = 0,   /**< i-rate (init-time scalar) */
  UGEN_ARG_TYPE_K,       /**< k-rate (control-rate scalar) */
  UGEN_ARG_TYPE_A,       /**< a-rate (audio-rate vector, ksmps samples) */
  UGEN_ARG_TYPE_S,       /**< S (string) */
  UGEN_ARG_TYPE_F,       /**< f (fsig / spectral) */
  UGEN_ARG_TYPE_UNKNOWN  /**< unknown / unsupported */
} UGEN_ARG_TYPE;

/** Opcode info struct returned by the listing API.
 * Contains strings and metadata for one opcode entry. */
typedef struct {
  const char* opname;
  const char* outypes;
  const char* intypes;
  size_t dsblksiz;
  int32_t flags;
} UGEN_OPCODE_INFO;

/* ==== Factory API ==== */

/** Creates a UGEN_FACTORY, used to list available UGENs (Csound Opcodes),
 * as well as create instances of UGENs. User should configure the CSOUND
 * instance for sr and ksmps before creating a factory. */
PUBLIC UGEN_FACTORY* csoundUgenFactoryNew(CSOUND* csound);

/** Delete a UGEN_FACTORY */
PUBLIC bool csoundUgenFactoryDelete(UGEN_FACTORY* factory);

/* ==== Context API ==== */

/** Create a new UGEN_CONTEXT for instrument-like state management */
PUBLIC UGEN_CONTEXT* csoundUgenContextNew(UGEN_FACTORY* factory);

/** Delete a UGEN_CONTEXT */
PUBLIC bool csoundUgenContextDelete(UGEN_CONTEXT* context);

/** Associate a UGEN with a context for hold/release/MIDI support.
 * Must be called before csoundUgenInit() if the opcode needs
 * instrument-like state. */
PUBLIC bool csoundUgenSetContext(UGEN* ugen, UGEN_CONTEXT* context);

/* ==== UGEN Creation/Destruction ==== */

/** Create a new UGEN, using the given UGEN_FACTORY and opcode name/types.
 * The outargTypes and inargTypes must match an OENTRY exactly. */
PUBLIC UGEN* csoundUgenNew(UGEN_FACTORY* factory, char* opName,
                      char* outargTypes, char* inargTypes);

/** Delete a UGEN and free all associated resources */
PUBLIC bool csoundUgenDelete(UGEN* ugen);

/* ==== UGEN_VAR: Typed Variable Handles ==== */

/** Get a handle to the output variable at the given index.
 * The returned UGEN_VAR is owned by the UGEN and must NOT be freed
 * by the caller. It remains valid until the UGEN is deleted. */
PUBLIC UGEN_VAR* csoundUgenGetOutVar(UGEN* ugen, int32_t index);

/** Get a handle to the input variable at the given index.
 * The returned UGEN_VAR is owned by the UGEN and must NOT be freed
 * by the caller. It remains valid until the UGEN is deleted. */
PUBLIC UGEN_VAR* csoundUgenGetInVar(UGEN* ugen, int32_t index);

/** Connect a UGEN_VAR to a UGEN's input at the given index.
 * This performs zero-copy pointer wiring: the destination opcode's
 * input will read directly from the var's data.
 * Typically used to wire one UGEN's output to another's input:
 *   csoundUgenSetInputVar(dest, 0, csoundUgenGetOutVar(source, 0));
 */
PUBLIC bool csoundUgenSetInputVar(UGEN* ugen, int32_t inIdx, UGEN_VAR* var);

/** Create a standalone UGEN_VAR of the given type.
 * Useful for user-controlled parameters (e.g. a k-rate frequency value).
 * The caller must free it with csoundUgenVarDelete(). */
PUBLIC UGEN_VAR* csoundUgenVarNew(UGEN_FACTORY* factory, UGEN_ARG_TYPE type);

/** Delete a standalone UGEN_VAR created with csoundUgenVarNew().
 * Do NOT call this on vars returned by csoundUgenGetOutVar/GetInVar. */
PUBLIC void csoundUgenVarDelete(UGEN_VAR* var);

/* ==== UGEN_VAR: Query ==== */

/** Get the type of a UGEN_VAR. */
PUBLIC UGEN_ARG_TYPE csoundUgenVarGetType(UGEN_VAR* var);

/** Get the size in bytes of a UGEN_VAR's data. */
PUBLIC size_t csoundUgenVarGetSize(UGEN_VAR* var);

/* ==== UGEN_VAR: Numeric Access (i/k) ==== */

/** Set a scalar (i-rate or k-rate) value on a UGEN_VAR. */
PUBLIC void csoundUgenVarSetValue(UGEN_VAR* var, MYFLT value);

/** Get a scalar (i-rate or k-rate) value from a UGEN_VAR. */
PUBLIC MYFLT csoundUgenVarGetValue(UGEN_VAR* var);

/* ==== UGEN_VAR: Data Access (generic) ==== */

/** Get a raw pointer to the var's data.
 * For i/k types: points to a single MYFLT.
 * For a-rate: points to MYFLT[ksmps].
 * For S type: points to the internal STRINGDAT struct.
 * For f type: points to the internal PVSDAT struct.
 * The pointer is valid until the owning UGEN or standalone var is deleted. */
PUBLIC void* csoundUgenVarGetData(UGEN_VAR* var);

/* ==== UGEN_VAR: String Access ==== */

/** Set a string value on a UGEN_VAR of type S.
 * Returns false if var is NULL or not of type S. */
PUBLIC bool csoundUgenVarSetString(UGEN_VAR* var, const char* str);

/** Get the string value from a UGEN_VAR of type S.
 * Returns NULL if var is NULL or not of type S. */
PUBLIC const char* csoundUgenVarGetString(UGEN_VAR* var);

/* ==== UGEN convenience: scalar access by index ==== */

/** Convenience: set a scalar value on input argument at the given index.
 * Equivalent to csoundUgenVarSetValue(csoundUgenGetInVar(ugen, index), value)
 * but avoids the UGEN_VAR lookup.  Best for one-off init-time setup.
 * For per-k-cycle updates in a tight loop, prefer caching the UGEN_VAR
 * handle from csoundUgenGetInVar() and calling csoundUgenVarSetValue(). */
PUBLIC void csoundUgenSetValue(UGEN* ugen, int32_t index, MYFLT value);

/** Convenience: get a scalar value from output argument at the given index.
 * Equivalent to csoundUgenVarGetValue(csoundUgenGetOutVar(ugen, index)).
 * For repeated reads in a tight loop, prefer caching the UGEN_VAR handle. */
PUBLIC MYFLT csoundUgenGetValue(UGEN* ugen, int32_t index);

/* ==== UGEN convenience: string access by index ==== */

/** Convenience: set a string on input argument at the given index.
 * Equivalent to csoundUgenVarSetString(csoundUgenGetInVar(ugen, index), str).
 * Returns false if the argument is not an S type or index is out of range. */
PUBLIC bool csoundUgenSetString(UGEN* ugen, int32_t index, const char* str);

/** Convenience: get a string from output argument at the given index.
 * Equivalent to csoundUgenVarGetString(csoundUgenGetOutVar(ugen, index)).
 * Returns NULL if the argument is not an S type or index is out of range. */
PUBLIC const char* csoundUgenGetString(UGEN* ugen, int32_t index);

/* ==== Argument Query (convenience) ==== */

/** Get number of input arguments */
PUBLIC int32_t csoundUgenGetInCount(UGEN* ugen);

/** Get number of output arguments */
PUBLIC int32_t csoundUgenGetOutCount(UGEN* ugen);

/** Get the argument type for input argument at index */
PUBLIC UGEN_ARG_TYPE csoundUgenGetInType(UGEN* ugen, int32_t index);

/** Get the argument type for output argument at index */
PUBLIC UGEN_ARG_TYPE csoundUgenGetOutType(UGEN* ugen, int32_t index);

/* ==== Init/Perform ==== */

/** Run the init-pass for the opcode instance held in UGEN. */
PUBLIC int32_t csoundUgenInit(UGEN* ugen);

/** Run the perf-pass for the opcode instance held in UGEN. */
PUBLIC int32_t csoundUgenPerform(UGEN* ugen);

/* ==== Opcode Listing API ==== */

/** Get a list of all available opcodes.
 * Sets *list to a newly allocated array and *count to the number of entries.
 * Caller must free with csoundUgenFreeOpcodeList(). */
PUBLIC int32_t csoundUgenListOpcodes(UGEN_FACTORY* factory,
                                 UGEN_OPCODE_INFO** list, int32_t* count);

/** Free opcode list returned by csoundUgenListOpcodes(). */
PUBLIC void csoundUgenFreeOpcodeList(UGEN_FACTORY* factory,
                                  UGEN_OPCODE_INFO* list);

/** Check whether a specific opcode entry exists by name and types.
 * Returns true if found, false otherwise. */
PUBLIC bool csoundUgenFindOpcode(UGEN_FACTORY* factory, const char* opname,
                             const char* outargTypes, const char* inargTypes);

/* ==== UGen Graph API ==== */

/** Create a new empty UGen graph */
PUBLIC UGEN_GRAPH* csoundUgenGraphNew(UGEN_FACTORY* factory);

/** Add a UGEN to the graph. Returns the index of the UGEN in the graph,
 * or -1 on error. */
PUBLIC int32_t csoundUgenGraphAdd(UGEN_GRAPH* graph, UGEN* ugen);

/** Initialize all UGENs in graph order */
PUBLIC int32_t csoundUgenGraphInit(UGEN_GRAPH* graph);

/** Perform one ksmps block for all UGENs in graph order */
PUBLIC int32_t csoundUgenGraphPerform(UGEN_GRAPH* graph);

/** Delete a UGen graph (does NOT delete the individual UGENs) */
PUBLIC bool csoundUgenGraphDelete(UGEN_GRAPH* graph);

/** Delete a UGen graph AND all UGENs contained in it */
PUBLIC bool csoundUgenGraphDeleteAll(UGEN_GRAPH* graph);

#ifdef  __cplusplus
}
#endif

