/*
    newfils.c:
    filter opcodes

    (c) Victor Lazzarini, 2004

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

#include "stdopcod.h"

#include "newfils.h"
#include <math.h>

static int moogladder_init(CSOUND *csound,moogladder *p)
{
    int i;
    if (*p->istor == FL(0.0)) {
      for (i = 0; i < 6; i++)
        p->delay[i] = 0.0;
      for (i = 0; i < 3; i++)
        p->tanhstg[i] = 0.0;
      p->oldfreq = FL(0.0);
      p->oldres = -FL(1.0);     /* ensure calculation on first cycle */
    }
    return OK;
}

static int moogladder_process(CSOUND *csound,moogladder *p)
{
    MYFLT   *out = p->out;
    MYFLT   *in = p->in;
    MYFLT   freq = *p->freq;
    MYFLT   res = *p->res;
    double  res4;
    double  *delay = p->delay;
    double  *tanhstg = p->tanhstg;
    double  stg[4], input;
    double  acr, tune;
#define THERMAL (0.000025) /* (1.0 / 40000.0) transistor thermal voltage  */
    int     j, k;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    if (res < 0) res = 0;

    if (p->oldfreq != freq || p->oldres != res) {
      double  f, fc, fc2, fc3, fcr;
      p->oldfreq = freq;
      /* sr is half the actual filter sampling rate  */
      fc =  (double)(freq/CS_ESR);
      f  =  0.5*fc;
      fc2 = fc*fc;
      fc3 = fc2*fc;
      /* frequency & amplitude correction  */
      fcr = 1.8730*fc3 + 0.4955*fc2 - 0.6490*fc + 0.9988;
      acr = -3.9364*fc2 + 1.8409*fc + 0.9968;
      tune = (1.0 - exp(-(TWOPI*f*fcr))) / THERMAL;   /* filter tuning  */
      p->oldres = res;
      p->oldacr = acr;
      p->oldtune = tune;
    }
    else {
      res = p->oldres;
      acr = p->oldacr;
      tune = p->oldtune;
    }
    res4 = 4.0*(double)res*acr;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i < nsmps; i++) {
      /* oversampling  */
      for (j = 0; j < 2; j++) {
        /* filter stages  */
        input = in[i] - res4 /*4.0*res*acr*/ *delay[5];
        delay[0] = stg[0] = delay[0] + tune*(tanh(input*THERMAL) - tanhstg[0]);
#if 0
        input = stg[0];
        stg[1] = delay[1] + tune*((tanhstg[0] = tanh(input*THERMAL)) - tanhstg[1]);
        input = delay[1] = stg[1];
        stg[2] = delay[2] + tune*((tanhstg[1] = tanh(input*THERMAL)) - tanhstg[2]);
        input = delay[2] = stg[2];
        stg[3] = delay[3] + tune*((tanhstg[2] =
                                   tanh(input*THERMAL)) - tanh(delay[3]*THERMAL));
        delay[3] = stg[3];
#else
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = tanh(input*THERMAL))
                    - (k != 3 ? tanhstg[k] : tanh(delay[k]*THERMAL)));
          delay[k] = stg[k];
        }
#endif
        /* 1/2-sample delay for phase compensation  */
        delay[5] = (stg[3] + delay[4])*0.5;
        delay[4] = stg[3];
      }
      out[i] = (MYFLT) delay[5];
    }
    return OK;
}

