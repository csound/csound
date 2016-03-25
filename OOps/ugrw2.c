/*
    ugrw2.c:

    Copyright (C) 1995, 1998 Robin Whittle, John ffitch

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

/* These files are based on Robin Whittle's
 *       ugrw2.c and ugrw2.h of 9 September 1995
 *
 * In February 1997, John Fitch reformatted the comments.
 * In February 1998, John Fitch modified the code wrt types so it
 * compiled with MicroSoft C without warnings.
*
 *
 * Copyright notice - Robin Whittle  25 February 1997
 *
 * Documentation files, and the original .c and .h files, with more
 * spaced out comments, are available from http://www.firstpr.com.au
 *
 * The code in both ugrw1 and ugrw2 is copyright Robin Whittle.
 * Permission is granted to use this in whole or in part for any
 * purpose, provided this copyright notice remains intact and
 * any alterations to the source code, including comments, are
 * clearly indicated as such.
 */
#if SOME_FINE_DAY

#include "ugrw2.h"
#include <math.h>

/*
 *      Unit generators by Robin Whittle        9 September 1995
 *      UGRW2.H contains data structures.
 *
 *      Ugens:          Subroutines:    Data structure:
 *
 *      kport           kporset()       KPORT
 *                      kport()         "
 *
 *      This is the same as the k rate port (portamento) ugen in ugens5.c,
 *      except the half time is a k rate variable, rather than an i rate.
 *
 *      Ugens:          Subroutines:    Data structure:
 *
 *      ktone           ktonset()       KTONE
 *                      ktone()         "
 *      katone          katone()        "
 *
 *      kreson          krsnset()       KRESON
 *                      kreson()        "
 *                      katone()        "
 *
 *      These are the same as tone, atone, reson and areson filters as per
 *      the Csound manual, except they operate with k rate data rather than
 *      a rate.
 *
 *      The code for all the above has been adapted from a commented version
 *      of ugens5.c.
 *
 *
 *      Ugens:          Subroutines:    Data structure:
 *
 *      limit           limit()         LIMIT
 *      ilimit          klimit()
 *
 *      limit sets upper and lower limits on k or a rate variables, where
 *      the limits are set by k rate variables.  ilimit does the same for
 *      init time variables.
 *
 */

/****************************************************************************
 *
 *      Filters - syntax
 *
*** kr  kport   ksig, khtime [,isig]
 *
 * ksig         k rate output.
 *
 * khtime       k rate time for output ot reach halfway to input.
 *
 * isig         i time, initialisation for internal state.
 *
 *
*** kr  ktone   ksig, khp [,istor]
*** kr  katone  ksig, khp [,istor]
 *
 * ksig         k rate output.
 *
 * khp          k rate frequency at which half power is reached.
 *
 * istor        i time, initialise internal state. Default 0 = clear.
 *
 *
*** kr  kreson  ksig, kcf, kbw [, iscl] [,istor]
*** kr  kareson ksig, kcf, kbw [, iscl] [,istor]
 *
 * ksig         k rate output.
 *
 * kcf          k rate centre frequency.
 *
 * kbw          k rate bandwidth - between lower and upper half power points.
 *
 * iscl         i time, 0 = no gain change (default or a rate reson, areson
 *                                          but leads to huge signals.)
 *                      1 = gain at centre = 1 (default in kreson, kareson)
 *                      2 = overal RMS gain = 1 (What exactly does this mean?)
 *
 * istor        i time, initialise internal state. Default 0 = clear.
 *
 *
 *>>>   Add these to the end of opcodlst in entry.c.
 *
 * opcode   dspace      thread  outarg  inargs   isub    ksub    asub
 *

{ "kport",   S(KPORT),  3,      "k",    "kko",  kporset, kport, NULL    },
{ "ktone",   S(KTONE),  3,      "k",    "kko",  ktonset, ktone, NULL    },
{ "katone",  S(KTONE),  3,      "k",    "kko",  ktonset, katone, NULL   },
{ "kreson",  S(KRESON), 3,      "k",    "kkkpo",krsnset, kreson, NULL   },
{ "kareson", S(KRESON), 3,      "k",    "kkkpo",krsnset, kareson, NULL  },

 *
 */

