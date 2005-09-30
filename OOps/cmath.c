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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/*      Math functions for Csound coded by Paris Smaragdis 1994         */
/*      Berklee College of Music Csound development team                */

#include "csoundCore.h"
#include "cmath.h"
#include <math.h>

int ipow(CSOUND *csound, POW *p)        /*      Power for i-rate */
{
    MYFLT in = *p->in;
    MYFLT powerOf = *p->powerOf;
    if (in == FL(0.0) && powerOf == FL(0.0))
      return csound->PerfError(csound, Str("NaN in pow\n"));
    else
      *p->sr = (MYFLT)pow((double)in, (double)powerOf) / *p->norm;
    return OK;
}

int apow(CSOUND *csound, POW *p)        /* Power routine for a-rate  */
{
    int   n, nsmps = csound->ksmps;
    MYFLT *in = p->in, *out = p->sr;
    MYFLT powerOf = *p->powerOf;

    if (powerOf == FL(0.0)) {
      MYFLT yy = FL(1.0) / *p->norm;
      for (n = 0; n < nsmps; n++) {
        MYFLT xx = in[n];
        if (xx == FL(0.0)) {
          return csound->PerfError(csound, Str("NaN in pow\n"));
        }
        else
          out[n] = yy;
      }
    }
    else {
      for (n = 0; n < nsmps; n++)
        out[n] = (MYFLT) pow(in[n], powerOf) / *p->norm;
    }
    return OK;
}

int seedrand(CSOUND *csound, PRAND *p)
{
    uint32_t  seedVal = (uint32_t) 0;

    if (*p->out > FL(0.0))
      seedVal = (uint32_t) ((double) *p->out + 0.5);
    if (!seedVal) {
      seedVal = (uint32_t) csound->GetRandomSeedFromTime();
      csound->Message(csound, Str("Seeding from current time %u\n"),
                              (unsigned int) seedVal);
    }
    else
      csound->Message(csound, Str("Seeding with %u\n"), (unsigned int) seedVal);
    csound->SeedRandMT(&(csound->randState_), NULL, seedVal);
    csound->holdrand = (int) (seedVal & (uint32_t) 0x7FFFFFFF);
    while (seedVal >= (uint32_t) 0x7FFFFFFE)
      seedVal -= (uint32_t) 0x7FFFFFFE;
    csound->randSeed1 = ((int) seedVal + 1);

    return OK;
}

/* * * * * * RANDOM NUMBER GENERATORS * * * * * */

#define UInt32toFlt(x)  (((double) ((int32_t) ((x) ^ (uint32_t) 0x80000000U)) \
                          + 2147483648.03125) * (1.0 / 4294967295.0625))

static inline MYFLT unirand(CSOUND *csound)
{
    return (MYFLT) UInt32toFlt(csoundRandMT(&(csound->randState_)));
}

static inline MYFLT unifrand(CSOUND *csound, MYFLT range)
{
    return range * unirand(csound);
}

/* linear distribution routine */

static inline MYFLT linrand(CSOUND *csound, MYFLT range)
{
    uint32_t  r1, r2;

    r1 = csoundRandMT(&(csound->randState_));
    r2 = csoundRandMT(&(csound->randState_));

    return ((MYFLT) UInt32toFlt(r1 < r2 ? r1 : r2) * range);
}

/* triangle distribution routine */

static inline MYFLT trirand(CSOUND *csound, MYFLT range)
{
    double  r1, r2;

    r1 = (double) ((int32_t) (csoundRandMT(&(csound->randState_))
                              ^ (uint32_t) 0x80000000U));
    r2 = (double) ((int32_t) (csoundRandMT(&(csound->randState_))
                              ^ (uint32_t) 0x80000000U));

    return ((MYFLT) ((r1 + r2 + 1.0) * (1.0 / 4294967295.0625)) * range);
}

/* exponential distribution routine */

static MYFLT exprand(CSOUND *csound, MYFLT l)
{
    uint32_t  r1;

    if (l < FL(0.0)) return (FL(0.0));

    do {
      r1 = csoundRandMT(&(csound->randState_));
    } while (!r1);

    return -((MYFLT) log(UInt32toFlt(r1)) * l);
}

/* bilateral exponential distribution routine */

static MYFLT biexprand(CSOUND *csound, MYFLT l)
{
    int32_t r1;

    if (l < FL(0.0)) return (FL(0.0));

    do {
      r1 = (int32_t) csoundRandMT(&(csound->randState_));
    } while (!r1);

    if (r1 < (int32_t) 0) {
      return -((MYFLT) log(-((double) r1) * (1.0 / 2147483648.0)) * l);
    }
    return ((MYFLT) log((double) r1 * (1.0 / 2147483648.0)) * l);
}

/* gaussian distribution routine */

