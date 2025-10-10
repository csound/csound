/*
    structs.c:

    Copyright (C) 2023

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"
#include "interlocks.h"
#include "csound_standard_types.h"
#include "csound_orc_structs.h"  /* CS_STRUCT_VAR + free helper */
#include <math.h>  /* for fabs */

/* Free helper from semantics for deep-freeing member arrays safely */
extern void csound_free_struct_members(CSOUND* cs, CS_STRUCT_VAR* v);

typedef struct {
    OPDS          h;
    MYFLT*        out;
    MYFLT*        var;          // Struct variable (will be cast to CS_STRUCT_VAR*)
    MYFLT*        nths[1];      // Member index (constant) - MUST be last, variable length!
} STRUCT_GET;

typedef struct {
    OPDS          h;
    MYFLT*        var;          // Struct variable (will be cast to CS_STRUCT_VAR*)
    MYFLT*        nths[1];      // Member index (constant)
    MYFLT*        in;           // Value to set
} STRUCT_SET;

typedef struct {
    OPDS          h;
    MYFLT*        var;          // Struct variable (will be cast to CS_STRUCT_VAR*)
    MYFLT*        nths[1];      // Member index (constant)
    ARRAYDAT*     in;           // Array to assign
} STRUCT_MEMBER_ARRAY_ASSIGN;

typedef struct {
  OPDS      h;
  CS_STRUCT_VAR* dst;
  CS_STRUCT_VAR* src;
  /* Deferred-free capture of pre-alias destination members */
  CS_VAR_MEM** oldMembers;
  int32_t      oldMemberCount;
  int32_t      oldOwned;
} STRUCT_ALIAS;

typedef struct {
    OPDS          h;
    CS_STRUCT_VAR*   out;
    MYFLT*        args[VARGMAX];
} STRUCT_INIT;

typedef struct {
  OPDS      h;
  MYFLT*    out;
  ARRAYDAT* arrayDat;
  MYFLT*    indicies[VARGMAX];
} STRUCT_ARRAY_GET;

static void struct_array_member_assign(
    ARRAYDAT* arraySrc,
    ARRAYDAT* arrayDst,
    const CS_TYPE* memberVarType
) {
    /* Shallow alias: destination does not own storage */
    arrayDst->allocated = 0;
    arrayDst->arrayMemberSize = arraySrc->arrayMemberSize;
    arrayDst->data = arraySrc->data;
    arrayDst->dimensions = arraySrc->dimensions;
    arrayDst->sizes = arraySrc->sizes;
    arrayDst->arrayType = arraySrc->arrayType;
}

/* Forward declarations */
static int32_t struct_member_get(CSOUND *csound, STRUCT_GET *p);

/* Built-in struct member get/set SUBRs (generic, any struct) */
static int32_t struct_member_get_init(CSOUND *csound, STRUCT_GET *p)
{
  return OK;
}

/* Combined init+perf function for i-rate opcodes */
static int32_t struct_member_get_init_and_perf(CSOUND *csound, STRUCT_GET *p)
{
  int32_t result = struct_member_get_init(csound, p);
  if (result != OK) return result;

  // For i-rate opcodes, execute the read immediately during init phase
  // This ensures the read happens at i-time, regardless of instrument duration
  return struct_member_get(csound, p);
}