/****************************************************************************
 *
 *      limit, ilimit - syntax
 *
*** ir  ilimit  isig, ilow, ihigh
 *
*** kr  limit   ksig, klow, khigh
*** ar  limit   asig, klow, khigh
 *
 *      These set lower and upper limits on the xsig value they process.
 *
 *      If xhigh is lower than xlow, then the output will be the average of
 *      the two - it will not be affected by xsig.
 *
 *>>>   Add these to the end of opcodlst in entry.c.
 *
 * opcode   dspace      thread  outarg  inargs   isub    ksub    asub
 *

{ "ilimit", S(LIMIT),   1,      "i",    "iii",   klimit, NULL,   NULL},
{ "limit",  S(LIMIT),   2,      "k",    "xkk",   NULL, klimit, limit},
{ "limit",  S(LIMIT),   4,      "a",    "xkk",   NULL, klimit, limit},

 *
 */

/*---------------------------------------------------------------------------*/

/* kport = portamento
 * k rate low pass filter  with half time controlled by a k rate value.  */
int kporset(CSOUND *csound, KPORT *p)
{
    /* Set internal state to last input variable, but only if it is not
     * negative. (Why?) -- because that is how ugens without re-init is done
     * everywhere in Csound -- JPff */
    if (UNLIKELY(*p->isig >= 0))
      p->yt1 = *p->isig;
    p->prvhtim = -FL(100.0);
    return OK;
}

/*-----------------------------------*/

/* kport function.               */

int kport(CSOUND *csound, KPORT *p)
{
    /* Set up variables local to this instance of the port ugen, if
     * khtim has changed.
     *
     * onedkr = one divided by k rate.
     *
     * c2 = sqrt 1 / kr * half time
     * c1 = 1 - c2
     *
     */
    /* This previous comment is WRONG;  do not be misled -- JPff */

    if (UNLIKELY(p->prvhtim != *p->khtim)) {
      p->c2 = POWER(FL(0.5), CS_ONEDKR / *p->khtim);
      p->c1 = FL(1.0) - p->c2;
      p->prvhtim = *p->khtim;
    }
    /* New state =   (c2 * old state) + (c1 * input value)       */
    *p->kr = p->yt1 = p->c1 * *p->ksig + p->c2 * p->yt1;
    return OK;
}

/*****************************************************************************/

/* ktone = low pass single pole  filter.  ktonset function.      */
int ktonset(CSOUND *csound, KTONE *p)
{
    /* Initialise internal variables to  0 or 1.                 */
    p->c1 = p->prvhp = FL(0.0);
    p->c2 = FL(1.0);
    if (LIKELY(!(*p->istor)))
      p->yt1 = FL(0.0);
    return OK;
}
/*-----------------------------------*/

/* ktone function                                */
int ktone(CSOUND *csound, KTONE *p)
{
    /* If the current frequency is different from before, then calculate
     * new values for c1 and c2.                                 */
    if (UNLIKELY(*p->khp != p->prvhp)) {
      MYFLT b;
      p->prvhp = *p->khp;
      /* b = cos ( 2 * pi * freq / krate)
       * c2 = b - sqrt ( b squared - 1)
       * c1 = 1 - c2
       * tpidsr = 2 * pi / a sample rate
       * so tpidsr * ksmps = 2 * pi / k rate.
       * We need this since we are filtering at k rate, not a rate. */
    /* This previous comment is WRONG;  do not be misled -- JPff */

      b = FL(2.0) - COS(*p->khp * csound->tpidsr * CS_KSMPS);
      p->c2 = b - SQRT(b * b - FL(1.0));
      p->c1 = FL(1.0) - p->c2;
    }
    /* Filter calculation on each a rate sample:
     * New state =   (c2 * old state) + (c1 * input value)               */
    *p->kr = p->yt1 = p->c1 * *p->ksig + p->c2 * p->yt1;
    return OK;
}

/*-----------------------------------*/

/* katone = high pass single pole filter.   Uses toneset to set up its
 * variables. Identical to tone, except for the output calculation.      */
int katone(CSOUND *csound, KTONE *p)
{
    if (UNLIKELY(*p->khp != p->prvhp)) {
      MYFLT b;
      p->prvhp = *p->khp;
      b = FL(2.0) - COS(*p->khp * csound->tpidsr * CS_KSMPS);
      p->c2 = b - SQRT(b * b - FL(1.0));
      p->c1 = FL(1.0) - p->c2;
    }
    /* Output = c2 * (prev state + input)
     * new state = prev state - input            */

    *p->kr = p->yt1 = p->c2 * (p->yt1 + *p->ksig);
    p->yt1 -= *p->ksig;         /* yt1 contains yt1-xt1 */
    return OK;
}

