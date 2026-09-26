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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "interlocks.h"

#include <math.h>

typedef struct {
        cs_float   *srcp;
        int32    cntr;
} PULDAT;

typedef struct {
        cs_float   *kfrq;
        uint32_t phase, phsinc;
} VOCDAT;

#define PULMAX  8
#define VOCMAX  4

typedef struct {
        OPDS    h;
        cs_float   *ar, *asig, *koct, *kfrq1, *kfrq2;
        cs_float   *kfrq3, *kfrq4, *icpsmode, *ilowest, *ipolarity;  //4
  // or cs_float   *kfrq3, *icpsmode, *ilowest, *ipolarity, *dummy;  //3
  // or cs_float   *icpsmode, *ilowest, *ipolarity, *dummy, *dummy1; //2
  //Local
        int32_t nbufsmps, n2bufsmps, period;
        int16   cpsmode, polarity, poslead;
        cs_float   prvoct, minoct, sicvt;
        cs_float   *bufp, *midp, *inp1, *inp2;
        cs_float   *pulsbuf[4], *sigmoid, *curpuls;
        cs_float   vocamp, vocinc, ampinc;
        PULDAT  puldat[PULMAX], *endp, *limp;
        VOCDAT  vocdat[VOCMAX], *vlim;
        int32_t maxprd, pulslen;
        int16   pbufcnt, switching;
        AUXCH   auxch;
        int32_t     hmrngflg;
} HARM234;

/* static void print32_t32_t32_t_data(HARM234* p, int32_t x) */
/* { */
/*     printf("DATA %d\n=======\n", x); */
/*     printf("nbufsmps, n2bufsmps, period, cpsmode, polarity, poslead " */
/*            "= (%d,%d,%d,%d,%d,%d\n", */
/*            p->nbufsmps, p->n2bufsmps, p->period, p->cpsmode, p->polarity, */
/*            p->poslead); */
/*     printf("prvoct, minoct, sicvt = %f, %f, %f\n", */
/*             p->prvoct, p->minoct, p->sicvt); */
/*     //cs_float   *bufp, *midp, *inp1, *inp2; */
/*     //cs_float   *pulsbuf[4], *sigmoid, *curpuls; */
/*     printf("vocamp, vocinc, ampinc = %f, %f, %f\n", */
/*            p->vocamp, p->vocinc, p->ampinc); */
/*     //PULDAT  puldat[PULMAX], *endp, *limp; */
/*     //VOCDAT  vocdat[VOCMAX], *vlim; */
/*     printf("pbufcnt, maxprd, pulslen, switching = %d, %d, %d, %d, %d\n", */
/*            p->pbufcnt, p->maxprd, p->pulslen, p->switching, p->hmrngflg); */
/*     //AUXCH   auxch; */
/*     //int32_t     hmrngflg; */
/*     printf("pulse: %p %p %p %p; %d %d %d %d\n", */
/*            p->puldat[0].srcp, p->puldat[1].srcp, */
/*            p->puldat[2].srcp, p->puldat[3].srcp, */
/*            p->puldat[0].cntr, p->puldat[1].cntr, */
/*            p->puldat[2].cntr, p->puldat[3].cntr); */
/*     printf("voc: (%p %d %d) (%p %d %d) (%p %d %d) (%p %d %d)\n", */
/*            p->vocdat[0].kfrq, p->vocdat[0].phase, p->vocdat[0].phsinc, */
/*            p->vocdat[1].kfrq, p->vocdat[1].phase, p->vocdat[1].phsinc, */
/*            p->vocdat[2].kfrq, p->vocdat[2].phase, p->vocdat[2].phsinc, */
/*            p->vocdat[3].kfrq, p->vocdat[3].phase, p->vocdat[3].phsinc); */
/*     printf("output: %f %f %f %f ...\n", */
/*            p->ar[0], p->ar[1], p->ar[2], p->ar[3]); */
/* } */
#define PBUFS   4
#define PBMSK   0x3
#define SLEN    256
#define LCNT    75

