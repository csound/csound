/*
    lowpassr.c:

    Copyright (C) 1998 Gabriel Maldonado

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

/* Resonant Lowpass filters by G.Maldonado */

#include "stdopcod.h"
#include "lowpassr.h"
#include "filter_math.h"
#include <math.h>

/* Validate only when coefficients change, before dividing by either input. */
#define LOWRES_CHECK_PARAMS(cutoff, resonance)                            \
    do {                                                                 \
      if (UNLIKELY(!((cutoff) > FL(0.0) && (resonance) > FL(0.0))))       \
        return csound->PerfError(csound, &p->h, "%s\n",                     \
                  Str("lowres: cutoff and resonance must be positive")); \
    } while (0)

#ifdef USE_FLOAT
/* The direct form loses low-order bits at low cutoffs. In this form ynm2
   stores the last output increment. Compute the next increment directly
   and retain the same unity DC gain as the original recurrence. */
#define LOWRES_COEFS(coef, cutoff, resonance)                              \
    do {                                                                  \
      LOWRES_CHECK_PARAMS(cutoff, resonance);                              \
      const cs_double denom = FL(1000.0) +                                 \
        FL(10.0) * SQRT(cutoff) / (resonance);                             \
      const cs_double reciprocal = FL(1.0) / denom;                       \
      (coef).feedback = FL(1000.0) * reciprocal;                           \
      (coef).gain = (cutoff) * reciprocal;                                \
    } while (0)
#if defined(FP_FAST_FMAF)
#define LOWRES_STEP(coef, input, ynm1, ynm2)                                \
    do {                                                                   \
      const cs_double drive = (coef).gain * (cs_double)(input);             \
      const cs_double next = CS_FILTER_MADD((coef).feedback, (ynm2), drive); \
      (ynm2) = CS_FILTER_MADD(-(coef).gain, (ynm1), next);                   \
      (ynm1) += (ynm2);                                                     \
    } while (0)
#else
#define LOWRES_STEP(coef, input, ynm1, ynm2)                                \
    do {                                                                   \
      const cs_double force = (coef).gain * ((cs_double)(input) - (ynm1));  \
      (ynm2) = CS_FILTER_MADD((coef).feedback, (ynm2), force);               \
      (ynm1) += (ynm2);                                                     \
    } while (0)
#endif
#else
#define LOWRES_COEFS(coef, cutoff, resonance)                               \
    do {                                                                   \
      LOWRES_CHECK_PARAMS(cutoff, resonance);                               \
      const cs_double b = 10.0 / ((resonance) * sqrt((cs_double)(cutoff))) - 1.0; \
      const cs_double k = 1000.0 / (cs_double)(cutoff);                      \
      (coef).feedback = k;                                                  \
      (coef).feedback1 = b + 2.0 * k;                                        \
      (coef).gain = 1.0 / (1.0 + b + k);                                    \
    } while (0)
#define LOWRES_STEP(coef, input, ynm1, ynm2)                                \
    do {                                                                   \
      const cs_double next_output = ((coef).feedback1 * (ynm1) -            \
        (coef).feedback * (ynm2) + (cs_double)(input)) * (coef).gain;        \
      (ynm2) = (ynm1);                                                      \
      (ynm1) = next_output;                                                \
    } while (0)
#endif

static int32_t lowpr_set(CSOUND *csound, LOWPR *p)
{

    IGN(csound);
    if (*p->istor==FL(0.0))
      p->ynm1 = p->ynm2 = 0.0;
    /* Force the first coefficient calculation, including invalid inputs. */
    p->okf = NAN;
    p->okr = 0.0;
    p->coef = (LOWRES_COEFFICIENTS){0};
    return OK;
}

CS_FILTER_FUNCTION
static int32_t lowpr(CSOUND *csound, LOWPR *p)
{
    CS_FILTER_ORDERED
    cs_float *ar, *asig;
    cs_double ynm1, ynm2;
    cs_float kfco = *p->kfco;
    cs_float kres = *p->kres;
    LOWRES_COEFFICIENTS coef = p->coef;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (p->okf != kfco || p->okr != kres) { /* Only if changed */
      LOWRES_COEFS(coef, kfco, kres);
      p->coef = coef;
      p->okf = kfco; p->okr = kres; /* remember to save recalculation */
    }
    ar = p->ar;
    asig = p->asig;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps;n++) {
      LOWRES_STEP(coef, asig[n], ynm1, ynm2);
      ar[n] = (cs_float)ynm1;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;             /* And save */

    return OK;
}

