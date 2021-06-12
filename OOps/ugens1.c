 /*
    gens1.c:

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

#include "csoundCore.h"         /*                      UGENS1.C        */
#include "ugens1.h"
#include <math.h>

#define FHUND (FL(100.0))


int32_t linset(CSOUND *csound, LINE *p)
{
    double       dur;
    if (LIKELY((dur = *p->idur) > FL(0.0))) {
      p->incr = (*p->ib - *p->ia) / dur * csound->onedsr;
      p->kincr = p->incr*CS_KSMPS;
      p->val = *p->ia;
    }
    return OK;
}

int32_t kline(CSOUND *csound, LINE *p)
{
    IGN(csound);
    *p->xr = p->val;            /* rslt = val   */
    p->val += p->kincr;          /* val += incr  */
    return OK;
}

int32_t aline(CSOUND *csound, LINE *p)
{
    IGN(csound);
    double val, inc;
    MYFLT *ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    ar = p->xr;
    val = p->val;
    inc = p->incr;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }

    //p->val += inc;/* nxtval = val + inc */
    //inc /= (nsmps - offset);
    for (n=offset; n<nsmps; n++) {
      ar[n] = (MYFLT)val;
      val += inc;       /* interp val for ksmps */
    }
    p->val = val;
    return OK;
}

