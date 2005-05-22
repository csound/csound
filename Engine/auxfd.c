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

#include "cs.h"                                 /*      AUXFD.C         */
#include <sndfile.h>

static void auxrecord(ENVIRON *, AUXCH *);
static void auxchprint(ENVIRON *, INSDS *);
static void fdchprint(ENVIRON *, INSDS *);

void csoundAuxAlloc(void *csound_, long nbytes, AUXCH *auxchp)
     /* allocate an auxds, or expand an old one */
     /*    call only from init (xxxset) modules */
{
    ENVIRON *csound = (ENVIRON*) csound_;
    void    *auxp;

    if ((auxp = auxchp->auxp) != NULL)  /* if size change only,      */
      mfree(csound, auxp);              /*      free the old space   */
    else
      auxrecord(csound, auxchp);        /* else linkin new auxch blk */
    auxp = mcalloc(csound, nbytes);     /* now alloc the space       */
    auxchp->size = nbytes;              /* update the internal data  */
    auxchp->auxp = auxp;
    auxchp->endp = (char*) auxp + nbytes;
    if (csound->oparms->odebug)
      auxchprint(csound, csound->curip);
}

static void auxrecord(ENVIRON *csound, AUXCH *auxchp)
     /* put auxch into chain of xp's for this instr */
     /* called only from auxalloc       */
{
    AUXCH       *prvchp, *nxtchp;

    prvchp = &(csound->curip->auxch);           /* from current insds,  */
    while ((nxtchp = prvchp->nxtchp) != NULL)   /* chain through xplocs */
      prvchp = nxtchp;
    prvchp->nxtchp = auxchp;                    /* then add this xploc  */
    auxchp->nxtchp = NULL;                      /* & terminate the chain */
}

void fdrecord(ENVIRON *csound, FDCH *fdchp)
{                             /* put fdchp into chain of fd's for this instr */
    FDCH    *prvchp, *nxtchp; /*      call only from init (xxxset) modules   */

    prvchp = &(csound->curip->fdch);            /* from current insds,  */
    while ((nxtchp = prvchp->nxtchp) != NULL)   /* chain through fdlocs */
      prvchp = nxtchp;
    prvchp->nxtchp = fdchp;                     /* then add this fdloc  */
    fdchp->nxtchp = NULL;                       /* & terminate the chain */
    if (csound->oparms->odebug)
      fdchprint(csound, csound->curip);
}

void fdclose(ENVIRON *csound, FDCH *fdchp)
{                               /* close a file and remove from fd chain */
    FDCH    *prvchp, *nxtchp;   /*  call only from inits, after fdrecord */

    prvchp = &(csound->curip->fdch);            /* from current insds,  */
    while ((nxtchp = prvchp->nxtchp) != NULL) { /* chain through fdlocs */
      if (nxtchp == fdchp) {                    /*   till find this one */
        if (fdchp->fd) {
          csoundFileClose(csound, fdchp->fd);   /* then close the file  */
          fdchp->fd = NULL;                     /*   delete the fd &    */
        }
        prvchp->nxtchp = fdchp->nxtchp;         /* unlnk from fdchain   */
        if (csound->oparms->odebug)
          fdchprint(csound, csound->curip);
        return;
      }
      else prvchp = nxtchp;
    }
    fdchprint(csound, csound->curip);
    csound->Die(csound, Str("fdclose: no record of fd %p"), fdchp->fd);
}

void auxchfree(void *csound, INSDS *ip) /* release all xds in instr auxp chain*/
{                                       /*   called by insert at orcompact    */
    ENVIRON *p = (ENVIRON*) csound;
    AUXCH   *curchp = &ip->auxch;
    char    *auxp;

    if (p->oparms->odebug)
      auxchprint(p, ip);
    while ((curchp = curchp->nxtchp) != NULL) { /* for all xp's in chain: */
      if ((auxp = curchp->auxp) == NULL) {
        auxchprint(p, ip);
        csoundDie(csound, Str("auxchfree: illegal auxp %p in chain"), auxp);
      }
      mfree(csound, auxp);                      /*      free the space    */
      curchp->auxp = NULL;                      /*      & delete the pntr */
    }
    ip->auxch.nxtchp = NULL;                    /* finally, delete the chain */
    if (p->oparms->odebug)
      auxchprint(p, ip);
}

void fdchclose(ENVIRON *csound, INSDS *ip)
{                               /* close all files in instr fd chain        */
    FDCH  *curchp = &ip->fdch;  /* called by insert on deact & expire       */
                                /* (also musmon on s-code, & fgens for gen01) */
    if (csound->oparms->odebug)
      fdchprint(csound, ip);
    while ((curchp = curchp->nxtchp) != NULL) { /* for all fd's in chain: */
      if (curchp->fd) {
        csoundFileClose(csound, curchp->fd);    /*      close the file  */
        curchp->fd = NULL;                      /*      & delete the fd */
      }
    }
    ip->fdch.nxtchp = NULL;                     /* finally, delete the chain */
    if (csound->oparms->odebug)
      fdchprint(csound, ip);
}

static void auxchprint(ENVIRON *csound, INSDS *ip)
{                                   /* print the xp chain for this insds blk */
    AUXCH   *curchp = &ip->auxch;

    csound->Message(csound, Str("auxlist for instr %d (%p):\n"), ip->insno, ip);
    while ((curchp = curchp->nxtchp) != NULL)   /* chain through auxlocs */
      csound->Message(csound,
                      Str("\tauxch at %p: size %ld, auxp %p, endp %p\n"),
                      curchp, curchp->size, curchp->auxp, curchp->endp);
}

static void fdchprint(ENVIRON *csound, INSDS *ip)
{                                   /* print the fd chain for this insds blk */
    FDCH    *curchp = &ip->fdch;

    csound->Message(csound, Str("fdlist for instr %d (%p):"), ip->insno, ip);
    while ((curchp = curchp->nxtchp) != NULL)    /* chain through fdlocs */
      csound->Message(csound, Str("  fd %p in %p"), curchp->fd, curchp);
    csound->Message(csound, "\n");
}

