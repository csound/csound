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
#include <math.h>
#include "array_ops.h"
#include "arrays_internal.h"
#include "csoundCore.h"
#include "csound_standard_types.h"
#include "csound_orc_structs.h"
#include "struct_ops.h"

static int32_t nonnegative_index_from_myflt(MYFLT value, int32_t *result)
{
  double widened = (double)value;

  /* Widen before comparing so a float MYFLT cannot round INT32_MAX upward. */
  if (UNLIKELY(result == NULL || isnan(widened) || widened < 0.0 ||
               widened > (double)INT32_MAX)) {
    return NOTOK;
  }
  /* Fractional indexes retain the established truncation behavior. */
  *result = (int32_t)widened;
  return OK;
}

static int32_t struct_value_matches_type(const CS_TYPE *type,
                                         const CS_STRUCT_VAR *value)
{
  CONS_CELL *expectedMember;

  if (type == NULL || type->members == NULL || value == NULL ||
      value->members == NULL) {
    return 0;
  }
  expectedMember = type->members;
  for (int32_t i = 0; i < value->memberCount; i++) {
    if (expectedMember == NULL) {
      return 0;
    }
    CS_VARIABLE *expected = (CS_VARIABLE *)expectedMember->value;
    CS_VAR_MEM *actual = value->members[i];

    if (expected == NULL || expected->varType == NULL || actual == NULL ||
        actual->varType == NULL ||
        expected->varType != actual->varType) {
      return 0;
    }
    if (expected->varType == &CS_VAR_TYPE_ARRAY &&
        expected->subType != NULL &&
        ((ARRAYDAT *)&actual->value)->arrayType != expected->subType) {
      return 0;
    }
    expectedMember = expectedMember->next;
  }
  return expectedMember == NULL;
}

static int32_t struct_array_flat_index(CSOUND *csound, OPDS *opds,
                                       const char *opcodeName,
                                       const ARRAYDAT *array,
                                       MYFLT *const *indexes,
                                       int32_t indexCount,
                                       size_t *result)
{
  size_t flatIndex = 0;

  if (UNLIKELY(result == NULL || opcodeName == NULL || array == NULL ||
               indexes == NULL || array->dimensions <= 0 ||
               array->sizes == NULL || indexCount != array->dimensions)) {
    return csound->PerfError(csound, opds,
                             "%s: array dimensions do not match indexes",
                             opcodeName);
  }
  for (int32_t i = 0; i < indexCount; i++) {
    int32_t coordinate;
    size_t dimension;
    MYFLT rawIndex;

    if (UNLIKELY(indexes[i] == NULL || array->sizes[i] <= 0)) {
      return csound->PerfError(csound, opds,
                               "%s: invalid index or dimension %d",
                               opcodeName, i + 1);
    }
    rawIndex = *indexes[i];
    if (UNLIKELY(nonnegative_index_from_myflt(rawIndex, &coordinate) != OK)) {
      return csound->PerfError(csound, opds,
                               "%s: invalid index for dimension %d",
                               opcodeName, i + 1);
    }
    dimension = (size_t)array->sizes[i];
    if (UNLIKELY(coordinate < 0 || (size_t)coordinate >= dimension)) {
      return csound->PerfError(csound, opds,
                               "%s: index %d out of range for dimension %d",
                               opcodeName, coordinate, i + 1);
    }
    if (UNLIKELY(flatIndex >
                 (SIZE_MAX - (size_t)coordinate) / dimension)) {
      return csound->PerfError(csound, opds, "%s: index overflow",
                               opcodeName);
    }
    flatIndex = flatIndex * dimension + (size_t)coordinate;
  }
  *result = flatIndex;
  return OK;
}

static int32_t struct_array_element(CSOUND *csound, OPDS *opds,
                                    const char *opcodeName,
                                    const ARRAYDAT *array, size_t index,
                                    void **result)
{
  size_t elementSize;
  size_t offset;
  size_t allocated;

  if (UNLIKELY(result == NULL || opcodeName == NULL || array == NULL ||
               array->data == NULL ||
               array->arrayMemberSize <= 0)) {
    return csound->PerfError(csound, opds, "%s: invalid array storage",
                             opcodeName);
  }
  if (UNLIKELY(!csound_array_storage_matches(csound, array))) {
    return csound->PerfError(csound, opds,
                             "%s: inconsistent shared array storage",
                             opcodeName);
  }
  elementSize = (size_t)array->arrayMemberSize;
  if (UNLIKELY(index > (SIZE_MAX - elementSize) / elementSize)) {
    return csound->PerfError(csound, opds, "%s: offset overflow",
                             opcodeName);
  }
  offset = index * elementSize;
  allocated = csound_array_allocated_bytes(csound, array);
  if (UNLIKELY(allocated < elementSize ||
               offset > allocated - elementSize)) {
    return csound->PerfError(csound, opds,
                             "%s: element exceeds allocated storage",
                             opcodeName);
  }
  *result = (char *)array->data + offset;
  return OK;
}

