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

#define FZERO (FL(0.0))
#define FONE  (FL(1.0))
#define FHUND (FL(100.0))

int linset(LINE *p)
{
    MYFLT       dur;

    if ((dur = *p->idur) > FZERO) {
      p->incr = (*p->ib - *p->ia) / dur * onedkr;
      p->val = *p->ia;
    }
    return OK;
}

int kline(LINE *p)
{
    *p->xr = p->val;            /* rslt = val   */
    p->val += p->incr;          /* val += incr  */
    return OK;
}

int aline(LINE *p)
{
    MYFLT       val, inc, *ar;
    int nsmps = ksmps;

    val = p->val;
    inc = p->incr;
    p->val += inc;              /* nxtval = val + inc */
    inc /= ensmps;
    ar = p->xr;
    do {
      *ar++ = val;
      val += inc;       /* interp val for ksmps */
    }
    while (--nsmps);
    return OK;
}

int expset(EXPON *p)
{
    MYFLT       dur, a, b;

    if ((dur = *p->idur) > FZERO ) {
      a = *p->ia;
      b = *p->ib;
      if ((a * b) > FZERO) {
        p->mlt = (MYFLT) pow((double)(b / a),(onedkr/(double)dur));
        p->val = a;
      }
      else if (a == FZERO)
        return initerror(Str(X_600,"arg1 is zero"));
      else if (b == FZERO)
        return initerror(Str(X_601,"arg2 is zero"));
      else return initerror(Str(X_1346,"unlike signs"));
    }
    return OK;
}

int kexpon(EXPON *p)
{
    *p->xr = p->val;            /* rslt = val   */
    p->val *= p->mlt;           /* val *= mlt  */
    return OK;
}

int expon(EXPON *p)
{
    MYFLT       val, mlt, inc, *ar,nxtval;
    int nsmps = ksmps;

    val = p->val;
    mlt = p->mlt;
    nxtval = val * mlt;
    inc = nxtval - val;
    inc /= ensmps;              /* increment per sample */
    ar = p->xr;
    do {
      *ar++ = val;
      val += inc;       /* interp val for ksmps */
    }
    while (--nsmps);
    p->val = nxtval;    /*store next value */
    return OK;
}


/* static int counter; */
int lsgset(LINSEG *p)
{
    SEG *segp;
    int nsegs;
    MYFLT       **argp, val;

    nsegs = p->INOCOUNT >> 1;           /* count segs & alloc if nec */
/*     printf("nsegs=%d\n", nsegs); */
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(SEG) < (unsigned int)p->auxch.size) {
      auxalloc((long)nsegs*sizeof(SEG), &p->auxch);
      p->cursegp = segp = (SEG *) p->auxch.auxp;
      segp[nsegs-1].cnt = MAXPOS; /* set endcount for safety */
    }
    argp = p->argums;
    val = **argp++;
    if (**argp <= FZERO)  return OK;    /* if idur1 <= 0, skip init  */
    p->curval = val;
    p->curcnt = 0;
    p->cursegp = segp - 1;          /* else setup null seg0 */
    p->segsrem = nsegs + 1;
/*     printf("Val starts at %f for %f\n", val, **argp); */
    do {                                /* init each seg ..  */
      MYFLT dur = **argp++;
      segp->nxtpt = **argp++;
      if ((segp->cnt = (long)(dur * ekr + FL(0.5))) < 0)
        segp->cnt = 0;
/*       printf("%f: cnt=%ld nxt=%f\n", dur, segp->cnt, segp->nxtpt);  */
      segp++;
    } while (--nsegs);
    p->xtra = -1;
/*     { */
/*       int i; */
/*       for (i=0; i<p->segsrem-1; i++) */
/*         printf("%d: cnt=%ld nxt=%f\n", i, */
/*            ((SEG*)p->auxch.auxp)[i].cnt, ((SEG*)p->auxch.auxp)[i].nxtpt); */
/*       counter = 0; */
/*     } */
    return OK;
}

