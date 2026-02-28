 /*
    ugen.c:

    Copyright (C) 2021, 2026
    Steven Yi

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/** API Functions for creating instances of Csound Opcodes as
 * individual unit generators. Based on the design from:
 *
 * "Extending Aura with Csound Opcodes"
 * Steven Yi, Victor Lazzarini, Roger Dannenberg, John ffitch
 * ICMC/SMC 2014
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
 * - context: required for things like hold, releasing, etc.
 * */

#include "ugen_internal.h"
#include "csound_standard_types.h"
#include "csound_orc.h"
#include "csound_orc_semantics.h"
#include "pstream.h"
#include <string.h>

/* ============================================================
 *  Internal helpers
 * ============================================================ */

/**
 * Convert a single OENTRY type character to a CS_TYPE pointer.
 */
static const CS_TYPE* ugen_char_to_cs_type(char c) {
    switch (c) {
        case 'i': return &CS_VAR_TYPE_I;
        case 'k': return &CS_VAR_TYPE_K;
        case 'a': return &CS_VAR_TYPE_A;
        case 'S': return &CS_VAR_TYPE_S;
        case 'f': return &CS_VAR_TYPE_F;
        default:  return &CS_VAR_TYPE_K;
    }
}

/**
 * Convert a CS_TYPE pointer to a UGEN_ARG_TYPE enum value.
 */
static UGEN_ARG_TYPE ugen_cs_type_to_arg_type(const CS_TYPE* type) {
    if (type == &CS_VAR_TYPE_I) return UGEN_ARG_TYPE_I;
    if (type == &CS_VAR_TYPE_K) return UGEN_ARG_TYPE_K;
    if (type == &CS_VAR_TYPE_A) return UGEN_ARG_TYPE_A;
    if (type == &CS_VAR_TYPE_S) return UGEN_ARG_TYPE_S;
    if (type == &CS_VAR_TYPE_F) return UGEN_ARG_TYPE_F;
    return UGEN_ARG_TYPE_UNKNOWN;
}

/**
 * Convert a UGEN_ARG_TYPE enum value to a CS_TYPE pointer.
 */
static const CS_TYPE* ugen_arg_type_to_cs_type(UGEN_ARG_TYPE type) {
    switch (type) {
        case UGEN_ARG_TYPE_I: return &CS_VAR_TYPE_I;
        case UGEN_ARG_TYPE_K: return &CS_VAR_TYPE_K;
        case UGEN_ARG_TYPE_A: return &CS_VAR_TYPE_A;
        case UGEN_ARG_TYPE_S: return &CS_VAR_TYPE_S;
        case UGEN_ARG_TYPE_F: return &CS_VAR_TYPE_F;
        default: return &CS_VAR_TYPE_K;
    }
}

/**
 * Returns the size in bytes of an argument given its UGEN_ARG_TYPE and the
 * current ksmps value.
 */
static size_t ugen_arg_type_size(UGEN_ARG_TYPE type, int32_t ksmps) {
    switch (type) {
        case UGEN_ARG_TYPE_A:
            return (size_t)ksmps * sizeof(MYFLT);
        case UGEN_ARG_TYPE_S:
            return CS_FLOAT_ALIGN(sizeof(STRINGDAT));
        case UGEN_ARG_TYPE_F:
            return CS_FLOAT_ALIGN(sizeof(PVSDAT));
        default:
            /* k, i, and other scalar types */
            return sizeof(MYFLT);
    }
}

/**
 * Initialize an INSDS from a CSOUND instance's current settings.
 */
static void insds_init_from_csound(INSDS* insds, CSOUND* csound) {
    insds->esr = csound->esr;
    insds->pidsr = csound->pidsr;
    insds->sicvt = csound->sicvt;
    insds->onedsr = csound->onedsr;
    insds->ksmps = csound->ksmps;
    insds->ekr = csound->ekr;
    insds->kcounter = csound->kcounter;
    insds->onedksmps = csound->onedksmps;
    insds->onedkr = csound->onedkr;
    insds->kicvt = csound->kicvt;
}

/**
 * Copy INSDS fields from a source INSDS.
 */
