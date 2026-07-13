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

/* Shared backing for arrays of user-defined structs. Recursive struct links
   are represented by arrays, making this the ownership boundary needed when
   an instrument frame is recycled. It tracks one allocation, not the complete
   object graph, and is not a garbage collector; reference cycles are therefore
   not collected. The count protects storage shared by independently
   synchronized views. Copying or mutating the same ARRAYDAT still requires
   Csound's usual lock. */
typedef struct cs_array_storage {
    int32_t refs;
    /* Kept in the layout on every target so separately built plugins agree
       on the sidecar ABI. It is allocated only when native atomics are absent. */
    void *refLock;
    int32_t dimensions;
    int32_t arrayMemberSize;
    const CS_TYPE *arrayType;
    int32_t *sizes;
    MYFLT *data;
    size_t allocated;
} CS_ARRAY_STORAGE;

static inline int32_t cs_array_storage_ref_count(
    CSOUND *csound, CS_ARRAY_STORAGE *storage)
{
#if defined(MSVC)
    IGN(csound);
    return (int32_t)InterlockedCompareExchange(
      (volatile LONG *)&storage->refs, 0, 0);
#elif defined(HAVE_ATOMIC_BUILTIN)
    IGN(csound);
    return __atomic_load_n(&storage->refs, __ATOMIC_ACQUIRE);
#else
    int32_t refs;
    if (UNLIKELY(storage->refLock == NULL)) {
        return 0;
    }
    csound->LockMutex(storage->refLock);
    refs = storage->refs;
    csound->UnlockMutex(storage->refLock);
    return refs;
#endif
}

static inline int32_t cs_array_storage_try_add_ref(
    CSOUND *csound, CS_ARRAY_STORAGE *storage)
{
#if defined(MSVC)
    IGN(csound);
    LONG refs = InterlockedCompareExchange(
      (volatile LONG *)&storage->refs, 0, 0);
    while (refs > 0 && refs < INT32_MAX) {
        LONG observed = InterlockedCompareExchange(
          (volatile LONG *)&storage->refs, refs + 1, refs);
        if (observed == refs) {
            return (int32_t)(refs + 1);
        }
        refs = observed;
    }
    return NOTOK;
#elif defined(HAVE_ATOMIC_BUILTIN)
    IGN(csound);
    int32_t refs = __atomic_load_n(&storage->refs, __ATOMIC_RELAXED);
    while (refs > 0 && refs < INT32_MAX) {
        if (__atomic_compare_exchange_n(&storage->refs, &refs, refs + 1,
                                        0, __ATOMIC_ACQ_REL,
                                        __ATOMIC_ACQUIRE)) {
            return refs + 1;
        }
    }
    return NOTOK;
#else
    int32_t refs;
    if (UNLIKELY(storage->refLock == NULL)) {
        return NOTOK;
    }
    csound->LockMutex(storage->refLock);
    if (storage->refs <= 0 || storage->refs == INT32_MAX) {
        csound->UnlockMutex(storage->refLock);
        return NOTOK;
    }
    refs = ++storage->refs;
    csound->UnlockMutex(storage->refLock);
    return refs;
#endif
}

static inline int32_t cs_array_storage_try_claim(
    CSOUND *csound, CS_ARRAY_STORAGE *storage)
{
#if defined(MSVC)
    IGN(csound);
    return InterlockedCompareExchange((volatile LONG *)&storage->refs,
                                      0, 1) == 1 ? OK : NOTOK;
#elif defined(HAVE_ATOMIC_BUILTIN)
    IGN(csound);
    int32_t expected = 1;
    return __atomic_compare_exchange_n(&storage->refs, &expected, 0, 0,
                                       __ATOMIC_ACQ_REL,
                                       __ATOMIC_ACQUIRE) ? OK : NOTOK;
#else
    int32_t result = NOTOK;
    if (UNLIKELY(storage->refLock == NULL)) {
        return result;
    }
    csound->LockMutex(storage->refLock);
    if (storage->refs != 1) {
        csound->UnlockMutex(storage->refLock);
        return result;
    }
    storage->refs = 0;
    result = OK;
    csound->UnlockMutex(storage->refLock);
    return result;
#endif
}

