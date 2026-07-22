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
#include "csound_orc_structs.h"
#include "pstream.h"
#include "find_opcode.h"
#include <stdlib.h>

/* MEMORY COPYING FUNCTIONS */
static void myflt_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                      const void* src, INSDS *ctx) {
  if (UNLIKELY(src == NULL || dest == NULL)) return;
  memcpy(dest, src, sizeof(MYFLT));
}

static void asig_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                     const void* src, INSDS *ctx) {
  if (UNLIKELY(src == NULL || dest == NULL)) return;
  int32_t ksmps = ctx ? ctx->ksmps : csound->ksmps;
  memcpy(dest, src, sizeof(MYFLT) * ksmps);
}

static void complex_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                        const void* src, INSDS *ctx) {
  if (UNLIKELY(src == NULL || dest == NULL)) return;
  memcpy(dest, src, sizeof(COMPLEXDAT));
}

static void wsig_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                     const void* src, INSDS *ctx) {
  if (UNLIKELY(src == NULL || dest == NULL)) return;
  memcpy(dest, src, sizeof(SPECDAT));
  //TODO - check if this needs to copy SPECDAT's DOWNDAT member and AUXCH
}

static void fsig_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                     const void* src, INSDS *ctx) {
  if (UNLIKELY(src == NULL || dest == NULL)) return;
  PVSDAT *fsigout = (PVSDAT*) dest;
  const PVSDAT *fsigin = (const PVSDAT*) src;
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
    if (UNLIKELY(src == NULL || dest == NULL)) return;
    STRINGDAT* sDest = (STRINGDAT*)dest;
    const STRINGDAT* sSrc = (const STRINGDAT*)src;

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

/* The ownership implementation is private to the engine. A short-held lock
   makes sidecar discovery and retention one operation; allocation and element
   copying never run while that lock is held. Mutating the same ARRAYDAT still
   requires its usual channel or engine lock. */
typedef struct cs_array_storage {
    int32_t refs;
    int32_t dimensions;
    int32_t arrayMemberSize;
    const CS_TYPE *arrayType;
    int32_t *sizes;
    MYFLT *data;
    size_t allocated;
} CS_ARRAY_STORAGE;

static CS_ARRAY_STORAGE cs_array_storage_write_owned;

/* Keep this capability test aligned with Top/threads.c. */
#if defined(MSVC) || defined(MACOSX) || \
    (defined(__GNUC__) && \
     (defined(HAVE_PTHREAD_SPIN_LOCK) || defined(HAVE_ATOMIC_BUILTIN)))
#define CS_ARRAY_STORAGE_USES_SPINLOCK 1
#else
#define CS_ARRAY_STORAGE_USES_SPINLOCK 0
#endif

static int32_t cs_array_storage_protocol_lock(CSOUND *csound)
{
#if CS_ARRAY_STORAGE_USES_SPINLOCK
    csoundSpinLock(&csound->array_storage_spinlock);
    return OK;
#else
    if (UNLIKELY(csound->array_storage_lock == NULL)) {
        return NOTOK;
    }
    csound->LockMutex(csound->array_storage_lock);
    return OK;
#endif
}

static void cs_array_storage_protocol_unlock(CSOUND *csound)
{
#if CS_ARRAY_STORAGE_USES_SPINLOCK
    csoundSpinUnLock(&csound->array_storage_spinlock);
#else
    csound->UnlockMutex(csound->array_storage_lock);
#endif
}

static int32_t cs_array_storage_is_write_owned(
    const CS_ARRAY_STORAGE *storage)
{
    return storage == &cs_array_storage_write_owned;
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
    size_t allocated;

    if (array == NULL) {
        return 0;
    }
    /* Owners retain their capacity in ARRAYDAT even after a sidecar is
       installed, so the common path does not need the protocol lock. */
    if (array->allocated > 0) {
        return array->allocated;
    }
    if (UNLIKELY(cs_array_storage_protocol_lock(csound) != OK)) {
        return 0;
    }
    storage = (const CS_ARRAY_STORAGE *)array->storage;
    allocated = storage != NULL &&
      !cs_array_storage_is_write_owned(storage) ? storage->allocated : 0;
    cs_array_storage_protocol_unlock(csound);
    return allocated;
}

int32_t csound_array_storage_matches(CSOUND *csound,
                                     const ARRAYDAT *array)
{
    const CS_ARRAY_STORAGE *storage;
    int32_t matches;

    if (array == NULL) {
        return 0;
    }
    if (UNLIKELY(cs_array_storage_protocol_lock(csound) != OK)) {
        return 0;
    }
    storage = (const CS_ARRAY_STORAGE *)array->storage;
    if (storage == NULL || cs_array_storage_is_write_owned(storage)) {
        cs_array_storage_protocol_unlock(csound);
        return 1;
    }
    matches = storage->data == array->data &&
      storage->sizes == array->sizes &&
      storage->dimensions == array->dimensions &&
      storage->arrayType == array->arrayType &&
      storage->arrayMemberSize == array->arrayMemberSize &&
      (array->allocated == 0 || storage->allocated == array->allocated);
    cs_array_storage_protocol_unlock(csound);
    return matches;
}

static int32_t csound_array_values_alias(CSOUND *csound,
                                         const ARRAYDAT *left,
                                         const ARRAYDAT *right)
{
    int32_t aliases;

    if (UNLIKELY(cs_array_storage_protocol_lock(csound) != OK)) {
        return 0;
    }
    aliases = left->data != NULL && left->data == right->data &&
      left->sizes == right->sizes &&
      left->dimensions == right->dimensions &&
      left->arrayType == right->arrayType &&
      left->arrayMemberSize == right->arrayMemberSize;
    cs_array_storage_protocol_unlock(csound);
    return aliases;
}

static int32_t csound_array_share(CSOUND *csound, ARRAYDAT *destination,
                                  ARRAYDAT *source);

enum { CS_ARRAY_SHARE_INDEPENDENT = 1 };

static int32_t csound_array_copy_dimensions(CSOUND *csound,
                                            ARRAYDAT *destination,
                                            const ARRAYDAT *source,
                                            int32_t allowAllocation)
{
    if (source->dimensions == 0) {
        if (!allowAllocation && (destination->dimensions != 0 ||
                                 destination->sizes != NULL)) {
            return NOTOK;
        }
        csound->Free(csound, destination->sizes);
        destination->sizes = NULL;
    }
    else if (!allowAllocation) {
        if (destination->dimensions != source->dimensions ||
            destination->sizes == NULL) {
            return NOTOK;
        }
        memcpy(destination->sizes, source->sizes,
               sizeof(int32_t) * (size_t)source->dimensions);
    }
    else {
        destination->sizes = (int32_t *)csound->ReAlloc(
          csound, destination->sizes,
          sizeof(int32_t) * (size_t)source->dimensions);
        memcpy(destination->sizes, source->sizes,
               sizeof(int32_t) * (size_t)source->dimensions);
    }
    destination->dimensions = source->dimensions;
    return OK;
}

static int32_t csound_array_copy(CSOUND *csound, ARRAYDAT *destination,
                                 const ARRAYDAT *source, INSDS *ctx,
                                 int32_t shareStructured,
                                 int32_t allowAllocation)
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

    /* A performance-owned or legacy UDT source is copied independently. */
    if (shareStructured && source->arrayType != NULL &&
        source->arrayType->userDefinedType) {
        int32_t shareResult = csound_array_share(
          csound, destination, (ARRAYDAT *)source);
        if (shareResult == OK) {
            return OK;
        }
        if (shareResult != CS_ARRAY_SHARE_INDEPENDENT) {
            return NOTOK;
        }
        shareStructured = 0;
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

    /* Independently copying two views of the same logical value is a no-op.
       In particular, do not detach one side of a shared nested array merely
       to copy it back onto the other side. */
    if (csound_array_values_alias(csound, destination, source)) {
        return OK;
    }

    {
      int32_t result = allowAllocation
        ? csound_array_prepare_write(csound, destination, ctx)
        : csound_array_try_prepare_write(csound, destination, ctx);
      if (result != OK) {
          return NOTOK;
      }
    }
    if (!allowAllocation && memberCount == 0) {
        if (UNLIKELY(csound_array_copy_dimensions(
                       csound, destination, source, 0) != OK)) {
            return NOTOK;
        }
        destination->arrayMemberSize = source->arrayMemberSize;
        destination->arrayType = source->arrayType;
        return OK;
    }
    if (destination->data == source->data && destination->data != NULL &&
        destination->allocated > 0) {
        /* The destination already owns the allocation exposed by a legacy
           source view. The value is identical and must not be reallocated. */
        if (destination->sizes != source->sizes) {
            if (UNLIKELY(csound_array_copy_dimensions(
                           csound, destination, source,
                           allowAllocation) != OK)) {
                return NOTOK;
            }
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

    if (UNLIKELY(needsAllocation && !allowAllocation)) {
        return NOTOK;
    }
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
        if (UNLIKELY(csound_array_copy_dimensions(
                       csound, destination, source, 1) != OK)) {
            csound->Free(csound, var);
            return NOTOK;
        }
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
            if (UNLIKELY(!allowAllocation && destination->data != NULL)) {
                return NOTOK;
            }
            csound_free_array_storage(csound, destination);
            destination->arrayMemberSize = source->arrayMemberSize;
        }
        if (UNLIKELY(csound_array_copy_dimensions(
                       csound, destination, source,
                       allowAllocation) != OK)) {
            return NOTOK;
        }
    }

    for (size_t i = 0; i < memberCount; i++) {
        size_t offset = i * (size_t)source->arrayMemberSize;
        void *destinationElement = (char *)destination->data + offset;
        const void *sourceElement = (const char *)source->data + offset;

        if (!shareStructured &&
            destination->arrayType->userDefinedType) {
            if (csound_copy_struct_value(
                  csound, destination->arrayType, destinationElement,
                  sourceElement, ctx,
                  allowAllocation
                    ? CSOUND_STRUCT_COPY_INDEPENDENT_ALLOW_ALLOCATION
                    : CSOUND_STRUCT_COPY_INDEPENDENT_NO_ALLOCATION) != OK) {
                return NOTOK;
            }
        }
        else if (destination->arrayType->copyValue != NULL) {
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
                                      const ARRAYDAT *source, INSDS *ctx,
                                      CSOUND_ARRAY_COPY_MODE mode)
{
    return csound_array_copy(csound, destination, source, ctx, 0,
                             mode == CSOUND_ARRAY_COPY_ALLOW_ALLOCATION);
}

/* Structured arrays share backing storage and detach on write. Other arrays
   retain their established deep-copy semantics. */
static void array_copy_value(CSOUND *csound, const CS_TYPE *cstype, void *dest,
                             const void *src, INSDS *ctx)
{
    if (UNLIKELY(src == NULL || dest == NULL)) return;
    IGN(cstype);
    if (UNLIKELY(csound_array_copy(csound, (ARRAYDAT *)dest,
                                   (const ARRAYDAT *)src, ctx, 1, 1) != OK)) {
        csound->ErrorMsg(csound, "%s\n", Str("array copy failed"));
    }
}

static void opcodedef_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                      const void* src, INSDS *ctx) {
  if (UNLIKELY(src == NULL || dest == NULL)) return;
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
  if (UNLIKELY(src == NULL || dest == NULL)) return;
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
  if (UNLIKELY(src == NULL || dest == NULL)) return;
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
  if (UNLIKELY(src == NULL || dest == NULL)) return;
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

static CS_VARIABLE* create_asig(void* cs, const CS_TYPE *type,
                                const void *typeArg, INSDS *ctx) {
    int32_t ksmps;
    CSOUND* csound = (CSOUND*)cs;
    IGN(type);
    IGN(typeArg);

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

static CS_VARIABLE* create_myflt(void* cs, const CS_TYPE *type,
                                 const void *typeArg, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    IGN(type);
    IGN(typeArg);
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof (MYFLT));
    var->initializeVariableMemory = &var_init_memory;
    var->ctx = ctx;
    return var;
}

static CS_VARIABLE* create_complex(void* cs, const CS_TYPE *type,
                                   const void *typeArg, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    IGN(type);
    IGN(typeArg);
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(COMPLEXDAT));
    var->initializeVariableMemory = &var_init_memory;
    var->ctx = ctx;
    return var;
}

static CS_VARIABLE* create_bool(void* cs, const CS_TYPE *type,
                                const void *typeArg, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    IGN(type);
    IGN(typeArg);
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(int32_t));
    var->initializeVariableMemory = &var_init_memory;
    var->ctx = ctx;
    return var;
}

static CS_VARIABLE* create_wsig(void* cs, const CS_TYPE *type,
                                const void *typeArg, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    IGN(type);
    IGN(typeArg);
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(SPECDAT));
    var->initializeVariableMemory = &var_init_memory;
    var->ctx = ctx;
    return var;
}

