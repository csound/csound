/*
    cmath.c:

    Copyright (C) 1994 Paris Smaragdis, John ffitch

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

/*      Math functions for Csound coded by Paris Smaragdis 1994         */
/*      Berklee College of Music Csound development team                */

#include "csoundCore.h"
#include "cmath.h"
#include <math.h>

int32_t ipow(CSOUND *csound, POW *p)        /*      Power for i-rate */
{
    MYFLT in = *p->in;
    MYFLT powerOf = *p->powerOf;
    if (UNLIKELY(in == FL(0.0) && powerOf == FL(0.0)))
      return csound->PerfError(csound, &(p->h), Str("NaN in pow\n"));
    else if (p->norm!=NULL && *p->norm != FL(0.0))
      *p->sr = POWER(in, powerOf) / *p->norm;
    else
      *p->sr = POWER(in, powerOf);
    return OK;
}

int32_t apow(CSOUND *csound, POW *p)        /* Power routine for a-rate  */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *in = p->in, *out = p->sr;
    MYFLT powerOf = *p->powerOf;
    MYFLT norm = (p->norm!=NULL ? *p->norm : FL(1.0));
    if (norm==FL(0.0)) norm = FL(1.0);
    memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (UNLIKELY(powerOf == FL(0.0))) {
      MYFLT yy = FL(1.0) / norm;
      for (n = offset; n < nsmps; n++) {
        MYFLT xx = in[n];
        if (UNLIKELY(xx == FL(0.0))) {
          return csound->PerfError(csound, &(p->h),Str("NaN in pow\n"));
        }
        else
          out[n] = yy;
      }
    }
    else {
      for (n = offset; n < nsmps; n++)
        out[n] = POWER(in[n], powerOf) / norm;
    }
    return OK;
}

int32_t seedrand(CSOUND *csound, PRAND *p)
{
    uint32_t  seedVal = (uint32_t)0;
    int32_t xx = (int32_t)((double)*p->out + 0.5);

    if (xx > FL(0.0))
      seedVal = (uint32_t)xx;
    else if (xx==0) {
      seedVal = (uint32_t)csound->GetRandomSeedFromTime();
      csound->Warning(csound, Str("Seeding from current time %u\n"),
                              (uint32_t)seedVal);
    }
    else
      csound->Warning(csound, Str("Seeding with %u\n"), (uint32_t)seedVal);
    csound->SeedRandMT(&(csound->randState_), NULL, seedVal);
    csound->holdrand = (int32_t)(seedVal & (uint32_t) 0x7FFFFFFF);
    while (seedVal >= (uint32_t)0x7FFFFFFE)
      seedVal -= (uint32_t)0x7FFFFFFE;
    if (seedVal==0) csound->randSeed1 = ((int32_t)1);
    csound->randSeed1 = ((int32_t)seedVal);

    return OK;
}

int32_t getseed(CSOUND *csound, GETSEED *p)
{
    *p->ans = csound->randSeed1;
    return OK;
}

/* * * * * * RANDOM NUMBER GENERATORS * * * * * */

#define UInt32toFlt(x) ((double)(x) * (1.0 / 4294967295.03125))

#define unirand(c) ((MYFLT) UInt32toFlt(csoundRandMT(&((c)->randState_))))

static inline MYFLT unifrand(CSOUND *csound, MYFLT range)
{
    return (range * unirand(csound));
}

/* linear distribution routine */

static inline MYFLT linrand(CSOUND *csound, MYFLT range)
{
    uint32_t  r1, r2;

    r1 = csoundRandMT(&(csound->randState_));
    r2 = csoundRandMT(&(csound->randState_));

    return ((MYFLT)UInt32toFlt(r1 < r2 ? r1 : r2) * range);
}

/* triangle distribution routine */

static inline MYFLT trirand(CSOUND *csound, MYFLT range)
{
    uint64_t  r1;

    r1 = (uint64_t)csoundRandMT(&(csound->randState_));
    r1 += (uint64_t)csoundRandMT(&(csound->randState_));

    return ((MYFLT) ((double)((int64_t)r1 - (int64_t)0xFFFFFFFFU)
                     * (1.0 / 4294967295.03125)) * range);
}

/* exponential distribution routine */

static MYFLT exprand(CSOUND *csound, MYFLT lambda)
{
    uint32_t  r1;

    if (UNLIKELY(lambda < FL(0.0))) return (FL(0.0)); /* for safety */

    do {
      r1 = csoundRandMT(&(csound->randState_));
    } while (!r1);

    return -((MYFLT)log(UInt32toFlt(r1)) * lambda);
}