static void insds_init_from_insds(INSDS* dest, const INSDS* src) {
    dest->esr = src->esr;
    dest->pidsr = src->pidsr;
    dest->sicvt = src->sicvt;
    dest->onedsr = src->onedsr;
    dest->ksmps = src->ksmps;
    dest->ekr = src->ekr;
    dest->kcounter = src->kcounter;
    dest->onedksmps = src->onedksmps;
    dest->onedkr = src->onedkr;
    dest->kicvt = src->kicvt;
}

/**
 * Parse an OENTRY type string for input arguments, expanding
 * polymorphic/optional specifiers to concrete types.
 * Returns arrays of CS_TYPE* and the total count including var-arg expansion.
 */
static int32_t parse_in_types(const char* intypes, const CS_TYPE** outArray,
                              int32_t maxSlots) {
    int32_t count = 0;
    const char* p = intypes;

    while (*p != 0 && count < maxSlots) {
        char c = *p;

        /* var-arg types: expand to UGEN_MAX_VAR_ARGS slots */
        if (strchr("My", c)) {
            for (int32_t j = 0; j < UGEN_MAX_VAR_ARGS && count < maxSlots; j++) {
                outArray[count++] = &CS_VAR_TYPE_A;
            }
            break;  /* var-arg is always last */
        } else if (strchr("mnzZN", c)) {
            for (int32_t j = 0; j < UGEN_MAX_VAR_ARGS && count < maxSlots; j++) {
                outArray[count++] = &CS_VAR_TYPE_K;
            }
            break;
        }

        /* Map optional/polymorphic specifiers to concrete types */
        if (strchr("opqvjh", c) != NULL) {
            c = 'i';
        } else if (strchr("OJVP", c) != NULL) {
            c = 'k';
        }

        outArray[count++] = ugen_char_to_cs_type(c);
        p++;
    }
    return count;
}

/**
 * Parse an OENTRY type string for output arguments.
 */
static int32_t parse_out_types(const char* outypes, const CS_TYPE** outArray,
                               int32_t maxSlots) {
    int32_t count = 0;
    const char* p = outypes;

    while (*p != 0 && count < maxSlots) {
        char c = *p;

        /* Map signal-type specifiers */
        if (strchr("s", c) != NULL) {
            c = 'a';
        }

        outArray[count++] = ugen_char_to_cs_type(c);
        p++;
    }
    return count;
}

/**
 * Get the MYFLT** pointer array inside the opcode memory block.
 * After OPDS, the opcode struct contains MYFLT* pointers for
 * outputs first, then inputs.
 */
static MYFLT** get_arg_pointers(void* opcodeMem) {
    return (MYFLT**)((char*)opcodeMem + sizeof(OPDS));
}

/* ============================================================
 *  Factory API
 * ============================================================ */

UGEN_FACTORY* csoundUgenFactoryNew(CSOUND* csound) {
    UGEN_FACTORY* factory = csound->Calloc(csound, sizeof(UGEN_FACTORY));
    INSDS* insds = csound->Calloc(csound, sizeof(INSDS));

    factory->csound = csound;
    factory->insds = insds;

    /* Inherit values from CSOUND */
    insds_init_from_csound(insds, csound);

    return factory;
}

bool csoundUgenFactoryDelete(UGEN_FACTORY* factory) {
    if (factory == NULL) return false;
    CSOUND* csound = factory->csound;
    csound->Free(csound, factory->insds);
    csound->Free(csound, factory);
    return true;
}

/* ============================================================
 *  Context API
 * ============================================================ */

UGEN_CONTEXT* csoundUgenContextNew(UGEN_FACTORY* factory) {
    CSOUND* csound = factory->csound;
    UGEN_CONTEXT* ctx = csound->Calloc(csound, sizeof(UGEN_CONTEXT));
    ctx->csound = csound;

    /* Create a dedicated INSDS for this context so that UGENs
       using it have their own hold/release state */
    INSDS* insds = csound->Calloc(csound, sizeof(INSDS));
    insds_init_from_insds(insds, factory->insds);
    ctx->insds = insds;

    return ctx;
}

bool csoundUgenContextDelete(UGEN_CONTEXT* context) {
    if (context == NULL) return false;
    CSOUND* csound = context->csound;
    csound->Free(csound, context->insds);
    csound->Free(csound, context);
    return true;
}

