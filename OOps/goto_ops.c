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

#include "csoundCore.h" /*                            GOTO_OPS.C        */
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
      csound->ids = p->lblblk->prvi;
    return OK;
}

int kngoto(ENVIRON *csound, CGOTO *p)
{
    if (!*p->cond)
      csound->pds = p->lblblk->prvp;
    return OK;
}

/* an i-rate version that ALWAYS jumps at p-time */

int iingoto(ENVIRON *csound, CGOTO *p)
{
    if (!*p->cond)
      csound->ids = p->lblblk->prvi;
    return OK;
}

int kingoto(ENVIRON *csound, CGOTO *p)
{
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

int turnoff(ENVIRON *csound, LINK *p)   /* terminate the current instrument  */
{                                       /* called by turnoff statmt at Ptime */
    INSDS  *lcurip = csound->pds->insdshead;
    /* IV - Oct 16 2002: check for subinstr and user opcode */
    /* find top level instrument instance */
    while (lcurip->opcod_iobufs)
      lcurip = ((OPCOD_IOBUFS*) lcurip->opcod_iobufs)->parent_ip;
    xturnoff(csound, lcurip);
    if (lcurip->xtratim <= 0)
      while (csound->pds->nxtp != NULL)
        csound->pds = csound->pds->nxtp;                /* loop to last opds */
    return OK;
}

/* turnoff2 opcode */

int turnoff2(ENVIRON *csound, TURNOFF2 *p)
{
    MYFLT p1;
    INSDS *ip, *ip2;
    int   mode, insno, allow_release;

    if (*(p->kInsNo) <= FL(0.0))
      return OK;    /* not triggered */
    p1 = *(p->kInsNo);
    insno = (int) p1;
    if (insno < 1 || insno > (int) csound->maxinsno ||
        csound->instrtxtp[insno] == NULL) {
      csoundPerfError(csound, Str("turnoff2: invalid instrument number"));
      return NOTOK;
    }
    mode = (int) (*(p->kFlags) + FL(0.5));
    allow_release = (*(p->kRelease) == FL(0.0) ? 0 : 1);
    if (mode < 0 || mode > 15 || (mode & 3) == 3) {
      csoundPerfError(csound, Str("turnoff2: invalid mode parameter"));
      return NOTOK;
    }
    ip = &(csound->actanchor);
    ip2 = NULL;
    while ((ip = ip->nxtact) != NULL && (int) ip->insno != insno);
    if (ip == NULL)
      return OK;
    do {
      if (((mode & 8) && ip->offtim >= 0.0) ||
          ((mode & 4) && ip->p1 != p1) ||
          (allow_release && ip->relesing))
        continue;
      if (!(mode & 3)) {
        if (allow_release)
          xturnoff(csound, ip);
        else
          xturnoff_now(csound, ip);
      }
      else {
        ip2 = ip;
        if ((mode & 3) == 1)
          break;
      }
    } while ((ip = ip->nxtact) != NULL && (int) ip->insno == insno);
    if (ip2 != NULL) {
      if (allow_release)
        xturnoff(csound, ip2);
      else
        xturnoff_now(csound, ip2);
    }
    if (!p->h.insdshead->actflg) {  /* if current note was deactivated: */
      while (csound->pds->nxtp != NULL)
        csound->pds = csound->pds->nxtp;            /* loop to last opds */
    }
    return OK;
}

int loop_l_i(ENVIRON *csound, LOOP_OPS *p)
{
    /* if ((indxvar += iincr) < ilimit) igoto l */
    *(p->ndxvar) += *(p->incr);
    if (*(p->ndxvar) < *(p->limit))
      csound->ids = p->l->prvi;
    return OK;
}

int loop_le_i(ENVIRON *csound, LOOP_OPS *p)
{
    /* if ((indxvar += iincr) <= ilimit) igoto l */
    *(p->ndxvar) += *(p->incr);
    if (*(p->ndxvar) <= *(p->limit))
      csound->ids = p->l->prvi;
    return OK;
}

int loop_g_i(ENVIRON *csound, LOOP_OPS *p)
{
    /* if ((indxvar -= idecr) > ilimit) igoto l */
    *(p->ndxvar) -= *(p->incr);
    if (*(p->ndxvar) > *(p->limit))
      csound->ids = p->l->prvi;
    return OK;
}

int loop_ge_i(ENVIRON *csound, LOOP_OPS *p)
{
    /* if ((indxvar -= idecr) >= ilimit) igoto l */
    *(p->ndxvar) -= *(p->incr);
    if (*(p->ndxvar) >= *(p->limit))
      csound->ids = p->l->prvi;
    return OK;
}

int loop_l_p(ENVIRON *csound, LOOP_OPS *p)
{
    /* if ((kndxvar += kincr) < klimit) kgoto l */
    *(p->ndxvar) += *(p->incr);
    if (*(p->ndxvar) < *(p->limit))
      csound->pds = p->l->prvp;
    return OK;
}

int loop_le_p(ENVIRON *csound, LOOP_OPS *p)
{
    /* if ((kndxvar += kincr) <= klimit) kgoto l */
    *(p->ndxvar) += *(p->incr);
    if (*(p->ndxvar) <= *(p->limit))
      csound->pds = p->l->prvp;
    return OK;
}

int loop_g_p(ENVIRON *csound, LOOP_OPS *p)
{
    /* if ((kndxvar -= kdecr) > klimit) kgoto l */
    *(p->ndxvar) -= *(p->incr);
    if (*(p->ndxvar) > *(p->limit))
      csound->pds = p->l->prvp;
    return OK;
}

int loop_ge_p(ENVIRON *csound, LOOP_OPS *p)
{
    /* if ((kndxvar -= kdecr) >= klimit) kgoto l */
    *(p->ndxvar) -= *(p->incr);
    if (*(p->ndxvar) >= *(p->limit))
      csound->pds = p->l->prvp;
    return OK;
}

