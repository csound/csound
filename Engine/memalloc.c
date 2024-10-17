/*
    memalloc.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, Richard Dobson,
              (C) 2005 Istvan Varga

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

#include "csoundCore.h"                 /*              MEMALLOC.C      */

#ifdef CUSTOM_MALLOC
#ifndef MALLOC_BASE
#define MALLOC_BASE 0xC0000000  // STM32H7
#endif

static unsigned long cur = MALLOC_BASE;

void *my_malloc(unsigned long bytes) {
  unsigned long tmp = cur;
  cur += bytes;
  return (void *) tmp;
}

void *my_calloc(unsigned long items, unsigned long bytes) {
  unsigned long tmp = cur;
  cur += bytes*items;
  memset((void *) tmp, 0, bytes*items);
  return (void *) tmp;  
}

void *my_realloc(void *old, unsigned long bytes) {
    unsigned long tmp = cur;
    cur += bytes;
    memcpy((void *) tmp, old, bytes*items);
    return (void *) tmp;
}

void my_free(void *old) {
  // nothing to do
  // TODO: implement freeing
  return;
}

#define CS_MALLOC mymalloc
#define CS_CALLOC mycalloc
#define CS_REALLOC myrealloc
#define CS_FREE myfree
#else
#define CS_MALLOC malloc
#define CS_CALLOC calloc
#define CS_REALLOC realloc
#define CS_FREE free
#endif

/* This code wraps malloc etc with maintaining a list of allocated memory
   so it can be freed on a reset.  It would not be necessary with a zoned
   allocator.
*/
#if defined(BETA) && !defined(MEMDEBUG)
#define MEMDEBUG  1
#endif

#define MEMALLOC_MAGIC  0x6D426C6B
/* The memory list must be controlled by mutex */
#define CSOUND_MEM_SPINLOCK csoundSpinLock(&csound->memlock);
#define CSOUND_MEM_SPINUNLOCK csoundSpinUnLock(&csound->memlock);

typedef struct memAllocBlock_s {
#ifdef MEMDEBUG
    int32_t                     magic;      /* 0x6D426C6B ("mBlk")          */
    void                    *ptr;       /* pointer to allocated area    */
#endif
    struct memAllocBlock_s  *prv;       /* previous structure in chain  */
    struct memAllocBlock_s  *nxt;       /* next structure in chain      */
} memAllocBlock_t;

#define HDR_SIZE    (((int32_t) sizeof(memAllocBlock_t) + 7) & (~7))
#define ALLOC_BYTES(n)  ((size_t) HDR_SIZE + (size_t) (n))
#define DATA_PTR(p) ((void*) ((unsigned char*) (p) + (int32_t) HDR_SIZE))
#define HDR_PTR(p)  ((memAllocBlock_t*) ((unsigned char*) (p) - (int32_t) HDR_SIZE))

#define MEMALLOC_DB (csound->memalloc_db)

static void memdie(CSOUND *csound, size_t nbytes)
{
    csound->ErrorMsg(csound, Str("memory allocate failure for %zd \n"),
                             nbytes);
    csound->LongJmp(csound, CSOUND_MEMORY);
}

void *mmalloc(CSOUND *csound, size_t size)
{
    void  *p;

#ifdef MEMDEBUG
    if (UNLIKELY(size == (size_t) 0)) {
      csound->DebugMsg(csound,
              " *** internal error: mmalloc() called with zero nbytes\n");
      return NULL;
    }
#endif
    /* allocate memory */
    if (UNLIKELY((p = CS_MALLOC(ALLOC_BYTES(size))) == NULL)) {
        csound->ErrorMsg(csound, "Malloc failed: ");
        memdie(csound, size);     /* does a long jump */
    }
    /* link into chain */
#ifdef MEMDEBUG
    ((memAllocBlock_t*) p)->magic = MEMALLOC_MAGIC;
    ((memAllocBlock_t*) p)->ptr = DATA_PTR(p);
#endif
    CSOUND_MEM_SPINLOCK
    ((memAllocBlock_t*) p)->prv = (memAllocBlock_t*) NULL;
    ((memAllocBlock_t*) p)->nxt = (memAllocBlock_t*) MEMALLOC_DB;
    if (MEMALLOC_DB != NULL)
      ((memAllocBlock_t*) MEMALLOC_DB)->prv = (memAllocBlock_t*) p;
    MEMALLOC_DB = (void*) p;
    CSOUND_MEM_SPINUNLOCK
    /* return with data pointer */
    return DATA_PTR(p);
}

void *mmallocDebug(CSOUND *csound, size_t size, char *file, int32_t line)
{
    void *ans = mmalloc(csound,size);
    csound->DebugMsg(csound, "Alloc %p (%zu) %s:%d\n", ans, size, file, line);
    return ans;
}