bool csoundUgenSetContext(UGEN* ugen, UGEN_CONTEXT* context) {
    if (ugen == NULL || context == NULL) return false;
    OPDS* opds = (OPDS*)ugen->opcodeMem;
    ugen->insds = context->insds;
    opds->insdshead = context->insds;
    return true;
}

/* ============================================================
 *  UGEN Creation / Destruction
 * ============================================================ */

static OENTRY* ugen_resolve_opcode(OENTRIES* entries,
                                   char* outargTypes, char* inargTypes) {
    int32_t i;
    for (i = 0; i < entries->count; i++) {
        OENTRY* temp = entries->entries[i];
        if (strcmp(outargTypes, temp->outypes) == 0 &&
            strcmp(inargTypes, temp->intypes) == 0) {
            return temp;
        }
    }
    return NULL;
}

UGEN* csoundUgenNew(UGEN_FACTORY* factory, char* opName,
               char* outargTypes, char* inargTypes) {
    UGEN* ugen;
    OPDS* opds;
    OPTXT* optxt;
    CSOUND* csound = factory->csound;
    INSDS* insds = factory->insds;
    int32_t ksmps = insds->ksmps;
    int32_t i;
    int32_t maxArgs = 64; /* max total arg slots */

    OENTRIES* entries = find_opcode2(csound, opName);
    if (entries == NULL) {
        return NULL;
    }

    OENTRY* oentry = ugen_resolve_opcode(entries, outargTypes, inargTypes);
    if (oentry == NULL) {
        return NULL;
    }

    /* Allocate the UGEN, OPTXT, and opcode memory */
    ugen = csound->Calloc(csound, sizeof(UGEN));
    optxt = (OPTXT*)csound->Calloc(csound, sizeof(OPTXT));

    ugen->csound = csound;
    ugen->insds = insds;
    ugen->oentry = oentry;
    ugen->opcodeMem = csound->Calloc(csound, oentry->dsblksiz);

    /* Wire up OPDS header */
    opds = ugen->opcodeMem;
    opds->insdshead = insds;
    opds->init = oentry->init;
    opds->perf = oentry->perf;
    opds->deinit = oentry->deinit;
    opds->optext = optxt;

    /* Parse output and input type strings */
    const CS_TYPE** parsedOutTypes = csound->Calloc(csound, maxArgs * sizeof(CS_TYPE*));
    const CS_TYPE** parsedInTypes = csound->Calloc(csound, maxArgs * sizeof(CS_TYPE*));

    int32_t outCount = parse_out_types(oentry->outypes, parsedOutTypes, maxArgs);
    int32_t inCount = parse_in_types(oentry->intypes, parsedInTypes, maxArgs);

    ugen->outCount = outCount;
    ugen->inCount = inCount;

    /* Store type arrays as UGEN_ARG_TYPE for the public query API */
    ugen->outTypes = csound->Calloc(csound, outCount * sizeof(UGEN_ARG_TYPE));
    ugen->inTypes = csound->Calloc(csound, inCount * sizeof(UGEN_ARG_TYPE));
    for (i = 0; i < outCount; i++) {
        ugen->outTypes[i] = ugen_cs_type_to_arg_type(parsedOutTypes[i]);
    }
    for (i = 0; i < inCount; i++) {
        ugen->inTypes[i] = ugen_cs_type_to_arg_type(parsedInTypes[i]);
    }

    /* Set TEXT metadata */
    optxt->t.outArgCount = outCount;
    optxt->t.inArgCount = inCount;
    optxt->t.oentry = oentry;

    /* Create variable pools using proper csoundCreateVarPool */
    ugen->outPool = csoundCreateVarPool(csound);
    ugen->inPool = csoundCreateVarPool(csound);

    /* Create CS_VARIABLEs and add to pools */
    char name[32];
    for (i = 0; i < outCount; i++) {
        snprintf(name, sizeof(name), "out%d", i);
        CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool,
                                                parsedOutTypes[i], name, NULL);
        if (var != NULL) {
            csoundAddVariable(csound, ugen->outPool, var);
        }
    }
    for (i = 0; i < inCount; i++) {
        snprintf(name, sizeof(name), "in%d", i);
        CS_VARIABLE* var = csoundCreateVariable(csound, csound->typePool,
                                                parsedInTypes[i], name, NULL);
        if (var != NULL) {
            csoundAddVariable(csound, ugen->inPool, var);
        }
    }

    /* Recalculate pool memory sizes */
    csoundRecalculateVarPoolMemory(csound, ugen->outPool);
    csoundRecalculateVarPoolMemory(csound, ugen->inPool);

    /* Allocate the data block for all arguments.
     * Layout: [output arg data | input arg data]
     * Each variable's data is placed at its memBlockIndex in the data block.
     * CS_VAR_TYPE_OFFSET is added per variable for the type header. */
    size_t totalDataSize = (size_t)ugen->outPool->poolSize +
                           (size_t)ugen->inPool->poolSize +
                           (size_t)(outCount + inCount) *
                                CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET);

    ugen->data = (MYFLT*)csound->Calloc(csound, totalDataSize);
    ugen->outDataOffset = (ugen->outPool->poolSize +
                           outCount * CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET))
                          / sizeof(MYFLT);

    /* Wire argument pointers in the opcode memory.
     * After OPDS, the opcode struct has MYFLT* pointers:
     *   p[0..outCount-1]  → output arg addresses
     *   p[outCount..outCount+inCount-1] → input arg addresses
     *
     * Each points into the data block at the correct offset,
     * skipping CS_VAR_TYPE_OFFSET for the type header. */
    MYFLT** p = get_arg_pointers(ugen->opcodeMem);

    int32_t pIdx = 0;
    CS_VARIABLE* var = ugen->outPool->head;
    while (var != NULL && pIdx < outCount) {
        /* memBlockIndex already includes per-variable header offsets
         * (set by csoundRecalculateVarPoolMemory), so it points to the
         * value slot.  The CS_VAR_MEM header lives immediately before. */
        MYFLT* base = ugen->data + var->memBlockIndex;
        p[pIdx] = base;

        CS_VAR_MEM* varmem = (CS_VAR_MEM*)((char*)base - CS_VAR_TYPE_OFFSET);
        varmem->varType = var->varType;

        pIdx++;
        var = var->next;
    }

    var = ugen->inPool->head;
    while (var != NULL && (pIdx - outCount) < inCount) {
        MYFLT* base = ugen->data + ugen->outDataOffset + var->memBlockIndex;
        p[pIdx] = base;

        CS_VAR_MEM* varmem = (CS_VAR_MEM*)((char*)base - CS_VAR_TYPE_OFFSET);
        varmem->varType = var->varType;

        pIdx++;
        var = var->next;
    }

    /* Initialize variable memory (allocates STRINGDAT buffers, etc.) */
    csoundInitializeVarPool(csound, ugen->data, ugen->outPool);
    csoundInitializeVarPool(csound,
                            ugen->data + ugen->outDataOffset,
                            ugen->inPool);

    /* Create UGEN_VAR arrays for output and input vars */
    ugen->outVars = csound->Calloc(csound, outCount * sizeof(UGEN_VAR));
    for (i = 0; i < outCount; i++) {
        ugen->outVars[i].csound = csound;
        ugen->outVars[i].data = p[i];
        ugen->outVars[i].type = ugen->outTypes[i];
        ugen->outVars[i].ksmps = ksmps;
        ugen->outVars[i].owned = false;
    }

    ugen->inVars = csound->Calloc(csound, inCount * sizeof(UGEN_VAR));
    for (i = 0; i < inCount; i++) {
        ugen->inVars[i].csound = csound;
        ugen->inVars[i].data = p[outCount + i];
        ugen->inVars[i].type = ugen->inTypes[i];
        ugen->inVars[i].ksmps = ksmps;
        ugen->inVars[i].owned = false;
    }

    /* Clean up temporary arrays */
    csound->Free(csound, parsedOutTypes);
    csound->Free(csound, parsedInTypes);

    return ugen;
}

