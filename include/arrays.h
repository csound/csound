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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef __ARRAY_H__
#define __ARRAY_H__

static inline void tabinit(CSOUND *csound, ARRAYDAT *p, int size)
{
    size_t ss;
    if (p->dimensions==0) {
        p->dimensions = 1;
        p->sizes = (int32_t*)csound->Calloc(csound, sizeof(int32_t));
    }
    if (p->data == NULL) {
        CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
        p->arrayMemberSize = var->memBlockSize;
        ss = p->arrayMemberSize*size;
        p->data = (MYFLT*)csound->Calloc(csound, ss);
        p->allocated = ss;
    } else if( (ss = p->arrayMemberSize*size) > p->allocated) {
        p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
        memset((char*)(p->data)+p->allocated, '\0', ss-p->allocated);
        p->allocated = ss;
    }
    if (p->dimensions==1) p->sizes[0] = size;
    //p->dimensions = 1;
}

static inline void tabinit_like(CSOUND *csound, ARRAYDAT *p, ARRAYDAT *tp)
{
    uint32_t ss = 1;
    if(p->data == tp->data) {
        return;
    }
    if (p->dimensions != tp->dimensions) {
      p->sizes = (int32_t*)csound->ReAlloc(csound, p->sizes,
                                           sizeof(int32_t)*tp->dimensions);
      p->dimensions = tp->dimensions;
    }

    for (int i=0; i<tp->dimensions; i++) {
      p->sizes[i] = tp->sizes[i];
      ss *= tp->sizes[i];
    }

    if (p->data == NULL) {
        CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
        p->arrayMemberSize = var->memBlockSize;
        ss = p->arrayMemberSize*ss;
        p->data = (MYFLT*)csound->Calloc(csound, ss);
        p->allocated = ss;
    } else if( (ss = p->arrayMemberSize*ss) > p->allocated) {
        p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
        p->allocated = ss;
    }
}

static inline int tabcheck(CSOUND *csound, ARRAYDAT *p, int size, OPDS *q)
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
      return NOTOK;
    }
    p->sizes[0] = size;
    return OK;
}

#if 0
static inline void tabensure(CSOUND *csound, ARRAYDAT *p, int size)
{
    if (p->data==NULL || p->dimensions == 0 ||
        (p->dimensions==1 && p->sizes[0] < size)) {
      size_t ss;
      if (p->data == NULL) {
        CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
        p->arrayMemberSize = var->memBlockSize;
      }
      ss = p->arrayMemberSize*size;
      if (p->data==NULL) {
        p->data = (MYFLT*)csound->Calloc(csound, ss);
        p->allocated = ss;
      }
      else if (ss > p->allocated) {
        p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
        p->allocated = ss;
      }
      if (p->dimensions==0) {
        p->dimensions = 1;
        p->sizes = (int32_t*)csound->Malloc(csound, sizeof(int32_t));
      }
      p->sizes[0] = size;
    }
    //p->sizes[0] = size;
}
#endif

#endif /* end of include guard: __ARRAY_H__ */