/*****************************************************************************/

/* kreson = resonant filter                      */

int krsnset(CSOUND *csound, KRESON *p)
{
    /* Check scale = 0, 1 or 2.  */
    int scale;

    p->scale = scale = (int)*p->iscl;
    if (UNLIKELY(scale && scale != 1 && scale != 2)) {
      return csound->InitError(csound, Str("Illegal resonk iscl value, %f"),
                                       *p->iscl);
    }
    /* Put dummy values into previous centre freq and bandwidth.         */
    p->prvcf = p->prvbw = -FL(100.0);
    /* Set intial state to 0 if istor = 0.       */
    if (LIKELY(!(*p->istor)))
      p->yt1 = p->yt2 = FL(0.0);
    return OK;
}

/*-----------------------------------*/

/* kreson                        */
int kreson(CSOUND *csound, KRESON *p)
{
    int flag = 0; /* Set to 1 if either centre freq or bandwidth changed */
    MYFLT       c3p1, c3t4, omc3, c2sqr;

    /* Calculations for centre frequency if it changes.
     * cosf = cos (2pi * freq / krate)                   */
    if (UNLIKELY(*p->kcf != p->prvcf)) {
      p->prvcf = *p->kcf;
      p->cosf = COS(*p->kcf * csound->tpidsr * CS_KSMPS);
      flag = 1;
    }

    /* Calculations for bandwidth if it changes.
     * c3 = exp (-2pi * bwidth / krate)                  */
    if (UNLIKELY(*p->kbw != p->prvbw)) {
      p->prvbw = *p->kbw;
      p->c3 = EXP(*p->kbw * csound->mtpdsr * CS_KSMPS);
      flag = 1;
    }
    /* Final calculations for the factors
     * for the filter. Each multiplies
     * something and sums it to be the
     * output, and the input to the first
     * delay.
     * c1       Gain for input signal.
     * c2       Gain for output of delay 1.
     * c3       Gain for output of delay 2.
     */

    if (UNLIKELY(flag)) {
      c3p1 = p->c3 + FL(1.0);
      c3t4 = p->c3 * FL(4.0);
      omc3 = 1 - p->c3;

      /* c2= (c3 * 4 * cosf / (c3 + 1)      */
      p->c2 = c3t4 * p->cosf / c3p1;            /* -B, so + below */
      c2sqr = p->c2 * p->c2;

      if (p->scale == 1)
        /* iscl = 1. Make gain at centre = 1.
         * c1= (1 - c3) * sqrt( 1 - (c2 * c2 / (c3 * 4) )
         */
        p->c1 = omc3 * SQRT(FL(1.0) - c2sqr / c3t4);
      else if (p->scale == 2)
        /* iscl = 2 Higher gain, so "RMS gain" = 1.
         * c1= sqrt((c3 + 1)*(c3 + 1) - cs*c2) * (1 - c3) / (c3 + 1)
         * (I am not following the maths!)       */

        p->c1 = SQRT((c3p1*c3p1-c2sqr) * omc3/c3p1);
      /* iscl = 0. No scaling of the signal. Input gain c1 = 1.      */
      else p->c1 = FL(1.0);
    }

    /* Filter section, see c1, c2, c3 notes above.  Calculate output and
     * the new values for the first and second delays.     */

    *p->kr = p->c1 * *p->ksig + p->c2 * p->yt1 - p->c3 * p->yt2;
    p->yt2 = p->yt1;
    p->yt1 = *p->kr;
    return OK;
}

/*****************************************************************************/

/* karseson - band reject filter.
 *
 * uses krsnset() above.
 *
 * Comments not done yet. Modifications to make it k rate done with great care!
 */