static int32_t struct_member_get(CSOUND *csound, STRUCT_GET *p)
{
  if (UNLIKELY(p->nths[0] == NULL)) {
    return csound->PerfError(csound, &(p->h), "Invalid member index pointer (NULL)");
  }

  if (UNLIKELY(p->var == NULL)) {
    return csound->PerfError(csound, &(p->h), "Invalid struct pointer (NULL)");
  }

  CS_STRUCT_VAR* varIn = (CS_STRUCT_VAR*)p->var;

  if (UNLIKELY(varIn == NULL || varIn->members == NULL)) {
    return csound->PerfError(csound, &(p->h), "Invalid struct for member_get");
  }

  /* Safety: detect type confusion (ARRAYDAT being accessed as CS_STRUCT_VAR) */
  if (UNLIKELY(varIn->memberCount <= 0 || varIn->memberCount > 1000)) {
    ARRAYDAT* arrayDat = (ARRAYDAT*)varIn;
    if (arrayDat->arrayType && arrayDat->arrayType->userDefinedType) {
      return csound->PerfError(csound, &(p->h),
        "Type confusion: trying to access array of structs as single struct. "
        "Use array[index].member syntax instead of array.member");
    }
    return csound->PerfError(csound, &(p->h), "Corrupted struct: invalid memberCount=%d", varIn->memberCount);
  }

  MYFLT memberIndexFloat = *p->nths[0];
  int nthInt = (int)memberIndexFloat;

  if (UNLIKELY(nthInt < 0 || nthInt >= varIn->memberCount)) {
    return csound->PerfError(csound, &(p->h), "Invalid member index %d (memberCount=%d)", nthInt, varIn->memberCount);
  }

  CS_VAR_MEM* member = varIn->members[nthInt];









  /* Use type-aware copy so array members and non-scalars are handled */
  if (member->varType && member->varType->copyValue) {
    if (member->varType == &CS_VAR_TYPE_ARRAY) {
      ARRAYDAT *src = (ARRAYDAT*)&member->value;
      ARRAYDAT *dst = (ARRAYDAT*)p->out;

      if (src->arrayType && src->arrayType->userDefinedType) {
        // For arrays of structs: shallow alias
        dst->arrayType       = src->arrayType;
        dst->dimensions      = src->dimensions;
        dst->sizes           = src->sizes;
        dst->arrayMemberSize = src->arrayMemberSize;
        dst->data            = src->data;
        dst->allocated       = 0;
        /* Ensure dimension metadata exists for destination alias (Option A, UDT arrays) */
        if ((dst->sizes == NULL || dst->dimensions <= 0) && src && src->arrayMemberSize > 0) {
          // First try to copy dimensions from source if they exist
          if (src->dimensions > 0 && src->sizes != NULL) {
            // Copy dimensions from source
            dst->dimensions = src->dimensions;
            dst->sizes = src->sizes;  // Shallow copy is fine for aliases
          } else {
            // Fallback: try to infer 1D length from allocation
            int len = 0;
            if (src->allocated > 0) {
              len = (int)(src->allocated / (size_t)src->arrayMemberSize);
            }
            if (len > 0) {
              int32_t* sz = (int32_t*) csound->Calloc(csound, sizeof(int32_t) * 1);
              sz[0] = len;
              dst->dimensions = 1;
              dst->sizes = sz;
            }
          }
        }

      } else {
          /* Ensure dimension metadata exists for destination alias (Option A) */
          if ((dst->sizes == NULL || dst->dimensions <= 0) && src && src->data && src->arrayMemberSize > 0) {
            int dims = (src->dimensions > 0 ? src->dimensions : 1);
            if (dims == 1) {
              int len = 0;
              if (src->allocated > 0) {
                len = (int)(src->allocated / (size_t)src->arrayMemberSize);
              }
              if (len > 0) {
                int32_t* sz = (int32_t*) csound->Calloc(csound, sizeof(int32_t) * 1);
                sz[0] = len;
                dst->dimensions = 1;
                dst->sizes = sz;
              }
            }
          }

        // For arrays of primitives (e.g., S[], k[], i[]): perform deep copy
        member->varType->copyValue(csound, member->varType, (void*)dst, (void*)src, p->h.insdshead);
      }
    } else {
      member->varType->copyValue(csound, member->varType,
                                 (void*)p->out, (void*)&member->value,
                                 p->h.insdshead);
    }
  } else {
    *p->out = member->value;
  }
  return OK;
}


static int32_t struct_member_set_init(CSOUND *csound, STRUCT_SET *p)
{
  return OK;
}