/* bilateral exponential distribution routine */

static MYFLT biexprand(CSOUND *csound, MYFLT range)
{
    int32_t r1;

    if (UNLIKELY(range < FL(0.0))) return (FL(0.0)); /* For safety */

    while ((r1 = (int32_t)csoundRandMT(&(csound->randState_)))==0);

    if (r1 < (int32_t)0) {
      return -(LOG(-(r1) * (FL(1.0) / FL(2147483648.0))) * range);
    }
    return (LOG(r1 * (FL(1.0) / FL(2147483648.0))) * range);
}

/* gaussian distribution routine */

static MYFLT gaussrand(CSOUND *csound, MYFLT s)
{
    int64_t   r1 = -((int64_t)0xFFFFFFFFU * 6);
    int32_t       n = 12;
    double    x;

    do {
      r1 += (int64_t)csoundRandMT(&(csound->randState_));
    } while (--n);
    x = (double)r1;
    return (MYFLT)(x * ((double)s * (1.0 / (3.83 * 4294967295.03125))));
}

/* cauchy distribution routine */

static MYFLT cauchrand(CSOUND *csound, MYFLT a)
{
    uint32_t  r1;
    MYFLT     x;

    do {
      r1 = csoundRandMT(&(csound->randState_)); /* Limit range artificially */
    } while (r1 > (uint32_t)2143188560U && r1 < (uint32_t)2151778735U);
    x = TAN((MYFLT)r1 * (PI_F / FL(4294967295.0))) * (FL(1.0) / FL(318.3));
    return (x * a);
}

/* positive cauchy distribution routine */

static MYFLT pcauchrand(CSOUND *csound, MYFLT a)
{
    uint32_t  r1;
    MYFLT     x;

    do {
      r1 = csoundRandMT(&(csound->randState_));
    } while (r1 > (uint32_t)4286377121U);      /* Limit range artificially */
    x = TAN((MYFLT)r1 * HALFPI_F / FL(4294967295.0))
      * (FL(1.0) / FL(318.3));
    return (x * a);
}

/* beta distribution routine */

static MYFLT betarand(CSOUND *csound, MYFLT range, MYFLT a, MYFLT b)
{
    double  r1, r2;
    double aa, bb;
    if (UNLIKELY(a <= FL(0.0) || b <= FL(0.0)))
      return FL(0.0);

    aa = (double)a; bb = (double)b;
    do {
      uint32_t  tmp;
      do {
        tmp = csoundRandMT(&(csound->randState_));
      } while (!tmp);
      r1 = pow(UInt32toFlt(tmp), 1.0 / aa);
      do {
        tmp = csoundRandMT(&(csound->randState_));
      } while (!tmp);
      r2 = r1 + pow(UInt32toFlt(tmp), 1.0 / bb);
    } while (r2 > 1.0);

    return (((MYFLT)r1 / (MYFLT)r2) * range);
}

/* weibull distribution routine */

static MYFLT weibrand(CSOUND *csound, MYFLT s, MYFLT t)
{
    uint32_t  r1;
    double    r2;

    if (UNLIKELY(t <= FL(0.0))) return FL(0.0);

    do {
      r1 = csoundRandMT(&(csound->randState_));
    } while (!r1 || r1 == (uint32_t)0xFFFFFFFFU);

    r2 = 1.0 - ((double)r1 * (1.0 / 4294967295.0));

    return (s * (MYFLT)pow(-(log(r2)), (1.0 / (double)t)));
}

/* Poisson distribution routine */

static MYFLT poissrand(CSOUND *csound, MYFLT lambda)
{
    MYFLT r1, r2, r3;

    if (UNLIKELY(lambda < FL(0.0))) return FL(0.0);

    r1 = unirand(csound);
    r2 = EXP(-lambda);
    r3 = FL(0.0);

    while (r1 >= r2) {
      r3++;
      r1 *= unirand(csound);
    }

    return (r3);
}

 /* ------------------------------------------------------------------------ */

int32_t auniform(CSOUND *csound, PRAND *p)  /* Uniform distribution */
{
    MYFLT   *out = p->out;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    double  scale = (double)*p->arg1 * (1.0 / 4294967295.03125);

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      out[n] = (MYFLT)((double)csoundRandMT(&(csound->randState_)) * scale);
    }
    return OK;
}

int32_t ikuniform(CSOUND *csound, PRAND *p)
{
    *p->out = unifrand(csound, *p->arg1);
    return OK;
}

