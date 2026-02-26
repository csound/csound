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
 * - User connects arguments using csoundUgenSetInput / csoundUgenSetOutput
 *   or csoundUgenGraphConnect to build a signal graph
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

/* ==== Argument Handling: By Pointer (zero-copy) ==== */

/** Set output argument pointer for opcode's data struct by index.
 * The pointer must point to memory of the correct size for the arg type
 * (MYFLT for k/i, MYFLT[ksmps] for a-rate). */
PUBLIC bool csoundUgenSetOutput(UGEN* ugen, int32_t index, void* arg);

/** Set input argument pointer for opcode's data struct by index.
 * The pointer must point to memory of the correct size for the arg type. */
PUBLIC bool csoundUgenSetInput(UGEN* ugen, int32_t index, void* arg);

/* ==== Argument Handling: By Value (copy) ==== */

/** Copy a value into the output argument data for index.
 * Copies csoundUgenGetOutArgSize() bytes from arg into the internal buffer.
 * For scalar types (i/k) this is sizeof(MYFLT); for a-rate it is
 * ksmps * sizeof(MYFLT), so arg must point to a buffer of that size. */
PUBLIC bool csoundUgenSetOutputValue(UGEN* ugen, int32_t index, void* arg);

/** Copy a value into the input argument data for index.
 * Copies csoundUgenGetInArgSize() bytes from arg into the internal buffer.
 * For scalar types (i/k) this is sizeof(MYFLT); for a-rate it is
 * ksmps * sizeof(MYFLT), so arg must point to a buffer of that size. */
PUBLIC bool csoundUgenSetInputValue(UGEN* ugen, int32_t index, void* arg);

/** Read output argument value for index into dest buffer.
 * Copies csoundUgenGetOutArgSize() bytes. For a-rate arguments,
 * dest must be a buffer of at least ksmps * sizeof(MYFLT).
 * Returns the number of bytes copied, or 0 on error. */
PUBLIC size_t csoundUgenGetOutputValue(UGEN* ugen, int32_t index, void* dest);

/** Read input argument value for index into dest buffer.
 * Copies csoundUgenGetInArgSize() bytes. For a-rate arguments,
 * dest must be a buffer of at least ksmps * sizeof(MYFLT).
 * Returns the number of bytes copied, or 0 on error. */
PUBLIC size_t csoundUgenGetInputValue(UGEN* ugen, int32_t index, void* dest);

/* ==== Argument Query ==== */

/** Get number of input arguments */
PUBLIC int32_t csoundUgenGetInCount(UGEN* ugen);

/** Get number of output arguments */
PUBLIC int32_t csoundUgenGetOutCount(UGEN* ugen);

/** Get the argument type for input argument at index */
PUBLIC UGEN_ARG_TYPE csoundUgenGetInType(UGEN* ugen, int32_t index);

/** Get the argument type for output argument at index */
PUBLIC UGEN_ARG_TYPE csoundUgenGetOutType(UGEN* ugen, int32_t index);

/** Get the size in bytes of the argument at the given index for input args */
PUBLIC size_t csoundUgenGetInArgSize(UGEN* ugen, int32_t index);

/** Get the size in bytes of the argument at the given index for output args */
PUBLIC size_t csoundUgenGetOutArgSize(UGEN* ugen, int32_t index);

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

/** Connect output of source UGEN to input of dest UGEN by pointer.
 * This wires source's output[outIdx] memory to dest's input[inIdx]. */
PUBLIC bool csoundUgenGraphConnect(UGEN* source, int32_t outIdx,
                               UGEN* dest, int32_t inIdx);

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