// Forward declaration
static int32_t struct_member_set(CSOUND *csound, STRUCT_SET *p);

static int32_t struct_member_set_init_and_perf(CSOUND *csound, STRUCT_SET *p)
{
  int32_t result = struct_member_set_init(csound, p);
  if (result != OK) return result;

  // For i-rate opcodes, execute the assignment immediately during init phase
  // This ensures the assignment happens at i-time, regardless of instrument duration
  return struct_member_set(csound, p);
}

static int32_t struct_member_set(CSOUND *csound, STRUCT_SET *p)
{
  // Check if the member index pointer is NULL before dereferencing
  if (UNLIKELY(p->nths[0] == NULL)) {
    return csound->PerfError(csound, &(p->h), "Invalid member index pointer (NULL)");
  }

  // Check if p->var is NULL before casting
  if (UNLIKELY(p->var == NULL)) {
    return csound->PerfError(csound, &(p->h), "Invalid struct pointer (NULL)");
  }

  CS_STRUCT_VAR* var = (CS_STRUCT_VAR*)p->var;

  if (UNLIKELY(var == NULL || var->members == NULL))
    return csound->PerfError(csound, &(p->h), "Invalid struct for member_set");

  if (UNLIKELY(var->memberCount <= 0 || var->memberCount > 1000)) {
    return csound->PerfError(csound, &(p->h), "Corrupted struct: invalid memberCount=%d", var->memberCount);
  }

  // Read and validate the member index
  MYFLT memberIndexFloat = *p->nths[0];
  int nthInt = (int)memberIndexFloat;

  if (UNLIKELY(nthInt < 0 || nthInt >= var->memberCount)) {
    return csound->PerfError(csound, &(p->h), "Invalid member index %d (memberCount=%d)", nthInt, var->memberCount);
  }

  CS_VAR_MEM* member = var->members[nthInt];





  /* Type-aware assignment; fall back to scalar write */
  if (member->varType && member->varType->copyValue) {
    member->varType->copyValue(csound, member->varType,
                               (void*)&member->value, (void*)p->in,
                               p->h.insdshead);
  } else {
    member->value = *p->in;
  }

  return OK;
}


static int32_t struct_member_array_assign(
    CSOUND *csound, STRUCT_MEMBER_ARRAY_ASSIGN *p
) {
    // Check if the member index pointer is NULL before dereferencing
    if (UNLIKELY(p->nths[0] == NULL)) {
      return csound->PerfError(csound, &(p->h), "Invalid member index pointer (NULL)");
    }

    // Check if p->var is NULL before casting
    if (UNLIKELY(p->var == NULL)) {
      return csound->PerfError(csound, &(p->h), "Invalid struct pointer (NULL)");
    }

    int nthInt = (int) *p->nths[0];
    CS_STRUCT_VAR* var = (CS_STRUCT_VAR*)p->var;

    if (UNLIKELY(var == NULL || var->members == NULL))
      return csound->PerfError(csound, &(p->h), "Invalid struct for member_array_assign");

    if (UNLIKELY(nthInt < 0 || nthInt >= var->memberCount)) {
      return csound->PerfError(csound, &(p->h),
        "Member index %d out of bounds (memberCount=%d)", nthInt, var->memberCount);
    }

    CS_VAR_MEM* member = var->members[nthInt];

    if (UNLIKELY(member == NULL || member->varType != &CS_VAR_TYPE_ARRAY)) {
      return csound->PerfError(csound, &(p->h), "Selected member is not an array for member_array_assign");
    }

    ARRAYDAT* dst = (ARRAYDAT*) &member->value;
    ARRAYDAT* src = p->in;


    struct_array_member_assign(src, dst, member->varType);

    /* Option A: ensure dimension metadata is present for destination view.
       If sizes are missing but we can infer 1-D length from source allocation, populate it. */
    if ((dst->sizes == NULL || dst->dimensions <= 0) && src && src->arrayMemberSize > 0) {
      // First try to copy dimensions from source if they exist
      if (src->dimensions > 0 && src->sizes != NULL) {
        // Copy dimensions from source
        dst->dimensions = src->dimensions;
        dst->sizes = src->sizes;  // Shallow copy is fine for aliases
      } else {
        // Fallback: try to infer 1D length from allocation
        int len = 0;
        if (src->allocated > 0) {
          len = (int)(src->allocated / (size_t)src->arrayMemberSize);
        }
        if (len > 0) {
          int32_t* sz = (int32_t*) csound->Calloc(csound, sizeof(int32_t) * 1);
          sz[0] = len;
          dst->dimensions = 1;
          dst->sizes = sz;
        }
      }
    }

    return OK;


}

