/*
 csound_standard_types.c:

 Copyright (C) 2012,2013 Steven Yi

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA
 */

#include "csoundCore.h"
#include "csound_standard_types.h"
#include "pstream.h"
#include "find_opcode.h"
#include <stdlib.h>


/* MEMORY COPYING FUNCTIONS */
static void myflt_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                      const void* src, INSDS *ctx) {
   memcpy(dest, src, sizeof(MYFLT));
}

static void asig_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                     const void* src, INSDS *ctx) {
  int32_t ksmps = ctx ? ctx->ksmps : csound->ksmps;
  memcpy(dest, src, sizeof(MYFLT) * ksmps);
}

static void complex_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                        const void* src, INSDS *ctx) {
  memcpy(dest, src, sizeof(COMPLEXDAT));
}

static void wsig_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                     const void* src, INSDS *ctx) {
    memcpy(dest, src, sizeof(SPECDAT));
    //TODO - check if this needs to copy SPECDAT's DOWNDAT member and AUXCH
}

static void fsig_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                     const void* src, INSDS *ctx) {
    PVSDAT *fsigout = (PVSDAT*) dest;
    PVSDAT *fsigin = (PVSDAT*) src;
    int32_t N = fsigin->N;
    memcpy(dest, src, sizeof(PVSDAT) - sizeof(AUXCH));
    if(fsigout->frame.auxp == NULL ||
       fsigout->frame.size < (N + 2) * sizeof(float))
      ((CSOUND *)csound)->AuxAlloc(csound,
                                   (N + 2) * sizeof(float), &fsigout->frame);
    memcpy(fsigout->frame.auxp, fsigin->frame.auxp, (N + 2) * sizeof(float));
}

/* String buffer management utility */
static void string_free_internal(CSOUND* csound, STRINGDAT* str) {
    if (!str || !str->data) return;

    // refcount == -1 means this is an alias (doesn't own the data)
    if (str->refcount == -1) {
        str->data = NULL;  // Just clear the pointer, don't free
        str->size = 0;
        return;
    }

    // refcount == 0: we own the buffer and should free it
    csound->Free(csound, str->data);
    str->data = NULL;
    str->size = 0;
}

static void string_resize_internal(CSOUND* csound, STRINGDAT* str, size_t newSize) {
    if (!str) return;
    if (newSize == 0) newSize = 1;  // always room for '\0'
    // If this is an alias (refcount == -1), we need to allocate our own buffer
    if (str->refcount == -1) {
        char* oldData = str->data;
        str->data = csound->Calloc(csound, newSize);
        str->size = newSize;
        str->refcount = 0;  // Now we own it
        if (oldData) {
            strncpy(str->data, oldData, newSize - 1);
            str->data[newSize - 1] = '\0';
        }
    } else {
        // Safe to resize - we own the buffer
        str->data = csound->ReAlloc(csound, str->data, newSize);
        str->size = newSize;
        str->data[newSize - 1] = '\0';
    }
}

static void string_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                       const void* src, INSDS *p) {
    STRINGDAT* sDest = (STRINGDAT*)dest;
    STRINGDAT* sSrc = (STRINGDAT*)src;

    if (UNLIKELY(src == NULL)) return;
    if (UNLIKELY(dest == NULL)) return;

    /* Check for buffer aliasing: if both STRINGDATs point to the same data buffer,
       they are either the same variable or share the same underlying memory.
       In either case, no copy is needed - just update the timestamp. */
    if (UNLIKELY(sDest->data == sSrc->data)) {
        sDest->timestamp = csound->kcounter;
        return;
    }

    /* Guard against NULL data pointers */
    if (UNLIKELY(sSrc->data == NULL)) return;
    if (UNLIKELY(sDest->data == NULL)) {
        /* Destination has no buffer; allocate one to match source size */
        sDest->data = csound->Calloc(csound, sSrc->size);
        sDest->size = sSrc->size;
        sDest->refcount = 0;
    }

    int64_t kcnt = csound->kcounter;
    if (sSrc->size > sDest->size) {
      string_resize_internal(csound, sDest, sSrc->size);
      memcpy(sDest->data, sSrc->data, sSrc->size);
    } else {
        strncpy(sDest->data, sSrc->data, sDest->size-1);
        sDest->data[sDest->size-1] = '\0';
    }
    sDest->timestamp = kcnt;
}

