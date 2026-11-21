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

static int32_t nonnegative_index_from_myflt(cs_float value, int32_t *result)
{
  cs_double widened = (cs_double)value;

  /* Widen before comparing so a float cs_float cannot round INT32_MAX upward. */
  if (UNLIKELY(result == NULL || isnan(widened) || widened < 0.0 ||
               widened > (INT32_MAX + 0.0))) {
    return NOTOK;
  }
  /* Fractional indexes retain the established truncation behavior. */
  *result = (int32_t)widened;
  return OK;
}

/* Whether a member of a struct value has the type its struct declares. */
static int32_t struct_member_matches_type(const CS_VARIABLE *expected,
                                          const CS_VAR_MEM *actual)
{
  if (expected == NULL || expected->varType == NULL || actual == NULL ||
      actual->varType == NULL ||
      expected->varType != actual->varType) {
    return 0;
  }
  return expected->varType != &CS_VAR_TYPE_ARRAY ||
    expected->subType == NULL ||
    ((const ARRAYDAT *)&actual->value)->arrayType == expected->subType;
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
    if (!struct_member_matches_type((CS_VARIABLE *)expectedMember->value,
                                    value->members[i])) {
      return 0;
    }
    expectedMember = expectedMember->next;
  }
  return expectedMember == NULL;
}

