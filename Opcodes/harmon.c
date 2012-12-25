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

static int hm234set(CSOUND *csound, HARM234 *q, HARMON2 *p)
{
    MYFLT minoct = *q->ilowest;
    q->hmrngflg = 0;
    if (q->auxch.auxp == NULL || minoct < q->minoct) {
      MYFLT minfrq = POWER(FL(2.0), minoct) * ONEPT;
      int16 nbufs = (int16)(csound->ekr * 3 / minfrq) + 1;/* recalc max pulse prd */
      int16 nbufsmps = nbufs * CS_KSMPS;
      int16 maxprd = (int16)(csound->esr * 2 / minfrq);   /* incl sigmoid ends */
      int16 cnt;
      int32  totalsiz = nbufsmps * 2 + maxprd * 4 + (SLEN+1);
      MYFLT *pulsbuf, *sigp;                            /*  & realloc buffers */

      csound->AuxAlloc(csound, totalsiz * sizeof(MYFLT), &q->auxch);
      q->bufp = (MYFLT *) q->auxch.auxp;
      q->midp = q->bufp + nbufsmps;                     /* each >= maxprd * 3 */
      pulsbuf = q->midp + nbufsmps;
      q->pulsbuf[0] = pulsbuf;  pulsbuf += maxprd;
      q->pulsbuf[1] = pulsbuf;  pulsbuf += maxprd;
      q->pulsbuf[2] = pulsbuf;  pulsbuf += maxprd;
      q->pulsbuf[3] = pulsbuf;  pulsbuf += maxprd;      /* cnt must = PBUFS     */
      q->sigmoid = sigp = pulsbuf;
      for (cnt = 0; cnt < SLEN+1; cnt++)                /* make sigmoid inplace */
        *sigp++ = (FL(1.0) - COS(PI_F * cnt / SLEN)) * FL(0.5);
      q->maxprd = maxprd;
      q->nbufsmps = nbufsmps;
      q->n2bufsmps = nbufsmps * 2;
    }
    q->minoct = minoct;
    q->sicvt = FL(65536.0) * csound->onedsr;
    q->cpsmode = ((*q->icpsmode != FL(0.0)));
    q->polarity = (int16)*q->ipolarity;
    q->poslead = 0;
    q->inp1 = q->bufp;
    q->inp2 = q->midp;
    q->endp = q->puldat;                        /* nothing in PULDAT array */
    q->limp = q->puldat + PULMAX;
    q->prvoct = FL(0.0);
    q->period = 0;
    q->curpuls = NULL;
    q->pbufcnt = 0;
    q->vocamp = FL(0.0);                        /* begin unvoiced */
    q->ampinc = FL(10.0) * csound->onedsr;      /* .1 sec lin ramp for uv to v */
    q->switching = 0;
    return OK;
}