static size_t array_get_num_members(ARRAYDAT* aSrc) {
    int32_t i, retVal = 0;

    if (aSrc->dimensions <= 0) {
      return retVal;
    }

    retVal = aSrc->sizes[0];

    for (i = 1; i < aSrc->dimensions; i++) {
      retVal *= aSrc->sizes[i];
    }
    return (size_t)retVal;
}

/*
 * array_copy_value - Copy array data with optimized handling for user-defined types
 *
 * Memory Ownership Semantics:
 * - For UDT arrays (user-defined structs): Creates non-owning aliases (allocated=0)
 *   where aDest->sizes and aDest->data point to aSrc's buffers
 * - For regular arrays: Performs deep copy with owned memory allocation
 * - Pre-existing destination memory is always freed before reallocation
 * - Early alias check prevents unnecessary allocations and potential leaks
 *
 * Parameters:
 *   csound - Csound instance
 *   cstype - Type information for validation
 *   dest - Destination ARRAYDAT (may be uninitialized)
 *   src - Source ARRAYDAT (must be valid)
 *   ctx - Instrument context for variable creation
 */
static void array_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                      const void* src, INSDS *ctx) {
    ARRAYDAT* aDest = (ARRAYDAT*)dest;
    ARRAYDAT* aSrc = (ARRAYDAT*)src;
    CSOUND* cs = (CSOUND*)csound;
    CS_VARIABLE* var;
    size_t j;
    int32_t memMyfltSize;
    size_t arrayNumMembers;

    /* If destination is NULL, nothing to do. */
    if (UNLIKELY(aDest == NULL)) {
        return;
    }

    /* Optimization and safety: for arrays of user-defined structs, perform a
       shallow alias instead of deep-copying elements. This avoids heavy copy
       and shared-ownership bugs; alias is non-owning (allocated=0).
       This check is done BEFORE any allocations to prevent memory leaks. */
    if (aSrc && aSrc->arrayType && aSrc->arrayType->userDefinedType) {
        /* Free any existing destination storage before creating alias */
        if (aDest->sizes != NULL) {
            cs->Free(cs, aDest->sizes);
            aDest->sizes = NULL;
        }
        if (aDest->data != NULL) {
            cs->Free(cs, aDest->data);
            aDest->data = NULL;
        }

        /* Shallow alias for arrays of structs - non-owning reference */
        aDest->arrayMemberSize = aSrc->arrayMemberSize;
        aDest->dimensions      = aSrc->dimensions;
        aDest->sizes           = aSrc->sizes;
        aDest->arrayType       = aSrc->arrayType;
        aDest->data            = aSrc->data;
        aDest->allocated       = 0; /* Non-owning alias */
        return;
    }

    /* If destination is an uninitialised array shell (arrayType == NULL),
       initialise it from the source so we can perform a copy. */
    if (UNLIKELY(aDest->arrayType == NULL) && aSrc && aSrc->arrayType != NULL) {
        /* Bootstrap destination metadata and storage from source */
        aDest->arrayType = aSrc->arrayType;
        aDest->dimensions = aSrc->dimensions;
        if (aDest->sizes != NULL) { cs->Free(cs, aDest->sizes); }
        aDest->sizes = cs->Malloc(cs, sizeof(int32_t) * aSrc->dimensions);
        memcpy(aDest->sizes, aSrc->sizes, sizeof(int32_t) * aSrc->dimensions);
        aDest->arrayMemberSize = aSrc->arrayMemberSize;
        size_t nm = array_get_num_members(aSrc);
        if (aDest->data != NULL) { cs->Free(cs, aDest->data); }
        aDest->data = cs->Calloc(cs, aSrc->arrayMemberSize * nm);
        aDest->allocated = aSrc->arrayMemberSize * nm;
    }

    arrayNumMembers = array_get_num_members(aSrc);
    memMyfltSize = aSrc->arrayMemberSize / sizeof(MYFLT);

    /* If source and destination are the same array object, skip copy */
    if (aDest == aSrc) {
        return;
    }

    if(aDest->data == NULL ||
       aSrc->arrayMemberSize != aDest->arrayMemberSize ||
       aSrc->dimensions != aDest->dimensions ||
       aSrc->arrayType != aDest->arrayType ||
       arrayNumMembers != array_get_num_members(aDest)) {

        aDest->arrayMemberSize = aSrc->arrayMemberSize;
        aDest->dimensions = aSrc->dimensions;
        if(aDest->sizes != NULL) {
            cs->Free(cs, aDest->sizes);
        }
        aDest->sizes = cs->Malloc(cs, sizeof(int32_t) * aSrc->dimensions);
        memcpy(aDest->sizes, aSrc->sizes, sizeof(int32_t) * aSrc->dimensions);
        aDest->arrayType = aSrc->arrayType;
        if(aDest->data != NULL) {
            cs->Free(cs, aDest->data);
        }
        aDest->data = cs->Calloc(cs, aSrc->arrayMemberSize * arrayNumMembers);
        aDest->allocated = aSrc->arrayMemberSize * arrayNumMembers;
    }

    var = aDest->arrayType->createVariable(cs, (void *)aDest->arrayType, ctx);
    for (j = 0; j < arrayNumMembers; j++) {
        size_t index = j * memMyfltSize;
        if(var->initializeVariableMemory != NULL) {
          var->initializeVariableMemory(csound, var, aDest->data + index);
        }
        aDest->arrayType->copyValue(csound, aDest->arrayType,
                                    aDest->data + index,
                                    aSrc->data + index, ctx);
    }

}