static CS_VARIABLE* create_fsig(void* cs, const CS_TYPE *type,
                                const void *typeArg, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    IGN(type);
    IGN(typeArg);
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(PVSDAT));
    var->initializeVariableMemory = &var_init_memory_fsig;
    var->ctx = ctx;
    return var;
}

static CS_VARIABLE* create_string(void* cs, const CS_TYPE *type,
                                  const void *typeArg, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)cs;
    IGN(type);
    IGN(typeArg);
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(STRINGDAT));
    var->initializeVariableMemory = &var_init_memory_string;
    var->ctx = ctx;
    return var;
}

CS_VARIABLE* create_array(void* csnd, const CS_TYPE *type,
                          const void *typeArg, INSDS *ctx) {
    CSOUND* csound = (CSOUND*)csnd;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(type);

    // typeArg, when present, contains array-specific initialization data
    if(typeArg) {
      const ARRAY_VAR_INIT* state = (const ARRAY_VAR_INIT*) typeArg;
      const CS_TYPE* elementType = state->type;
      var->subType = elementType;
      var->dimensions = state->dimensions;
    }

    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(ARRAYDAT));
    var->initializeVariableMemory = &array_init_memory;
    var->ctx = ctx;
    return var;
}


static CS_VARIABLE* create_opcodedef(void* csnd, const CS_TYPE *type,
                                     const void *typeArg, INSDS *ctx) {
   CSOUND* csound = (CSOUND*)csnd;
   IGN(type);
   IGN(typeArg);
   CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
   var->memBlockSize = CS_FLOAT_ALIGN(sizeof(OPCODEREF));
   var->initializeVariableMemory = &var_init_memory;
   var->ctx = ctx;
   return var;
}

