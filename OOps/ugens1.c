/*
    ugens1.c:

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

#include "cs.h"                 /*                      UGENS1.C        */
#include "ugens1.h"
#include <math.h>

#define FHUND (FL(100.0))

int linset(ENVIRON *csound, LINE *p)
{
    MYFLT       dur;

    if ((dur = *p->idur) > FL(0.0)) {
      p->incr = (*p->ib - *p->ia) / dur * csound->onedkr;
      p->val = *p->ia;
    }
    return OK;
}

int kline(ENVIRON *csound, LINE *p)
{
    *p->xr = p->val;            /* rslt = val   */
    p->val += p->incr;          /* val += incr  */
    return OK;
}

int aline(ENVIRON *csound, LINE *p)
{
    MYFLT       val, inc, *ar;
    int n, nsmps=csound->ksmps;

    val = p->val;
    inc = p->incr;
    p->val += inc;              /* nxtval = val + inc */
    inc *= csound->onedksmps;
    ar = p->xr;
    for (n=0; n<nsmps; n++) {
      ar[n] = val;
      val += inc;       /* interp val for ksmps */
    }
    return OK;
}

int expset(ENVIRON *csound, EXPON *p)
{
    MYFLT       dur, a, b;

    if ((dur = *p->idur) > FL(0.0) ) {
      a = *p->ia;
      b = *p->ib;
      if ((a * b) > FL(0.0)) {
        p->mlt = (MYFLT) pow((double)(b / a),(csound->onedkr/(double)dur));
        p->val = a;
      }
      else if (a == FL(0.0))
        return csound->InitError(csound, Str("arg1 is zero"));
      else if (b == FL(0.0))
        return csound->InitError(csound, Str("arg2 is zero"));
      else return csound->InitError(csound, Str("unlike signs"));
    }
    return OK;
}

int kexpon(ENVIRON *csound, EXPON *p)
{
    *p->xr = p->val;            /* rslt = val   */
    p->val *= p->mlt;           /* val *= mlt  */
    return OK;
}

int expon(ENVIRON *csound, EXPON *p)
{
    MYFLT       val, mlt, inc, *ar,nxtval;
    int n, nsmps=csound->ksmps;

    val = p->val;
    mlt = p->mlt;
    nxtval = val * mlt;
    inc = nxtval - val;
    inc *= csound->onedksmps;   /* increment per sample */
    ar = p->xr;
    for (n=0; n<nsmps; n++) {
      ar[n] = val;
      val += inc;               /* interp val for ksmps */
    }
    p->val = nxtval;            /* store next value */
    return OK;
}

/* static int counter; */
int lsgset(ENVIRON *csound, LINSEG *p)
{
    SEG *segp;
    int nsegs;
    MYFLT       **argp, val;

    nsegs = p->INOCOUNT >> 1;           /* count segs & alloc if nec */
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(SEG) < (unsigned int)p->auxch.size) {
      csound->AuxAlloc(csound, (long)nsegs*sizeof(SEG), &p->auxch);
      p->cursegp = segp = (SEG *) p->auxch.auxp;
      segp[nsegs-1].cnt = MAXPOS; /* set endcount for safety */
    }
    argp = p->argums;
    val = **argp++;
    if (**argp <= FL(0.0))  return OK;    /* if idur1 <= 0, skip init  */
    p->curval = val;
    p->curcnt = 0;
    p->cursegp = segp - 1;          /* else setup null seg0 */
    p->segsrem = nsegs + 1;
    do {                                /* init each seg ..  */
      MYFLT dur = **argp++;
      segp->nxtpt = **argp++;
      if ((segp->cnt = (long)(dur * csound->ekr + FL(0.5))) < 0)
        segp->cnt = 0;
      segp++;
    } while (--nsegs);
    p->xtra = -1;
/*     { */
/*       int i; */
/*       for (i=0; i<p->segsrem-1; i++) */
/*       counter = 0; */
/*     } */
    return OK;
}

int klnseg(ENVIRON *csound, LINSEG *p)
{
    *p->rslt = p->curval;               /* put the cur value    */
    if (p->auxch.auxp==NULL) {          /* RWD fix */
      csound->Die(csound, Str("\nError: linseg not initialised (krate)\n"));
    }
    if (p->segsrem) {                   /* done if no more segs */
      if (--p->curcnt <= 0) {           /* if done cur segment  */
        SEG *segp = p->cursegp;
        if (!(--p->segsrem))  {
          p->curval = segp->nxtpt;      /* advance the cur val  */
          return OK;
        }
        p->cursegp = ++segp;            /*   find the next      */
        if (!(p->curcnt = segp->cnt)) { /*   nonlen = discontin */
          p->curval = segp->nxtpt;      /*   poslen = new slope */
          /*          p->curval += p->curinc;  ??????? */
          return OK;
        }
        else {
          p->curinc = (segp->nxtpt - p->curval) / segp->cnt;
          p->curval += p->curinc;
          return OK;
        }
      }
      if (p->curcnt<10)         /* This is a fiddle to get rounding right!  */
        p->curinc = (p->cursegp->nxtpt - p->curval) / p->curcnt; /* recalc */
      p->curval += p->curinc;           /* advance the cur val  */
    }
/*     counter++; */
    return OK;
}

