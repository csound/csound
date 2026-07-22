/*
    array.h:

    Copyright (C) 2011, 2017 John ffitch and Stephen Kyne

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

#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Ownership remains engine-private. Installed plugins dispatch the detach
   operation through their CSOUND instance instead of embedding that logic. */
static inline int32_t csound_array_prepare_write(CSOUND *csound,
                                                  ARRAYDAT *array,
                                                  INSDS *ctx)
{
    if (csound == NULL || csound->ArrayPrepareWrite == NULL) {
        return NOTOK;
    }
    return csound->ArrayPrepareWrite(csound, array, ctx, 1);
}

/* Performance-time writers may claim uniquely referenced storage, but must
   not clone a shared structured array. */
static inline int32_t csound_array_try_prepare_write(CSOUND *csound,
                                                      ARRAYDAT *array,
                                                      INSDS *ctx)
{
    if (csound == NULL || csound->ArrayPrepareWrite == NULL) {
        return NOTOK;
    }
    return csound->ArrayPrepareWrite(csound, array, ctx, 0);
}

/* Managed elements own memory or contain nested runtime values. They must be
   copied and cleared through their type callbacks, never as raw bytes. */
static inline int32_t csound_array_has_managed_elements(
    const ARRAYDAT *array)
{
    return array != NULL && array->arrayType != NULL &&
      (array->arrayType->userDefinedType ||
       array->arrayType->freeVariableMemory != NULL);
}

typedef struct {
    OPDS    h;
    MYFLT   *r, *a;
} AEVAL;


static inline CS_VARIABLE *array_element_create_variable(CSOUND *csound,
                                                         const CS_TYPE *arrayType,
                                                         INSDS *ctx)
{
    if (arrayType == NULL || arrayType->createVariable == NULL) {
        return NULL;
    }
    return csoundCreateVariableForType(csound, arrayType, NULL, ctx);
}

static inline int32_t csound_array_element_types_compatible(
    const CS_TYPE *destination, const CS_TYPE *source)
{
    const char *destinationName;
    const char *sourceName;

    if (destination == source) {
        return 1;
    }
    if (source == NULL) {
        return 0;
    }
    if (destination == NULL) {
        return 1;
    }
    destinationName = destination->varTypeName;
    sourceName = source->varTypeName;
    /* Pointer equality above handles canonical built-in and user-defined
       types, including multi-character names. Name-based compatibility is
       deliberately limited to the legacy one-letter i/k rate exception. */
    if (destinationName == NULL || sourceName == NULL ||
        destinationName[0] == '\0' || sourceName[0] == '\0' ||
        destinationName[1] != '\0' || sourceName[1] != '\0') {
        return 0;
    }
    /* Array channels have historically allowed i[] and k[] to connect; both
       store MYFLT elements, while the receiving variable retains its rate. */
    return (destinationName[0] == 'i' && sourceName[0] == 'k') ||
           (destinationName[0] == 'k' && sourceName[0] == 'i');
}

static inline int32_t csound_array_member_count(const ARRAYDAT *array,
                                                size_t *result)
{
    size_t count = 1;

    if (result == NULL) {
        return NOTOK;
    }
    *result = 0;
    if (array == NULL || array->dimensions < 0) {
        return NOTOK;
    }
    if (array->dimensions == 0) {
        return OK;
    }
    if (array->sizes == NULL ||
        (size_t)array->dimensions > SIZE_MAX / sizeof(int32_t)) {
        return NOTOK;
    }
    for (int32_t i = 0; i < array->dimensions; i++) {
        if (array->sizes[i] < 0) {
            return NOTOK;
        }
        if (array->sizes[i] == 0) {
            count = 0;
        } else if (count != 0) {
            if ((size_t)array->sizes[i] > SIZE_MAX / count) {
                return NOTOK;
            }
            count *= (size_t)array->sizes[i];
        }
    }
    *result = count;
    return OK;
}

static inline int32_t csound_array_allocation_size(int32_t memberSize,
                                                   size_t count,
                                                   size_t *result)
{
    if (result == NULL || memberSize <= 0 ||
        count > SIZE_MAX / (size_t)memberSize) {
        return NOTOK;
    }
    *result = count * (size_t)memberSize;
    return OK;
}