bool csoundUgenDelete(UGEN* ugen) {
    if (ugen == NULL) return false;
    CSOUND* csound = ugen->csound;
    OPDS* opds = (OPDS*)ugen->opcodeMem;

    /* Call deinit if available */
    if (opds != NULL && ugen->oentry != NULL && ugen->oentry->deinit != NULL) {
        (*ugen->oentry->deinit)(csound, ugen->opcodeMem);
    }

    /* Free variable memory (e.g. STRINGDAT.data buffers) before
     * freeing the data block they reside in. Walk each pool and call
     * the type's freeVariableMemory hook when present. */
    if (ugen->outPool != NULL) {
        CS_VARIABLE* var = ugen->outPool->head;
        while (var != NULL) {
            if (var->varType != NULL && var->varType->freeVariableMemory != NULL) {
                MYFLT* base = ugen->data + var->memBlockIndex;
                var->varType->freeVariableMemory(csound, base);
            }
            var = var->next;
        }
    }
    if (ugen->inPool != NULL) {
        CS_VARIABLE* var = ugen->inPool->head;
        while (var != NULL) {
            if (var->varType != NULL && var->varType->freeVariableMemory != NULL) {
                MYFLT* base = ugen->data + ugen->outDataOffset + var->memBlockIndex;
                var->varType->freeVariableMemory(csound, base);
            }
            var = var->next;
        }
    }

    /* Free OPTXT */
    if (opds != NULL && opds->optext != NULL) {
        csound->Free(csound, opds->optext);
    }

    csound->Free(csound, ugen->opcodeMem);

    if (ugen->outPool != NULL) csoundFreeVarPool(csound, ugen->outPool);
    if (ugen->inPool != NULL) csoundFreeVarPool(csound, ugen->inPool);

    csound->Free(csound, ugen->data);
    csound->Free(csound, ugen->outTypes);
    csound->Free(csound, ugen->inTypes);
    csound->Free(csound, ugen->outVars);
    csound->Free(csound, ugen->inVars);
    csound->Free(csound, ugen);
    return true;
}