static int moogladder_process_aa(CSOUND *csound,moogladder *p)
{
    MYFLT   *out = p->out;
    MYFLT   *in = p->in;
    MYFLT   *freq = p->freq;
    MYFLT   *res = p->res;
    MYFLT   cfreq = freq[0], cres = res[0];
    double  res4;
    double  *delay = p->delay;
    double  *tanhstg = p->tanhstg;
    double  stg[4], input;
    double  acr, tune;
#define THERMAL (0.000025) /* (1.0 / 40000.0) transistor thermal voltage  */
    int     j, k;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    if (p->oldfreq != cfreq || p->oldres != cres) {
      double  f, fc, fc2, fc3, fcr;
      p->oldfreq = cfreq;
      /* sr is half the actual filter sampling rate  */
      fc =  (double)(cfreq/CS_ESR);
      f  =  0.5*fc;
      fc2 = fc*fc;
      fc3 = fc2*fc;
      /* frequency & amplitude correction  */
      fcr = 1.8730*fc3 + 0.4955*fc2 - 0.6490*fc + 0.9988;
      acr = -3.9364*fc2 + 1.8409*fc + 0.9968;
      tune = (1.0 - exp(-(TWOPI*f*fcr))) / THERMAL;   /* filter tuning  */
      p->oldres = cres;
      p->oldacr = acr;
      p->oldtune = tune;
    }
    else {
      cres = p->oldres;
      acr = p->oldacr;
      tune = p->oldtune;
    }
    res4 = 4.0*(double)cres*acr;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i < nsmps; i++) {
      if (p->oldfreq != freq[i] || p->oldres != res[i]) {
        double  f, fc, fc2, fc3, fcr;
        p->oldfreq = freq[i];
        /* sr is half the actual filter sampling rate  */
        fc =  (double)(freq[i]/CS_ESR);
        f  =  0.5*fc;
        fc2 = fc*fc;
        fc3 = fc2*fc;
        /* frequency & amplitude correction  */
        fcr = 1.8730*fc3 + 0.4955*fc2 - 0.6490*fc + 0.9988;
        acr = -3.9364*fc2 + 1.8409*fc + 0.9968;
        tune = (1.0 - exp(-(TWOPI*f*fcr))) / THERMAL;   /* filter tuning  */
        p->oldres = cres;
        p->oldacr = acr;
        p->oldtune = tune;
        res4 = 4.0*(double)res[i]*acr;
      }
      /* oversampling  */
      for (j = 0; j < 2; j++) {
        /* filter stages  */
        input = in[i] - res4 /*4.0*res*acr*/ *delay[5];
        delay[0] = stg[0] = delay[0] + tune*(tanh(input*THERMAL) - tanhstg[0]);
#if 0
        input = stg[0];
        stg[1] = delay[1] + tune*((tanhstg[0] = tanh(input*THERMAL)) - tanhstg[1]);
        input = delay[1] = stg[1];
        stg[2] = delay[2] + tune*((tanhstg[1] = tanh(input*THERMAL)) - tanhstg[2]);
        input = delay[2] = stg[2];
        stg[3] = delay[3] + tune*((tanhstg[2] =
                                   tanh(input*THERMAL)) - tanh(delay[3]*THERMAL));
        delay[3] = stg[3];
#else
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = tanh(input*THERMAL))
                    - (k != 3 ? tanhstg[k] : tanh(delay[k]*THERMAL)));
          delay[k] = stg[k];
        }
#endif
        /* 1/2-sample delay for phase compensation  */
        delay[5] = (stg[3] + delay[4])*0.5;
        delay[4] = stg[3];
      }
      out[i] = (MYFLT) delay[5];
    }
    return OK;
}

static int moogladder_process_ak(CSOUND *csound,moogladder *p)
{
    MYFLT   *out = p->out;
    MYFLT   *in = p->in;
    MYFLT   *freq = p->freq;
    MYFLT   res = *p->res;
    double  res4;
    double  *delay = p->delay;
    double  *tanhstg = p->tanhstg;
    double  stg[4], input;
    double  acr, tune;
#define THERMAL (0.000025) /* (1.0 / 40000.0) transistor thermal voltage  */
    int     j, k;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    if (res < 0) res = 0;

    if (p->oldfreq != freq[0] || p->oldres != res) {
      double  f, fc, fc2, fc3, fcr;
      p->oldfreq = freq[0];
      /* sr is half the actual filter sampling rate  */
      fc =  (double)(freq[0]/CS_ESR);
      f  =  0.5*fc;
      fc2 = fc*fc;
      fc3 = fc2*fc;
      /* frequency & amplitude correction  */
      fcr = 1.8730*fc3 + 0.4955*fc2 - 0.6490*fc + 0.9988;
      acr = -3.9364*fc2 + 1.8409*fc + 0.9968;
      tune = (1.0 - exp(-(TWOPI*f*fcr))) / THERMAL;   /* filter tuning  */
      p->oldres = res;
      p->oldacr = acr;
      p->oldtune = tune;
    }
    else {
      res = p->oldres;
      acr = p->oldacr;
      tune = p->oldtune;
    }
    res4 = 4.0*(double)res*acr;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i < nsmps; i++) {
      if (p->oldfreq != freq[i]) {
        double  f, fc, fc2, fc3, fcr;
        p->oldfreq = freq[i];
        /* sr is half the actual filter sampling rate  */
        fc =  (double)(freq[i]/CS_ESR);
        f  =  0.5*fc;
        fc2 = fc*fc;
        fc3 = fc2*fc;
        /* frequency & amplitude correction  */
        fcr = 1.8730*fc3 + 0.4955*fc2 - 0.6490*fc + 0.9988;
        acr = -3.9364*fc2 + 1.8409*fc + 0.9968;
        tune = (1.0 - exp(-(TWOPI*f*fcr))) / THERMAL;   /* filter tuning  */
        p->oldacr = acr;
        p->oldtune = tune;
        res4 = 4.0*(double)res*acr;
      }
      /* oversampling  */
      for (j = 0; j < 2; j++) {
        /* filter stages  */
        input = in[i] - res4 /*4.0*res*acr*/ *delay[5];
        delay[0] = stg[0] = delay[0] + tune*(tanh(input*THERMAL) - tanhstg[0]);
#if 0
        input = stg[0];
        stg[1] = delay[1] + tune*((tanhstg[0] = tanh(input*THERMAL)) - tanhstg[1]);
        input = delay[1] = stg[1];
        stg[2] = delay[2] + tune*((tanhstg[1] = tanh(input*THERMAL)) - tanhstg[2]);
        input = delay[2] = stg[2];
        stg[3] = delay[3] + tune*((tanhstg[2] =
                                   tanh(input*THERMAL)) - tanh(delay[3]*THERMAL));
        delay[3] = stg[3];
#else
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = tanh(input*THERMAL))
                    - (k != 3 ? tanhstg[k] : tanh(delay[k]*THERMAL)));
          delay[k] = stg[k];
        }
