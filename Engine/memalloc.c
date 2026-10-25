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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
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
    void                    *base;      /* address to pass to CS_FREE   */
#ifdef CSOUND_WASI_REALTIME
    CSOUND_WASI_ALLOC_SCOPE *owner;
    struct memAllocBlock_s *owner_prv, *owner_nxt;
    void (*dispose)(CSOUND *, void *);
#endif
} memAllocBlock_t;
#ifdef ALIGN_MEMORY
 #define MALIGN (sizeof(cs_float)-1)
#else 
 #define MALIGN 0
#endif

/* Keep the data offset a multiple of 16 as the private header grows. */
#define HDR_SIZE    (((size_t) sizeof(memAllocBlock_t) + 15) & (~(size_t)15))
#define ALLOC_BYTES(n)  (((size_t) HDR_SIZE + (size_t) (n) + MALIGN) & (~ MALIGN))
#define DATA_PTR(p) ((void*) ((unsigned char*) (p) + (int32_t) HDR_SIZE))
#define HDR_PTR(p)  ((memAllocBlock_t*) ((unsigned char*) (p) - (int32_t) HDR_SIZE))

/* Only aligned blocks need a stored size and alignment for ReAlloc. This
   record lives at base, before the padding and the regular block header. */
typedef struct {
    size_t size, align;
} alignedAllocInfo_t;

static memAllocBlock_t *alloc_aligned(size_t size, size_t align)
{
    const size_t overhead = sizeof(alignedAllocInfo_t) + HDR_SIZE;
    alignedAllocInfo_t *base;
    memAllocBlock_t *block;
    uintptr_t data;

    if (UNLIKELY(align == 0 || (align & (align - 1)) != 0)) return NULL;
    /* The header also needs alignment, even for a one-byte data request. */
    if (align < 16) align = 16;
    if (UNLIKELY(align - 1 > SIZE_MAX - overhead ||
                 size > SIZE_MAX - overhead - (align - 1))) return NULL;
    base = CS_CALLOC(overhead + align - 1 + size, 1);
    if (UNLIKELY(base == NULL)) return NULL;

    base->size = size;
    base->align = align;
    /* Align the data, not the allocation base. Keep the regular header
       immediately before it so Free, ReAlloc and ownership tracking agree. */
    data = ((uintptr_t)base + overhead + align - 1) & ~(uintptr_t)(align - 1);
    block = HDR_PTR((void *)data);
    block->base = base;
    return block;
}

#define MEMALLOC_DB (csound->memalloc_db)

#ifdef CSOUND_WASI_REALTIME
static _Thread_local CSOUND_WASI_ALLOC_SCOPE *alloc_scope;

CSOUND_WASI_ALLOC_SCOPE *csoundWasiSwapAllocScope(
    CSOUND_WASI_ALLOC_SCOPE *scope)
{
    CSOUND_WASI_ALLOC_SCOPE *previous = alloc_scope;
    alloc_scope = scope;
    return previous;
}

int32_t csoundWasiHasAllocScope(CSOUND *csound)
{
    return alloc_scope != NULL && alloc_scope->csound == csound;
}

/* Called with the allocation list locked. A node may leave the AST during
   rewriting, so ownership follows its allocation, not its tree links. */
static void unlink_owner(memAllocBlock_t *block)
{
    if (block->owner == NULL) return;
    if (block->owner_prv != NULL)
      block->owner_prv->owner_nxt = block->owner_nxt;
    else
      block->owner->allocations = block->owner_nxt;
    if (block->owner_nxt != NULL)
      block->owner_nxt->owner_prv = block->owner_prv;
    block->owner = NULL;
}

void *csoundWasiOwnAllocation(CSOUND *csound, void *ptr,
                            void (*dispose)(CSOUND *, void *))
{
    if (ptr != NULL && csoundWasiHasAllocScope(csound)) {
      memAllocBlock_t *block = HDR_PTR(ptr);
      CSOUND_MEM_SPINLOCK
      if (block->owner == NULL) {
        block->owner = alloc_scope;
        block->dispose = dispose;
        block->owner_prv = NULL;
        block->owner_nxt = alloc_scope->allocations;
        if (block->owner_nxt != NULL)
          block->owner_nxt->owner_prv = block;
        alloc_scope->allocations = block;
      }
      CSOUND_MEM_SPINUNLOCK
    }
    return ptr;
}

void csoundWasiReleaseAllocation(CSOUND *csound, void *ptr)
{
    if (ptr == NULL) return;
    CSOUND_MEM_SPINLOCK
    unlink_owner(HDR_PTR(ptr));
    CSOUND_MEM_SPINUNLOCK
}