static int32_t struct_alias(CSOUND *csound, STRUCT_ALIAS *p)
{
  CS_STRUCT_VAR* dst = p->dst;
  CS_STRUCT_VAR* src = p->src;
  if (UNLIKELY(dst == NULL || src == NULL))
    return csound->PerfError(csound, &(p->h), "Invalid struct for struct_alias");

  /* Capture existing destination members for deferred free at deinit */
  p->oldMembers = NULL;
  p->oldMemberCount = 0;
  p->oldOwned = 0;

  if (dst->ownsMembers && dst->members && dst->memberCount > 0) {
    p->oldMembers = dst->members;
    p->oldMemberCount = dst->memberCount;
    p->oldOwned = 1;
  }

  /* BIDIRECTIONAL ALIASING: Both structs should point to the same shared memory
     Choose src->members as the shared memory (since src is the source of the assignment) */
  CS_VAR_MEM** sharedMembers = src->members;
  int sharedMemberCount = src->memberCount;

  /* Make dst point to shared memory */
  dst->members     = sharedMembers;
  dst->memberCount = sharedMemberCount;
  dst->ownsMembers = 0;

  /* CRITICAL: Also make src point to the same shared memory (bidirectional) */
  src->members     = sharedMembers;
  src->memberCount = sharedMemberCount;
  src->ownsMembers = 0;



  /* Safety: if destination was already aliased to the same source, skip deferred free */
  if (p->oldMembers == src->members) {
    p->oldMembers = NULL;
    p->oldMemberCount = 0;
    p->oldOwned = 0;
  }

  return OK;
}