static CS_VARIABLE* create_opcode(void* csnd, const CS_TYPE *type,
                                  const void *typeArg, INSDS *ctx) {
   CSOUND* csound = (CSOUND*)csnd;
   IGN(type);
   IGN(typeArg);
   CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
   var->memBlockSize = CS_FLOAT_ALIGN(sizeof(OPCODEOBJ));
   var->initializeVariableMemory = &var_init_memory;
   var->ctx = ctx;
   return var;
}

static CS_VARIABLE* create_instrdef(void* csnd, const CS_TYPE *type,
                                    const void *typeArg, INSDS *ctx) {
   CSOUND* csound = (CSOUND*)csnd;
   IGN(type);
   IGN(typeArg);
   CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
   var->memBlockSize = CS_FLOAT_ALIGN(sizeof(INSTREF));
   var->initializeVariableMemory = &var_init_memory;
   var->ctx = ctx;
   return var;
}

static CS_VARIABLE* create_instr(void* csnd, const CS_TYPE *type,
                                 const void *typeArg, INSDS *ctx) {
   CSOUND* csound = (CSOUND*)csnd;
   IGN(type);
   IGN(typeArg);
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
    ARRAYDAT released = {0};
    CS_ARRAY_STORAGE *storage;
    int32_t destroyStorage = 0;
    int32_t ownsData;

    if (dat == NULL) {
        return;
    }
    if (UNLIKELY(cs_array_storage_protocol_lock(csound) != OK)) {
        return;
    }
    storage = (CS_ARRAY_STORAGE *)dat->storage;
    if (storage != NULL && !cs_array_storage_is_write_owned(storage)) {
        if (UNLIKELY(storage->refs <= 0)) {
            cs_array_storage_protocol_unlock(csound);
            csound->ErrorMsg(csound, "%s\n",
                             Str("invalid shared array reference count"));
            return;
        }
        destroyStorage = --storage->refs == 0;
        array_clear_view(dat);
        cs_array_storage_protocol_unlock(csound);
        if (destroyStorage) {
            cs_array_storage_destroy(csound, storage);
        }
        return;
    }

    released = *dat;
    released.storage = NULL;
    array_clear_view(dat);
    cs_array_storage_protocol_unlock(csound);

    /* Aliases have data but allocated == 0. An empty array still owns its
       one-element placeholder, so allocated remains greater than zero. */
    ownsData = released.allocated > 0 && released.data != NULL;
    if (ownsData) {
        csound_array_free_elements(csound, released.arrayType, released.data,
                                   released.allocated,
                                   released.arrayMemberSize);
        csound->Free(csound, released.data);
    }

    /* An uninitialised array shell owns its sizes metadata. Aliases own
       neither sizes nor data, and must only be detached. */
    if (released.sizes != NULL && (ownsData || released.data == NULL)) {
        csound->Free(csound, released.sizes);
    }
}

