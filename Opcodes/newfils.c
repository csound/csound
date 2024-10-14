/*
  newfils.c:
  filter opcodes

  Copyright (c) Victor Lazzarini, 2004, Gleb Rogozinsky, 2020

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

#include "stdopcod.h"

#include "newfils.h"
#include <math.h>

#define TABSIZE 20000
static inline MYFLT nlf(MYFLT *t, double x, MYFLT mx, size_t siz){
  double p =  (x*mx + 0.5)*siz;
  size_t n = (size_t) p;
  return n > 0 ? (n < siz ? t[n] + (p - n)*(t[n+1] - t[n]) : t[siz-1]) : t[0];
}


static inline
double fast_tanh(double x)
{
  double x2 = x * x;
  double a = x * (135135.0 + x2 * (17325.0 + x2 * (378.0 + x2)));
  double b = 135135.0 + x2 * (62370.0 + x2 * (3150.0 + x2 * 28.0));
  return a / b;
}

static double TanH(double x)
{
  /* use the fact that tanh(-x) = - tanh(x)
     and if x>~4 tanh is approx constant 1
     and for small x tanh(x) =~ x
     So giving a cheap approximation */
  int32_t sign = 1;
  if (x<0) sign=-1, x= -x;
  if (x>=4.0) {
    return sign;
  }
  if (x<0.5) return x*sign;
#ifdef JPFF
  printf("x=%g (%g,%g)\n",x, fast_tanh(x),tanh(x));
#endif
  return sign*fast_tanh(x);
}

static int32_t moogladder_init(CSOUND *csound, moogladder *p)
{
  /* int32_t i; */
  IGN(csound);
  if (LIKELY(*p->istor == FL(0.0))) {
    /* for (i = 0; i < 6; i++) */
    /*   p->delay[i] = 0.0; */
    memset(p->delay, '\0', 6*sizeof(double));
    /* for (i = 0; i < 3; i++) */
    /*   p->tanhstg[i] = 0.0; */
    memset(p->tanhstg, '\0', 3*sizeof(double));
    p->oldfreq = FL(0.0);
    p->oldres = -FL(1.0);     /* ensure calculation on first cycle */
  }
  return OK;
}

static int32_t moogladder_process(CSOUND *csound, moogladder *p)
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
  double vt = 1./(1.22070315*csound->Get0dBFS(csound)); /* (1.0 / 40000.0) transistor thermal voltage  */
  int32_t     j;
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
    tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
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
      input = in[i] - res4*delay[5];
      delay[0] = stg[0] = delay[0] + tune*(tanh(input*vt) - tanhstg[0]);
#if 1
      input = stg[0];
      stg[1] = delay[1] + tune*((tanhstg[0] = tanh(input*vt)) - tanhstg[1]);
      input = delay[1] = stg[1];
      stg[2] = delay[2] + tune*((tanhstg[1] = tanh(input*vt)) - tanhstg[2]);
      input = delay[2] = stg[2];
      stg[3] = delay[3] + tune*((tanhstg[2] =
                                 tanh(input*vt)) - tanh(delay[3]*vt));
      delay[3] = stg[3];
#else
      { int32_t k;
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = tanh(input*vt))
                    - (k != 3 ? tanhstg[k] : tanh(delay[k]*vt)));
          delay[k] = stg[k];
        }
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

static int32_t moogladder_process_aa(CSOUND *csound, moogladder *p)
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
  double vt = 1./(1.22070315*csound->Get0dBFS(csound)); /* (1.0 / 40000.0) transistor thermal voltage  */
  int32_t     j;
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
    tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
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
      tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
      p->oldres = cres;
      p->oldacr = acr;
      p->oldtune = tune;
      res4 = 4.0*(double)res[i]*acr;
    }
    /* oversampling  */
    for (j = 0; j < 2; j++) {
      /* filter stages  */
      input = in[i] - res4 /*4.0*res*acr*/ *delay[5];
      delay[0] = stg[0] = delay[0] + tune*(tanh(input*vt) - tanhstg[0]);
#if 1
      input = stg[0];
      stg[1] = delay[1] + tune*((tanhstg[0] = tanh(input*vt)) - tanhstg[1]);
      input = delay[1] = stg[1];
      stg[2] = delay[2] + tune*((tanhstg[1] = tanh(input*vt)) - tanhstg[2]);
      input = delay[2] = stg[2];
      stg[3] = delay[3] + tune*((tanhstg[2] =
                                 tanh(input*vt)) - tanh(delay[3]*vt));
      delay[3] = stg[3];
#else
      { int32_t k;
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = tanh(input*vt))
                    - (k != 3 ? tanhstg[k] : tanh(delay[k]*vt)));
          delay[k] = stg[k];
        }
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

static int32_t moogladder_process_ak(CSOUND *csound, moogladder *p)
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
  double vt = 1./(1.22070315*csound->Get0dBFS(csound)); /* (1.0 / 40000.0) transistor thermal voltage  */
  int32_t     j;
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
    tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
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
      tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
      p->oldacr = acr;
      p->oldtune = tune;
      res4 = 4.0*(double)res*acr;
    }
    /* oversampling  */
    for (j = 0; j < 2; j++) {
      /* filter stages  */
      input = in[i] - res4 /*4.0*res*acr*/ *delay[5];
      delay[0] = stg[0] = delay[0] + tune*(tanh(input*vt) - tanhstg[0]);
#if 1
      input = stg[0];
      stg[1] = delay[1] + tune*((tanhstg[0] = tanh(input*vt)) - tanhstg[1]);
      input = delay[1] = stg[1];
      stg[2] = delay[2] + tune*((tanhstg[1] = tanh(input*vt)) - tanhstg[2]);
      input = delay[2] = stg[2];
      stg[3] = delay[3] + tune*((tanhstg[2] =
                                 tanh(input*vt)) - tanh(delay[3]*vt));
      delay[3] = stg[3];
#else
      { int32_t k;
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = tanh(input*vt))
                    - (k != 3 ? tanhstg[k] : tanh(delay[k]*vt)));
          delay[k] = stg[k];
        }
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

static int32_t moogladder_process_ka(CSOUND *csound, moogladder *p)
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
  double vt = 1./(1.22070315*csound->Get0dBFS(csound)); /* (1.0 / 40000.0) transistor thermal voltage  */
  int32_t     j;
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
    tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
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
      tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
      p->oldres = cres = res[i];
      p->oldacr = acr;
      p->oldtune = tune;
      res4 = 4.0*(double)cres*acr;
    }
    /* oversampling  */
    for (j = 0; j < 2; j++) {
      /* filter stages  */
      input = in[i] - res4 /*4.0*res*acr*/ *delay[5];
      delay[0] = stg[0] = delay[0] + tune*(tanh(input*vt) - tanhstg[0]);
#if 1
      input = stg[0];
      stg[1] = delay[1] + tune*((tanhstg[0] = tanh(input*vt)) - tanhstg[1]);
      input = delay[1] = stg[1];
      stg[2] = delay[2] + tune*((tanhstg[1] = tanh(input*vt)) - tanhstg[2]);
      input = delay[2] = stg[2];
      stg[3] = delay[3] + tune*((tanhstg[2] =
                                 tanh(input*vt)) - tanh(delay[3]*vt));
      delay[3] = stg[3];
#else
      { int32_t k;
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = tanh(input*vt))
                    - (k != 3 ? tanhstg[k] : tanh(delay[k]*vt)));
          delay[k] = stg[k];
        }
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

static int32_t moogladder2_process(CSOUND *csound, moogladder *p)
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
  double vt = 1./(1.22070315*csound->Get0dBFS(csound)); /* (1.0 / 40000.0) transistor thermal voltage  */
  int32_t     j;
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
    tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
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
      input = in[i] - res4*delay[5];
      delay[0] = stg[0] = delay[0] + tune*(TanH(input*vt) - tanhstg[0]);
#if 1
      input = stg[0];
      stg[1] = delay[1] + tune*((tanhstg[0] = TanH(input*vt)) - tanhstg[1]);
      input = delay[1] = stg[1];
      stg[2] = delay[2] + tune*((tanhstg[1] = TanH(input*vt)) - tanhstg[2]);
      input = delay[2] = stg[2];
      stg[3] = delay[3] + tune*((tanhstg[2] =
                                 TanH(input*vt)) - TanH(delay[3]*vt));
      delay[3] = stg[3];
#else
      { int32_t k;
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = TanH(input*vt))
                    - (k != 3 ? tanhstg[k] : TanH(delay[k]*vt)));
          delay[k] = stg[k];
        }
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

static int32_t moogladder2_process_aa(CSOUND *csound, moogladder *p)
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
  double vt = 1./(1.22070315*csound->Get0dBFS(csound)); /* (1.0 / 40000.0) transistor thermal voltage  */
  int32_t     j;
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
    tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
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
      tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
      p->oldres = cres;
      p->oldacr = acr;
      p->oldtune = tune;
      res4 = 4.0*(double)res[i]*acr;
    }
    /* oversampling  */
    for (j = 0; j < 2; j++) {
      /* filter stages  */
      input = in[i] - res4 /*4.0*res*acr*/ *delay[5];
      delay[0] = stg[0] = delay[0] + tune*(TanH(input*vt) - tanhstg[0]);
#if 1
      input = stg[0];
      stg[1] = delay[1] + tune*((tanhstg[0] = TanH(input*vt)) - tanhstg[1]);
      input = delay[1] = stg[1];
      stg[2] = delay[2] + tune*((tanhstg[1] = TanH(input*vt)) - tanhstg[2]);
      input = delay[2] = stg[2];
      stg[3] = delay[3] + tune*((tanhstg[2] =
                                 TanH(input*vt)) - TanH(delay[3]*vt));
      delay[3] = stg[3];
#else
      { int32_t k;
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = TanH(input*vt))
                    - (k != 3 ? tanhstg[k] : TanH(delay[k]*vt)));
          delay[k] = stg[k];
        }
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

static int32_t moogladder2_process_ak(CSOUND *csound, moogladder *p)
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
  double vt = 1./(1.22070315*csound->Get0dBFS(csound)); /* (1.0 / 40000.0) transistor thermal voltage  */
  int32_t     j;
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
    tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
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
      tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
      p->oldacr = acr;
      p->oldtune = tune;
      res4 = 4.0*(double)res*acr;
    }
    /* oversampling  */
    for (j = 0; j < 2; j++) {
      /* filter stages  */
      input = in[i] - res4 /*4.0*res*acr*/ *delay[5];
      delay[0] = stg[0] = delay[0] + tune*(TanH(input*vt) - tanhstg[0]);
#if 1
      input = stg[0];
      stg[1] = delay[1] + tune*((tanhstg[0] = TanH(input*vt)) - tanhstg[1]);
      input = delay[1] = stg[1];
      stg[2] = delay[2] + tune*((tanhstg[1] = TanH(input*vt)) - tanhstg[2]);
      input = delay[2] = stg[2];
      stg[3] = delay[3] + tune*((tanhstg[2] =
                                 TanH(input*vt)) - TanH(delay[3]*vt));
      delay[3] = stg[3];
#else
      { int32_t k;
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = TanH(input*vt))
                    - (k != 3 ? tanhstg[k] : TanH(delay[k]*vt)));
          delay[k] = stg[k];
        }
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

