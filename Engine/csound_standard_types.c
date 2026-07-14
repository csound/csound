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
 Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
 */

#include "csoundCore.h"
#include "csound_standard_types.h"
#include "arrays.h"
#include "arrays_internal.h"
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

/* The ownership implementation is private to the engine. Concurrent copies
   can atomically retain or install a sidecar; mutating the same ARRAYDAT still
   requires its usual channel or engine lock. Installed plugins see only the
   opaque pointer and detach callback in arrays.h. */
typedef struct cs_array_storage {
    int32_t refs;
    int32_t dimensions;
    int32_t arrayMemberSize;
    const CS_TYPE *arrayType;
    int32_t *sizes;
    MYFLT *data;
    size_t allocated;
} CS_ARRAY_STORAGE;

static int32_t cs_array_storage_ref_count(CSOUND *csound,
                                          CS_ARRAY_STORAGE *storage)
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
    if (UNLIKELY(csound->array_storage_lock == NULL)) {
        return 0;
    }
    csound->LockMutex(csound->array_storage_lock);
    refs = storage->refs;
    csound->UnlockMutex(csound->array_storage_lock);
    return refs;
#endif
}

static int32_t cs_array_storage_try_add_ref(CSOUND *csound,
                                            CS_ARRAY_STORAGE *storage)
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
    if (UNLIKELY(csound->array_storage_lock == NULL)) {
        return NOTOK;
    }
    csound->LockMutex(csound->array_storage_lock);
    if (storage->refs <= 0 || storage->refs == INT32_MAX) {
        csound->UnlockMutex(csound->array_storage_lock);
        return NOTOK;
    }
    refs = ++storage->refs;
    csound->UnlockMutex(csound->array_storage_lock);
    return refs;
#endif
}

static int32_t cs_array_storage_try_claim(CSOUND *csound,
                                          CS_ARRAY_STORAGE *storage)
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
    if (UNLIKELY(csound->array_storage_lock == NULL)) {
        return result;
    }
    csound->LockMutex(csound->array_storage_lock);
    if (storage->refs == 1) {
        storage->refs = 0;
        result = OK;
    }
    csound->UnlockMutex(csound->array_storage_lock);
    return result;
#endif
}

static int32_t cs_array_storage_release_ref(CSOUND *csound,
                                            CS_ARRAY_STORAGE *storage)
{
#if defined(MSVC)
    IGN(csound);
    return (int32_t)InterlockedDecrement((volatile LONG *)&storage->refs);
#elif defined(HAVE_ATOMIC_BUILTIN)
    IGN(csound);
    return __atomic_sub_fetch(&storage->refs, 1, __ATOMIC_ACQ_REL);
#else
    int32_t refs;
    if (UNLIKELY(csound->array_storage_lock == NULL)) {
        return NOTOK;
    }
    csound->LockMutex(csound->array_storage_lock);
    refs = --storage->refs;
    csound->UnlockMutex(csound->array_storage_lock);
    return refs;
#endif
}

static CS_ARRAY_STORAGE *cs_array_storage_load(CSOUND *csound,
                                               const ARRAYDAT *array)
{
#if defined(MSVC)
    IGN(csound);
    return (CS_ARRAY_STORAGE *)InterlockedCompareExchangePointer(
      (void * volatile *)&((ARRAYDAT *)array)->storage, NULL, NULL);
#elif defined(HAVE_ATOMIC_BUILTIN)
    IGN(csound);
    return __atomic_load_n(&array->storage, __ATOMIC_ACQUIRE);
#else
    CS_ARRAY_STORAGE *storage;
    if (UNLIKELY(csound->array_storage_lock == NULL)) {
        return NULL;
    }
    csound->LockMutex(csound->array_storage_lock);
    storage = (CS_ARRAY_STORAGE *)array->storage;
    csound->UnlockMutex(csound->array_storage_lock);
    return storage;
#endif
}