int32_t expset(CSOUND *csound, EXPON *p)
{
    double       dur, a, b;
    //printf("kr = %f , 1/kr = %f \n",CS_EKR, CS_ONEDKR);
    if (LIKELY((dur = *p->idur) > FL(0.0) )) {
      a = *p->ia;
      b = *p->ib;
      if (LIKELY((a * b) > FL(0.0))) {
        p->mlt = POWER(b/a, csound->onedsr/dur);
        p->kmlt = POWER(b/a, CS_ONEDKR/dur);
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

int32_t kexpon(CSOUND *csound, EXPON *p)
{
   IGN(csound);
    *p->xr = p->val;            /* rslt = val   */
    p->val *= p->kmlt;           /* val *= mlt  */
    return OK;
}

int32_t expon(CSOUND *csound, EXPON *p)
{
    IGN(csound);
    double val, mlt;//, inc;//, nxtval;
    MYFLT *ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    val = p->val;
    mlt = p->mlt;
    // nxtval = val * mlt;
    ar = p->xr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    //inc = nxtval - val;
    //inc /= (nsmps - offset);   /* increment per sample */
    for (n=offset; n<nsmps; n++) {
      ar[n] = (MYFLT)val;
      val *= mlt;               /* interp val for ksmps */
    }
    // p->val = nxtval;            /* store next value */
    p->val = val;
    return OK;
}

int32_t lsgset(CSOUND *csound, LINSEG *p)
{
    SEG *segp;
    int32_t nsegs;
    MYFLT       **argp;
    double val;

    if (UNLIKELY(!(p->INCOUNT & 1))) {
      return csound->InitError(csound, Str("incomplete number of input arguments"));
    }

    /* count segs & alloc if nec */
    nsegs = (p->INOCOUNT - (!(p->INOCOUNT & 1))) >> 1;
    /* VL: 29.05.17 allocating one extra empty segment
       so that the breakpoint version of this opcode
       can work properly without a fencepost bug */
    if (UNLIKELY((p->cursegp = (SEG *) p->auxch.auxp) == NULL ||
                 (nsegs+1)*sizeof(SEG) < (uint32_t)p->auxch.size)) {
      csound->AuxAlloc(csound, (int32_t)(nsegs+1)*sizeof(SEG), &p->auxch);
      p->cursegp = (SEG *) p->auxch.auxp;
      segp = p->cursegp + 1; /* point to first seg */
      p->cursegp->cnt = 0;   /* zero duration of segment 0  */
      segp[nsegs-1].cnt = MAXPOS; /* set endcount for safety */
    } else segp = p->cursegp + 1; /* point to first seg */
    argp = p->argums;
    val = (double)**argp++;
    if (UNLIKELY(**argp <= FL(0.0)))  return OK;    /* if idur1 <= 0, skip init  */
    p->curval = val;
    p->curcnt = 0;
    /* VL: 29.05.17 this was causing a fencepost error in
       the breakpoint version (linsegb) */
    //p->cursegp = segp - 1;          /* else setup null seg0 */
    p->segsrem = nsegs + 1;
    do {                                /* init each seg ..  */
      double dur = (double)**argp++;
      segp->nxtpt = (double)**argp++;
      if (UNLIKELY((segp->cnt = (int32_t)(dur * CS_EKR + FL(0.5))) < 0))
        segp->cnt = 0;
      if (UNLIKELY((segp->acnt = (int32_t)(dur * csound->esr + FL(0.5))) < 0))
        segp->acnt = 0;
      segp++;
    } while (--nsegs);
    p->xtra = -1;

    return OK;

}

int32_t lsgset_bkpt(CSOUND *csound, LINSEG *p)
{
    int32_t cnt = 0, bkpt = 0;
    int32_t nsegs;
    int32_t n;
    SEG *segp;
    n = lsgset(csound, p);
    if (UNLIKELY(n!=0)) return n;
    nsegs = p->segsrem;
    segp = p->cursegp;
    do {
      if (UNLIKELY(cnt > segp->cnt))
        return csound->InitError(csound, Str("Breakpoint %d not valid"), bkpt);
      segp->cnt -= cnt;
      cnt += segp->cnt;
      segp++;
      bkpt++;
    } while (--nsegs);
    return OK;
}


int32_t klnseg(CSOUND *csound, LINSEG *p)
{
    IGN(csound);
    *p->rslt = p->curval;               /* put the cur value    */
    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;          /* RWD fix */
    if (UNLIKELY(p->segsrem)) {                   /* done if no more segs */
      if (--p->curcnt <= 0) {           /* if done cur segment  */
        SEG *segp = p->cursegp;
        if (UNLIKELY(!(--p->segsrem)))  {
          p->curval = segp->nxtpt;      /* advance the cur val  */
          return OK;
        }
        p->cursegp = ++segp;            /*   find the next      */
        if (UNLIKELY(!(p->curcnt = segp->cnt))) { /*   nonlen = discontin */
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
    return OK;
 err1:
    return csound->InitError(csound, Str("linseg not initialised (krate)\n"));
}

int32_t linseg(CSOUND *csound, LINSEG *p)
{
    double val, ainc;
    MYFLT *rs = p->rslt;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;  /* RWD fix */

    val = p->curval;                      /* sav the cur value    */
    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=offset; n<nsmps; n++) {
      if (LIKELY(p->segsrem)) {             /* if no more segs putk */
        if (--p->curcnt <= 0) {             /*  if done cur segment */
          SEG *segp = p->cursegp;
        chk1:
          if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
            val = p->curval = segp->nxtpt;
            goto putk;                      /*      put endval      */
          }
          p->cursegp = ++segp;              /*   else find the next */
          //printf("newseg: nxtpt=%f acnt=%d\n", segp->nxtpt, segp->acnt);
          if (UNLIKELY(!(p->curcnt = segp->acnt))) {
            val = p->curval = segp->nxtpt;  /* nonlen = discontin */
            goto chk1;
          }                                 /*   poslen = new slope */
          p->curainc = (segp->nxtpt - val) / segp->acnt;
          // p->curainc = p->curinc * CS_ONEDKSMPS;
        }
        // p->curval = val + p->curinc;        /* advance the cur val  */
        if (UNLIKELY((ainc = p->curainc) == FL(0.0)))
          goto putk;
        rs[n] = (MYFLT)val;
        val += ainc;
      }
      else {                      /* no more segments */
      putk:
        rs[n] = (MYFLT)val;
      }
    }
    p->curval = val;
    return OK;
 err1:

    return csound->PerfError(csound, &(p->h),
                             Str("linseg: not initialised (arate)\n"));
}

/* **** ADSR is just a construction and use of linseg */

#define MAXSEGDUR (INT_MAX/CS_ESR)

static int32_t adsrset1(CSOUND *csound, LINSEG *p, int32_t midip)
{
    SEG         *segp;
    int32_t     nsegs;
    MYFLT       **argp = p->argums;
    double      dur;
    MYFLT       len = csound->curip->p3.value;
    MYFLT       release = *argp[3];
    int32_t     relestim;

    //printf("len = %f\n", len);
    if (UNLIKELY(len<=FL(0.0)))
      len = (int32_t) MAXSEGDUR;// FL(10000.0); /* MIDI case set int32_t */
    nsegs = 6;          /* DADSR */
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(SEG) < (uint32_t)p->auxch.size) {
      csoundAuxAlloc(csound, (size_t) nsegs * sizeof(SEG), &p->auxch);
      p->cursegp = segp = (SEG *) p->auxch.auxp;
      segp[nsegs-1].cnt = MAXPOS; /* set endcount for safety */
    }
    else if (**argp > FL(0.0))
      memset(p->auxch.auxp, 0, (size_t)nsegs*sizeof(SEG));
    if (**argp <= FL(0.0))  return OK;       /* if idur1 <= 0, skip init  */
    p->curval = 0.0;
    p->curcnt = 0;
    p->cursegp = segp - 1;      /* else setup null seg0 */
    p->segsrem = nsegs;
    //printf("args: %f %f %f %f %f\n",
    //       *argp[0], *argp[1], *argp[2], *argp[3],* argp[4]);
                                /* Delay */
    dur = (double)*argp[4];
    segp->nxtpt = FL(0.0);
    segp->cnt = (int32_t)(dur * CS_EKR + FL(0.5));
    //printf("delay: dur=%f cnt=%d\n", dur, segp->cnt);
    segp++;
                                /* Attack */
    dur = (double)*argp[0];
    segp->nxtpt = FL(1.0);
    segp->cnt = (int32_t)(dur * CS_EKR + FL(0.5));
    if (UNLIKELY((segp->acnt = (int32_t)(dur * csound->esr + FL(0.5))) < 0))
        segp->acnt = 0;
    //printf("attack: dur=%f cnt=%d acnt=%d nxt=%f\n",
    //       dur, segp->cnt, segp->acnt, segp->nxtpt);
    segp++;
                                /* Decay */
    dur = *argp[1];
    segp->nxtpt = *argp[2];
    segp->cnt = (int32_t)(dur * CS_EKR + FL(0.5));
    if (UNLIKELY((segp->acnt = (int32_t)(dur * csound->esr + FL(0.5))) < 0))
      segp->acnt = 0;
    //printf("decay: dur=%f cnt=%d acnt=%d nxt=%f\n",
    //       dur, segp->cnt, segp->acnt, segp->nxtpt);
    segp++;
                                /* Sustain */
    /* Should use p3 from score, but how.... */
    dur = len - *argp[4] - *argp[0] - *argp[1] - *argp[3];
    if (!midip && dur <0.0)
      csound->Warning(csound, Str("length of ADSR note too short"));
    segp->nxtpt = *argp[2];
    segp->cnt = (int32_t)(dur * CS_EKR + FL(0.5));
    if (UNLIKELY((segp->acnt = (int32_t)(dur * csound->esr + FL(0.5))) < 0))
      segp->acnt = 0;
    //printf("sustain: dur=%f cnt=%d acnt=%d nxt=%f\n",
    //       dur, segp->cnt, segp->acnt, segp->nxtpt);
    segp++;
                                /* Release */
    // **FIXME is this needed? dur = *argp[3];
    segp->nxtpt = FL(0.0);
    segp->cnt = (int32_t)(release * CS_EKR + FL(0.5));
    if (UNLIKELY((segp->acnt = (int32_t)(release * csound->esr + FL(0.5))) < 0))
      segp->acnt = 0;
    //printf("release: dur=%f cnt=%d acnt=%d nxt=%f\n",
    //       dur, segp->cnt, segp->acnt, segp->nxtpt);
    if (midip) {
      relestim = (p->cursegp + p->segsrem - 1)->cnt;
      p->xtra = relestim;
          /*  VL 4-1-2011 was (int32_t)(*argp[5] * CS_EKR + FL(0.5));
              this seems to fix it */
      if (relestim > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = (int32_t)relestim;
    }
    else
      p->xtra = 0L;
    return OK;
}

int32_t adsrset(CSOUND *csound, LINSEG *p)
{
    return adsrset1(csound, p, 0);
}

int32_t madsrset(CSOUND *csound, LINSEG *p)
{
    return adsrset1(csound, p, 1);
}

/* End of ADSR */

int32_t lsgrset(CSOUND *csound, LINSEG *p)
{
    int32_t relestim;
    if (lsgset(csound,p) == OK){
    relestim = (p->cursegp + p->segsrem - 1)->cnt;
    p->xtra = relestim;
    /* VL 4-1-2011 was -1, making all linsegr
                            releases in an instr => xtratim
                            set to relestim seems to fix this */
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = (int32_t)relestim;
    return OK;
    }
    else return NOTOK;
}

int32_t klnsegr(CSOUND *csound, LINSEG *p)
{
    IGN(csound);
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

int32_t linsegr(CSOUND *csound, LINSEG *p)
{
    IGN(csound);
    MYFLT  val, ainc, *rs = p->rslt;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }
    val = p->curval;                          /* sav the cur value    */
    for (n=offset; n<nsmps; n++) {
      if (LIKELY(p->segsrem)) {               /* if no more segs putk */
        SEG *segp;
        if (p->h.insdshead->relesing && p->segsrem > 1) {
          while (p->segsrem > 1) {            /* release flag new:    */
            segp = ++p->cursegp;              /*   go to last segment */
            p->segsrem--;
          }                                   /*   get univ relestim  */
          segp->acnt = (p->xtra >=0 ? p->xtra : p->h.insdshead->xtratim)*CS_KSMPS;
          goto newi;                          /*   and set new curinc */
        }
        if (--p->curcnt <= 0) {               /* if done cur seg      */
        chk2:
          if (p->segsrem == 2) goto putk;     /*   seg Y rpts lastval */
          if (!(--p->segsrem)) goto putk;     /*   seg Z now done all */
          segp = ++p->cursegp;                /*   else find nextseg  */
        newi:
          if (!(p->curcnt = segp->acnt)) {    /*   nonlen = discontin */
            val = p->curval = segp->nxtpt;    /*   reload & rechk  */
            goto chk2;
          }                                   /*   else get new slope */
          p->curainc = (segp->nxtpt - val) / segp->acnt;
          // p->curainc = p->curinc * CS_ONEDKSMPS;
        }
        //p->curval = val + p->curainc*CS_KSMPS;    /* advance the cur val  */
        if ((ainc = p->curainc) == FL(0.0))
          goto putk;

        rs[n] = val;
        val += ainc;
      }
      else {
      putk:
        rs[n] = val;
      }
    }
    p->curval = val;
    return OK;
}

int32_t xsgset(CSOUND *csound, EXXPSEG *p)
{
    XSEG        *segp;
    int32_t         nsegs;
    MYFLT       d, **argp, val, dur, nxtval;
    int32_t         n=0;

    if (!(p->INCOUNT & 1)) {
      return csound->InitError(csound, Str("incomplete number of input arguments"));
    }

    /* count segs & alloc if nec */
    nsegs = (p->INOCOUNT - (!(p->INOCOUNT & 1))) >> 1;
    if ((segp = (XSEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(XSEG) < (uint32_t)p->auxch.size) {
      csound->AuxAlloc(csound, (int32_t)nsegs*sizeof(XSEG), &p->auxch);
      p->cursegp = segp = (XSEG *) p->auxch.auxp;
      (segp+nsegs-1)->cnt = MAXPOS;   /* set endcount for safety */
    }
    argp = p->argums;
    nxtval = **argp++;
    if (**argp <= FL(0.0))  return OK;          /* if idur1 <= 0, skip init  */
    p->cursegp = segp;                          /* else proceed from 1st seg */
    segp--;
    p->segsrem = nsegs;
    do {
      segp++;           /* init each seg ..  */
      val = nxtval;
      dur = **argp++;
      nxtval = **argp++;
      if (UNLIKELY(val * nxtval <= FL(0.0)))
        goto experr;
      d = dur * CS_EKR;
      segp->val = val;
      segp->mlt = (MYFLT) pow((double)(nxtval / val), (1.0/(double)d));
      segp->cnt = (int32_t) (d + FL(0.5));
      d = dur * csound->esr;
      segp->amlt = (MYFLT) pow((double)(nxtval / val), (1.0/(double)d));
      segp->acnt = (int32_t) (d + FL(0.5));
    } while (--nsegs);
    segp->cnt = MAXPOS;         /* set last cntr to infin */
    segp->acnt = MAXPOS;         /* set last cntr to infin */
    return OK;

 experr:
    n = segp - p->cursegp + 1;
    if (val == FL(0.0))
      return csound->InitError(csound, Str("ival%d is zero"), n);
    else if (nxtval == FL(0.0))
      return csound->InitError(csound, Str("ival%d is zero"), n+1);
    return csound->InitError(csound, Str("ival%d sign conflict"), n+1);
}

int32_t xsgset_bkpt(CSOUND *csound, EXXPSEG *p)
{
   XSEG        *segp;
    int32_t         nsegs;
    MYFLT       d, **argp, val, dur, dursum = FL(0.0), bkpt, nxtval;
    int32_t         n=0;


    if (!(p->INCOUNT & 1)){
      return csound->InitError(csound, Str("incomplete number of input arguments"));
    }

    /* count segs & alloc if nec */
    nsegs = (p->INOCOUNT - (!(p->INOCOUNT & 1))) >> 1;
    if ((segp = (XSEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(XSEG) < (uint32_t)p->auxch.size) {
      csound->AuxAlloc(csound, (int32_t)nsegs*sizeof(XSEG), &p->auxch);
      p->cursegp = segp = (XSEG *) p->auxch.auxp;
      (segp+nsegs-1)->cnt = MAXPOS;   /* set endcount for safety */
    }
    argp = p->argums;
    nxtval = **argp++;
    if (**argp <= FL(0.0))  return OK;          /* if idur1 <= 0, skip init  */
    p->cursegp = segp;                          /* else proceed from 1st seg */
    segp--;
    p->segsrem = nsegs;
    do {
      segp++;           /* init each seg ..  */
      val = nxtval;
      bkpt = **argp++;
      if (UNLIKELY(bkpt < dursum))
          return csound->InitError(csound,
                                   Str("Breakpoint time %f not valid"), bkpt);
      dur = bkpt - dursum;
      dursum += dur;
      nxtval = **argp++;
      if (UNLIKELY(val * nxtval <= FL(0.0)))
        goto experr;
      d = dur * CS_EKR;
      segp->val = val;
      segp->mlt = (MYFLT) pow((double)(nxtval / val), (1.0/(double)d));
      segp->cnt = (int32_t) (d + FL(0.5));
      d = dur * csound->esr;
      segp->amlt = (MYFLT) pow((double)(nxtval / val), (1.0/(double)d));
      segp->acnt = (int32_t) (d + FL(0.5));
    } while (--nsegs);
    segp->cnt = MAXPOS;         /* set last cntr to infin */
    segp->acnt = MAXPOS;
    return OK;

 experr:
    n = segp - p->cursegp + 1;
    if (val == FL(0.0))
      return csound->InitError(csound, Str("ival%d is zero"), n);
    else if (nxtval == FL(0.0))
      return csound->InitError(csound, Str("ival%d is zero"), n+1);
    return csound->InitError(csound, Str("ival%d sign conflict"), n+1);
}


int32_t xsgset2b(CSOUND *csound, EXPSEG2 *p)
{
    XSEG        *segp;
    int32_t         nsegs;
    MYFLT       d, **argp, val, dur, dursum = FL(0.0), bkpt, nxtval;
    int32_t         n;


    if (!(p->INCOUNT & 1)){
      return csound->InitError(csound, Str("incomplete number of input arguments"));
    }

    /* count segs & alloc if nec */
    nsegs = (p->INOCOUNT - (!(p->INOCOUNT & 1))) >> 1;
    if ((segp = (XSEG*) p->auxch.auxp) == NULL ||
        (uint32_t)nsegs*sizeof(XSEG) > (uint32_t)p->auxch.size) {
      csound->AuxAlloc(csound, (int32_t)nsegs*sizeof(XSEG), &p->auxch);
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
      bkpt = **argp++;
      if (UNLIKELY(bkpt < dursum))
          return csound->InitError(csound,
                                   Str("Breakpoint time %f not valid"), bkpt);
      dur = bkpt - dursum;
      dursum += dur;
      nxtval = **argp++;
/*       if (dur > FL(0.0)) { */
      if (UNLIKELY(val * nxtval <= FL(0.0)))
          goto experr;
        d = dur * csound->esr;
        segp->val = val;
        segp->mlt = POWER((nxtval / val), FL(1.0)/d);
        segp->cnt = (int32_t) (d + FL(0.5));
         d = dur * csound->esr;
         segp->amlt = (MYFLT) pow((double)(nxtval / val), (1.0/(double)d));
         segp->acnt = (int32_t) (d + FL(0.5));
/*       } */
/*       else break;               /\*  .. til 0 dur or done *\/ */
    } while (--nsegs);
    segp->cnt = MAXPOS;         /* set last cntr to infin */
     segp->acnt = MAXPOS;
    return OK;

 experr:
    n = segp - p->cursegp + 1;
    if (val == FL(0.0))
      return csound->InitError(csound, Str("ival%d is zero"), n);
    else if (nxtval == FL(0.0))
      return csound->InitError(csound, Str("ival%d is zero"), n+1);
    return csound->InitError(csound, Str("ival%d sign conflict"), n+1);
}

int32_t xsgset2(CSOUND *csound, EXPSEG2 *p)   /*gab-A1 (G.Maldonado) */
{
    XSEG        *segp;
    int32_t         nsegs;
    MYFLT       d, **argp, val, dur, nxtval;
    int32_t         n;


    if (!(p->INCOUNT & 1)){
      return csound->InitError(csound, Str("incomplete number of input arguments"));
    }

    /* count segs & alloc if nec */
    nsegs = (p->INOCOUNT - (!(p->INOCOUNT & 1))) >> 1;
    if ((segp = (XSEG*) p->auxch.auxp) == NULL ||
        (uint32_t)nsegs*sizeof(XSEG) > (uint32_t)p->auxch.size) {
      csound->AuxAlloc(csound, (int32_t)nsegs*sizeof(XSEG), &p->auxch);
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
/*       if (dur > FL(0.0)) { */
      if (UNLIKELY(val * nxtval <= FL(0.0)))
          goto experr;
        d = dur * csound->esr;
        segp->val = val;
        segp->mlt = POWER((nxtval / val), FL(1.0)/d);
        segp->cnt = (int32_t) (d + FL(0.5));
        d = dur * csound->esr;
        segp->amlt = (MYFLT) pow((double)(nxtval / val), (1.0/(double)d));
        segp->acnt = (int32_t) (d + FL(0.5));
/*       } */
/*       else break;               /\*  .. til 0 dur or done *\/ */
    } while (--nsegs);
    segp->cnt = MAXPOS;         /* set last cntr to infin */
    segp->acnt = MAXPOS;
    return OK;

 experr:
    n = segp - p->cursegp + 1;
    if (val == FL(0.0))
      return csound->InitError(csound, Str("ival%d is zero"), n);
    else if (nxtval == FL(0.0))
      return csound->InitError(csound, Str("ival%d is zero"), n+1);
    return csound->InitError(csound, Str("ival%d sign conflict"), n+1);
}

/***************************************/

int32_t expseg2(CSOUND *csound, EXPSEG2 *p)             /* gab-A1 (G.Maldonado) */
{
    IGN(csound);
    XSEG        *segp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       val, *rs;
    segp = p->cursegp;
    val  = segp->val;
    rs   = p->rslt;
    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
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

int32_t xdsrset(CSOUND *csound, EXXPSEG *p)
{
    XSEG    *segp;
    int32_t     nsegs;
    MYFLT   **argp = p->argums;
    MYFLT   len = csound->curip->p3.value;
    MYFLT   delay = *argp[4], attack = *argp[0], decay = *argp[1];
    MYFLT   sus, dur;
    MYFLT   release = *argp[3];

    if (UNLIKELY(len<FL(0.0))) len = FL(100000.0); /* MIDI case set long */
    if (csound->curip->p3.value-delay-attack-decay<FL(0.0))
      csound->Warning(csound, Str("length of XADSR note too short"));
    len -= release;                      /* len is time remaining */
    if (UNLIKELY(len<FL(0.0))) { /* Odd case of release time greater than dur */
      release = csound->curip->p3.value; len = FL(0.0);
    }
    nsegs = 5;          /* DXDSR */
    if ((segp = (XSEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(XSEG) < (uint32_t)p->auxch.size) {
      csound->AuxAlloc(csound, (int32_t)nsegs*sizeof(XSEG), &p->auxch);
      segp = (XSEG *) p->auxch.auxp;
    }
    segp[nsegs-1].cnt = MAXPOS;         /* set endcount for safety */
    if (**argp <= FL(0.0))  return OK;  /* if idur1 <= 0, skip init  */
    p->cursegp = segp;                  /* else setup null seg0 */
    p->segsrem = nsegs;
    delay += FL(0.001);
    if (delay > len) {delay = len; len -= delay;}
    attack -= FL(0.001);
    if (attack > len) {attack = len; len -= attack;}
    if (decay > len) {decay = len; len -= decay;}
    sus = len-attack-decay;
    segp[0].val = FL(0.001);   /* Like zero start, but exponential */
    segp[0].mlt = FL(1.0);
    segp[0].cnt = (int32_t) (delay*CS_EKR + FL(0.5));
    segp[0].amlt =  FL(1.0);
    segp[0].acnt = (int32_t) (delay*CS_ESR + FL(0.5));
    dur = attack*CS_EKR;
    segp[1].val = FL(0.001);
    segp[1].mlt = POWER(FL(1000.0), FL(1.0)/dur);
    segp[1].cnt = (int32_t) (dur + FL(0.5));
    dur = attack*CS_ESR;
    segp[1].amlt = POWER(FL(1000.0), FL(1.0)/dur);
    segp[1].acnt = (int32_t) (dur + FL(0.5));
    //printf("attack: %f %f %d / %f %d\n",
    //       segp[1].val,segp[1].mlt,segp[1].cnt,segp[1].amlt, segp[1].acnt);
    dur = decay*CS_EKR;
    segp[2].val = FL(1.0);
    segp[2].mlt = POWER(*argp[2], FL(1.0)/dur);
    segp[2].cnt = (int32_t) (dur + FL(0.5));
    dur = decay*CS_ESR;
    segp[2].amlt = POWER(*argp[2], FL(1.0)/dur);
    segp[2].acnt = (int32_t) (dur + FL(0.5));
    //printf("decay: %f %f %d %f %d\n",
    //       segp[2].val,segp[2].mlt,segp[2].cnt,segp[2].amlt, segp[2].acnt);
    segp[3].val = *argp[2];
    segp[3].mlt = FL(1.0);
    segp[3].cnt = (int32_t) (sus*CS_EKR + FL(0.5));

    segp[3].amlt = FL(1.0);
    segp[3].acnt = (int32_t) (sus*CS_ESR + FL(0.5));
    //printf("sustain: %f %f %d %f %d\n",
    //       segp[3].val,segp[3].mlt,segp[3].cnt,segp[3].amlt, segp[3].acnt);
    dur = release*CS_EKR;
    segp[4].val = *argp[2];
    segp[4].mlt = POWER(FL(0.001)/(*argp[2]), FL(1.0)/dur);
    segp[4].cnt = MAXPOS; /*(int32_t) (dur + FL(0.5)); */

    dur = release*CS_ESR;
    segp[4].amlt = POWER(FL(0.001)/(*argp[2]), FL(1.0)/dur);
    segp[4].acnt = MAXPOS; /*(int32_t) (dur + FL(0.5)); */
    //printf("releaase: %f %f %d %f %d\n",
    //       segp[4].val,segp[4].mlt,segp[4].cnt,segp[4].amlt, segp[4].acnt);
    return OK;
}

/* end of XDSR */

int32_t kxpseg(CSOUND *csound, EXXPSEG *p)
{
    XSEG        *segp;


    segp = p->cursegp;
    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    while (--segp->cnt < 0)
      p->cursegp = ++segp;
    *p->rslt = segp->val;
    segp->val *= segp->mlt;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("expseg (krate): not initialised"));
}


int32_t expseg(CSOUND *csound, EXXPSEG *p)
{
    XSEG        *segp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *rs = p->rslt;



    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      segp = p->cursegp;
      if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;
      while (--segp->acnt < 0) {
        //printf("seg: val=%f amlt=%f\n", segp->val,segp->amlt );
        p->cursegp = ++segp;
        //printf("nxtseg: val=%f amlt=%f acnt=%d\n",
        //       segp->val,segp->amlt,segp->acnt);
      }
      rs[n] = segp->val;
      segp->val *= segp->amlt;
    }
    return OK;
  err1:
    return csound->PerfError(csound, &(p->h),
                             Str("expseg (arate): not initialised"));
}

int32_t xsgrset(CSOUND *csound, EXPSEG *p)
{
    int32_t     relestim;
    SEG     *segp;
    int32_t     nsegs, n = 0;
    MYFLT   **argp, prvpt;


    if (!(p->INCOUNT & 1)){
      return csound->InitError(csound, Str("incomplete number of input arguments"));
    }

    //p->xtra = -1;
    /* count segs & alloc if nec */
    nsegs = (p->INOCOUNT - (!(p->INOCOUNT & 1))) >> 1;
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        (uint32_t)nsegs*sizeof(SEG) > (uint32_t)p->auxch.size) {
      csound->AuxAlloc(csound, (int32_t)nsegs*sizeof(SEG), &p->auxch);
      p->cursegp = segp = (SEG *) p->auxch.auxp;
    }
    argp = p->argums;
    prvpt = **argp++;
    if (**argp < FL(0.0))  return OK; /* if idur1 < 0, skip init      */
    p->curval  = prvpt;
    p->curcnt  = 0;                   /* else setup null seg0         */
    p->cursegp = segp - 1;
    p->segsrem = nsegs + 1;
    do {                              /* init & chk each real seg ..  */
      MYFLT dur = **argp++;
      segp->nxtpt = **argp++;
      if ((segp->cnt = (int32_t)(dur * CS_EKR + FL(0.5))) <= 0)
        segp->cnt = 0;
      else if (segp->nxtpt * prvpt <= FL(0.0))
        goto experr;
      if ((segp->acnt = (int32_t)(dur * CS_ESR )) <= 0)
        segp->acnt = 0;
      prvpt = segp->nxtpt;
      segp++;
    } while (--nsegs);
    relestim = (int32_t)(p->cursegp + p->segsrem - 1)->cnt;
    p->xtra = relestim;
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = relestim;
    return OK;

 experr:
    n = segp - p->cursegp;// + 2;
    if (prvpt == FL(0.0))
      return csound->InitError(csound, Str("ival%d is zero"), n);
    else if (segp->nxtpt == FL(0.0))
      return csound->InitError(csound, Str("ival%d is zero"), n+1);
    return csound->InitError(csound, Str("ival%d sign conflict"), n+1);
}

/* **** MXDSR is just a construction and use of expseg */

int32_t mxdsrset(CSOUND *csound, EXPSEG *p)
{
    int32_t         relestim;
    SEG         *segp;
    int32_t         nsegs;
    MYFLT       **argp = p->argums;
    MYFLT       delay = *argp[4], attack = *argp[0], decay = *argp[1];
    MYFLT       rel = *argp[3];

    nsegs = 4;          /* DXDSR */
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(SEG) < (uint32_t)p->auxch.size) {
      csound->AuxAlloc(csound, (int32_t)nsegs*sizeof(SEG), &p->auxch);
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
    segp[0].cnt = (int32_t) (delay*CS_EKR + FL(0.5));
    segp[0].acnt = (int32_t) (delay*CS_ESR + FL(0.5));
    segp[1].nxtpt = FL(1.0);
    segp[1].cnt = (int32_t) (attack*CS_EKR + FL(0.5));
    segp[1].acnt = (int32_t) (attack*CS_ESR + FL(0.5));
    segp[2].nxtpt = *argp[2];
    segp[2].cnt = (int32_t) (decay*CS_EKR + FL(0.5));
    segp[2].acnt = (int32_t) (decay*CS_ESR + FL(0.5));
    segp[3].nxtpt = FL(0.001);
    segp[3].cnt = (int32_t) (rel*CS_EKR + FL(0.5));
    segp[3].acnt = (int32_t) (rel*CS_ESR + FL(0.5));
    relestim = (int32_t)(p->cursegp + p->segsrem - 1)->cnt;
    p->xtra = relestim;//(int32_t)(*argp[5] * CS_EKR + FL(0.5)); /* Release time?? */
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = relestim;
    return OK;
}

/* end of MXDSR */

int32_t kxpsegr(CSOUND *csound, EXPSEG *p)
{
    IGN(csound);
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

int32_t expsegr(CSOUND *csound, EXPSEG *p)
{
    IGN(csound);
    MYFLT  val, amlt, *rs = p->rslt;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;


    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }
                      /* sav the cur value    */
    val = p->curval;
    for (n=offset; n<nsmps; n++) {

      if (p->segsrem) {                   /* if no more segs putk */
        SEG *segp;
        if (p->h.insdshead->relesing && p->segsrem > 1) {
          while (p->segsrem > 1) {        /* if reles flag new    */
            segp = ++p->cursegp;          /*   go to last segment */
            p->segsrem--;
          }                               /*   get univ relestim  */
          segp->acnt = (p->xtra>=0 ? p->xtra : p->h.insdshead->xtratim)*CS_KSMPS;
          goto newm;                      /*   and set new curmlt */
        }
        if (--p->curcnt <= 0) {           /* if done cur seg      */
        chk2:
          if (p->segsrem == 2) goto putk; /*   seg Y rpts lastval */
          if (!(--p->segsrem)) goto putk; /*   seg Z now done all */
          segp = ++p->cursegp;            /*   else find nextseg  */
        newm:
          if (!(p->curcnt = segp->acnt)) { /*   nonlen = discontin */
            val = p->curval = segp->nxtpt; /*   reload & rechk  */
            goto chk2;
          }                               /*   else get new mlts  */
          if (segp->nxtpt == val) {
            p->curmlt = p->curamlt = FL(1.0);
            p->curval = val;
            goto putk;
          }
          else {
            p->curmlt = POWER((segp->nxtpt/val), FL(1.0)/segp->cnt);
            // VL: this line introduces a bug
            //p->curamlt = POWER(p->curmlt, FL(1.0)/(MYFLT)(nsmps-offset));
            // VL: this line fixes it but does not take account of offset
            p->curamlt = POWER((segp->nxtpt/val), FL(1.0)/segp->acnt);
          }
        }
        if ((amlt = p->curamlt) == FL(1.0)) goto putk;
        rs[n] = val;
        val *= amlt;
      }
      else {
      putk:
        rs[n] =  val;
      }
    }
    p->curval = val;
    return OK;
}

int32_t lnnset(CSOUND *csound, LINEN *p)
{
    MYFLT a,b,dur;
    MYFLT len = csound->curip->p3.value;

    if ((dur = *p->idur) > FL(0.0)) {
      MYFLT iris = *p->iris, idec = *p->idec;
      if (len<(iris<idec?idec:iris))
        csound->Warning(csound, Str("p3 too short in linen"));

      p->cnt1 = (int32_t)(iris * CS_EKR + FL(0.5));
      if (p->cnt1 > (int32_t)0) {
        p->inc1 = FL(1.0) / (MYFLT) p->cnt1;
      }
      else p->inc1 = FL(1.0);
      a = dur * CS_EKR + FL(0.5);
      b = idec * CS_EKR + FL(0.5);
      if ((int32_t) b > 0) {
        p->cnt2 = (int32_t) (a - b);
        p->inc2 = FL(1.0) /  b;
      }
      else {
        p->inc2 = FL(1.0);
        p->cnt2 = (int32_t) a;
      }
      p->lin1 = FL(0.0);
      p->lin2 = FL(1.0);
    }
    return OK;
}

int32_t alnnset(CSOUND *csound, LINEN *p)
{
    MYFLT a,b,dur;
    MYFLT len = csound->curip->p3.value;

    if ((dur = *p->idur) > FL(0.0)) {
      MYFLT iris = *p->iris, idec = *p->idec;
      if (len<(iris<idec?idec:iris))
        csound->Warning(csound, Str("p3 too short in linen"));
      p->cnt1 = (int64_t)(*p->iris * CS_ESR + FL(0.5));
      if (p->cnt1 > 0) {
        p->inc1 = FL(1.0) / (MYFLT) p->cnt1;
      }
      else p->inc1 = FL(1.0);
      a = dur * CS_ESR + FL(0.5);
      b = *p->idec * CS_ESR + FL(0.5);
      if ((int64_t) b > 0) {
        p->cnt2 = (int64_t) (a - b);
        p->inc2 = FL(1.0) /  b;
      }
      else {
        p->inc2 = FL(1.0);
        p->cnt2 = (int64_t) a;
      }
      p->lin1 = FL(0.0);
      p->lin2 = FL(1.0);
    }
    return OK;
}

int32_t klinen(CSOUND *csound, LINEN *p)
{
    IGN(csound);
    MYFLT fact = FL(1.0);

    if (p->cnt1 > 0) {
      fact = p->lin1;
      p->lin1 += p->inc1;
      p->cnt1--;
    }
    if (p->cnt2 > 0)
      p->cnt2--;
    else {
      fact *= p->lin2;
      p->lin2 -= p->inc2;
    }
    *p->rslt = *p->sig * fact;
    return OK;
}

int32_t linen(CSOUND *csound, LINEN *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t flag=0, n, nsmps = CS_KSMPS;
    MYFLT *rs,*sg,val;
    int32_t    asgsg = IS_ASIG_ARG(p->sig);
    int64_t cnt2 = p->cnt2;
    int64_t cnt1 = p->cnt1;
    MYFLT lin1 = p->lin1;
    MYFLT lin2 = p->lin2;

    rs = p->rslt;
    sg = p->sig;

    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=offset; n<nsmps; n++) {
    val = FL(1.0);
    if (cnt1 > 0) {
      flag = 1;
      val = lin1;
      lin1 += p->inc1;
      cnt1--;
    }

    if (cnt2 > 0){
      cnt2--;
    }
    else {
      val *= lin2;
      lin2 -= p->inc2;
      flag = 1;
    }

    if (flag) {
      if (asgsg)
          rs[n] = sg[n] * val;
      else
          rs[n] = *sg * val;
      }
    else {
     if (asgsg)
        rs[n] = sg[n];
     else rs[n] = *sg;
     }
    flag = 0;
    }
    p->cnt2 = cnt2;
    p->cnt1 = cnt1;
    p->lin1 = lin1;
    p->lin2 = lin2;
    return OK;
}



int32_t lnrset(CSOUND *csound, LINENR *p)
{
    p->cnt1 = (int32_t)(*p->iris * CS_EKR + FL(0.5));
    if (p->cnt1 > 0L) {
      p->inc1 = FL(1.0) / (MYFLT)p->cnt1;
      p->val = FL(0.0);
    }
    else p->inc1 = p->val = FL(1.0);
    if (*p->idec > FL(0.0)) {
      int32_t relestim = (int32_t)(*p->idec * CS_EKR + FL(0.5));

      if (relestim > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = relestim;
      if (UNLIKELY(*p->iatdec <= FL(0.0))) {
        return csound->InitError(csound, Str("non-positive iatdec"));
      }
      else p->mlt2 = POWER(*p->iatdec, CS_ONEDKR / *p->idec);
    }
    else p->mlt2 = FL(1.0);
    p->lin1 = FL(0.0);
    p->val2 = FL(1.0);
    return OK;
}

int32_t alnrset(CSOUND *csound, LINENR *p)
{
    p->cnt1 = (int32_t)(*p->iris * CS_ESR);
    if (p->cnt1 > 0L) {
      p->inc1 = FL(1.0) / (MYFLT)p->cnt1;
      p->val = FL(0.0);
    }
    else p->inc1 = p->val = FL(1.0);
    if (*p->idec > FL(0.0)) {
      int32_t relestim = (int32_t)(*p->idec * CS_EKR + FL(0.5));
      if (relestim > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = relestim;
      if (UNLIKELY(*p->iatdec <= FL(0.0))) {
        return csound->InitError(csound, Str("non-positive iatdec"));
      }
      else p->mlt2 = POWER(*p->iatdec, csound->onedsr / *p->idec);
    }
    else p->mlt2 = FL(1.0);
    p->lin1 = FL(0.0);
    p->val2 = FL(1.0);
    return OK;
}


int32_t klinenr(CSOUND *csound, LINENR *p)
{
   IGN(csound);
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

int32_t linenr(CSOUND *csound, LINENR *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t flag=0, n, nsmps = CS_KSMPS;
    MYFLT *rs,*sg,val,val2 = p->val2;
    int32_t    asgsg = IS_ASIG_ARG(p->sig);
    val = p->val;
    rs = p->rslt;
    sg = p->sig;
    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
       nsmps -= early;
       memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      if (p->cnt1 > 0L) {
        flag = 1;
        val = p->lin1;
        p->lin1 += p->inc1;
        p->cnt1--;
      }
      if (p->h.insdshead->relesing) {
        flag = 1;
        val = p->cnt1==0L ? val2 :val*val2;
        //val *= val2;              /* If val = val2 jumps */
        val2 *= p->mlt2;
      }
      if (flag) {
        if (asgsg)
          rs[n] = sg[n] * val;
        else
          rs[n] = *sg * val;
      }
      else {
        if (asgsg) rs[n] = sg[n];
        else rs[n] = *sg;
      }
    }
    p->val = val;
    p->val2 = val2;
    return OK;
}

int32_t evxset(CSOUND *csound, ENVLPX *p)
{
    FUNC        *ftp;
    MYFLT       ixmod, iatss, idur, prod, diff, asym, nk, denom, irise;
    int32_t       cnt1;
    MYFLT       len = csound->curip->p3.value;

    if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
      return NOTOK;
    p->ftp = ftp;
    if ((idur = *p->idur) > FL(0.0)) {
      if (UNLIKELY((iatss = FABS(*p->iatss)) == FL(0.0))) {
        return csound->InitError(csound, "iatss = 0");
      }
      if (iatss != FL(1.0) && (ixmod = *p->ixmod) != FL(0.0)) {
        if (UNLIKELY(FABS(ixmod) > FL(0.95))) {
          return csound->InitError(csound, Str("ixmod out of range."));
        }
        ixmod = -SIN(SIN(ixmod));
        prod = ixmod * iatss;
        diff = ixmod - iatss;
        denom = diff + prod + FL(1.0);
        if (denom == FL(0.0))
          asym = FHUND;
        else {
          asym = FL(2.0) * prod / denom;
          if (FABS(asym) > FHUND)
            asym = FHUND;
        }
        iatss = (iatss - asym) / (FL(1.0) - asym);
        asym = asym* *(ftp->ftable + ftp->flen); /* +1 */
      }
      else asym = FL(0.0);
      if ((irise = *p->irise) > FL(0.0)) {
        if (irise + *p->idec > len)
          csound->Warning(csound, Str("p3 too short in envlpx"));
        p->phs = 0;
        p->ki = (int32_t) (CS_KICVT / irise);
        p->val = *ftp->ftable;
      }
      else {
        p->phs = -1;
        p->val = *(ftp->ftable + ftp->flen)-asym;
        irise = FL(0.0);  /* in case irise < 0 */
      }
      if (UNLIKELY(!(*(ftp->ftable + ftp->flen)))) {
        return csound->InitError(csound, Str("rise func ends with zero"));
      }
      cnt1 = (int32_t) ((idur - irise - *p->idec) * CS_EKR + FL(0.5));
      if (cnt1 < 0L) {
        cnt1 = 0;
        nk = CS_EKR;
      }
      else {
        if (*p->iatss < FL(0.0) || cnt1 <= 4L)
          nk = CS_EKR;
        else nk = (MYFLT) cnt1;
      }
      p->mlt1 = POWER(iatss, (FL(1.0)/nk));
      if (*p->idec > FL(0.0)) {
        if (UNLIKELY(*p->iatdec <= FL(0.0))) {
          return csound->InitError(csound, Str("non-positive iatdec"));
        }
        p->mlt2 = POWER(*p->iatdec, (CS_ONEDKR / *p->idec));
      }
      p->cnt1 = cnt1;
      p->asym = asym;
    }
    return OK;
}

int32_t knvlpx(CSOUND *csound, ENVLPX *p)
{
    FUNC        *ftp;
    int32_t       phs;
    MYFLT       fact, v1, fract, *ftab;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;        /* RWD fix */

    if ((phs = p->phs) >= 0) {
      fract = (MYFLT) PFRAC(phs);
      ftab = ftp->ftable + (phs >> ftp->lobits);
      v1 = *ftab++;
      fact = (v1 + (*ftab - v1) * fract);
      phs += p->ki;
      if (phs >= MAXLEN) {  /* check that 2**N+1th pnt is good */
        p->val = *(ftp->ftable + ftp->flen );
        if (UNLIKELY(!p->val)) {
          return csound->PerfError(csound, &(p->h),
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
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("envlpx(krate): not initialised"));
}

int32_t aevxset(CSOUND *csound, ENVLPX *p)
{
    FUNC        *ftp;
    MYFLT       ixmod, iatss, idur, prod, diff, asym, nk, denom, irise;
    int32_t       cnt1;
    MYFLT       len = csound->curip->p3.value;

    if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
      return NOTOK;
    p->ftp = ftp;
    if ((idur = *p->idur) > FL(0.0)) {
      if (UNLIKELY((iatss = FABS(*p->iatss)) == FL(0.0))) {
        return csound->InitError(csound, "iatss = 0");
      }
      if (iatss != FL(1.0) && (ixmod = *p->ixmod) != FL(0.0)) {
        if (UNLIKELY(FABS(ixmod) > FL(0.95))) {
          return csound->InitError(csound, Str("ixmod out of range."));
        }
        ixmod = -SIN(SIN(ixmod));
        prod = ixmod * iatss;
        diff = ixmod - iatss;
        denom = diff + prod + FL(1.0);
        if (denom == FL(0.0))
          asym = FHUND;
        else {
          asym = FL(2.0) * prod / denom;
          if (FABS(asym) > FHUND)
            asym = FHUND;
        }
        iatss = (iatss - asym) / (FL(1.0) - asym);
        asym = asym* *(ftp->ftable + ftp->flen); /* +1 */
      }
      else asym = FL(0.0);

      if ((irise = *p->irise) > FL(0.0)) {
        if (irise + *p->idec > len)
          csound->Warning(csound, Str("p3 too short in envlpx"));
        p->phs = 0;
        p->ki = (int32_t) ((FMAXLEN / CS_ESR )/ irise);
        p->val = *ftp->ftable;
      }
      else {
        p->phs = -1;
        p->val = *(ftp->ftable + ftp->flen)-asym;
        irise = FL(0.0);  /* in case irise < 0 */
      }
      if (UNLIKELY(!(*(ftp->ftable + ftp->flen)))) {
        return csound->InitError(csound, Str("rise func ends with zero"));
      }
      cnt1 = (int32_t) ((idur - irise - *p->idec) * CS_ESR);
      if (cnt1 < 0L) {
        cnt1 = 0;
        nk = CS_ESR;
      }
      else {
        if (*p->iatss < FL(0.0) || cnt1 <= 4L)
          nk = CS_ESR;
        else nk = (MYFLT) cnt1;
      }
      p->mlt1 = POWER(iatss, (FL(1.0)/nk));
      if (*p->idec > FL(0.0)) {
        if (UNLIKELY(*p->iatdec <= FL(0.0))) {
          return csound->InitError(csound, Str("non-positive iatdec"));
        }
        p->mlt2 = POWER(*p->iatdec, (csound->onedsr / *p->idec));
      }
      p->cnt1 = cnt1;
      p->asym = asym;
    }
    return OK;
}


int32_t envlpx(CSOUND *csound, ENVLPX *p)
{
    int32_t       phs;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int64_t pos, lobits, lomask;
    MYFLT      fact, *xamp, *rslt, val, asym, mlt, mlt2, v1, fract, *ftab, lodiv;
    int32_t        asgsg = IS_ASIG_ARG(p->xamp);
    xamp = p->xamp;
    rslt = p->rslt;
    val  = p->val;
    mlt = p->mlt1;
    mlt2 = p->mlt2;
    asym = p->asym;

    if (UNLIKELY(p->ftp==NULL))
      return csound->PerfError(csound, &(p->h),
                             Str("envlpx(krate): not initialised"));
    ftab = p->ftp->ftable;
    lobits = p->ftp->lobits;
    lomask = p->ftp->lomask;
    lodiv  = p->ftp->lodiv;
    if (UNLIKELY(ftab[p->ftp->flen] == 0.0))
      return csound->PerfError(csound, &(p->h),
                             Str("envlpx rise func ends with zero"));

    if (UNLIKELY(offset)) memset(rslt, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }


    for (n=offset; n<nsmps;n++) {

     if ((phs = p->phs) >= 0L) {
       fract = ((phs) & lomask) * lodiv;
       pos = (int64_t) (phs >> lobits);
       v1 = ftab[pos+1];
       fact = (v1 + (ftab[pos] - v1) * fract);
       phs += p->ki;
       if (phs >= MAXLEN) {
        val = ftab[p->ftp->flen];
        val -= p->asym;
        phs = -1;
      }
      p->phs = phs;
    }
    else {
      fact = val;
      if (p->cnt1 > 0L) {
        val *= mlt;
        fact += asym;
        p->cnt1--;
        if (p->cnt1 == 0L)
          val += asym;
      }
      else val *= mlt2;
    }
    if (asgsg)
        rslt[n] = xamp[n] * fact;
    else
        rslt[n] = *xamp * fact;
    }
    p->val = val;
    return OK;
}

int32_t evrset(CSOUND *csound, ENVLPR *p)
{
    FUNC        *ftp;
    MYFLT       ixmod, iatss, prod, diff, asym, denom, irise;

    if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
      return NOTOK;
    p->ftp = ftp;
    if (UNLIKELY((iatss = FABS(*p->iatss)) == FL(0.0))) {
      return csound->InitError(csound, "iatss = 0");
    }
    if (iatss != FL(1.0) && (ixmod = *p->ixmod) != FL(0.0)) {
      if (UNLIKELY(FABS(ixmod) > FL(0.95))) {
        return csound->InitError(csound, Str("ixmod out of range."));
      }
      ixmod = -SIN(SIN(ixmod));
      prod  = ixmod * iatss;
      diff  = ixmod - iatss;
      denom = diff + prod + FL(1.0);
      if (denom == FL(0.0))
        asym = FHUND;
      else {
        asym = FL(2.0) * prod / denom;
        if (FABS(asym) > FHUND)
          asym = FHUND;
      }
      iatss = (iatss - asym) / (FL(1.0) - asym);
      asym = asym * *(ftp->ftable + ftp->flen); /* +1 */
    }
    else asym = FL(0.0);
    if ((irise = *p->irise) > FL(0.0)) {
      p->phs = 0;
      p->ki = (int32_t) (CS_KICVT / irise);
      p->val = *ftp->ftable;
    }
    else {
      p->phs = -1;
      p->val = *(ftp->ftable + ftp->flen)-asym;
      /* irise = FL(0.0); */          /* in case irise < 0 */
    }
    if (UNLIKELY(!(*(ftp->ftable + ftp->flen)))) {
      return csound->InitError(csound, Str("rise func ends with zero"));
    }
    p->mlt1 = POWER(iatss, CS_ONEDKR);
    if (*p->idec > FL(0.0)) {
      int32_t rlscnt = (int32_t)(*p->idec * CS_EKR + FL(0.5));
      if ((p->rindep = (int32_t)*p->irind))
        p->rlscnt = rlscnt;
      else if (rlscnt > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = (int32_t)rlscnt;
      if (UNLIKELY((p->atdec = *p->iatdec) <= FL(0.0) )) {
        return csound->InitError(csound, Str("non-positive iatdec"));
      }
    }
    p->asym = asym;
    p->rlsing = 0;
    return OK;
}

int32_t aevrset(CSOUND *csound, ENVLPR *p)
{
    FUNC        *ftp;
    MYFLT       ixmod, iatss, prod, diff, asym, denom, irise;

    if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
      return NOTOK;
    p->ftp = ftp;
    if (UNLIKELY((iatss = FABS(*p->iatss)) == FL(0.0))) {
      return csound->InitError(csound, "iatss = 0");
    }
    if (iatss != FL(1.0) && (ixmod = *p->ixmod) != FL(0.0)) {
      if (UNLIKELY(FABS(ixmod) > FL(0.95))) {
        return csound->InitError(csound, Str("ixmod out of range."));
      }
      ixmod = -SIN(SIN(ixmod));
      prod  = ixmod * iatss;
      diff  = ixmod - iatss;
      denom = diff + prod + FL(1.0);
      if (denom == FL(0.0))
        asym = FHUND;
      else {
        asym = FL(2.0) * prod / denom;
        if (FABS(asym) > FHUND)
          asym = FHUND;
      }
      iatss = (iatss - asym) / (FL(1.0) - asym);
      asym = asym * *(ftp->ftable + ftp->flen); /* +1 */
    }
    else asym = FL(0.0);
    if ((irise = *p->irise) > FL(0.0)) {
      p->phs = 0;
      p->ki = (int32_t) ((FMAXLEN / CS_ESR)/ irise);
      p->val = *ftp->ftable;
    }
    else {
      p->phs = -1;
      p->val = *(ftp->ftable + ftp->flen)-asym;
      /* irise = FL(0.0); */          /* in case irise < 0 */
    }
    if (UNLIKELY(!(*(ftp->ftable + ftp->flen)))) {
      return csound->InitError(csound, Str("rise func ends with zero"));
    }
    p->mlt1 = POWER(iatss, csound->onedsr);
    if (*p->idec > FL(0.0)) {
      int32_t rlscnt = (int32_t)(*p->idec * CS_EKR + FL(0.5));
      if ((p->rindep = (int32_t)*p->irind))
        p->rlscnt = rlscnt;
      else if (rlscnt > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = (int32_t)rlscnt;
      if (UNLIKELY((p->atdec = *p->iatdec) <= FL(0.0) )) {
        return csound->InitError(csound, Str("non-positive iatdec"));
      }
    }
    p->asym = asym;
    p->rlsing = 0;
    return OK;
}


int32_t knvlpxr(CSOUND *csound, ENVLPR *p)
{
    IGN(csound);
    MYFLT  fact;
    int32_t  rlscnt;

    if (!p->rlsing) {                   /* if not in reles seg  */
      if (p->h.insdshead->relesing) {
        p->rlsing = 1;                  /*   if new flag, set mlt2 */
        rlscnt = (p->rindep) ? p->rlscnt : p->h.insdshead->xtratim;
        if (rlscnt)
          p->mlt2 = POWER(p->atdec, FL(1.0)/rlscnt);
        else p->mlt2 = FL(1.0);
      }
      if (p->phs >= 0) {                /* do fn rise for seg 1 */
        FUNC *ftp = p->ftp;
        int32_t phs = p->phs;
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

int32_t envlpxr(CSOUND *csound, ENVLPR *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  rlscnt;
    int64_t lobits, lomask, pos, phs = p->phs;
    MYFLT fact, *xamp, *rslt, val, asym, mlt, v1, fract, *ftab, lodiv;
    int32_t    asgsg = IS_ASIG_ARG(p->xamp);

    xamp = p->xamp;
    rslt = p->rslt;
    val  = p->val;
    mlt = p->mlt1;
    //mlt2 = p->mlt2;
    asym = p->asym;

    if (UNLIKELY(p->ftp==NULL))
      return csound->PerfError(csound, &(p->h),
                             Str("envlpx(krate): not initialised"));
    ftab = p->ftp->ftable;
    lobits = p->ftp->lobits;
    lomask = p->ftp->lomask;
    lodiv  = p->ftp->lodiv;
    if (UNLIKELY(ftab[p->ftp->flen] == 0.0))
      return csound->PerfError(csound, &(p->h),
                             Str("envlpx rise func ends with zero"));

    if (UNLIKELY(offset)) memset(rslt, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=offset; n<nsmps; n++) {

      if (!p->rlsing) {                   /* if not in reles seg  */
        if (p->h.insdshead->relesing) {
          p->rlsing = 1;                  /*   if new flag, set mlt2 */
          rlscnt = (p->rindep) ? p->rlscnt : p->h.insdshead->xtratim;
          rlscnt *= CS_KSMPS;
          if (rlscnt)
            p->mlt2 = POWER(p->atdec, FL(1.0)/rlscnt);
          else p->mlt2 = FL(1.0);
        }
        if (p->phs >= 0) {                /* do fn rise for seg 1 */
          fract = ((phs) & lomask) * lodiv;
          pos = (int64_t) (phs >> lobits);
          v1 = ftab[pos+1];
          fact = (v1 + (ftab[pos] - v1) * fract);
          phs += p->ki;
          if (phs >= MAXLEN) {
            val = ftab[p->ftp->flen];
            val -= p->asym;
            phs = -1;
          }
          else val = fact;      /* JPff: in case very early release */
          p->phs = phs;
        }
        else {
          fact = val + asym;
          val *= mlt;
          if (p->rlsing)
            val += asym;
        }
      }
      else
        fact = val *= p->mlt2;     /* else do seg 3 decay  */

      if (asgsg)
        rslt[n] = xamp[n] * fact;
      else
        rslt[n] = *xamp * fact;
    }
    p->val = val;
    return OK;
}

int32_t csgset(CSOUND *csound, COSSEG *p)
{
    SEG *segp, *sp;
    int32_t nsegs;
    MYFLT       **argp;
    double val, y1, y2;


    if (!(p->INCOUNT & 1)) {
      return csound->InitError(csound, Str("incomplete number of input arguments"));
    }

    /* count segs & alloc if nec */
    nsegs = (p->INOCOUNT - (!(p->INOCOUNT & 1))) >> 1;
    //printf("****nsegs = %d\n", nsegs);
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(SEG) < (uint32_t)p->auxch.size) {
      csound->AuxAlloc(csound, (int32_t)(1+nsegs)*sizeof(SEG), &p->auxch);
      p->cursegp = 1+(segp = (SEG *) p->auxch.auxp);
      segp[nsegs-1].cnt = MAXPOS; /* set endcount for safety */
      segp[nsegs-1].acnt = MAXPOS;
    }
    sp = segp;
    argp = p->argums;
    y1 = val = (double)**argp++;
    if (UNLIKELY(**argp <= FL(0.0)))  return OK;    /* if idur1 <= 0, skip init  */
    p->curcnt = 0;
    p->cursegp = segp+1;          /* else setup first seg */
    p->segsrem = nsegs;
    //printf("****current seg = %p segp = %p\n", p->cursegp, segp);
    do {                                /* init each seg ..  */
      double dur = (double)**argp++;
      segp->nxtpt = (double)**argp++;
      if (UNLIKELY((segp->cnt = (int32_t)(dur * CS_EKR + FL(0.5))) < 0))
        segp->cnt = 0;
      if (UNLIKELY((segp->acnt = (int32_t)(dur * CS_ESR)) < 0))
        segp->acnt = 0;

      //printf("****i: %d(%p): cnt=%d nxtpt=%f\n",
      //       p->segsrem-nsegs, segp, segp->cnt, segp->nxtpt);
      segp++;
    } while (--nsegs);
    p->y1 = y1;
    p->y2 = y2 = sp->nxtpt;
    p->x = 0.0;
    if (IS_ASIG_ARG(p->rslt)) {
      p->inc = (y2!=y1 ? 1.0/(sp->acnt) : 0.0);
      p->curcnt = sp->acnt;
    }
    else {
      p->inc = (y2!=y1 ? 1.0/(sp->cnt) : 0.0);
      p->curcnt = sp->cnt;
    }
    //printf("****incx, y1,y2 = %g, %f, %f\n", p->inc, p->y1, p->y2);
    p->val = p->y1;
    return OK;
}

int32_t csgset_bkpt(CSOUND *csound, COSSEG *p)
{
    int32_t cnt, bkpt = 0;
    int32_t nsegs;
    int32_t n;
    SEG *segp;
    n = csgset(csound, p);
    if (UNLIKELY(n!=0)) return n;
    cnt = p->curcnt;
    nsegs = p->segsrem-1;
    segp = p->cursegp;
    if (IS_ASIG_ARG(p->rslt))
    do {
      if (UNLIKELY(cnt > segp->acnt))
        return csound->InitError(csound, Str("Breakpoint %d not valid"), bkpt);
      segp->acnt -= cnt;
      cnt += segp->acnt;
      segp++;
      bkpt++;
    } while (--nsegs);
    else
    do {
      //csound->Message(csound, "%d/ %d: %d, %d ", nsegs, bkpt, cnt, segp->cnt);
      if (UNLIKELY(cnt > segp->cnt))
        return csound->InitError(csound, Str("Breakpoint %d not valid"), bkpt);
      segp->cnt -= cnt;
      cnt += segp->cnt;
      //csound->Message(csound, "-> %d, %d %f\n", cnt, segp->cnt, segp->nxtpt);
      segp++;
      bkpt++;
    } while (--nsegs);

    return OK;
}

int32_t csgrset(CSOUND *csound, COSSEG *p)
{
    int32_t relestim;
    if (csgset(csound,p) != 0) return NOTOK;
    relestim = (p->cursegp + p->segsrem-2)->cnt;
    p->xtra = relestim;
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = (int32_t)relestim;
    return OK;
}

int32_t kosseg(CSOUND *csound, COSSEG *p)
{
    double val1 = p->y1, val2 = p->y2, x = p->x;
    double inc = p->inc;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;          /* RWD fix */

    if (LIKELY(p->segsrem)) {             /* if no more segs putk */
      if (--p->curcnt <= 0) {             /*  if done cur segment */
        SEG *segp = p->cursegp;
      chk1:
        p->y1 = val1 = val2;
        if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
          p->y2 = val2 = segp->nxtpt;
          goto putk;                      /*      put endval      */
        }
        //printf("new seg: %d %f\n", segp->cnt, segp->nxtpt);
        val2 = p->y2 = segp->nxtpt;          /* Base of next segment */
        inc = p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
        x = 0.0;
        p->cursegp = segp+1;              /*   else find the next */
        if (UNLIKELY(!(p->curcnt = segp->cnt))) {
          val2 = p->y2 = segp->nxtpt;  /* nonlen = discontin */
          /* inc = */ p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
          goto chk1;
        }                                 /*   poslen = new slope */
      }
      {
        double mu2 = (1.0-cos(x*PI))*0.5;
        *p->rslt = (MYFLT)(val1*(1.0-mu2)+val2*mu2);
        x += inc;
      }
    }
    else {
    putk:
      *p->rslt = (MYFLT)val1;
    }
    p->x = x;
    return OK;
 err1:
    return csound->InitError(csound, Str("cosseg not initialised (krate)\n"));
}

int32_t cosseg(CSOUND *csound, COSSEG *p)
{
    double val1 = p->y1, val2 = p->y2, x = p->x;
    MYFLT *rs = p->rslt;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    double inc = p->inc;///(nsmps-offset);

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;

    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=offset; n<nsmps; n++) {
      double mu2;
      if (LIKELY(p->segsrem)) {             /* if no more segs putk */
        if (--p->curcnt <= 0) {             /*  if done cur segment */
          SEG *segp = p->cursegp;
        chk1:
          p->y1 = val1 = val2;
          if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
            p->y2 = val2 = segp->nxtpt;
            goto putk;                      /*      put endval      */
          }
          val2 = p->y2 = segp->nxtpt;          /* Base of next segment */
          inc = (segp->acnt ? 1.0/(segp->acnt) : 0.0);
          x = 0.0;
          //printf("****new seg val1.val2=%f,%f inc=%f\n", val1,val2, inc);
          p->cursegp = segp+1;              /*   else find the next */
          if (UNLIKELY(!(p->curcnt = segp->acnt))) {
            val2 = p->y2 = segp->nxtpt;  /* nonlen = discontin */
            inc = (segp->acnt ? 1.0/(segp->acnt) : 0.0);
            //printf("****val1,val2=%f,%f inc=%f\n", val1, val2, inc);
            goto chk1;
          }                                 /*   poslen = new slope */
        }
        mu2 = (1.0-cos(x*PI))*0.5;
        //printf("****x=%f inc=%f mu2=%f\n", x, inc, mu2);
        rs[n] = (MYFLT)(val1*(1.0-mu2)+val2*mu2);
        x += inc;
      }
      else {
      putk:
        rs[n] = (MYFLT)val1;

      }
    }
    p->inc = inc;
    p->x = x;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("cosseg: not initialised (arate)\n"));
}

int32_t cossegr(CSOUND *csound, COSSEG *p)
{
    double val1 = p->y1, val2 = p->y2, x = p->x, val = p->val;
    MYFLT *rs = p->rslt;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t n, nsmps = CS_KSMPS;
    double inc = p->inc;

    memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;

    for (n=offset; n<nsmps; n++) {
      if (LIKELY(p->segsrem)) {             /* if no more segs putk */
        SEG *segp = p->cursegp;
        if (p->h.insdshead->relesing && p->segsrem > 1) {
          while (p->segsrem > 1) {            /* release flag new:    */
            segp = ++p->cursegp;              /*   go to last segment */
            p->segsrem--;
          }
          segp--;                                 /*   get univ relestim  */
          segp->acnt = (p->xtra >=0 ? p->xtra : p->h.insdshead->xtratim)*CS_KSMPS;
          //p->y1 = val1 = val2;
          p->y1 = val1 = val;
          //printf("%d(%p): cnt=%d strt= %f nxtpt=%f\n",
          //      p->segsrem, segp, segp->cnt,val1, segp->nxtpt);
          goto newi;                          /*   and set new curinc */
        }
        if (p->segsrem == 1 && !p->h.insdshead->relesing) {
          goto putk;
        }
        if (--p->curcnt <= 0) {             /*  if done cur segment */
        chk1:
          p->y1 = val1 = val2;
          if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
            p->y2 = val2 = segp->nxtpt;
            goto putk;                      /*      put endval      */
          }
        newi:
          //printf("new seg: %d %f\n", segp->cnt, segp->nxtpt);
          val2 = p->y2 = segp->nxtpt;          /* Base of next segment */
          inc =p->inc = (segp->acnt ? 1.0/(segp->acnt) : 0.0);
          x = 0.0;
          p->cursegp = segp+1;              /*   else find the next */
          if (UNLIKELY(!(p->curcnt = segp->acnt))) {
            val2 = p->y2 = segp->nxtpt;  /* nonlen = discontin */
            inc = p->inc = (segp->acnt ? 1.0/(segp->acnt) : 0.0);
            goto chk1;
          }                                 /*   poslen = new slope */
          //printf("New segment incx, y1,y2 = %g, %f, %f\n", inc, val1, val2);
        }
        {
          double mu2 = (1.0-cos(x*PI))*0.5;
          val = rs[n] = (MYFLT)(val1*(1.0-mu2)+val2*mu2);
          x += inc;
          //if (x>1 || x<0) printf("x=%f out of range\n", x);
        }
      }
      else {
      putk:
        rs[n] = (MYFLT)val1;
      }
    }
    p->inc = inc;
    p->x = x;
    p->val = val;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("cossegr: not initialised (arate)\n"));
}


#if 0
int32_t cossegr(CSOUND *csound, COSSEG *p)
{
    double val1 = p->y1, val2 = p->y2, x = p->x;
    MYFLT *rs = p->rslt;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t n, nsmps = CS_KSMPS;
    double inc = p->inc/(nsmps-offset);

    memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;

    if (LIKELY(p->segsrem)) {             /* if no more segs putk */
      SEG *segp = p->cursegp;
      if (p->h.insdshead->relesing && p->segsrem > 1) {
        while (p->segsrem > 1) {            /* release flag new:    */
          segp = ++p->cursegp;              /*   go to last segment */
          p->segsrem--;
        }                                   /*   get univ relestim  */
        segp->cnt = p->xtra >=0 ? p->xtra : p->h.insdshead->xtratim;
        goto newi;                          /*   and set new curinc */
      }
      if (--p->curcnt <= 0) {             /*  if done cur segment */
      chk1:
        p->y1 = val1 = val2;
        if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
          p->y2 = val2 = segp->nxtpt;
          goto putk;                      /*      put endval      */
        }
      newi:
        //printf("new seg: %d %f\n", segp->cnt, segp->nxtpt);
        val2 = p->y2 = segp->nxtpt;          /* Base of next segment */
        p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
        inc /= nsmps;
        x = 0.0;
        p->cursegp = segp+1;              /*   else find the next */
        if (UNLIKELY(!(p->curcnt = segp->cnt))) {
          val2 = p->y2 = segp->nxtpt;  /* nonlen = discontin */
          p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
          inc /= nsmps;
          //printf("zero length: incx, y1,y2 = %f, %f, %f\n", inc, val1, val2);
          goto chk1;
        }                                 /*   poslen = new slope */
        //printf("New segment incx, y1,y2 = %g, %f, %f\n", inc, val1, val2);
      }
      for (n=offset; n<nsmps; n++) {
        double mu2 = (1.0-cos(x*PI))*0.5;
        rs[n] = (MYFLT)(val1*(1.0-mu2)+val2*mu2);
        x += inc;
        //if (x>1 || x<0) printf("x=%f out of range\n", x);
      }
    }
    else {
    putk:
      //printf("ending at %f\n", val1);
      for (n=offset; n<nsmps; n++) {
        rs[n] = (MYFLT)val1;
      }
    }
    p->x = x;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("cossegr: not initialised (arate)\n"));
}
#endif

int32_t kcssegr(CSOUND *csound, COSSEG *p)
{
    double val1 = p->y1, val2 = p->y2, x = p->x, val = p->val;
    double inc = p->inc;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;          /* RWD fix */

    if (LIKELY(p->segsrem)) {             /* if no more segs putk */
      SEG *segp = p->cursegp;
      if (p->h.insdshead->relesing && p->segsrem > 1) {
        while (p->segsrem > 1) {           /* reles flag new:      */
          segp = ++p->cursegp;             /*   go to last segment */
          p->segsrem--;
        }
        segp--;                             /*   get univ relestim  */
        segp->cnt = p->xtra>= 0 ? p->xtra : p->h.insdshead->xtratim;
        p->y1 = val1 = val;
        goto newi;                         /*   and set new curinc */
      }
      if (p->segsrem == 1 && !p->h.insdshead->relesing) {
       goto putk;
      }
      if (--p->curcnt <= 0) {             /*  if done cur segment */
      chk1:
        p->y1 = val1 = val2;
        if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
          p->y2 = val2 = segp->nxtpt;
          goto putk;                      /*      put endval      */
        }
      newi:
        val2 = p->y2 = segp->nxtpt;          /* Base of next segment */
        inc = p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
        x = 0.0;
        p->cursegp = segp+1;              /*   else find the next */
        if (UNLIKELY(!(p->curcnt = segp->cnt))) {
          val2 = p->y2 = segp->nxtpt;  /* nonlen = discontin */
          /* inc = */ p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
          goto chk1;
        }                                 /*   poslen = new slope */
      }
      {
        double mu2 = (1.0-cos(x*PI))*0.5;
        val = *p->rslt = (MYFLT)(val1*(1.0-mu2)+val2*mu2);
        x += inc;
      }
    }
    else {
    putk:
        *p->rslt = (MYFLT)val1;
    }
    p->x = x;
    p->val = val;
    return OK;
 err1:
    return csound->InitError(csound, Str("cosseg not initialised (krate)\n"));
}