static int32_t struct_array_get(CSOUND *csound, STRUCT_ARRAY_GET* dat)
{
  ARRAYDAT* arrayDat = dat->arrayDat;

  if (UNLIKELY(arrayDat == NULL)) {
    return csound->PerfError(csound, &(dat->h), "struct_array_get: array is NULL");
  }

  if (arrayDat->dimensions <= 0) {
    csound->Warning(csound, "struct_array_get: input array has no dimensions\n");
    return OK;
  }

  // If array data is NULL, try to initialize it
  if (arrayDat->data == NULL && arrayDat->arrayType != NULL && arrayDat->arrayType->userDefinedType) {
    // Calculate total size from dimensions
    int32_t totalSize = 1;
    for (int32_t i = 0; i < arrayDat->dimensions; i++) {
      totalSize *= arrayDat->sizes[i];
    }

    CS_VARIABLE* var = arrayDat->arrayType->createVariable(csound, (void*)arrayDat->arrayType, dat->h.insdshead);
    arrayDat->arrayMemberSize = var->memBlockSize;
    arrayDat->data = csound->Calloc(csound, arrayDat->arrayMemberSize * totalSize);
    arrayDat->allocated = arrayDat->arrayMemberSize * totalSize;

    // Initialize each struct element
    char *mem = (char *) arrayDat->data;
    for (int32_t i = 0; i < totalSize; i++) {
      if (var->initializeVariableMemory != NULL) {
        var->initializeVariableMemory(csound, var, (MYFLT*)(mem + i * var->memBlockSize));
      }
    }
  }

  if (UNLIKELY(arrayDat->data == NULL)) {
    csound->Warning(csound, "struct_array_get: array data is still NULL after initialization attempt");
    return OK;
  }

  int index = (int)(*dat->indicies[0]);

  /* CRITICAL FIX: Add bounds checking */
  if (arrayDat->sizes == NULL || arrayDat->dimensions <= 0) {
    return csound->PerfError(csound, &(dat->h),
        "Struct array has invalid dimensions or sizes");
  }

  if (index < 0 || index >= arrayDat->sizes[0]) {
    return csound->PerfError(csound, &(dat->h),
        "Struct array index %d out of bounds (0-%d)", index, arrayDat->sizes[0]-1);
  }

  char* mem = (char *) arrayDat->data;

  if (UNLIKELY(mem == NULL)) {
    return csound->PerfError(csound, &(dat->h),
        "Struct array data is NULL");
  }

  // CRITICAL: arrayMemberSize is in BYTES for struct arrays, not MYFLT units
  // Verify the computed address is correct
  size_t elemSize = (size_t)arrayDat->arrayMemberSize;
  size_t offset = (size_t)index * elemSize;

  /* Check if offset is within allocated memory
     Note: alias views set allocated=0; in that case compute effective capacity
     from sizes[0] * elemSize. */
  size_t allocatedBytes = (size_t)arrayDat->allocated;
  if (allocatedBytes == 0 && arrayDat->sizes && arrayDat->dimensions > 0) {
    allocatedBytes = (size_t)arrayDat->sizes[0] * elemSize;
  }
  if (offset + elemSize > allocatedBytes) {
    return csound->PerfError(csound, &(dat->h),
        "Struct array access would exceed allocated memory");
  }

  CS_STRUCT_VAR* srcVar = (CS_STRUCT_VAR*)(mem + offset);
  CS_STRUCT_VAR* dstVar = (CS_STRUCT_VAR*) dat->out;

  if (csound->GetDebug(csound)) {
    csound->Message(csound,
      "ARRAY_GET_STRUCT: srcVar=%p members=%p dstVar(out)=%p\n",
      (void*)srcVar, (void*)(srcVar ? srcVar->members : NULL), (void*)dstVar);
  }

  /* Add safety checks before accessing srcVar */
  if (UNLIKELY(srcVar == NULL)) {

    return csound->PerfError(csound, &(dat->h),
        "Struct array element at index %d is NULL", index);
  }

  if (UNLIKELY(dstVar == NULL)) {
    return csound->PerfError(csound, &(dat->h),
        "Destination struct variable is NULL");
  }

  /* Ensure srcVar is initialized */
  if (srcVar->members == NULL) {

    if (arrayDat->arrayType && arrayDat->arrayType->createVariable) {
      CS_VARIABLE* helper = arrayDat->arrayType->createVariable(csound, (void*)arrayDat->arrayType, dat->h.insdshead);
      if (helper && helper->initializeVariableMemory) {
        helper->initializeVariableMemory(csound, helper, (MYFLT*)srcVar);
      }
    }
    if (srcVar->members == NULL) {
      return csound->PerfError(csound, &(dat->h),
          "Struct array element at index %d is not properly initialized", index);
    }
  }

  dstVar = (CS_STRUCT_VAR*)dat->out;
  /* Make dstVar an alias to srcVar by copying the members pointer and metadata */
  dstVar->members = srcVar->members;
  dstVar->memberCount = srcVar->memberCount;
  dstVar->ownsMembers = 0;  /* Non-owning to avoid double free */
  return OK;
}


static int32_t struct_alias_deinit(CSOUND *csound, STRUCT_ALIAS *p)
{
  if (p->oldOwned && p->oldMembers) {
    CS_STRUCT_VAR tmp;
    tmp.members     = p->oldMembers;
    tmp.memberCount = p->oldMemberCount;
    tmp.ownsMembers = 1;
    /* Deep-free the previously owned member storage now that i-time is over */
    csound_free_struct_members(csound, &tmp);
    p->oldMembers = NULL;
    p->oldMemberCount = 0;
    p->oldOwned = 0;
  }
  return OK;
}