#endif
        /* 1/2-sample delay for phase compensation  */
        delay[5] = (stg[3] + delay[4])*0.5;
        delay[4] = stg[3];
      }
      out[i] = (MYFLT) delay[5];
    }
    return OK;
}

static int moogladder_process_ka(CSOUND *csound,moogladder *p)
{
    MYFLT   *out = p->out;
    MYFLT   *in = p->in;
    MYFLT   freq = *p->freq;
    MYFLT   *res = p->res;
    MYFLT cres = res[0];
    double  res4;
    double  *delay = p->delay;
    double  *tanhstg = p->tanhstg;
    double  stg[4], input;
    double  acr, tune;
#define THERMAL (0.000025) /* (1.0 / 40000.0) transistor thermal voltage  */
    int     j, k;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    if (cres < 0) cres = 0;

    if (p->oldfreq != freq || p->oldres != cres) {
      double  f, fc, fc2, fc3, fcr;
      p->oldfreq = freq;
      /* sr is half the actual filter sampling rate  */
      fc =  (double)(freq/CS_ESR);
      f  =  0.5*fc;
      fc2 = fc*fc;
      fc3 = fc2*fc;
      /* frequency & amplitude correction  */
      fcr = 1.8730*fc3 + 0.4955*fc2 - 0.6490*fc + 0.9988;
      acr = -3.9364*fc2 + 1.8409*fc + 0.9968;
      tune = (1.0 - exp(-(TWOPI*f*fcr))) / THERMAL;   /* filter tuning  */
      p->oldres = cres;
      p->oldacr = acr;
      p->oldtune = tune;
    }
    else {
      cres = p->oldres;
      acr = p->oldacr;
      tune = p->oldtune;
    }
    res4 = 4.0*(double)cres*acr;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i < nsmps; i++) {
      if (cres != res[i]) {
        double  f, fc, fc2, fc3, fcr;
        /* sr is half the actual filter sampling rate  */
        fc =  (double)(freq/CS_ESR);
        f  =  0.5*fc;
        fc2 = fc*fc;
        fc3 = fc2*fc;
        /* frequency & amplitude correction  */
        fcr = 1.8730*fc3 + 0.4955*fc2 - 0.6490*fc + 0.9988;
        acr = -3.9364*fc2 + 1.8409*fc + 0.9968;
        tune = (1.0 - exp(-(TWOPI*f*fcr))) / THERMAL;   /* filter tuning  */
        p->oldres = cres = res[i];
        p->oldacr = acr;
        p->oldtune = tune;
        res4 = 4.0*(double)cres*acr;
      }
      /* oversampling  */
      for (j = 0; j < 2; j++) {
        /* filter stages  */
        input = in[i] - res4 /*4.0*res*acr*/ *delay[5];
        delay[0] = stg[0] = delay[0] + tune*(tanh(input*THERMAL) - tanhstg[0]);
#if 0
        input = stg[0];
        stg[1] = delay[1] + tune*((tanhstg[0] = tanh(input*THERMAL)) - tanhstg[1]);
        input = delay[1] = stg[1];
        stg[2] = delay[2] + tune*((tanhstg[1] = tanh(input*THERMAL)) - tanhstg[2]);
        input = delay[2] = stg[2];
        stg[3] = delay[3] + tune*((tanhstg[2] =
                                   tanh(input*THERMAL)) - tanh(delay[3]*THERMAL));
        delay[3] = stg[3];
#else
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = tanh(input*THERMAL))
                    - (k != 3 ? tanhstg[k] : tanh(delay[k]*THERMAL)));
          delay[k] = stg[k];
        }
