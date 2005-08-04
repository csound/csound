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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csoundCore.h"                         /*      AUXFD.C         */

static CS_NOINLINE void auxchprint(ENVIRON *, INSDS *);
static CS_NOINLINE void fdchprint(ENVIRON *, INSDS *);

/* allocate an auxds, or expand an old one */
/*    call only from init (xxxset) modules */

void csoundAuxAlloc(void *csound_, long nbytes, AUXCH *auxchp)
{
    register ENVIRON *csound = (ENVIRON*) csound_;

    if (auxchp->auxp != NULL) {
      /* if allocd with same size, just clear to zero */
      if (nbytes == (long) auxchp->size) {
        memset(auxchp->auxp, 0, (size_t) nbytes);
        return;
      }
      else {
        void  *tmp = auxchp->auxp;
        /* if size change only, free the old space and re-allocate */
        auxchp->auxp = NULL;
        mfree(csound, tmp);
      }
    }
    else {                                  /* else linkin new auxch blk */
      auxchp->nxtchp = csound->curip->auxchp;
      csound->curip->auxchp = auxchp;
    }
    /* now alloc the space and update the internal data */
    auxchp->size = nbytes;
    auxchp->auxp = mcalloc(csound, nbytes);
    auxchp->endp = (char*) auxchp->auxp + nbytes;
    if (csound->oparms->odebug)
      auxchprint(csound, csound->curip);
}

/* put fdchp into chain of fd's for this instr */
/*      call only from init (xxxset) modules   */

void fdrecord(ENVIRON *csound, FDCH *fdchp)
{
    fdchp->nxtchp = csound->curip->fdchp;
    csound->curip->fdchp = fdchp;
    if (csound->oparms->odebug)
      fdchprint(csound, csound->curip);
}

/* close a file and remove from fd chain */
/*  call only from inits, after fdrecord */

void fdclose(ENVIRON *csound, FDCH *fdchp)
{
    FDCH    *prvchp = NULL, *nxtchp;

    nxtchp = csound->curip->fdchp;              /* from current insds,  */
    while (nxtchp != NULL) {                    /* chain through fdlocs */
      if (nxtchp == fdchp) {                    /*   till find this one */
        void  *fd = fdchp->fd;
        if (fd) {
          fdchp->fd = NULL;                     /* then delete the fd   */
          csoundFileClose(csound, fd);          /*   close the file &   */
        }
        if (prvchp)
          prvchp->nxtchp = fdchp->nxtchp;       /* unlnk from fdchain   */
        else
          csound->curip->fdchp = fdchp->nxtchp;
        if (csound->oparms->odebug)
          fdchprint(csound, csound->curip);
        return;
      }
      prvchp = nxtchp;
      nxtchp = nxtchp->nxtchp;
    }
    fdchprint(csound, csound->curip);
    csound->Die(csound, Str("fdclose: no record of fd %p"), fdchp->fd);
}

/* release all xds in instr auxp chain */
/*   called by insert at orcompact     */

void auxchfree(void *csound, INSDS *ip)
{
    ENVIRON *p = (ENVIRON*) csound;

    if (p->oparms->odebug)
      auxchprint(p, ip);
    while (ip->auxchp != NULL) {                /* for all auxp's in chain: */
      void  *auxp = (void*) ip->auxchp->auxp;
      AUXCH *nxt = ip->auxchp->nxtchp;
      memset((void*) ip->auxchp, 0, sizeof(AUXCH)); /*  delete the pntr     */
      mfree(p, auxp);                               /*  & free the space    */
      ip->auxchp = nxt;
    }
    if (p->oparms->odebug)
      auxchprint(p, ip);
}

/* close all files in instr fd chain        */
/* called by insert on deact & expire       */
/* (also musmon on s-code, & fgens for gen01) */

void fdchclose(ENVIRON *csound, INSDS *ip)
{
    if (csound->oparms->odebug)
      fdchprint(csound, ip);
    /* for all fd's in chain: */
    for ( ; ip->fdchp != NULL; ip->fdchp = ip->fdchp->nxtchp) {
      void  *fd = ip->fdchp->fd;
      if (fd) {
        ip->fdchp->fd = NULL;           /*    delete the fd     */
        csoundFileClose(csound, fd);    /*    & close the file  */
      }
    }
    if (csound->oparms->odebug)
      fdchprint(csound, ip);
}

/* print the xp chain for this insds blk */

static CS_NOINLINE void auxchprint(ENVIRON *csound, INSDS *ip)
{
    AUXCH   *curchp;

    csound->Message(csound, Str("auxlist for instr %d (%p):\n"), ip->insno, ip);
    /* chain through auxlocs */
    for (curchp = ip->auxchp; curchp != NULL; curchp = curchp->nxtchp)
      csound->Message(csound,
                      Str("\tauxch at %p: size %ld, auxp %p, endp %p\n"),
                      curchp, curchp->size, curchp->auxp, curchp->endp);
}

/* print the fd chain for this insds blk */

static CS_NOINLINE void fdchprint(ENVIRON *csound, INSDS *ip)
{
    FDCH    *curchp;

    csound->Message(csound, Str("fdlist for instr %d (%p):"), ip->insno, ip);
    /* chain through fdlocs */
    for (curchp = ip->fdchp; curchp != NULL; curchp = curchp->nxtchp)
      csound->Message(csound, Str("  fd %p in %p"), curchp->fd, curchp);
    csound->Message(csound, "\n");
}