void *mcalloc(CSOUND *csound, size_t size)
{
    void  *p;

#ifdef MEMDEBUG
    if (UNLIKELY(size == (size_t) 0)) {
      csound->DebugMsg(csound,
              " *** internal error: csound->Calloc() called with zero nbytes\n");
      return NULL;
    }
#endif
    /* allocate memory */
    if (UNLIKELY((p = CS_CALLOC(ALLOC_BYTES(size), (size_t) 1)) == NULL)) {
      csound->ErrorMsg(csound, "Calloc failed: ");
      memdie(csound, size);     /* does longjump */
    }
    /* link into chain */
#ifdef MEMDEBUG
    ((memAllocBlock_t*) p)->magic = MEMALLOC_MAGIC;
    ((memAllocBlock_t*) p)->ptr = DATA_PTR(p);
#endif
    CSOUND_MEM_SPINLOCK
    ((memAllocBlock_t*) p)->prv = (memAllocBlock_t*) NULL;
    ((memAllocBlock_t*) p)->nxt = (memAllocBlock_t*) MEMALLOC_DB;
    if (MEMALLOC_DB != NULL)
      ((memAllocBlock_t*) MEMALLOC_DB)->prv = (memAllocBlock_t*) p;
    MEMALLOC_DB = (void*) p;
    CSOUND_MEM_SPINUNLOCK
    /* return with data pointer */
    return DATA_PTR(p);
}

void *mcallocDebug(CSOUND *csound, size_t size, char *file, int32_t line)
{
    void *ans = mcalloc(csound,size);
    csound->DebugMsg(csound, "Alloc %p (%zu) %s:%d\n", ans, size, file, line);
    return ans;
}


void mfree(CSOUND *csound, void *p)
{
    memAllocBlock_t *pp;

    if (UNLIKELY(p == NULL))
      return;
    pp = HDR_PTR(p);
 #ifdef MEMDEBUG
    if (UNLIKELY(pp->magic != MEMALLOC_MAGIC || pp->ptr != p)) {
      csound->Warning(csound, "csound->Free() called with invalid "
                      "pointer (%p) %x %p %x",
                      p, pp->magic, pp->ptr, MEMALLOC_MAGIC);
      /* exit() is ugly, but this is a fatal error that can only occur */
      /* as a result of a bug */
      /*  exit(-1);  */
      /*VL 28-12-12 - returning from here instead of exit() */
      return;
    }
    pp->magic = 0;
 #endif
    CSOUND_MEM_SPINLOCK
    /* unlink from chain */
    {
      memAllocBlock_t *prv = pp->prv, *nxt = pp->nxt;
      if (nxt != NULL)
        nxt->prv = prv;
      if (prv != NULL)
        prv->nxt = nxt;
      else
        MEMALLOC_DB = (void*)nxt;
    }
    //csound->Message(csound, "free\n");
    /* free memory */
    CS_FREE((void*) pp);
    CSOUND_MEM_SPINUNLOCK
}

void mfreeDebug(CSOUND *csound, void *ans, char *file, int32_t line)
{
    printf("Free %p %s:%d\n", ans, file, line);
    mfree(csound,ans);
}

void *mrealloc(CSOUND *csound, void *oldp, size_t size)
{
    memAllocBlock_t *pp;
    void            *p;

    if (UNLIKELY(oldp == NULL))
      return mmalloc(csound, size);
    if (UNLIKELY(size == (size_t) 0)) {
      mfree(csound, oldp);
      return NULL;
    }
    pp = HDR_PTR(oldp);
#ifdef MEMDEBUG
    if (UNLIKELY(pp->magic != MEMALLOC_MAGIC || pp->ptr != oldp)) {
      csound->DebugMsg(csound, " *** internal error: mrealloc() called with invalid "
                      "pointer (%p)\n", oldp);
      /* exit() is ugly, but this is a fatal error that can only occur */
      /* as a result of a bug */
      exit(-1);
    }
    /* mark old header as invalid */
    pp->magic = 0;
    pp->ptr = NULL;
#endif
    /* allocate memory */
    p = CS_REALLOC((void*) pp, ALLOC_BYTES(size));
    pp = p;
    if (UNLIKELY(p == NULL)) {
#ifdef MEMDEBUG
      CSOUND_MEM_SPINLOCK
      /* alloc failed, restore original header */
      pp->magic = MEMALLOC_MAGIC;
      pp->ptr = oldp;
      CSOUND_MEM_SPINUNLOCK
#endif
      csound->ErrorMsg(csound, "Realloc failed: ");  
      memdie(csound, size);
      return NULL;
    }
    CSOUND_MEM_SPINLOCK
    /* create new header and update chain pointers */
    pp = (memAllocBlock_t*) p;
#ifdef MEMDEBUG
    pp->magic = MEMALLOC_MAGIC;
    pp->ptr = DATA_PTR(pp);
#endif
    {
      memAllocBlock_t *prv = pp->prv, *nxt = pp->nxt;
      if (nxt != NULL)
        nxt->prv = pp;
      if (prv != NULL)
        prv->nxt = pp;
      else
        MEMALLOC_DB = (void*) pp;
    }
    CSOUND_MEM_SPINUNLOCK
    /* return with data pointer */
    return DATA_PTR(pp);
}

void *mreallocDebug(CSOUND *csound, void *oldp, size_t size, char *file, int32_t line)
{
    void *p = mrealloc(csound, oldp, size);
    csound->DebugMsg(csound, "Realloc %p->%p (%zu) %s:%d\n", oldp, p, size, file, line);
    return p;
}

void memRESET(CSOUND *csound)
{
    memAllocBlock_t *pp, *nxtp;

    pp = (memAllocBlock_t*) MEMALLOC_DB;
    MEMALLOC_DB = NULL;
    while (pp != NULL) {
      nxtp = pp->nxt;
#ifdef MEMDEBUG
      pp->magic = 0;
#endif
      CS_FREE((void*) pp);
      pp = nxtp;
    }
}