#endif
        /* 1/2-sample delay for phase compensation  */
        delay[5] = (stg[3] + delay[4])*0.5;
        delay[4] = stg[3];
      }
      out[i] = (MYFLT) delay[5];
    }
    return OK;
}

static int statevar_init(CSOUND *csound,statevar *p)
{
    if (*p->istor==FL(0.0)) {
      p->bpd = p->lpd = p->lp = 0.0;
      p->oldfreq = FL(0.0);
      p->oldres = FL(0.0);
    }
    if (*p->osamp<=FL(0.0)) p->ostimes = 3;
    else p->ostimes = (int) *p->osamp;
    return OK;
}

static int statevar_process(CSOUND *csound,statevar *p)
{
    MYFLT  *outhp = p->outhp;
    MYFLT  *outlp = p->outlp;
    MYFLT  *outbp = p->outbp;
    MYFLT  *outbr = p->outbr;
    MYFLT  *in = p->in;
    MYFLT  *freq = p->freq;
    MYFLT  *res  = p->res;
    double  lpd = p->lpd;
    double  bpd = p->bpd;
    double  lp  = p->lp, hp = 0.0, bp = 0.0, br = 0.0;
    double  f,q,lim;
    int ostimes = p->ostimes,j;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) {
      memset(outhp, '\0', offset*sizeof(MYFLT));
      memset(outlp, '\0', offset*sizeof(MYFLT));
      memset(outbp, '\0', offset*sizeof(MYFLT));
      memset(outbr, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&outhp[nsmps], '\0', early*sizeof(MYFLT));
      memset(&outlp[nsmps], '\0', early*sizeof(MYFLT));
      memset(&outbp[nsmps], '\0', early*sizeof(MYFLT));
      memset(&outbr[nsmps], '\0', early*sizeof(MYFLT));
    }
    q = p->oldq;
    f = p->oldf;

    for (i=offset; i<nsmps; i++) {
      MYFLT fr = (IS_ASIG_ARG(p->freq) ? freq[i] : *freq);
      MYFLT rs = (IS_ASIG_ARG(p->res) ? res[i] : *res);
      if (p->oldfreq != fr|| p->oldres != rs) {
        f = 2.0*sin(fr*(double)csound->pidsr/ostimes);
        q = 1.0/rs;
        lim = ((2.0 - f) *0.05)/ostimes;
        /* csound->Message(csound, "lim: %f, q: %f \n", lim, q); */

        if (q < lim) q = lim;
        p->oldq = q;
        p->oldf = f;
        p->oldfreq = fr;
        p->oldres = rs;
      }
      for (j=0; j<ostimes; j++) {

        hp = in[i] - q*bpd - lp;
        bp = hp*f + bpd;
        lp = bpd*f + lpd;
        br = lp + hp;
        bpd = bp;
        lpd = lp;
      }

      outhp[i] = (MYFLT) hp;
      outlp[i] = (MYFLT) lp;
      outbp[i] = (MYFLT) bp;
      outbr[i] = (MYFLT) br;

    }
    p->bpd = bpd;
    p->lpd = lpd;
    p->lp = lp;

    return OK;
}

static int fofilter_init(CSOUND *csound,fofilter *p)
{
    int i;
    if (*p->istor==FL(0.0)) {
      for (i=0;i<4; i++)
        p->delay[i] = 0.0;
    }
    return OK;
}

