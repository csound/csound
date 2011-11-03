/*
    harmon.c:

    Copyright (C) 1996 Barry Vercoe

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

#include "csoundCore.h"
#include "interlocks.h"

#include <math.h>

typedef struct {
        MYFLT   *srcp;
        int32    cntr;
} PULDAT;

typedef struct {
        MYFLT   *kfrq;
        int32    phase, phsinc;
} VOCDAT;

#define PULMAX  8
#define VOCMAX  4

typedef struct {
        MYFLT   *ar, *asig, *koct, *icpsmode, *ilowest, *ipolarity;
        int16   nbufsmps, n2bufsmps, period, cpsmode, polarity, poslead;
        MYFLT   prvoct, minoct, sicvt;
        MYFLT   *bufp, *midp, *inp1, *inp2;
        MYFLT   *pulsbuf[4], *sigmoid, *curpuls;
        MYFLT   vocamp, vocinc, ampinc;
        PULDAT  puldat[PULMAX], *endp, *limp;
        VOCDAT  vocdat[VOCMAX], *vlim;
        int16   pbufcnt, maxprd, pulslen, switching;
        AUXCH   auxch;
        int     hmrngflg;
} HARM234;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *koct, *kfrq1, *kfrq2,
                *icpsmode, *ilowest, *ipolarity;
        HARM234 hrmdat;
} HARMON2;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *koct, *kfrq1, *kfrq2, *kfrq3,
                *icpsmode, *ilowest, *ipolarity;
        HARM234 hrmdat;
} HARMON3;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *koct, *kfrq1, *kfrq2, *kfrq3, *kfrq4,
                *icpsmode, *ilowest, *ipolarity;
        HARM234 hrmdat;
} HARMON4;

#define PBUFS   4
#define PBMSK   0x3
#define SLEN    256
#define LCNT    75

static int hm234set(CSOUND *csound, HARM234 *p)
{
    MYFLT minoct = *p->ilowest;
    p->hmrngflg = 0;
    if (p->auxch.auxp == NULL || minoct < p->minoct) {
      MYFLT minfrq = POWER(FL(2.0), minoct) * ONEPT;
      int16 nbufs = (int16)(csound->ekr * 3 / minfrq) + 1;/* recalc max pulse prd */
      int16 nbufsmps = nbufs * csound->ksmps;
      int16 maxprd = (int16)(csound->esr * 2 / minfrq);   /* incl sigmoid ends */
      int16 cnt;
      int32  totalsiz = nbufsmps * 2 + maxprd * 4 + (SLEN+1);
      MYFLT *pulsbuf, *sigp;                            /*  & realloc buffers */

      csound->AuxAlloc(csound, totalsiz * sizeof(MYFLT), &p->auxch);
      p->bufp = (MYFLT *) p->auxch.auxp;
      p->midp = p->bufp + nbufsmps;                     /* each >= maxprd * 3 */
      pulsbuf = p->midp + nbufsmps;
      p->pulsbuf[0] = pulsbuf;  pulsbuf += maxprd;
      p->pulsbuf[1] = pulsbuf;  pulsbuf += maxprd;
      p->pulsbuf[2] = pulsbuf;  pulsbuf += maxprd;
      p->pulsbuf[3] = pulsbuf;  pulsbuf += maxprd;      /* cnt must = PBUFS     */
      p->sigmoid = sigp = pulsbuf;
      for (cnt = 0; cnt < SLEN+1; cnt++)                /* make sigmoid inplace */
        *sigp++ = (FL(1.0) - COS(PI_F * cnt / SLEN)) * FL(0.5);
      p->maxprd = maxprd;
      p->nbufsmps = nbufsmps;
      p->n2bufsmps = nbufsmps * 2;
      p->minoct = minoct;
    }
    p->sicvt = FL(65536.0) * csound->onedsr;
    p->cpsmode = ((*p->icpsmode != FL(0.0)));
    p->polarity = (int16)*p->ipolarity;
    p->poslead = 0;
    p->inp1 = p->bufp;
    p->inp2 = p->midp;
    p->endp = p->puldat;                        /* nothing in PULDAT array */
    p->limp = p->puldat + PULMAX;
    p->prvoct = FL(0.0);
    p->period = 0;
    p->curpuls = NULL;
    p->pbufcnt = 0;
    p->vocamp = FL(0.0);                        /* begin unvoiced */
    p->ampinc = FL(10.0) * csound->onedsr;      /* .1 sec lin ramp for uv to v */
    p->switching = 0;
    return OK;
}