int klnseg(LINSEG *p)
{
    *p->rslt = p->curval;               /* put the cur value    */
    if (p->auxch.auxp==NULL) {          /* RWD fix */
      die(Str(X_550,"\nError: linseg not initialised (krate)\n"));
    }
    if (p->segsrem) {                   /* done if no more segs */
      if (--p->curcnt <= 0) {           /* if done cur segment  */
        SEG *segp = p->cursegp;
        if (!(--p->segsrem))  {
          p->curval = segp->nxtpt;      /* advance the cur val  */
/*           printf("Exit case; value now %f\n", p->curval); */
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
/*           printf("Target=%f; current=%f, count=%d; inc=%.8f\n", */
/*                  segp->nxtpt,p->curval, segp->cnt, p->curinc); */
          p->curval += p->curinc;
          return OK;
        }
      }
      if (p->curcnt<10)         /* This is a fiddle to get rounding right!  */
        p->curinc = (p->cursegp->nxtpt - p->curval) / p->curcnt; /* recalc */
      p->curval += p->curinc;           /* advance the cur val  */
/*       printf("Counter = %d(%d) (%d/%f); Curval = %f ; inc = %f\n", */
/*          counter, p->curcnt, counter*ksmps, (double)(counter*ksmps_)/ekr, */
/*              p->curval, p->curinc); */
    }
/*     counter++; */
    return OK;
}

int linseg(LINSEG *p)
{
    MYFLT  val, ainc, *rs = p->rslt;
    int         nsmps = ksmps;

    if (p->auxch.auxp==NULL) {  /* RWD fix */
      return perferror(Str(X_967,"linseg: not initialised (arate)\n"));
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
        p->curainc = p->curinc / ensmps;
      }
      p->curval = val + p->curinc;        /* advance the cur val  */
      if ((ainc = p->curainc) == FZERO)
        goto putk;
      do {
        *rs++ = val;
        val += ainc;
      } while (--nsmps);
    }
    else {
putk:
      do *rs++ = val;
      while (--nsmps);
    }
    return OK;
}

/* **** ADSR is just a construction and use of linseg */

static void adsrset1(LINSEG *p, int midip)
{
    SEG         *segp;
    int         nsegs;
    MYFLT       **argp = p->argums;
    MYFLT       dur;
    MYFLT       len = curip->p3;
    MYFLT       release = *argp[3];
    long        relestim;

/*    printf("ADSR len=%f, release=%f\n", len, release); */
    if (len<=FZERO) len = FL(100000.0); /* MIDI case set long */
    len -= release;         /* len is time remaining */
    if (len<FZERO) {         /* Odd case of release time greater than dur */
      release = curip->p3; len = FZERO;
    }
    nsegs = 6;          /* DADSR */
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(SEG) < (unsigned int)p->auxch.size) {
      auxalloc((long)nsegs*sizeof(SEG), &p->auxch);
      p->cursegp = segp = (SEG *) p->auxch.auxp;
      segp[nsegs-1].cnt = MAXPOS; /* set endcount for safety */
    }
    else if (**argp > FZERO) memset(p->auxch.auxp, 0, (long)nsegs*sizeof(SEG));
    if (**argp <= FZERO)  return;       /* if idur1 <= 0, skip init  */
    p->curval = FZERO;
    p->curcnt = 0;
    p->cursegp = segp - 1;      /* else setup null seg0 */
    p->segsrem = nsegs;
                                /* Delay */
    dur = *argp[4];
    if (dur > len) dur = len;
    len -= dur;
/*      printf("    len=%f : delay=%f\n",len, dur); */
    segp->nxtpt = FZERO;
    if ((segp->cnt = (long)(dur * ekr + FL(0.5))) == 0)
      segp->cnt = 0;
    segp++;
                                /* Attack */
    dur = *argp[0];
    if (dur > len) dur = len;
    len -= dur;
/*      printf("    len=%f : attack=%f\n",len, dur); */
    segp->nxtpt = FL(1.0);
    if ((segp->cnt = (long)(dur * ekr + FL(0.5))) == 0)
      segp->cnt = 0;
    segp++;
                                /* Decay */
    dur = *argp[1];
    if (dur > len) dur = len;
    len -= dur;
/*      printf("    len=%f : decay=%f\n",len, dur); */
    segp->nxtpt = *argp[2];
    if ((segp->cnt = (long)(dur * ekr + FL(0.5))) == 0)
      segp->cnt = 0;
    segp++;
                                /* Sustain */
    /* Should use p3 from score, but how.... */
    dur = len;