static MYFLT gaussrand(CSOUND *csound, MYFLT s)
{
    uint64_t  r1 = (uint64_t) 0;
    int       n = 12;
    double    x;

    do {
      r1 += (uint64_t) csoundRandMT(&(csound->randState_));
    } while (--n);
    x = (double) ((int64_t) r1) - 25769803770.0;
    return (MYFLT) (x * ((double) s * (1.0 / (3.83 * 4294967295.0625))));
}

/* cauchy distribution routine */

static MYFLT cauchrand(CSOUND *csound, MYFLT a)
{
    uint32_t  r1;
    MYFLT     x;

    do {
      r1 = csoundRandMT(&(csound->randState_)) ^ (uint32_t) 0x80000000U;
      x = (MYFLT) tan(((double) ((int32_t) r1) + 2147483648.03125)
                      * (PI / 4294967295.0625)) * (MYFLT) (1.0 / 318.3);
    } while (x > FL(1.0) || x < -FL(1.0));      /* Limit range artificially */
    return (x * a);
}

/* positive cauchy distribution routine */

static MYFLT pcauchrand(CSOUND *csound, MYFLT a)
{
    uint32_t  r1;
    MYFLT     x;

    do {
      do {
        r1 = csoundRandMT(&(csound->randState_));
      } while (r1 == (uint32_t) 0xFFFFFFFFU);
      r1 ^= (uint32_t) 0x80000000U;
      x = (MYFLT) tan(((double) ((int32_t) r1) + 2147483648.03125)
                      * (PI * 0.5 / 4294967295.0625)) * (MYFLT) (1.0 / 318.3);
    } while (x > FL(1.0));                      /* Limit range artificially */
    return (x * a);
}

/* beta distribution routine */

static MYFLT betarand(CSOUND *csound, MYFLT range, MYFLT a, MYFLT b)
{
    MYFLT r1, r2;

    if (a < FL(0.0) || b < FL(0.0))
      return FL(0.0);

    do {
      do {
        r1 = unirand(csound);
      } while (r1 == FL(0.0));

      do {
        r2 = unirand(csound);
      } while (r2 == FL(0.0));

      r1 = (MYFLT) pow(r1, 1.0 / (double) a);
      r2 = (MYFLT) pow(r2, 1.0 / (double) b);
    } while ((r1 + r2) > FL(1.0));

    return ((r1 / (r1 + r2)) * range);
}

/* weibull distribution routine */

static MYFLT weibrand(CSOUND *csound, MYFLT s, MYFLT t)
{
    MYFLT r1, r2;

    if (t < FL(0.0))
      return FL(0.0);

    do {
      r1 = unirand(csound);
    } while (r1 == FL(0.0) || r1 == FL(1.0));

    r2 = FL(1.0) /  (FL(1.0) - r1);

    return (s * (MYFLT) pow(log((double) r2), (1.0 / (double) t)));
}

/* Poisson distribution routine */

static MYFLT poissrand(CSOUND *csound, MYFLT l)
{
    MYFLT r1, r2, r3;

    if (l < FL(0.0))
      return FL(0.0);

    r1 = unirand(csound);
    r2 = (MYFLT) exp(-l);
    r3 = FL(0.0);

    while (r1 >= r2) {
      r3++;
      r1 *= unirand(csound);
    }

    return (r3);
}

 /* ------------------------------------------------------------------------ */

int auniform(CSOUND *csound, PRAND *p)  /* Uniform distribution */
{
    int   n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n = 0; n < nsmps; n++)
      out[n] = unifrand(csound, arg1);
    return OK;
}

int ikuniform(CSOUND *csound, PRAND *p)
{
    *p->out = unifrand(csound, *p->arg1);
    return OK;
}

int alinear(CSOUND *csound, PRAND *p)   /* Linear random functions      */
{
    int   n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n = 0; n < nsmps; n++)
      out[n] = linrand(csound, arg1);
    return OK;
}

int iklinear(CSOUND *csound, PRAND *p)
{
    *p->out = linrand(csound, *p->arg1);
    return OK;
}

int atrian(CSOUND *csound, PRAND *p)    /* Triangle random functions  */
{
    int   n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n = 0; n < nsmps; n++)
      out[n] = trirand(csound, arg1);
    return OK;
}

int iktrian(CSOUND *csound, PRAND *p)
{
    *p->out = trirand(csound, *p->arg1);
    return OK;
}

int aexp(CSOUND *csound, PRAND *p)      /* Exponential random functions */
{
    int   n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n = 0; n < nsmps; n++)
      out[n] = exprand(csound, arg1);
    return OK;
}

int ikexp(CSOUND *csound, PRAND *p)
{
    *p->out = exprand(csound, *p->arg1);
    return OK;
}

int abiexp(CSOUND *csound, PRAND *p)    /* Bilateral exponential rand */
{                                       /* functions */
    int   n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n = 0; n < nsmps; n++)
      out[n] = biexprand(csound, arg1);
    return OK;
}