int32_t array_set_struct_init(CSOUND *csound, ARRAY_SET *p)
{
  ARRAYDAT *dat = p->arrayDat;

  if (UNLIKELY(dat == NULL || dat->arrayType == NULL ||
               !dat->arrayType->userDefinedType)) {
    return csound->InitError(csound, "%s",
                             Str("array_set_struct: invalid destination"));
  }
  /* A k-rate element write cannot allocate a detached array backing store.
     Detach during this opcode's init callback so its performance callback
     only mutates owned storage. */
  if (UNLIKELY(dat->storage != NULL &&
               csound_array_prepare_write(csound, dat,
                                          p->h.insdshead) != OK)) {
    return csound->InitError(
      csound, "%s",
      Str("array_set_struct: could not prepare writable array"));
  }
  return OK;
}

int32_t array_set_struct(CSOUND *csound, ARRAY_SET *p)
{
  ARRAYDAT* dat = p->arrayDat;
  CS_STRUCT_VAR *source;
  CS_STRUCT_VAR *destination;
  size_t index;
  void *element;
  int32_t indexCount = p->INOCOUNT - 2;

  if (UNLIKELY(dat == NULL || p->value == NULL)) {
    return csound->PerfError(csound, &p->h,
                             "array_set_struct: NULL array or value");
  }
  if (UNLIKELY(dat->arrayType == NULL ||
               !dat->arrayType->userDefinedType ||
               dat->arrayType->copyValue == NULL)) {
    return csound->PerfError(csound, &p->h,
                             "array_set_struct: invalid element type");
  }
  if (UNLIKELY(dat->storage != NULL &&
               csound_array_prepare_write_for_mode(
                 csound, dat, p->h.insdshead) != OK)) {
    return csound->PerfError(
      csound, &p->h,
      "array_set_struct: could not detach shared array");
  }
  if (UNLIKELY(struct_array_flat_index(csound, &p->h, "array_set_struct",
                                       dat, p->indexes, indexCount,
                                       &index) != OK)) {
    return NOTOK;
  }
  if (UNLIKELY(struct_array_element(csound, &p->h, "array_set_struct",
                                    dat, index, &element) != OK)) {
    return NOTOK;
  }

  source = (CS_STRUCT_VAR *)p->value;
  destination = (CS_STRUCT_VAR *)element;
  if (UNLIKELY(!struct_value_matches_type(dat->arrayType, source))) {
    return csound->PerfError(csound, &p->h,
                             "array_set_struct: value does not match type");
  }
  if (destination->members == NULL) {
    CS_VARIABLE *var = array_element_create_variable(
      csound, dat->arrayType, p->h.insdshead);
    if (UNLIKELY(var == NULL || var->initializeVariableMemory == NULL)) {
      if (var != NULL) {
        csound->Free(csound, var);
      }
      return csound->PerfError(csound, &p->h,
                               "array_set_struct: cannot initialize element");
    }
    var->initializeVariableMemory(csound, var, element);
    csound->Free(csound, var);
  }
  if (UNLIKELY(!struct_value_matches_type(dat->arrayType, destination))) {
    return csound->PerfError(csound, &p->h,
                             "array_set_struct: destination does not match type");
  }

  /* The registered copier carries nested-array ownership through ordinary
     struct assignment and through this array-specific lowering path. */
  dat->arrayType->copyValue(csound, dat->arrayType,
                            destination, source, p->h.insdshead);
  return OK;
}

