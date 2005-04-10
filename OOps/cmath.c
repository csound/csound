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

#include "cs.h"
#include "cmath.h"
#include <math.h>
#include <time.h>   /* for random seeding from the time - mkc July 1997 */

#ifndef RAND_MAX
#define RAND_MAX        (32767)
#endif

int ipow(ENVIRON *csound, POW *p)               /*      Power for i-rate */
{
    MYFLT in = *p->in;
    MYFLT powerOf = *p->powerOf;
    if (in == FL(0.0) && powerOf == FL(0.0))
      return csound->PerfError(csound, Str("NaN in pow\n"));
    else
      *p->sr = (MYFLT)pow((double)in, (double)powerOf) / *p->norm;
    return OK;
}

int apow(ENVIRON *csound, POW *p)               /* Power routine for a-rate  */
{
    int n, nsmps = csound->ksmps;
    MYFLT *in = p->in, *out = p->sr;
    MYFLT powerOf = *p->powerOf;

    if (powerOf == FL(0.0)) {
      MYFLT yy = FL(1.0) / *p->norm;
      for (n=0; n<csound->ksmps; n++) {
        MYFLT xx = in[n];
        if (xx == FL(0.0)) {
          return csound->PerfError(csound, Str("NaN in pow\n"));
        }
        else
          out[n] = yy;
      }
    }
    else {
      for (n=0; n<nsmps; n++)
        out[n] = (MYFLT)pow(in[n], powerOf) / *p->norm;
    }
    return OK;
}

/* Now global: long holdrand = 2345678L;  gab d5 */

int seedrand(ENVIRON *csound, PRAND *p)
{
    if ((unsigned int)*p->out == 0) {
      holdrand = time(NULL);
      printf(Str("Seeding from current time %d\n"), holdrand);
      srand((unsigned int)holdrand);
    }
    else {
      printf(Str("Seeding with %.3f\n"), *p->out);
      srand((unsigned int)(holdrand = (int)*p->out));
    }
    return OK;
}

int auniform(ENVIRON *csound, PRAND *p)         /* Uniform distribution */
{
    int n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n=0; n<nsmps; n++)
      out[n] = unifrand(arg1);
    return OK;
}

int ikuniform(ENVIRON *csound, PRAND *p)
{
    *p->out = unifrand(*p->arg1);
    return OK;
}

int alinear(ENVIRON *csound, PRAND *p)          /* Linear random functions      */
{
    int n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n=0; n<nsmps; n++)
      out[n] = linrand(arg1);
    return OK;
}

int iklinear(ENVIRON *csound, PRAND *p)
{
    *p->out = linrand(*p->arg1);
    return OK;
}

int atrian(ENVIRON *csound, PRAND *p)           /* Triangle random functions  */
{
    int n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n=0; n<nsmps; n++)
      out[n] = trirand(arg1);
    return OK;
}

int iktrian(ENVIRON *csound, PRAND *p)
{
    *p->out = trirand(*p->arg1);
    return OK;
}

int aexp(ENVIRON *csound, PRAND *p)             /* Exponential random functions */
{
    int n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n=0; n<nsmps; n++)
      out[n] = exprand(arg1);
    return OK;
}

int ikexp(ENVIRON *csound, PRAND *p)
{
    *p->out = exprand(*p->arg1);
    return OK;
}

int abiexp(ENVIRON *csound, PRAND *p)           /* Bilateral exponential rand */
{                                               /* functions */
    int n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n=0; n<nsmps; n++)
      out[n] = biexprand(arg1);
    return OK;
}

int ikbiexp(ENVIRON *csound, PRAND *p)
{
    *p->out = biexprand(*p->arg1);
    return OK;
}

int agaus(ENVIRON *csound, PRAND *p)            /* Gaussian random functions */
{
    int n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n=0; n<nsmps; n++)
      out[n] = gaussrand(arg1);
    return OK;
}

int ikgaus(ENVIRON *csound, PRAND *p)
{
    *p->out = gaussrand(*p->arg1);
    return OK;
}

int acauchy(ENVIRON *csound, PRAND *p)          /* Cauchy random functions */
{
    int n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n=0; n<nsmps; n++)
      out[n] = cauchrand(arg1);
    return OK;
}

int ikcauchy(ENVIRON *csound, PRAND *p)
{
    *p->out = cauchrand(*p->arg1);
    return OK;
}

int apcauchy(ENVIRON *csound, PRAND *p)         /* +ve Cauchy random functions */
{
    int n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n=0; n<nsmps; n++)
      out[n] = pcauchrand(arg1);
    return OK;
}

int ikpcauchy(ENVIRON *csound, PRAND *p)
{
    *p->out = pcauchrand(*p->arg1);
    return OK;
}

int abeta(ENVIRON *csound, PRAND *p)            /* Beta random functions   */
{
    int n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;
    MYFLT arg2 = *p->arg2;
    MYFLT arg3 = *p->arg3;

    for (n=0; n<nsmps; n++)
      out[n] = betarand(arg1, arg2, arg3);
    return OK;
}