int linseg(ENVIRON *csound, LINSEG *p)
{
    MYFLT  val, ainc, *rs = p->rslt;
    int         n, nsmps=csound->ksmps;

    if (p->auxch.auxp==NULL) {  /* RWD fix */
      return csound->PerfError(csound, Str("linseg: not initialised (arate)\n"));
    }

    val = p->curval;                      /* sav the cur value    */
    if (p->segsrem) {                     /* if no more segs putk */
      if (--p->curcnt <= 0) {             /*  if done cur segment */
        SEG *segp = p->cursegp;
      chk1:
        if (!--p->segsrem) {              /*   if none left       */
          val = p->curval = segp->nxtpt;
          goto putk;                      /*      put endval      */
        }
        p->cursegp = ++segp;              /*   else find the next */
        if (!(p->curcnt = segp->cnt)) {
          val = p->curval = segp->nxtpt;  /* nonlen = discontin */
          goto chk1;
        }                                 /*   poslen = new slope */
        p->curinc = (segp->nxtpt - val) / segp->cnt;
        p->curainc = p->curinc * csound->onedksmps;
      }
      p->curval = val + p->curinc;        /* advance the cur val  */
      if ((ainc = p->curainc) == FL(0.0))
        goto putk;
      for (n=0; n<nsmps; n++) {
        rs[n] = val;
        val += ainc;
      }
    }
    else {
    putk:
      for (n=0; n<nsmps; n++) {
        rs[n] = val;
      }
    }
    return OK;
}

/* **** ADSR is just a construction and use of linseg */

static void adsrset1(ENVIRON *csound, LINSEG *p, int midip)
{
    SEG         *segp;
    int         nsegs;
    MYFLT       **argp = p->argums;
    MYFLT       dur;
    MYFLT       len = csound->curip->p3;
    MYFLT       release = *argp[3];
    long        relestim;

    if (len<=FL(0.0)) len = FL(100000.0); /* MIDI case set long */
    len -= release;         /* len is time remaining */
    if (len<FL(0.0)) {         /* Odd case of release time greater than dur */
      release = csound->curip->p3; len = FL(0.0);
    }
    nsegs = 6;          /* DADSR */
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(SEG) < (unsigned int)p->auxch.size) {
      csoundAuxAlloc(csound, (long) nsegs * sizeof(SEG), &p->auxch);
      p->cursegp = segp = (SEG *) p->auxch.auxp;
      segp[nsegs-1].cnt = MAXPOS; /* set endcount for safety */
    }
    else if (**argp > FL(0.0))
      memset(p->auxch.auxp, 0, (long)nsegs*sizeof(SEG));
    if (**argp <= FL(0.0))  return;       /* if idur1 <= 0, skip init  */
    p->curval = FL(0.0);
    p->curcnt = 0;
    p->cursegp = segp - 1;      /* else setup null seg0 */
    p->segsrem = nsegs;
                                /* Delay */
    dur = *argp[4];
    if (dur > len) dur = len;
    len -= dur;
    segp->nxtpt = FL(0.0);
    if ((segp->cnt = (long)(dur * csound->ekr + FL(0.5))) == 0)
      segp->cnt = 0;
    segp++;
                                /* Attack */
    dur = *argp[0];
    if (dur > len) dur = len;
    len -= dur;
    segp->nxtpt = FL(1.0);
    if ((segp->cnt = (long)(dur * csound->ekr + FL(0.5))) == 0)
      segp->cnt = 0;
    segp++;
                                /* Decay */
    dur = *argp[1];
    if (dur > len) dur = len;
    len -= dur;
    segp->nxtpt = *argp[2];
    if ((segp->cnt = (long)(dur * csound->ekr + FL(0.5))) == 0)
      segp->cnt = 0;
    segp++;
                                /* Sustain */
    /* Should use p3 from score, but how.... */
    dur = len;
/*  dur = csound->curip->p3 - *argp[4] - *argp[0] - *argp[1] - *argp[3]; */
    segp->nxtpt = *argp[2];
    if ((segp->cnt = (long)(dur * csound->ekr + FL(0.5))) == 0)
      segp->cnt = 0;
    segp++;
                                /* Release */
    segp->nxtpt = FL(0.0);
    if ((segp->cnt = (long)(release * csound->ekr + FL(0.5))) == 0)
      segp->cnt = 0;
    if (midip) {
      p->xtra = (long)(*argp[5] * csound->ekr + FL(0.5));   /* Release time?? */
      relestim = (p->cursegp + p->segsrem - 1)->cnt;
      if (relestim > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = (int)relestim;
    }
    else
      p->xtra = 0L;
}

int adsrset(ENVIRON *csound, LINSEG *p)
{
    adsrset1(csound, p, 0);
    return OK;
}

int madsrset(ENVIRON *csound, LINSEG *p)
{
    adsrset1(csound, p, 1);
    return OK;
}

/* End of ADSR */

int lsgrset(ENVIRON *csound, LINSEG *p)
{
    long relestim;
    lsgset(csound,p);
    relestim = (p->cursegp + p->segsrem - 1)->cnt;
    p->xtra = -1;
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = (int)relestim;
    return OK;
}

int klnsegr(ENVIRON *csound, LINSEG *p)
{
    *p->rslt = p->curval;                   /* put the cur value    */
    if (p->segsrem) {                       /* done if no more segs */
      SEG *segp;
      if (p->h.insdshead->relesing && p->segsrem > 1) {
        while (p->segsrem > 1) {           /* reles flag new:      */
          segp = ++p->cursegp;             /*   go to last segment */
          p->segsrem--;
        }                                  /*   get univ relestim  */
        segp->cnt = p->xtra>= 0 ? p->xtra : p->h.insdshead->xtratim;
        goto newi;                         /*   and set new curinc */
      }
      if (--p->curcnt <= 0) {              /* if done cur seg      */
      chk2:
        if (p->segsrem == 2) return OK;    /*   seg Y rpts lastval */
        if (!(--p->segsrem)) return OK;    /*   seg Z now done all */
        segp = ++p->cursegp;               /*   else find nextseg  */
      newi:
        if (!(p->curcnt = segp->cnt)) {    /*   nonlen = discontin */
          p->curval = segp->nxtpt;         /*     reload & rechk   */
          goto chk2;
        }                                  /*   else get new slope */
        p->curinc = (segp->nxtpt - p->curval) / segp->cnt;
      }
      p->curval += p->curinc;              /* advance the cur val  */
    }
    return OK;
}

