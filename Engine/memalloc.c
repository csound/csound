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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "cs.h"                         /*              MEMALLOC.C      */
/*RWD 9:2000 for pvocex support */
#include "pvfileio.h"
/* global here so reachable by all standalones */

#define MEMALLOC_MAGIC  0x6D426C6B

typedef struct memAllocBlock_s {
    int                     magic;      /* 0x6D426C6B ("mBlk")          */
    void                    *ptr;       /* pointer to allocated area    */
    struct memAllocBlock_s  *prv;       /* previous structure in chain  */
    struct memAllocBlock_s  *nxt;       /* next structure in chain      */
} memAllocBlock_t;

#define HDR_SIZE    (((int) sizeof(memAllocBlock_t) + 15) & (~15))
#define ALLOC_BYTES(n)  ((size_t) HDR_SIZE + (size_t) (n))
#define DATA_PTR(p) ((void*) ((unsigned char*) (p) + (int) HDR_SIZE))
#define HDR_PTR(p)  ((memAllocBlock_t*) ((unsigned char*) (p) - (int) HDR_SIZE))

#define MEMALLOC_DB (((ENVIRON*) csound)->memalloc_db)

#define NO_ZERO_ALLOCS  1

static void memdie(void *csound, int nbytes)
{
    csoundDie(csound, Str("memory allocate failure for %d\n"), nbytes);
}

void *mmalloc(void *csound, size_t size)
{
    void  *p;

#ifdef NO_ZERO_ALLOCS
    if (size == (size_t) 0) {
      fprintf(stderr,
              " *** internal error: mmalloc() called with zero nbytes\n");
      return NULL;
    }
#endif
    /* allocate memory */
    if ((p = malloc(ALLOC_BYTES(size))) == NULL) {
      memdie(csound, (int) size);
      return NULL;
    }
    /* link into chain */
    ((memAllocBlock_t*) p)->magic = MEMALLOC_MAGIC;
    ((memAllocBlock_t*) p)->ptr = DATA_PTR(p);
    ((memAllocBlock_t*) p)->prv = (memAllocBlock_t*) NULL;
    ((memAllocBlock_t*) p)->nxt = (memAllocBlock_t*) MEMALLOC_DB;
    if (MEMALLOC_DB != NULL)
      ((memAllocBlock_t*) MEMALLOC_DB)->prv = (memAllocBlock_t*) p;
    MEMALLOC_DB = (void*) p;
    /* return with data pointer */
    return DATA_PTR(p);
}

void *mcalloc(void *csound, size_t size)
{
    void  *p;

#ifdef NO_ZERO_ALLOCS
    if (size == (size_t) 0) {
      fprintf(stderr,
              " *** internal error: mcalloc() called with zero nbytes\n");
      return NULL;
    }
#endif
    /* allocate memory */
    if ((p = calloc(ALLOC_BYTES(size), (size_t) 1)) == NULL) {
      memdie(csound, (int) size);
      return NULL;
    }
    /* link into chain */
    ((memAllocBlock_t*) p)->magic = MEMALLOC_MAGIC;
    ((memAllocBlock_t*) p)->ptr = DATA_PTR(p);
    ((memAllocBlock_t*) p)->prv = (memAllocBlock_t*) NULL;
    ((memAllocBlock_t*) p)->nxt = (memAllocBlock_t*) MEMALLOC_DB;
    if (MEMALLOC_DB != NULL)
      ((memAllocBlock_t*) MEMALLOC_DB)->prv = (memAllocBlock_t*) p;
    MEMALLOC_DB = (void*) p;
    /* return with data pointer */
    return DATA_PTR(p);
}

void mfree(void *csound, void *p)
{
    memAllocBlock_t *pp;

    if (p == NULL)
      return;
    pp = HDR_PTR(p);
    if (pp->magic != MEMALLOC_MAGIC || pp->ptr != p) {
      fprintf(stderr, " *** internal error: mfree() called with invalid "
                      "pointer (0x%p)\n", p);
      /* exit() is ugly, but this is a fatal error that can only occur */
      /* as a result of a bug */
      exit(-1);
    }
    /* unlink from chain */
    if (pp->nxt != NULL)
      pp->nxt->prv = pp->prv;
    if (pp->prv != NULL)
      pp->prv->nxt = pp->nxt;
    else
      MEMALLOC_DB = (void*) pp->nxt;
    /* free memory */
    free((void*) pp);
}

void *mrealloc(void *csound, void *oldp, size_t size)
{
    memAllocBlock_t *pp;
    void            *p;

    if (oldp == NULL)
      return mmalloc(csound, size);
    if (size == (size_t) 0) {
      mfree(csound, oldp);
      return NULL;
    }
    pp = HDR_PTR(oldp);
    if (pp->magic != MEMALLOC_MAGIC || pp->ptr != oldp) {
      fprintf(stderr, " *** internal error: mrealloc() called with invalid "
                      "pointer (0x%p)\n", oldp);
      /* exit() is ugly, but this is a fatal error that can only occur */
      /* as a result of a bug */
      exit(-1);
    }
    /* mark old header as invalid */
    pp->magic = 0;
    pp->ptr = NULL;
    /* allocate memory */
    p = realloc((void*) pp, ALLOC_BYTES(size));
    if (p == NULL) {
      /* alloc failed, restore original header */
      pp->magic = MEMALLOC_MAGIC;
      pp->ptr = oldp;
      memdie(csound, (int) size);
      return NULL;
    }
    /* create new header and update chain pointers */
    pp = (memAllocBlock_t*) p;
    pp->magic = MEMALLOC_MAGIC;
    pp->ptr = DATA_PTR(pp);
    if (pp->nxt != NULL)
      pp->nxt->prv = pp;
    if (pp->prv != NULL)
      pp->prv->nxt = pp;
    else
      MEMALLOC_DB = (void*) pp;
    /* return with data pointer */
    return DATA_PTR(pp);
}

void all_free(void *csound)
{
    memAllocBlock_t *pp, *nxtp;

    if (MEMALLOC_DB == NULL)
      return;           /* no allocs to free */
    rlsmemfiles(csound);
    pp = (memAllocBlock_t*) MEMALLOC_DB;
    MEMALLOC_DB = NULL;
    do {
      nxtp = pp->nxt;
      free((void*) pp);
      pp = nxtp;
    } while (pp != NULL);
}

void memRESET(ENVIRON *csound)
{
    all_free(csound);
    /* RWD 9:2000 not terribly vital, but good to do this somewhere... */
    pvsys_release(csound);
}