void csoundWasiTransferAllocScope(CSOUND_WASI_ALLOC_SCOPE *source,
                                 CSOUND_WASI_ALLOC_SCOPE *destination)
{
    CSOUND *csound = source->csound;
    CSOUND_MEM_SPINLOCK
    while (source->allocations != NULL) {
      memAllocBlock_t *block = source->allocations;
      unlink_owner(block);
      block->owner = destination;
      block->owner_prv = NULL;
      block->owner_nxt = destination->allocations;
      if (block->owner_nxt != NULL) block->owner_nxt->owner_prv = block;
      destination->allocations = block;
    }
    CSOUND_MEM_SPINUNLOCK
}

void csoundWasiFinishAllocScope(CSOUND_WASI_ALLOC_SCOPE *scope,
                               int32_t discard)
{
    CSOUND *csound = scope->csound;
    while (scope->allocations != NULL) {
      memAllocBlock_t *block = scope->allocations;
      void (*dispose)(CSOUND *, void *) = block->dispose;
      CSOUND_MEM_SPINLOCK
      unlink_owner(block);
      CSOUND_MEM_SPINUNLOCK
      if (discard) {
        if (dispose != NULL) dispose(csound, DATA_PTR(block));
        else csoundFree(csound, DATA_PTR(block));
      }
    }
}
#endif

static void memdie(CSOUND *csound, size_t nbytes)
{
#ifdef CSOUND_WASI_REALTIME
    /* Allocation failure can interrupt a persistent engine update too. It
       must not look like an ordinary, recoverable source error. */
    CSOUND_WASI_COMPILE_JUMP *context = csoundWasiCompileJumpContext(csound);
    if (context != NULL) context->enabled = 0;
#endif
    csound->ErrorMsg(csound, Str("memory allocate failure for %zd \n"),
                             nbytes);
    csound->LongJmp(csound, CSOUND_MEMORY);
}

void *csoundMalloc(CSOUND *csound, size_t size)
{
    void  *p;

#ifdef MEMDEBUG
    if (UNLIKELY(size == (size_t) 0)) {
      csound->DebugMsg(csound,
              " *** internal error: csoundMalloc() called with zero nbytes\n");
      return NULL;
    }
#endif
    /* allocate memory */
    if (UNLIKELY((p = CS_MALLOC(ALLOC_BYTES(size))) == NULL)) {
        csound->ErrorMsg(csound, Str("Malloc failed: "));
        memdie(csound, size);     /* does a long jump */
    }
    /* link into chain */
    ((memAllocBlock_t*) p)->base = p;
#ifdef CSOUND_WASI_REALTIME
    ((memAllocBlock_t*) p)->owner = NULL;
#endif
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

void *csoundMalloc_debug(CSOUND *csound, size_t size, char *file, int32_t line)
{
    void *ans = csoundMalloc(csound,size);
    csound->DebugMsg(csound, "Alloc %p (%zu) %s:%d\n", ans, size, file, line);
    return ans;
}


void *csoundCalloc(CSOUND *csound, size_t size)
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
      csound->ErrorMsg(csound, Str("Calloc failed: "));
      memdie(csound, size);     /* does longjump */
    }
    /* link into chain */
    ((memAllocBlock_t*) p)->base = p;
#ifdef CSOUND_WASI_REALTIME
    ((memAllocBlock_t*) p)->owner = NULL;
#endif
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

void *csoundCalloc_debug(CSOUND *csound, size_t size, char *file, int32_t line)
{
    void *ans = csoundCalloc(csound,size);
    csound->DebugMsg(csound, "Alloc %p (%zu) %s:%d\n", ans, size, file, line);
    return ans;
}