static void opcodedef_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                      const void* src, INSDS *ctx) {
  OPCODEREF *p = (OPCODEREF *) dest;
  if(!p->readonly) {
   memcpy(dest, src, sizeof(OPCODEREF));
   p->readonly = 0; // clear readonly flag (which is not copied)
  }
  else csound->Warning(csound, "%s (:OpcodeDef) is read-only: "
                                "cannot be redefined, ignoring assignment",
                       get_opcode_short_name(csound, p->entries->entries[0]->opname));
}

// from opcode.c
int32_t context_check(CSOUND* csound, OPCODEOBJ *p, INSDS *ctx);
static void opcode_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                      const void* src, INSDS *ctx) {
  OPCODEOBJ *p = (OPCODEOBJ *) dest;
  OPCODEOBJ *psrc = (OPCODEOBJ *) src;
  if(psrc->dataspace != NULL && context_check(csound, psrc, ctx) != 0) {
    csound->Warning(csound, "mismatching context: copy value bypassed");
    return;
  }
  if(!p->readonly) {
   memcpy(dest, src, sizeof(OPCODEOBJ));
   p->readonly = 0; // clear readonly flag (which is not copied)
  }
  else csound->Warning(csound, "opcode instance var is read-only:"
                       " copy value bypassed");
}


static void instrdef_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                      const void* src, INSDS *ctx) {
  INSTREF *p = (INSTREF *) dest;
  if(!p->readonly) {
   memcpy(dest, src, sizeof(INSTREF));
   p->readonly = 0; // clear readonly flag (which is not copied)
  }
  else csound->Warning(csound, "instr ref var %s is read-only: copy value bypassed",
                       p->instr ? p->instr->insname : "(uninitialized)");
}

static void instr_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                      const void* src, INSDS *ctx) {
  INSTANCEREF *p = (INSTANCEREF *) dest;
  if(!p->readonly) {
   memcpy(dest, src, sizeof(INSTANCEREF));
   p->readonly = 0; // clear readonly flag (which is not copied)
  }
  else csound->Warning(csound, "instance ref var is read-only: copy value bypassed");
}


/* MEM SIZE UPDATING FUNCTIONS */
static void update_asig_memblock(CSOUND* csound, CS_VARIABLE* var) {
    int32_t ksmps = csound->ksmps;
    var->memBlockSize = CS_FLOAT_ALIGN(ksmps * sizeof (MYFLT));
}