static inline int32_t cs_array_storage_release_ref(
    CSOUND *csound, CS_ARRAY_STORAGE *storage)
{
#if defined(MSVC)
    IGN(csound);
    return (int32_t)InterlockedDecrement((volatile LONG *)&storage->refs);
#elif defined(HAVE_ATOMIC_BUILTIN)
    IGN(csound);
    return __atomic_sub_fetch(&storage->refs, 1, __ATOMIC_ACQ_REL);
#else
    int32_t refs;
    if (UNLIKELY(storage->refLock == NULL)) {
        return NOTOK;
    }
    csound->LockMutex(storage->refLock);
    refs = --storage->refs;
    csound->UnlockMutex(storage->refLock);
    return refs;
#endif
}

#ifdef __cplusplus
extern "C" {
#endif
/* Internal runtime entry points used by core array writers. The inline helper
   below remains self-contained because utility plugins do not link these
   symbols. */
void csound_array_prepare_write(CSOUND *csound, ARRAYDAT *array,
                                INSDS *ctx);
void csound_array_share(CSOUND *csound, ARRAYDAT *dest, ARRAYDAT *src);
void csound_free_array_storage(CSOUND *csound, ARRAYDAT *array);
#ifdef __cplusplus
}
#endif

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
    void *typeArg = (arrayType && arrayType->userDefinedType)
                      ? (void *)arrayType : NULL;
    return arrayType->createVariable(csound, typeArg, ctx);
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

static inline void csound_array_free_elements_inline(
    CSOUND *csound, const CS_TYPE *arrayType, MYFLT *data,
    size_t allocated, int32_t memberSize)
{
    /* Empty arrays own one initialized placeholder, so teardown follows
       allocated capacity rather than the current logical element count. */
    if (arrayType != NULL && arrayType->freeVariableMemory != NULL &&
        data != NULL && memberSize > 0) {
        /* If legacy metadata is malformed, free every complete element and
           leave only an incomplete trailing fragment to the raw-buffer free. */
        size_t count = allocated / (size_t)memberSize;
        for (size_t i = 0; i < count; i++) {
            void *element = (char *)data + i * (size_t)memberSize;
            arrayType->freeVariableMemory(csound, element);
        }
    }
}

static inline void cs_array_storage_destroy_inline(
    CSOUND *csound, CS_ARRAY_STORAGE *storage)
{
    if (storage == NULL) {
        return;
    }
    csound_array_free_elements_inline(csound, storage->arrayType,
                                      storage->data, storage->allocated,
                                      storage->arrayMemberSize);
    csound->Free(csound, storage->data);
    csound->Free(csound, storage->sizes);
    if (storage->refLock != NULL) {
        csound->DestroyMutex(storage->refLock);
    }
    csound->Free(csound, storage);
}

static inline int32_t cs_array_storage_layout(
    const ARRAYDAT *array, const CS_ARRAY_STORAGE *storage,
    size_t *logicalCount, size_t *capacity, size_t *strideBytes)
{
    if (array == NULL || storage == NULL || logicalCount == NULL ||
        capacity == NULL || strideBytes == NULL ||
        storage->dimensions <= 0 ||
        storage->dimensions != array->dimensions ||
        storage->arrayType == NULL ||
        !storage->arrayType->userDefinedType ||
        storage->arrayType->copyValue == NULL || storage->data == NULL ||
        storage->sizes != array->sizes || storage->data != array->data ||
        storage->arrayType != array->arrayType ||
        storage->arrayMemberSize != array->arrayMemberSize ||
        storage->arrayMemberSize <= 0 ||
        storage->allocated < (size_t)storage->arrayMemberSize ||
        storage->allocated % (size_t)storage->arrayMemberSize != 0 ||
        (array->allocated != 0 && array->allocated != storage->allocated) ||
        csound_array_member_count(array, logicalCount) != OK) {
        return NOTOK;
    }

    *capacity = storage->allocated / (size_t)storage->arrayMemberSize;
    *strideBytes = (size_t)storage->arrayMemberSize;
    return *logicalCount <= *capacity ? OK : NOTOK;
}

