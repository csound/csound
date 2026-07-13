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

/* Structured arrays share backing storage and detach on write. Other arrays
   retain their established deep-copy semantics; they do not retain recursive
   struct graphs and do not need ownership sharing for this lifetime fix. */
static void array_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                      const void* src, INSDS *ctx) {
    ARRAYDAT* aDest = (ARRAYDAT*)dest;
    /* The type callback declares src const. Structured copies may attach an
       ownership sidecar, which changes bookkeeping but not the source value. */
    ARRAYDAT* aSrc = (ARRAYDAT*)src;
    CS_VARIABLE* var = NULL;
    size_t arrayNumMembers, capacity, requiredBytes = 0;
    int32_t needsAllocation;

    IGN(cstype);

    if (UNLIKELY(aDest == NULL || aSrc == NULL) || aDest == aSrc) {
        return;
    }
    if (UNLIKELY(!csound_array_element_types_compatible(
                   aDest->arrayType, aSrc->arrayType))) {
        csound->Die(csound, "array element types do not match during copy");
        return;
    }
    if (aDest->arrayType == NULL) {
        aDest->arrayType = aSrc->arrayType;
    }
    /* Structured arrays share backing storage until a writer detaches. */
    if (aSrc->arrayType != NULL && aSrc->arrayType->userDefinedType) {
        csound_array_share(csound, aDest, aSrc);
        return;
    }
    if (aDest->data != NULL && aDest->data == aSrc->data) {
        /* A legacy view can reach this callback with a different ARRAYDAT but
           the same allocation. Reallocating the destination would invalidate
           the source, while the value is already identical. */
        return;
    }

    if (UNLIKELY(csound_array_member_count(aSrc, &arrayNumMembers) != OK)) {
        csound->Die(csound, "invalid array dimensions during copy");
        return;
    }
    if (UNLIKELY(arrayNumMembers > 0 && aSrc->data == NULL)) {
        csound->Die(csound, "array data is missing during copy");
        return;
    }
    capacity = arrayNumMembers > 0 ? arrayNumMembers : 1;
    if (aSrc->data != NULL) {
        if (UNLIKELY(csound_array_allocation_size(
                       aSrc->arrayMemberSize, capacity,
                       &requiredBytes) != OK)) {
            csound->Die(csound, "array allocation size overflow during copy");
            return;
        }
        /* allocated == 0 is the legacy non-owning-view marker. Owners must
           report enough capacity for every logical element copied below. */
        if (UNLIKELY(aSrc->allocated > 0 &&
                     aSrc->allocated < requiredBytes)) {
            csound->Die(csound, "array source capacity is too small");
            return;
        }
    }

    /* Use exact capacity so shrinking a managed array releases trailing
       element storage instead of retaining it until instrument teardown. */
    needsAllocation = aDest->storage != NULL || aDest->data == NULL ||
      aSrc->arrayMemberSize != aDest->arrayMemberSize ||
      aSrc->dimensions != aDest->dimensions ||
      (aSrc->dimensions > 0 && aDest->sizes == NULL) ||
      requiredBytes != aDest->allocated;

    if (needsAllocation) {
        csound_free_array_storage(csound, aDest);
        aDest->arrayMemberSize = aSrc->arrayMemberSize;
        aDest->dimensions = aSrc->dimensions;
        if (aSrc->dimensions > 0 && aSrc->sizes != NULL) {
            aDest->sizes = csound->Malloc(
              csound, sizeof(int32_t) * (size_t)aSrc->dimensions);
            memcpy(aDest->sizes, aSrc->sizes,
                   sizeof(int32_t) * (size_t)aSrc->dimensions);
        }

        if (aSrc->data == NULL || aSrc->arrayMemberSize <= 0) {
            return;
        }
        aDest->allocated = requiredBytes;
        aDest->data = csound->Calloc(csound, aDest->allocated);

        var = array_element_create_variable(csound, aDest->arrayType, ctx);
        if (var != NULL && var->initializeVariableMemory != NULL) {
            for (size_t i = 0; i < capacity; i++) {
                var->initializeVariableMemory(csound, var,
                  (MYFLT*)((char*)aDest->data +
                           i * (size_t)aDest->arrayMemberSize));
            }
        }
    } else if (aDest->dimensions > 0) {
        memcpy(aDest->sizes, aSrc->sizes,
               sizeof(int32_t) * (size_t)aDest->dimensions);
    }

    for (size_t i = 0; i < arrayNumMembers; i++) {
        size_t offset = i * (size_t)aSrc->arrayMemberSize;
        void* destElement = (char*)aDest->data + offset;
        const void* srcElement = (const char*)aSrc->data + offset;
        if (aDest->arrayType != NULL &&
            aDest->arrayType->copyValue != NULL) {
            aDest->arrayType->copyValue(csound, aDest->arrayType,
                                        destElement, srcElement, ctx);
        } else {
            memcpy(destElement, srcElement, (size_t)aSrc->arrayMemberSize);
        }
    }
    if (var != NULL) {
        csound->Free(csound, var);
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
        CS_ARRAY_STORAGE* storage = dat->storage;
        int32_t refs = cs_array_storage_release_ref(csound, storage);
        if (UNLIKELY(refs < 0)) {
            csound->Die(csound, "negative shared array reference count");
            return;
        }
        if (refs == 0) {
            cs_array_storage_destroy_inline(csound, storage);
        }
        array_clear_view(dat);
        return;
    }

    /* Aliases have data but allocated == 0. An empty array still owns its
       one-element placeholder, so allocated remains greater than zero. */
    ownsData = dat->allocated > 0 && dat->data != NULL;
    if (ownsData) {
        csound_array_free_elements_inline(csound, dat->arrayType, dat->data,
                                          dat->allocated,
                                          dat->arrayMemberSize);
        csound->Free(csound, dat->data);
    }

    /* An uninitialised array shell owns its sizes metadata. Aliases own
       neither sizes nor data, and must only be detached. */
    if (dat->sizes != NULL && (ownsData || dat->data == NULL)) {
        csound->Free(csound, dat->sizes);
    }

    array_clear_view(dat);
}