static inline int32_t csound_array_initialize_struct_range(
    CSOUND *csound, const CS_TYPE *arrayType, CS_VARIABLE *var,
    MYFLT *data, int32_t memberSize, size_t begin, size_t end)
{
    if (arrayType == NULL || !arrayType->userDefinedType || begin == end) {
        return OK;
    }
    if (var == NULL || var->initializeVariableMemory == NULL ||
        var->memBlockSize != memberSize) {
        return NOTOK;
    }
    for (size_t i = begin; i < end; i++) {
        void *element = (char *)data + i * (size_t)memberSize;
        var->initializeVariableMemory(csound, var, (MYFLT *)element);
    }
    return OK;
}

static inline int32_t csound_array_ensure_capacity(CSOUND *csound,
                                                   ARRAYDAT *array,
                                                   size_t capacity,
                                                   INSDS *ctx)
{
    CS_VARIABLE *var = NULL;
    size_t oldCapacity = 0;
    size_t bytes;
    int32_t fresh = array->data == NULL;

    if (array->arrayType == NULL || capacity == 0) {
        return NOTOK;
    }
    if (fresh) {
        var = array_element_create_variable(csound, array->arrayType, ctx);
        if (var == NULL || var->memBlockSize <= 0 ||
            (array->arrayType->userDefinedType &&
             var->initializeVariableMemory == NULL)) {
            if (var != NULL) {
                csound->Free(csound, var);
            }
            return NOTOK;
        }
        array->arrayMemberSize = var->memBlockSize;
    }
    else {
        /* allocated == 0 marks a legacy non-owning view. Only managed
           structured views can be detached safely before resizing. A view
           may still reuse its known logical extent without taking ownership. */
        if (array->allocated == 0) {
            size_t logicalCapacity;

            if (array->arrayMemberSize <= 0 ||
                csound_array_member_count(array, &logicalCapacity) != OK ||
                capacity > logicalCapacity) {
                return NOTOK;
            }
            return OK;
        }
        if (array->arrayMemberSize <= 0 ||
            array->allocated % (size_t)array->arrayMemberSize != 0) {
            return NOTOK;
        }
        oldCapacity = array->allocated / (size_t)array->arrayMemberSize;
    }
    if (csound_array_allocation_size(array->arrayMemberSize, capacity,
                                     &bytes) != OK) {
        if (var != NULL) {
            csound->Free(csound, var);
        }
        return NOTOK;
    }
    if (!fresh && bytes <= array->allocated) {
        return OK;
    }

    if (!fresh && array->arrayType->userDefinedType) {
        var = array_element_create_variable(csound, array->arrayType, ctx);
        if (var == NULL || var->initializeVariableMemory == NULL ||
            var->memBlockSize != array->arrayMemberSize) {
            if (var != NULL) {
                csound->Free(csound, var);
            }
            return NOTOK;
        }
    }
    if (fresh) {
        array->data = (MYFLT *)csound->Calloc(csound, bytes);
    }
    else {
        array->data = (MYFLT *)csound->ReAlloc(csound, array->data, bytes);
        memset((char *)array->data + array->allocated, 0,
               bytes - array->allocated);
    }
    array->allocated = bytes;
    if (csound_array_initialize_struct_range(
          csound, array->arrayType, var, array->data,
          array->arrayMemberSize, oldCapacity, capacity) != OK) {
        csound->Free(csound, var);
        return NOTOK;
    }
    if (var != NULL) {
        csound->Free(csound, var);
    }
    return OK;
}