static CS_ARRAY_STORAGE *cs_array_storage_install(
    CSOUND *csound, ARRAYDAT *array, CS_ARRAY_STORAGE *candidate)
{
#if defined(MSVC)
    IGN(csound);
    CS_ARRAY_STORAGE *previous =
      (CS_ARRAY_STORAGE *)InterlockedCompareExchangePointer(
        (void * volatile *)&array->storage, candidate, NULL);
    return previous == NULL ? candidate : previous;
#elif defined(HAVE_ATOMIC_BUILTIN)
    IGN(csound);
    void *expected = NULL;
    if (__atomic_compare_exchange_n(&array->storage, &expected, candidate, 0,
                                    __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
        return candidate;
    }
    return (CS_ARRAY_STORAGE *)expected;
#else
    CS_ARRAY_STORAGE *storage;
    if (UNLIKELY(csound->array_storage_lock == NULL)) {
        return NULL;
    }
    csound->LockMutex(csound->array_storage_lock);
    if (array->storage == NULL) {
        array->storage = candidate;
    }
    storage = (CS_ARRAY_STORAGE *)array->storage;
    csound->UnlockMutex(csound->array_storage_lock);
    return storage;
#endif
}

static void csound_array_free_elements(CSOUND *csound,
                                       const CS_TYPE *arrayType,
                                       MYFLT *data, size_t allocated,
                                       int32_t memberSize)
{
    /* Managed allocation paths initialize or zero every capacity slot before
       increasing allocated. Empty arrays also own one initialized placeholder,
       so teardown follows capacity rather than the logical element count. */
    if (arrayType != NULL && arrayType->freeVariableMemory != NULL &&
        data != NULL && memberSize > 0) {
        size_t count = allocated / (size_t)memberSize;
        for (size_t i = 0; i < count; i++) {
            void *element = (char *)data + i * (size_t)memberSize;
            arrayType->freeVariableMemory(csound, element);
        }
    }
}

static void cs_array_storage_destroy(CSOUND *csound,
                                     CS_ARRAY_STORAGE *storage)
{
    if (storage == NULL) {
        return;
    }
    csound_array_free_elements(csound, storage->arrayType, storage->data,
                               storage->allocated,
                               storage->arrayMemberSize);
    csound->Free(csound, storage->data);
    csound->Free(csound, storage->sizes);
    csound->Free(csound, storage);
}

static int32_t cs_array_storage_layout(
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

static int32_t cs_array_storage_clone(
    CSOUND *csound, const ARRAYDAT *array, const CS_ARRAY_STORAGE *storage,
    INSDS *ctx, size_t capacity, size_t strideBytes, ARRAYDAT *clone)
{
    CS_VARIABLE *var;

    memset(clone, 0, sizeof(*clone));
    clone->dimensions = storage->dimensions;
    clone->arrayMemberSize = storage->arrayMemberSize;
    clone->arrayType = storage->arrayType;
    clone->allocated = storage->allocated;
    clone->sizes = (int32_t *)csound->Malloc(
      csound, sizeof(int32_t) * (size_t)clone->dimensions);
    memcpy(clone->sizes, array->sizes,
           sizeof(int32_t) * (size_t)clone->dimensions);
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

    for (size_t i = 0; i < capacity; i++) {
        size_t offset = i * strideBytes;
        clone->arrayType->copyValue(csound, clone->arrayType,
                                    (char *)clone->data + offset,
                                    (char *)storage->data + offset, ctx);
    }
    return OK;
}

size_t csound_array_allocated_bytes(CSOUND *csound, const ARRAYDAT *array)
{
    const CS_ARRAY_STORAGE *storage;

    if (array == NULL) {
        return 0;
    }
    storage = cs_array_storage_load(csound, array);
    return storage != NULL ? storage->allocated : array->allocated;
}

int32_t csound_array_storage_matches(CSOUND *csound,
                                     const ARRAYDAT *array)
{
    const CS_ARRAY_STORAGE *storage;

    if (array == NULL) {
        return 0;
    }
    storage = cs_array_storage_load(csound, array);
    if (storage == NULL) {
        return 1;
    }
    return storage->data == array->data &&
           storage->sizes == array->sizes &&
           storage->dimensions == array->dimensions &&
           storage->arrayType == array->arrayType &&
           storage->arrayMemberSize == array->arrayMemberSize &&
           (array->allocated == 0 ||
            storage->allocated == array->allocated);
}

static int32_t csound_array_share(CSOUND *csound, ARRAYDAT *destination,
                                  ARRAYDAT *source);

static void csound_array_copy_dimensions(CSOUND *csound,
                                         ARRAYDAT *destination,
                                         const ARRAYDAT *source)
{
    if (source->dimensions == 0) {
        csound->Free(csound, destination->sizes);
        destination->sizes = NULL;
    }
    else {
        destination->sizes = (int32_t *)csound->ReAlloc(
          csound, destination->sizes,
          sizeof(int32_t) * (size_t)source->dimensions);
        memcpy(destination->sizes, source->sizes,
               sizeof(int32_t) * (size_t)source->dimensions);
    }
    destination->dimensions = source->dimensions;
}

static int32_t csound_array_copy(CSOUND *csound, ARRAYDAT *destination,
                                 const ARRAYDAT *source, INSDS *ctx,
                                 int32_t shareStructured)
{
    CS_VARIABLE *var = NULL;
    size_t memberCount;
    size_t capacity = 0;
    size_t requiredBytes = 0;
    size_t sourceAllocated;
    int32_t needsAllocation;

    if (UNLIKELY(destination == NULL || source == NULL)) {
        return NOTOK;
    }
    if (destination == source) {
        return OK;
    }
    if (UNLIKELY(!csound_array_element_types_compatible(
                   destination->arrayType, source->arrayType))) {
        return NOTOK;
    }
    if (destination->arrayType == NULL) {
        destination->arrayType = source->arrayType;
    }
    if (UNLIKELY(csound_array_member_count(source, &memberCount) != OK ||
                 (memberCount > 0 && source->data == NULL))) {
        return NOTOK;
    }

    /* Legacy non-owning UDT views cannot safely acquire a sidecar because the
       actual owner is unknown. Copy those independently instead. */
    if (shareStructured && source->arrayType != NULL &&
        source->arrayType->userDefinedType &&
        (source->data == NULL ||
         cs_array_storage_load(csound, source) != NULL ||
         source->allocated > 0)) {
        return csound_array_share(csound, destination,
                                  (ARRAYDAT *)source);
    }

    if (source->data != NULL) {
        capacity = memberCount > 0 ? memberCount : 1;
        if (UNLIKELY(csound_array_allocation_size(
                       source->arrayMemberSize, capacity,
                       &requiredBytes) != OK)) {
            return NOTOK;
        }
        sourceAllocated = csound_array_allocated_bytes(csound, source);
        /* A zero value is the established legacy non-owning-view marker. */
        if (UNLIKELY(sourceAllocated > 0 &&
                     sourceAllocated < requiredBytes)) {
            return NOTOK;
        }
    }

    if (destination->storage != NULL &&
        csound_array_prepare_write_for_mode(csound, destination, ctx) != OK) {
        return NOTOK;
    }
    if (destination->data == source->data && destination->data != NULL &&
        destination->allocated > 0) {
        /* The destination already owns the allocation exposed by a legacy
           source view. The value is identical and must not be reallocated. */
        if (destination->sizes != source->sizes) {
            csound_array_copy_dimensions(csound, destination, source);
        }
        else {
            destination->dimensions = source->dimensions;
        }
        return OK;
    }

    needsAllocation = source->data != NULL &&
      (destination->data == NULL || destination->allocated == 0 ||
       destination->arrayMemberSize != source->arrayMemberSize ||
       requiredBytes > destination->allocated);

    if (needsAllocation) {
        var = array_element_create_variable(csound,
                                            destination->arrayType, ctx);
        if (UNLIKELY(var == NULL ||
                     (destination->arrayType->userDefinedType &&
                      var->initializeVariableMemory == NULL))) {
            if (var != NULL) {
                csound->Free(csound, var);
            }
            return NOTOK;
        }
        csound_free_array_storage(csound, destination);
        destination->arrayMemberSize = source->arrayMemberSize;
        csound_array_copy_dimensions(csound, destination, source);
        destination->allocated = requiredBytes;
        destination->data = (MYFLT *)csound->Calloc(csound, requiredBytes);
        if (var->initializeVariableMemory != NULL) {
            for (size_t i = 0; i < capacity; i++) {
                void *element = (char *)destination->data +
                  i * (size_t)destination->arrayMemberSize;
                var->initializeVariableMemory(csound, var, (MYFLT *)element);
            }
        }
        csound->Free(csound, var);
    }
    else {
        if (source->data == NULL) {
            csound_free_array_storage(csound, destination);
            destination->arrayMemberSize = source->arrayMemberSize;
        }
        csound_array_copy_dimensions(csound, destination, source);
    }

    for (size_t i = 0; i < memberCount; i++) {
        size_t offset = i * (size_t)source->arrayMemberSize;
        void *destinationElement = (char *)destination->data + offset;
        const void *sourceElement = (const char *)source->data + offset;

        if (destination->arrayType->copyValue != NULL) {
            destination->arrayType->copyValue(
              csound, destination->arrayType, destinationElement,
              sourceElement, ctx);
        }
        else {
            memcpy(destinationElement, sourceElement,
                   (size_t)source->arrayMemberSize);
        }
    }
    return OK;
}

int32_t csound_array_copy_independent(CSOUND *csound,
                                      ARRAYDAT *destination,
                                      const ARRAYDAT *source, INSDS *ctx)
{
    return csound_array_copy(csound, destination, source, ctx, 0);
}

/* Structured arrays share backing storage and detach on write. Other arrays
   retain their established deep-copy semantics. */
static void array_copy_value(CSOUND *csound, const CS_TYPE *cstype, void *dest,
                             const void *src, INSDS *ctx)
{
    IGN(cstype);
    if (UNLIKELY(csound_array_copy(csound, (ARRAYDAT *)dest,
                                   (const ARRAYDAT *)src, ctx, 1) != OK)) {
        csound->ErrorMsg(csound, "%s\n", Str("array copy failed"));
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
    dat->storage = NULL;

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

static void array_clear_view(ARRAYDAT* dat) {
    /* Keep arrayType: it is the variable's declared element type and is needed
       if the same ARRAYDAT is initialized again. */
    dat->dimensions = 0;
    dat->sizes = NULL;
    dat->arrayMemberSize = 0;
    dat->data = NULL;
    dat->allocated = 0;
    dat->storage = NULL;
}

void csound_free_array_storage(CSOUND* csound, ARRAYDAT* dat) {
    int32_t ownsData;

    if (dat == NULL) {
        return;
    }

    if (dat->storage != NULL) {
        CS_ARRAY_STORAGE* storage = (CS_ARRAY_STORAGE *)dat->storage;
        int32_t refs = cs_array_storage_release_ref(csound, storage);
        if (UNLIKELY(refs < 0)) {
            csound->ErrorMsg(csound, "%s\n",
                             Str("negative shared array reference count"));
            array_clear_view(dat);
            return;
        }
        if (refs == 0) {
            cs_array_storage_destroy(csound, storage);
        }
        array_clear_view(dat);
        return;
    }

    /* Aliases have data but allocated == 0. An empty array still owns its
       one-element placeholder, so allocated remains greater than zero. */
    ownsData = dat->allocated > 0 && dat->data != NULL;
    if (ownsData) {
        csound_array_free_elements(csound, dat->arrayType, dat->data,
                                   dat->allocated, dat->arrayMemberSize);
        csound->Free(csound, dat->data);
    }

    /* An uninitialised array shell owns its sizes metadata. Aliases own
       neither sizes nor data, and must only be detached. */
    if (dat->sizes != NULL && (ownsData || dat->data == NULL)) {
        csound->Free(csound, dat->sizes);
    }

    array_clear_view(dat);
}

static int32_t csound_array_share(CSOUND *csound, ARRAYDAT *dest,
                                  ARRAYDAT *src)
{
    CS_ARRAY_STORAGE *storage;
    size_t logicalCount, capacity, strideBytes;

    if (dest == NULL || src == NULL || dest == src) {
        return dest == src ? OK : NOTOK;
    }
    if (UNLIKELY(src->arrayType == NULL ||
                 !src->arrayType->userDefinedType ||
                 csound_array_member_count(src, &logicalCount) != OK)) {
        return NOTOK;
    }
    storage = cs_array_storage_load(csound, src);
    if (dest->data != NULL && dest->data == src->data &&
        dest->storage != NULL && dest->storage == storage) {
        return OK;
    }
    if (src->data == NULL) {
        if (UNLIKELY(storage != NULL)) {
            return NOTOK;
        }
        csound_free_array_storage(csound, dest);
        dest->arrayMemberSize = src->arrayMemberSize;
        dest->arrayType = src->arrayType;
        csound_array_copy_dimensions(csound, dest, src);
        return OK;
    }

    if (storage == NULL) {
        CS_ARRAY_STORAGE *candidate;
        CS_ARRAY_STORAGE *installed;

        /* A raw alias (allocated == 0) has no owner that can participate in
           reference counting. Internal structured-array copies now always
           carry a sidecar, so accepting such a source would risk freeing
           memory owned elsewhere. */
        if (UNLIKELY(src->allocated == 0 || src->arrayMemberSize <= 0 ||
                     src->allocated % (size_t)src->arrayMemberSize != 0)) {
            return NOTOK;
        }

        candidate = (CS_ARRAY_STORAGE *)csound->Calloc(
          csound, sizeof(CS_ARRAY_STORAGE));
        candidate->refs = 1;
        candidate->dimensions = src->dimensions;
        candidate->arrayMemberSize = src->arrayMemberSize;
        candidate->arrayType = src->arrayType;
        candidate->sizes = src->sizes;
        candidate->data = src->data;
        candidate->allocated = src->allocated;
        if (UNLIKELY(cs_array_storage_layout(
                       src, candidate, &logicalCount,
                       &capacity, &strideBytes) != OK)) {
            csound->Free(csound, candidate);
            return NOTOK;
        }

        /* Multiple readers may first copy the same source concurrently. The
           winner publishes one sidecar; losers discard only their sidecar
           object because the data and sizes still belong to the winner. */
        installed = cs_array_storage_install(csound, src, candidate);
        if (UNLIKELY(installed == NULL)) {
            csound->Free(csound, candidate);
            return NOTOK;
        }
        if (installed != candidate) {
            csound->Free(csound, candidate);
        }
        storage = installed;
    }
    if (UNLIKELY(cs_array_storage_layout(
                   src, storage, &logicalCount,
                   &capacity, &strideBytes) != OK)) {
        return NOTOK;
    }

    /* Validate and retain the source before releasing the destination. This
       matters for legacy aliases that may point at destination-owned data. */
    if (UNLIKELY(dest->data == src->data && dest->storage != storage &&
                 (dest->storage != NULL || dest->allocated > 0))) {
        return NOTOK;
    }
    if (UNLIKELY(cs_array_storage_try_add_ref(csound, storage) == NOTOK)) {
        return NOTOK;
    }
    csound_free_array_storage(csound, dest);

    dest->dimensions = src->dimensions;
    dest->sizes = src->sizes;
    dest->arrayMemberSize = src->arrayMemberSize;
    dest->arrayType = src->arrayType;
    dest->data = src->data;
    dest->allocated = 0;
    dest->storage = storage;
    return OK;
}

int32_t csound_array_prepare_write_impl(CSOUND *csound, ARRAYDAT *array,
                                        INSDS *ctx, int32_t allowAllocation)
{
    CS_ARRAY_STORAGE *storage;
    ARRAYDAT clone = {0};
    size_t logicalCount, capacity, strideBytes;
    int32_t refs;

    if (array == NULL) {
        return NOTOK;
    }
    if (array->storage == NULL) {
        return OK;
    }

    storage = cs_array_storage_load(csound, array);
    if (UNLIKELY(storage == NULL ||
                 cs_array_storage_layout(array, storage, &logicalCount,
                                         &capacity, &strideBytes) != OK)) {
        return NOTOK;
    }
    refs = cs_array_storage_ref_count(csound, storage);
    if (UNLIKELY(refs <= 0)) {
        return NOTOK;
    }
    /* A successful 1 -> 0 claim transfers the allocation from the sidecar
       back to the final view. A concurrent retain makes the claim fail and
       takes the clone path instead. */
    if (refs == 1 && cs_array_storage_try_claim(csound, storage) == OK) {
        array->dimensions = storage->dimensions;
        array->arrayMemberSize = storage->arrayMemberSize;
        array->arrayType = storage->arrayType;
        array->sizes = storage->sizes;
        array->data = storage->data;
        array->allocated = storage->allocated;
        array->storage = NULL;
        csound->Free(csound, storage);
        return OK;
    }
    if (!allowAllocation) {
        return NOTOK;
    }
    if (UNLIKELY(cs_array_storage_clone(csound, array, storage, ctx,
                                        capacity, strideBytes,
                                        &clone) != OK)) {
        return NOTOK;
    }
    refs = cs_array_storage_release_ref(csound, storage);
    if (UNLIKELY(refs < 0)) {
        csound_array_free_elements(csound, clone.arrayType, clone.data,
                                   clone.allocated,
                                   clone.arrayMemberSize);
        csound->Free(csound, clone.data);
        csound->Free(csound, clone.sizes);
        return NOTOK;
    }
    if (refs == 0) {
        cs_array_storage_destroy(csound, storage);
    }
    *array = clone;
    return OK;
}

static void array_free_var_mem(void* csnd, void* p) {
    csound_free_array_storage((CSOUND*)csnd, (ARRAYDAT*)p);
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
    "K", "cprki",               /* k-rate with initialization */
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
    "i", "pi",
    "K", "cprki",               /* k-rate with initialization */
    NULL
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
