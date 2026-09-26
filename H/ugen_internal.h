/** Internal header for UGen API implementation.
 *
 * This file contains the full struct definitions for the opaque types
 * declared in ugen.h.  It should ONLY be included by:
 *   - Engine/ugen.c  (the implementation)
 *   - White-box unit tests that need to inspect struct internals
 *
 * Public consumers of the UGen API should include ugen.h only.
 */

#pragma once

#include "ugen.h"
#include "csoundCore.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * UGEN_VAR is a typed handle to a variable's data slot.
 * Vars returned by csoundUgenGetOutVar/GetInVar point into the
 * UGEN's data block and are owned by the UGEN.
 * Vars created by csoundUgenVarNew are standalone and must be
 * freed by the caller with csoundUgenVarDelete.
 */
struct UGEN_VAR {
  CSOUND* csound;
  MYFLT* data;              /**< Points past CS_VAR_MEM header to value slot */
  UGEN_ARG_TYPE type;       /**< Cached type enum */
  int32_t ksmps;            /**< Needed for audio size calculations */
  bool owned;               /**< true if standalone (caller must free) */
};

/**
 * UGEN represents a single instantiated Csound opcode that can be
 * used outside of the normal Csound instrument compilation pipeline.
 */
struct UGEN {
  CSOUND* csound;
  INSDS* insds;
  OENTRY* oentry;
  void* opcodeMem;
  MYFLT* data;
  CS_VAR_POOL* inPool;
  CS_VAR_POOL* outPool;
  int32_t inCount;          /**< Number of input arguments */
  int32_t outCount;         /**< Number of output arguments */
  UGEN_ARG_TYPE* inTypes;   /**< Array of UGEN_ARG_TYPE for each input arg */
  UGEN_ARG_TYPE* outTypes;  /**< Array of UGEN_ARG_TYPE for each output arg */
  int32_t outDataOffset;    /**< Offset in data block where input args begin (in MYFLTs) */
  UGEN_VAR* outVars;        /**< Array of UGEN_VAR for output args (owned by UGEN) */
  UGEN_VAR* inVars;         /**< Array of UGEN_VAR for input args (owned by UGEN) */
};

/**
 * UGEN_FACTORY creates and manages individual opcode instances.
 * A factory shares a single CSOUND and INSDS context across all
 * UGENs it creates.
 */
struct UGEN_FACTORY {
  CSOUND* csound;
  INSDS* insds;
};

/**
 * UGEN_CONTEXT provides instrument-like context for UGENs
 * (hold/release state, MIDI context, etc.)
 */
struct UGEN_CONTEXT {
  CSOUND* csound;
  INSDS* insds;
};

/**
 * UGEN_GRAPH represents a connected graph of UGENs that can be
 * performed together as a unit.
 */
struct UGEN_GRAPH {
  UGEN** ugens;
  int32_t count;
  int32_t capacity;
  UGEN_FACTORY* factory;
};

#ifdef  __cplusplus
}
#endif