CS_FILTER_FUNCTION
static int32_t lowpraa(CSOUND *csound, LOWPR *p)
{
    CS_FILTER_ORDERED
    cs_float *ar, *asig;
    cs_double ynm1, ynm2;
    cs_float *fco = p->kfco;
    cs_float *res = p->kres;
    cs_float okf = p->okf, okr = p->okr;
    LOWRES_COEFFICIENTS coef = p->coef;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ar = p->ar;
    asig = p->asig;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps;n++) {
      if (okf!= fco[n] || okr != res[n]) { /* Only if changed */
        LOWRES_COEFS(coef, fco[n], res[n]);
        p->coef = coef;
        okf = fco[n]; okr = res[n];
        /* remember to save recalculation */
      }
      LOWRES_STEP(coef, asig[n], ynm1, ynm2);
      ar[n] = (cs_float)ynm1;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;             /* And save */
    p->okf = okf; p->okr = okr;
    return OK;
}

CS_FILTER_FUNCTION
static int32_t lowprak(CSOUND *csound, LOWPR *p)
{
    CS_FILTER_ORDERED
    cs_float *ar, *asig;
    cs_double ynm1, ynm2;
    cs_float *fco = p->kfco;
    cs_float kres = *p->kres;
    cs_float okf = p->okf, okr = p->okr;
    LOWRES_COEFFICIENTS coef = p->coef;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ar = p->ar;
    asig = p->asig;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps;n++) {
      if (okf != fco[n] || okr != kres) { /* Only if changed */
        LOWRES_COEFS(coef, fco[n], kres);
        p->coef = coef;
        okf = fco[n]; okr = kres; /* remember to save recalculation */
      }
      LOWRES_STEP(coef, asig[n], ynm1, ynm2);
      ar[n] = (cs_float)ynm1;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;             /* And save */
    p->okf = okf; p->okr = okr;

    return OK;
}

CS_FILTER_FUNCTION
static int32_t lowprka(CSOUND *csound, LOWPR *p)
{
    CS_FILTER_ORDERED
    cs_float *ar, *asig;
    cs_double ynm1, ynm2;
    cs_float fco = *p->kfco;
    cs_float *res = p->kres;
    cs_float okr = p->okr, okf = p->okf;
    LOWRES_COEFFICIENTS coef = p->coef;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ar = p->ar;
    asig = p->asig;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps;n++) {
      if (okf != fco || okr != res[n]) { /* Only if changed */
        LOWRES_COEFS(coef, fco, res[n]);
        p->coef = coef;
        okf = fco; okr = res[n]; /* remember to save recalculation */
      }
      LOWRES_STEP(coef, asig[n], ynm1, ynm2);
      ar[n] = (cs_float)ynm1;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;             /* And save */
    p->okf = okf; p->okr = okr;

    return OK;
}

static int32_t lowpr_setx(CSOUND *csound, LOWPRX *p)
{
    int32_t j;
    if ((p->loop = (int32_t) CS_FLOAT2LONG(*p->ord)) < 1) p->loop = 4; /*default value*/
    else if (UNLIKELY(p->loop > 10)) {
      return csound->InitError(csound, "%s\n", Str("illegal order num. (min 1, max 10)"));
    }
    if (*p->istor == FL(0.0))
      for (j=0; j< p->loop; j++)  p->ynm1[j] = p->ynm2[j] = FL(0.0);
    p->okr = -FL(1.0);
    p->coef = (LOWRES_COEFFICIENTS){0};
    p->okf = NAN;
    return OK;
}

CS_FILTER_FUNCTION
static int32_t lowprx(CSOUND *csound, LOWPRX *p)
{
    CS_FILTER_ORDERED
    IGN(csound);
    cs_float   *ar, *asig, yn;
    cs_double  *ynm1, *ynm2;
    LOWRES_COEFFICIENTS coef = p->coef;
    cs_float    *kfco = p->kfco, *kres = p->kres;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      j;
    int32_t      asgf = IS_ASIG_ARG(p->kfco), asgr = IS_ASIG_ARG(p->kres);

    ynm1 = p->ynm1;
    ynm2 = p->ynm2;
    asig = p->asig;
    if (UNLIKELY(offset)) memset(p->ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->ar[nsmps], '\0', early*sizeof(cs_float));
    }

    ar = p->ar;
    for (n=offset;n<nsmps;n++) {
      cs_float fco = (asgf ? kfco[n] : *kfco);
      cs_float res = (asgr ? kres[n] : *kres);
      if (p->okf != fco || p->okr != res) { /* Only if changed */
        LOWRES_COEFS(coef, fco, res);
        p->okf = fco; p->okr = res; /* remember to save recalculation */
      }
      yn = asig[n];
      for (j=0; j< p->loop; j++) {
        LOWRES_STEP(coef, yn, ynm1[j], ynm2[j]);
        /* Keep cs_double history, but pass cs_float between layers like lowres. */
        yn = (cs_float)ynm1[j];
      }
      ar[n] = yn;
    }
    p->coef = coef;
    return OK;
}