static int harmon234(CSOUND *csound, HARM234 *p)
{
    MYFLT       *srcp, *outp, *dirp;
    MYFLT       *inp1, *inp2;
    MYFLT       koct, vocamp, diramp;
    PULDAT      *endp;
    VOCDAT      *vdp;
    int16       n, nsmps, oflow = 0;

    if ((koct = *p->koct) != p->prvoct) {               /* if new pitch estimate */
      if (koct >= p->minoct) {                          /*   above requested low */
        MYFLT cps = POWER(FL(2.0), koct) * ONEPT;     /*   recalc pulse period */
        p->period = (int16) (csound->esr / cps);
        if (!p->cpsmode)
          p->sicvt = cps * FL(65536.0) * csound->onedsr; /* k64dsr;*/
      }
      p->prvoct = koct;
    }
    inp1 = p->inp1;
    inp2 = p->inp2;
    //memcpy(p->inp1, p->asig, sizeof(MYFLT)*nsmps);
    //memcpy(p->inp2, p->asig, sizeof(MYFLT)*nsmps);
    for (srcp = p->asig, nsmps = csound->ksmps; nsmps--; )
      *inp1++ = *inp2++ = *srcp++;              /* dbl store the wavform */

    if (koct >= p->minoct) {                    /* PERIODIC: find the pulse */
      MYFLT     val0, *buf0, *p0, *plim, *x;
      int16     period, triprd, xdist;

      period = p->period;                       /* set srch range of 2 periods */
      triprd = period * 3;
      p0 = inp2 - triprd;                       /* btwn 3 prds back & 1 prd back */
      plim = inp2 - period;
      buf0 = p->bufp;
      if (UNLIKELY(p0 < buf0))
        p0 = buf0;

      x = p0;                                   /* locate first zero crossing   */
      if ((val0 = *x++) == FL(0.0))
        while (*x == FL(0.0) && ++x < plim);    /* if no signal in this range   */
      else if (val0 > FL(0.0))
        while (*x > 0. && ++x < plim);          /* or unipolar with no z-crossing */
      else while (*x < FL(0.0) && ++x < plim);
      if (x >= plim) goto nonprd;               /*      then non-periodic       */

      if (p->polarity > 0) {
        MYFLT pospk = FL(0.0);                  /* POSITIVE polarity:   */
        MYFLT *posp = NULL;
        for ( ; x < plim; x++) {                /*      find ensuing max val */
          MYFLT val = *x;
          if (val > pospk) { pospk = val; posp = x; }
        }
        if (posp == NULL)
          goto nonprd;
        for (x = posp;
             x >= buf0 && *x > FL(0.0); x--);   /* & its preceding z-crossing */
        xdist = posp - x;
      } else if (p->polarity < 0) {
        MYFLT negpk = FL(0.0);                  /* NEGATIVE polarity:   */
        MYFLT *negp = NULL;
        for ( ; x < plim; x++) {                /* find ensuing min val */
          MYFLT val = *x;
          if (val < negpk) { negpk = val; negp = x; }
        }
        if (negp == NULL)
          goto nonprd;
        for (x = negp;
             x >= buf0 && *x < FL(0.0); x--); /* & its preceding z-crossing */
        xdist = negp - x;
      }
      else {
        MYFLT pospk, negpk, *posp, *negp;               /* NOT SURE:    */
        MYFLT *poscross, *negcross;
        int16 posdist, negdist;
        pospk = negpk = FL(0.0);
        posp = negp = NULL;
        for ( ; x < plim; x++) {                /* find ensuing max & min vals */
          MYFLT val = *x;
          if (val > FL(0.0)) {
            if (val > pospk) { pospk = val; posp = x; }
          } else
            if (val < negpk) { negpk = val; negp = x; }
        }
        if (posp == NULL || negp == NULL)
          goto nonprd;
        for (x = posp; x >= buf0 &&
               *x > FL(0.0); x--); /* & their preceding z-crossings */
        posdist = posp - x;
        poscross = x;
        for (x = negp; x >= buf0 && *x < FL(0.0); x--);
        negdist = negp - x;
        negcross = x;

        if (pospk / posdist > -negpk / negdist) {
          /* find z-cross with grtst slope to peak */
          if (UNLIKELY(p->poslead < LCNT)) {   /*      and consistent polarity */
            if (p->poslead == 1)
              csound->Warning(csound, Str("harm signal has positive lead\n"));
            p->poslead += 1;
          }
        }
        else {
          if (UNLIKELY(p->poslead > -LCNT)) {
            if (p->poslead == -1)
              csound->Warning(csound, Str("harm signal has negative lead\n"));
            p->poslead -= 1;
          }
        }
        if (p->poslead >= 0) {                  /* use this as pulse beginning  */
          x = poscross;
          xdist = posdist;
        }
        else {
          x = negcross;
          xdist = negdist;
        }
      }

      if (x != p->curpuls) {                    /* if pulse positn is new       */
        int16 nn, pulslen, sigdist, ndirect;
        MYFLT *bufp, signdx, siginc;
        MYFLT *z, zval;

        p->curpuls = x;                         /*      record this new positn  */
        z = x + period;                         /*  and from estimated end      */
        if ((zval = *z) != FL(0.0)) {
          int16 n, nlim = inp2 - z;
          for (n = 1; n < nlim; n++) {
            if (zval * *(z+n) <= FL(0.0)) {     /*       find nearest zcrossing */
              z += n;
              break;
            } else if (zval * *(z-n) <= FL(0.0)) {
              z -= n;
              break;
            }                                   /* (true period is now z - x) */
          }
        }
        x -= xdist;                             /* now extend for sig ris-dec   */
        z += xdist;
        if (x < buf0)
          x = buf0;                             /*      truncated if necessary  */
        else if (z > inp2)
          z = inp2;                             /*      by input limits         */
        pulslen = z - x;
        if (pulslen > p->maxprd)
          pulslen = p->maxprd;                  /*      & storage limits        */
        sigdist = xdist * 2;
        ndirect = pulslen - sigdist*2;
        if (ndirect < 0) goto nostor;

        p->pbufcnt++;                           /* select a new puls buffr      */
        p->pbufcnt &= PBMSK;
        bufp = p->pulsbuf[p->pbufcnt];
        signdx = FL(0.0);                       /*      & store extended pulse  */
        siginc = (MYFLT)SLEN / sigdist;
        for (nn = sigdist; nn--; signdx += siginc) {
          MYFLT *sigp = p->sigmoid + (int)signdx;
          *bufp++ = *x++ * *sigp;               /*      w. sigmoid-envlpd ends  */
        }
        //memcpy(bufp, x, sizeof(MYFLT)*ndirect);
        while (ndirect--)
          *bufp++ = *x++;
        signdx = (MYFLT)SLEN - siginc;
        for (nn = sigdist; nn--; signdx -= siginc) {
          MYFLT *sigp = p->sigmoid + (int)signdx;
          *bufp++ = *x++ * *sigp;
        }
        p->pulslen = pulslen;
      }
    nostor:
      if (p->vocamp < FL(1.0)) {                /* if onset             */
        p->vocinc = p->ampinc;                  /*   set pos voice ramp */
        p->switching = 1;
      }
    } else {                                    /* NON-PERIODIC: */
    nonprd:
      if (p->vocamp > FL(0.0)) {                /* if onset             */
        p->vocinc = -p->ampinc;                 /*   set neg voice ramp */
        p->switching = 1;
      }
      p->curpuls = NULL;                        /* start no new pulses */
    }
    /* HARMONIZER */
    for (vdp=p->vocdat; vdp<p->vlim; vdp++)     /* get new frequencies  */
      vdp->phsinc = (int32)(*vdp->kfrq * p->sicvt);
    outp = p->ar;
    nsmps = csound->ksmps;
    vocamp = p->vocamp;
    diramp = FL(1.0) - vocamp;
    dirp = p->asig;
    endp = p->endp;
    do {                                        /* insert pulses into output: */
      MYFLT sum = FL(0.0);
      PULDAT *pdp = p->puldat;
      while (pdp < endp) {
      addin:
        sum += *pdp->srcp++;                    /* get any ongoing pulsdata */
        if (--pdp->cntr <= 0) {                 /* & phase out if now done  */
          if (--endp == pdp)
            break;
          pdp->srcp = endp->srcp;
          pdp->cntr = endp->cntr;
          goto addin;
        }
        pdp++;
      }                                         /* if time to start a new one */
      for (vdp=p->vocdat; vdp<p->vlim; vdp++)
        if (vdp->phsinc && (vdp->phase += vdp->phsinc) & (~0xFFFF)) {
          vdp->phase &= 0x0000FFFFL;
          if (p->curpuls != NULL) {             /*      & pulses are current    */
            if (endp < p->limp) {               /*      set one up              */
              endp->srcp = p->pulsbuf[p->pbufcnt];
              endp->cntr = p->pulslen;          /*  w. extended len     */
              endp++;
            } else oflow = 1;
          }
        }
      if (p->switching) {                       /* if v/uv switching    */
        vocamp += p->vocinc;                    /*   do linear ramp     */
        if (vocamp <= FL(0.0)) {
          vocamp = FL(0.0);
          p->switching = 0;
        } else if (vocamp >= FL(1.0)) {
          vocamp = FL(1.0);
          p->switching = 0;
        }
        diramp = FL(1.0) - vocamp;
      }
      *outp++ = sum * vocamp + *dirp++ * diramp;        /* output combined */
    } while (--nsmps);
    p->endp = endp;
    p->vocamp = vocamp;

    if (UNLIKELY(oflow && ++p->hmrngflg > 10)) {
      csound->Warning(csound, Str("harmon234: out of range\n"));
      p->hmrngflg = 0;
    }
    if (inp1 >= p->midp) {                       /* if end of pq bufs */
      int32 bsmps = p->nbufsmps;
      p->inp1 = p->bufp;                         /*   reset all ptrs  */
      p->inp2 = p->midp;
      if (p->curpuls != NULL)
        p->curpuls -= bsmps;
    }
    else {
      p->inp1 = inp1;
      p->inp2 = inp2;
    }
    return OK;
}