static void var_init_memory(CSOUND *csound, CS_VARIABLE* var, MYFLT* memblock) {
    IGN(csound);
    memset(memblock, 0, var->memBlockSize);
}


static void array_init_memory(CSOUND *csound, CS_VARIABLE* var, MYFLT* memblock) {
    ARRAYDAT* dat = (ARRAYDAT*)memblock;

    dat->arrayType = var->subType;

    // Always initialize all fields to prevent uninitialized memory issues
    dat->data = NULL;
    dat->allocated = 0;
    dat->arrayMemberSize = 0;
    dat->dimensions = 0;
    dat->sizes = NULL;

    // Initialize array dimensions if they were set during variable creation
    if (var->dimensions > 0) {
        dat->dimensions = var->dimensions;
        dat->sizes = csound->Calloc(csound, sizeof(int32_t) * var->dimensions);

        // For struct arrays declared with init (e.g., "relatives:Person[] init 2"),
        // we need to set default sizes. The actual sizing will happen during
        // the init opcode execution, but we need the metadata in place.
        for (int32_t i = 0; i < var->dimensions; i++) {
            dat->sizes[i] = 0; // Will be set by init opcode
        }
    }
}

static void var_init_memory_string(CSOUND *csound, CS_VARIABLE* var, MYFLT* memblock) {
    STRINGDAT *str = (STRINGDAT *)memblock;
    str->data = (char *) csound->Calloc(csound, DEFAULT_STRING_SIZE);
    str->size = DEFAULT_STRING_SIZE;
    str->timestamp = 0;
    str->refcount = 0;  // Initialize refcount (0 = unmanaged)
}

static void var_init_memory_fsig(CSOUND *csound, CS_VARIABLE* var, MYFLT* memblock) {
    PVSDAT *fsig = (PVSDAT *)memblock;
    IGN(csound);
    memset(fsig, 0, sizeof(PVSDAT));  /* VL: clear memory for now */
}

/* CREATE VAR FUNCTIONS */

static CS_VARIABLE* create_asig(void* cs, void* p, INSDS *ctx) {
    int32_t ksmps;
    CSOUND* csound = (CSOUND*)cs;
    IGN(p);

   if (ctx  != NULL) {
      ksmps = ctx->ksmps;
   } else {
    ksmps = csound->ksmps;
    }

    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = CS_FLOAT_ALIGN(ksmps * sizeof (MYFLT));
    var->updateMemBlockSize = &update_asig_memblock;
    var->initializeVariableMemory = &var_init_memory;
    var->ctx = ctx;
    return var;
}

static CS_VARIABLE* create_myflt(void* cs, void* p, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof (MYFLT));
    var->initializeVariableMemory = &var_init_memory;
    var->ctx = ctx;
    return var;
}

static CS_VARIABLE* create_complex(void* cs, void* p, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(COMPLEXDAT));
    var->initializeVariableMemory = &var_init_memory;
    var->ctx = ctx;
    return var;
}

static CS_VARIABLE* create_bool(void* cs, void* p, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(int32_t));
    var->initializeVariableMemory = &var_init_memory;
    var->ctx = ctx;
    return var;
}

static CS_VARIABLE* create_wsig(void* cs, void* p, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(SPECDAT));
    var->initializeVariableMemory = &var_init_memory;
    var->ctx = ctx;
    return var;
}

static CS_VARIABLE* create_fsig(void* cs, void* p, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(PVSDAT));
    var->initializeVariableMemory = &var_init_memory_fsig;
    var->ctx = ctx;
    return var;
}

static CS_VARIABLE* create_string(void* cs, void* p, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(STRINGDAT));
    var->initializeVariableMemory = &var_init_memory_string;
    var->ctx = ctx;
    return var;
}

CS_VARIABLE* create_array(void* csnd, void* p, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)csnd;
    ARRAY_VAR_INIT* state = (ARRAY_VAR_INIT*)p;

    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(ARRAYDAT));
    var->initializeVariableMemory = &array_init_memory;
    var->ctx = ctx;

    if (state) { // NB: this function is being called with p=NULL
      const CS_TYPE* type = state->type;
      var->subType = type;
      var->dimensions = state->dimensions;
    }
    return var;
}