/* Built-in struct member get/set SUBRs (generic, any struct). */
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
    return csound->PerfError(csound, &p->h,
                             "Invalid member index pointer (NULL)");
  }
  if (UNLIKELY(p->out == NULL)) {
    return csound->PerfError(csound, &p->h,
                             "Invalid struct member output (NULL)");
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

  int32_t nthInt;
  if (UNLIKELY(nonnegative_index_from_myflt(*p->nths[0], &nthInt) != OK)) {
    return csound->PerfError(csound, &p->h, "Invalid member index");
  }

  if (UNLIKELY(nthInt >= varIn->memberCount)) {
    return csound->PerfError(csound, &(p->h), "Invalid member index %d (memberCount=%d)", nthInt, varIn->memberCount);
  }

  CS_VAR_MEM* member = varIn->members[nthInt];
  if (UNLIKELY(member == NULL)) {
    return csound->PerfError(csound, &p->h,
                             "Struct member %d is not initialized", nthInt);
  }

  /* Use type-aware copy so array members and non-scalars are handled */
  if (member->varType && member->varType->copyValue) {
    member->varType->copyValue(csound, member->varType,
                               (void*)p->out, (void*)&member->value,
                               p->h.insdshead);
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
    return csound->PerfError(csound, &p->h,
                             "Invalid struct pointer (NULL)");
  }
  if (UNLIKELY(p->in == NULL)) {
    return csound->PerfError(csound, &p->h,
                             "Invalid struct member input (NULL)");
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

  int32_t nthInt;
  if (UNLIKELY(nonnegative_index_from_myflt(*p->nths[0], &nthInt) != OK)) {
    return csound->PerfError(csound, &p->h, "Invalid member index");
  }

  if (UNLIKELY(nthInt >= var->memberCount)) {
    return csound->PerfError(csound, &(p->h), "Invalid member index %d (memberCount=%d)", nthInt, var->memberCount);
  }

  CS_VAR_MEM* member = var->members[nthInt];
  if (UNLIKELY(member == NULL)) {
    return csound->PerfError(csound, &p->h,
                             "Struct member %d is not initialized", nthInt);
  }

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
      return csound->PerfError(csound, &p->h,
                               "Invalid struct pointer (NULL)");
    }
    if (UNLIKELY(p->in == NULL)) {
      return csound->PerfError(csound, &p->h,
                               "Invalid array member input (NULL)");
    }

    CS_STRUCT_VAR* var = (CS_STRUCT_VAR*)p->var;
    int32_t nthInt;

    if (UNLIKELY(var == NULL || var->members == NULL))
      return csound->PerfError(csound, &(p->h), "Invalid struct for member_array_assign");

    if (UNLIKELY(nonnegative_index_from_myflt(*p->nths[0], &nthInt) != OK)) {
      return csound->PerfError(csound, &p->h, "Invalid member index");
    }
    if (UNLIKELY(nthInt >= var->memberCount)) {
      return csound->PerfError(csound, &(p->h),
        "Member index %d out of bounds (memberCount=%d)", nthInt, var->memberCount);
    }

    CS_VAR_MEM* member = var->members[nthInt];

    if (UNLIKELY(member == NULL || member->varType != &CS_VAR_TYPE_ARRAY)) {
      return csound->PerfError(csound, &(p->h), "Selected member is not an array for member_array_assign");
    }

    ARRAYDAT* dst = (ARRAYDAT*) &member->value;
    CS_VAR_TYPE_ARRAY.copyValue(csound, &CS_VAR_TYPE_ARRAY,
                                dst, p->in, p->h.insdshead);

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
  CS_STRUCT_VAR *source;
  CS_STRUCT_VAR *destination = (CS_STRUCT_VAR *)dat->out;
  size_t index;
  size_t totalSize;
  void *element;
  int32_t indexCount = dat->INOCOUNT - 1;

  if (UNLIKELY(arrayDat == NULL || destination == NULL)) {
    return csound->PerfError(csound, &dat->h,
                             "struct_array_get: NULL array or output");
  }
  if (UNLIKELY(arrayDat->arrayType == NULL ||
               !arrayDat->arrayType->userDefinedType ||
               arrayDat->arrayType->copyValue == NULL)) {
    return csound->PerfError(csound, &dat->h,
                             "struct_array_get: invalid element type");
  }

  /* A declared array can have complete dimensions before its backing store is
     created. Use the same initializer as other array paths before reading. */
  if (arrayDat->data == NULL) {
    if (UNLIKELY(csound_array_member_count(arrayDat, &totalSize) != OK ||
                 totalSize > INT32_MAX)) {
      return csound->PerfError(csound, &dat->h,
                               "Invalid struct array dimensions");
    }
    tabinit(csound, arrayDat, (int32_t)totalSize, dat->h.insdshead);
  }
  if (UNLIKELY(struct_array_flat_index(csound, &dat->h,
                                       "struct_array_get", arrayDat,
                                       dat->indicies, indexCount,
                                       &index) != OK)) {
    return NOTOK;
  }
  if (UNLIKELY(struct_array_element(csound, &dat->h, "struct_array_get",
                                    arrayDat, index, &element) != OK)) {
    return NOTOK;
  }

  source = (CS_STRUCT_VAR *)element;
  if (UNLIKELY(!struct_value_matches_type(arrayDat->arrayType, source))) {
    return csound->PerfError(csound, &dat->h,
                             "struct_array_get: element does not match type");
  }

  if (destination->members == NULL || !destination->ownsMembers ||
      destination->memberCount != source->memberCount) {
    CS_VARIABLE* helper;

    /* A read result must own its members. Copying into an alias would modify
       the struct that the previous result referenced. */
    if (destination->ownsMembers) {
      csound_free_struct_members(csound, destination);
    }
    else {
      destination->members = NULL;
      destination->memberCount = 0;
      destination->ownsMembers = 0;
    }
    helper = array_element_create_variable(csound, arrayDat->arrayType,
                                           dat->h.insdshead);
    if (UNLIKELY(helper == NULL ||
                 helper->initializeVariableMemory == NULL)) {
      if (helper != NULL) {
        csound->Free(csound, helper);
      }
      return csound->PerfError(csound, &dat->h,
                               "Could not initialize struct array output");
    }
    helper->initializeVariableMemory(csound, helper, (MYFLT *)destination);
    csound->Free(csound, helper);
  }
  if (UNLIKELY(!struct_value_matches_type(arrayDat->arrayType,
                                          destination))) {
    return csound->PerfError(csound, &dat->h,
                             "struct_array_get: output does not match type");
  }

  arrayDat->arrayType->copyValue(csound, arrayDat->arrayType,
                                 destination, source, dat->h.insdshead);
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

    if (member->varType && member->varType->copyValue) {
      member->varType->copyValue(csound, member->varType,
                                 (void*)&member->value, (void*)p->args[i],
                                 p->h.insdshead);
    } else {
      member->value = *p->args[i];
    }
  }

  return OK;
}