static int32_t struct_array_flat_index(CSOUND *csound, OPDS *opds,
                                       const char *opcodeName,
                                       const ARRAYDAT *array,
                                       cs_float *const *indexes,
                                       int32_t indexCount,
                                       int32_t initializing,
                                       size_t *result)
{
  size_t flatIndex = 0;

  if (UNLIKELY(result == NULL || opcodeName == NULL || array == NULL ||
               indexes == NULL || array->dimensions <= 0 ||
               array->sizes == NULL || indexCount != array->dimensions)) {
    return initializing
      ? csound->InitError(
          csound, Str("%s: array dimensions do not match indexes\n"), opcodeName)
      : csound->PerfError(
          csound, opds,
          Str("%s: array dimensions do not match indexes\n"), opcodeName);
  }
  for (int32_t i = 0; i < indexCount; i++) {
    int32_t coordinate;
    size_t dimension;
    cs_float rawIndex;

    if (UNLIKELY(indexes[i] == NULL || array->sizes[i] <= 0)) {
      return initializing
        ? csound->InitError(csound,
                            Str("%s: invalid index or dimension %d\n"),
                            opcodeName, i + 1)
        : csound->PerfError(csound, opds,
                            Str("%s: invalid index or dimension %d\n"),
                            opcodeName, i + 1);
    }
    rawIndex = *indexes[i];
    if (UNLIKELY(nonnegative_index_from_myflt(rawIndex, &coordinate) != OK)) {
      return initializing
        ? csound->InitError(csound,
                            Str("%s: invalid index for dimension %d\n"),
                            opcodeName, i + 1)
        : csound->PerfError(csound, opds,
                            Str("%s: invalid index for dimension %d\n"),
                            opcodeName, i + 1);
    }
    dimension = (size_t)array->sizes[i];
    if (UNLIKELY(coordinate < 0 || (size_t)coordinate >= dimension)) {
      return initializing
        ? csound->InitError(
            csound, Str("%s: index %d out of range for dimension %d\n"),
            opcodeName, coordinate, i + 1)
        : csound->PerfError(
            csound, opds,
            Str("%s: index %d out of range for dimension %d\n"),
            opcodeName, coordinate, i + 1);
    }
    if (UNLIKELY(flatIndex >
                 (SIZE_MAX - (size_t)coordinate) / dimension)) {
      return initializing
        ? csound->InitError(csound, Str("%s: index overflow\n"), opcodeName)
        : csound->PerfError(csound, opds, Str("%s: index overflow\n"),
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
                                    int32_t initializing,
                                    void **result)
{
  size_t elementSize;
  size_t offset;
  size_t allocated;

  if (UNLIKELY(result == NULL || opcodeName == NULL || array == NULL ||
               array->data == NULL ||
               array->arrayMemberSize <= 0)) {
    return initializing
      ? csound->InitError(csound, Str("%s: invalid array storage\n"), opcodeName)
      : csound->PerfError(csound, opds, Str("%s: invalid array storage\n"),
                          opcodeName);
  }
  if (UNLIKELY(!csound_array_storage_matches(csound, array))) {
    return initializing
      ? csound->InitError(
          csound, Str("%s: inconsistent shared array storage\n"), opcodeName)
      : csound->PerfError(
          csound, opds,
          Str("%s: inconsistent shared array storage\n"), opcodeName);
  }
  elementSize = (size_t)array->arrayMemberSize;
  if (UNLIKELY(index > (SIZE_MAX - elementSize) / elementSize)) {
    return initializing
      ? csound->InitError(csound, Str("%s: offset overflow\n"), opcodeName)
      : csound->PerfError(csound, opds, Str("%s: offset overflow\n"),
                          opcodeName);
  }
  offset = index * elementSize;
  allocated = csound_array_allocated_bytes(csound, array);
  if (UNLIKELY(allocated < elementSize ||
               offset > allocated - elementSize)) {
    return initializing
      ? csound->InitError(
          csound, Str("%s: element exceeds allocated storage\n"), opcodeName)
      : csound->PerfError(
          csound, opds,
          Str("%s: element exceeds allocated storage\n"), opcodeName);
  }
  *result = (char *)array->data + offset;
  return OK;
}

/* A declared array can have complete dimensions before its backing store is
   created. At init, create it with the same initializer as other array
   paths. */
static int32_t struct_array_create_storage(CSOUND *csound, ARRAYDAT *array,
                                           INSDS *ctx)
{
  size_t totalSize;

  if (UNLIKELY(csound_array_member_count(array, &totalSize) != OK ||
               totalSize > INT32_MAX)) {
    return csound->InitError(csound,
                             Str("Invalid struct array dimensions\n"));
  }
  if (UNLIKELY(tabinit(csound, array, (int32_t)totalSize, ctx) != OK)) {
    return csound_array_init_resize_error(csound);
  }
  return OK;
}

/* The element of a struct array a write at indexes changes: the outer
   storage made writable, the indexes checked against its bounds, a whole
   struct written (source, or NULL for a member) checked against the
   array's type, and the element prepared at init. */
static int32_t struct_array_write_element(CSOUND *csound, ARRAY_SET *p,
                                          cs_float *const *indexes,
                                          int32_t indexCount,
                                          const CS_STRUCT_VAR *source,
                                          int32_t initializing,
                                          CS_STRUCT_VAR **result)
{
  ARRAYDAT* dat = p->arrayDat;
  CS_STRUCT_VAR *destination;
  size_t index;
  void *element;

  if (UNLIKELY(dat == NULL || p->value == NULL)) {
    return initializing
      ? csound->InitError(csound, Str("array_set_struct: NULL array or value\n"))
      : csound->PerfError(csound, &p->h,
                          Str("array_set_struct: NULL array or value\n"));
  }
  if (UNLIKELY(dat->arrayType == NULL ||
               !dat->arrayType->userDefinedType ||
               dat->arrayType->copyValue == NULL)) {
    return initializing
      ? csound->InitError(csound,
                          Str("array_set_struct: invalid element type\n"))
      : csound->PerfError(csound, &p->h,
                          Str("array_set_struct: invalid element type\n"));
  }
  if (UNLIKELY(csound_array_prepare_opcode_write(
                 csound, dat, &p->h, initializing,
                 Str("array_set_struct: could not prepare array for writing"))
               != OK)) {
    return NOTOK;
  }
  if (UNLIKELY(struct_array_flat_index(csound, &p->h, "array_set_struct",
                                       dat, indexes, indexCount,
                                       initializing,
                                       &index) != OK)) {
    return NOTOK;
  }
  if (UNLIKELY(struct_array_element(csound, &p->h, "array_set_struct",
                                    dat, index, initializing,
                                    &element) != OK)) {
    return NOTOK;
  }

  if (UNLIKELY(source != NULL &&
               !struct_value_matches_type(dat->arrayType, source))) {
    return initializing
      ? csound->InitError(
          csound, Str("array_set_struct: value does not match type\n"))
      : csound->PerfError(
          csound, &p->h,
          Str("array_set_struct: value does not match type\n"));
  }
  destination = (CS_STRUCT_VAR *)element;
  if (destination->members == NULL) {
    if (!initializing) {
      return csound->PerfError(
        csound, &p->h,
        Str("array_set_struct: destination was not prepared during init\n"));
    }
    size_t allocated = csound_array_allocated_bytes(csound, dat);
    if (UNLIKELY(csound_array_initialize_element_range(
                   csound, dat, allocated, index, index + 1,
                   p->h.insdshead) != OK)) {
      return initializing
        ? csound->InitError(
            csound, Str("array_set_struct: cannot initialize element\n"))
        : csound->PerfError(
            csound, &p->h,
            Str("array_set_struct: cannot initialize element\n"));
    }
  }
  *result = destination;
  return OK;
}

static int32_t array_set_struct_copy(CSOUND *csound, ARRAY_SET *p,
                                     CSOUND_STRUCT_COPY_MODE copyMode,
                                     int32_t initializing)
{
  CS_STRUCT_VAR *source = (CS_STRUCT_VAR *)p->value;
  CS_STRUCT_VAR *destination;

  if (UNLIKELY(struct_array_write_element(csound, p, p->indexes,
                                          p->INOCOUNT - 2, source,
                                          initializing,
                                          &destination) != OK)) {
    return NOTOK;
  }
  if (UNLIKELY(!struct_value_matches_type(p->arrayDat->arrayType,
                                          destination))) {
    return initializing
      ? csound->InitError(
          csound, Str("array_set_struct: destination does not match type\n"))
      : csound->PerfError(
          csound, &p->h,
          Str("array_set_struct: destination does not match type\n"));
  }

  /* The registered copier carries nested-array ownership through ordinary
     struct assignment and through this array-specific lowering path. */
  if (UNLIKELY(csound_copy_struct_value(
                 csound, p->arrayDat->arrayType, destination, source,
                 p->h.insdshead, copyMode) != OK)) {
    return initializing
      ? csound->InitError(csound,
                          Str("array_set_struct: could not copy value\n"))
      : csound->PerfError(csound, &p->h,
                          Str("array_set_struct: could not copy value\n"));
  }
  return OK;
}

/* The member a struct type declares at index nth, or NULL. */
static const CS_VARIABLE *struct_type_member(const CS_TYPE *type, int32_t nth)
{
  const CONS_CELL *cell = type != NULL ? type->members : NULL;

  while (cell != NULL && nth-- > 0) {
    cell = cell->next;
  }
  return cell != NULL ? (const CS_VARIABLE *)cell->value : NULL;
}

/* ##array_member_set: array, value, the member path's depth, its member
   indexes, then the array's indexes. It writes the value into that member
   of the element in place, through the member type's copier: a scalar, or
   an array of scalars, which ##member_set wrote the same way into the
   element's copy before ##array_set_struct copied it back. Only the members
   on the path are checked against the types the struct declares, so the
   work does not grow with the members it leaves alone. */
static int32_t array_member_set_common(CSOUND *csound, ARRAY_SET *p,
                                       int32_t initializing)
{
  int32_t argCount = p->INOCOUNT - 2;
  int32_t depth, level, nth;
  const CS_TYPE *structType = p->arrayDat != NULL ? p->arrayDat->arrayType
                                                   : NULL;
  const CS_VARIABLE *declared;
  CS_STRUCT_VAR *var;
  CS_VAR_MEM *member = NULL;

  if (UNLIKELY(argCount < 3 || p->indexes[0] == NULL ||
               nonnegative_index_from_myflt(*p->indexes[0], &depth) != OK ||
               depth < 1 || depth > argCount - 2)) {
    return initializing
      ? csound->InitError(csound, Str("Invalid member index\n"))
      : csound->PerfError(csound, &p->h, Str("Invalid member index\n"));
  }
  /* the read the element's copy began with created the backing store at
     init when only the dimensions were set; so does this write */
  if (initializing && p->arrayDat != NULL && p->arrayDat->data == NULL &&
      p->arrayDat->arrayType != NULL &&
      p->arrayDat->arrayType->userDefinedType &&
      UNLIKELY(struct_array_create_storage(csound, p->arrayDat,
                                           p->h.insdshead) != OK)) {
    return NOTOK;
  }
  if (UNLIKELY(struct_array_write_element(csound, p, p->indexes + 1 + depth,
                                          argCount - 1 - depth, NULL,
                                          initializing, &var) != OK)) {
    return NOTOK;
  }
  for (level = 1; level <= depth; level++) {
    if (UNLIKELY(nonnegative_index_from_myflt(*p->indexes[level], &nth) != OK ||
                 var == NULL || var->members == NULL ||
                 nth >= var->memberCount ||
                 (member = var->members[nth]) == NULL ||
                 (declared = struct_type_member(structType, nth)) == NULL)) {
      return initializing
        ? csound->InitError(csound, Str("Invalid member index\n"))
        : csound->PerfError(csound, &p->h, Str("Invalid member index\n"));
    }
    if (UNLIKELY(!struct_member_matches_type(declared, member) ||
                 (level < depth && !member->varType->userDefinedType))) {
      return initializing
        ? csound->InitError(
            csound, Str("array_set_struct: destination does not match type\n"))
        : csound->PerfError(
            csound, &p->h,
            Str("array_set_struct: destination does not match type\n"));
    }
    structType = member->varType;
    var = (CS_STRUCT_VAR *)&member->value;
  }
  if (UNLIKELY(member->varType->copyValue == NULL)) {
    return initializing
      ? csound->InitError(csound,
                          Str("array_set_struct: could not copy value\n"))
      : csound->PerfError(csound, &p->h,
                          Str("array_set_struct: could not copy value\n"));
  }
  member->varType->copyValue(csound, member->varType, &member->value,
                             p->value, p->h.insdshead);
  return OK;
}

int32_t array_member_set_i(CSOUND *csound, ARRAY_SET *p)
{
  return array_member_set_common(csound, p, 1);
}

int32_t array_member_set(CSOUND *csound, ARRAY_SET *p)
{
  return array_member_set_common(csound, p, 0);
}

int32_t array_set_struct_init(CSOUND *csound, ARRAY_SET *p)
{
  ARRAYDAT *dat = p->arrayDat;

  if (UNLIKELY(dat == NULL || dat->arrayType == NULL ||
               !dat->arrayType->userDefinedType)) {
    return csound->InitError(csound,
                             Str("array_set_struct: invalid destination\n"));
  }
  /* Preparation must not perform the k-rate assignment early. It only makes
     the outer destination independent; nested value storage is copied when
     the assignment actually executes. */
  if (UNLIKELY(csound_array_prepare_opcode_write(
                 csound, dat, &p->h, 1,
                 Str("array_set_struct: could not prepare array for writing"))
               != OK)) {
    return NOTOK;
  }
  return OK;
}

int32_t array_set_struct_i(CSOUND *csound, ARRAY_SET *p)
{
  return array_set_struct_copy(
    csound, p, CSOUND_STRUCT_COPY_INDEPENDENT_ALLOW_ALLOCATION, 1);
}

int32_t array_set_struct(CSOUND *csound, ARRAY_SET *p)
{
  /* The outer array must already be writable, but a recursively structured
     value may legitimately change its nested array dimensions at k-rate. */
  return array_set_struct_copy(
    csound, p, CSOUND_STRUCT_COPY_INDEPENDENT_ALLOW_ALLOCATION, 0);
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
                             Str("Invalid member index pointer (NULL)\n"));
  }
  if (UNLIKELY(p->out == NULL)) {
    return csound->PerfError(csound, &p->h,
                             Str("Invalid struct member output (NULL)\n"));
  }

  if (UNLIKELY(p->var == NULL)) {
    return csound->PerfError(csound, &(p->h), Str("Invalid struct pointer (NULL)\n"));
  }

  CS_STRUCT_VAR* varIn = (CS_STRUCT_VAR*)p->var;

  if (UNLIKELY(varIn == NULL || varIn->members == NULL)) {
    return csound->PerfError(csound, &(p->h), Str("Invalid struct for member_get\n"));
  }

  /* Safety: detect type confusion (ARRAYDAT being accessed as CS_STRUCT_VAR) */
  if (UNLIKELY(varIn->memberCount <= 0 || varIn->memberCount > 1000)) {
    ARRAYDAT* arrayDat = (ARRAYDAT*)varIn;
    if (arrayDat->arrayType && arrayDat->arrayType->userDefinedType) {
      return csound->PerfError(csound, &(p->h),
        /* TRANSLATORS: Keep array[index].member and array.member unchanged. */
        Str("Type confusion: trying to access array of structs as single struct. "
        "Use array[index].member syntax instead of array.member\n"));
    }
    return csound->PerfError(csound, &(p->h), Str("Corrupted struct: invalid memberCount=%d\n"), varIn->memberCount);
  }

  int32_t nthInt;
  if (UNLIKELY(nonnegative_index_from_myflt(*p->nths[0], &nthInt) != OK)) {
    return csound->PerfError(csound, &p->h, Str("Invalid member index\n"));
  }

  if (UNLIKELY(nthInt >= varIn->memberCount)) {
    return csound->PerfError(csound, &(p->h), Str("Invalid member index %d (memberCount=%d)\n"), nthInt, varIn->memberCount);
  }

  CS_VAR_MEM* member = varIn->members[nthInt];
  if (UNLIKELY(member == NULL)) {
    return csound->PerfError(csound, &p->h,
                             Str("Struct member %d is not initialized\n"), nthInt);
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
    return csound->PerfError(csound, &(p->h), Str("Invalid member index pointer (NULL)\n"));
  }

  // Check if p->var is NULL before casting
  if (UNLIKELY(p->var == NULL)) {
    return csound->PerfError(csound, &p->h,
                             Str("Invalid struct pointer (NULL)\n"));
  }
  if (UNLIKELY(p->in == NULL)) {
    return csound->PerfError(csound, &p->h,
                             Str("Invalid struct member input (NULL)\n"));
  }

  CS_STRUCT_VAR* var = (CS_STRUCT_VAR*)p->var;

  // Check if this is actually an ARRAYDAT (array of structs) instead of a struct
  // This happens when array[index].member syntax is used incorrectly
  ARRAYDAT* arrayCheck = (ARRAYDAT*)p->var;
  if (arrayCheck && arrayCheck->dimensions >= 0 && arrayCheck->dimensions < 100 &&
      arrayCheck->arrayType && arrayCheck->arrayType->userDefinedType &&
      arrayCheck->data != NULL) {
    return csound->PerfError(csound, &(p->h),
      /* TRANSLATORS: Keep array[index].member and the code after "Use:" unchanged. */
      Str("Cannot directly access array[index].member for struct arrays. "
      "Use: temp = array[index]; temp.member = value; array[index] = temp\n"));
  }

  if (UNLIKELY(var == NULL || var->members == NULL))
    return csound->PerfError(csound, &(p->h), Str("Invalid struct for member_set\n"));

  if (UNLIKELY(var->memberCount <= 0 || var->memberCount > 1000)) {
    return csound->PerfError(csound, &(p->h), Str("Corrupted struct: invalid memberCount=%d\n"), var->memberCount);
  }

  int32_t nthInt;
  if (UNLIKELY(nonnegative_index_from_myflt(*p->nths[0], &nthInt) != OK)) {
    return csound->PerfError(csound, &p->h, Str("Invalid member index\n"));
  }

  if (UNLIKELY(nthInt >= var->memberCount)) {
    return csound->PerfError(csound, &(p->h), Str("Invalid member index %d (memberCount=%d)\n"), nthInt, var->memberCount);
  }

  CS_VAR_MEM* member = var->members[nthInt];
  if (UNLIKELY(member == NULL)) {
    return csound->PerfError(csound, &p->h,
                             Str("Struct member %d is not initialized\n"), nthInt);
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
      return csound->PerfError(csound, &(p->h), Str("Invalid member index pointer (NULL)\n"));
    }

    // Check if p->var is NULL before casting
    if (UNLIKELY(p->var == NULL)) {
      return csound->PerfError(csound, &p->h,
                               Str("Invalid struct pointer (NULL)\n"));
    }
    if (UNLIKELY(p->in == NULL)) {
      return csound->PerfError(csound, &p->h,
                               Str("Invalid array member input (NULL)\n"));
    }

    CS_STRUCT_VAR* var = (CS_STRUCT_VAR*)p->var;
    int32_t nthInt;

    if (UNLIKELY(var == NULL || var->members == NULL))
      return csound->PerfError(csound, &(p->h), Str("Invalid struct for member_array_assign\n"));

    if (UNLIKELY(nonnegative_index_from_myflt(*p->nths[0], &nthInt) != OK)) {
      return csound->PerfError(csound, &p->h, Str("Invalid member index\n"));
    }
    if (UNLIKELY(nthInt >= var->memberCount)) {
      return csound->PerfError(csound, &(p->h),
        Str("Member index %d out of bounds (memberCount=%d)\n"), nthInt, var->memberCount);
    }

    CS_VAR_MEM* member = var->members[nthInt];

    if (UNLIKELY(member == NULL || member->varType != &CS_VAR_TYPE_ARRAY)) {
      return csound->PerfError(csound, &(p->h), Str("Selected member is not an array for member_array_assign\n"));
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
    return csound->PerfError(csound, &(p->h), Str("Invalid struct for struct_alias\n"));

  /* Capture existing destination members for deferred free at deinit */
  p->oldMembers = NULL;
  p->oldMemberCount = 0;
  p->oldOwned = 0;

  if (dst->ownsMembers && dst->members && dst->memberCount > 0) {
    p->oldMembers = dst->members;
    p->oldMemberCount = dst->memberCount;
    p->oldOwned = dst->ownsMembers;
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


static int32_t struct_array_read(CSOUND *csound, STRUCT_ARRAY_GET *dat,
                                 int32_t initializing)
{
  ARRAYDAT* arrayDat = dat->arrayDat;
  CS_STRUCT_VAR *source;
  CS_STRUCT_VAR *destination = (CS_STRUCT_VAR *)dat->out;
  size_t index;
  void *element;
  int32_t indexCount = dat->INOCOUNT - 1;

  if (UNLIKELY(arrayDat == NULL || destination == NULL)) {
    return initializing
      ? csound->InitError(
          csound, Str("struct_array_get: NULL array or output\n"))
      : csound->PerfError(
          csound, &dat->h, Str("struct_array_get: NULL array or output\n"));
  }
  if (UNLIKELY(arrayDat->arrayType == NULL ||
               !arrayDat->arrayType->userDefinedType ||
               arrayDat->arrayType->copyValue == NULL)) {
    return initializing
      ? csound->InitError(
          csound, Str("struct_array_get: invalid element type\n"))
      : csound->PerfError(
          csound, &dat->h, Str("struct_array_get: invalid element type\n"));
  }

  if (arrayDat->data == NULL) {
    if (!initializing) {
      return csound->PerfError(
        csound, &dat->h, Str("struct_array_get: array is not initialized\n"));
    }
    if (UNLIKELY(struct_array_create_storage(csound, arrayDat,
                                             dat->h.insdshead) != OK)) {
      return NOTOK;
    }
  }
  if (UNLIKELY(struct_array_flat_index(csound, &dat->h,
                                       "struct_array_get", arrayDat,
                                       dat->indicies, indexCount,
                                       initializing,
                                       &index) != OK)) {
    return NOTOK;
  }
  if (UNLIKELY(struct_array_element(csound, &dat->h, "struct_array_get",
                                    arrayDat, index, initializing,
                                    &element) != OK)) {
    return NOTOK;
  }

  source = (CS_STRUCT_VAR *)element;
  if (UNLIKELY(!struct_value_matches_type(arrayDat->arrayType, source))) {
    return initializing
      ? csound->InitError(
          csound, Str("struct_array_get: element does not match type\n"))
      : csound->PerfError(
          csound, &dat->h,
          Str("struct_array_get: element does not match type\n"));
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
      return initializing
        ? csound->InitError(
            csound, Str("Could not initialize struct array output\n"))
        : csound->PerfError(
            csound, &dat->h,
            Str("Could not initialize struct array output\n"));
    }
    helper->initializeVariableMemory(csound, helper, (cs_float *)destination);
    csound->Free(csound, helper);
  }
  if (UNLIKELY(!struct_value_matches_type(arrayDat->arrayType,
                                          destination))) {
    return initializing
      ? csound->InitError(
          csound, Str("struct_array_get: output does not match type\n"))
      : csound->PerfError(
          csound, &dat->h,
          Str("struct_array_get: output does not match type\n"));
  }

  arrayDat->arrayType->copyValue(csound, arrayDat->arrayType,
                                 destination, source, dat->h.insdshead);
  return OK;
}

int32_t struct_array_get_init(CSOUND *csound, STRUCT_ARRAY_GET *dat)
{
  return struct_array_read(csound, dat, 1);
}

int32_t struct_array_get(CSOUND *csound, STRUCT_ARRAY_GET *dat)
{
  return struct_array_read(csound, dat, 0);
}


int32_t struct_alias_deinit(CSOUND *csound, STRUCT_ALIAS *p)
{
  if (p->oldOwned && p->oldMembers) {
    CS_STRUCT_VAR tmp;
    tmp.members     = p->oldMembers;
    tmp.memberCount = p->oldMemberCount;
    tmp.ownsMembers = p->oldOwned;
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
    return csound->PerfError(csound, &(p->h), Str("Invalid struct variable for initialization\n"));
  }

  // The struct variable should already be created by the type system
  // We just need to initialize its members with the provided values

  if (UNLIKELY(structVar->members == NULL || structVar->memberCount == 0)) {
    return csound->PerfError(csound, &(p->h), Str("Struct has no members to initialize\n"));
  }

  // Get the number of input arguments (excluding the output)
  int32_t argCount = 0;
  while (argCount < VARGMAX && p->args[argCount] != NULL) {
    argCount++;
  }

  if (UNLIKELY(argCount != structVar->memberCount)) {
    return csound->PerfError(csound, &(p->h),
                            Str("Struct initialization: expected %d arguments, got %d\n"),
                            structVar->memberCount, argCount);
  }

  // Initialize each member with the corresponding argument
  for (int32_t i = 0; i < structVar->memberCount; i++) {
    CS_VAR_MEM* member = structVar->members[i];
    if (UNLIKELY(member == NULL)) {
      return csound->PerfError(csound, &(p->h), Str("Struct member %d is NULL\n"), i);
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
