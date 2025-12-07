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

typedef struct {
    OPDS    h;
    MYFLT   *r, *a;
} AEVAL;

static inline void tabinit(CSOUND *csound, ARRAYDAT *p, int32_t size, INSDS *ctx)
{
    size_t ss;
    int32_t oldCount = (p->allocated > 0 && p->arrayMemberSize > 0)
                         ? (int32_t)(p->allocated / (size_t)p->arrayMemberSize)
                         : 0;
    if (p->dimensions==0) {
        p->dimensions = 1;
        p->sizes = (int32_t*)csound->Calloc(csound, sizeof(int32_t));
    }
    if (p->data == NULL) {
        if (UNLIKELY(p->arrayType == NULL)) {
          csound->Die(csound, "tabinit: arrayType is NULL");
          return;
        }
        CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL, ctx);
        p->arrayMemberSize = var->memBlockSize;
        ss = (size_t)p->arrayMemberSize * (size_t)size;
        p->data = (MYFLT*)csound->Calloc(csound, ss);
        p->allocated = ss;
        // Initialize struct elements if user-defined type
        if (p->arrayType && p->arrayType->userDefinedType && var->initializeVariableMemory) {
          char *base = (char*)p->data;
          size_t blockSize = (size_t)var->memBlockSize;
          // Batch process struct arrays
          int32_t i = 0;
          for (; i + 3 < size; i += 4) {
            var->initializeVariableMemory(csound, var, (MYFLT*)(base + i * blockSize));
            var->initializeVariableMemory(csound, var, (MYFLT*)(base + (i + 1) * blockSize));
            var->initializeVariableMemory(csound, var, (MYFLT*)(base + (i + 2) * blockSize));
            var->initializeVariableMemory(csound, var, (MYFLT*)(base + (i + 3) * blockSize));
          }
          // The second loop only handles the remainder elements (0-3 elements at most)
          for (; i < size; i++) {
            var->initializeVariableMemory(csound, var, (MYFLT*)(base + i * blockSize));
          }
          csound->Free(csound, var);
        }
    } else if( (ss = (size_t)p->arrayMemberSize * (size_t)size) > p->allocated) {
        size_t prevAllocated = p->allocated;
        p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
        memset((char*)(p->data)+prevAllocated, '\0', ss-prevAllocated);
        p->allocated = ss;
        // Initialize only the newly added struct elements if it's UDT
        if (p->arrayType && p->arrayType->userDefinedType) {
          CS_VARIABLE* var2 = p->arrayType->createVariable(csound, NULL, ctx);
          if (var2 && var2->initializeVariableMemory) {
            char *base = (char*)p->data;
            size_t blockSize = (size_t)var2->memBlockSize;
            // Batch process arrays
            int32_t i = oldCount;
            for (; i + 3 < size; i += 4) {
              var2->initializeVariableMemory(csound, var2, (MYFLT*)(base + i * blockSize));
              var2->initializeVariableMemory(csound, var2, (MYFLT*)(base + (i + 1) * blockSize));
              var2->initializeVariableMemory(csound, var2, (MYFLT*)(base + (i + 2) * blockSize));
              var2->initializeVariableMemory(csound, var2, (MYFLT*)(base + (i + 3) * blockSize));
            }
            // The second loop only handles the remainder elements (0-3 elements at most)
            for (; i < size; i++) {
              var2->initializeVariableMemory(csound, var2, (MYFLT*)(base + i * blockSize));
            }
          }
          csound->Free(csound, var2);
        }
    }
    if (p->dimensions==1) p->sizes[0] = size;
}