static inline int32_t cs_array_storage_clone(
    CSOUND *csound, const ARRAYDAT *array, const CS_ARRAY_STORAGE *storage,
    INSDS *ctx, size_t capacity, size_t strideBytes,
    ARRAYDAT *clone)
{
    CS_VARIABLE *var;

    memset(clone, 0, sizeof(*clone));
    clone->dimensions = storage->dimensions;
    clone->arrayMemberSize = storage->arrayMemberSize;
    clone->arrayType = storage->arrayType;
    clone->allocated = storage->allocated;
    if (clone->dimensions > 0) {
        clone->sizes = (int32_t *)csound->Malloc(
          csound, sizeof(int32_t) * (size_t)clone->dimensions);
        memcpy(clone->sizes, array->sizes,
               sizeof(int32_t) * (size_t)clone->dimensions);
    }
    clone->data = (MYFLT *)csound->Calloc(csound, clone->allocated);

    var = array_element_create_variable(csound, clone->arrayType, ctx);
    if (UNLIKELY(var == NULL || var->initializeVariableMemory == NULL)) {
        csound->Free(csound, clone->data);
        csound->Free(csound, clone->sizes);
        memset(clone, 0, sizeof(*clone));
        return NOTOK;
    }
    for (size_t i = 0; i < capacity; i++) {
        void *element = (char *)clone->data + i * strideBytes;
        var->initializeVariableMemory(csound, var, (MYFLT *)element);
    }
    csound->Free(csound, var);

    /* Capacity slots are all initialized and may become visible after a later
       resize, so copy them even when the current logical size is smaller. */
    for (size_t i = 0; i < capacity; i++) {
        size_t offset = i * strideBytes;
        clone->arrayType->copyValue(csound, clone->arrayType,
                                    (char *)clone->data + offset,
                                    (char *)storage->data + offset, ctx);
    }
    return OK;
}

/* Detach a shared structured array before an inline array helper mutates it.
   The first write to a shared value may allocate; later writes are in-place.
   This stays inline because utility plugins use tabinit without linking to
   internal Csound symbols. */
static inline void csound_array_prepare_write_inline(CSOUND *csound,
                                                      ARRAYDAT *array,
                                                      INSDS *ctx)
{
    CS_ARRAY_STORAGE *storage;
    ARRAYDAT clone = {0};
    size_t logicalCount, capacity, strideBytes;
    int32_t refs;

    if (array == NULL || array->storage == NULL) {
        return;
    }

    storage = array->storage;
    if (UNLIKELY(cs_array_storage_layout(array, storage, &logicalCount,
                                         &capacity, &strideBytes) != OK)) {
        csound->Die(csound, "invalid shared structured-array storage");
        return;
    }

    refs = cs_array_storage_ref_count(csound, storage);
    if (UNLIKELY(refs <= 0)) {
        csound->Die(csound, "invalid shared array reference count");
        return;
    }
    if (refs == 1 && cs_array_storage_try_claim(csound, storage) == OK) {
        /* The final view can discard the sidecar and resume direct ownership. */
        array->arrayMemberSize = storage->arrayMemberSize;
        array->arrayType = storage->arrayType;
        array->sizes = storage->sizes;
        array->data = storage->data;
        array->allocated = storage->allocated;
        array->storage = NULL;
        if (storage->refLock != NULL) {
            csound->DestroyMutex(storage->refLock);
        }
        csound->Free(csound, storage);
        return;
    }
    /* If the 1 -> 0 claim lost a race to a new reference, or another view
       released a 2 -> 1 reference after the load above, cloning is still
       valid: this ARRAYDAT continues to hold one live reference throughout. */
    if (UNLIKELY(cs_array_storage_clone(csound, array, storage, ctx,
                                        capacity, strideBytes,
                                        &clone) != OK)) {
        csound->Die(csound, "could not clone shared structured-array storage");
        return;
    }
    refs = cs_array_storage_release_ref(csound, storage);
    if (UNLIKELY(refs < 0)) {
        csound_array_free_elements_inline(csound, clone.arrayType, clone.data,
                                          clone.allocated,
                                          clone.arrayMemberSize);
        csound->Free(csound, clone.data);
        csound->Free(csound, clone.sizes);
        csound->Die(csound, "invalid shared array reference count");
        return;
    }
    /* Another independently locked view may release its reference while this
       one is being detached. The last releaser still owns final teardown. */
    if (refs == 0) {
        cs_array_storage_destroy_inline(csound, storage);
    }
    *array = clone;
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
           structured views can be detached safely before resizing. */
        if (array->allocated == 0 || array->arrayMemberSize <= 0 ||
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
    csound_array_prepare_write_inline(csound, p, ctx);
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
    csound_array_prepare_write_inline(csound, p, NULL);
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
    /* Updating the logical size is a write even when capacity is unchanged. */
    csound_array_prepare_write_inline(csound, p,
                                      q != NULL ? q->insdshead : NULL);
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