int linsegr(ENVIRON *csound, LINSEG *p)
{
    MYFLT  val, ainc, *rs = p->rslt;
    int    n, nsmps=csound->ksmps;

    val = p->curval;                        /* sav the cur value    */
    if (p->segsrem) {                       /* if no more segs putk */
      SEG *segp;
      if (p->h.insdshead->relesing && p->segsrem > 1) {
        while (p->segsrem > 1) {            /* reles flag new:      */
          segp = ++p->cursegp;              /*   go to last segment */
          p->segsrem--;
        }                                   /*   get univ relestim  */
        segp->cnt = p->xtra >=0 ? p->xtra : p->h.insdshead->xtratim;
        goto newi;                          /*   and set new curinc */
      }
      if (--p->curcnt <= 0) {               /* if done cur seg      */
      chk2:
        if (p->segsrem == 2) goto putk;     /*   seg Y rpts lastval */
        if (!(--p->segsrem)) goto putk;     /*   seg Z now done all */
        segp = ++p->cursegp;                /*   else find nextseg  */
      newi:
        if (!(p->curcnt = segp->cnt)) {     /*   nonlen = discontin */
          val = p->curval = segp->nxtpt;    /*   reload & rechk  */
          goto chk2;
        }                                   /*   else get new slope */
        p->curinc = (segp->nxtpt - val) / segp->cnt;
        p->curainc = p->curinc * csound->onedksmps;
      }
      p->curval = val + p->curinc;          /* advance the cur val  */
      if ((ainc = p->curainc) == FL(0.0))
        goto putk;
      for (n=0; n<nsmps; n++) {
        rs[n] = val;
        val += ainc;
      }
    }
    else {
    putk:
      for (n=0;n<nsmps; n++) rs[n] = val;
    }
    return OK;
}