static inline void tabinit_like(CSOUND *csound, ARRAYDAT *p, const ARRAYDAT *tp)
{
    uint32_t elemCount = 1;
    if(p->data == tp->data) {
        return;
    }
    // Additional safety check: if p and tp are the same ARRAYDAT structure, don't modify
    if(p == tp) {
        return;
    }
    if (p->dimensions != tp->dimensions) {
      p->sizes = (int32_t*)csound->ReAlloc(csound, p->sizes,
                                           sizeof(int32_t)*tp->dimensions);
      p->dimensions = tp->dimensions;
    }

    for (int32_t i=0; i<tp->dimensions; i++) {
      p->sizes[i] = tp->sizes[i];
      elemCount *= (uint32_t)tp->sizes[i];
    }
    if(p->arrayType == NULL) p->arrayType = tp->arrayType;
    int32_t oldCount = (p->allocated > 0 && p->arrayMemberSize > 0)
                         ? (int32_t)(p->allocated / (size_t)p->arrayMemberSize)
                         : 0;
    if (p->data == NULL) {
      CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL, NULL);
      p->arrayMemberSize = var->memBlockSize;
      size_t bytes = (size_t)p->arrayMemberSize * (size_t)elemCount;
      p->data = (MYFLT*)csound->Calloc(csound, bytes);
      p->allocated = bytes;
      // Initialize struct elements if it's UDT
      if (p->arrayType && p->arrayType->userDefinedType && var->initializeVariableMemory) {
        char *base = (char*)p->data;
        size_t blockSize = (size_t)var->memBlockSize;
        // Batch process arrays
        uint32_t i = 0;
        for (; i + 3 < elemCount; i += 4) {
          var->initializeVariableMemory(csound, var, (MYFLT*)(base + i * blockSize));
          var->initializeVariableMemory(csound, var, (MYFLT*)(base + (i + 1) * blockSize));
          var->initializeVariableMemory(csound, var, (MYFLT*)(base + (i + 2) * blockSize));
          var->initializeVariableMemory(csound, var, (MYFLT*)(base + (i + 3) * blockSize));
        }
        // The second loop only handles the remainder elements (0-3 elements at most)
        for (; i < elemCount; i++) {
          var->initializeVariableMemory(csound, var, (MYFLT*)(base + i * blockSize));
        }
      }
      csound->Free(csound, var);
    } else {
      size_t bytes = (size_t)p->arrayMemberSize * (size_t)elemCount;
      if (bytes > p->allocated) {
        size_t prevAllocated = p->allocated;
        p->data = (MYFLT*) csound->ReAlloc(csound, p->data, bytes);
        memset((char*)(p->data) + prevAllocated, '\0', bytes - prevAllocated);
        p->allocated = bytes;
        // Initialize only the newly added struct elements if it's UDT
        if (p->arrayType && p->arrayType->userDefinedType) {
          CS_VARIABLE* var2 = p->arrayType->createVariable(csound, NULL, NULL);
          if (var2 && var2->initializeVariableMemory) {
            char *base = (char*)p->data;
            size_t blockSize = (size_t)var2->memBlockSize;
            // Batch process arrays
            int32_t i = oldCount;
            for (; (uint32_t)(i + 3) < elemCount; i += 4) {
              var2->initializeVariableMemory(csound, var2, (MYFLT*)(base + i * blockSize));
              var2->initializeVariableMemory(csound, var2, (MYFLT*)(base + (i + 1) * blockSize));
              var2->initializeVariableMemory(csound, var2, (MYFLT*)(base + (i + 2) * blockSize));
              var2->initializeVariableMemory(csound, var2, (MYFLT*)(base + (i + 3) * blockSize));
            }
            for (; (uint32_t)i < elemCount; i++) {
              var2->initializeVariableMemory(csound, var2, (MYFLT*)(base + i * blockSize));
            }
            csound->Free(csound, var2);
          }
        }
      }
    }
}

static inline int32_t tabcheck(CSOUND *csound, ARRAYDAT *p, int32_t size, OPDS *q)
{
    if (p->data==NULL || p->dimensions == 0) {
      return csound->PerfError(csound, q, "%s", Str("Array not initialised"));
    }
    size_t s = p->arrayMemberSize*size;
    if (s > p->allocated) { /* was arr->allocate */
      return csound->PerfError(csound, q,
        Str("Array too small (allocated %zu < needed %zu), but cannot "
            "allocate during performance pass. Allocate a bigger array at init time"),
        p->allocated, s);
    }
    p->sizes[0] = size;
    return OK;
}

#endif /* end of include guard: __ARRAY_H__ */
