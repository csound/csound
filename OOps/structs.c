/*
    structs.c:

    Copyright (C) 2025
    Hlöðver Sigurðsson

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

#include <stddef.h>
#include <stdint.h>
#include "array_ops.h"
#include "csoundCore.h"
#include "csound_standard_types.h"
#include "csound_orc_structs.h"
#include "struct_ops.h"

int32_t array_set_struct(CSOUND* csound, ARRAY_SET *p)
{
  ARRAYDAT* dat = p->arrayDat;
  if (UNLIKELY(dat == NULL || dat->data == NULL)) {
    return csound->PerfError(csound, &(p->h), Str("array_set_struct: NULL array"));
  }
  int32_t indefArgCount = p->INOCOUNT - 2; // value + at least one index
  if (UNLIKELY(indefArgCount <= 0))
    return csound->PerfError(csound, &(p->h), Str("array_set_struct: no indexes"));
  if (UNLIKELY(dat->dimensions > 0 && indefArgCount != dat->dimensions)) {
    // Allow flat arrays with no metadata
    if (!(dat && dat->dimensions == 0)) {
      return csound->PerfError(csound, &(p->h), Str("array_set_struct: dimension mismatch"));
    }
  }
  // Compute flat index
  size_t index = 0;
  for (int32_t i = 0; i < indefArgCount; i++) {
    int32_t end = (int32_t)(*p->indexes[i]);

    // Enforce non-negative index check for all cases
    if (UNLIKELY(end < 0)) {
      return csound->PerfError(csound, &(p->h), Str("array_set_struct: index %d out of range (negative)"), i+1);
    }

    if (dat->dimensions > 0 && dat->sizes != NULL) {
      size_t dim = (size_t)dat->sizes[i];
      if (UNLIKELY((size_t)end >= dim)) {
        return csound->PerfError(csound, &(p->h), Str("array_set_struct: index %d out of range"), i+1);
      }
      if (UNLIKELY(index > (SIZE_MAX - (size_t)end) / dim)) {
        return csound->PerfError(csound, &(p->h), Str("array_set_struct: index overflow"));
      }
      index = (index * dim) + (size_t)end;
    } else {
      // For flat arrays (dat->dimensions == 0), we need to validate bounds differently
      // We'll check the final computed index against the array size after the loop
      index = (size_t)end; // flat arrays: last index wins
    }
  }

  // Validate capacity if known
  if (UNLIKELY(dat->arrayMemberSize <= 0)) {
    return csound->PerfError(csound, &(p->h), Str("array_set_struct: invalid element size"));
  }

  // Address element in bytes (safe for struct payloads)
  char* base = (char*)dat->data;

  // Validate byte offset is within allocated buffer
  size_t elemSize = (size_t)dat->arrayMemberSize;
  if (UNLIKELY(index > (SIZE_MAX - (elemSize - 1)) / elemSize)) {
    return csound->PerfError(csound, &(p->h), Str("array_set_struct: offset overflow"));
  }
  size_t allocatedBytes = (size_t)dat->allocated;
  if (allocatedBytes == 0 && dat->sizes && dat->dimensions > 0) {
    allocatedBytes = (size_t)dat->sizes[0] * elemSize;
  }
  size_t offset = (size_t)index * elemSize;
  if (allocatedBytes > 0 && UNLIKELY(offset + elemSize > allocatedBytes)) {
    return csound->PerfError(csound, &(p->h), Str("array_set_struct: computed offset %zu out of bounds"), offset);
  }

  char* dstPtr = base + offset;

  // Ensure destination struct is initialized (members allocated)
  if (dat->arrayType && dat->arrayType->userDefinedType) {
    CS_STRUCT_VAR* dst = (CS_STRUCT_VAR*)(void*)dstPtr;
    if (dst && dst->members == NULL) {
      CS_VARIABLE* var = dat->arrayType->createVariable(csound, (void*)dat->arrayType, p->h.insdshead);
      if (var && var->initializeVariableMemory) {
        var->initializeVariableMemory(csound, var, (MYFLT*)dst);
      }
      if (var) {
        csound->Free(csound, var);
      }
    }
  }

  // Copy value into destination using type-aware copier
  // For struct arrays, we need to copy member by member since the array elements
  // are raw struct data, not CS_STRUCT_VAR structures with members pointers
  if (dat->arrayType && dat->arrayType->userDefinedType) {
    CS_STRUCT_VAR* srcVar = (CS_STRUCT_VAR*)p->value;
    CS_STRUCT_VAR* dstVar = (CS_STRUCT_VAR*)dstPtr;

    // Validate that dstVar is within bounds before accessing
    if (UNLIKELY(dstVar == NULL)) {
      return csound->PerfError(csound, &(p->h), Str("array_set_struct: destination pointer is NULL"));
    }

    // Ensure both are initialized
    if (srcVar && srcVar->members && dstVar && dstVar->members && dat->arrayType->members) {
      // Get member count from the type definition, not the variable
      int32_t memberCount = cs_cons_length(dat->arrayType->members);
      for (int32_t i = 0; i < memberCount; i++) {
        CS_VAR_MEM* srcMember = srcVar->members[i];
        CS_VAR_MEM* dstMember = dstVar->members[i];
        if (srcMember && dstMember && srcMember->varType) {
          // Copy the member value
          if (srcMember->varType->copyValue) {
            srcMember->varType->copyValue(csound, srcMember->varType,
                                         &dstMember->value, &srcMember->value, p->h.insdshead);
          } else {
            dstMember->value = srcMember->value;
          }
        }
      }
    }
  } else if (dat->arrayType && dat->arrayType->copyValue) {
    dat->arrayType->copyValue(csound, dat->arrayType, (void*)dstPtr, p->value, p->h.insdshead);
  } else {
    // Fallback should not happen for struct, but keep parity with array_set
    if (LIKELY(dstPtr != NULL && p->value != NULL)) *((MYFLT*)dstPtr) = *((MYFLT*)p->value);
  }
  return OK;
}

void struct_array_member_assign(
    ARRAYDAT* arraySrc,
    ARRAYDAT* arrayDst
) {
    /* Shallow alias: destination does not own storage */
    arrayDst->allocated = 0;
    arrayDst->arrayMemberSize = arraySrc->arrayMemberSize;
    arrayDst->data = arraySrc->data;
    arrayDst->dimensions = arraySrc->dimensions;
    arrayDst->sizes = arraySrc->sizes;
    arrayDst->arrayType = arraySrc->arrayType;
}

