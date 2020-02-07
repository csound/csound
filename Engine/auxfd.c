/*
  auxfd.c:

  Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include "csoundCore.h"                         /*      AUXFD.C         */

static CS_NOINLINE void auxchprint(CSOUND *, INSDS *);
static CS_NOINLINE void fdchprint(CSOUND *, INSDS *);

/* allocate an auxds, or expand an old one */
/*    call only from init (xxxset) modules */

void csoundAuxAlloc(CSOUND *csound, size_t nbytes, AUXCH *auxchp)
{
    if (auxchp->auxp != NULL) {
      /* if allocd with same size, just clear to zero */
      if (nbytes == (size_t)auxchp->size) {
        memset(auxchp->auxp, 0, nbytes);
        return;
      }
      else {
        void  *tmp = auxchp->auxp;
        /* if size change only, free the old space and re-allocate */
        auxchp->auxp = NULL;
        csound->Free(csound, tmp);
      }
    }
    else {                                  /* else link in new auxch blk */
      auxchp->nxtchp = csound->curip->auxchp;
      csound->curip->auxchp = auxchp;
    }
    /* now alloc the space and update the internal data */
    auxchp->size = nbytes;
    auxchp->auxp = csound->Calloc(csound, nbytes);
    auxchp->endp = (char*)auxchp->auxp + nbytes;
    if (UNLIKELY(csound->oparms->odebug))
      auxchprint(csound, csound->curip);
}


static uintptr_t alloc_thread(void *p) {
    AUXASYNC *pp = (AUXASYNC *) p;
    CSOUND *csound = pp->csound;
    AUXCH newm;
    char *ptr;
    if (pp->auxchp->auxp == NULL) {
      /* Allocate new memory */
      newm.size = pp->nbytes;
      newm.auxp = csound->Calloc(csound, pp->nbytes);
      newm.endp = (char*) newm.auxp + pp->nbytes;
      ptr = (char *) newm.auxp;
      newm  = *(pp->notify(csound, pp->userData, &newm));
      /* check that the returned pointer is not
         NULL and that is not the memory we have
         just allocated in case the old memory was
         never swapped back.
      */
      if (newm.auxp != NULL && newm.auxp != ptr)
        csound->Free(csound, newm.auxp);
    } else {
      csoundAuxAlloc(csound,pp->nbytes,pp->auxchp);
      pp->notify(csound, pp->userData, pp->auxchp);
    }
    return 0;
}



/* Allocate an auxds asynchronously and
   pass the newly allocated memory via a
   callback, where it can be swapped if necessary.
*/
int csoundAuxAllocAsync(CSOUND *csound, size_t nbytes, AUXCH *auxchp,
                        AUXASYNC *as, aux_cb cb, void *userData) {
    as->csound = csound;
    as->nbytes = nbytes;
    as->auxchp = auxchp;
    as->notify = cb;
    as->userData = userData;
    if (UNLIKELY(csoundCreateThread(alloc_thread, as) == NULL))
      return CSOUND_ERROR;
    else
      return CSOUND_SUCCESS;
}


/* put fdchp into chain of fd's for this instr */
/*      call only from init (xxxset) modules   */

void fdrecord(CSOUND *csound, FDCH *fdchp)
{
    fdchp->nxtchp = csound->curip->fdchp;
    csound->curip->fdchp = fdchp;
    if (UNLIKELY(csound->oparms->odebug))
      fdchprint(csound, csound->curip);
}

/* close a file and remove from fd chain */
/*  call only from inits, after fdrecord */