static CS_VARIABLE* create_opcodedef(void* csnd, void* p, INSDS *ctx) {
   CSOUND* csound = (CSOUND*)csnd;
   CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
   var->memBlockSize = CS_FLOAT_ALIGN(sizeof(OPCODEREF));
   var->initializeVariableMemory = &var_init_memory;
   var->ctx = ctx;
   return var;
}

static CS_VARIABLE* create_opcode(void* csnd, void* p, INSDS *ctx) {
   CSOUND* csound = (CSOUND*)csnd;
   CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
   var->memBlockSize = CS_FLOAT_ALIGN(sizeof(OPCODEOBJ));
   var->initializeVariableMemory = &var_init_memory;
   var->ctx = ctx;
   return var;
}

static CS_VARIABLE* create_instrdef(void* csnd, void* p, INSDS *ctx) {
   CSOUND* csound = (CSOUND*)csnd;
   CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
   var->memBlockSize = CS_FLOAT_ALIGN(sizeof(INSTREF));
   var->initializeVariableMemory = &var_init_memory;
   var->ctx = ctx;
   return var;
}

static CS_VARIABLE* create_instr(void* csnd, void* p, INSDS *ctx) {
   CSOUND* csound = (CSOUND*)csnd;
   CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
   var->memBlockSize = CS_FLOAT_ALIGN(sizeof(INSTANCEREF));
   var->initializeVariableMemory = &var_init_memory;
   var->ctx = ctx;
   return var;
}


/* FREE VAR MEM FUNCTIONS */
static void string_free_var_mem(void* csnd, void* p) {
    CSOUND* csound = (CSOUND*)csnd;
    STRINGDAT* dat = (STRINGDAT*)p;
    string_free_internal(csound, dat);
}

static void array_free_var_mem(void* csnd, void* p) {
    CSOUND* csound = (CSOUND*)csnd;
    ARRAYDAT* dat = (ARRAYDAT*)p;

    /* Only free backing storage if this ARRAYDAT owns it. Aliases set allocated=0. */
    if (dat->allocated > 0 && dat->data != NULL) {
        const CS_TYPE* arrayType = dat->arrayType;

        if (arrayType->freeVariableMemory != NULL) {
            MYFLT* mem = dat->data;
            size_t memMyfltSize = 0;
            // Compute element count without trusting dat->sizes pointer, which may be shared/aliased
            size_t i;
            size_t elemCount = 0;

            // Safe division with checks for arrayMemberSize and sizeof(MYFLT)
            if (dat->arrayMemberSize > 0 && sizeof(MYFLT) > 0) {
                memMyfltSize = (size_t)dat->arrayMemberSize / sizeof(MYFLT);

                if (dat->allocated > 0) {
                    elemCount = (size_t)dat->allocated / (size_t)dat->arrayMemberSize;
                }
            } else if (dat->sizes) {
                // Fallback to sizes if available
                elemCount = (dat->dimensions > 0) ? (size_t)dat->sizes[0] : 0;
                for (i = 1; i < (size_t)dat->dimensions; i++) {
                    // Check for multiplication overflow
                    if (elemCount > 0 && (size_t)dat->sizes[i] > SIZE_MAX / elemCount) {
                        // Overflow would occur, cap at SIZE_MAX
                        elemCount = SIZE_MAX;
                        break;
                    }
                    elemCount *= (size_t)dat->sizes[i];
                }
            }

            for (i = 0; i < elemCount; i++) {
                arrayType->freeVariableMemory(csound,
                                              mem + (i * memMyfltSize));
            }
        }

        csound->Free(csound, dat->data);
    }

    if (dat->allocated > 0 && dat->sizes != NULL) {
        csound->Free(csound, dat->sizes);
    }
}