static int32_t moogladder2_process_ka(CSOUND *csound, moogladder *p)
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
  double vt = 1./(1.22070315*csound->Get0dBFS(csound)); /* (1.0 / 40000.0) transistor thermal voltage  */
  int32_t     j;
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
    tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
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
      tune = (1.0 - exp(-(TWOPI*f*fcr))) / vt;   /* filter tuning  */
      p->oldres = cres = res[i];
      p->oldacr = acr;
      p->oldtune = tune;
      res4 = 4.0*(double)cres*acr;
    }
    /* oversampling  */
    for (j = 0; j < 2; j++) {
      /* filter stages  */
      input = in[i] - res4 /*4.0*res*acr*/ *delay[5];
      delay[0] = stg[0] = delay[0] + tune*(TanH(input*vt) - tanhstg[0]);
#if 1
      input = stg[0];
      stg[1] = delay[1] + tune*((tanhstg[0] = TanH(input*vt)) - tanhstg[1]);
      input = delay[1] = stg[1];
      stg[2] = delay[2] + tune*((tanhstg[1] = TanH(input*vt)) - tanhstg[2]);
      input = delay[2] = stg[2];
      stg[3] = delay[3] + tune*((tanhstg[2] =
                                 TanH(input*vt)) - TanH(delay[3]*vt));
      delay[3] = stg[3];
#else
      { int32_t k;
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = TanH(input*vt))
                    - (k != 3 ? tanhstg[k] : TanH(delay[k]*vt)));
          delay[k] = stg[k];
        }
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

static int32_t statevar_init(CSOUND *csound,statevar *p)
{
  IGN(csound);
  if (*p->istor==FL(0.0)) {
    p->bpd = p->lpd = p->lp = 0.0;
    p->oldfreq = FL(0.0);
    p->oldres = FL(0.0);
  }
  if (*p->osamp<=FL(0.0)) p->ostimes = 3;
  else p->ostimes = (int32_t) *p->osamp;
  return OK;
}