int xsgset(ENVIRON *csound, EXXPSEG *p)
{
    XSEG        *segp;
    int nsegs;
    MYFLT       d, **argp, val, dur, nxtval;
    int    n=0;

    nsegs = p->INOCOUNT >> 1;                   /* count segs & alloc if nec */
    if ((segp = (XSEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(XSEG) < (unsigned int)p->auxch.size) {
      csound->AuxAlloc(csound, (long)nsegs*sizeof(XSEG), &p->auxch);
      p->cursegp = segp = (XSEG *) p->auxch.auxp;
      (segp+nsegs-1)->cnt = MAXPOS;   /* set endcount for safety */
    }
    argp = p->argums;
    nxtval = **argp++;
    if (**argp <= FL(0.0))  return OK;          /* if idur1 <= 0, skip init  */
    p->cursegp = segp;                          /* else proceed from 1st seg */
    segp--;
    do {
      segp++;           /* init each seg ..  */
      val = nxtval;
      dur = **argp++;
      nxtval = **argp++;
      if (dur > FL(0.0)) {
        if (val * nxtval <= FL(0.0))
          goto experr;
        d = dur * csound->ekr;
        segp->val = val;
        segp->mlt = (MYFLT) pow((double)(nxtval / val), (1.0/(double)d));
        segp->cnt = (long) (d + FL(0.5));
      }
      else break;               /*  .. til 0 dur or done */
    } while (--nsegs);
    segp->cnt = MAXPOS;         /* set last cntr to infin */
    return OK;

 experr:
    n = segp - p->cursegp + 1;
    if (val == FL(0.0))
      sprintf(csound->errmsg, Str("ival%d is zero"), n);
    else if (nxtval == FL(0.0))
      sprintf(csound->errmsg, Str("ival%d is zero"), n+1);
    else sprintf(csound->errmsg, Str("ival%d sign conflict"), n+1);
    return csound->InitError(csound, csound->errmsg);
}

int xsgset2(ENVIRON *csound, EXPSEG2 *p)  /*gab-A1 (G.Maldonado) */
{
    XSEG        *segp;
    int         nsegs;
    MYFLT       d, **argp, val, dur, nxtval;
    int         n;

    nsegs = p->INOCOUNT >> 1;           /* count segs & alloc if nec */
    if ((segp = (XSEG*) p->auxch.auxp) == NULL ||
        (unsigned int)nsegs*sizeof(XSEG) > (unsigned int)p->auxch.size) {
      csound->AuxAlloc(csound, (long)nsegs*sizeof(XSEG), &p->auxch);
      p->cursegp = segp = (XSEG *) p->auxch.auxp;
      (segp+nsegs-1)->cnt = MAXPOS;   /* set endcount for safety */
    }
    argp = p->argums;
    nxtval = **argp++;
    if (**argp <= FL(0.0))  return OK;        /* if idur1 <= 0, skip init  */
    p->cursegp = segp;                      /* else proceed from 1st seg */
    segp--;
    do {
      segp++;           /* init each seg ..  */
      val = nxtval;
      dur = **argp++;
      nxtval = **argp++;
      if (dur > FL(0.0)) {
        if (val * nxtval <= FL(0.0))
          goto experr;
        d = dur * csound->esr;
        segp->val = val;
        segp->mlt = (MYFLT) pow((double)(nxtval / val), (1.0/(double)d));
        segp->cnt = (long) (d + FL(0.5));
      }
      else break;               /*  .. til 0 dur or done */
    } while (--nsegs);
    segp->cnt = MAXPOS;         /* set last cntr to infin */
    return OK;

 experr:
    n = segp - p->cursegp + 1;
    if (val == FL(0.0))
      sprintf(csound->errmsg, Str("ival%d is zero"), n);
    else if (nxtval == FL(0.0))
      sprintf(csound->errmsg, Str("ival%d is zero"), n+1);
    else sprintf(csound->errmsg, Str("ival%d sign conflict"), n+1);
    return NOTOK;
}

/***************************************/
int expseg2(ENVIRON *csound, EXPSEG2 *p)            /* gab-A1 (G.Maldonado) */
{
    XSEG        *segp;
    int         n;
    MYFLT       val, *rs;
    segp = p->cursegp;
    val = segp->val;
    rs = p->rslt;
    for (n=0; n<csound->ksmps; n++) {
      while (--segp->cnt < 0)   {
        p->cursegp = ++segp;
        val = segp->val;
      }
      rs[n] = val;
      val *=  segp->mlt;
    }
    segp->val = val;
    return OK;
}

/* **** XDSR is just a construction and use of expseg */

int xdsrset(ENVIRON *csound, EXXPSEG *p)
{
    XSEG        *segp;
    int nsegs;
    MYFLT       **argp = p->argums;
    MYFLT       len = csound->curip->p3;
    MYFLT   delay = *argp[4], attack = *argp[0], decay = *argp[1];
    MYFLT   sus, dur;
    MYFLT   release = *argp[3];

    if (len<FL(0.0)) len = FL(100000.0); /* MIDI case set long */
    len -= release;         /* len is time remaining */
    if (len<FL(0.0)) {         /* Odd case of release time greater than dur */
      release = csound->curip->p3; len = FL(0.0);
    }
    nsegs = 5;          /* DXDSR */
    if ((segp = (XSEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(XSEG) < (unsigned int)p->auxch.size) {
      csound->AuxAlloc(csound, (long)nsegs*sizeof(XSEG), &p->auxch);
      segp = (XSEG *) p->auxch.auxp;
    }
    segp[nsegs-1].cnt = MAXPOS;         /* set endcount for safety */
    if (**argp <= FL(0.0))  return OK;  /* if idur1 <= 0, skip init  */
    p->cursegp = segp;                  /* else setup null seg0 */
    p->segsrem = nsegs;
    delay += FL(0.001);
    if (delay > len) delay = len; len -= delay;
    attack -= FL(0.001);
    if (attack > len) attack = len; len -= attack;
    if (decay > len) decay = len; len -= decay;
    sus = len;
    segp[0].val = FL(0.001);   /* Like zero start, but exponential */
    segp[0].mlt = FL(1.0);
    segp[0].cnt = (long) (delay*csound->ekr + FL(0.5));
    dur = attack*csound->ekr;
    segp[1].val = FL(0.001);
    segp[1].mlt = (MYFLT) pow(1000.0, 1.0/(double)dur);
    segp[1].cnt = (long) (dur + FL(0.5));
    dur = decay*csound->ekr;
    segp[2].val = FL(1.0);
    segp[2].mlt = (MYFLT) pow((double)*argp[2], 1.0/(double)dur);
    segp[2].cnt = (long) (dur + FL(0.5));
    segp[3].val = *argp[2];
    segp[3].mlt = FL(1.0);
    segp[3].cnt = (long) (sus*csound->ekr + FL(0.5));
    dur = release*csound->ekr;
    segp[4].val = *argp[2];
    segp[4].mlt = (MYFLT) pow(0.001/(double)*argp[2], 1.0/(double)dur);
    segp[4].cnt = MAXPOS; /*(long) (dur + FL(0.5)); */
    return OK;
}

/* end of XDSR */

int kxpseg(ENVIRON *csound, EXXPSEG *p)
{
    XSEG        *segp;

    segp = p->cursegp;
    if (p->auxch.auxp==NULL) { /* RWD fix */
      return csound->PerfError(csound, Str("expseg (krate): not initialised"));
    }
    while (--segp->cnt < 0)
      p->cursegp = ++segp;
    *p->rslt = segp->val;
    segp->val *= segp->mlt;
    return OK;
}

int expseg(ENVIRON *csound, EXXPSEG *p)
{
    XSEG        *segp;
    int n;
    MYFLT       li, val, *rs;
    MYFLT       nxtval;

    segp = p->cursegp;
    if (p->auxch.auxp==NULL) { /* RWD fix */
      return csound->PerfError(csound, Str("expseg (arate): not initialised"));
    }
    while (--segp->cnt < 0)
      p->cursegp = ++segp;
    val = segp->val;
    nxtval = val * segp->mlt;
    li = (nxtval - val) * csound->onedksmps;
    rs = p->rslt;
    for (n=0; n<csound->ksmps; n++) {
      rs[n] = val;
      val += li;
    }
    segp->val = nxtval;
    return OK;
}

int xsgrset(ENVIRON *csound, EXPSEG *p)
{
    int     relestim;
    SEG     *segp;
    int     nsegs, n = 0;
    MYFLT   **argp, prvpt;

    p->xtra = -1;
    nsegs = p->INOCOUNT >> 1;               /* count segs & alloc if nec */
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        (unsigned int)nsegs*sizeof(SEG) > (unsigned int)p->auxch.size) {
      csound->AuxAlloc(csound, (long)nsegs*sizeof(SEG), &p->auxch);
      p->cursegp = segp = (SEG *) p->auxch.auxp;
    }
    argp = p->argums;
    prvpt = **argp++;
    if (**argp < FL(0.0))  return OK; /* if idur1 < 0, skip init      */
    p->curval = prvpt;
    p->curcnt = 0;                  /* else setup null seg0         */
    p->cursegp = segp - 1;
    p->segsrem = nsegs + 1;
    do {                            /* init & chk each real seg ..  */
      MYFLT dur = **argp++;
      segp->nxtpt = **argp++;
      if ((segp->cnt = (long)(dur * csound->ekr + FL(0.5))) <= 0)
        segp->cnt = 0;
      else if (segp->nxtpt * prvpt <= FL(0.0))
        goto experr;
      prvpt = segp->nxtpt;
      segp++;
    } while (--nsegs);
    relestim = (int) (p->cursegp + p->segsrem - 1)->cnt;
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = relestim;
    return OK;

 experr:
    n = segp - p->cursegp + 2;
    if (prvpt == FL(0.0))
      sprintf(csound->errmsg, Str("ival%d is zero"), n);
    else if (segp->nxtpt == FL(0.0))
      sprintf(csound->errmsg, Str("ival%d is zero"), n+1);
    else sprintf(csound->errmsg, Str("ival%d sign conflict"), n+1);
    return csound->InitError(csound, csound->errmsg);
}

/* **** MXDSR is just a construction and use of expseg */

int mxdsrset(ENVIRON *csound, EXPSEG *p)
{
    int         relestim;
    SEG         *segp;
    int         nsegs;
    MYFLT       **argp = p->argums;
    MYFLT       delay = *argp[4], attack = *argp[0], decay = *argp[1];
    MYFLT       rel = *argp[3];

    nsegs = 4;          /* DXDSR */
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(SEG) < (unsigned int)p->auxch.size) {
      csound->AuxAlloc(csound, (long)nsegs*sizeof(SEG), &p->auxch);
      segp = (SEG *) p->auxch.auxp;
    }
    if (**argp <= FL(0.0))  return OK;  /* if idur1 <= 0, skip init  */
    p->cursegp = segp-1;                /* else setup null seg0 */
    p->segsrem = nsegs+1;
    p->curval = FL(0.001);
    p->curcnt = 0;                      /* else setup null seg0 */
    delay += FL(0.001);
    attack -= FL(0.001);
    segp[0].nxtpt = FL(0.001);
    segp[0].cnt = (long) (delay*csound->ekr + FL(0.5));
    segp[1].nxtpt = FL(1.0);
    segp[1].cnt = (long) (attack*csound->ekr + FL(0.5));
    segp[2].nxtpt = *argp[2];
    segp[2].cnt = (long) (decay*csound->ekr + FL(0.5));
    segp[3].nxtpt = FL(0.001);
    segp[3].cnt = (long) (rel*csound->ekr + FL(0.5));
    relestim = (int) (p->cursegp + p->segsrem - 1)->cnt;
    p->xtra = (long)(*argp[5] * csound->ekr + FL(0.5));     /* Release time?? */
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = relestim;
    return OK;
}

/* end of MXDSR */

int kxpsegr(ENVIRON *csound, EXPSEG *p)
{
    *p->rslt = p->curval;               /* put the cur value    */
    if (p->segsrem) {                   /* done if no more segs */
      SEG *segp;
      if (p->h.insdshead->relesing && p->segsrem > 1) {
        while (p->segsrem > 1) {        /* reles flag new:      */
          segp = ++p->cursegp;          /*   go to last segment */
          p->segsrem--;
        }                               /*   get univ relestim  */
        segp->cnt = p->xtra>=0 ? p->xtra : p->h.insdshead->xtratim;
        goto newm;                      /*   and set new curmlt */
      }
      if (--p->curcnt <= 0) {           /* if done cur seg      */
      chk2:
        if (p->segsrem == 2) return OK; /*   seg Y rpts lastval */
        if (!(--p->segsrem)) return OK; /*   seg Z now done all */
        segp = ++p->cursegp;            /*   else find nextseg  */
      newm:
        if (!(p->curcnt = segp->cnt)) { /*   nonlen = discontin */
          p->curval = segp->nxtpt;      /*     reload & rechk   */
          goto chk2;
        }
        if (segp->nxtpt == p->curval)   /*   else get new mlt   */
          p->curmlt = FL(1.0);
        else p->curmlt = (MYFLT) pow(segp->nxtpt/p->curval, 1.0/segp->cnt);
      }
      p->curval *= p->curmlt;           /* advance the cur val  */
    }
    return OK;
}

int expsegr(ENVIRON *csound, EXPSEG *p)
{
    MYFLT  val, amlt, *rs = p->rslt;
    int    n, nsmps=csound->ksmps;

    val = p->curval;                    /* sav the cur value    */
    if (p->segsrem) {                   /* if no more segs putk */
      SEG *segp;
      if (p->h.insdshead->relesing && p->segsrem > 1) {
        while (p->segsrem > 1) {        /* if reles flag new    */
          segp = ++p->cursegp;          /*   go to last segment */
          p->segsrem--;
        }                               /*   get univ relestim  */
        segp->cnt = p->xtra>=0 ? p->xtra : p->h.insdshead->xtratim;
        goto newm;                      /*   and set new curmlt */
      }
      if (--p->curcnt <= 0) {            /* if done cur seg      */
      chk2:
        if (p->segsrem == 2) goto putk; /*   seg Y rpts lastval */
        if (!(--p->segsrem)) goto putk; /*   seg Z now done all */
        segp = ++p->cursegp;            /*   else find nextseg  */
      newm:
        if (!(p->curcnt = segp->cnt)) { /*   nonlen = discontin */
          val = p->curval = segp->nxtpt; /*   reload & rechk  */
          goto chk2;
        }                               /*   else get new mlts  */
        if (segp->nxtpt == val) {
          p->curmlt = p->curamlt = FL(1.0);
          p->curval = val;
          goto putk;
        }
        else {
          p->curmlt = (MYFLT) pow((double)(segp->nxtpt/val),
                                  1.0/(double)segp->cnt);
          p->curamlt = (MYFLT) pow(p->curmlt, (double) csound->onedksmps);
        }
      }
      p->curval = val * p->curmlt;        /* advance the cur val  */
      if ((amlt = p->curamlt) == FL(1.0))
        goto putk;
      for (n=0; n<nsmps; n++) {
        rs[n] = val;
        val *= amlt;
      }
    } else {
    putk:
      for (n=0; n<nsmps; n++) rs[n] = val;
    }
    return OK;
}

int lnnset(ENVIRON *csound, LINEN *p)
{
    MYFLT a,b,dur;

    if ((dur = *p->idur) > FL(0.0)) {
      p->cnt1 = (long)(*p->iris * csound->ekr + FL(0.5));
      if (p->cnt1 > 0L) {
        p->inc1 = FL(1.0) / (MYFLT) p->cnt1;
        p->val = FL(0.0);
      }
      else p->inc1 = p->val = FL(1.0);
      a = dur * csound->ekr + FL(0.5);
      b = *p->idec * csound->ekr + FL(0.5);
      if ((long) b > 0L) {
        p->cnt2 = (long) (a - b);
        p->inc2 = FL(1.0) /  b;
      }
      else {
        p->inc2 = FL(1.0);
        p->cnt2 = (long) a;
      }
      p->lin1 = FL(0.0);
      p->lin2 = FL(1.0);
    }
    return OK;
}

int klinen(ENVIRON *csound, LINEN *p)
{
    MYFLT fact = FL(1.0);

    if (p->cnt1 > 0L) {
      fact = p->lin1;
      p->lin1 += p->inc1;
      p->cnt1--;
    }
    if (p->cnt2)
      p->cnt2--;
    else {
      fact *= p->lin2;
      p->lin2 -= p->inc2;
    }
    *p->rslt = *p->sig * fact;
    return OK;
}

int linen(ENVIRON *csound, LINEN *p)
{
    int flag=0, n, nsmps=csound->ksmps;
    MYFLT *rs,*sg,li,val,nxtval=FL(1.0);

    val = p->val;
    rs = p->rslt;
    sg = p->sig;
    if (p->cnt1 > 0L) {
      flag = 1;
      p->lin1 += p->inc1;
      p->cnt1--;
      nxtval = p->lin1;
    }
    if (p->cnt2 <= 0L) {
      flag = 1;
      p->lin2 -= p->inc2;
      nxtval *= p->lin2;
    }
    else p->cnt2--;
    p->val = nxtval;
    if (flag) {
      li = (nxtval - val) * csound->onedksmps;
      if (p->XINCODE) {
        for (n=0; n<nsmps; n++) {
          rs[n] = *sg++ * val;
          val += li;
        }
      }
      else {
        for (n=0; n<nsmps; n++) {
          rs[n] = *sg * val;
          val += li;
        }
      }
    }
    else {
      if (p->XINCODE) {
        for (n=0; n<nsmps; n++) rs[n] = sg[n];
      }
      else {
        MYFLT ss = *sg;
        for (n=0; n<nsmps; n++) rs[n] = ss;
      }
    }
    return OK;
}

int lnrset(ENVIRON *csound, LINENR *p)
{
    p->cnt1 = (long)(*p->iris * csound->ekr + FL(0.5));
    if (p->cnt1 > 0L) {
      p->inc1 = FL(1.0) / (MYFLT) p->cnt1;
      p->val = FL(0.0);
    }
    else p->inc1 = p->val = FL(1.0);
    if (*p->idec > FL(0.0)) {
      int relestim = (int) (*p->idec * csound->ekr + FL(0.5));
      if (relestim > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = relestim;
      if (*p->iatdec <= FL(0.0)) {
        return csound->InitError(csound, Str("non-positive iatdec"));
      }
      else p->mlt2 = (MYFLT) pow((double)*p->iatdec,
                                 ((double)csound->onedkr / *p->idec));
    }
    else p->mlt2 = FL(1.0);
    p->lin1 = FL(0.0);
    p->val2 = FL(1.0);
    return OK;
}

int klinenr(ENVIRON *csound, LINENR *p)
{
    MYFLT fact = FL(1.0);

    if (p->cnt1 > 0L) {
      fact = p->lin1;
      p->lin1 += p->inc1;
      p->cnt1--;
    }
    if (p->h.insdshead->relesing) {
      fact *= p->val2;
      p->val2 *= p->mlt2;
    }
    *p->rslt = *p->sig * fact;
    return OK;
}

int linenr(ENVIRON *csound, LINENR *p)
{
    int flag=0, n, nsmps=csound->ksmps;
    MYFLT *rs,*sg,li,val,nxtval=FL(1.0);

    val = p->val;
    rs = p->rslt;
    sg = p->sig;
    if (p->cnt1 > 0L) {
      flag = 1;
      p->lin1 += p->inc1;
      p->cnt1--;
      nxtval = p->lin1;
    }
    if (p->h.insdshead->relesing) {
      flag = 1;
      p->val2 *= p->mlt2;
      nxtval *= p->val2;
    }
    p->val = nxtval;
    if (flag) {
      li = (nxtval - val) * csound->onedksmps;
      if (p->XINCODE) {
        for (n=0; n<nsmps; n++) {
          rs[n] = sg[n] * val;
          val += li;
        }
      }
      else {
        MYFLT ss = *sg;
        for (n=0; n<nsmps; n++) {
          rs[n] =  ss * val;
          val += li;
        }
      }
    }
    else {
      if (p->XINCODE) {
        for (n=0; n<nsmps; n++) {
          rs[n] = sg[n];
        }
      }
      else {
        MYFLT ss = *sg;
        for (n=0; n<nsmps; n++) {
          rs[n] = ss;
        }
      }
    }
    return OK;
}

int evxset(ENVIRON *csound, ENVLPX *p)
{
    FUNC        *ftp;
    MYFLT       ixmod, iatss, idur, prod, diff, asym, nk, denom, irise;
    long        cnt1;

    if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
      return NOTOK;
    p->ftp = ftp;
    if ((idur = *p->idur) > FL(0.0)) {
      if ((iatss = (MYFLT)fabs(*p->iatss)) == FL(0.0)) {
        return csound->InitError(csound, "iatss = 0");
      }
      if (iatss != FL(1.0) && (ixmod = *p->ixmod) != FL(0.0)) {
        if (fabs(ixmod) > 0.95) {
          return csound->InitError(csound, Str("ixmod out of range."));
        }
        ixmod = -(MYFLT)sin(sin(ixmod));
        prod = ixmod * iatss;
        diff = ixmod - iatss;
        denom = diff + prod + FL(1.0);
        if (denom == FL(0.0))
          asym = FHUND;
        else {
          asym = 2 * prod / denom;
          if (fabs(asym) > FHUND)
            asym = FHUND;
        }
        iatss = (iatss - asym) / (FL(1.0) - asym);
        asym = asym* *(ftp->ftable + ftp->flen); /* +1 */
      }
      else asym = FL(0.0);
      if ((irise = *p->irise) > FL(0.0)) {
        p->phs = 0;
        p->ki = (long) (csound->kicvt / irise);
        p->val = *ftp->ftable;
      }
      else {
        p->phs = -1;
        p->val = *(ftp->ftable + ftp->flen)-asym;
        irise = FL(0.0);  /* in case irise < 0 */
      }
      if (!(*(ftp->ftable + ftp->flen))) {
        return csound->InitError(csound, Str("rise func ends with zero"));
      }
      cnt1 = (long) ((idur - irise - *p->idec) * csound->ekr + FL(0.5));
      if (cnt1 < 0L) {
        cnt1 = 0L;
        nk = csound->ekr;
      }
      else {
        if (*p->iatss < FL(0.0) || cnt1 <= 4L)
          nk = csound->ekr;
        else nk = (MYFLT) cnt1;
      }
      p->mlt1 = (MYFLT) pow((double)iatss, (1.0/nk));
      if (*p->idec > FL(0.0)) {
        if (*p->iatdec <= FL(0.0)) {
          return csound->InitError(csound, Str("non-positive iatdec"));
        }
        p->mlt2 = (MYFLT) pow((double)*p->iatdec,
                              ((double)csound->onedkr / *p->idec));
      }
      p->cnt1 = cnt1;
      p->asym = asym;
    }
    return OK;
}

int knvlpx(ENVIRON *csound, ENVLPX *p)
{
    FUNC        *ftp;
    long        phs;
    MYFLT       fact, v1, fract, *ftab;

    ftp = p->ftp;
    if (ftp==NULL) {        /* RWD fix */
      return csound->PerfError(csound, Str("envlpx(krate): not initialised"));
    }

    if ((phs = p->phs) >= 0) {
      fract = (MYFLT) PFRAC(phs);
      ftab = ftp->ftable + (phs >> ftp->lobits);
      v1 = *ftab++;
      fact = (v1 + (*ftab - v1) * fract);
      phs += p->ki;
      if (phs >= MAXLEN) {  /* check that 2**N+1th pnt is good */
        p->val = *(ftp->ftable + ftp->flen );
        if (!p->val) {
          return csound->PerfError(csound,
                                   Str("envlpx rise func ends with zero"));
        }
        p->val -= p->asym;
        phs = -1L;
      }
      p->phs = phs;
    }
    else {
      fact = p->val;
      if (p->cnt1 > 0L) {
        p->val *= p->mlt1;
        fact += p->asym;
        p->cnt1--;
        if (p->cnt1 == 0L)
          p->val += p->asym;
      }
      else p->val *= p->mlt2;
    }
    *p->rslt = *p->xamp * fact;
    return OK;
}

int envlpx(ENVIRON *csound, ENVLPX *p)
{
    FUNC        *ftp;
    long        phs;
    int         n, nsmps=csound->ksmps;
    MYFLT       *xamp, *rslt, val, nxtval, li, v1, fract, *ftab;

    xamp = p->xamp;
    rslt = p->rslt;
    val = p->val;
    if ((phs = p->phs) >= 0L) {
      ftp = p->ftp;
      if (ftp==NULL) { /* RWD fix */
        return csound->PerfError(csound, Str("envlpx(krate): not initialised"));
      }
      fract = (MYFLT) PFRAC(phs);
      ftab = ftp->ftable + (phs >> ftp->lobits);
      v1 = *ftab++;
      nxtval = (v1 + (*ftab - v1) * fract);
      phs += p->ki;
      if (phs >= MAXLEN) {  /* check that 2**N+1th pnt is good */
        nxtval = *(ftp->ftable + ftp->flen );
        if (!nxtval) {
          return csound->PerfError(csound,
                                   Str("envlpx rise func ends with zero"));
        }
        nxtval -= p->asym;
        phs = -1;
      }
      p->phs = phs;
    }
    else {
      nxtval = val;
      if (p->cnt1 > 0L) {
        nxtval *= p->mlt1;
        nxtval += p->asym;
        p->cnt1--;
      }
      else nxtval *= p->mlt2;
    }
    p->val = nxtval;
    li = (nxtval - val) * csound->onedksmps;  /* linear interpolation factor */
    if (p->XINCODE) {                         /* for audio rate amplitude: */
      for (n=0; n<nsmps;n++) {
        rslt[n] = xamp[n] * val;
        val += li;
      }
    }
    else {
      MYFLT sxamp = *xamp;
      for (n=0; n<nsmps;n++) {
        rslt[n] = sxamp * val;
        val += li;
      }
    }
    return OK;
}

int evrset(ENVIRON *csound, ENVLPR *p)
{
    FUNC        *ftp;
    MYFLT  ixmod, iatss, prod, diff, asym, denom, irise;

    if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
      return NOTOK;
    p->ftp = ftp;
    if ((iatss = (MYFLT)fabs((double)*p->iatss)) == FL(0.0)) {
      return csound->InitError(csound, "iatss = 0");
    }
    if (iatss != FL(1.0) && (ixmod = *p->ixmod) != FL(0.0)) {
      if (fabs(ixmod) > 0.95) {
        return csound->InitError(csound, Str("ixmod out of range."));
      }
      ixmod = -(MYFLT)sin(sin((double)ixmod));
      prod = ixmod * iatss;
      diff = ixmod - iatss;
      denom = diff + prod + FL(1.0);
      if (denom == FL(0.0))
        asym = FHUND;
      else {
        asym = 2 * prod / denom;
        if (fabs(asym) > FHUND)
          asym = FHUND;
      }
      iatss = (iatss - asym) / (FL(1.0) - asym);
      asym = asym * *(ftp->ftable + ftp->flen); /* +1 */
    }
    else asym = FL(0.0);
    if ((irise = *p->irise) > FL(0.0)) {
      p->phs = 0L;
      p->ki = (long) (csound->kicvt / irise);
      p->val = *ftp->ftable;
    }
    else {
      p->phs = -1L;
      p->val = *(ftp->ftable + ftp->flen)-asym;
      irise = FL(0.0);          /* in case irise < 0 */
    }
    if (!(*(ftp->ftable + ftp->flen))) {
      return csound->InitError(csound, Str("rise func ends with zero"));
    }
    p->mlt1 = (MYFLT)pow((double)iatss, (double)csound->onedkr);
    if (*p->idec > FL(0.0)) {
      long rlscnt = (long)(*p->idec * csound->ekr + FL(0.5));
      if ((p->rindep = (long)*p->irind))
        p->rlscnt = rlscnt;
      else if (rlscnt > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = (int)rlscnt;
      if ((p->atdec = *p->iatdec) <= FL(0.0) ) {
        return csound->InitError(csound, Str("non-positive iatdec"));
      }
    }
    p->asym = asym;
    p->rlsing = 0;
    return OK;
}

int knvlpxr(ENVIRON *csound, ENVLPR *p)
{
    MYFLT  fact;
    long   rlscnt;

    if (!p->rlsing) {                   /* if not in reles seg  */
      if (p->h.insdshead->relesing) {
        p->rlsing = 1;                  /*   if new flag, set mlt2 */
        rlscnt = (p->rindep) ? p->rlscnt : p->h.insdshead->xtratim;
        if (rlscnt)
          p->mlt2 = (MYFLT)pow((double)p->atdec, 1.0/(double)rlscnt);
        else p->mlt2 = FL(1.0);
      }
      if (p->phs >= 0) {                /* do fn rise for seg 1 */
        FUNC *ftp = p->ftp;
        long phs = p->phs;
        MYFLT fract = PFRAC(phs);
        MYFLT *ftab = ftp->ftable + (phs >> ftp->lobits);
        MYFLT v1 = *ftab++;
        fact = (v1 + (*ftab - v1) * fract);
        phs += p->ki;
        if (phs < MAXLEN || p->rlsing)  /* if more fn or beg rls */
          p->val = fact;                /*      save cur val     */
        else {                          /* else prep for seg 2  */
          p->val = *(ftp->ftable + ftp->flen) - p->asym;
          phs = -1L;
        }
        p->phs = phs;
      }
      else {
        fact = p->val + p->asym;        /* do seg 2 with asym */
        p->val *= p->mlt1;
        if (p->rlsing)                  /* if ending, rm asym */
          p->val += p->asym;
      }
    }
    else fact = p->val *= p->mlt2;      /* else do seg 3 decay */
    *p->rslt = *p->xamp * fact;
    return OK;
}

int envlpxr(ENVIRON *csound, ENVLPR *p)
{
#if defined(SYMANTEC) && !defined(THINK_C)
#pragma options(!global_optimizer)
#endif
    int    n, nsmps=csound->ksmps;
    long   rlscnt;
    MYFLT  *xamp, *rslt, val, nxtval, li;

    xamp = p->xamp;
    rslt = p->rslt;
    val = p->val;
    if (!p->rlsing) {                   /* if not in reles seg  */
      if (p->h.insdshead->relesing) {
        p->rlsing = 1;                  /*   if new flag, set mlt2 */
        rlscnt = (p->rindep) ? p->rlscnt : p->h.insdshead->xtratim;
        if (rlscnt)
          p->mlt2 = (MYFLT)pow((double)p->atdec, 1.0/(double)rlscnt);
        else p->mlt2 = FL(1.0);
      }
      if (p->phs >= 0) {                /* do fn rise for seg 1 */
        FUNC *ftp = p->ftp;
        long phs = p->phs;
        MYFLT fract = PFRAC(phs);
        MYFLT *ftab = ftp->ftable + (phs >> ftp->lobits);
        MYFLT v1 = *ftab++;
        ftp = p->ftp;
        fract = PFRAC(phs);
        ftab = ftp->ftable + (phs >> ftp->lobits);
        v1 = *ftab++;
        nxtval = (v1 + (*ftab - v1) * fract);
        phs += p->ki;
        if (phs < MAXLEN || p->rlsing)  /* if more fn or beg rls */
          p->val = nxtval;              /*      save 2nd brkpnt */
        else {                          /* else prep for seg 2  */
          p->val = *(ftp->ftable + ftp->flen) - p->asym;
          phs = -1;
        }
        p->phs = phs;
      }
      else {
        nxtval = p->val *= p->mlt1;     /* do seg 2 with asym   */
        val += p->asym;
        nxtval += p->asym;
        if (p->rlsing)                  /* if ending, rm asym   */
          p->val += p->asym;
      }
    }
    else p->val = nxtval = val * p->mlt2;     /* else do seg 3 decay  */
    li = (nxtval - val) * csound->onedksmps;  /* all segs use interp  */
    if (p->XINCODE) {
      for (n=0; n<nsmps; n++) {
        rslt[n] = xamp[n] * val;
        val += li;
      }
    }
    else {
      MYFLT sxamp = *xamp;
      for (n=0; n<nsmps; n++) {
        rslt[n] = sxamp * val;
        val += li;
      }
    }
    return OK;
}