/*      printf("    len=%f : sustain=%f\n",len, dur); */
/*  dur = curip->p3 - *argp[4] - *argp[0] - *argp[1] - *argp[3]; */
    segp->nxtpt = *argp[2];
    if ((segp->cnt = (long)(dur * ekr + FL(0.5))) == 0)
      segp->cnt = 0;
    segp++;
                                /* Release */
    segp->nxtpt = FZERO;
    if ((segp->cnt = (long)(release * ekr + FL(0.5))) == 0)
      segp->cnt = 0;
    if (midip) {
      p->xtra = (long)(*argp[5] * ekr + FL(0.5));      /* Release time?? */
      relestim = (p->cursegp + p->segsrem - 1)->cnt;
/*       printf("MIDI case xtra=%ld release=%ld\n", p->xtra, relestim); */
      if (relestim > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = (int)relestim;
/*       printf("MIDI case xtra=%ld release=%ld\n", p->xtra, relestim); */
    }
    else
      p->xtra = 0L;
}

int adsrset(LINSEG *p)
{
    adsrset1(p, 0);
    return OK;
}

int madsrset(LINSEG *p)
{
    adsrset1(p, 1);
    return OK;
}


/* End of ADSR */


int lsgrset(LINSEG *p)
{
    long relestim;
    lsgset(p);
    relestim = (p->cursegp + p->segsrem - 1)->cnt;
    p->xtra = -1;
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = (int)relestim;
    return OK;
}

int klnsegr(LINSEG *p)
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

int linsegr(LINSEG *p)
{
    MYFLT  val, ainc, *rs = p->rslt;
    int    nsmps = ksmps;

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
        p->curainc = p->curinc / ensmps;
      }
      p->curval = val + p->curinc;          /* advance the cur val  */
      if ((ainc = p->curainc) == FZERO)
        goto putk;
      do {
        *rs++ = val;
        val += ainc;
      } while (--nsmps);
    }
    else {
    putk:
      do *rs++ = val;
      while (--nsmps);
    }
    return OK;
}