int32_t alinear(CSOUND *csound, PRAND *p)   /* Linear random functions      */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      out[n] = linrand(csound, arg1);
    return OK;

}

int32_t iklinear(CSOUND *csound, PRAND *p)
{
    *p->out = linrand(csound, *p->arg1);
    return OK;
}

int32_t atrian(CSOUND *csound, PRAND *p)    /* Triangle random functions  */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      out[n] = trirand(csound, arg1);
    return OK;
}

int32_t iktrian(CSOUND *csound, PRAND *p)
{
    *p->out = trirand(csound, *p->arg1);
    return OK;
}

int32_t exprndiset(CSOUND *csound, PRANDI *p)
{
    p->num1 = exprand(csound, *p->arg1);
    p->num2 = exprand(csound, *p->arg1);
    p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    p->phs = 0;
    p->ampcod = IS_ASIG_ARG(p->xamp) ? 1 : 0;      /* (not used by krandi) */
    p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
    return OK;
}

int kexprndi(CSOUND *csound, PRANDI *p)
{                                       /* rslt = (num1 + diff*phs) * amp */
    /* IV - Jul 11 2002 */
    *p->ar = (p->num1 + (MYFLT)p->phs * p->dfdmax) * *p->xamp;
    p->phs += (int32_t)(*p->xcps * CS_KICVT); /* phs += inc           */
    if (UNLIKELY(p->phs >= MAXLEN)) {         /* when phs overflows,  */
      p->phs &= PHMASK;                       /*      mod the phs     */
      p->num1 = p->num2;                      /*      & new num vals  */
      p->num2 = exprand(csound, *p->arg1);
      p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    }
    return OK;
}

int32_t iexprndi(CSOUND *csound, PRANDI *p)
{
    exprndiset(csound, p);
    return kexprndi(csound, p);
}

int32_t aexprndi(CSOUND *csound, PRANDI *p)
{
   int32_t       phs = p->phs, inc;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *ar, *ampp, *cpsp;

    cpsp = p->xcps;
    ampp = p->xamp;
    ar = p->ar;
    inc = (int32_t)(cpsp[0] * csound->sicvt);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      /* IV - Jul 11 2002 */
      if (p->ampcod)
        ar[n] = (p->num1 + (MYFLT)phs * p->dfdmax) * ampp[n];
      else
        ar[n] = (p->num1 + (MYFLT)phs * p->dfdmax) * ampp[0];
      phs += inc;                                /* phs += inc       */
      if (p->cpscod)
        inc = (int32_t)(cpsp[n] * csound->sicvt);  /*   (nxt inc)      */
      if (UNLIKELY(phs >= MAXLEN)) {             /* when phs o'flows */
        phs &= PHMASK;
        p->num1 = p->num2;
        p->num2 = exprand(csound, *p->arg1);
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
      }
    }
    p->phs = phs;
    return OK;
}

int32_t aexp(CSOUND *csound, PRAND *p)      /* Exponential random functions */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      out[n] = exprand(csound, arg1);
    return OK;
}

int32_t ikexp(CSOUND *csound, PRAND *p)
{
    *p->out = exprand(csound, *p->arg1);
    return OK;
}

int32_t abiexp(CSOUND *csound, PRAND *p)    /* Bilateral exponential rand */
{                                       /* functions */
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      out[n] = biexprand(csound, arg1);
    return OK;
}

int32_t ikbiexp(CSOUND *csound, PRAND *p)
{
    *p->out = biexprand(csound, *p->arg1);
    return OK;
}

int32_t agaus(CSOUND *csound, PRAND *p)     /* Gaussian random functions */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      out[n] = gaussrand(csound, arg1);
    return OK;
}

int32_t ikgaus(CSOUND *csound, PRAND *p)
{
    *p->out = gaussrand(csound, *p->arg1);
    return OK;
}

int32_t acauchy(CSOUND *csound, PRAND *p)   /* Cauchy random functions */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      out[n] = cauchrand(csound, arg1);
    return OK;
}

int32_t gaussiset(CSOUND *csound, PRANDI *p)
{
    p->num1 = gaussrand(csound, *p->arg1);
    p->num2 = gaussrand(csound, *p->arg1);
    p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    p->phs = 0;
    p->ampcod = IS_ASIG_ARG(p->xamp) ? 1 : 0;      /* (not used by krandi) */
    p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
    return OK;
}