int harm2set(CSOUND *csound, HARMON2 *p)
{
    HARM234 *q = &p->hrmdat;
    VOCDAT *vdp = q->vocdat;
    q->ar = p->ar;
    q->asig = p->asig;
    q->koct = p->koct;
    vdp->kfrq = p->kfrq1;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq2;       vdp->phase = 0; vdp++;
    q->vlim = vdp;
    q->icpsmode = p->icpsmode;
    q->ilowest = p->ilowest;
    q->ipolarity = p->ipolarity;
    return hm234set(csound, q);
}

int harm3set(CSOUND *csound, HARMON3 *p)
{
    HARM234 *q = &p->hrmdat;
    VOCDAT *vdp = q->vocdat;
    q->ar = p->ar;
    q->asig = p->asig;
    q->koct = p->koct;
    vdp->kfrq = p->kfrq1;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq2;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq3;       vdp->phase = 0; vdp++;
    q->vlim = vdp;
    q->icpsmode = p->icpsmode;
    q->ilowest = p->ilowest;
    q->ipolarity = p->ipolarity;
    return hm234set(csound, q);
}

int harm4set(CSOUND *csound, HARMON4 *p)
{
    HARM234 *q = &p->hrmdat;
    VOCDAT *vdp = q->vocdat;
    q->ar = p->ar;
    q->asig = p->asig;
    q->koct = p->koct;
    vdp->kfrq = p->kfrq1;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq2;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq3;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq4;       vdp->phase = 0; vdp++;
    q->vlim = vdp;
    q->icpsmode = p->icpsmode;
    q->ilowest = p->ilowest;
    q->ipolarity = p->ipolarity;
    return hm234set(csound, q);
}

int harmon2(CSOUND *csound, HARMON2 *p) { return harmon234(csound, &p->hrmdat);}
int harmon3(CSOUND *csound, HARMON3 *p) { return harmon234(csound, &p->hrmdat);}
int harmon4(CSOUND *csound, HARMON4 *p) { return harmon234(csound, &p->hrmdat);}

#define S(x)    sizeof(x)

static OENTRY harmon_localops[] = {
  { "harmon2",S(HARMON2),5, "a",  "akkkiip",  (SUBR)harm2set,NULL, (SUBR)harmon2 },
  { "harmon3",S(HARMON3),5, "a",  "akkkkiip", (SUBR)harm3set,NULL, (SUBR)harmon3 },
  { "harmon4",S(HARMON4),5, "a",  "akkkkkiip",(SUBR)harm4set,NULL, (SUBR)harmon4 },
};

LINKAGE1(harmon_localops)