int ikbeta(ENVIRON *csound, PRAND *p)
{
    *p->out = betarand(*p->arg1, *p->arg2, *p->arg3);
    return OK;
}

int aweib(ENVIRON *csound, PRAND *p)            /* Weibull randon functions */
{
    int n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;
    MYFLT arg2 = *p->arg2;

    for (n=0; n<nsmps; n++)
      out[n] = weibrand(arg1, arg2);
    return OK;
}

int ikweib(ENVIRON *csound, PRAND *p)
{
    *p->out = weibrand(*p->arg1, *p->arg2);
    return OK;
}

int apoiss(ENVIRON *csound, PRAND *p)           /*      Poisson random funcions */
{
    int n, nsmps = csound->ksmps;
    MYFLT *out = p->out;
    MYFLT arg1 = *p->arg1;

    for (n=0; n<nsmps; n++)
      out[n] = poissrand(arg1);
    return OK;
}

int ikpoiss(ENVIRON *csound, PRAND *p)
{
    *p->out = poissrand(*p->arg1);
    return OK;
}


/* * * * * * RANDOM NUMBER GENERATORS * * * * * */


#define unirand()       (MYFLT)((double)rand()/(double)RAND_MAX)

MYFLT unifrand(MYFLT range)
{
    return range*unirand();
}

MYFLT linrand(MYFLT range)      /*      linear distribution routine     */
{
    MYFLT r1, r2;

    r1 = unirand();
    r2 = unirand();

    if (r1 > r2)
      r1 = r2;

    return (r1 * range);
}

MYFLT trirand(MYFLT range)      /*      triangle distribution routine   */
{
    MYFLT r1, r2;

    r1 = unirand();
    r2 = unirand();

    return (((r1 + r2) - FL(1.0)) * range);
}

MYFLT exprand(MYFLT l)          /*      exponential distribution routine */
{
    MYFLT r1;

    if (l < FL(0.0)) return (FL(0.0));

    do {
      r1 = unirand();
    } while (r1 == FL(0.0));

    return (-(MYFLT)log(r1 *l));
}

MYFLT biexprand(MYFLT l)        /* bilateral exponential distribution routine */
{
    MYFLT r1;

    if (l < FL(0.0)) return (FL(0.0));

    do {
      r1 = FL(2.0) * unirand();
    } while (r1 == FL(0.0) || r1 == FL(2.0));

    if (r1 > FL(1.0))     {
      r1 = FL(2.0) - r1;
      return (-(MYFLT)log(r1 * l));
    }
    return ((MYFLT)log(r1 * l));
}

MYFLT gaussrand(MYFLT s)        /*      gaussian distribution routine   */
{
    MYFLT r1 = FL(0.0);
    int n = 12;
    s /= FL(3.83);

    do {
      r1 += unirand();
    } while (--n);

    return (s * (r1 - FL(6.0)));
}

MYFLT cauchrand(MYFLT a)        /*      cauchy distribution routine     */
{
    MYFLT r1;
    a /= FL(318.3);

    do {
      do {
        r1 = unirand();
      } while (r1 == FL(0.5));

      r1 = a * (MYFLT)tan(PI*(double)r1);
    } while (r1>FL(1.0) || r1<-FL(1.0)); /* Limit range artificially */
    return r1;
}

MYFLT pcauchrand(MYFLT a)       /*      positive cauchy distribution routine */
{
    MYFLT r1;
    a /= FL(318.3);

    do {
      do {
        r1 = unirand();
      } while (r1 == FL(1.0));

      r1 = a * (MYFLT)tan( PI * 0.5 * (double)r1);
    } while (r1>FL(1.0));
    return r1;
}

MYFLT betarand(MYFLT range, MYFLT a, MYFLT b) /* beta distribution routine  */
{
    MYFLT r1, r2;

    if (a < FL(0.0) || b < FL(0.0) ) return (FL(0.0));

    do {
      do {
        r1 = unirand();
      } while (r1 == FL(0.0));

      do {
        r2 = unirand();
      } while (r2 == FL(0.0));

      r1 = (MYFLT)pow(r1, 1.0 / (double)a);
      r2 = (MYFLT)pow(r2, 1.0 / (double)b);
    } while ((r1 + r2) > FL(1.0));

    return ((r1 / (r1 +r2)) * range);
}

MYFLT weibrand(MYFLT s, MYFLT t) /*      weibull distribution routine    */
{
    MYFLT r1, r2;

    if (t < FL(0.0) ) return (FL(0.0));

    do {
      r1 = unirand();
    } while (r1 == FL(0.0) || r1 == FL(1.0));

    r2 = FL(1.0) /  (FL(1.0) - r1);

    return (s * (MYFLT)pow (log((double)r2),  (1.0 /(double)t)));
}

MYFLT poissrand(MYFLT l)        /*      Poisson distribution routine    */
{
    MYFLT r1, r2, r3;

    if (l < FL(0.0) ) return (FL(0.0));

    r1 = unirand();
    r2 = (MYFLT)exp(-l);
    r3 = FL(0.0);

    while (r1 >= r2) {
      r3++;
      r1 *= unirand();
    }

    return (r3);
}