int32_t kgaussi(CSOUND *csound, PRANDI *p)
{                                       /* rslt = (num1 + diff*phs) * amp */
    /* IV - Jul 11 2002 */
    *p->ar = (p->num1 + (MYFLT)p->phs * p->dfdmax) * *p->xamp;
    p->phs += (int32_t)(*p->xcps * CS_KICVT); /* phs += inc           */
    if (UNLIKELY(p->phs >= MAXLEN)) {           /* when phs overflows,  */
      p->phs &= PHMASK;                         /*      mod the phs     */
      p->num1 = p->num2;                        /*      & new num vals  */
      p->num2 = gaussrand(csound, *p->arg1);
      p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    }
    return OK;
}

int32_t igaussi(CSOUND *csound, PRANDI *p)
{
    gaussiset(csound, p);
    return kgaussi(csound, p);
}

int32_t agaussi(CSOUND *csound, PRANDI *p)
{
   int32_t       phs = p->phs, inc;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *ar, *ampp, *cpsp;

    cpsp = p->xcps;
    ampp = p->xamp;
    ar = p->ar;
    inc = (int32_t)(*cpsp * csound->sicvt);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      /* IV - Jul 11 2002 */
      if (p->ampcod)
        ar[n] = (p->num1 + (MYFLT)phs * p->dfdmax) * ampp[n];
      else
        ar[n] = (p->num1 + (MYFLT)phs * p->dfdmax) * ampp[0];
      phs += inc;                                /* phs += inc       */
      if (p->cpscod)
        inc = (int32_t)(cpsp[n] * csound->sicvt);  /*   (nxt inc)      */
      if (UNLIKELY(phs >= MAXLEN)) {             /* when phs o'flows */
        phs &= PHMASK;
        p->num1 = p->num2;
        p->num2 = gaussrand(csound, *p->arg1);
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
      }
    }
    p->phs = phs;
    return OK;
}

int32_t ikcauchy(CSOUND *csound, PRAND *p)
{
    *p->out = cauchrand(csound, *p->arg1);
    return OK;
}

int32_t apcauchy(CSOUND *csound, PRAND *p)  /* +ve Cauchy random functions */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      out[n] = pcauchrand(csound, arg1);
    return OK;
}

int32_t ikpcauchy(CSOUND *csound, PRAND *p)
{
    *p->out = pcauchrand(csound, *p->arg1);
    return OK;
}

int32_t cauchyiset(CSOUND *csound, PRANDI *p)
{
    p->num1 = cauchrand(csound, *p->arg1);
    p->num2 = cauchrand(csound, *p->arg1);
    p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    p->phs = 0;
    p->ampcod = IS_ASIG_ARG(p->xamp) ? 1 : 0;      /* (not used by krandi) */
    p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
    return OK;
}

int32_t kcauchyi(CSOUND *csound, PRANDI *p)
{                                       /* rslt = (num1 + diff*phs) * amp */
    /* IV - Jul 11 2002 */
    *p->ar = (p->num1 + (MYFLT)p->phs * p->dfdmax) * *p->xamp;
    p->phs += (int32_t)(*p->xcps * CS_KICVT); /* phs += inc           */
    if (UNLIKELY(p->phs >= MAXLEN)) {         /* when phs overflows,  */
      p->phs &= PHMASK;                       /*      mod the phs     */
      p->num1 = p->num2;                      /*      & new num vals  */
      p->num2 = cauchrand(csound, *p->arg1);
      p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    }
    return OK;
}

int32_t icauchyi(CSOUND *csound, PRANDI *p)
{
    cauchyiset(csound, p);
    return kcauchyi(csound, p);
}

int32_t acauchyi(CSOUND *csound, PRANDI *p)
{
   int32_t       phs = p->phs, inc;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *ar, *ampp, *cpsp;

    cpsp = p->xcps;
    ampp = p->xamp;
    ar = p->ar;
    inc = (int32_t)(*cpsp * csound->sicvt);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      /* IV - Jul 11 2002 */
      if (p->ampcod)
        ar[n] = (p->num1 + (MYFLT)phs * p->dfdmax) * ampp[n];
      else
        ar[n] = (p->num1 + (MYFLT)phs * p->dfdmax) * ampp[0];
      phs += inc;                                /* phs += inc       */
      if (p->cpscod)
        inc = (int32_t)(cpsp[n] * csound->sicvt);  /*   (nxt inc)      */
      if (UNLIKELY(phs >= MAXLEN)) {             /* when phs o'flows */
        phs &= PHMASK;
        p->num1 = p->num2;
        p->num2 = cauchrand(csound, *p->arg1);
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
      }
    }
    p->phs = phs;
    return OK;
}