/* ============================================================
 *  UGEN_VAR: Typed Variable Handles
 * ============================================================ */

UGEN_VAR* csoundUgenGetOutVar(UGEN* ugen, int32_t index) {
    if (ugen == NULL || index < 0 || index >= ugen->outCount) return NULL;
    return &ugen->outVars[index];
}

UGEN_VAR* csoundUgenGetInVar(UGEN* ugen, int32_t index) {
    if (ugen == NULL || index < 0 || index >= ugen->inCount) return NULL;
    return &ugen->inVars[index];
}

bool csoundUgenSetInputVar(UGEN* ugen, int32_t inIdx, UGEN_VAR* var) {
    if (ugen == NULL || var == NULL) return false;
    if (inIdx < 0 || inIdx >= ugen->inCount) return false;

    /* Wire the opcode's input pointer to the var's data */
    MYFLT** p = get_arg_pointers(ugen->opcodeMem);
    p[ugen->outCount + inIdx] = var->data;

    /* Update the UGEN_VAR handle to point at the same data */
    ugen->inVars[inIdx].data = var->data;
    return true;
}

/* ============================================================
 *  UGEN_VAR: Standalone Creation/Destruction
 * ============================================================ */

UGEN_VAR* csoundUgenVarNew(UGEN_FACTORY* factory, UGEN_ARG_TYPE type) {
    if (factory == NULL) return NULL;
    CSOUND* csound = factory->csound;
    int32_t ksmps = factory->insds->ksmps;

    size_t dataSize = CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET) +
                      ugen_arg_type_size(type, ksmps);

    /* Allocate the var struct and data block together */
    UGEN_VAR* var = csound->Calloc(csound, sizeof(UGEN_VAR));
    char* block = csound->Calloc(csound, dataSize);

    /* Set up the CS_VAR_MEM header */
    CS_VAR_MEM* varmem = (CS_VAR_MEM*)block;
    varmem->varType = ugen_arg_type_to_cs_type(type);

    var->csound = csound;
    var->data = (MYFLT*)(block + CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET));
    var->type = type;
    var->ksmps = ksmps;
    var->owned = true;

    /* Call initializeVariableMemory hook (e.g. allocate STRINGDAT buffer) */
    if (type == UGEN_ARG_TYPE_S || type == UGEN_ARG_TYPE_F) {
        const CS_TYPE* csType = ugen_arg_type_to_cs_type(type);
        /* Create a temporary variable to call the init hook */
        CS_VARIABLE* tempVar = csoundCreateVariable(csound, csound->typePool,
                                                     csType, "_tmp", NULL);
        if (tempVar != NULL && tempVar->initializeVariableMemory != NULL) {
            tempVar->initializeVariableMemory(csound, tempVar, var->data);
        }
        /* We don't add the temp variable to any pool; just free it */
        if (tempVar != NULL) csound->Free(csound, tempVar);
    }

    return var;
}