int kareson(CSOUND *csound, KRESON *p)
{
    int flag = 0;
    MYFLT       c3p1, c3t4, omc3, c2sqr /*,D = FL(2.0)*/; /* 1/RMS = root2 (rand) */
    /*      or 1/.5  (sine) */
    if (UNLIKELY(*p->kcf != p->prvcf)) {
      p->prvcf = *p->kcf;
      p->cosf = COS(*p->kcf * csound->tpidsr * CS_KSMPS);
      flag = 1;
    }
    if (UNLIKELY(*p->kbw != p->prvbw)) {
      p->prvbw = *p->kbw;
      p->c3 = EXP(*p->kbw * csound->mtpdsr * CS_KSMPS);
      flag = 1;
    }
    if (UNLIKELY(flag)) {
      c3p1 = p->c3 + FL(1.0);
      c3t4 = p->c3 * FL(4.0);
      omc3 = FL(1.0) - p->c3;
      p->c2 = c3t4 * p->cosf / c3p1;
      c2sqr = p->c2 * p->c2;
      if (p->scale == 1)                        /* i.e. 1 - A(reson) */
        p->c1 = FL(1.0) - omc3 * SQRT(FL(1.0) - c2sqr / c3t4);
      else if (p->scale == 2)                 /* i.e. D - A(reson) */
        p->c1 = FL(2.0) - SQRT((c3p1*c3p1-c2sqr)*omc3/c3p1);
      else p->c1 = FL(0.0);                        /* cannot tell        */
    }

    if (p->scale == 1 || p->scale == 0)
      {
        *p->kr = p->c1 * *p->ksig + p->c2 * p->yt1 - p->c3 * p->yt2;
        p->yt2 = p->yt1;
        p->yt1 = *p->kr - *p->ksig;     /* yt1 contains yt1-xt1 */
      }

    else if (p->scale == 2)
      {
        *p->kr = p->c1 * *p->ksig + p->c2 * p->yt1 - p->c3 * p->yt2;
        p->yt2 = p->yt1;
        p->yt1 = *p->kr - (*p->ksig + *p->ksig); /* yt1 contains yt1-D*xt1 */
      }
    return OK;
}

/*****************************************************************************/
/*****************************************************************************/

/* limit and ilimit
 */

/* klimit()
 *
 * Used for k and i rate variables.
 */
int klimit(CSOUND *csound, LIMIT *p)
{
    MYFLT       xsig, xlow, xhigh;

    /* Optimise for speed when xsig is within the limits.   */

    if (LIKELY(((xsig = *p->xsig) <= *p->xhigh) && (xsig >= *p->xlow))) {
      *p->xdest = xsig;
    }
    else {
      /* xsig was not within the limits.
       *
       * Check to see if xlow > xhigh. If so, then the result will be
       * the mid point between them.       */

      if ( (xlow = *p->xlow) >= (xhigh = *p->xhigh) ) {
        *p->xdest = FL(0.5) * (xlow + xhigh); /* times 0.5 rather that /2 -- JPff */
      }
      else {
        if (xsig > xhigh)
          *p->xdest = xhigh;
        else
          *p->xdest = xlow;
      }
    }
    return OK;
}

/*************************************/

/* alimit()
 *
 * Used for a rate variables, with k rate limits. */
int limit(CSOUND *csound, LIMIT *p)
{
    MYFLT       *adest, *asig;
    MYFLT       xlow, xhigh, xaverage, xsig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    /*-----------------------------------*/

    /* Optimise for speed when xsig is within the limits.     */

    /* Set up input and output pointers.    */
    adest = p->xdest;
    asig  = p->xsig;
    /* Test to see if xlow > xhigh. if so, then write the average to
     * the destination - no need to look at the input.
     *
     * Whilst doing the test, load the limit local variables.
     */

    if (UNLIKELY(offset)) memset(adest, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early))) {
      nsmps -= early;
      memset(&adest[nsmps], '\0', early*sizeof(MYFLT));
    }
    if ((xlow = *p->xlow) >= (xhigh = *p->xhigh)) {
      xaverage = (xlow + xhigh) * FL(0.5);/* times 0.5 rather that /2 -- JPff */
      for (n=offset; n<nsmps; n++) {
        adest[n] = xaverage;
      }
    }
    else
      for (n=offset; n<nsmps; n++) {
        /* Do normal processing, testing each input value against the limits. */
        if (((xsig = asig[n]) <= xhigh) && (xsig >= xlow)) {
          /* xsig was within the limits.      */
          adest[n] = xsig;
        }
        else {
          if (xsig > xhigh)
            adest[n] = xhigh;
          else
            adest[n] = xlow;
        }
      }
    return OK;
}

#endif /* SOME_FINE_DAY */