static int fofilter_process(CSOUND *csound,fofilter *p)
{
    MYFLT  *out = p->out;
    MYFLT  *in = p->in;
    MYFLT  *freq = p->freq;
    MYFLT  *ris = p->ris;
    MYFLT  *dec = p->dec;
    double  *delay = p->delay,ang=0,fsc,rrad1=0,rrad2=0;
    double  w1,y1,w2,y2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    MYFLT lfrq = -FL(1.0), lrs = -FL(1.0), ldc = -FL(1.0);

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i=offset;i<nsmps;i++) {
      MYFLT frq = IS_ASIG_ARG(p->freq) ? freq[i] : *freq;
      MYFLT rs = IS_ASIG_ARG(p->ris) ? ris[i] : *ris;
      MYFLT dc = IS_ASIG_ARG(p->dec) ? dec[i] : *dec;
      if (frq != lfrq || rs != lrs || dc != ldc) {
        lfrq = frq; lrs = rs; ldc = dc;
        ang = (double)csound->tpidsr*frq;         /* pole angle */
        fsc = sin(ang) - 3.0;                      /* freq scl   */
        rrad1 =  pow(10.0, fsc/(dc*CS_ESR));  /* filter radii */
        rrad2 =  pow(10.0, fsc/(rs*CS_ESR));
      }

      w1  = in[i] + 2.0*rrad1*cos(ang)*delay[0] - rrad1*rrad1*delay[1];
      y1 =  w1 - delay[1];
      delay[1] = delay[0];
      delay[0] = w1;

      w2  = in[i] + 2.0*rrad2*cos(ang)*delay[2] - rrad2*rrad2*delay[3];
      y2 =  w2 - delay[3];
      delay[3] = delay[2];
      delay[2] = w2;

      out[i] = (MYFLT) (y1 - y2);

    }
    return OK;
}

/* filter designs by Fons Adriaensen */
typedef struct _mvcf {
  OPDS h;
  MYFLT *out;
  MYFLT *in, *freq, *res, *skipinit;
  double c1, c2, c3, c4, c5;
  double fr, w;
} mvclpf24;

double exp2ap(double x) {
  int i = (int) (floor(x));
  x -= i;
  return ldexp(1 + x * (0.6930 +
		   x * (0.2416 + x * (0.0517 +
		   x * 0.0137))), i);
}


int mvclpf24_init(CSOUND *csound, mvclpf24 *p){
  if(!*p->skipinit){
     p->c1 = p->c2  = p->c3 = 
       p->c4 = p->c5 = FL(0.0);
     p->fr = FL(0.0);
  }
  return OK;
}

int mvclpf24_perf(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, res;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, t;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  
  if(p->fr != *p->freq) {
    MYFLT fr = *p->freq;///(0.5*csound->GetSr(csound));
    p->fr  = *p->freq;
    //fr = (fr*12)-6;
    printf("%f \n",fr);
    w = exp2ap(fr + 10.82)/csound->GetSr(csound);
    if (w < 0.8) w *= 1 - 0.4 * w - 0.125 * w * w;
    else {
     w *= 0.6; 
     if (w > 0.92) w = 0.92;
    }
    p->w = w;
  } else w = p->w;
  
  res = *p->res > FL(0.0) ?
    (*p->res < FL(1.0) ?
     *p->res : FL(1.0)) : FL(0.0);

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
   
  for(i=offset; i < nsmps; i++){
	x = -4.2*res*c5 + in[i] + 1e-10;
        t = c1 / (1 + fabs (c1));
	c1 += w*(x - t);
	x = c1 / (1 + fabs (c1));
        c2 += w * (x  - c2);
	c3 += w * (c2 - c3);
	c4 += w * (c3 - c4);
	out[i]  = c4;
	c5 += 0.5 * (c4 - c5);
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;
  
  return OK;
}

static OENTRY localops[] = {
  {"mvclpf24", sizeof(mvclpf24), 0, 5, "a", "akkp",
                (SUBR) mvclpf24_init, NULL, (SUBR) mvclpf24_perf},
  {"moogladder.kk", sizeof(moogladder), 0, 5, "a", "akkp",
                    (SUBR) moogladder_init, NULL, (SUBR) moogladder_process },
  {"moogladder.aa", sizeof(moogladder), 0, 5, "a", "aaap",
                    (SUBR) moogladder_init, NULL, (SUBR) moogladder_process_aa },
  {"moogladder.ak", sizeof(moogladder), 0, 5, "a", "aakp",
                    (SUBR) moogladder_init, NULL, (SUBR) moogladder_process_ak },
  {"moogladder.ka", sizeof(moogladder), 0, 5, "a", "akap",
                    (SUBR) moogladder_init, NULL, (SUBR) moogladder_process_ka },
  {"statevar", sizeof(statevar), 0, 5, "aaaa", "axxop",
                    (SUBR) statevar_init, NULL, (SUBR) statevar_process     },
  {"fofilter", sizeof(fofilter), 0, 5, "a", "axxxp",
                    (SUBR) fofilter_init, NULL, (SUBR) fofilter_process     }
};

int newfils_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}