void csoundUgenVarDelete(UGEN_VAR* var) {
    if (var == NULL || !var->owned) return;
    CSOUND* csound = var->csound;

    /* Call freeVariableMemory hook for S types (frees STRINGDAT.data) */
    if (var->type == UGEN_ARG_TYPE_S || var->type == UGEN_ARG_TYPE_F) {
        const CS_TYPE* csType = ugen_arg_type_to_cs_type(var->type);
        if (csType->freeVariableMemory != NULL) {
            csType->freeVariableMemory(csound, var->data);
        }
    }

    /* Free the data block (CS_VAR_MEM header + value data) */
    char* block = (char*)var->data - CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET);
    csound->Free(csound, block);
    csound->Free(csound, var);
}

/* ============================================================
 *  UGEN_VAR: Query
 * ============================================================ */

UGEN_ARG_TYPE csoundUgenVarGetType(UGEN_VAR* var) {
    if (var == NULL) return UGEN_ARG_TYPE_UNKNOWN;
    return var->type;
}

size_t csoundUgenVarGetSize(UGEN_VAR* var) {
    if (var == NULL) return 0;
    return ugen_arg_type_size(var->type, var->ksmps);
}

/* ============================================================
 *  UGEN_VAR: Numeric Access (i/k scalars)
 * ============================================================ */

void csoundUgenVarSetValue(UGEN_VAR* var, MYFLT value) {
    if (var == NULL) return;
    *(var->data) = value;
}

MYFLT csoundUgenVarGetValue(UGEN_VAR* var) {
    if (var == NULL) return FL(0.0);
    return *(var->data);
}

/* ============================================================
 *  UGEN_VAR: Data Access (generic)
 * ============================================================ */

void* csoundUgenVarGetData(UGEN_VAR* var) {
    if (var == NULL) return NULL;
    return (void*)var->data;
}

/* ============================================================
 *  UGEN_VAR: String Access
 * ============================================================ */

bool csoundUgenVarSetString(UGEN_VAR* var, const char* str) {
    if (var == NULL || var->type != UGEN_ARG_TYPE_S) return false;
    STRINGDAT* sd = (STRINGDAT*)var->data;
    if (str == NULL) {
        if (sd->data != NULL) sd->data[0] = '\0';
        sd->size = 0;
        return true;
    }
    size_t len = strlen(str);
    if (sd->data == NULL || sd->size < (int64_t)(len + 1)) {
        CSOUND* csound = var->csound;
        if (sd->data != NULL) csound->Free(csound, sd->data);
        sd->data = csound->Calloc(csound, len + 1);
        sd->size = (int64_t)(len + 1);
    }
    memcpy(sd->data, str, len + 1);
    return true;
}

const char* csoundUgenVarGetString(UGEN_VAR* var) {
    if (var == NULL || var->type != UGEN_ARG_TYPE_S) return NULL;
    STRINGDAT* sd = (STRINGDAT*)var->data;
    return sd->data;
}

/* ============================================================
 *  UGEN convenience: scalar and string access by index
 * ============================================================ */

void csoundUgenSetValue(UGEN* ugen, int32_t index, MYFLT value) {
    UGEN_VAR* var = csoundUgenGetInVar(ugen, index);
    if (var != NULL) csoundUgenVarSetValue(var, value);
}

MYFLT csoundUgenGetValue(UGEN* ugen, int32_t index) {
    UGEN_VAR* var = csoundUgenGetOutVar(ugen, index);
    if (var != NULL) return csoundUgenVarGetValue(var);
    return FL(0.0);
}

bool csoundUgenSetString(UGEN* ugen, int32_t index, const char* str) {
    UGEN_VAR* var = csoundUgenGetInVar(ugen, index);
    if (var != NULL) return csoundUgenVarSetString(var, str);
    return false;
}