static int32_t statevar_process(CSOUND *csound,statevar *p)
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
  int32_t ostimes = p->ostimes,j;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  int32_t      asgfr = IS_ASIG_ARG(p->freq), asgrs = IS_ASIG_ARG(p->res);

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
    MYFLT fr = (asgfr ? freq[i] : *freq);
    MYFLT rs = (asgrs ? res[i] : *res);
    if (p->oldfreq != fr|| p->oldres != rs) {
      f = 2.0*sin(fr*(double)CS_PIDSR/ostimes);
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

static int32_t fofilter_init(CSOUND *csound,fofilter *p)
{
  IGN(csound);
  int32_t i;
  if (*p->istor==FL(0.0)) {
    for (i=0;i<4; i++)
      p->delay[i] = 0.0;
  }
  return OK;
}

static int32_t fofilter_process(CSOUND *csound,fofilter *p)
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
  int32_t   asgfr = IS_ASIG_ARG(p->freq) , asgrs = IS_ASIG_ARG(p->ris);
  int32_t   asgdc = IS_ASIG_ARG(p->dec);

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (i=offset;i<nsmps;i++) {
    MYFLT frq = asgfr ? freq[i] : *freq;
    MYFLT rs = asgrs ? ris[i] : *ris;
    MYFLT dc = asgdc ? dec[i] : *dec;
    if (frq != lfrq || rs != lrs || dc != ldc) {
      lfrq = frq; lrs = rs; ldc = dc;
      ang = (double)CS_TPIDSR*frq;         /* pole angle */
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
  int32_t i = (int32_t) (floor(x));
  x -= i;
  return ldexp(1 + x * (0.6930 +
                        x * (0.2416 + x * (0.0517 +
                                           x * 0.0137))), i);
}


int32_t mvclpf24_init(CSOUND *csound, mvclpf24 *p){
  IGN(csound);
  if (!*p->skipinit){
    p->c1 = p->c2  = p->c3 =
      p->c4 = p->c5 = FL(0.0);
    p->fr = FL(0.0);
  }
  return OK;
}
#define CBASE 261.62556416

int32_t mvclpf24_perf1(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, res;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, t;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (p->fr != *p->freq) {
    MYFLT fr = log2(*p->freq/CBASE);
    p->fr  = *p->freq;
    w = exp2ap(fr + 10.82)/CS_ESR;
    if (w < 0.8) w *= 1 - 0.4 * w - 0.125 * w * w;
    else {
      w *= 0.6;
      if (w > 0.92) w = 0.92;
    }
    p->w = w;
  } else w = p->w;

  res = *p->res;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    x = -4.2*res*c5 + in[i]/scal + 1e-10;
    t = c1 / (1 + fabs (c1));
    c1 += w*(x - t);
    x = c1 / (1 + fabs (c1));
    c2 += w * (x  - c2);
    c3 += w * (c2 - c3);
    c4 += w * (c3 - c4);
    out[i]  = c4*scal;
    c5 += 0.5 * (c4 - c5);
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf1_ak(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, res, *freq = p->freq;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, t;
  int32_t wi;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  res = *p->res;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    t  = log2(freq[i]/CBASE) + 10.82;
    wi = (int32_t) (floor(t));
    t -= wi;
    t  = ldexp(1 + t * (0.6930 +
                        t * (0.2416 + t * (0.0517 +
                                           t * 0.0137))), wi);
    w  = t/CS_ESR;
    if (w < 0.8)
      w *= 1 - 0.4 * w - 0.125 * w * w;
    else {
      w *= 0.6;
      if (w > 0.92) w = 0.92;
    }

    x = -4.2*res*c5 + in[i]/scal + 1e-10;
    t = c1 / (1 + fabs (c1));
    c1 += w*(x - t);
    x = c1 / (1 + fabs (c1));
    c2 += w * (x  - c2);
    c3 += w * (c2 - c3);
    c4 += w * (c3 - c4);
    out[i]  = c4*scal;
    c5 += 0.5 * (c4 - c5);
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf1_ka(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, *res = p->res;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, t;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (p->fr != *p->freq) {
    MYFLT fr = log2(*p->freq/CBASE);
    p->fr  = *p->freq;
    w = exp2ap(fr + 10.82)/CS_ESR;
    if (w < 0.8) w *= 1 - 0.4 * w - 0.125 * w * w;
    else {
      w *= 0.6;
      if (w > 0.92) w = 0.92;
    }
    p->w = w;
  } else w = p->w;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){

    x = -4.2*c5*res[i]
      + in[i]/scal + 1e-10;
    t = c1 / (1 + fabs (c1));
    c1 += w*(x - t);
    x = c1 / (1 + fabs (c1));
    c2 += w * (x  - c2);
    c3 += w * (c2 - c3);
    c4 += w * (c3 - c4);
    out[i]  = c4*scal;
    c5 += 0.5 * (c4 - c5);
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf1_aa(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, *res = p->res, *freq = p->freq;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, t;
  int32_t wi;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    t = log2(freq[i]/CBASE) + 10.82;
    wi = (int32_t) (floor(t));
    t -= wi;
    t = ldexp(1 + t * (0.6930 +
                       t * (0.2416 + t * (0.0517 +
                                          t * 0.0137))), wi);
    w = t/CS_ESR;
    if (w < 0.8)
      w *= 1 - 0.4 * w - 0.125 * w * w;
    else {
      w *= 0.6;
      if (w > 0.92) w = 0.92;
    }

    x = -4.2*c5*res[i]
      + in[i]/scal + 1e-10;
    t = c1 / (1 + fabs (c1));
    c1 += w*(x - t);
    x = c1 / (1 + fabs (c1));
    c2 += w * (x  - c2);
    c3 += w * (c2 - c3);
    c4 += w * (c3 - c4);
    out[i]  = c4*scal;
    c5 += 0.5 * (c4 - c5);
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf2(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, res;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (p->fr != *p->freq) {
    MYFLT fr = log2(*p->freq/CBASE);
    p->fr  = *p->freq;
    w = exp2ap(fr + 10.71)/CS_ESR;
    if (w < 0.8) w *= 1 - 0.4 * w - 0.125 * w * w;
    else {
      w *= 0.6;
      if (w > 0.92) w = 0.92;
    }
    p->w = w;
  } else w = p->w;

  res = *p->res;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    x = -4.5*res*c5 + in[i]/scal + 1e-10;
    x /= sqrt (1 + x * x);
    c1 += w * (x  - c1) / (1 + c1 * c1);
    c2 += w * (c1 - c2) / (1 + c2 * c2);
    c3 += w * (c2 - c3) / (1 + c3 * c3);
    c4 += w * (c3 - c4) / (1 + c4 * c4);
    out[i]  = c4*scal;
    c5 += 0.5 * (c4 - c5);
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf2_ak(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, res, *freq = p->freq ;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, t;
  int32_t wi;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  res = *p->res;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    t = log2(freq[i]/CBASE) + 10.71;
    wi = (int32_t) (floor(t));
    t -= wi;
    t = ldexp(1 + t * (0.6930 +
                       t * (0.2416 + t * (0.0517 +
                                          t * 0.0137))), wi);
    w = t/CS_ESR;
    if (w < 0.8)
      w *= 1 - 0.4 * w - 0.125 * w * w;
    else {
      w *= 0.6;
      if (w > 0.92) w = 0.92;
    }
    x = -4.5*res*c5 + in[i]/scal + 1e-10;
    x /= sqrt (1 + x * x);
    c1 += w * (x  - c1) / (1 + c1 * c1);
    c2 += w * (c1 - c2) / (1 + c2 * c2);
    c3 += w * (c2 - c3) / (1 + c3 * c3);
    c4 += w * (c3 - c4) / (1 + c4 * c4);
    out[i]  = c4*scal;
    c5 += 0.5 * (c4 - c5);
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf2_ka(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, *res = p->res;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (p->fr != *p->freq) {
    MYFLT fr = log2(*p->freq/CBASE);
    p->fr  = *p->freq;
    w = exp2ap(fr + 10.71)/CS_ESR;
    if (w < 0.8) w *= 1 - 0.4 * w - 0.125 * w * w;
    else {
      w *= 0.6;
      if (w > 0.92) w = 0.92;
    }
    p->w = w;
  } else w = p->w;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    x = -4.5*c5*res[i]
      + in[i]/scal + 1e-10;
    x /= sqrt (1 + x * x);
    c1 += w * (x  - c1) / (1 + c1 * c1);
    c2 += w * (c1 - c2) / (1 + c2 * c2);
    c3 += w * (c2 - c3) / (1 + c3 * c3);
    c4 += w * (c3 - c4) / (1 + c4 * c4);
    out[i]  = c4*scal;
    c5 += 0.5 * (c4 - c5);
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf2_aa(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, *res = p->res, *freq = p->freq ;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, t;
  int32_t wi;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    t = log2(freq[i]/CBASE) + 10.71;
    wi = (int32_t) (floor(t));
    t -= wi;
    t = ldexp(1 + t * (0.6930 +
                       t * (0.2416 + t * (0.0517 +
                                          t * 0.0137))), wi);
    w = t/CS_ESR;
    if (w < 0.8)
      w *= 1 - 0.4 * w - 0.125 * w * w;
    else {
      w *= 0.6;
      if (w > 0.92) w = 0.92;
    }
    x = -4.5*c5*res[i]
      + in[i]/scal + 1e-10;
    x /= sqrt (1 + x * x);
    c1 += w * (x  - c1) / (1 + c1 * c1);
    c2 += w * (c1 - c2) / (1 + c2 * c2);
    c3 += w * (c2 - c3) / (1 + c3 * c3);
    c4 += w * (c3 - c4) / (1 + c4 * c4);
    out[i]  = c4*scal;
    c5 += 0.5 * (c4 - c5);
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf3(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, res;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, d;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (p->fr != *p->freq) {
    MYFLT fr = log2(*p->freq/CBASE);
    p->fr  = *p->freq;
    w = exp2ap(fr + 9.70)/CS_ESR;
    if (w < 0.75) w *= 1.005 - w * (0.624 - w * (0.65 - w * 0.54));
    else {
      w *= 0.6748;
      if (w > 0.82) w = 0.82;
    }
    p->w = w;
  } else w = p->w;

  res = *p->res;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    x = in[i]/scal - (4.3 - 0.2 * w) * res * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);

    x = in[i]/scal -(4.3 - 0.2 * w) * res * c5;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);
    out[i] = c4*scal;
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf3_ak(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, res, *freq = p->freq;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, t, d;
  int32_t wi;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);
  res = *p->res;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    t = log2(freq[i]/CBASE) + 9.70;
    wi = (int32_t) (floor(t));
    t -= wi;
    t = ldexp(1 + t * (0.6930 +
                       t * (0.2416 + t * (0.0517 +
                                          t * 0.0137))), wi);
    w = t/CS_ESR;
    if (w < 0.75)
      w *= 1.005 - w * (0.624 - w * (0.65 - w * 0.54));
    else {
      w *= 0.6748;
      if (w > 0.82) w = 0.82;
    }


    x = in[i]/scal - (4.3 - 0.2 * w) * res * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);

    x = in[i]/scal -(4.3 - 0.2 * w) * res * c5;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);
    out[i] = c4*scal;
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf3_ka(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, *res  = p->res;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, d;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (p->fr != *p->freq) {
    MYFLT fr = log2(*p->freq/CBASE);
    p->fr  = *p->freq;
    w = exp2ap(fr + 9.70)/CS_ESR;
    if (w < 0.75) w *= 1.005 - w * (0.624 - w * (0.65 - w * 0.54));
    else {
      w *= 0.6748;
      if (w > 0.82) w = 0.82;
    }
    p->w = w;
  } else w = p->w;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    x = in[i]/scal - (4.3 - 0.2 * w) *
      (res[i] > FL(0.0) ?
       (res[i] < FL(1.0) ?
        res[i] : FL(1.0)) : FL(0.0)) * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);

    x = in[i]/scal - (4.3 - 0.2 * w) *
      res[i]  * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);
    out[i] = c4*scal;
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf3_aa(CSOUND *csound, mvclpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, *res = p->res, *freq = p->freq;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, t, x, d;
  int32_t wi;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    t = log2(freq[i]/CBASE) + 9.70;
    wi = (int32_t) (floor(t));
    t -= wi;
    t = ldexp(1 + t * (0.6930 +
                       t * (0.2416 + t * (0.0517 +
                                          t * 0.0137))), wi);
    w = t/CS_ESR;
    if (w < 0.75)
      w *= 1.005 - w * (0.624 - w * (0.65 - w * 0.54));
    else {
      w *= 0.6748;
      if (w > 0.82) w = 0.82;
    }


    x = in[i]/scal - (4.3 - 0.2 * w) *
      (res[i] > FL(0.0) ?
       (res[i] < FL(1.0) ?
        res[i] : FL(1.0)) : FL(0.0)) * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);

    x = in[i]/scal - (4.3 - 0.2 * w) *
      res[i]  * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);
    out[i] = c4*scal;
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

typedef struct _mvcf4 {
  OPDS h;
  MYFLT *out0, *out1, *out2, *out3;
  MYFLT *in, *freq, *res, *skipinit;
  double c1, c2, c3, c4, c5;
  double fr, w;
} mvclpf24_4;


int32_t mvclpf24_4_init(CSOUND *csound, mvclpf24_4 *p){
  IGN(csound);
  if (!*p->skipinit){
    p->c1 = p->c2  = p->c3 =
      p->c4 = p->c5 = FL(0.0);
    p->fr = FL(0.0);
  }
  return OK;
}

int32_t mvclpf24_perf4(CSOUND *csound, mvclpf24_4 *p){
  MYFLT *out0 = p->out0, *out1 = p->out1,
    *out2 = p->out2, *out3 = p->out3;
  MYFLT *in = p->in, res;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, d;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (p->fr != *p->freq) {
    MYFLT fr = log2(*p->freq/CBASE);
    p->fr  = *p->freq;
    w = exp2ap(fr + 9.70)/CS_ESR;
    if (w < 0.75) w *= 1.005 - w * (0.624 - w * (0.65 - w * 0.54));
    else {
      w *= 0.6748;
      if (w > 0.82) w = 0.82;
    }
    p->w = w;
  } else w = p->w;

  res = *p->res > FL(0.0) ?
    (*p->res < FL(1.0) ?
     *p->res : FL(1.0)) : FL(0.0);

  if (UNLIKELY(offset)) {
    memset(out0, '\0', offset*sizeof(MYFLT));
    memset(out1, '\0', offset*sizeof(MYFLT));
    memset(out2, '\0', offset*sizeof(MYFLT));
    memset(out3, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out0[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out3[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    x = in[i]/scal - (4.3 - 0.2 * w) * res * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);

    x = in[i]/scal -(4.3 - 0.2 * w) * res * c5;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);
    out0[i] = c1*scal;
    out1[i] = c2*scal;
    out2[i] = c3*scal;
    out3[i] = c4*scal;
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}


int32_t mvclpf24_perf4_ak(CSOUND *csound, mvclpf24_4 *p){
  MYFLT *out0 = p->out0, *out1 = p->out1,
    *out2 = p->out2, *out3 = p->out3;
  MYFLT *in = p->in, res, *freq = p->freq;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, t, d;
  int32_t wi;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  res = *p->res;

  if (UNLIKELY(offset)) {
    memset(out0, '\0', offset*sizeof(MYFLT));
    memset(out1, '\0', offset*sizeof(MYFLT));
    memset(out2, '\0', offset*sizeof(MYFLT));
    memset(out3, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out0[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out3[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    t = log2(freq[i]/CBASE) + 9.70;
    wi = (int32_t) (floor(t));
    t -= wi;
    t = ldexp(1 + t * (0.6930 +
                       t * (0.2416 + t * (0.0517 +
                                          t * 0.0137))), wi);
    w = t/CS_ESR;
    if (w < 0.75)
      w *= 1.005 - w * (0.624 - w * (0.65 - w * 0.54));
    else {
      w *= 0.6748;
      if (w > 0.82) w = 0.82;
    }
    x = in[i]/scal - (4.3 - 0.2 * w) * res * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);

    x = in[i]/scal -(4.3 - 0.2 * w) * res * c5;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);
    out0[i] = c1*scal;
    out1[i] = c2*scal;
    out2[i] = c3*scal;
    out3[i] = c4*scal;
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf4_ka(CSOUND *csound, mvclpf24_4 *p){
  MYFLT *out0 = p->out0, *out1 = p->out1,
    *out2 = p->out2, *out3 = p->out3;
  MYFLT *in = p->in, *res = p->res;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, d;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (p->fr != *p->freq) {
    MYFLT fr = log2(*p->freq/CBASE);
    p->fr  = *p->freq;
    w = exp2ap(fr + 9.70)/CS_ESR;
    if (w < 0.75) w *= 1.005 - w * (0.624 - w * (0.65 - w * 0.54));
    else {
      w *= 0.6748;
      if (w > 0.82) w = 0.82;
    }
    p->w = w;
  } else w = p->w;

  if (UNLIKELY(offset)) {
    memset(out0, '\0', offset*sizeof(MYFLT));
    memset(out1, '\0', offset*sizeof(MYFLT));
    memset(out2, '\0', offset*sizeof(MYFLT));
    memset(out3, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out0[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out3[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    x = in[i]/scal - (4.3 - 0.2 * w) *
      res[i]  * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);

    x = in[i]/scal - (4.3 - 0.2 * w) *
      (res[i] > FL(0.0) ?
       (res[i] < FL(1.0) ?
        res[i] : FL(1.0)) : FL(0.0)) * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);
    out0[i] = c1*scal;
    out1[i] = c2*scal;
    out2[i] = c3*scal;
    out3[i] = c4*scal;
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

int32_t mvclpf24_perf4_aa(CSOUND *csound, mvclpf24_4 *p){
  MYFLT *out0 = p->out0, *out1 = p->out1,
    *out2 = p->out2, *out3 = p->out3;
  MYFLT *in = p->in, *res = p->res, *freq = p->freq;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x, t, d;
  int32_t wi;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (UNLIKELY(offset)) {
    memset(out0, '\0', offset*sizeof(MYFLT));
    memset(out1, '\0', offset*sizeof(MYFLT));
    memset(out2, '\0', offset*sizeof(MYFLT));
    memset(out3, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out0[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out3[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    t = log2(freq[i]/CBASE) + 9.70;
    wi = (int32_t) (floor(t));
    t -= wi;
    t = ldexp(1 + t * (0.6930 +
                       t * (0.2416 + t * (0.0517 +
                                          t * 0.0137))), wi);
    w = t/CS_ESR;
    if (w < 0.75)
      w *= 1.005 - w * (0.624 - w * (0.65 - w * 0.54));
    else {
      w *= 0.6748;
      if (w > 0.82) w = 0.82;
    }
    x = in[i]/scal - (4.3 - 0.2 * w) *
      res[i]  * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);

    x = in[i]/scal - (4.3 - 0.2 * w) *
      (res[i] > FL(0.0) ?
       (res[i] < FL(1.0) ?
        res[i] : FL(1.0)) : FL(0.0)) * c5 + 1e-10;
    x /= sqrt (1 + x * x);
    d = w * (x  - c1) / (1 + c1 * c1);
    x = c1 + 0.77 * d;
    c1 = x + 0.23 * d;
    d = w * (x  - c2) / (1 + c2 * c2);
    x = c2 + 0.77 * d;
    c2 = x + 0.23 * d;
    d = w * (x  - c3) / (1 + c3 * c3);
    x = c3 + 0.77 * d;
    c3 = x + 0.23 * d;
    d = w * (x  - c4);
    x = c4 + 0.77 * d;
    c4 = x + 0.23 * d;
    c5 += 0.85 * (c4 - c5);
    out0[i] = c1*scal;
    out1[i] = c2*scal;
    out2[i] = c3*scal;
    out3[i] = c4*scal;
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;

  return OK;
}

#define PEAKHCF FL(1.4)

typedef struct _mvcfh {
  OPDS h;
  MYFLT *out;
  MYFLT *in, *freq, *skipinit;
  double c1, c2, c3, c4, c5;
  double fr, w, x;
} mvchpf24;

int32_t mvchpf24_init(CSOUND *csound, mvchpf24 *p){
  IGN(csound);
  if (!*p->skipinit){
    p->c1 = p->c2  = p->c3 =
      p->c4 = p->c5 = p->x = FL(0.0);
    p->fr = FL(0.0);
  }
  return OK;
}

int32_t mvchpf24_perf(CSOUND *csound, mvchpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x = p->x, t, d, y;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (p->fr != *p->freq) {
    MYFLT fr = log2(*p->freq/CBASE);
    p->fr  = *p->freq;
    w = CS_ESR/exp2ap(fr+ 9.2);
    if (w < FL(2.0)) w = FL(2.0);
    p->w = w;
  } else w = p->w;

  if (UNLIKELY(offset)) {
    memset(out, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    x = y = in[i]/scal  - 0.3 * x;
    d = x - c1 + 1e-10;
    t = d * d;
    d *= (1 + t) / (w + t);
    c1 += d;
    x -= c1;
    c1 += d;
    d = x - c2 + 1e-10;
    t = d * d;
    d *= (1 + t) / (w + t);
    c2 += d;
    x -= c2;
    c2 += d;
    d = x - c3 + 1e-10;
    t = d * d;
    d *= (1 + t) / (w + t);
    c3 += d;
    x -= c3;
    c3 += d;
    d = x - c4 + 1e-10;
    t = d * d;
    d *= (1 + t) / (w + t);
    c4 += d;
    x -= c4;
    c4 += d;
    out[i] = scal*x/PEAKHCF;
    x -= y;
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;
  p->x  = x;

  return OK;
}

int32_t mvchpf24_perf_a(CSOUND *csound, mvchpf24 *p){
  MYFLT *out = p->out;
  MYFLT *in = p->in, *freq = p->freq;
  double c1 = p->c1+1e-6, c2 = p->c2, c3 = p->c3,
    c4 = p->c4, c5 = p->c5, w, x = p->x, t, d, y;
  int32_t wi;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT scal = csound->Get0dBFS(csound);

  if (UNLIKELY(offset)) {
    memset(out, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i < nsmps; i++){
    t = log2(freq[i]/CBASE) + 9.70;
    wi = (int32_t) (floor(t));
    t -= wi;
    t = ldexp(1 + t * (0.6930 +
                       t * (0.2416 + t * (0.0517 +
                                          t * 0.0137))), wi);
    w = CS_ESR/t;
    if (w < FL(2.0)) w = FL(2.0);

    x = y = in[i]/scal  - 0.3 * x;
    d = x - c1 + 1e-10;
    t = d * d;
    d *= (1 + t) / (w + t);
    c1 += d;
    x -= c1;
    c1 += d;
    d = x - c2 + 1e-10;
    t = d * d;
    d *= (1 + t) / (w + t);
    c2 += d;
    x -= c2;
    c2 += d;
    d = x - c3 + 1e-10;
    t = d * d;
    d *= (1 + t) / (w + t);
    c3 += d;
    x -= c3;
    c3 += d;
    d = x - c4 + 1e-10;
    t = d * d;
    d *= (1 + t) / (w + t);
    c4 += d;
    x -= c4;
    c4 += d;
    out[i] = scal*x/PEAKHCF;
    x -= y;
  }
  p->c1 = c1;
  p->c2 = c2;
  p->c3 = c3;
  p->c4 = c4;
  p->c5 = c5;
  p->x  = x;

  return OK;
}

/* Bob is a port of bob~ filter object from Pd.
   The design is based on the papers by Tim Stilson,
   Timothy E. Stinchcombe, and Antti Huovilainen.
   Ported from PD code by Gleb Rogozinsky, Summer of 2020
   cased on code by Miller Puckette
*/

static int32_t calc_derivatives(CSOUND *csound, BOB *p, double *dstate,
                                double *state, MYFLT in)
{
  MYFLT  freq = *p->freq;
  MYFLT  res = *p->res;
  double k = TWOPI * freq;
  double sat = *p->sat;
  double satinv = 1.0/sat;

  double satstate0 = sat * tanh(state[0] * satinv);
  double satstate1 = sat * tanh(state[1] * satinv);
  double satstate2 = sat * tanh(state[2] * satinv);

  dstate[0] = k *
    (sat * tanh((in - res * state[3]) * satinv) - satstate0);
  dstate[1] = k * (satstate0 - satstate1);
  dstate[2] = k * (satstate1 - satstate2);
  dstate[3] = k * (satstate2 - (sat * tanh(state[3]*satinv)));

  return OK;
}

static int32_t bob_init(CSOUND *csound,BOB *p)
{
  IGN(csound);
  if (*p->istor==FL(0.0)) {
    p->oldfreq = FL(0.0);
    p->oldres = FL(0.0);
    p->oldsat = FL(0.0);
  }

  if (*p->osamp<=FL(0.0)) p->ostimes = 2;
  else if (*p->osamp< FL(1.0)) p->ostimes = 1;
  else p->ostimes = (int32_t) *p->osamp;

  int32_t i;
  for (i = 0; i < DIM; i++) {
    p->state[i] = 0;
  }

  return OK;
}

static int32_t bob_process(CSOUND *csound,BOB *p)
{
  MYFLT  *out = p->out;
  MYFLT  *in = p->in;
  MYFLT  *freq = p->freq;
  MYFLT  *res  = p->res;
  MYFLT  *sat  = p->sat;

  int32_t ostimes = p->ostimes,j;
  double stepsize = 1./(ostimes * CS_ESR);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i,l, nsmps = CS_KSMPS;
  int32_t      asgfr = IS_ASIG_ARG(p->freq),
    asgrs = IS_ASIG_ARG(p->res), asgsa = IS_ASIG_ARG(p->sat);
  double deriv1[DIM], deriv2[DIM], deriv3[DIM], deriv4[DIM],
    tempstate[DIM];

  if (UNLIKELY(offset)) {
    memset(out, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    MYFLT fr = (asgfr ? freq[i] : *freq);
    MYFLT rs = (asgrs ? res[i] : *res);
    MYFLT sa = (asgsa ? sat[i] : *sat);
    *p->freq = fr;
    *p->res = rs;
    *p->sat = sa;

    if (p->oldfreq != fr|| p->oldres != rs || p->oldsat != sa) {
      p->oldfreq = fr;
      p->oldres = rs;
      p->oldsat = sa;
    }

    for (j=0; j<ostimes; j++) {
      //solver_rungekutte
      calc_derivatives(csound,p, deriv1, tempstate, in[i]);
      for (l = 0; l < DIM; l++)
        tempstate[l] = p->state[l] + 0.5 * stepsize * deriv1[l];
      calc_derivatives(csound,p, deriv2, tempstate, in[i]);
      for (l = 0; l < DIM; l++)
        tempstate[l] = p->state[l] + 0.5 * stepsize * deriv2[l];
      calc_derivatives(csound,p, deriv3, tempstate, in[i]);
      for (l = 0; l < DIM; l++)
        tempstate[l] = p->state[l] + 0.5 * stepsize * deriv3[l];
      calc_derivatives(csound,p, deriv4, tempstate, in[i]);
      for (l = 0; l < DIM; l++)
        p->state[l] += (1./6.) * stepsize *
          (deriv1[l] + 2 * deriv2[l] + 2 * deriv3[l] +
           deriv4[l]);
    }
    out[i] = p->state[0];
  }
  return OK;
}

typedef struct vps {
  OPDS h;
  MYFLT *out, *in, *kd, *ke;
} VPS;

int32_t vps_process(CSOUND *csound, VPS *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT *in = p->in, s;
  MYFLT *out = p->out;
  MYFLT kd = *p->kd < 1. ? (*p->kd >= 0. ? *p->kd : 0.) : 1.;
  MYFLT ke = *p->ke;

  if (UNLIKELY(offset)) {
    memset(out, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }


  for (i=offset; i<nsmps; i++) {
    s = in[i];
    s = s < 1. ? (s >= 0. ? s : 0.) : 1.;
    if (s < kd)
      out[i] = s*ke/kd;
    else
      out[i] = ke + (1.-ke)*(s-kd)/(1.-kd);
  }
  return OK;
}

typedef struct vcfnl {
  OPDS h;
  MYFLT *y, *y1, *x, *f, *r, *kn, *istor;
  double s[4];
  double A, G[4];
  MYFLT ff;
  double piosr;
  MYFLT max;
  MYFLT *tab;
  size_t size;
} VCFNL;

int32_t vcfnl_init(CSOUND *csound, VCFNL *p) {
  MYFLT *tab;
  double g, *G = p->G;
  p->piosr = CS_PIDSR;
  p->ff = *p->f;
  g = TAN(p->ff*p->piosr);
  G[0] = g/(1+g);
  p->A = (g-1)/(1+g);
  G[1] = G[0]*G[0]; // G^2
  G[2] = G[0]*G[1]; // G^3
  G[3] = G[0]*G[2]; // G^4
  if(*p->istor == 0) memset(p->s, 0, 4*sizeof(MYFLT));
  tab = csound->QueryGlobalVariable(csound, "::TANH::");
  if(tab == NULL) {
    int32_t i;
    csound->CreateGlobalVariable(csound,"::TANH::",sizeof(MYFLT)*(TABSIZE+1));
    tab =  csound->QueryGlobalVariable(csound, "::TANH::");
    MYFLT step  = 8./TABSIZE, x = -4.;
    for(i=0; i <= TABSIZE; x += step, i++)
      tab[i] = TANH(x);
  }
  tab[TABSIZE] = tab[TABSIZE-1];
  p->max = .125;
  p->tab = tab;
  p->size = TABSIZE;
  return OK;
}

int32_t vcfnl_perfk(CSOUND *csound, VCFNL *p) {
  double *G = p->G, A = p->A, *s = p->s, ss;
  MYFLT *y = p->y, *x = p->x, *y1 = p->y1,
    kn = (*p->kn > 0 ? *p->kn : 0)+FL(1.0), kno1;
  double w, u, o;
  MYFLT k = *p->r * FL(4.0), max = p->max, *tab = p->tab;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, j, nsmps = CS_KSMPS;
  size_t size = p->size;
  kno1 = FL(1.0)/kn;
  
  if(*p->f != p->ff) {
    MYFLT g;
    p->ff = *p->f;
    g = TAN(p->ff*p->piosr);
    G[0] = g/(1+g);
    p->A = A = (g-1)/(1+g);
    G[1] = G[0]*G[0];
    G[2] = G[0]*G[1];
    G[3] = G[0]*G[2];
  }
  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }
 
  for (i=offset; i<nsmps; i++) {
    ss = s[3];
    for(j = 0; j < 3; j++) ss += s[j]*G[2-j];
    o = (G[3]*x[i] + ss)/(1. + k*G[3]);
    u = G[0]*nlf(tab,(x[i] - k*o)*kn,max,size)*kno1;
    for(j = 0; j < 3; j++) {
      w = u + s[j];
      s[j] = u - A*w;
      u = G[0]*nlf(tab,w*kn,max,size)*kno1;
      if(j == 1) y1[i] = s[1];
    }
    s[3] = G[0]*w - A*o;
    y[i] = o;
  }
  return OK;
}

int32_t vcfnl_perfak(CSOUND *csound, VCFNL *p) {
  double *G = p->G, A = p->A, *s = p->s, ss, g;
  MYFLT *y = p->y, *x = p->x, *y1 = p->y1, *f = p->f,
    kn = (*p->kn > 0 ? *p->kn : 0)+FL(1.0), kno1;
  double w, u, o, piosr = p->piosr;
  MYFLT k = *p->r * FL(4.0), max = p->max, *tab = p->tab;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, j, nsmps = CS_KSMPS;
  size_t size = p->size;
  kno1 = FL(1.0)/kn;
  
  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }
 
  for (i=offset; i<nsmps; i++) {
    g = TAN(f[i]*piosr);
    G[0] = g/(1+g);
    A = (g-1)/(1+g);
    G[1] = G[0]*G[0];
    G[2] = G[0]*G[1];
    G[3] = G[0]*G[2];
    ss = s[3];
    for(j = 0; j < 3; j++) ss += s[j]*G[2-j];
    o = (G[3]*x[i] + ss)/(1. + k*G[3]);
    u = G[0]*nlf(tab,(x[i] - k*o)*kn,max,size)*kno1;
    for(j = 0; j < 3; j++) {
      w = u + s[j];
      s[j] = u - A*w;
      u = G[0]*nlf(tab,w*kn,max,size)*kno1;
      if(j == 1) y1[i] = s[1];
    }
    s[3] = G[0]*w - A*o;
    y[i] = o;
  }
  return OK;
}

int32_t vcfnl_perfka(CSOUND *csound, VCFNL *p) {
  double *G = p->G, A = p->A, *s = p->s, ss;
  MYFLT *y = p->y, *x = p->x, *y1 = p->y1, *r = p->r,
    kn = (*p->kn > 0 ? *p->kn : 0)+FL(1.0), kno1;
  double w, u, o, piosr = p->piosr;
  MYFLT max = p->max, *tab = p->tab, k;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, j, nsmps = CS_KSMPS;
  size_t size = p->size;
  kno1 = FL(1.0)/kn;
  
  if(*p->f != p->ff) {
    MYFLT g;
    p->ff = *p->f;
    g = TAN(p->ff*piosr);
    G[0] = g/(1+g);
    p->A = A = (g-1)/(1+g);
    G[1] = G[0]*G[0];
    G[2] = G[0]*G[1];
    G[3] = G[0]*G[2];
  }
  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }
 
  for (i=offset; i<nsmps; i++) {
    k = r[i]*FL(4.0);
    ss = s[3];
    for(j = 0; j < 3; j++) ss += s[j]*G[2-j];
    o = (G[3]*x[i] + ss)/(1. + k*G[3]);
    u = G[0]*nlf(tab,(x[i] - k*o)*kn,max,size)*kno1;
    for(j = 0; j < 3; j++) {
      w = u + s[j];
      s[j] = u - A*w;
      u = G[0]*nlf(tab,w*kn,max,size)*kno1;
      if(j == 1) y1[i] = s[1];
    }
    s[3] = G[0]*w - A*o;
    y[i] = o;
  }
  return OK;
}

int32_t vcfnl_perfaa(CSOUND *csound, VCFNL *p) {
  double *G = p->G, A = p->A, *s = p->s, ss, g;
  MYFLT *y = p->y, *x = p->x, *y1 = p->y1, *f = p->f, *r = p->r,
    kn = (*p->kn > 0 ? *p->kn : 0)+FL(1.0), kno1;
  double w, u, o, piosr = p->piosr;
  MYFLT k, max = p->max, *tab = p->tab;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, j, nsmps = CS_KSMPS;
  size_t size = p->size;
  kno1 = FL(1.0)/kn;
  
  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }
 
  for (i=offset; i<nsmps; i++) {
    g = TAN(f[i]*piosr);
    G[0] = g/(1+g);
    A = (g-1)/(1+g);
    G[1] = G[0]*G[0];
    G[2] = G[0]*G[1];
    G[3] = G[0]*G[2];
    k = r[i]*FL(4.0);
    ss = s[3];
    for(j = 0; j < 3; j++) ss += s[j]*G[2-j];
    o = (G[3]*x[i] + ss)/(1. + k*G[3]);
    u = G[0]*nlf(tab,(x[i] - k*o)*kn,max,size)*kno1;
    for(j = 0; j < 3; j++) {
      w = u + s[j];
      s[j] = u - A*w;
      u = G[0]*nlf(tab,w*kn,max,size)*kno1;
      if(j == 1) y1[i] = s[1];
    }
    s[3] = G[0]*w - A*o;
    y[i] = o;
  }
  return OK;
}



typedef struct vcf {
  OPDS h;
  MYFLT *y, *x, *f, *r, *istor;
  double s[4];
  double A, G[4];
  MYFLT ff;
  double piosr;
} VCF;

int32_t vcf_init(CSOUND *csound, VCFNL *p) {
  double g, *G = p->G;
  p->piosr = PI/CS_ESR;
  p->ff = *p->f;
  g = TAN(p->ff*p->piosr);
  G[0] = g/(1+g);
  p->A = (g-1)/(1+g);
  G[1] = G[0]*G[0]; // G^2
  G[2] = G[0]*G[1]; // G^3
  G[3] = G[0]*G[2]; // G^4
  if(*p->istor == 0)
    memset(p->s, 0, 4*sizeof(MYFLT));
  return OK;
}

int32_t vcf_perfk(CSOUND *csound, VCF *p) {
  double *G = p->G, A = p->A, *s = p->s, ss;
  MYFLT *y = p->y, *x = p->x;
  double w, u, o;
  MYFLT k = *p->r <=  1 ? (*p->r >= 0 ? *p->r*4 : 0)  : 4;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, j, nsmps = CS_KSMPS;
  if(*p->f != p->ff) {
    MYFLT g;
    p->ff = *p->f;
    g = TAN(p->ff*p->piosr);
    G[0] = g/(1+g);
    p->A = A = (g-1)/(1+g);
    G[1] = G[0]*G[0];
    G[2] = G[0]*G[1];
    G[3] = G[0]*G[2];
  }

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }
 
  for (i=offset; i<nsmps; i++) {
    ss = s[3];
    for(j = 0; j < 3; j++) ss += s[j]*G[2-j];
    o = (G[3]*x[i] + ss)/(1 + k*G[3]);
    u = G[0]*(x[i] - k*o);
    for(j = 0; j < 3; j++) {
      w = u + s[j];
      s[j] = u - A*w;
      u = G[0]*w;
    }
    s[3] = G[0]*w - A*o;
    y[i] = o;
  }
  return OK;
}

int32_t vcf_perfak(CSOUND *csound, VCF *p) {
  double *G = p->G, A, *s = p->s, ss, g;
  MYFLT *y = p->y, *x = p->x, *f = p->f;
  double w, u, o;
  MYFLT k = *p->r <=  1 ? (*p->r >= 0 ? *p->r*4 : 0)  : 4;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, j, nsmps = CS_KSMPS;
  MYFLT piosr = p->piosr;

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    g = TAN(f[i]*piosr);
    G[0] = g/(1+g);
    A = (g-1)/(1+g);
    G[1] = G[0]*G[0];
    G[2] = G[0]*G[1];
    G[3] = G[0]*G[2];
    ss = s[3];
    for(j = 0; j < 3; j++) ss += s[j]*G[2-j];
    o = (G[3]*x[i] + ss)/(1 + k*G[3]);
    u = G[0]*(x[i] - k*o);
    for(j = 0; j < 3; j++) {
      w = u + s[j];
      s[j] = u - A*w;
      u = G[0]*w;
    }
    s[3] = G[0]*w - A*o;
    y[i] = o;
  }
  return OK;
}


int32_t vcf_perfka(CSOUND *csound, VCF *p) {
  double *G = p->G, A = p->A, *s = p->s, ss;
  MYFLT *y = p->y, *x = p->x;
  double w, u, o;
  MYFLT *r = p->r, k;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, j, nsmps = CS_KSMPS;
  if(*p->f != p->ff) {
    MYFLT g;
    p->ff = *p->f;
    g = TAN(p->ff*p->piosr);
    G[0] = g/(1+g);
    p->A = A = (g-1)/(1+g);
    G[1] = G[0]*G[0];
    G[2] = G[0]*G[1];
    G[3] = G[0]*G[2];
  }

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    k = r[i] <=  1 ? (r[i] >= 0 ? r[i]*4 : 0)  : 4;
    ss = s[3];
    for(j = 0; j < 3; j++) ss += s[j]*G[2-j];
    o = (G[3]*x[i] + ss)/(1 + k*G[3]);
    u = G[0]*(x[i] - k*o);
    for(j = 0; j < 3; j++) {
      w = u + s[j];
      s[j] = u - A*w;
      u = G[0]*w;
    }
    s[3] = G[0]*w - A*o;
    y[i] = o;
  }
  return OK;
}

int32_t vcf_perfaa(CSOUND *csound, VCF *p) {
  double *G = p->G, A, *s = p->s, ss, g;
  MYFLT *y = p->y, *x = p->x, *f = p->f;
  double w, u, o;
  MYFLT *r = p->r, k;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, j, nsmps = CS_KSMPS;
  MYFLT piosr = p->piosr;

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    k = r[i] <=  1 ? (r[i] >= 0 ? r[i]*4 : 0)  : 4;
    g = TAN(f[i]*piosr);
    G[0] = g/(1+g);
    A = (g-1)/(1+g);
    G[1] = G[0]*G[0];
    G[2] = G[0]*G[1];
    G[3] = G[0]*G[2];
    ss = s[3];
    for(j = 0; j < 3; j++) ss += s[j]*G[2-j];
    o = (G[3]*x[i] + ss)/(1 + k*G[3]);
    u = G[0]*(x[i] - k*o);
    for(j = 0; j < 3; j++) {
      w = u + s[j];
      s[j] = u - A*w;
      u = G[0]*w;
    }
    s[3] = G[0]*w - A*o;
    y[i] = o;
  }
  return OK;
}

typedef struct _spf {
  OPDS h;
  MYFLT *y,*xl,*xh,*xb,*f,*r, *istor;
  MYFLT ff, R;
  double s[2],sl[2],sh[2],sb[2];
  double al[2],ah[2],ab,b[2];
  double piosr;
} SPF;


int32_t spf_init(CSOUND *csound, SPF *p) {
  double w, w2, fac;
  double *sh = p->sh, *sl = p->sl, *sb = p->sb, *s = p->s;
  p->piosr = PI/CS_ESR;
  w = TAN(*p->f*p->piosr);
  w2 = w*w;
  p->R = *p->r >  0 ? (*p->r <= 2. ? *p->r : 2.) : 0.;
  fac = 1./(1. + p->R*w + w2);
  p->al[0] = w2*fac;
  p->al[1] = 2*w2*fac;
  p->ah[0] = fac;
  p->ah[1] = -2*fac;
  p->ab = w*fac*p->R;
  p->b[0] = -2*(1 - w2)*fac;
  p->b[1] = (1. - p->R*w + w2)*fac;
  p->ff = *p->f;
  if(*p->istor == FL(0.0))
    s[0] = s[1] = sl[0] = sl[1] = sh[0] = sh[1] = sb[0] = sb[1] = 0.f;
  return OK;
}

int32_t spf_perfkk(CSOUND *csound, SPF *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  double *b = p->b,*al = p->al,*ah = p->ah, ab = p->ab;
  MYFLT *xl = p->xl, *xh = p->xh, *xb = p->xb, *y = p->y;
  double x;
  double *sh = p->sh, *sl = p->sl, *sb = p->sb, *s = p->s;

  if(p->ff != *p->f ||
     p->R  != *p->r) {
    double w, w2, fac;
    p->R = *p->r >  0 ? (*p->r <= 2. ? *p->r : 2.) : 0.;
    w = TAN(*p->f*p->piosr);
    w2 = w*w;
    fac = 1./(1. + p->R*w + w2);
    al[0] = w2*fac;
    al[1] = 2*w2*fac;
    ah[0] = fac;
    ah[1] = -2*fac;
    ab = w*fac*p->R;
    b[0] = -2*(1 - w2)*fac;
    b[1] = (1. - p->R*w + w2)*fac;
    p->ff = *p->f;
    p->ab = ab;
  }

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    x = xh[i]*ah[0] + sh[0]*ah[1] + sh[1]*ah[0];
    sh[1] = sh[0];
    sh[0] = xh[i];
    x += xl[i]*al[0] + sl[0]*al[1] + sl[1]*al[0];
    sl[1] = sl[0];
    sl[0] = xl[i];
    x += (xb[i] - sb[1])*ab;
    sb[1] = sb[0];
    sb[0] = xb[i];
    y[i] = x - b[0]*s[0] - b[1]*s[1];
    s[1] = s[0];
    s[0] = y[i];
  }
  return OK;

}


int32_t spf_perfak(CSOUND *csound, SPF *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  double *b = p->b,*al = p->al,*ah = p->ah, ab;
  MYFLT *xl = p->xl, *xh = p->xh, *xb = p->xb, *y = p->y;
  double x;
  double *sh = p->sh, *sl = p->sl, *sb = p->sb, *s = p->s;
  double w, w2, fac, R;
  MYFLT *f = p->f;

  R = *p->r >  0 ? (*p->r <= 2. ? *p->r : 2.) : 0.;

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    w = TAN(f[i]*p->piosr);
    w2 = w*w;
    fac = 1./(1. + R*w + w2);
    al[0] = w2*fac;
    al[1] = 2*w2*fac;
    ah[0] = fac;
    ah[1] = -2*fac;
    ab = w*fac*R;
    b[0] = -2*(1 - w2)*fac;
    b[1] = (1. - R*w + w2)*fac;
    x = xh[i]*ah[0] + sh[0]*ah[1] + sh[1]*ah[0];
    sh[1] = sh[0];
    sh[0] = xh[i];
    x += xl[i]*al[0] + sl[0]*al[1] + sl[1]*al[0];
    sl[1] = sl[0];
    sl[0] = xl[i];
    x += (xb[i] - sb[1])*ab;
    sb[1] = sb[0];
    sb[0] = xb[i];
    y[i] = x - b[0]*s[0] - b[1]*s[1];
    s[1] = s[0];
    s[0] = y[i];
  }
  return OK;
}

int32_t spf_perfaa(CSOUND *csound, SPF *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  double *b = p->b,*al = p->al,*ah = p->ah, ab;
  MYFLT *xl = p->xl, *xh = p->xh, *xb = p->xb, *y = p->y;
  double x;
  double *sh = p->sh, *sl = p->sl, *sb = p->sb, *s = p->s;
  double w, w2, fac, R;
  MYFLT *r = p->r, *f = p->f;

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    w = TAN(f[i]*p->piosr);
    w2 = w*w;
    R = r[i] > 0 ? (r[i] <= 2. ? r[i] : 2.) : 0.;
    fac = 1./(1. + R*w + w2);
    al[0] = w2*fac;
    al[1] = 2*w2*fac;
    ah[0] = fac;
    ah[1] = -2*fac;
    ab = w*fac*R;
    b[0] = -2*(1 - w2)*fac;
    b[1] = (1. - R*w + w2)*fac;
    x = xh[i]*ah[0] + sh[0]*ah[1] + sh[1]*ah[0];
    sh[1] = sh[0];
    sh[0] = xh[i];
    x += xl[i]*al[0] + sl[0]*al[1] + sl[1]*al[0];
    sl[1] = sl[0];
    sl[0] = xl[i];
    x += (xb[i] - sb[1])*ab;
    sb[1] = sb[0];
    sb[0] = xb[i];
    y[i] = x - b[0]*s[0] - b[1]*s[1];
    s[1] = s[0];
    s[0] = y[i];
  }
  return OK;
}

int32_t spf_perfka(CSOUND *csound, SPF *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  double *b = p->b,*al = p->al,*ah = p->ah, ab;
  MYFLT *xl = p->xl, *xh = p->xh, *xb = p->xb, *y = p->y;
  double x;
  double *sh = p->sh, *sl = p->sl, *sb = p->sb, *s = p->s;
  double w, w2, fac, R;
  MYFLT *r = p->r;

  w = TAN(*p->f*p->piosr);
  w2 = w*w;

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    R = r[i] > 0 ? (r[i] <= 2. ? r[i] : 2.) : 0.;
    fac = 1./(1. + R*w + w2);
    al[0] = w2*fac;
    al[1] = 2*w2*fac;
    ah[0] = fac;
    ah[1] = -2*fac;
    ab = w*fac*R;
    b[0] = -2*(1 - w2)*fac;
    b[1] = (1. - R*w + w2)*fac;
    x = xh[i]*ah[0] + sh[0]*ah[1] + sh[1]*ah[0];
    sh[1] = sh[0];
    sh[0] = xh[i];
    x += xl[i]*al[0] + sl[0]*al[1] + sl[1]*al[0];
    sl[1] = sl[0];
    sl[0] = xl[i];
    x += (xb[i] - sb[1])*ab;
    sb[1] = sb[0];
    sb[0] = xb[i];
    y[i] = x - b[0]*s[0] - b[1]*s[1];
    s[1] = s[0];
    s[0] = y[i];
  }
  return OK;
}

typedef struct _skf {
  OPDS h;
  MYFLT *y,*x,*f,*K,*ihp,*istor;
  MYFLT ff, R, KK;
  double s[2];
  double a[2],b[2];
  double piosr;
} SKF;

int32_t skf_init(CSOUND *csound, SKF *p) {
  double w, w2, fac;
  double *s = p->s;
  p->piosr = PI/CS_ESR;
  w = TAN(*p->f*p->piosr);
  w2 = w*w;
  p->KK = (*p->K > 1 ? (*p->K <= 3. ? *p->K : 3.) : 1.);
  p->R = 3 - p->KK;
  fac = 1./(1. + p->R*w + w2);
  if(*p->ihp) {
    p->a[0] = fac;
    p->a[1] = -2*fac;
  }
  else {
    p->a[0] = w2*fac;
    p->a[1] = 2*w2*fac;
  }
  p->b[0] = -2*(1 - w2)*fac;
  p->b[1] = (1. - p->R*w + w2)*fac;
  p->ff = *p->f;
  if(*p->istor == FL(0.0))
    s[0] = s[1] = 0.f;
  return OK;
}

int32_t skf_perfkk(CSOUND *csound, SKF *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  double *b = p->b,*a = p->a;
  MYFLT *x = p->x, *y = p->y;
  double yy;
  double *s = p->s;

  if(p->ff != *p->f ||
     p->KK != *p->K) {
    double w, w2, fac;
    p->KK = (*p->K > 1 ? (*p->K <= 3. ? *p->K : 3.) : 1.);
    p->R = 3 - p->KK;
    w = TAN(*p->f*p->piosr);
    w2 = w*w;
    fac = 1./(1. + p->R*w + w2);
    if(*p->ihp) {
      a[0] = fac;
      a[1] = -2*fac;
    } else {
      a[0] = w2*fac;
      a[1] = 2*w2*fac;
    }
    b[0] = -2*(1 - w2)*fac;
    b[1] = (1. - p->R*w + w2)*fac;
    p->ff = *p->f;
  }

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    yy = x[i] - b[0]*s[0] - b[1]*s[1];
    y[i] = a[0]*yy + a[1]*s[0] + a[0]*s[1];
    s[1] = s[0];
    s[0] = yy;
  }
  return OK;
}

int32_t skf_perfak(CSOUND *csound, SKF *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  double *b = p->b,*a = p->a;
  MYFLT *x = p->x, *y = p->y, *f = p->f;
  double yy, R;
  double *s = p->s;
  R = 3 - (*p->K > 1 ? (*p->K <= 3. ? *p->K : 3.) : 1.);

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    double w, w2, fac;
    w = TAN(f[i]*p->piosr);
    w2 = w*w;
    fac = 1./(1. + R*w + w2);
    if(*p->ihp) {
      a[0] = fac;
      a[1] = -2*fac;
    } else {
      a[0] = w2*fac;
      a[1] = 2*w2*fac;
    }
    b[0] = -2*(1 - w2)*fac;
    b[1] = (1. - R*w + w2)*fac;
    yy = x[i] - b[0]*s[0] - b[1]*s[1];
    y[i] = a[0]*yy + a[1]*s[0] + a[0]*s[1];
    s[1] = s[0];
    s[0] = yy;
  }
  return OK;
}

int32_t skf_perfaa(CSOUND *csound, SKF *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  double *b = p->b,*a = p->a;
  MYFLT *x = p->x, *y = p->y, *f = p->f;
  double yy, R;
  MYFLT *K = p->K;
  double *s = p->s;

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    double w, w2, fac;
    R = 3 - (K[i] > 1 ? (K[i] <= 3. ? K[i] : 3.) : 1.);
    w = TAN(f[i]*p->piosr);
    w2 = w*w;
    fac = 1./(1. + R*w + w2);
    if(*p->ihp) {
      a[0] = fac;
      a[1] = -2*fac;
    } else {
      a[0] = w2*fac;
      a[1] = 2*w2*fac;
    }
    b[0] = -2*(1 - w2)*fac;
    b[1] = (1. - R*w + w2)*fac;
    yy = x[i] - b[0]*s[0] - b[1]*s[1];
    y[i] = a[0]*yy + a[1]*s[0] + a[0]*s[1];
    s[1] = s[0];
    s[0] = yy;
  }
  return OK;
}

int32_t skf_perfka(CSOUND *csound, SKF *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  double *b = p->b,*a = p->a;
  MYFLT *x = p->x, *y = p->y;
  double yy, R;
  MYFLT *K = p->K;
  double *s = p->s;
  double w, w2, fac;
  w = TAN(*p->f*p->piosr);
  w2 = w*w;

  if (UNLIKELY(offset)) {
    memset(y, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&y[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=offset; i<nsmps; i++) {
    R = 3 - (K[i] > 1 ? (K[i] <= 3. ? K[i] : 3.) : 1.);
    fac = 1./(1. + R*w + w2);
    if(*p->ihp) {
      a[0] = fac;
      a[1] = -2*fac;
    } else {
      a[0] = w2*fac;
      a[1] = 2*w2*fac;
    }
    b[0] = -2*(1 - w2)*fac;
    b[1] = (1. - R*w + w2)*fac;
    yy = x[i] - b[0]*s[0] - b[1]*s[1];
    y[i] = a[0]*yy + a[1]*s[0] + a[0]*s[1];
    s[1] = s[0];
    s[0] = yy;
  }
  return OK;
}


typedef struct _svn {
  OPDS h;
  MYFLT *yh,*yl,*yb,*yr,*x,*f,*q,*kn,*ifn,*inm,*mx,*istor;
  MYFLT ff, Q;
  double fac, w;
  double s[2];
  double piosr;
  MYFLT *tab, max;
  int32_t size;
} SVN;



int32_t svn_init(CSOUND *csound, SVN *p) {
  double w2;
  double *s = p->s;
  p->piosr = PI/CS_ESR;
  p->w = TAN(*p->f*p->piosr);
  w2 = p->w*p->w;
  p->Q = *p->q >  0.5 ? *p->q : 0.5;
  p->fac = 1./(1. + p->w/p->Q + w2);
  p->ff = *p->f;
  if(*p->istor == FL(0.0)) s[0] = s[1]  = 0.f;
  if(*p->ifn == FL(0.0)) {
    MYFLT *tab;
    tab = csound->QueryGlobalVariable(csound, "::TANH::");
    if(tab == NULL) {
      int32_t i;
      csound->CreateGlobalVariable(csound,"::TANH::",sizeof(MYFLT)*(TABSIZE+1));
      tab =  csound->QueryGlobalVariable(csound, "::TANH::");
      MYFLT step  = 8./TABSIZE, x = -4.;
      for(i=0; i <= TABSIZE; x += step, i++)
        tab[i] = TANH(x);
    }
    tab[TABSIZE] = tab[TABSIZE-1];
    p->max = .125;
    p->tab = tab;
    p->size = TABSIZE;
  } else {
    FUNC *ftab = csound->FTFind(csound, p->ifn);
    p->tab = ftab->ftable;
    p->size = ftab->flen;
    p->max = 1./(*p->mx*2);
  }
  return OK;
}

int32_t svn_perfkk(CSOUND *csound, SVN *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT *yl = p->yl, *yh = p->yh, *yb = p->yb, *yr = p->yr;
  MYFLT *x = p->x , kn = *p->kn, kno1;
  double u, w = p->w, fac = p->fac, Q = p->Q, D;
  double *s = p->s;
  MYFLT *tab = p->tab, *tn = NULL;
  double max = p->max, mx = *p->mx;
  int32_t size = p->size, sz = 0;
  FUNC *ftab = csound->FTFind(csound, p->inm);
  double scal = csound->Get0dBFS(csound), iscal;
  iscal = 1./scal;
  D = 1./Q;

  if(p->ff != *p->f ||
     p->Q  != *p->q) {
    w = p->w = TAN(*p->f*p->piosr);
    Q = p->Q = *p->q >  0.5 ? *p->q : 0.5;
    D = 1./Q;
    fac = p->fac = 1./(1. + w*D + w*w);
    p->ff = *p->f;
  }

  if (UNLIKELY(offset)) {
    memset(yl, '\0', offset*sizeof(MYFLT));
    memset(yh, '\0', offset*sizeof(MYFLT));
    memset(yb, '\0', offset*sizeof(MYFLT));
    memset(yr, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&yl[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yr[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yh[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yb[nsmps], '\0', early*sizeof(MYFLT));
  }


  if(kn > 0.) {
    if(ftab != NULL) {
      tn = ftab->ftable;
      sz = ftab->flen;
      if(kn > mx) kn = mx;
    } else kn /= max;
    kno1 = 1./kn;
    for (i=offset; i<nsmps; i++) {
      u = x[i]*iscal;
      yh[i] = (u - (D + w) * s[0] - s[1])*fac;
      u = w * nlf(tab,yh[i]*kn,max,size)*(tn ? tn[(int)(sz*kn/mx)] : kno1);
      yb[i] = u + s[0];
      s[0] = yb[i] + u;
      u = w * nlf(tab,yb[i]*kn,max,size)*(tn ? tn[(int)(sz*kn/mx)] : kno1);
      yl[i] = u + s[1];
      s[1] =  yl[i] + u;
      yr[i] = (yh[i] + yl[i])*scal;
      yl[i] *= scal;
      yb[i] *= scal;
      yh[i] *= scal;
    }
  }
  else {
    for (i=offset; i<nsmps; i++) {
      u = x[i]*iscal;
      yh[i] = (u - (D + w) * s[0] - s[1])*fac;
      u = w * yh[i];
      yb[i] = u + s[0];
      s[0] = yb[i] + u;
      u = w * yb[i];
      yl[i] = u + s[1];
      s[1] =  yl[i] + u;
      yr[i] = (yh[i] + yl[i])*scal;
      yl[i] *= scal;
      yb[i] *= scal;
      yh[i] *= scal;
    }
  }
  return OK;
}

int32_t svn_perfak(CSOUND *csound, SVN *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT *yl = p->yl, *yh = p->yh, *yb = p->yb, *yr = p->yr;
  MYFLT *x = p->x , kn = *p->kn > 0 ? *p->kn : .0001, kno1, *f = p->f;
  double u, w, fac, Q = p->Q, D;
  double *s = p->s;
  MYFLT *tab = p->tab, *tn = NULL;
  double max = p->max, mx = *p->mx;
  int32_t size = p->size, sz = 0;
  FUNC *ftab = csound->FTFind(csound, p->inm);
  double scal = csound->Get0dBFS(csound), iscal;
  iscal = 1./scal;
  Q = p->Q = *p->q >  0.5 ? *p->q : 0.5;
  D = 1./Q;

  if (UNLIKELY(offset)) {
    memset(yl, '\0', offset*sizeof(MYFLT));
    memset(yh, '\0', offset*sizeof(MYFLT));
    memset(yb, '\0', offset*sizeof(MYFLT));
    memset(yr, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&yl[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yr[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yh[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yb[nsmps], '\0', early*sizeof(MYFLT));
  }

  if(kn > 0.) {
    if(ftab != NULL) {
      tn = ftab->ftable;
      sz = ftab->flen;
      if(kn > mx) kn = mx;
    }
    if (kn < 0) kn = 0.;
    kno1 = 1./kn;
    for (i=offset; i<nsmps; i++) {
      w = TAN(f[i]*p->piosr);
      fac = 1./(1. + w*D + w*w);
      u = x[i]*iscal;
      yh[i] = (u - (D + w) * s[0] - s[1])*fac;
      u = w * nlf(tab,yh[i]*kn,max,size)*(tn ? tn[(int)(sz*kn/mx)] : kno1);
      yb[i] = u + s[0];
      s[0] = yb[i] + u;
      u = w * nlf(tab,yb[i]*kn,max,size)*(tn ? tn[(int)(sz*kn/mx)] : kno1);
      yl[i] = u + s[1];
      s[1] =  yl[i] + u;
      yr[i] = (yh[i] + yl[i])*scal;
      yl[i] *= scal;
      yb[i] *= scal;
      yh[i] *= scal;
    }
  } else {
    for (i=offset; i<nsmps; i++) {
      w = TAN(f[i]*p->piosr);
      fac = 1./(1. + w*D + w*w);
      u = x[i]*iscal;
      yh[i] = (u - (D + w) * s[0] - s[1])*fac;
      u = w * yh[i];
      yb[i] = u + s[0];
      s[0] = yb[i] + u;
      u = w * yb[i];
      yl[i] = u + s[1];
      s[1] =  yl[i] + u;
      yr[i] = (yh[i] + yl[i])*scal;
      yl[i] *= scal;
      yb[i] *= scal;
      yh[i] *= scal;
    }
  }
  return OK;
}

int32_t svn_perfka(CSOUND *csound, SVN *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT *yl = p->yl, *yh = p->yh, *yb = p->yb, *yr = p->yr;
  MYFLT *x = p->x,  kn = *p->kn > 0 ? *p->kn : .0001, kno1, *q = p->q;
  double u, w = p->w, w2, fac = p->fac, D;
  double *s = p->s;
  MYFLT *tab = p->tab, *tn = NULL;
  double max = p->max, mx = *p->mx;
  int32_t size = p->size, sz = 0;
  FUNC *ftab = csound->FTFind(csound, p->inm);
  double scal = csound->Get0dBFS(csound), iscal;
  iscal = 1./scal;
  w2 = w*w;

  if(p->ff != *p->f) {
    w = p->w = TAN(*p->f*p->piosr);
    w2 = w*w;
    p->ff = *p->f;
  }

  if (UNLIKELY(offset)) {
    memset(yl, '\0', offset*sizeof(MYFLT));
    memset(yh, '\0', offset*sizeof(MYFLT));
    memset(yb, '\0', offset*sizeof(MYFLT));
    memset(yr, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&yl[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yr[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yh[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yb[nsmps], '\0', early*sizeof(MYFLT));
  }

  if(kn > 0.) {
    if(ftab != NULL) {
      tn = ftab->ftable;
      sz = ftab->flen;
      if(kn > mx) kn = mx;
    }
    if (kn < 0) kn = 0.;
    kno1 = 1./kn;
    for (i=offset; i<nsmps; i++) {
      D = 1./(q[i] >  0.5 ? q[i] : 0.5);
      fac = 1./(1. + w*D + w2);
      u = x[i]*iscal;
      yh[i] = (u - (D + w) * s[0] - s[1])*fac;
      u = w * nlf(tab,yh[i]*kn,max,size)*(tn ? tn[(int)(sz*kn/mx)] : kno1);
      yb[i] = u + s[0];
      s[0] = yb[i] + u;
      u = w * nlf(tab,yb[i]*kn,max,size)*(tn ? tn[(int)(sz*kn/mx)] : kno1);
      yl[i] = u + s[1];
      s[1] =  yl[i] + u;
      yr[i] = (yh[i] + yl[i])*scal;
      yl[i] *= scal;
      yb[i] *= scal;
      yh[i] *= scal;
    }
  }
  else {
    for (i=offset; i<nsmps; i++) {
      D = 1./(q[i] >  0.5 ? q[i] : 0.5);
      fac = 1./(1. + w*D + w2);
      u = x[i]*iscal;
      yh[i] = (u - (D + w) * s[0] - s[1])*fac;
      u = w * yh[i];
      yb[i] = u + s[0];
      s[0] = yb[i] + u;
      u = w * yb[i];
      yl[i] = u + s[1];
      s[1] =  yl[i] + u;
      yr[i] = (yh[i] + yl[i])*scal;
      yl[i] *= scal;
      yb[i] *= scal;
      yh[i] *= scal;
    }
  }
  return OK;
}

int32_t svn_perfaa(CSOUND *csound, SVN *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT *yl = p->yl, *yh = p->yh, *yb = p->yb, *yr = p->yr;
  MYFLT *x = p->x ,kn = *p->kn > 0 ? *p->kn : .0001, kno1, *f = p->f, *q = p->q;
  double u, w, fac, D;
  double *s = p->s;
  MYFLT *tab = p->tab, *tn = NULL;
  double max = p->max, mx = *p->mx;
  int32_t size = p->size, sz = 0;
  FUNC *ftab = csound->FTFind(csound, p->inm);
  double scal = csound->Get0dBFS(csound), iscal;
  iscal = 1./scal;

  if (UNLIKELY(offset)) {
    memset(yl, '\0', offset*sizeof(MYFLT));
    memset(yh, '\0', offset*sizeof(MYFLT));
    memset(yb, '\0', offset*sizeof(MYFLT));
    memset(yr, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&yl[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yr[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yh[nsmps], '\0', early*sizeof(MYFLT));
    memset(&yb[nsmps], '\0', early*sizeof(MYFLT));
  }

  if(kn > 0.) {
    if(ftab != NULL) {
      tn = ftab->ftable;
      sz = ftab->flen;
      if(kn > mx) kn = mx;
    }
    if (kn < 0) kn = 0.;
    kno1 = 1./kn;

    for (i=offset; i<nsmps; i++) {
      D = 1./(q[i] >  0.5 ? q[i] : 0.5);
      w = TAN(f[i]*p->piosr);
      fac = 1./(1. + w*D + w*w);
      u = x[i]*iscal;
      yh[i] = (u - (D + w) * s[0] - s[1])*fac;
      u = w * nlf(tab,yh[i]*kn,max,size)*(tn ? tn[(int)(sz*kn/mx)] : kno1);
      yb[i] = u + s[0];
      s[0] = yb[i] + u;
      u = w * nlf(tab,yb[i]*kn,max,size)*(tn ? tn[(int)(sz*kn/mx)] : kno1);
      yl[i] = u + s[1];
      s[1] =  yl[i] + u;
      yr[i] = (yh[i] + yl[i])*scal;
      yl[i] *= scal;
      yb[i] *= scal;
      yh[i] *= scal;
    }
  } else {
    for (i=offset; i<nsmps; i++) {
      D = 1./(q[i] >  0.5 ? q[i] : 0.5);
      w = TAN(f[i]*p->piosr);
      fac = 1./(1. + w*D + w*w);
      u = x[i]*iscal;
      yh[i] = (u - (D + w) * s[0] - s[1])*fac;
      u = w * yh[i];
      yb[i] = u + s[0];
      s[0] = yb[i] + u;
      u = w * yb[i];
      yl[i] = u + s[1];
      s[1] =  yl[i] + u;
      yr[i] = (yh[i] + yl[i])*scal;
      yl[i] *= scal;
      yb[i] *= scal;
      yh[i] *= scal;
    }
  }
  return OK;
}

typedef struct midsid {
  OPDS h;
  MYFLT *a0, *a1, *a2, *a3, *kw;
} MIDSID;

int32_t ms_encod(CSOUND *csound, MIDSID *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT *m = p->a0, *s = p->a1, *l = p->a2, *r = p->a3;

  if (UNLIKELY(offset)) {
    memset(m, '\0', offset*sizeof(MYFLT));
    memset(s, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&m[nsmps], '\0', early*sizeof(MYFLT));
    memset(&s[nsmps], '\0', early*sizeof(MYFLT));
  }

  for(i=0; i < nsmps; i++) {
    m[i] = l[i] + r[i];
    s[i] = l[i] - r[i];
  }
  return OK;
}

int32_t ms_decod(CSOUND *csound, MIDSID *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  MYFLT *m = p->a2, *s = p->a3, *l = p->a0, *r = p->a1;
  MYFLT w = *p->kw, wm1;
  wm1 = 1. - (w = w > 0. ? (w < 1. ? w : 1.) : 0.);

  if (UNLIKELY(offset)) {
    memset(l, '\0', offset*sizeof(MYFLT));
    memset(r, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&l[nsmps], '\0', early*sizeof(MYFLT));
    memset(&r[nsmps], '\0', early*sizeof(MYFLT));
  }

  for(i=0; i < nsmps; i++) {
    MYFLT mm, ss;
    mm = m[i]*wm1;
    ss = s[i]*w;
    l[i] = mm + ss;
    r[i] = mm - ss;
  }
  return OK;
}



static OENTRY localops[] =
  {
    {"mvchpf", sizeof(mvchpf24), 0, "a", "ako",
     (SUBR) mvchpf24_init, (SUBR) mvchpf24_perf},
    {"mvchpf", sizeof(mvchpf24), 0, "a", "aao",
     (SUBR) mvchpf24_init, (SUBR) mvchpf24_perf_a},
    {"mvclpf1", sizeof(mvclpf24), 0, "a", "akko",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf1},
    {"mvclpf1", sizeof(mvclpf24), 0, "a", "aako",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf1_ak},
    {"mvclpf1", sizeof(mvclpf24), 0, "a", "akao",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf1_ka},
    {"mvclpf1", sizeof(mvclpf24), 0, "a", "aaao",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf1_aa},
    {"mvclpf2", sizeof(mvclpf24), 0, "a", "akko",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf2},
    {"mvclpf2", sizeof(mvclpf24), 0, "a", "aako",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf2_ak},
    {"mvclpf2", sizeof(mvclpf24), 0, "a", "akao",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf2_ka},
    {"mvclpf2", sizeof(mvclpf24), 0, "a", "aaao",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf2_aa},
    {"mvclpf3", sizeof(mvclpf24), 0, "a", "akko",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf3},
    {"mvclpf3", sizeof(mvclpf24), 0, "a", "aako",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf3_ak},
    {"mvclpf3", sizeof(mvclpf24), 0, "a", "akao",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf3_ka},
    {"mvclpf3", sizeof(mvclpf24), 0, "a", "aaao",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf3_aa},
    {"mvclpf4", sizeof(mvclpf24_4), 0, "aaaa", "akko",
     (SUBR) mvclpf24_4_init, (SUBR) mvclpf24_perf4},
    {"mvclpf4", sizeof(mvclpf24), 0, "aaaa", "aako",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf4_ak},
    {"mvclpf4", sizeof(mvclpf24), 0, "aaaa", "akao",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf4_ka},
    {"mvclpf4", sizeof(mvclpf24), 0, "aaaa", "aaao",
     (SUBR) mvclpf24_init, (SUBR) mvclpf24_perf4_aa},
    {"moogladder.kk", sizeof(moogladder), 0, "a", "akko",
     (SUBR) moogladder_init, (SUBR) moogladder_process },
    {"moogladder.aa", sizeof(moogladder), 0, "a", "aaao",
     (SUBR) moogladder_init, (SUBR) moogladder_process_aa },
    {"moogladder.ak", sizeof(moogladder), 0, "a", "aako",
     (SUBR) moogladder_init, (SUBR) moogladder_process_ak },
    {"moogladder.ka", sizeof(moogladder), 0, "a", "akao",
     (SUBR) moogladder_init, (SUBR) moogladder_process_ka },
    {"moogladder2.kk", sizeof(moogladder), 0, "a", "akko",
     (SUBR) moogladder_init, (SUBR) moogladder2_process },
    {"moogladder2.aa", sizeof(moogladder), 0, "a", "aaao",
     (SUBR) moogladder_init, (SUBR) moogladder2_process_aa },
    {"moogladder2.ak", sizeof(moogladder), 0, "a", "aako",
     (SUBR) moogladder_init, (SUBR) moogladder2_process_ak },
    {"moogladder2.ka", sizeof(moogladder), 0, "a", "akao",
     (SUBR) moogladder_init, (SUBR) moogladder2_process_ka },
    {"statevar", sizeof(statevar), 0, "aaaa", "axxoo",
     (SUBR) statevar_init, (SUBR) statevar_process     },
    {"fofilter", sizeof(fofilter), 0, "a", "axxxo",
     (SUBR) fofilter_init, (SUBR) fofilter_process     },
    {"bob", sizeof(BOB), 0, "a", "axxxoo",
     (SUBR) bob_init, (SUBR) bob_process     },
    {"vps", sizeof(VPS), 0,  "a", "akk",
     (SUBR) NULL, (SUBR) vps_process },
    {"vclpf", sizeof(VCF), 0, "a", "akko",
     (SUBR) vcf_init, (SUBR) vcf_perfk },
    {"vclpf", sizeof(VCF), 0, "a", "aako",
     (SUBR) vcf_init, (SUBR) vcf_perfak },
    {"vclpf", sizeof(VCF), 0, "a", "akao",
     (SUBR) vcf_init, (SUBR) vcf_perfka },
    {"vclpf", sizeof(VCF), 0, "a", "aaao",
     (SUBR) vcf_init, (SUBR) vcf_perfaa },
    {"spf", sizeof(SPF), 0, "a", "aaakko",
     (SUBR) spf_init, (SUBR) spf_perfkk },
    {"spf", sizeof(SPF), 0, "a", "aaaako",
     (SUBR) spf_init, (SUBR) spf_perfak },
    {"spf", sizeof(SPF), 0, "a", "aaaaao",
     (SUBR) spf_init, (SUBR) spf_perfaa },
    {"spf", sizeof(SPF), 0, "a", "aaakao",
     (SUBR) spf_init, (SUBR) spf_perfka },
    {"skf", sizeof(SKF), 0, "a", "akkoo",
     (SUBR) skf_init, (SUBR) skf_perfkk },
    {"skf", sizeof(SKF), 0, "a", "aakoo",
     (SUBR) skf_init, (SUBR) skf_perfak },
    {"skf", sizeof(SKF), 0, "a", "aaaoo",
     (SUBR) skf_init, (SUBR) skf_perfaa },
    {"skf", sizeof(SKF), 0, "a", "akaoo",
     (SUBR) skf_init, (SUBR) skf_perfka },
    {"svn", sizeof(SVN), 0, "aaaa", "akkkoopo",
     (SUBR) svn_init, (SUBR) svn_perfkk },
    {"svn", sizeof(SVN), 0, "aaaa", "aakkoopo",
     (SUBR) svn_init, (SUBR) svn_perfak },
    {"svn", sizeof(SVN), 0, "aaaa", "akakoopo",
     (SUBR) svn_init, (SUBR) svn_perfka },
    {"svn", sizeof(SVN), 0, "aaaa", "aaakoopo",
     (SUBR) svn_init, (SUBR) svn_perfaa },
    {"st2ms", sizeof(MIDSID), 0,  "aa", "aa",
     (SUBR) NULL, (SUBR) ms_encod},
    {"ms2st", sizeof(MIDSID), 0,  "aa", "aak",
     (SUBR) NULL, (SUBR) ms_decod },
    {"otafilter", sizeof(VCFNL), 0, "aa", "akkko",
     (SUBR) vcfnl_init, (SUBR) vcfnl_perfk},
    {"otafilter", sizeof(VCFNL), 0, "aa", "aakko",
     (SUBR) vcfnl_init, (SUBR) vcfnl_perfak},
    {"otafilter", sizeof(VCFNL), 0, "aa", "akako",
     (SUBR) vcfnl_init, (SUBR) vcfnl_perfka},
    {"otafilter", sizeof(VCFNL), 0, "aa", "aaako",
     (SUBR) vcfnl_init, (SUBR) vcfnl_perfaa},
  };

int32_t newfils_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int32_t
                                ) (sizeof(localops) / sizeof(OENTRY)));
}