static int32_t csound_array_share(CSOUND *csound, ARRAYDAT *dest,
                                  ARRAYDAT *src)
{
    CS_ARRAY_STORAGE *candidate = NULL;
    CS_ARRAY_STORAGE *storage;
    size_t logicalCount, capacity, strideBytes;
    int32_t alreadyShared = 0;
    int32_t result = NOTOK;

    if (dest == NULL || src == NULL || dest == src) {
        return dest == src ? OK : NOTOK;
    }
retry:
    if (UNLIKELY(cs_array_storage_protocol_lock(csound) != OK)) {
        csound->Free(csound, candidate);
        return NOTOK;
    }
    storage = (CS_ARRAY_STORAGE *)src->storage;
    if (UNLIKELY(src->arrayType == NULL ||
                 !src->arrayType->userDefinedType)) {
        goto unlock;
    }
    if (cs_array_storage_is_write_owned(storage) || src->data == NULL) {
        result = CS_ARRAY_SHARE_INDEPENDENT;
        goto unlock;
    }
    if (dest->data != NULL && dest->data == src->data &&
        dest->storage != NULL && dest->storage == storage) {
        alreadyShared = 1;
        result = OK;
        goto unlock;
    }

    if (storage == NULL) {
        /* A raw alias (allocated == 0) has no owner that can participate in
           reference counting. Copy it independently instead. */
        if (UNLIKELY(src->allocated == 0 || src->arrayMemberSize <= 0 ||
                     src->allocated % (size_t)src->arrayMemberSize != 0)) {
            result = CS_ARRAY_SHARE_INDEPENDENT;
            goto unlock;
        }
        if (candidate == NULL) {
            cs_array_storage_protocol_unlock(csound);
            candidate = (CS_ARRAY_STORAGE *)csound->Calloc(
              csound, sizeof(CS_ARRAY_STORAGE));
            if (UNLIKELY(candidate == NULL)) {
                return NOTOK;
            }
            goto retry;
        }
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
            goto unlock;
        }
        src->storage = candidate;
        storage = candidate;
        candidate = NULL;
    }
    if (UNLIKELY(cs_array_storage_layout(
                   src, storage, &logicalCount,
                   &capacity, &strideBytes) != OK)) {
        goto unlock;
    }

    /* Validate and retain the source before releasing the destination. This
       matters for legacy aliases that may point at destination-owned data. */
    if (UNLIKELY(dest->data == src->data && dest->storage != storage &&
                 (dest->storage != NULL || dest->allocated > 0))) {
        goto unlock;
    }
    if (UNLIKELY(storage->refs <= 0 || storage->refs == INT32_MAX)) {
        goto unlock;
    }
    storage->refs++;
    result = OK;