const char* csoundUgenGetString(UGEN* ugen, int32_t index) {
    UGEN_VAR* var = csoundUgenGetOutVar(ugen, index);
    if (var != NULL) return csoundUgenVarGetString(var);
    return NULL;
}

/* ============================================================
 *  Argument Query (convenience)
 * ============================================================ */

int32_t csoundUgenGetInCount(UGEN* ugen) {
    return (ugen != NULL) ? ugen->inCount : 0;
}

int32_t csoundUgenGetOutCount(UGEN* ugen) {
    return (ugen != NULL) ? ugen->outCount : 0;
}

UGEN_ARG_TYPE csoundUgenGetInType(UGEN* ugen, int32_t index) {
    if (ugen == NULL || index < 0 || index >= ugen->inCount)
        return UGEN_ARG_TYPE_UNKNOWN;
    return ugen->inTypes[index];
}

UGEN_ARG_TYPE csoundUgenGetOutType(UGEN* ugen, int32_t index) {
    if (ugen == NULL || index < 0 || index >= ugen->outCount)
        return UGEN_ARG_TYPE_UNKNOWN;
    return ugen->outTypes[index];
}

/* ============================================================
 *  Init / Perform
 * ============================================================ */

int32_t csoundUgenInit(UGEN* ugen) {
    if (ugen == NULL) return CSOUND_ERROR;
    OENTRY* oentry = ugen->oentry;
    if (oentry->init != NULL) {
        return (*oentry->init)(ugen->csound, ugen->opcodeMem);
    }
    return CSOUND_SUCCESS;
}

int32_t csoundUgenPerform(UGEN* ugen) {
    if (ugen == NULL) return CSOUND_ERROR;
    OENTRY* oentry = ugen->oentry;
    if (oentry->perf != NULL) {
        return (*oentry->perf)(ugen->csound, ugen->opcodeMem);
    }
    return CSOUND_SUCCESS;
}

/* ============================================================
 *  Opcode Listing API
 * ============================================================ */

int32_t csoundUgenListOpcodes(UGEN_FACTORY* factory,
                          UGEN_OPCODE_INFO** list, int32_t* count) {
    if (factory == NULL || list == NULL || count == NULL)
        return CSOUND_ERROR;

    CSOUND* csound = factory->csound;

    /* Use Csound's internal opcode hash table to enumerate all opcodes.
     * csound->opcodes is a CS_HASH_TABLE. cs_hash_table_values() returns
     * a CONS_CELL list where each cell's value is itself a CONS_CELL chain
     * containing OENTRY* values (matching the pattern in Top/opcode.c). */
    if (csound->opcodes == NULL) {
        *list = NULL;
        *count = 0;
        return CSOUND_SUCCESS;
    }

    CONS_CELL* head = cs_hash_table_values(csound, csound->opcodes);

    /* First pass: count all opcode entries */
    int32_t totalCount = 0;
    CONS_CELL* items = head;
    while (items != NULL) {
        CONS_CELL* temp = (CONS_CELL*)items->value;
        while (temp != NULL) {
            OENTRY* ep = (OENTRY*)temp->value;
            if (ep != NULL && ep->opname != NULL && ep->opname[0] != '\0'
                && ep->outypes != NULL && ep->intypes != NULL) {
                totalCount++;
            }
            temp = temp->next;
        }
        items = items->next;
    }

    /* Allocate the info array */
    UGEN_OPCODE_INFO* info = csound->Calloc(csound,
                                            totalCount * sizeof(UGEN_OPCODE_INFO));
    if (info == NULL) {
        cs_cons_free(csound, head);
        *list = NULL;
        *count = 0;
        return CSOUND_MEMORY;
    }

    /* Second pass: fill the array */
    int32_t idx = 0;
    items = head;
    while (items != NULL) {
        CONS_CELL* temp = (CONS_CELL*)items->value;
        while (temp != NULL) {
            OENTRY* ep = (OENTRY*)temp->value;
            if (ep != NULL && ep->opname != NULL && ep->opname[0] != '\0'
                && ep->outypes != NULL && ep->intypes != NULL
                && idx < totalCount) {
                info[idx].opname = ep->opname;
                info[idx].outypes = ep->outypes;
                info[idx].intypes = ep->intypes;
                info[idx].dsblksiz = ep->dsblksiz;
                info[idx].flags = ep->flags;
                idx++;
            }
            temp = temp->next;
        }
        items = items->next;
    }

    cs_cons_free(csound, head);
    *list = info;
    *count = idx;
    return CSOUND_SUCCESS;
}