static int32_t lowpr_w_sep_set(CSOUND *csound, LOWPR_SEP *p)
{
    int32_t j;
    if ((p->loop = (int32_t) CS_FLOAT2LONG(*p->ord)) < 1)
      p->loop = 4; /*default value*/
    else if (UNLIKELY(p->loop > 10)) {
      return csound->InitError(csound, "%s\n", Str("illegal order num. (min 1, max 10)"));
    }
    for (j=0; j< p->loop; j++)  p->ynm1[j] = p->ynm2[j] = FL(0.0);
    return OK;
}

CS_FILTER_FUNCTION
static int32_t lowpr_w_sep(CSOUND *csound, LOWPR_SEP *p)
{
    CS_FILTER_ORDERED
    IGN(csound);
    cs_float   *ar, *asig;
    cs_double   *ynm1, *ynm2;
    cs_float    kfcobase = *p->kfco;
    cs_float    sep = (*p->sep / p->loop);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      j = 0;

    cs_float kres = *p->kres;
    cs_float kfco;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;
    asig = p->asig;

    if (UNLIKELY(offset)) memset(p->ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->ar[nsmps], '\0', early*sizeof(cs_float));
    }
    ar = p->ar;
#ifdef USE_FLOAT
    /* Work on two stages together. During each iteration the first stage
       advances the next sample while the second consumes the current one. */
    for (; j + 1 < p->loop; j += 2) {
      const cs_float firstCutoff = kfcobase * (FL(1.0) + sep * j);
      const cs_float secondCutoff = kfcobase * (FL(1.0) + sep * (j + 1));
      LOWRES_COEFFICIENTS first, second;
      LOWRES_COEFS(first, firstCutoff, kres);
      LOWRES_COEFS(second, secondCutoff, kres);
      if (offset < nsmps) {
        cs_double firstOut = ynm1[j], firstStep = ynm2[j];
        cs_double secondOut = ynm1[j + 1], secondStep = ynm2[j + 1];
        LOWRES_STEP(first, asig[offset], firstOut, firstStep);
        for (n = offset; n + 1 < nsmps; n++) {
          const cs_float sample = (cs_float)firstOut;
          LOWRES_STEP(first, asig[n + 1], firstOut, firstStep);
          LOWRES_STEP(second, sample, secondOut, secondStep);
          ar[n] = (cs_float)secondOut;
        }
        LOWRES_STEP(second, firstOut, secondOut, secondStep);
        ar[n] = (cs_float)secondOut;
        ynm1[j] = firstOut;
        ynm2[j] = firstStep;
        ynm1[j + 1] = secondOut;
        ynm2[j + 1] = secondStep;
        asig = ar;
      }
    }
#endif
    for (; j < p->loop; j++) {
      cs_double lynm1 = ynm1[j];
      cs_double lynm2 = ynm2[j];
                /*
                linfco=log((double) kfco)*ONEtoLOG2     ;
                linfco = linfco + (sep / p->loop)*j;
                kfco = (cs_float) pow(2.0,linfco);
                */
      kfco = kfcobase * (FL(1.0) + (sep * j));

      LOWRES_COEFFICIENTS coef;
      LOWRES_COEFS(coef, kfco, kres);

      for (n=offset;n<nsmps; n++) {
        LOWRES_STEP(coef, asig[n], lynm1, lynm2);
        ar[n] = (cs_float)lynm1;
      }
      ynm1[j] = lynm1;
      ynm2[j] = lynm2;
      asig= p->ar;
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] =
  {
    { "lowres.kk",   S(LOWPR), 0,  "a", "akko",  (SUBR)lowpr_set, (SUBR)lowpr   },
    { "lowres.aa",   S(LOWPR), 0,  "a", "aaao",  (SUBR)lowpr_set, (SUBR)lowpraa },
    { "lowres.ak",   S(LOWPR), 0, "a", "aako",  (SUBR)lowpr_set, (SUBR)lowprak },
    { "lowres.ka",   S(LOWPR), 0,  "a", "akao",  (SUBR)lowpr_set, (SUBR)lowprka },

    { "lowresx.kk",  S(LOWPRX),0,  "a", "akkoo", (SUBR)lowpr_setx, (SUBR)lowprx },
    { "lowresx.ak",  S(LOWPRX),0,  "a", "aakoo", (SUBR)lowpr_setx, (SUBR)lowprx },
    { "lowresx.ka",  S(LOWPRX),0,  "a", "akaoo", (SUBR)lowpr_setx, (SUBR)lowprx },
    { "lowresx.aa",  S(LOWPRX),0,  "a", "aaaoo", (SUBR)lowpr_setx, (SUBR)lowprx },

    { "vlowres", S(LOWPR_SEP),0,  "a", "akkik",  (SUBR)lowpr_w_sep_set, (SUBR)lowpr_w_sep }
};

int32_t lowpassr_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