void csound_fd_close(CSOUND *csound, FDCH *fdchp)
{
    FDCH    *prvchp = NULL, *nxtchp;

    nxtchp = csound->curip->fdchp;              /* from current insds,  */
    while (LIKELY(nxtchp != NULL)) {            /* chain through fdlocs */
      if (UNLIKELY(nxtchp == fdchp)) {          /*   till find this one */
        void  *fd = fdchp->fd;
        if (LIKELY(fd)) {
          fdchp->fd = NULL;                     /* then delete the fd   */
          csoundFileClose(csound, fd);          /*   close the file &   */
        }
        if (prvchp)
          prvchp->nxtchp = fdchp->nxtchp;       /* unlnk from fdchain   */
        else
          csound->curip->fdchp = fdchp->nxtchp;
        if (UNLIKELY(csound->oparms->odebug))
          fdchprint(csound, csound->curip);
        return;
      }
      prvchp = nxtchp;
      nxtchp = nxtchp->nxtchp;
    }
    fdchprint(csound, csound->curip);
    csound->Die(csound, Str("csound_fd_close: no record of fd %p"), fdchp->fd);
}

/* release all xds in instr auxp chain */
/*   called by insert at orcompact     */

void auxchfree(CSOUND *csound, INSDS *ip)
{
    if (UNLIKELY(csound->oparms->odebug))
      auxchprint(csound, ip);
    while (LIKELY(ip->auxchp != NULL)) {        /* for all auxp's in chain: */
      void  *auxp = (void*) ip->auxchp->auxp;
      AUXCH *nxt = ip->auxchp->nxtchp;
      memset((void*) ip->auxchp, 0, sizeof(AUXCH)); /*  delete the pntr     */
      csound->Free(csound, auxp);                   /*  & free the space    */
      ip->auxchp = nxt;
    }
    if (UNLIKELY(csound->oparms->odebug))
      auxchprint(csound, ip);
}

/* close all files in instr fd chain        */
/* called by insert on deact & expire       */
/* (also musmon on s-code, & fgens for gen01) */

void fdchclose(CSOUND *csound, INSDS *ip)
{
    if (UNLIKELY(csound->oparms->odebug))
      fdchprint(csound, ip);
    /* for all fd's in chain: */
    for ( ; ip->fdchp != NULL; ip->fdchp = ip->fdchp->nxtchp) {
      void  *fd = ip->fdchp->fd;
      if (LIKELY(fd)) {
        ip->fdchp->fd = NULL;           /*    delete the fd     */
        csoundFileClose(csound, fd);    /*    & close the file  */
      }
    }
    if (UNLIKELY(csound->oparms->odebug))
      fdchprint(csound, ip);
}

/* print the xp chain for this insds blk */

static CS_NOINLINE void auxchprint(CSOUND *csound, INSDS *ip)
{
    AUXCH *curchp;
    char *name = csound->engineState.instrtxtp[ip->insno]->insname;

    if (name)
      csoundMessage(csound, Str("auxlist for instr %s [%d] (%p):\n"),
                      name, ip->insno, ip);
    else
      csoundMessage(csound, Str("auxlist for instr %d (%p):\n"),
                      ip->insno, ip);
    /* chain through auxlocs */
    for (curchp = ip->auxchp; curchp != NULL; curchp = curchp->nxtchp)
      csoundMessage(csound,
                      Str("\tauxch at %p: size %zu, auxp %p, endp %p\n"),
                      curchp, curchp->size, curchp->auxp, curchp->endp);
}

/* print the fd chain for this insds blk */

static CS_NOINLINE void fdchprint(CSOUND *csound, INSDS *ip)
{
    FDCH *curchp;
    char *name = csound->engineState.instrtxtp[ip->insno]->insname;

    if (name)
      csoundMessage(csound, Str("fdlist for instr %s [%d] (%p):"),
                      name, ip->insno, ip);
    else
      csoundMessage(csound, Str("fdlist for instr %d (%p):"), ip->insno, ip);
    /* chain through fdlocs */
    for (curchp = ip->fdchp; curchp != NULL; curchp = curchp->nxtchp)
      csoundMessage(csound, Str("  fd %p in %p"), curchp->fd, curchp);
    csoundMessage(csound, "\n");
}