static int32_t hm234set(CSOUND *csound, HARM234 *p)
{
    cs_float minoct = p->minoct;
    p->hmrngflg = 0;
    /*if (p->auxch.auxp == NULL || minoct < p->minoct ) */ {
      cs_float minfrq = POWER(FL(2.0), minoct) * ONEPT;
      cs_double nsamples = (floor(CS_EKR * 3 / minfrq) + 1.0) * CS_KSMPS;
      cs_double prdsamples = floor(CS_ESR * 2 / minfrq); /* incl sigmoid ends */
      cs_double total = nsamples * 2.0 + prdsamples * 4.0 + (SLEN+1);
      if (UNLIKELY(!(nsamples >= CS_KSMPS && prdsamples >= 2.0 &&
                     total <= (INT32_MAX + 0.0) && total <= SIZE_MAX / sizeof(cs_float))))
        return csound->InitError(csound, "%s",
                                 Str("harmon234: lowest pitch is out of range"));
      int32_t nbufsmps = (int32_t)nsamples, maxprd = (int32_t)prdsamples;
      int32_t cnt;
      size_t totalsiz = (size_t)total;
      cs_float *pulsbuf, *sigp;                            /*  & realloc buffers */

      csound->AuxAlloc(csound, totalsiz * sizeof(cs_float), &p->auxch);
      p->bufp = (cs_float *) p->auxch.auxp;
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
    }
    //p->minoct = minoct;
    p->sicvt = FL(65536.0) * CS_ONEDSR;
    //printf("sicvt = %f\n", p->sicvt);
    p->poslead = 0;
    p->inp1 = p->bufp;
    p->inp2 = p->midp;
    p->endp = p->puldat;                        /* nothing in PULDAT array */
    p->limp = p->puldat + PULMAX;
    p->prvoct = FL(0.0);
    p->period = 0;
    p->curpuls = NULL;
    p->pbufcnt = 0;
    p->pulslen = 0;
    p->vocamp = FL(0.0);                        /* begin unvoiced */
    p->ampinc = FL(10.0) * CS_ONEDSR;      /* .1 sec lin ramp for uv to v */
    //printf("ampinc = %f\n", p->ampinc);
    p->switching = 0;
    return OK;
}