static inline void tabinit(CSOUND *csound, ARRAYDAT *p, int32_t size,
                           INSDS *ctx)
{
    size_t capacity;

    if (UNLIKELY(p == NULL || size < 0 || p->dimensions < 0)) {
        csound->Die(csound, "tabinit: invalid array or size");
        return;
    }
    if (UNLIKELY(csound_array_prepare_write(csound, p, ctx) != OK)) {
        csound->Die(csound, "tabinit: could not detach shared array");
        return;
    }
    if (p->dimensions == 0) {
        p->dimensions = 1;
    }
    if (p->dimensions == 1 && p->sizes == NULL) {
        p->sizes = (int32_t *)csound->Calloc(csound, sizeof(int32_t));
    }
    if (UNLIKELY(p->dimensions > 1 && p->sizes == NULL)) {
        csound->Die(csound, "tabinit: multidimensional array has no sizes");
        return;
    }
    capacity = size > 0 ? (size_t)size : 1;
    if (UNLIKELY(csound_array_ensure_capacity(csound, p, capacity, ctx)
                 != OK)) {
        csound->Die(csound, "tabinit: could not allocate array storage");
        return;
    }
    if (p->dimensions == 1) {
        p->sizes[0] = size;
    }
}

static inline void tabinit_like(CSOUND *csound, ARRAYDAT *p,
                                const ARRAYDAT *tp)
{
    size_t elementCount;
    size_t capacity;

    if (UNLIKELY(p == NULL || tp == NULL || tp->dimensions < 0 ||
                 csound_array_member_count(tp, &elementCount) != OK)) {
        csound->Die(csound, "tabinit_like: invalid source array");
        return;
    }
    if (p == tp) {
        return;
    }
    if (p->arrayType == NULL) {
        p->arrayType = tp->arrayType;
    }
    if (UNLIKELY(p->arrayType == NULL ||
                 !csound_array_element_types_compatible(
                   p->arrayType, tp->arrayType))) {
        csound->Die(csound, "tabinit_like: array types do not match");
        return;
    }
    if (UNLIKELY(csound_array_prepare_write(csound, p, NULL) != OK)) {
        csound->Die(csound, "tabinit_like: could not detach shared array");
        return;
    }
    if (p->data == tp->data) {
        return;
    }

    if (tp->dimensions == 0) {
        csound->Free(csound, p->sizes);
        p->sizes = NULL;
        p->dimensions = 0;
    }
    else if (p->dimensions != tp->dimensions || p->sizes == NULL) {
        p->sizes = (int32_t *)csound->ReAlloc(
          csound, p->sizes, sizeof(int32_t) * (size_t)tp->dimensions);
        p->dimensions = tp->dimensions;
    }
    if (tp->dimensions > 0) {
        memcpy(p->sizes, tp->sizes,
               sizeof(int32_t) * (size_t)tp->dimensions);
    }
    capacity = elementCount > 0 ? elementCount : 1;
    if (UNLIKELY(csound_array_ensure_capacity(csound, p, capacity, NULL)
                 != OK)) {
        csound->Die(csound, "tabinit_like: could not allocate array storage");
    }
}

static inline int32_t tabcheck(CSOUND *csound, ARRAYDAT *p, int32_t size, OPDS *q)
{
    size_t bytes;

    if (UNLIKELY(p == NULL || size < 0)) {
      return csound->PerfError(csound, q, "%s", Str("Invalid array size"));
    }
    /* The caller writes this buffer during performance. Claiming a
       sole reference is safe here, but cloning shared storage would allocate. */
    if (UNLIKELY(csound_array_try_prepare_write(
                   csound, p, q != NULL ? q->insdshead : NULL) != OK)) {
      return csound->PerfError(csound, q, "%s",
                               Str("Cannot write shared array during "
                                   "performance pass"));
    }
    if (p->data == NULL || p->dimensions == 0 || p->sizes == NULL) {
      return csound->PerfError(csound, q, "%s", Str("Array not initialised"));
    }
    if (UNLIKELY(csound_array_allocation_size(
                   p->arrayMemberSize, (size_t)size, &bytes) != OK)) {
      return csound->PerfError(csound, q, "%s",
                               Str("Array size overflow"));
    }
    if (bytes > p->allocated) { /* was arr->allocate */
      return csound->PerfError(csound, q,
        Str("Array too small (allocated %zu < needed %zu), but cannot "
            "allocate during performance pass. Allocate a bigger array at init time"),
        p->allocated, bytes);
    }
    p->sizes[0] = size;
    return OK;
}

#endif /* end of include guard: __ARRAY_H__ */