/* Generic struct initialization function */
static int32_t struct_init(CSOUND *csound, STRUCT_INIT *p)
{
  CS_STRUCT_VAR* structVar = p->out;

  if (UNLIKELY(structVar == NULL)) {
    return csound->PerfError(csound, &(p->h), "Invalid struct variable for initialization");
  }

  // The struct variable should already be created by the type system
  // We just need to initialize its members with the provided values

  if (UNLIKELY(structVar->members == NULL || structVar->memberCount == 0)) {
    return csound->PerfError(csound, &(p->h), "Struct has no members to initialize");
  }

  // Get the number of input arguments (excluding the output)
  int32_t argCount = 0;
  while (argCount < VARGMAX && p->args[argCount] != NULL) {
    argCount++;
  }

  if (UNLIKELY(argCount != structVar->memberCount)) {
    return csound->PerfError(csound, &(p->h),
                            "Struct initialization: expected %d arguments, got %d",
                            structVar->memberCount, argCount);
  }

  // Initialize each member with the corresponding argument
  for (int32_t i = 0; i < structVar->memberCount; i++) {
    CS_VAR_MEM* member = structVar->members[i];
    if (UNLIKELY(member == NULL)) {
      return csound->PerfError(csound, &(p->h), "Struct member %d is NULL", i);
    }

    // For now, assume all members are scalars (MYFLT)
    // In a full implementation, we'd need to handle different member types
    member->value = *p->args[i];
  }

  return OK;
}


static OENTRY structops_localops[] = {
  { "##array_get_struct", sizeof(STRUCT_ARRAY_GET), 0, ".", ".[]m", (SUBR)struct_array_get, NULL, NULL },

  { "##member_get", sizeof(STRUCT_GET), 0, ".", ".i", (SUBR)struct_member_get_init_and_perf, (SUBR)struct_member_get, NULL },
  { "##member_get.i", sizeof(STRUCT_GET), 0, "i", ".i", (SUBR)struct_member_get_init_and_perf, (SUBR)struct_member_get, NULL },
  { "##member_get.k", sizeof(STRUCT_GET), 0, "k", ".i", (SUBR)struct_member_get_init, (SUBR)struct_member_get, NULL },
  { "##member_get.S", sizeof(STRUCT_GET), 0, "S", ".i", (SUBR)struct_member_get_init, (SUBR)struct_member_get, NULL },
  { "##member_get.a", sizeof(STRUCT_GET), 0, "a", ".i", (SUBR)struct_member_get_init, (SUBR)struct_member_get, NULL },
  { "##member_get.b", sizeof(STRUCT_GET), 0, "b", ".i", (SUBR)struct_member_get_init, (SUBR)struct_member_get, NULL },
  { "##member_set", sizeof(STRUCT_SET), 0, "", ".i.", (SUBR)struct_member_set_init_and_perf, (SUBR)struct_member_set, NULL },
  { "##member_array_assign", sizeof(STRUCT_MEMBER_ARRAY_ASSIGN),
    0, "", ".i.[]", (SUBR)struct_member_array_assign, NULL, NULL },
  { "##struct_alias", sizeof(STRUCT_ALIAS), 0, "", "..", (SUBR)struct_alias, NULL, (SUBR)struct_alias_deinit },

  // Generic struct initialization opcodes - these will be registered dynamically for each struct type
  // For now, add some common patterns to test
  { "init", sizeof(STRUCT_INIT), 0, "", "m", (SUBR)struct_init, NULL, NULL },
  { "init.i", sizeof(STRUCT_INIT), 0, "", "m", (SUBR)struct_init, NULL, NULL },
};


LINKAGE_BUILTIN(structops_localops)
