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
 * - User connects arguments using csoundUgenSetInput / csoundUgenSetOutput
 *   or csoundUgenGraphConnect to build a signal graph
 * - User calls csoundUgenInit / csoundUgenPerform (or graph equivalents) to
 *   run the processing
 *
 * - context: required for things like hold, releasing, etc.
 * */

#include "ugen_internal.h"
#include "csound_standard_types.h"
#include "csound_orc.h"
#include "csound_orc_semantics.h"
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
 * current ksmps value.  Returns sizeof(MYFLT) for scalar types (i, k),
 * ksmps * sizeof(MYFLT) for audio-rate.
 */
static size_t ugen_arg_type_size(UGEN_ARG_TYPE type, int32_t ksmps) {
    if (type == UGEN_ARG_TYPE_A) {
        return (size_t)ksmps * sizeof(MYFLT);
    }
    /* k, i, and other scalar types */
    return sizeof(MYFLT);
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

    /* Reject opcodes whose resolved signature contains S or f types.
     * The UGen data layout does not run type-specific init/free hooks
     * (e.g. STRINGDAT allocation), so these would malfunction. */
    for (i = 0; i < outCount; i++) {
        if (ugen->outTypes[i] == UGEN_ARG_TYPE_S ||
            ugen->outTypes[i] == UGEN_ARG_TYPE_F) {
            goto reject_unsupported_types;
        }
    }
    for (i = 0; i < inCount; i++) {
        if (ugen->inTypes[i] == UGEN_ARG_TYPE_S ||
            ugen->inTypes[i] == UGEN_ARG_TYPE_F) {
            goto reject_unsupported_types;
        }
    }
    if (0) {
reject_unsupported_types:
        csound->Free(csound, ugen->outTypes);
        csound->Free(csound, ugen->inTypes);
        csound->Free(csound, ugen->opcodeMem);
        csound->Free(csound, optxt);
        csound->Free(csound, ugen);
        csound->Free(csound, parsedOutTypes);
        csound->Free(csound, parsedInTypes);
        return NULL;
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
    csound->Free(csound, ugen);
    return true;
}

/* ============================================================
 *  Argument Handling: By Pointer (zero-copy)
 * ============================================================ */

bool csoundUgenSetOutput(UGEN* ugen, int32_t index, void* arg) {
    if (ugen == NULL || index < 0 || index >= ugen->outCount) return false;

    MYFLT** p = get_arg_pointers(ugen->opcodeMem);
    p[index] = (MYFLT*)arg;
    return true;
}

bool csoundUgenSetInput(UGEN* ugen, int32_t index, void* arg) {
    if (ugen == NULL || index < 0 || index >= ugen->inCount) return false;

    MYFLT** p = get_arg_pointers(ugen->opcodeMem);
    p[ugen->outCount + index] = (MYFLT*)arg;
    return true;
}

/* ============================================================
 *  Argument Handling: By Value (copy)
 * ============================================================ */

bool csoundUgenSetOutputValue(UGEN* ugen, int32_t index, void* arg) {
    if (ugen == NULL || index < 0 || index >= ugen->outCount || arg == NULL)
        return false;

    MYFLT** p = get_arg_pointers(ugen->opcodeMem);
    size_t sz = ugen_arg_type_size(ugen->outTypes[index], ugen->insds->ksmps);
    memcpy(p[index], arg, sz);
    return true;
}

bool csoundUgenSetInputValue(UGEN* ugen, int32_t index, void* arg) {
    if (ugen == NULL || index < 0 || index >= ugen->inCount || arg == NULL)
        return false;

    MYFLT** p = get_arg_pointers(ugen->opcodeMem);
    size_t sz = ugen_arg_type_size(ugen->inTypes[index], ugen->insds->ksmps);
    memcpy(p[ugen->outCount + index], arg, sz);
    return true;
}

size_t csoundUgenGetOutputValue(UGEN* ugen, int32_t index, void* dest) {
    if (ugen == NULL || index < 0 || index >= ugen->outCount || dest == NULL)
        return 0;

    MYFLT** p = get_arg_pointers(ugen->opcodeMem);
    size_t sz = ugen_arg_type_size(ugen->outTypes[index], ugen->insds->ksmps);
    memcpy(dest, p[index], sz);
    return sz;
}

size_t csoundUgenGetInputValue(UGEN* ugen, int32_t index, void* dest) {
    if (ugen == NULL || index < 0 || index >= ugen->inCount || dest == NULL)
        return 0;

    MYFLT** p = get_arg_pointers(ugen->opcodeMem);
    size_t sz = ugen_arg_type_size(ugen->inTypes[index], ugen->insds->ksmps);
    memcpy(dest, p[ugen->outCount + index], sz);
    return sz;
}

/* ============================================================
 *  Argument Query
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

size_t csoundUgenGetInArgSize(UGEN* ugen, int32_t index) {
    if (ugen == NULL || index < 0 || index >= ugen->inCount) return 0;
    return ugen_arg_type_size(ugen->inTypes[index], ugen->insds->ksmps);
}

size_t csoundUgenGetOutArgSize(UGEN* ugen, int32_t index) {
    if (ugen == NULL || index < 0 || index >= ugen->outCount) return 0;
    return ugen_arg_type_size(ugen->outTypes[index], ugen->insds->ksmps);
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

bool csoundUgenGraphConnect(UGEN* source, int32_t outIdx,
                        UGEN* dest, int32_t inIdx) {
    if (source == NULL || dest == NULL) return false;
    if (outIdx < 0 || outIdx >= source->outCount) return false;
    if (inIdx < 0 || inIdx >= dest->inCount) return false;

    /* Get the pointer to source's output data */
    MYFLT** srcP = get_arg_pointers(source->opcodeMem);
    MYFLT* outPtr = srcP[outIdx];

    /* Set dest's input to point to source's output (zero-copy wiring) */
    return csoundUgenSetInput(dest, inIdx, outPtr);
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