/* STANDARD TYPE DEFINITIONS */
const CS_TYPE CS_VAR_TYPE_A = {
    "a", "audio rate vector", CS_ARG_TYPE_BOTH, create_asig, asig_copy_value,
    NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_K = {
  "k", "control rate var", CS_ARG_TYPE_BOTH, create_myflt, myflt_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_I = {
  "i", "init time var", CS_ARG_TYPE_BOTH, create_myflt, myflt_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_S = {
  "S", "String var", CS_ARG_TYPE_BOTH, create_string, string_copy_value, string_free_var_mem, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_P = {
  "p", "p-field", CS_ARG_TYPE_BOTH, create_myflt, myflt_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_R = {
  "r", "reserved symbol", CS_ARG_TYPE_BOTH, create_myflt, myflt_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_C = {
  "c", "constant", CS_ARG_TYPE_IN, create_myflt, myflt_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_W = {
  "w", "spectral", CS_ARG_TYPE_BOTH, create_wsig, wsig_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_F = {
  "f", "f-sig", CS_ARG_TYPE_BOTH, create_fsig, fsig_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_B = {
  "B", "boolean", CS_ARG_TYPE_BOTH, create_bool, myflt_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_b = {
  "b", "boolean", CS_ARG_TYPE_BOTH, create_bool, myflt_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_ARRAY = {
  "[", "array", CS_ARG_TYPE_BOTH, create_array, array_copy_value,
  array_free_var_mem, NULL, 0
};


const CS_TYPE CS_VAR_TYPE_OPCODEREF = {
  "OpcodeDef", "opcode definition reference", CS_ARG_TYPE_BOTH,
  create_opcodedef, opcodedef_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_OPCODEOBJ = {
  "Opcode", "opcode instance reference", CS_ARG_TYPE_BOTH,
  create_opcode, opcode_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_INSTR = {
  "InstrDef", "instrument definition reference", CS_ARG_TYPE_BOTH,
  create_instrdef, instrdef_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_INSTR_INSTANCE = {
  "Instr", "instrument instance reference", CS_ARG_TYPE_BOTH,
  create_instr, instr_copy_value, NULL, NULL, 0
};

const CS_TYPE CS_VAR_TYPE_COMPLEX = {
  "Complex", "complex", CS_ARG_TYPE_BOTH, create_complex, complex_copy_value,
    NULL, NULL, 0
};

void add_standard_types(CSOUND* csound, TYPE_POOL* pool) {
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_A);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_K);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_I);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_COMPLEX);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_S);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_P);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_R);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_C);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_W);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_F);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_B);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_b);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_OPCODEREF);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_OPCODEOBJ);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_ARRAY);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_INSTR);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_INSTR_INSTANCE);

    // CS_OBJ_TYPE & OPS
    add_csobj(csound, pool);
}


/* Type maps for poly, optional, and var arg types
 * format is in pairs of specified type and types it can resolve into,
 * termintated by a NULL */
const char* POLY_IN_TYPES[] = {
    "x", "kacpri",              /* ***Deprecated*** */
    "T", "Sicpr",
    "U", "Sikcpr",
    "i", "cpri",
    "k", "cprki",
    "B", "Bb", NULL};
const char* OPTIONAL_IN_TYPES[] = {
    "o", "icpr",
    "p", "icpr",
    "q", "icpr",
    "v", "icpr",
    "j", "icpr",
    "h", "icpr",
    "O", "kicpr",
    "J", "kicpr",
    "V", "kicpr",
    "P", "kicpr", NULL
};
const char* VAR_ARG_IN_TYPES[] = {
    "m", "icrpb",
    "M", "icrpkabB",
    "N", "icrpkaSbB",
    "n", "icrpb",   /* this one requires odd number of args... */
    "W", "S",
    "y", "a",
    "z", "kicrpbB",
    "Z", "kaicrpbB",  NULL  /* this one needs to be ka alternatating... */
};

const char* POLY_OUT_TYPES[] = {
    "s", "ka",                  /* ***Deprecated*** */
    "i", "pi", NULL
};

const char* VAR_ARG_OUT_TYPES[] = {
    "m", "a",
    "z", "k",
    "I", "Sip", /* had comment of (not implemented yet) in entry.c */
    "X", "akip",
    "N", "akipS",
    "v", "b",
    "V", "B",
    "F", "f", NULL
};