static int32_t harmon234(CSOUND *csound, HARM234 *p)
{
    cs_float       *outp, *dirp;
    cs_float       *inp1, *inp2;
    cs_float       koct, vocamp, diramp;
    PULDAT      *endp;
    VOCDAT      *vdp;
    uint32_t    nsmps = CS_KSMPS;
    int32_t     oflow = 0;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    //print_data(p, 1);

    /* A zero octave is also a valid first pitch estimate. */
    if ((koct = *p->koct) != p->prvoct || p->period == 0) {
      if (koct >= p->minoct) {                          /*   above requested low */
        cs_float cps = POWER(FL(2.0), koct) * ONEPT;     /*   recalc pulse period */
        cs_double period = CS_ESR / cps;
        if (UNLIKELY(!(period >= 1.0 && period <= p->maxprd)))
          return csound->PerfError(csound, &p->h, "%s",
                                   Str("harmon234: pitch estimate is out of range"));
        p->period = (int32_t)period;
        if (!p->cpsmode)
          p->sicvt = cps * FL(65536.0) * CS_ONEDSR; /* k64dsr;*/
      }
      p->prvoct = koct;
    }
    inp1 = p->inp1;
    inp2 = p->inp2;
    if (UNLIKELY(offset)) {
      memset(inp1, '\0', offset*sizeof(cs_float));
      memset(inp2, '\0', offset*sizeof(cs_float));
      memset(p->ar, '\0', offset*sizeof(cs_float));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&inp1[nsmps], '\0', early*sizeof(cs_float));
      memset(&inp2[nsmps], '\0', early*sizeof(cs_float));
      memset(&p->ar[nsmps], '\0', early*sizeof(cs_float));
    }
    if (UNLIKELY(nsmps <= offset)) return OK;
    memcpy(&inp1[offset], &p->asig[offset], sizeof(cs_float)*(nsmps-offset));
    memcpy(&inp2[offset], &p->asig[offset], sizeof(cs_float)*(nsmps-offset));
    /* Keep history writes aligned to whole blocks, with silence outside the note. */
    inp1 += CS_KSMPS; inp2 += CS_KSMPS;
    nsmps -= offset;
    //for (srcp = q->asig, nsmps = CS_KSMPS; nsmps--; )
    //  *inp1++ = *inp2++ = *srcp++;              /* dbl store the wavform */

    //print_data(p, 2);
    if (koct >= p->minoct) {                    /* PERIODIC: find the pulse */
      cs_float     val0, *buf0, *p0, *plim, *x;
      int32_t   period, triprd, xdist;

      period = p->period;                       /* set srch range of 2 periods */
      triprd = period * 3;
      buf0 = p->bufp;
      p0 = inp2 - buf0 < triprd ? buf0 : inp2 - triprd;
      plim = inp2 - period;                     /* btwn 3 prds back & 1 prd back */

      x = p0;                                   /* locate first zero crossing   */
      if ((val0 = *x++) == FL(0.0))
        while (x < plim && *x == FL(0.0)) x++;    /* if no signal in this range   */
      else if (val0 > FL(0.0))
        while (x < plim && *x > FL(0.0)) x++;          /* or unipolar with no z-crossing */
      else while (x < plim && *x < FL(0.0)) x++;
      if (x >= plim) goto nonprd;               /*      then non-periodic       */

      if (p->polarity > 0) {
        cs_float pospk = FL(0.0);                  /* POSITIVE polarity:   */
        cs_float *posp = NULL;
        for ( ; x < plim; x++) {                /*      find ensuing max val */
          cs_float val = *x;
          if (val > pospk) { pospk = val; posp = x; }
        }
        if (posp == NULL)
          goto nonprd;
        for (x = posp;
             x > buf0 && *x > FL(0.0); x--);   /* & its preceding z-crossing */
        if (*x > FL(0.0)) goto nonprd;
        xdist = posp - x;
      } else if (p->polarity < 0) {
        cs_float negpk = FL(0.0);                  /* NEGATIVE polarity:   */
        cs_float *negp = NULL;
        for ( ; x < plim; x++) {                /* find ensuing min val */
          cs_float val = *x;
          if (val < negpk) { negpk = val; negp = x; }
        }
        if (negp == NULL)
          goto nonprd;
        for (x = negp;
             x > buf0 && *x < FL(0.0); x--); /* & its preceding z-crossing */
        if (*x < FL(0.0)) goto nonprd;
        xdist = negp - x;
      }
      else {
        cs_float pospk, negpk, *posp, *negp;               /* NOT SURE:    */
        cs_float *poscross, *negcross;
        int32_t posdist, negdist;
        pospk = negpk = FL(0.0);
        posp = negp = NULL;
        for ( ; x < plim; x++) {                /* find ensuing max & min vals */
          cs_float val = *x;
          if (val > FL(0.0)) {
            if (val > pospk) { pospk = val; posp = x; }
          } else
            if (val < negpk) { negpk = val; negp = x; }
        }
        if (posp == NULL || negp == NULL)
          goto nonprd;
        for (x = posp; x > buf0 &&
               *x > FL(0.0); x--); /* & their preceding z-crossings */
        if (*x > FL(0.0)) goto nonprd;
        posdist = posp - x;
        poscross = x;
        for (x = negp; x > buf0 && *x < FL(0.0); x--);
        if (*x < FL(0.0)) goto nonprd;
        negdist = negp - x;
        negcross = x;

        if (pospk / posdist > -negpk / negdist) {
          /* find z-cross with grtst slope to peak */
          if (UNLIKELY(p->poslead < LCNT)) {   /*      and consistent polarity */
            if (p->poslead == 1)
              csound->Warning(csound, "%s", Str("harm signal has positive lead\n"));
            p->poslead += 1;
          }
        }
        else {
          if (UNLIKELY(p->poslead > -LCNT)) {
            if (p->poslead == -1)
              csound->Warning(csound, "%s", Str("harm signal has negative lead\n"));
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
        int32_t nn, pulslen, sigdist, ndirect;
        cs_float *bufp;
        /* Avoid float index drift when the sigmoid spans a long pulse. */
        cs_double signdx, siginc;
        cs_float *z, zval, *newpuls = x;

        z = x + period;                         /*  and from estimated end      */
        if ((zval = *z) != FL(0.0)) {
          int32_t n, nlim = inp2 - z, nback = z - buf0;
          for (n = 1; n < nlim; n++) {
            if (zval * *(z+n) <= FL(0.0)) {     /*       find nearest zcrossing */
              z += n;
              break;
            } else if (n <= nback && zval * *(z-n) <= FL(0.0)) {
              z -= n;
              break;
            }                                   /* (true period is now z - x) */
          }
        }
        /* Extend both ends without forming pointers outside the history. */
        x = x - buf0 < xdist ? buf0 : x - xdist;
        z = inp2 - z < xdist ? inp2 : z + xdist;
        pulslen = z - x;
        if (pulslen > p->maxprd)
          pulslen = p->maxprd;                  /*      & storage limits        */
        if (xdist > pulslen / 4) goto nostor;
        sigdist = xdist * 2;
        ndirect = pulslen - sigdist*2;

        p->pbufcnt++;                           /* select a new puls buffr      */
        p->pbufcnt &= PBMSK;
        bufp = p->pulsbuf[p->pbufcnt];
        signdx = FL(0.0);                       /*      & store extended pulse  */
        siginc = (cs_double)SLEN / sigdist;
        for (nn = sigdist; nn--; signdx += siginc) {
          cs_float *sigp = p->sigmoid + (int32_t)signdx;
          *bufp++ = *x++ * *sigp;               /*      w. sigmoid-envlpd ends  */
        }
        //memcpy(bufp, x, sizeof(cs_float)*ndirect);
        while (ndirect--)
          *bufp++ = *x++;
        signdx = (cs_double)SLEN - siginc;
        for (nn = sigdist; nn--; signdx -= siginc) {
          cs_float *sigp = p->sigmoid + (int32_t)signdx;
          *bufp++ = *x++ * *sigp;
        }
        p->pulslen = pulslen;
        p->curpuls = newpuls;
      }
    nostor:
      if (p->curpuls == NULL) goto nonprd;
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
    //print_data(p, 3);
    /* HARMONIZER */
    for (vdp=p->vocdat; vdp<p->vlim; vdp++) {   /* get new frequencies  */
      cs_double inc = *vdp->kfrq * p->sicvt;
      if (UNLIKELY(!(inc >= INT32_MIN && inc <= (INT32_MAX + 0.0))))
        return csound->PerfError(csound, &p->h, "%s",
                                 Str("harmon234: voice frequency is out of range"));
      vdp->phsinc = (uint32_t)(int32_t)inc;
    }
    outp = p->ar + offset;
    vocamp = p->vocamp;
    diramp = FL(1.0) - vocamp;
    dirp = p->asig + offset;
    endp = p->endp;
    do {                                        /* insert pulses into output: */
      cs_float sum = FL(0.0);
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
        if (vdp->phsinc && (vdp->phase += vdp->phsinc) & 0xFFFF0000u) {
          vdp->phase &= 0x0000FFFFu;
        if (p->curpuls != NULL) {               /*      & pulses are current    */
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
      csound->Warning(csound, "%s", Str("harmon234: out of range\n"));
      p->hmrngflg = 0;
    }
    if (inp1 >= p->midp) {                       /* if end of pq bufs */
      int32 bsmps = p->nbufsmps;
      p->inp1 = p->bufp;                         /*   reset all ptrs  */
      p->inp2 = p->midp;
      if (p->curpuls != NULL)
        p->curpuls = p->curpuls >= p->midp ? p->curpuls - bsmps : NULL;
    }
    else {
      p->inp1 = inp1;
      p->inp2 = inp2;
    }
    //print_data(p, 4);
    return OK;
}

int32_t harm2set(CSOUND *csound, HARM234 *p)
{
    VOCDAT *vdp = p->vocdat;
    vdp->kfrq = p->kfrq1;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq2;       vdp->phase = 0; vdp++;
    p->vlim = vdp;
    p->polarity = (*p->icpsmode >= FL(1.0)) - (*p->icpsmode <= FL(-1.0));
    p->minoct = *p->kfrq4;
    p->cpsmode = ((*p->kfrq3 != FL(0.0)));
    return hm234set(csound, p);
}

int32_t harm3set(CSOUND *csound, HARM234 *p)
{
    VOCDAT *vdp = p->vocdat;
    vdp->kfrq = p->kfrq1;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq2;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq3;       vdp->phase = 0; vdp++;
    p->vlim = vdp;
    //printf("mode, lowest, polar = %p,%p,%p\n",
    //       p->icpsmode, p->ilowest, p->ipolarity);
    p->polarity = (*p->ilowest >= FL(1.0)) - (*p->ilowest <= FL(-1.0));
    p->minoct = *p->icpsmode;
    p->cpsmode = (*p->kfrq4 != FL(0.0));
    //printf("mode, lowest, polar = (%d,%f,%d)\n",
    //       p->cpsmode, p->minoct, p->ipolarity);
    return hm234set(csound, p);
}

int32_t harm4set(CSOUND *csound, HARM234 *p)
{
    VOCDAT *vdp = p->vocdat;
    vdp->kfrq = p->kfrq1;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq2;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq3;       vdp->phase = 0; vdp++;
    vdp->kfrq = p->kfrq4;       vdp->phase = 0; vdp++;
    p->vlim = vdp;
    p->polarity = (*p->ipolarity >= FL(1.0)) - (*p->ipolarity <= FL(-1.0));
    p->minoct = *p->ilowest;
    p->cpsmode = (*p->icpsmode != FL(0.0));
    return hm234set(csound, p);
}

#define S(x)    sizeof(x)

static OENTRY harmon_localops[] =
  {
   { "harmon2",S(HARM234),0,"a","akkkiip",  (SUBR)harm2set, (SUBR)harmon234 },
   { "harmon3",S(HARM234),0,"a","akkkkiip", (SUBR)harm3set, (SUBR)harmon234 },
   { "harmon4",S(HARM234),0,"a","akkkkkiip",(SUBR)harm4set, (SUBR)harmon234 },
};

LINKAGE_BUILTIN(harmon_localops)
