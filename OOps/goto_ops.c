/*
    goto_ops.c:

    Copyright (C) 1991, 1997, 1999, 2002, 2005
    Barry Vercoe, Istvan Varga, John ffitch,
    Gabriel Maldonado, matt ingalls

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

#include "cs.h"         /*                            GOTO_OPS.C        */
#include "oload.h"
#include "insert.h"     /* for goto's */
#include "aops.h"       /* for cond's */

int igoto(ENVIRON *csound, GOTO *p)
{
    csound->ids = p->lblblk->prvi;
    return OK;
}

int kgoto(ENVIRON *csound, GOTO *p)
{
    csound->pds = p->lblblk->prvp;
    return OK;
}

int icgoto(ENVIRON *csound, CGOTO *p)
{
    if (*p->cond)
      csound->ids = p->lblblk->prvi;
    return OK;
}

int kcgoto(ENVIRON *csound, CGOTO *p)
{
    if (*p->cond)
      csound->pds = p->lblblk->prvp;
    return OK;
}

/* an 'if-then' variant of 'if-goto' */
int ingoto(ENVIRON *csound, CGOTO *p)
{
    /* Make sure we have an i-time conditional */
    if (p->h.optext->t.intype == 'b' && !*p->cond)
      csound->pds = p->lblblk->prvp;
    return OK;
}

int kngoto(ENVIRON *csound, CGOTO *p)
{
    if (!*p->cond)
      csound->pds = p->lblblk->prvp;
    return OK;
}

int timset(ENVIRON *csound, TIMOUT *p)
{
    if ((p->cnt1 = (long)(*p->idel * csound->ekr + FL(0.5))) < 0L
        || (p->cnt2 = (long)(*p->idur * csound->ekr + FL(0.5))) < 0L)
      return csoundInitError(csound, Str("negative time period"));
    return OK;
}

int timout(ENVIRON *csound, TIMOUT *p)
{
    if (p->cnt1)                            /* once delay has expired, */
      p->cnt1--;
    else if (--p->cnt2 >= 0L)               /*  br during idur countdown */
      csound->pds = p->lblblk->prvp;
    return OK;
}

int rireturn(ENVIRON *csound, LINK *p)
{
    IGN(p);
    return OK;
}

int reinit(ENVIRON *csound, GOTO *p)
{
    csound->reinitflag = 1;
    csound->curip = p->h.insdshead;
    csound->ids = p->lblblk->prvi;        /* now, despite ANSI C warning:  */
    while ((csound->ids = csound->ids->nxti) != NULL &&
           csound->ids->iopadr != (SUBR) rireturn)
      (*csound->ids->iopadr)(csound, csound->ids);
    csound->reinitflag = 0;
    return OK;
}

int rigoto(ENVIRON *csound, GOTO *p)
{
    if (csound->reinitflag)
      csound->ids = p->lblblk->prvi;
    return OK;
}

int tigoto(ENVIRON *csound, GOTO *p)    /* I-time only, NOP at reinit */
{
    if (csound->tieflag && !csound->reinitflag)
      csound->ids = p->lblblk->prvi;
    return OK;
}

int tival(ENVIRON *csound, EVAL *p)     /* I-time only, NOP at reinit */
{
    if (!csound->reinitflag)
      *p->r = (csound->tieflag ? FL(1.0) : FL(0.0));
    return OK;
}

int ihold(ENVIRON *csound, LINK *p)     /* make this note indefinit duration */
{                                       /* called by ihold statmnt at Itime  */
    if (!csound->reinitflag) {          /* no-op at reinit                   */
      csound->curip->offbet = -1.0;
      csound->curip->offtim = -1.0;
    }
    return OK;
}