unlock:
    cs_array_storage_protocol_unlock(csound);
    csound->Free(csound, candidate);
    if (result != OK) {
        return result;
    }
    if (alreadyShared) {
        return OK;
    }
    csound_free_array_storage(csound, dest);

    dest->dimensions = storage->dimensions;
    dest->sizes = storage->sizes;
    dest->arrayMemberSize = storage->arrayMemberSize;
    dest->arrayType = storage->arrayType;
    dest->data = storage->data;
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
    int32_t destroyStorage = 0;

    if (array == NULL) {
        return NOTOK;
    }
    if (UNLIKELY(cs_array_storage_protocol_lock(csound) != OK)) {
        return NOTOK;
    }
    storage = (CS_ARRAY_STORAGE *)array->storage;
    if (storage == NULL) {
        /* Once a performance writer is admitted, later copies must not install
           a sidecar over storage that is about to change. */
        if (!allowAllocation && array->data != NULL &&
            array->arrayType != NULL &&
            array->arrayType->userDefinedType) {
            array->storage = &cs_array_storage_write_owned;
        }
        cs_array_storage_protocol_unlock(csound);
        return OK;
    }
    if (cs_array_storage_is_write_owned(storage)) {
        cs_array_storage_protocol_unlock(csound);
        return OK;
    }
    if (UNLIKELY(cs_array_storage_layout(
                   array, storage, &logicalCount,
                   &capacity, &strideBytes) != OK || storage->refs <= 0)) {
        cs_array_storage_protocol_unlock(csound);
        return NOTOK;
    }
    if (storage->refs == 1) {
        storage->refs = 0;
        array->dimensions = storage->dimensions;
        array->arrayMemberSize = storage->arrayMemberSize;
        array->arrayType = storage->arrayType;
        array->sizes = storage->sizes;
        array->data = storage->data;
        array->allocated = storage->allocated;
        array->storage = allowAllocation
          ? NULL : &cs_array_storage_write_owned;
        cs_array_storage_protocol_unlock(csound);
        csound->Free(csound, storage);
        return OK;
    }
    if (!allowAllocation) {
        cs_array_storage_protocol_unlock(csound);
        return NOTOK;
    }
    /* This ARRAYDAT's reference keeps storage alive while the clone is made.
       Other writers must detach before changing the shared data. */
    cs_array_storage_protocol_unlock(csound);
    if (UNLIKELY(cs_array_storage_clone(csound, array, storage, ctx,
                                        capacity, strideBytes,
                                        &clone) != OK)) {
        return NOTOK;
    }
    if (UNLIKELY(cs_array_storage_protocol_lock(csound) != OK)) {
        csound_array_free_elements(csound, clone.arrayType, clone.data,
                                   clone.allocated,
                                   clone.arrayMemberSize);
        csound->Free(csound, clone.data);
        csound->Free(csound, clone.sizes);
        return NOTOK;
    }
    if (UNLIKELY(array->storage != storage || storage->refs <= 0)) {
        cs_array_storage_protocol_unlock(csound);
        csound_array_free_elements(csound, clone.arrayType, clone.data,
                                   clone.allocated,
                                   clone.arrayMemberSize);
        csound->Free(csound, clone.data);
        csound->Free(csound, clone.sizes);
        return NOTOK;
    }
    destroyStorage = --storage->refs == 0;
    *array = clone;
    cs_array_storage_protocol_unlock(csound);
    if (destroyStorage) {
        cs_array_storage_destroy(csound, storage);
    }
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