int xsgset(EXXPSEG *p)
{
    XSEG        *segp;
    int nsegs;
    MYFLT       d, **argp, val, dur, nxtval;
    int    n=0;

    nsegs = p->INOCOUNT >> 1;                   /* count segs & alloc if nec */
    if ((segp = (XSEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(XSEG) < (unsigned int)p->auxch.size) {
      auxalloc((long)nsegs*sizeof(XSEG), &p->auxch);
      p->cursegp = segp = (XSEG *) p->auxch.auxp;
      (segp+nsegs-1)->cnt = MAXPOS;   /* set endcount for safety */
    }
    argp = p->argums;
    nxtval = **argp++;
    if (**argp <= FZERO)  return OK;            /* if idur1 <= 0, skip init  */
    p->cursegp = segp;                          /* else proceed from 1st seg */
    segp--;
    do {
      segp++;           /* init each seg ..  */
      val = nxtval;
      dur = **argp++;
      nxtval = **argp++;
      if (dur > FZERO) {
        if (val * nxtval <= FZERO)
          goto experr;
        d = dur * ekr;
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
    if (val == FZERO)
      sprintf(errmsg,Str(X_952,"ival%d is zero"), n);
    else if (nxtval == FZERO)
      sprintf(errmsg,Str(X_952,"ival%d is zero"), n+1);
    else sprintf(errmsg,Str(X_953,"ival%d sign conflict"), n+1);
    return initerror(errmsg);
}

int xsgset2(EXPSEG2 *p)  /*gab-A1 (G.Maldonado) */
{
    XSEG        *segp;
    int         nsegs;
    MYFLT       d, **argp, val, dur, nxtval;
    int         n;

    nsegs = p->INOCOUNT >> 1;           /* count segs & alloc if nec */
    if ((segp = (XSEG*) p->auxch.auxp) == NULL ||
        (unsigned int)nsegs*sizeof(XSEG) > (unsigned int)p->auxch.size) {
      auxalloc((long)nsegs*sizeof(XSEG), &p->auxch);
      p->cursegp = segp = (XSEG *) p->auxch.auxp;
      (segp+nsegs-1)->cnt = MAXPOS;   /* set endcount for safety */
    }
    argp = p->argums;
    nxtval = **argp++;
    if (**argp <= FZERO)  return OK;        /* if idur1 <= 0, skip init  */
    p->cursegp = segp;                      /* else proceed from 1st seg */
    segp--;
    do {
      segp++;           /* init each seg ..  */
      val = nxtval;
      dur = **argp++;
      nxtval = **argp++;
      if (dur > FZERO) {
        if (val * nxtval <= FZERO)
          goto experr;
        d = dur * esr;
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
    if (val == FZERO)
      sprintf(errmsg,Str(X_952,"ival%d is zero"), n);
    else if (nxtval == FZERO)
      sprintf(errmsg,Str(X_952,"ival%d is zero"), n+1);
    else sprintf(errmsg,Str(X_953,"ival%d sign conflict"), n+1);
    return NOTOK;
}


/***************************************/
int expseg2(EXPSEG2 *p)                  /*gab-A1 (G.Maldonado) */
{
    XSEG        *segp;
    int         nsmps = ksmps;
    MYFLT       val, *rs;
    segp = p->cursegp;
    val = segp->val;
    rs = p->rslt;
    do {
      while (--segp->cnt < 0)   {
        p->cursegp = ++segp;
        val = segp->val;
      }
      *rs++ = val;
      val *=  segp->mlt;
    } while (--nsmps);
    segp->val = val;
    return OK;
}

/* **** XDSR is just a construction and use of expseg */

int xdsrset(EXXPSEG *p)
{
    XSEG        *segp;
    int nsegs;
    MYFLT       **argp = p->argums;
    MYFLT       len = curip->p3;
    MYFLT   delay = *argp[4], attack = *argp[0], decay = *argp[1];
    MYFLT   sus, dur;
    MYFLT   release = *argp[3];

    if (len<FZERO) len = FL(100000.0); /* MIDI case set long */
    len -= release;         /* len is time remaining */
    if (len<FZERO) {         /* Odd case of release time greater than dur */
      release = curip->p3; len = FZERO;
    }
    nsegs = 5;          /* DXDSR */
    if ((segp = (XSEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(XSEG) < (unsigned int)p->auxch.size) {
      auxalloc((long)nsegs*sizeof(XSEG), &p->auxch);
      segp = (XSEG *) p->auxch.auxp;
    }
    segp[nsegs-1].cnt = MAXPOS; /* set endcount for safety */
    if (**argp <= FZERO)  return OK;            /* if idur1 <= 0, skip init  */
    p->cursegp = segp;          /* else setup null seg0 */
    p->segsrem = nsegs;
    delay += FL(0.001);
    if (delay > len) delay = len; len -= delay;
/*      printf("    len=%f : delay=%f\n",len, delay); */
    attack -= FL(0.001);
    if (attack > len) attack = len; len -= attack;
/*      printf("    len=%f : attack=%f\n",len, attack); */
    if (decay > len) decay = len; len -= decay;
/*      printf("    len=%f : decay=%f\n",len, decay); */
    sus = len;
/*      printf("    len=%f : sustain=%f\n",len, sus); */
    segp[0].val = FL(0.001);   /* Like zero start, but exponential */
    segp[0].mlt = FL(1.0);
    segp[0].cnt = (long) (delay*ekr + FL(0.5));
/*          printf("xseg[0] = %f, %f, %ld\n", */
/*                 segp[0].val, segp[0].mlt, segp[0].cnt); */
    dur = attack*ekr;
    segp[1].val = FL(0.001);
    segp[1].mlt = (MYFLT) pow(1000.0, 1.0/(double)dur);
    segp[1].cnt = (long) (dur + FL(0.5));
/*          printf("xseg[1] = %f, %f, %ld\n", */
/*                 segp[1].val, segp[1].mlt, segp[1].cnt); */
    dur = decay*ekr;
    segp[2].val = FL(1.0);
    segp[2].mlt = (MYFLT) pow((double)*argp[2], 1.0/(double)dur);
    segp[2].cnt = (long) (dur + FL(0.5));
/*          printf("xseg[2] = %f, %f, %ld\n", */
/*                 segp[2].val, segp[2].mlt, segp[2].cnt); */
    segp[3].val = *argp[2];
    segp[3].mlt = FL(1.0);
    segp[3].cnt = (long) (sus*ekr + FL(0.5));
/*          printf("xseg[3] = %f, %f, %ld\n", */
/*                 segp[3].val, segp[3].mlt, segp[3].cnt); */
    dur = release*ekr;
    segp[4].val = *argp[2];
    segp[4].mlt = (MYFLT) pow(0.001/(double)*argp[2], 1.0/(double)dur);
    segp[4].cnt = MAXPOS; /*(long) (dur + FL(0.5)); */
/*          printf("xseg[4] = %f, %f, %ld\n", */
/*                 segp[4].val, segp[4].mlt, segp[4].cnt); */
    return OK;
}

/* end of XDSR */

int kxpseg(EXXPSEG *p)
{
    XSEG        *segp;

    segp = p->cursegp;
    if (p->auxch.auxp==NULL) { /* RWD fix */
      return perferror(Str(X_750,"expseg (krate): not initialised"));
    }
    while (--segp->cnt < 0)
      p->cursegp = ++segp;
    *p->rslt = segp->val;
    segp->val *= segp->mlt;
    return OK;
}

int expseg(EXXPSEG *p)
{
    XSEG        *segp;
    int nsmps = ksmps;
    MYFLT       li, val, *rs;
    MYFLT       nxtval;

    segp = p->cursegp;
    if (p->auxch.auxp==NULL) { /* RWD fix */
      return perferror(Str(X_749,"expseg (arate): not initialised"));
    }
    while (--segp->cnt < 0)
      p->cursegp = ++segp;
    val = segp->val;
    nxtval = val * segp->mlt;
    li = (nxtval - val) / ensmps;
    rs = p->rslt;
    do {
      *rs++ = val;
      val += li;
    } while (--nsmps);
    segp->val = nxtval;
    return OK;
}

int xsgrset(EXPSEG *p)
{
    u_short relestim;
    SEG     *segp;
    int     nsegs, n = 0;
    MYFLT   **argp, prvpt;

    p->xtra = -1;
    nsegs = p->INOCOUNT >> 1;               /* count segs & alloc if nec */
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        (unsigned int)nsegs*sizeof(SEG) > (unsigned int)p->auxch.size) {
      auxalloc((long)nsegs*sizeof(SEG), &p->auxch);
      p->cursegp = segp = (SEG *) p->auxch.auxp;
    }
    argp = p->argums;
    prvpt = **argp++;
    if (**argp < FZERO)  return OK; /* if idur1 < 0, skip init      */
    p->curval = prvpt;
    p->curcnt = 0;                  /* else setup null seg0         */
    p->cursegp = segp - 1;
    p->segsrem = nsegs + 1;
    do {                            /* init & chk each real seg ..  */
      MYFLT dur = **argp++;
      segp->nxtpt = **argp++;
      if ((segp->cnt = (long)(dur * ekr + FL(0.5))) <= 0)
        segp->cnt = 0;
      else if (segp->nxtpt * prvpt <= FZERO)
        goto experr;
      prvpt = segp->nxtpt;
      segp++;
    } while (--nsegs);
    relestim = (u_short)(p->cursegp + p->segsrem - 1)->cnt;
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = relestim;
    return OK;

 experr:
    n = segp - p->cursegp + 2;
    if (prvpt == FZERO)
      sprintf(errmsg,Str(X_952,"ival%d is zero"), n);
    else if (segp->nxtpt == FZERO)
      sprintf(errmsg,Str(X_952,"ival%d is zero"), n+1);
    else sprintf(errmsg,Str(X_953,"ival%d sign conflict"), n+1);
    return initerror(errmsg);
}

/* **** MXDSR is just a construction and use of expseg */

int mxdsrset(EXPSEG *p)
{
    u_short relestim;
    SEG         *segp;
    int         nsegs;
    MYFLT       **argp = p->argums;
    MYFLT       delay = *argp[4], attack = *argp[0], decay = *argp[1];
    MYFLT       rel = *argp[3];

    nsegs = 4;          /* DXDSR */
    if ((segp = (SEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(SEG) < (unsigned int)p->auxch.size) {
      auxalloc((long)nsegs*sizeof(SEG), &p->auxch);
      segp = (SEG *) p->auxch.auxp;
    }
    if (**argp <= FZERO)  return OK;            /* if idur1 <= 0, skip init  */
    p->cursegp = segp-1;          /* else setup null seg0 */
    p->segsrem = nsegs+1;
    p->curval = FL(0.001);
    p->curcnt = 0;                  /* else setup null seg0         */
    delay += FL(0.001);
    attack -= FL(0.001);
    segp[0].nxtpt = FL(0.001);
    segp[0].cnt = (long) (delay*ekr + FL(0.5));
    segp[1].nxtpt = FL(1.0);
    segp[1].cnt = (long) (attack*ekr + FL(0.5));
    segp[2].nxtpt = *argp[2];
    segp[2].cnt = (long) (decay*ekr + FL(0.5));
    segp[3].nxtpt = FL(0.001);
    segp[3].cnt = (long) (rel*ekr + FL(0.5));
    relestim = (u_short)(p->cursegp + p->segsrem - 1)->cnt;
    p->xtra = (long)(*argp[5] * ekr + FL(0.5));      /* Release time?? */
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = relestim;
    return OK;
}

/* end of MXDSR */

int kxpsegr(EXPSEG *p)
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
          p->curmlt = FONE;
        else p->curmlt = (MYFLT) pow(segp->nxtpt/p->curval, 1.0/segp->cnt);
      }
      p->curval *= p->curmlt;           /* advance the cur val  */
    }
    return OK;
}

int expsegr(EXPSEG *p)
{
    MYFLT  val, amlt, *rs = p->rslt;
    int    nsmps = ksmps;

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
          p->curmlt = p->curamlt = FONE;
          p->curval = val;
          goto putk;
        }
        else {
          p->curmlt = (MYFLT) pow((double)(segp->nxtpt/val),
                                  1.0/(double)segp->cnt);
          p->curamlt = (MYFLT) pow(p->curmlt,  1.0/(double)ensmps);
        }
      }
      p->curval = val * p->curmlt;        /* advance the cur val  */
      if ((amlt = p->curamlt) == FONE)
        goto putk;
      do {
        *rs++ = val;
        val *= amlt;
      } while (--nsmps);
    } else {
    putk:
      do *rs++ = val;
      while (--nsmps);
    }
    return OK;
}

int lnnset(LINEN *p)
{
    MYFLT a,b,dur;

    if ((dur = *p->idur) > FZERO) {
      p->cnt1 = (long)(*p->iris * ekr + FL(0.5));
      if (p->cnt1 > 0L) {
        p->inc1 = FONE / (MYFLT) p->cnt1;
        p->val = FZERO;
      }
      else p->inc1 = p->val = FONE;
      a = dur * ekr + FL(0.5);
      b = *p->idec * ekr + FL(0.5);
      if ((long) b > 0L) {
        p->cnt2 = (long) (a - b);
        p->inc2 = FONE /  b;
      }
      else {
        p->inc2 = FONE;
        p->cnt2 = (long) a;
      }
      p->lin1 = FZERO;
      p->lin2 = FONE;
    }
    return OK;
}

int klinen(LINEN *p)
{
    MYFLT fact = FONE;

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

int linen(LINEN *p)
{
    int flag=0, nsmps=ksmps;
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
      li = (nxtval - val)/ensmps;
      if (p->XINCODE) {
        do {
          *rs++ = *sg++ * val;
          val += li;
        }
        while(--nsmps);
      }
      else {
        do {
          *rs++ = *sg * val;
          val += li;
        }
        while(--nsmps);
      }
    }
    else {
      if (p->XINCODE) {
        do *rs++ = *sg++;
        while(--nsmps);
      }
      else {
        do *rs++ = *sg;
        while(--nsmps);
      }
    }
    return OK;
}

int lnrset(LINENR *p)
{
    p->cnt1 = (long)(*p->iris * ekr + FL(0.5));
    if (p->cnt1 > 0L) {
      p->inc1 = FONE / (MYFLT) p->cnt1;
      p->val = FZERO;
    }
    else p->inc1 = p->val = FONE;
    if (*p->idec > FZERO) {
      u_short relestim = (u_short)(*p->idec * ekr + FL(0.5));
      if (relestim > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = relestim;
      if (*p->iatdec <= FZERO) {
        return initerror(Str(X_1075,"non-positive iatdec"));
      }
      else p->mlt2 = (MYFLT) pow((double)*p->iatdec, ((double)onedkr/ *p->idec));
    }
    else p->mlt2 = FONE;
    p->lin1 = FZERO;
    p->val2 = FONE;
    return OK;
}

int klinenr(LINENR *p)
{
    MYFLT fact = FONE;

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

int linenr(LINENR *p)
{
    int flag=0, nsmps=ksmps;
    MYFLT *rs,*sg,li,val,nxtval=FONE;

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
      li = (nxtval - val)/ensmps;
      if (p->XINCODE) {
        do {
          *rs++ = *sg++ * val;
          val += li;
        }
        while (--nsmps);
      }
      else {
        do {
          *rs++ = *sg * val;
          val += li;
        }
        while(--nsmps);
      }
    }
    else {
      if (p->XINCODE) {
        do *rs++ = *sg++;
        while (--nsmps);
      }
      else {
        do *rs++ = *sg;
        while (--nsmps);
      }
    }
    return OK;
}

int evxset(ENVLPX *p)
{
    FUNC        *ftp;
    MYFLT       ixmod, iatss, idur, prod, diff, asym, nk, denom, irise;
    long        cnt1;

    if ((ftp = ftfind(p->ifn)) == NULL)
      return NOTOK;
    p->ftp = ftp;
    if ((idur = *p->idur) > FZERO) {
      if ((iatss = (MYFLT)fabs(*p->iatss)) == FZERO) {
        return initerror("iatss = 0");
      }
      if (iatss != FONE && (ixmod = *p->ixmod) != FZERO) {
        if (fabs(ixmod) > .95) {
          return initerror(Str(X_954,"ixmod out of range."));
        }
        ixmod = -(MYFLT)sin(sin(ixmod));
        prod = ixmod * iatss;
        diff = ixmod - iatss;
        denom = diff + prod + FL(1.0);
        if (denom == FZERO)
          asym = FHUND;
        else {
          asym = 2 * prod / denom;
          if (fabs(asym) > FHUND)
            asym = FHUND;
        }
        iatss = (iatss - asym) / (FL(1.0) - asym);
        asym = asym* *(ftp->ftable + ftp->flen); /* +1 */
      }
      else asym = FZERO;
      if ((irise = *p->irise) > FZERO) {
        p->phs = 0;
        p->ki = (long) (kicvt / irise);
        p->val = *ftp->ftable;
      }
      else {
        p->phs = -1;
        p->val = *(ftp->ftable + ftp->flen)-asym;
        irise = FZERO;  /* in case irise < 0 */
      }
      if (!(*(ftp->ftable + ftp->flen))) {
        return initerror(Str(X_1168,"rise func ends with zero"));
      }
      cnt1 = (long) ((idur - irise - *p->idec) * ekr + FL(0.5));
      if (cnt1 < 0L) {
        cnt1 = 0L;
        nk = ekr;
      }
      else {
        if (*p->iatss < FZERO || cnt1 <= 4L)
          nk = ekr;
        else nk = (MYFLT) cnt1;
      }
      p->mlt1 = (MYFLT) pow((double)iatss, (1.0/nk));
      if (*p->idec > FZERO) {
        if (*p->iatdec <= FZERO) {
          return initerror(Str(X_1075,"non-positive iatdec"));
        }
        p->mlt2 = (MYFLT) pow((double)*p->iatdec,
                              ((double)onedkr / *p->idec));
      }
      p->cnt1 = cnt1;
      p->asym = asym;
    }
    return OK;
}

int knvlpx(ENVLPX *p)
{
    FUNC        *ftp;
    long        phs;
    MYFLT       fact, v1, fract, *ftab;

    ftp = p->ftp;
    if (ftp==NULL) {        /* RWD fix */
      return perferror(Str(X_720,"envlpx(krate): not initialised"));
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
          return perferror(Str(X_719,"envlpx rise func ends with zero"));
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

int envlpx(ENVLPX *p)
{
    FUNC        *ftp;
    long        phs;
    int nsmps = ksmps;
    MYFLT       *xamp, *rslt, val, nxtval, li, v1, fract, *ftab;

    xamp = p->xamp;
    rslt = p->rslt;
    val = p->val;
    if ((phs = p->phs) >= 0L) {
      ftp = p->ftp;
      if (ftp==NULL) { /* RWD fix */
        return perferror(Str(X_720,"envlpx(krate): not initialised"));
      }
      fract = (MYFLT) PFRAC(phs);
      ftab = ftp->ftable + (phs >> ftp->lobits);
      v1 = *ftab++;
      nxtval = (v1 + (*ftab - v1) * fract);
      phs += p->ki;
      if (phs >= MAXLEN) {  /* check that 2**N+1th pnt is good */
        nxtval = *(ftp->ftable + ftp->flen );
        if (!nxtval) {
          return perferror(Str(X_719,"envlpx rise func ends with zero"));
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
    li = (nxtval - val)/ensmps; /* linear interpolation factor */
    if (p->XINCODE) {           /* for audio rate amplitude: */
      do {
        *rslt++ = *xamp++ * val;
        val += li;
      }
      while (--nsmps);
    }
    else {
      do {
        *rslt++ = *xamp * val;
        val += li;
      }
      while (--nsmps);
    }
    return OK;
}

int evrset(ENVLPR *p)
{
    FUNC        *ftp;
    MYFLT  ixmod, iatss, prod, diff, asym, denom, irise;

    if ((ftp = ftfind(p->ifn)) == NULL)
      return NOTOK;
    p->ftp = ftp;
    if ((iatss = (MYFLT)fabs((double)*p->iatss)) == FZERO) {
      return initerror("iatss = 0");
    }
    if (iatss != FONE && (ixmod = *p->ixmod) != FZERO) {
      if (fabs(ixmod) > .95) {
        return initerror(Str(X_954,"ixmod out of range."));
      }
      ixmod = -(MYFLT)sin(sin((double)ixmod));
      prod = ixmod * iatss;
      diff = ixmod - iatss;
      denom = diff + prod + FL(1.0);
      if (denom == FZERO)
        asym = FHUND;
      else {
        asym = 2 * prod / denom;
        if (fabs(asym) > FHUND)
          asym = FHUND;
      }
      iatss = (iatss - asym) / (FL(1.0) - asym);
      asym = asym * *(ftp->ftable + ftp->flen); /* +1 */
    }
    else asym = FZERO;
    if ((irise = *p->irise) > FZERO) {
      p->phs = 0L;
      p->ki = (long) (kicvt / irise);
      p->val = *ftp->ftable;
    }
    else {
      p->phs = -1L;
      p->val = *(ftp->ftable + ftp->flen)-asym;
      irise = FZERO;          /* in case irise < 0 */
    }
    if (!(*(ftp->ftable + ftp->flen))) {
      return initerror(Str(X_1168,"rise func ends with zero"));
    }
    p->mlt1 = (MYFLT)pow((double)iatss, (double)onedkr);
    if (*p->idec > FZERO) {
      long rlscnt = (long)(*p->idec * ekr + .5);
      if ((p->rindep = (long)*p->irind))
        p->rlscnt = rlscnt;
      else if (rlscnt > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = (int)rlscnt;
      if ((p->atdec = *p->iatdec) <= FZERO ) {
        return initerror(Str(X_1075,"non-positive iatdec"));
      }
    }
    p->asym = asym;
    p->rlsing = 0;
    return OK;
}

int knvlpxr(ENVLPR *p)
{
        MYFLT  fact;
        long   rlscnt;

        if (!p->rlsing) {                       /* if not in reles seg  */
          if (p->h.insdshead->relesing) {
            p->rlsing = 1;                  /*   if new flag, set mlt2 */
            rlscnt = (p->rindep) ? p->rlscnt : p->h.insdshead->xtratim;
            if (rlscnt)
              p->mlt2 = (MYFLT)pow((double)p->atdec, 1.0/(double)rlscnt);
            else p->mlt2 = FL(1.0);
          }
          if (p->phs >= 0) {                  /* do fn rise for seg 1 */
            FUNC *ftp = p->ftp;
            long phs = p->phs;
            MYFLT fract = PFRAC(phs);
            MYFLT *ftab = ftp->ftable + (phs >> ftp->lobits);
            MYFLT v1 = *ftab++;
            fact = (v1 + (*ftab - v1) * fract);
            phs += p->ki;
            if (phs < MAXLEN || p->rlsing)  /* if more fn or beg rls */
              p->val = fact;              /*      save cur val     */
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
        else fact = p->val *= p->mlt2;          /* else do seg 3 decay */
        *p->rslt = *p->xamp * fact;
        return OK;
}

int envlpxr(ENVLPR *p)
{
#if defined(SYMANTEC) && !defined(THINK_C)
#pragma options(!global_optimizer)
#endif
        int    nsmps = ksmps;
        long   rlscnt;
        MYFLT  *xamp, *rslt, val, nxtval, li;

        xamp = p->xamp;
        rslt = p->rslt;
        val = p->val;
        if (!p->rlsing) {                       /* if not in reles seg  */
          if (p->h.insdshead->relesing) {
            p->rlsing = 1;                      /*   if new flag, set mlt2 */
            rlscnt = (p->rindep) ? p->rlscnt : p->h.insdshead->xtratim;
            if (rlscnt)
              p->mlt2 = (MYFLT)pow((double)p->atdec, 1.0/(double)rlscnt);
            else p->mlt2 = FL(1.0);
          }
          if (p->phs >= 0) {                    /* do fn rise for seg 1 */
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
        else p->val = nxtval = val * p->mlt2;   /* else do seg 3 decay  */
        li = (nxtval - val) / ensmps;           /* all segs use interp  */
        if (p->XINCODE) {
          do {
            *rslt++ = *xamp++ * val;
            val += li;
          } while(--nsmps);
        }
        else {
          do {
            *rslt++ = *xamp * val;
            val += li;
          } while(--nsmps);
        }
        return OK;
}