void csoundUgenFreeOpcodeList(UGEN_FACTORY* factory, UGEN_OPCODE_INFO* list) {
    if (factory != NULL && list != NULL) {
        CSOUND* csound = factory->csound;
        csound->Free(csound, list);
    }
}

bool csoundUgenFindOpcode(UGEN_FACTORY* factory, const char* opname,
                      const char* outargTypes, const char* inargTypes) {
    if (factory == NULL || opname == NULL) return false;

    CSOUND* csound = factory->csound;
    OENTRIES* entries = find_opcode2(csound, (char*)opname);
    if (entries == NULL) return false;

    for (int32_t i = 0; i < entries->count; i++) {
        OENTRY* temp = entries->entries[i];
        if (temp == NULL) continue;

        bool outMatch = (outargTypes == NULL) ||
                        (strcmp(outargTypes, temp->outypes) == 0);
        bool inMatch = (inargTypes == NULL) ||
                       (strcmp(inargTypes, temp->intypes) == 0);

        if (outMatch && inMatch) {
            return true;
        }
    }
    return false;
}

/* ============================================================
 *  UGen Graph API
 * ============================================================ */

#define UGEN_GRAPH_INITIAL_CAPACITY 16

UGEN_GRAPH* csoundUgenGraphNew(UGEN_FACTORY* factory) {
    if (factory == NULL) return NULL;
    CSOUND* csound = factory->csound;

    UGEN_GRAPH* graph = csound->Calloc(csound, sizeof(UGEN_GRAPH));
    graph->factory = factory;
    graph->capacity = UGEN_GRAPH_INITIAL_CAPACITY;
    graph->count = 0;
    graph->ugens = csound->Calloc(csound,
                                  UGEN_GRAPH_INITIAL_CAPACITY * sizeof(UGEN*));
    return graph;
}

int32_t csoundUgenGraphAdd(UGEN_GRAPH* graph, UGEN* ugen) {
    if (graph == NULL || ugen == NULL) return -1;

    /* Grow array if needed */
    if (graph->count >= graph->capacity) {
        CSOUND* csound = graph->factory->csound;
        int32_t newCap = graph->capacity * 2;
        UGEN** newArr = csound->Calloc(csound, newCap * sizeof(UGEN*));
        memcpy(newArr, graph->ugens, graph->count * sizeof(UGEN*));
        csound->Free(csound, graph->ugens);
        graph->ugens = newArr;
        graph->capacity = newCap;
    }

    int32_t idx = graph->count;
    graph->ugens[graph->count++] = ugen;
    return idx;
}

int32_t csoundUgenGraphInit(UGEN_GRAPH* graph) {
    if (graph == NULL) return CSOUND_ERROR;

    for (int32_t i = 0; i < graph->count; i++) {
        int32_t ret = csoundUgenInit(graph->ugens[i]);
        if (ret != CSOUND_SUCCESS) return ret;
    }
    return CSOUND_SUCCESS;
}

int32_t csoundUgenGraphPerform(UGEN_GRAPH* graph) {
    if (graph == NULL) return CSOUND_ERROR;

    for (int32_t i = 0; i < graph->count; i++) {
        int32_t ret = csoundUgenPerform(graph->ugens[i]);
        if (ret != CSOUND_SUCCESS) return ret;
    }
    return CSOUND_SUCCESS;
}

bool csoundUgenGraphDelete(UGEN_GRAPH* graph) {
    if (graph == NULL) return false;
    CSOUND* csound = graph->factory->csound;
    csound->Free(csound, graph->ugens);
    csound->Free(csound, graph);
    return true;
}

bool csoundUgenGraphDeleteAll(UGEN_GRAPH* graph) {
    if (graph == NULL) return false;
    CSOUND* csound = graph->factory->csound;

    for (int32_t i = 0; i < graph->count; i++) {
        if (graph->ugens[i] != NULL) {
            csoundUgenDelete(graph->ugens[i]);
        }
    }
    csound->Free(csound, graph->ugens);
    csound->Free(csound, graph);
    return true;
}