void csound_array_share(CSOUND* csound, ARRAYDAT* dest, ARRAYDAT* src) {
    CS_ARRAY_STORAGE* storage;
    size_t logicalCount, capacity, strideBytes;

    if (dest == NULL || src == NULL || dest == src) {
        return;
    }
    if (UNLIKELY(src->arrayType == NULL ||
                 !src->arrayType->userDefinedType ||
                 csound_array_member_count(src, &logicalCount) != OK)) {
        csound->Die(csound, "invalid structured array during copy");
        return;
    }
    if (dest->data != NULL && dest->data == src->data &&
        dest->storage != NULL && dest->storage == src->storage) {
        return;
    }
    if (src->data == NULL) {
        if (UNLIKELY(src->storage != NULL)) {
            csound->Die(csound,
                        "structured array has storage without array data");
            return;
        }
        csound_free_array_storage(csound, dest);
        dest->dimensions = src->dimensions;
        dest->arrayMemberSize = src->arrayMemberSize;
        dest->arrayType = src->arrayType;
        if (src->dimensions > 0 && src->sizes != NULL) {
            dest->sizes = csound->Malloc(
              csound, sizeof(int32_t) * (size_t)src->dimensions);
            memcpy(dest->sizes, src->sizes,
                   sizeof(int32_t) * (size_t)src->dimensions);
        }
        return;
    }

    storage = src->storage;
    if (storage == NULL) {
        CS_ARRAY_STORAGE candidate = {0};

        /* A raw alias (allocated == 0) has no owner that can participate in
           reference counting. Internal structured-array copies now always
           carry a sidecar, so accepting such a source would risk freeing
           memory owned elsewhere. */
        if (UNLIKELY(src->allocated == 0 || src->arrayMemberSize <= 0 ||
                     src->allocated % (size_t)src->arrayMemberSize != 0)) {
            csound->Die(csound,
                        "cannot share non-owning structured-array storage");
            return;
        }

        candidate.refs = 1;
        candidate.dimensions = src->dimensions;
        candidate.arrayMemberSize = src->arrayMemberSize;
        candidate.arrayType = src->arrayType;
        candidate.sizes = src->sizes;
        candidate.data = src->data;
        candidate.allocated = src->allocated;
        if (UNLIKELY(cs_array_storage_layout(
                       src, &candidate, &logicalCount,
                       &capacity, &strideBytes) != OK)) {
            csound->Die(csound,
                        "invalid structured-array storage during copy");
            return;
        }

        /* Install the sidecar lazily. This is the bookkeeping-only source
           mutation mentioned in array_copy_value; normal array access remains
           unchanged, and additional views report allocated == 0. */
        storage = csound->Calloc(csound, sizeof(CS_ARRAY_STORAGE));
        *storage = candidate;
#if !defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN)
        /* Csound's generic atomic fallback is intentionally non-atomic. A
           sidecar can outlive the lock protecting its original ARRAYDAT, so
           non-atomic targets serialize reference changes with a mutex. */
        storage->refLock = csound->Create_Mutex(0);
        if (UNLIKELY(storage->refLock == NULL)) {
            csound->Free(csound, storage);
            csound->Die(csound,
                        "could not create shared-array reference lock");
            return;
        }
#endif
        src->storage = storage;
    }
    else if (UNLIKELY(cs_array_storage_layout(
                        src, storage, &logicalCount,
                        &capacity, &strideBytes) != OK)) {
        csound->Die(csound, "invalid structured-array storage during copy");
        return;
    }

    /* Validate and retain the source before releasing the destination. This
       matters for legacy aliases that may point at destination-owned data. */
    if (UNLIKELY(dest->data == src->data && dest->storage != storage &&
                 (dest->storage != NULL || dest->allocated > 0))) {
        csound->Die(csound, "conflicting structured-array ownership");
        return;
    }
    if (UNLIKELY(cs_array_storage_try_add_ref(csound, storage) == NOTOK)) {
        csound->Die(csound, "invalid shared array reference count");
        return;
    }
    csound_free_array_storage(csound, dest);

    dest->dimensions = src->dimensions;
    dest->sizes = src->sizes;
    dest->arrayMemberSize = src->arrayMemberSize;
    dest->arrayType = src->arrayType;
    dest->data = src->data;
    dest->allocated = 0;
    dest->storage = storage;
}

void csound_array_prepare_write(CSOUND* csound, ARRAYDAT* array,
                                INSDS* ctx) {
    csound_array_prepare_write_inline(csound, array, ctx);
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