int32_t abeta(CSOUND *csound, PRAND *p)     /* Beta random functions   */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;
    MYFLT arg2 = *p->arg2;
    MYFLT arg3 = *p->arg3;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = 0; n < nsmps; n++)
      out[n] = betarand(csound, arg1, arg2, arg3);
    return OK;
}

int32_t ikbeta(CSOUND *csound, PRAND *p)
{
    *p->out = betarand(csound, *p->arg1, *p->arg2, *p->arg3);
    return OK;
}

int32_t aweib(CSOUND *csound, PRAND *p)     /* Weibull randon functions */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;
    MYFLT arg2 = *p->arg2;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      out[n] = weibrand(csound, arg1, arg2);
    return OK;
}

int32_t ikweib(CSOUND *csound, PRAND *p)
{
    *p->out = weibrand(csound, *p->arg1, *p->arg2);
    return OK;
}

int32_t apoiss(CSOUND *csound, PRAND *p)    /*      Poisson random funcions */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      out[n] = poissrand(csound, arg1);
    return OK;
}

int32_t ikpoiss(CSOUND *csound, PRAND *p)
{
    *p->out = poissrand(csound, *p->arg1);
    return OK;
}

 /* ------------------------------------------------------------------------ */

int32_t gen21_rand(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    int32_t  i, n;
    MYFLT   *ft;
    MYFLT   scale;
    int32_t nargs = ff->e.pcnt - 4;

    ft = ftp->ftable;
    scale = (nargs > 1 ? ff->e.p[6] : FL(1.0));
    n = ff->flen;
    if (ff->guardreq)
      n++;
    switch ((int32_t) ff->e.p[5]) {
    case 1:                     /* Uniform distribution */
      for (i = 0 ; i < n ; i++)
        ft[i] = unifrand(csound, scale);
      break;
    case 2:                     /* Linear distribution */
      for (i = 0 ; i < n ; i++)
        ft[i] = linrand(csound, scale);
      break;
    case 3:                     /* Triangular about 0.5 */
      for (i = 0 ; i < n ; i++)
        ft[i] = trirand(csound, scale);
      break;
    case 4:                     /* Exponential */
      for (i = 0 ; i < n ; i++)
        ft[i] = exprand(csound, scale);
      break;
    case 5:                     /* Bilateral exponential */
      for (i = 0 ; i < n ; i++)
        ft[i] = biexprand(csound, scale);
      break;
    case 6:                     /* Gaussian distribution */
      for (i = 0 ; i < n ; i++)
        ft[i] = gaussrand(csound, scale);
      break;
    case 7:                     /* Cauchy distribution */
      for (i = 0 ; i < n ; i++)
        ft[i] = cauchrand(csound, scale);
      break;
    case 8:                     /* Positive Cauchy */
      for (i = 0 ; i < n ; i++)
        ft[i] = pcauchrand(csound, scale);
      break;
    case 9:                     /* Beta distribution */
      if (UNLIKELY(nargs < 3)) {
        return -1;
      }
      for (i = 0 ; i < n ; i++)
        ft[i] = betarand(csound, scale, (MYFLT) ff->e.p[7], (MYFLT) ff->e.p[8]);
      break;
    case 10:                    /* Weibull Distribution */
      if (UNLIKELY(nargs < 2)) {
        return -1;
      }
      for (i = 0 ; i < n ; i++)
        ft[i] = weibrand(csound, scale, (MYFLT) ff->e.p[7]);
      break;
    case 11:                    /* Poisson Distribution */
      for (i = 0 ; i < n ; i++)
        ft[i] = poissrand(csound, scale);
      break;
    default:
      return -2;
    }
    return OK;
}

/* ---------------------------------------------- */

/* New Gauss generator using Box-Mueller transform
   VL April 2020
 */

MYFLT gausscompute(CSOUND *csound, GAUSS *p) {
  if(p->flag == 0) {
    MYFLT u1 = unirand(csound);
    MYFLT u2 = unirand(csound);
    MYFLT z = SQRT(-2.*LOG(u1))*cos(2*PI*u2);
    p->z = SQRT(-2.*LOG(u1))*sin(2*PI*u2);
    p->flag = 1;
    return *p->sigma*z + *p->mu;
  } else {
    p->flag = 0;
    return  *p->sigma*p->z + *p->mu;
  }
  return OK;
}


int32_t gauss_scalar(CSOUND *csound, GAUSS *p){
  *p->a = gausscompute(csound,p);
  return OK;

}

int32_t gauss_vector(CSOUND *csound, GAUSS *p) {
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->a;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      out[n] = gausscompute(csound,p);
    return OK;
}