int ikbiexp(CSOUND *csound, PRAND *p)
{
    *p->out = biexprand(csound, *p->arg1);
    return OK;
}

int agaus(CSOUND *csound, PRAND *p)     /* Gaussian random functions */
{
    int   n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n = 0; n < nsmps; n++)
      out[n] = gaussrand(csound, arg1);
    return OK;
}

int ikgaus(CSOUND *csound, PRAND *p)
{
    *p->out = gaussrand(csound, *p->arg1);
    return OK;
}

int acauchy(CSOUND *csound, PRAND *p)   /* Cauchy random functions */
{
    int   n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n = 0; n < nsmps; n++)
      out[n] = cauchrand(csound, arg1);
    return OK;
}

int ikcauchy(CSOUND *csound, PRAND *p)
{
    *p->out = cauchrand(csound, *p->arg1);
    return OK;
}

int apcauchy(CSOUND *csound, PRAND *p)  /* +ve Cauchy random functions */
{
    int   n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n = 0; n < nsmps; n++)
      out[n] = pcauchrand(csound, arg1);
    return OK;
}

int ikpcauchy(CSOUND *csound, PRAND *p)
{
    *p->out = pcauchrand(csound, *p->arg1);
    return OK;
}

int abeta(CSOUND *csound, PRAND *p)     /* Beta random functions   */
{
    int   n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;
    MYFLT arg2 = *p->arg2;
    MYFLT arg3 = *p->arg3;

    for (n = 0; n < nsmps; n++)
      out[n] = betarand(csound, arg1, arg2, arg3);
    return OK;
}

int ikbeta(CSOUND *csound, PRAND *p)
{
    *p->out = betarand(csound, *p->arg1, *p->arg2, *p->arg3);
    return OK;
}

int aweib(CSOUND *csound, PRAND *p)     /* Weibull randon functions */
{
    int   n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;
    MYFLT arg2 = *p->arg2;

    for (n = 0; n < nsmps; n++)
      out[n] = weibrand(csound, arg1, arg2);
    return OK;
}

int ikweib(CSOUND *csound, PRAND *p)
{
    *p->out = weibrand(csound, *p->arg1, *p->arg2);
    return OK;
}

int apoiss(CSOUND *csound, PRAND *p)    /*      Poisson random funcions */
{
    int   n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n = 0; n < nsmps; n++)
      out[n] = poissrand(csound, arg1);
    return OK;
}

int ikpoiss(CSOUND *csound, PRAND *p)
{
    *p->out = poissrand(csound, *p->arg1);
    return OK;
}

 /* ------------------------------------------------------------------------ */

int gen21_rand(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    long    i, n;
    MYFLT   *ft;
    MYFLT   scale;
    int     nargs = ff->e.pcnt - 4;

    ft = ftp->ftable;
    scale = (nargs > 1 ? ff->e.p[6] : FL(1.0));
    n = ff->flen;
    if (ff->guardreq)
      n++;
    switch ((int) ff->e.p[5]) {
    case 1:                     /* Uniform distribution */
      for (i = 0 ; i < n ; i++)
        *ft++ = unifrand(csound, scale);
      break;
    case 2:                     /* Linear distribution */
      for (i = 0 ; i < n ; i++)
        *ft++ = linrand(csound, scale);
      break;
    case 3:                     /* Triangular about 0.5 */
      for (i = 0 ; i < n ; i++)
        *ft++ = trirand(csound, scale);
      break;
    case 4:                     /* Exponential */
      for (i = 0 ; i < n ; i++)
        *ft++ = exprand(csound, scale);
      break;
    case 5:                     /* Bilateral exponential */
      for (i = 0 ; i < n ; i++)
        *ft++ = biexprand(csound, scale);
      break;
    case 6:                     /* Gaussian distribution */
      for (i = 0 ; i < n ; i++)
        *ft++ = gaussrand(csound, scale);
      break;
    case 7:                     /* Cauchy distribution */
      for (i = 0 ; i < n ; i++)
        *ft++ = cauchrand(csound, scale);
      break;
    case 8:                     /* Positive Cauchy */
      for (i = 0 ; i < n ; i++)
        *ft++ = pcauchrand(csound, scale);
      break;
    case 9:                     /* Beta distribution */
      if (nargs < 3) {
        return -1;
      }
      for (i = 0 ; i < n ; i++)
        *ft++ = betarand(csound, scale, (MYFLT)ff->e.p[7], (MYFLT)ff->e.p[8]);
      break;
    case 10:                    /* Weibull Distribution */
      if (nargs < 2) {
        return -1;
      }
      for (i = 0 ; i < n ; i++)
        *ft++ = weibrand(csound, scale, (MYFLT) ff->e.p[7]);
      break;
    case 11:                    /* Poisson Distribution */
      for (i = 0 ; i < n ; i++)
        *ft++ = poissrand(csound, scale);
      break;
    default:
      return -2;
    }
    return OK;
}