void *csoundCallocAligned(CSOUND *csound, size_t size, size_t align) {
    void  *p;

#ifdef MEMDEBUG
    if (UNLIKELY(size == (size_t) 0)) {
      csound->DebugMsg(csound,
              " *** internal error: csound->Calloc() called with zero nbytes\n");
      return NULL;
    }
#endif
    /* allocate memory */
    if (UNLIKELY((p = alloc_aligned(size, align)) == NULL)) {
      csound->ErrorMsg(csound, Str("CallocAligned failed: "));
      memdie(csound, size);     /* does longjump */
      return NULL;
    }
    /* link into chain */
#ifdef CSOUND_WASI_REALTIME
    ((memAllocBlock_t*) p)->owner = NULL;
#endif
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



void csoundFree(CSOUND *csound, void *p)
{
    memAllocBlock_t *pp;

    if (UNLIKELY(p == NULL))
      return;
#ifdef MEMDEBUG
    /* In debug builds, avoid touching freed headers: search the live list first */
    memAllocBlock_t *cur;
    pp = NULL;
    CSOUND_MEM_SPINLOCK
    cur = (memAllocBlock_t*) MEMALLOC_DB;
    while (cur != NULL) {
      if (cur->ptr == p) { pp = cur; break; }
      cur = cur->nxt;
    }
    CSOUND_MEM_SPINUNLOCK
    if (UNLIKELY(pp == NULL || pp->magic != MEMALLOC_MAGIC || pp->ptr != p)) {
      if (pp != NULL && pp->magic != MEMALLOC_MAGIC) {
        csound->Warning(csound, Str("csound->Free() called with corrupted pointer (%p)\n"),
                        p);
      }
      return;
    }
    pp->magic = 0;
    CSOUND_MEM_SPINLOCK
#else
    pp = HDR_PTR(p);
    CSOUND_MEM_SPINLOCK
#endif
    /* unlink from chain */
#ifdef CSOUND_WASI_REALTIME
    unlink_owner(pp);
#endif
    {
      memAllocBlock_t *prv = pp->prv, *nxt = pp->nxt;
      if (nxt != NULL)
        nxt->prv = prv;
      if (prv != NULL)
        prv->nxt = nxt;
      else
        MEMALLOC_DB = (void*)nxt;
    }
    /* free memory */
    CS_FREE(pp->base);
    CSOUND_MEM_SPINUNLOCK
}

void csoundFree_debug(CSOUND *csound, void *ans, char *file, int32_t line)
{
    printf("Free %p %s:%d\n", ans, file, line);
    csoundFree(csound,ans);
}

void *csoundRealloc(CSOUND *csound, void *oldp, size_t size)
{
    memAllocBlock_t *pp;
    void            *p;

    if (UNLIKELY(oldp == NULL))
      return csoundMalloc(csound, size);
    if (UNLIKELY(size == (size_t) 0)) {
      csoundFree(csound, oldp);
      return NULL;
    }
    pp = HDR_PTR(oldp);
    /* Keep the live list locked while realloc can move its block header.
       Other threads must not follow the old links before we repair them. */
    CSOUND_MEM_SPINLOCK
#ifdef MEMDEBUG
    if (UNLIKELY(pp->magic != MEMALLOC_MAGIC || pp->ptr != oldp)) {
      CSOUND_MEM_SPINUNLOCK
      csound->DebugMsg(csound, " *** internal error: csoundRealloc() called with invalid "
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
    if (pp->base == pp) {
      p = CS_REALLOC((void*) pp, ALLOC_BYTES(size));
      if (p != NULL) ((memAllocBlock_t*)p)->base = p;
    }
    else {
      alignedAllocInfo_t *info = (alignedAllocInfo_t*)pp->base;
      memAllocBlock_t *newblock = alloc_aligned(size, info->align);
      p = newblock;
      if (newblock != NULL) {
        void *base = newblock->base;
        memcpy(DATA_PTR(newblock), oldp, size < info->size ? size : info->size);
        /* Preserve list links and any WASI allocation owner. The common
           code below repairs their pointers after moving the header. */
        *newblock = *pp;
        newblock->base = base;
        CS_FREE(pp->base);
      }
    }
    if (UNLIKELY(p == NULL)) {
#ifdef MEMDEBUG
      /* alloc failed, restore original header */
      pp->magic = MEMALLOC_MAGIC;
      pp->ptr = oldp;
#endif
      CSOUND_MEM_SPINUNLOCK
      csound->ErrorMsg(csound, Str("Realloc failed: "));
      memdie(csound, size);
      return NULL;
    }
    /* create new header and update chain pointers */
    pp = (memAllocBlock_t*) p;
#ifdef CSOUND_WASI_REALTIME
    if (pp->owner != NULL) {
      if (pp->owner_prv != NULL) pp->owner_prv->owner_nxt = pp;
      else pp->owner->allocations = pp;
      if (pp->owner_nxt != NULL) pp->owner_nxt->owner_prv = pp;
    }
#endif
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

void *csoundRealloc_debug(CSOUND *csound, void *oldp, size_t size, char *file, int32_t line)
{
    void *p = csoundRealloc(csound, oldp, size);
    csound->DebugMsg(csound, "Realloc %p->%p (%zu) %s:%d\n", oldp, p, size, file, line);
    return p;
}

void memreset(CSOUND *csound)
{
    memAllocBlock_t *pp, *nxtp;

    pp = (memAllocBlock_t*) MEMALLOC_DB;
    MEMALLOC_DB = NULL;
    while (pp != NULL) {
      nxtp = pp->nxt;
#ifdef MEMDEBUG
      pp->magic = 0;
#endif
      CS_FREE(pp->base);
      pp = nxtp;
    }
}