static int harmon234(CSOUND *csound, HARM234 *q, HARMON2 *p)
{
    MYFLT       *outp, *dirp; // *srcp, 
    MYFLT       *inp1, *inp2;
    MYFLT       koct, vocamp, diramp;
    PULDAT      *endp;
    VOCDAT      *vdp;
    int16       nsmps = CS_KSMPS, oflow = 0;
    uint32_t offset = p->h.insdshead->ksmps_offset;

    if ((koct = *q->koct) != q->prvoct) {               /* if new pitch estimate */
      if (koct >= q->minoct) {                          /*   above requested low */
        MYFLT cps = POWER(FL(2.0), koct) * ONEPT;     /*   recalc pulse period */
        q->period = (int16) (csound->esr / cps);
        if (!q->cpsmode)
          q->sicvt = cps * FL(65536.0) * csound->onedsr; /* k64dsr;*/
      }
      q->prvoct = koct;
    }
    inp1 = q->inp1;
    inp2 = q->inp2;
    memset(inp1, '\0', offset*sizeof(MYFLT));
    memset(inp2, '\0', offset*sizeof(MYFLT));
    memcpy(inp1+offset, q->asig+offset, sizeof(MYFLT)*(nsmps-offset));
    memcpy(inp2+offset, q->asig+offset, sizeof(MYFLT)*(nsmps-offset));
    //for (srcp = q->asig, nsmps = CS_KSMPS; nsmps--; )
    //  *inp1++ = *inp2++ = *srcp++;              /* dbl store the wavform */

    if (koct >= q->minoct) {                    /* PERIODIC: find the pulse */
      MYFLT     val0, *buf0, *p0, *plim, *x;
      int16     period, triprd, xdist;

      period = q->period;                       /* set srch range of 2 periods */
      triprd = period * 3;
      p0 = inp2 - triprd;                       /* btwn 3 prds back & 1 prd back */
      plim = inp2 - period;
      buf0 = q->bufp;
      if (UNLIKELY(p0 < buf0))
        p0 = buf0;

      x = p0;                                   /* locate first zero crossing   */
      if ((val0 = *x++) == FL(0.0))
        while (*x == FL(0.0) && ++x < plim);    /* if no signal in this range   */
      else if (val0 > FL(0.0))
        while (*x > 0. && ++x < plim);          /* or unipolar with no z-crossing */
      else while (*x < FL(0.0) && ++x < plim);
      if (x >= plim) goto nonprd;               /*      then non-periodic       */

      if (q->polarity > 0) {
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
      }
      else if (q->polarity < 0) {
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
          if (UNLIKELY(q->poslead < LCNT)) {   /*      and consistent polarity */
            if (q->poslead == 1)
              csound->Warning(csound, Str("harm signal has positive lead\n"));
            q->poslead += 1;
          }
        }
        else {
          if (UNLIKELY(q->poslead > -LCNT)) {
            if (q->poslead == -1)
              csound->Warning(csound, Str("harm signal has negative lead\n"));
            q->poslead -= 1;
          }
        }
        if (q->poslead >= 0) {                  /* use this as pulse beginning  */
          x = poscross;
          xdist = posdist;
        }
        else {
          x = negcross;
          xdist = negdist;
        }
      }

      if (x != q->curpuls) {                    /* if pulse positn is new       */
        int16 nn, pulslen, sigdist, ndirect;
        MYFLT *bufp, signdx, siginc;
        MYFLT *z, zval;

        q->curpuls = x;                         /*      record this new positn  */
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
        if (pulslen > q->maxprd)
          pulslen = q->maxprd;                  /*      & storage limits        */
        sigdist = xdist * 2;
        ndirect = pulslen - sigdist*2;
        if (ndirect < 0) goto nostor;

        q->pbufcnt++;                           /* select a new puls buffr      */
        q->pbufcnt &= PBMSK;
        bufp = q->pulsbuf[q->pbufcnt];
        signdx = FL(0.0);                       /*      & store extended pulse  */
        siginc = (MYFLT)SLEN / sigdist;
        for (nn = sigdist; nn--; signdx += siginc) {
          MYFLT *sigp = q->sigmoid + (int)signdx;
          *bufp++ = *x++ * *sigp;               /*      w. sigmoid-envlpd ends  */
        }
        //memcpy(bufp, x, sizeof(MYFLT)*ndirect);
        while (ndirect--)
          *bufp++ = *x++;
        signdx = (MYFLT)SLEN - siginc;
        for (nn = sigdist; nn--; signdx -= siginc) {
          MYFLT *sigp = q->sigmoid + (int)signdx;
          *bufp++ = *x++ * *sigp;
        }
        q->pulslen = pulslen;
      }
    nostor:
      if (q->vocamp < FL(1.0)) {                /* if onset             */
        q->vocinc = q->ampinc;                  /*   set pos voice ramp */
        q->switching = 1;
      }
    } else {                                    /* NON-PERIODIC: */
    nonprd:
      if (q->vocamp > FL(0.0)) {                /* if onset             */
        q->vocinc = -q->ampinc;                 /*   set neg voice ramp */
        q->switching = 1;
      }
      q->curpuls = NULL;                        /* start no new pulses */
    }
    /* HARMONIZER */
    for (vdp=q->vocdat; vdp<q->vlim; vdp++)     /* get new frequencies  */
      vdp->phsinc = (int32)(*vdp->kfrq * q->sicvt);
    outp = q->ar;
    vocamp = q->vocamp;
    diramp = FL(1.0) - vocamp;
    dirp = q->asig;
    endp = q->endp;
    do {                                        /* insert pulses into output: */
      MYFLT sum = FL(0.0);
      PULDAT *pdp = q->puldat;
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
      for (vdp=q->vocdat; vdp<q->vlim; vdp++)
        if (vdp->phsinc && (vdp->phase += vdp->phsinc) & (~0xFFFF)) {
          vdp->phase &= 0x0000FFFFL;
          if (q->curpuls != NULL) {             /*      & pulses are current    */
            if (endp < q->limp) {               /*      set one up              */
              endp->srcp = q->pulsbuf[q->pbufcnt];
              endp->cntr = q->pulslen;          /*  w. extended len     */
              endp++;
            } else oflow = 1;
          }
        }
      if (q->switching) {                       /* if v/uv switching    */
        vocamp += q->vocinc;                    /*   do linear ramp     */
        if (vocamp <= FL(0.0)) {
          vocamp = FL(0.0);
          q->switching = 0;
        } else if (vocamp >= FL(1.0)) {
          vocamp = FL(1.0);
          q->switching = 0;
        }
        diramp = FL(1.0) - vocamp;
      }
      *outp++ = sum * vocamp + *dirp++ * diramp;        /* output combined */
    } while (--nsmps);
    q->endp = endp;
    q->vocamp = vocamp;

    if (UNLIKELY(oflow && ++q->hmrngflg > 10)) {
      csound->Warning(csound, Str("harmon234: out of range\n"));
      q->hmrngflg = 0;
    }
    if (inp1 >= q->midp) {                       /* if end of pq bufs */
      int32 bsmps = q->nbufsmps;
      q->inp1 = q->bufp;                         /*   reset all ptrs  */
      q->inp2 = q->midp;
      if (q->curpuls != NULL)
        q->curpuls -= bsmps;
    }
    else {
      q->inp1 = inp1;
      q->inp2 = inp2;
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
    return hm234set(csound, q, p);
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
    return hm234set(csound, q, (HARMON2*)p);
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
    return hm234set(csound, q, (HARMON2*)p);
}

int harmon2(CSOUND *csound, HARMON2 *p)
{ return harmon234(csound, &p->hrmdat, p);}
int harmon3(CSOUND *csound, HARMON3 *p)
{ return harmon234(csound, &p->hrmdat, (HARMON2*)p);}
int harmon4(CSOUND *csound, HARMON4 *p)
{ return harmon234(csound, &p->hrmdat, (HARMON2*)p);}

#define S(x)    sizeof(x)

static OENTRY harmon_localops[] = {
  { "harmon2",S(HARMON2),5, "a",  "akkkiip",  (SUBR)harm2set,NULL, (SUBR)harmon2 },
  { "harmon3",S(HARMON3),5, "a",  "akkkkiip", (SUBR)harm3set,NULL, (SUBR)harmon3 },
  { "harmon4",S(HARMON4),5, "a",  "akkkkkiip",(SUBR)harm4set,NULL, (SUBR)harmon4 },
};

LINKAGE_BUILTIN(harmon_localops)

