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

#include <math.h>
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
    CS_VARIABLE *var;

    if (arrayType == NULL || arrayType->createVariable == NULL) {
        return NULL;
    }

    /* arrays.h is used by opcode modules that do not link directly against
       libcsound. Keep this installed inline helper self-contained while
       preserving the invariant enforced by csoundCreateVariableForType(). */
    var = arrayType->createVariable(csound, arrayType, NULL, ctx);
    if (var != NULL) {
        var->varType = arrayType;
    }
    return var;
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
    MYFLT *newData;
    size_t oldCapacity = 0;
    size_t bytes;
    int32_t memberSize = array->arrayMemberSize;
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
        memberSize = var->memBlockSize;
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
    if (csound_array_allocation_size(memberSize, capacity,
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
            var->memBlockSize != memberSize) {
            if (var != NULL) {
                csound->Free(csound, var);
            }
            return NOTOK;
        }
    }
    if (fresh) {
        newData = (MYFLT *)csound->Calloc(csound, bytes);
        if (UNLIKELY(newData == NULL)) {
            csound->Free(csound, var);
            return NOTOK;
        }
        if (UNLIKELY(csound_array_initialize_struct_range(
                       csound, array->arrayType, var, newData, memberSize,
                       oldCapacity, capacity) != OK)) {
            csound->Free(csound, newData);
            csound->Free(csound, var);
            return NOTOK;
        }
        array->arrayMemberSize = memberSize;
        array->data = newData;
        array->allocated = bytes;
    }
    else {
        newData = (MYFLT *)csound->ReAlloc(csound, array->data, bytes);
        if (UNLIKELY(newData == NULL)) {
            csound->Free(csound, var);
            return NOTOK;
        }
        array->data = newData;
        memset((char *)newData + array->allocated, 0,
               bytes - array->allocated);
        array->allocated = bytes;
        if (UNLIKELY(csound_array_initialize_struct_range(
                       csound, array->arrayType, var, newData, memberSize,
                       oldCapacity, capacity) != OK)) {
            csound->Free(csound, var);
            return NOTOK;
        }
    }
    if (var != NULL) {
        csound->Free(csound, var);
    }
    return OK;
}

/* Resize an array. Return NOTOK without publishing a new logical size when
   validation, detachment, or allocation fails. */
static inline int32_t tabinit(CSOUND *csound, ARRAYDAT *p, int32_t size,
                              INSDS *ctx)
{
    int32_t *newSizes = NULL;
    size_t capacity;

    if (UNLIKELY(p == NULL || size < 0 || p->dimensions < 0)) {
        return NOTOK;
    }
    if (UNLIKELY(p->dimensions > 1 && p->sizes == NULL)) {
        return NOTOK;
    }
    if (p->dimensions <= 1 && p->sizes == NULL) {
        newSizes = (int32_t *)csound->Calloc(csound, sizeof(int32_t));
        if (UNLIKELY(newSizes == NULL)) {
            return NOTOK;
        }
    }
    if (UNLIKELY(csound_array_prepare_write(csound, p, ctx) != OK)) {
        csound->Free(csound, newSizes);
        return NOTOK;
    }
    capacity = size > 0 ? (size_t)size : 1;
    if (UNLIKELY(csound_array_ensure_capacity(csound, p, capacity, ctx)
                 != OK)) {
        csound->Free(csound, newSizes);
        return NOTOK;
    }
    if (newSizes != NULL) {
        p->sizes = newSizes;
    }
    if (p->dimensions <= 1) {
        p->dimensions = 1;
        p->sizes[0] = size;
    }
    return OK;
}

/* Match another array's layout. Return NOTOK without publishing partial size
   metadata when validation, detachment, or allocation fails. */
static inline int32_t tabinit_like(CSOUND *csound, ARRAYDAT *p,
                                   const ARRAYDAT *tp)
{
    int32_t *newSizes = NULL;
    const CS_TYPE *originalArrayType;
    const CS_TYPE *targetArrayType;
    size_t elementCount;
    size_t capacity;

    if (UNLIKELY(p == NULL || tp == NULL || p->dimensions < 0 ||
                 tp->dimensions < 0 ||
                 csound_array_member_count(tp, &elementCount) != OK)) {
        return NOTOK;
    }
    if (p == tp) {
        return OK;
    }
    originalArrayType = p->arrayType;
    targetArrayType = originalArrayType != NULL
      ? originalArrayType : tp->arrayType;
    if (UNLIKELY(targetArrayType == NULL ||
                 !csound_array_element_types_compatible(
                   targetArrayType, tp->arrayType))) {
        return NOTOK;
    }
    if (tp->dimensions > 0 &&
        (p->dimensions != tp->dimensions || p->sizes == NULL)) {
        newSizes = (int32_t *)csound->Calloc(
          csound, sizeof(int32_t) * (size_t)tp->dimensions);
        if (UNLIKELY(newSizes == NULL)) {
            return NOTOK;
        }
        memcpy(newSizes, tp->sizes,
               sizeof(int32_t) * (size_t)tp->dimensions);
    }
    if (UNLIKELY(csound_array_prepare_write(csound, p, NULL) != OK)) {
        p->arrayType = originalArrayType;
        csound->Free(csound, newSizes);
        return NOTOK;
    }
    if (p->data == tp->data) {
        p->arrayType = targetArrayType;
        csound->Free(csound, newSizes);
        return OK;
    }

    capacity = elementCount > 0 ? elementCount : 1;
    p->arrayType = targetArrayType;
    if (UNLIKELY(csound_array_ensure_capacity(csound, p, capacity, NULL)
                 != OK)) {
        p->arrayType = originalArrayType;
        csound->Free(csound, newSizes);
        return NOTOK;
    }
    if (tp->dimensions == 0) {
        csound->Free(csound, p->sizes);
        p->sizes = NULL;
        p->dimensions = 0;
    }
    else if (newSizes != NULL) {
        csound->Free(csound, p->sizes);
        p->sizes = newSizes;
        p->dimensions = tp->dimensions;
        newSizes = NULL;
    }
    else {
        memcpy(p->sizes, tp->sizes,
               sizeof(int32_t) * (size_t)tp->dimensions);
    }
    return OK;
}

static inline int32_t csound_array_init_resize_error(CSOUND *csound)
{
    return csound->InitError(csound, "%s", Str("Could not resize array"));
}

static inline int32_t csound_array_perf_resize_error(CSOUND *csound,
                                                     OPDS *ctx)
{
    return csound->PerfError(csound, ctx, "%s",
                             Str("Could not resize array"));
}

static inline int32_t csound_array_size_to_int32(MYFLT requestedSize,
                                                 int32_t *size)
{
    double value = (double)requestedSize;

    if (size == NULL || isnan(value) || value < 0.0 ||
        value >= (double)INT32_MAX + 1.0) {
        return NOTOK;
    }
    *size = (int32_t)value;
    return OK;
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