// /* Built-in struct member get/set SUBRs (generic, any struct) */
int32_t struct_member_get_init(CSOUND *csound, STRUCT_GET *p)
{
  return OK;
}

int32_t struct_member_get_init_and_perf(CSOUND *csound, STRUCT_GET *p)
{
  int32_t result = struct_member_get_init(csound, p);
  if (result != OK) return result;

  // For i-rate opcodes, execute the read immediately during init phase
  // This ensures the read happens at i-time, regardless of instrument duration
  return struct_member_get(csound, p);
}

int32_t struct_member_get(CSOUND *csound, STRUCT_GET *p)
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
          // Copy dimensions from source if they exist
          // For aliases (allocated=0), only use shallow copy - never allocate new metadata
          if (src->dimensions > 0 && src->sizes != NULL) {
            dst->dimensions = src->dimensions;
            dst->sizes = src->sizes;
          }
        }

      } else {
          /* Ensure dimension metadata exists for destination alias (Option A) */
          if ((dst->sizes == NULL || dst->dimensions <= 0) && src && src->data && src->arrayMemberSize > 0) {
            // For aliases (allocated=0), only use shallow copy - never allocate new metadata
            if (src->dimensions > 0 && src->sizes != NULL) {
              dst->dimensions = src->dimensions;
              dst->sizes = src->sizes;
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


int32_t struct_member_set_init(CSOUND *csound, STRUCT_SET *p)
{
  return OK;
}

int32_t struct_member_set_init_and_perf(CSOUND *csound, STRUCT_SET *p)
{
  int32_t result = struct_member_set_init(csound, p);
  if (result != OK) return result;

  // For i-rate opcodes, execute the assignment immediately during init phase
  // This ensures the assignment happens at i-time, regardless of instrument duration
  return struct_member_set(csound, p);
}

int32_t struct_member_set(CSOUND *csound, STRUCT_SET *p)
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

  // Check if this is actually an ARRAYDAT (array of structs) instead of a struct
  // This happens when array[index].member syntax is used incorrectly
  ARRAYDAT* arrayCheck = (ARRAYDAT*)p->var;
  if (arrayCheck && arrayCheck->dimensions >= 0 && arrayCheck->dimensions < 100 &&
      arrayCheck->arrayType && arrayCheck->arrayType->userDefinedType &&
      arrayCheck->data != NULL) {
    return csound->PerfError(csound, &(p->h),
      "Cannot directly access array[index].member for struct arrays. "
      "Use: temp = array[index]; temp.member = value; array[index] = temp");
  }

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


int32_t struct_member_array_assign(
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


    struct_array_member_assign(src, dst);

    /* Option A: ensure dimension metadata is present for destination view.
       For aliases, only use shallow copy - never allocate new metadata. */
    if ((dst->sizes == NULL || dst->dimensions <= 0) && src && src->arrayMemberSize > 0) {
      // Copy dimensions from source if they exist
      if (src->dimensions > 0 && src->sizes != NULL) {
        dst->dimensions = src->dimensions;
        dst->sizes = src->sizes;
      }
    }

    return OK;


}

int32_t struct_alias(CSOUND *csound, STRUCT_ALIAS *p)
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

  /* Safety: if destination was already aliased to the same source, skip deferred free */
  if (p->oldMembers == src->members) {
    p->oldMembers = NULL;
    p->oldMemberCount = 0;
    p->oldOwned = 0;
  }

  return OK;
}


int32_t struct_array_get(CSOUND *csound, STRUCT_ARRAY_GET* dat)
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
    csound->Free(csound, var);
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

  // Enforce non-negative index check
  if (index < 0) {
    return csound->PerfError(csound, &(dat->h),
        "Struct array index %d is negative", index);
  }

  if (index >= arrayDat->sizes[0]) {
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
      if (helper) {
        csound->Free(csound, helper);
      }
    }
    if (srcVar->members == NULL) {
      return csound->PerfError(csound, &(dat->h),
          "Struct array element at index %d is not properly initialized", index);
    }
  }

  dstVar = (CS_STRUCT_VAR*)dat->out;

  /* For struct arrays, we copy values also when they are references, because the array
     element's member storage is separate from the output struct's storage */
  if (dstVar->members && srcVar->members && dstVar->memberCount == srcVar->memberCount) {
    for (int i = 0; i < srcVar->memberCount; i++) {
      CS_VAR_MEM *d = dstVar->members[i], *s = srcVar->members[i];
      if (!d || !s) continue;
      if (d->varType && d->varType->copyValue) {
        d->varType->copyValue(csound, d->varType, &d->value, &s->value, dat->h.insdshead);
      } else {
        d->value = s->value;
      }
    }
    /* keep existing ownership as-is after deep copy */
  } else {
    /* free previously owned members before aliasing to avoid leaks */
    if (dstVar->members && dstVar->ownsMembers) {
      csound_free_struct_members(csound, dstVar);
      dstVar->members = NULL;
      dstVar->memberCount = 0;
      dstVar->ownsMembers = 0;
    }
    dstVar->members = srcVar->members;
    dstVar->memberCount = srcVar->memberCount;
    dstVar->ownsMembers = 0;
  }
  return OK;
}


int32_t struct_alias_deinit(CSOUND *csound, STRUCT_ALIAS *p)
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
int32_t struct_init(CSOUND *csound, STRUCT_INIT *p)
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
