/*
    ugsc.c:

    Copyright (C) 1999 Sean Costello

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

/* ugsc.c -- Opcodes from Sean Costello <costello@seanet.com> */

#include <math.h>
#include "csdl.h"
#include "ugsc.h"

/* svfilter.c
 *
 * Copyright 1999, by Sean M. Costello
 *
 * svfilter is an implementation of Hal Chamberlin's state variable filter
 * algorithm, from "Musical Applications of Microprocessors" (Hayden Books,
 * Indianapolis, Indiana, 1985), 2nd. edition, pp. 489-492. It implements
 * a second-order resonant filter, with lowpass, highpass and bandpass
 * outputs.
 *
 */

int svfset(ENVIRON *csound, SVF *p)
{
    /* set initial delay states to 0 */
    p->ynm1 = p->ynm2 = FL(0.0);
    return OK;
}

int svf(ENVIRON *csound, SVF *p)
{
    MYFLT f1, q1, scale;
    MYFLT *low, *high, *band, *in, ynm1, ynm2;
    MYFLT low2, high2, band2;
    MYFLT kfco = *p->kfco, kq = *p->kq;
    int nsmps = csound->ksmps;

    /* calculate frequency and Q coefficients */
    f1 = FL(2.0) * (MYFLT)sin((double)(kfco * csound->pidsr));
    if (kq<FL(0.000001)) kq = FL(1.0); /* Protect against division by zero */
    q1 = FL(1.0) / kq;

    in = p->in;
    low = p->low;
    band = p->band;
    high = p->high;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    /* if there is a non-zero value for iscl, set scale to be
     * equal to the Q coefficient.
     */
    if (*p->iscl)
      scale = q1;
    else
      scale = FL(1.0);

    /* equations derived from Hal Chamberlin, "Musical Applications
     * of Microprocessors.
     */
    do {
      *low++ = low2 = ynm2 + f1 * ynm1;
      *high++ = high2 = scale * *in++ - low2 - q1 * ynm1;
      *band++ = band2 = f1 * high2 + ynm1;
      ynm1 = band2;
      ynm2 = low2;
    } while (--nsmps);
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;
    return OK;
}

/* hilbert.c
 *
 * Copyright 1999, by Sean M. Costello
 *
 * hilbert is an implementation of an IIR Hilbert transformer.
 * The structure is based on two 6th-order allpass filters in
 * parallel, with a constant phase difference of 90 degrees
 * (+- some small amount of error) between the two outputs.
 * Allpass coefficients are calculated at i-time.
 */

int hilbertset(ENVIRON *csound, HILBERT *p)
{
    int j;  /* used to increment for loop */

    /* pole values taken from Bernie Hutchins, "Musical Engineer's Handbook" */
    double poles[12] = {.3609, 2.7412, 11.1573, 44.7581, 179.6242, 798.4578,
                        1.2524, 5.5671, 22.3423, 89.6271, 364.7914, 2770.1114};
    double polefreq[12], rc[12], alpha[12], beta[12];
    /* calculate coefficients for allpass filters, based on sampling rate */
    for (j=0; j<12; j++) {
      /*      p->coef[j] = (1 - (15 * PI * pole[j]) / csound->esr) /
              (1 + (15 * PI * pole[j]) / csound->esr); */
      polefreq[j] = poles[j] * 15.0;
      rc[j] = 1.0 / (2.0 * PI * polefreq[j]);
      alpha[j] = 1.0 / rc[j];
      beta[j] = (1.0 - (alpha[j] / (2.0 * (double)csound->esr))) /
                (1.0 + (alpha[j] / (2.0 * (double)csound->esr)));
      p->xnm1[j] = p->ynm1[j] = FL(0.0);
      p->coef[j] = -(MYFLT)beta[j];
    }
    return OK;
}

int hilbert(ENVIRON *csound, HILBERT *p)
{
    MYFLT xn1 = FL(0.0), yn1 = FL(0.0), xn2 = FL(0.0), yn2 = FL(0.0);
    MYFLT *out1, *out2, *in;
    MYFLT *xnm1, *ynm1, *coef;
    int nsmps = csound->ksmps;
    int j;

    xnm1 = p->xnm1;
    ynm1 = p->ynm1;
    coef = p->coef;
    out1 = p->out1;
    out2 = p->out2;
    in = p->in;

    do {
      xn1 = *in;
      /* 6th order allpass filter for cosine output. Structure is
       * 6 first-order allpass sections in series. Coefficients
       * taken from arrays calculated at i-time.
       */
      for (j=0; j < 6; j++) {
        yn1 = coef[j] * (xn1 - p->ynm1[j]) + p->xnm1[j];
        p->xnm1[j] = xn1;
        p->ynm1[j] = yn1;
        xn1 = yn1;
      }
      xn2 = *in++;
      /* 6th order allpass filter for sine output. Structure is
       * 6 first-order allpass sections in series. Coefficients
       * taken from arrays calculated at i-time.
       */
      for (j=6; j < 12; j++) {
        yn2 = coef[j] * (xn2 - p->ynm1[j]) + p->xnm1[j];
        p->xnm1[j] = xn2;
        p->ynm1[j] = yn2;
        xn2 = yn2;
      }
      *out1++ = yn1;
      *out2++ = yn2;
    } while (--nsmps);
    return OK;
}

/* resonrz.c
 *
 * Copyright 1999, by Sean M. Costello
 *
 * resonr and resonz are implementations of second-order
 * bandpass resonators, with added zeroes in the transfer function.
 * The algorithms are based upon the work of Julius O. Smith and
 * John Stautner at Stanford, and Ken Steiglitz at Princeton.
 *
 */

int resonzset(ENVIRON *csound, RESONZ *p)
{
    /* error message code derived from code for reson in ugens5.c */
    int scaletype;
    p->scaletype = scaletype = (int)*p->iscl;
    if (scaletype && scaletype != 1 && scaletype != 2) {
      return csound->InitError(csound, Str("illegal reson iscl value, %f"),
                                       *p->iscl);
    }
    if (!(*p->istor))
      p->xnm1 = p->xnm2 = p->ynm1 = p->ynm2 = FL(0.0);

/*     p->aratemod = (XINARG2) ? 1 : 0; */

    return OK;
}

int resonr(ENVIRON *csound, RESONZ *p)
{
    /*
     *
     * An implementation of the 2-pole, 2-zero reson filter
     * described by Julius O. Smith and James B. Angell in
     * "A Constant Gain Digital Resonator Tuned by a Single
     * Coefficient," Computer Music Journal, Vol. 6, No. 4,
     * Winter 1982, p.36-39. resonr implements the version
     * where the zeros are located at + and - the square root
     * of r, where r is the pole radius of the reson filter.
     *
     */

    MYFLT r, scale; /* radius & scaling factor */
    MYFLT c1, c2;   /* filter coefficients */
    MYFLT *out, *in, xn, yn, xnm1, xnm2, ynm1, ynm2;
    MYFLT kcf = *p->kcf, kbw = *p->kbw;
    int nsmps = csound->ksmps;

    r = (MYFLT)exp((double)(kbw * csound->mpidsr));
    c1 = FL(2.0) * r * (MYFLT)cos((double)(kcf * csound->tpidsr));
    c2 = r * r;

    /* calculation of scaling coefficients */
    if (p->scaletype == 1)
      scale = FL(1.0) - r;
    else if (p->scaletype == 2)
      scale = (MYFLT)sqrt(1.0 - (double)r);
    else scale = FL(1.0);

    out = p->out;
    in = p->in;
    xnm1 = p->xnm1;
    xnm2 = p->xnm2;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    do {
      xn = *in++;
      *out++ = yn = scale * (xn - r * xnm2) + c1 * ynm1 - c2 * ynm2;
      xnm2 = xnm1;
      xnm1 = xn;
      ynm2 = ynm1;
      ynm1 = yn;
    } while (--nsmps);
    p->xnm1 = xnm1;
    p->xnm2 = xnm2;
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;
    return OK;
}

int resonz(ENVIRON *csound, RESONZ *p)
{
    /*
     *
     * An implementation of the 2-pole, 2-zero reson filter
     * described by Julius O. Smith and James B. Angell in
     * "A Constant Gain Digital Resonator Tuned by a Single
     * Coefficient," Computer Music Journal, Vol. 6, No. 4,
     * Winter 1982, p.36-39. resonr implements the version
     * where the zeros are located at z = 1 and z = -1.
     *
     */

    MYFLT r, scale; /* radius & scaling factor */
    MYFLT c1, c2;   /* filter coefficients */
    MYFLT *out, *in, xn, yn, xnm1, xnm2, ynm1, ynm2;
    MYFLT kcf = *p->kcf, kbw = *p->kbw;
    int nsmps = csound->ksmps;

    r = (MYFLT)exp(-(double)(kbw * csound->pidsr));
    c1 = FL(2.0) * r * (MYFLT)cos((double)(csound->tpidsr*kcf));
    c2 = r * r;

    /* Normalizing factors derived from equations in Ken Steiglitz,
     * "A Note on Constant-Gain Digital Resonators," Computer
     * Music Journal, vol. 18, no. 4, pp. 8-10, Winter 1982.
     */
    if (p->scaletype == 1)
      scale = (FL(1.0) - c2) * FL(0.5);
    else if (p->scaletype == 2)
      scale = (MYFLT)sqrt((1.0 - (double)c2) * 0.5);
    else scale = FL(1.0);

    out  = p->out;
    in   = p->in;
    xnm1 = p->xnm1;
    xnm2 = p->xnm2;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    do {
      xn = *in++;
      *out++ = yn = scale * (xn - xnm2) + c1 * ynm1 - c2 * ynm2;
      xnm2 = xnm1;
      xnm1 = xn;
      ynm2 = ynm1;
      ynm1 = yn;
    } while (--nsmps);

    p->xnm1 = xnm1;
    p->xnm2 = xnm2;
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;
    return OK;
}

int phaser1set(ENVIRON *csound, PHASER1 *p)
{
    int j;
    int loop = (int) (*p->iorder + FL(0.5));
    if (!(*p->istor) || p->auxx.auxp == NULL || p->auxy.auxp == NULL) {
      csound->AuxAlloc(csound, (long)loop*sizeof(MYFLT), &p->auxx);
      csound->AuxAlloc(csound, (long)loop*sizeof(MYFLT), &p->auxy);
      p->xnm1 = (MYFLT *) p->auxx.auxp;
      p->ynm1 = (MYFLT *) p->auxy.auxp;
      for (j=0; j< loop; j++)
        p->xnm1[j] = p->ynm1[j] = FL(0.0);
    }
    else if (p->auxx.size < loop || p->auxy.size < loop ) {
                                /* Existing arrays too small so copy */
      AUXCH tmp1, tmp2;
      tmp1.auxp = tmp2.auxp = NULL;
      csound->AuxAlloc(csound, (long)loop*sizeof(MYFLT), &tmp1);
      csound->AuxAlloc(csound, (long)loop*sizeof(MYFLT), &tmp2);
      for (j=0; j< loop; j++) {
        ((MYFLT*)tmp1.auxp)[j] = p->xnm1[j];
        ((MYFLT*)tmp2.auxp)[j] = p->ynm1[j];
      }
      csound->Free(csound, p->auxx.auxp);       /* and fiddle it */
      csound->Free(csound, p->auxy.auxp);
      p->auxx = tmp1;
      p->auxy = tmp2;
      p->xnm1 = (MYFLT *) p->auxx.auxp;
      p->ynm1 = (MYFLT *) p->auxy.auxp;
    }
    p->loop = loop;
    return OK;
}

int phaser1(ENVIRON *csound, PHASER1 *p)
{
    MYFLT xn = FL(0.0), yn = FL(0.0);
    MYFLT *out, *in;
    MYFLT *xnm1, *ynm1, feedback;
    MYFLT coef = *p->kcoef, fbgain = *p->fbgain;
    MYFLT beta, wp;
    int nsmps = csound->ksmps;
    int i, j;

    xnm1 = p->xnm1;
    ynm1 = p->ynm1;
    feedback = p->feedback;
    out = p->out;
    in = p->in;

    if (coef <= FL(0.0))
      coef = -coef;     /* frequency will "fold over" if <= 0 Hz */
    /* next two lines implement bilinear z-transform, to convert
     * frequency value into a useable coefficient for the
     * allpass filters.
     */
    wp = csound->pidsr * coef;
    beta = (FL(1.0) - wp)/(FL(1.0) + wp);

    for (i=0; i<nsmps; i++) {
      xn = in[i] + feedback * fbgain;
      for (j=0; j < p->loop; j++) {
        /* Difference equation for 1st order
         * allpass filter */
        yn = beta * (xn + p->ynm1[j]) - p->xnm1[j];
        /* Stores state values in arrays */
        p->xnm1[j] = xn;
        p->ynm1[j] = yn;
        xn = yn;
      }
      out[i] = yn;
      feedback = yn;
    }
    p->feedback = feedback;
    return OK;
}

int phaser2set(ENVIRON *csound, PHASER2 *p)
{
    int modetype, j;
    int loop;

    p->modetype = modetype = (int)*p->mode;
    if (modetype && modetype != 1 && modetype != 2) {
      return csound->InitError(csound,
                               Str("Phaser mode must be either 1 or 2"));
    }

    loop = p->loop = (int) (*p->order + FL(0.5));
    csound->AuxAlloc(csound, (long)loop*sizeof(MYFLT), &p->aux1);
    csound->AuxAlloc(csound, (long)loop*sizeof(MYFLT), &p->aux2);
    p->nm1 = (MYFLT *) p->aux1.auxp;
    p->nm2 = (MYFLT *) p->aux2.auxp;
    for (j=0; j< loop; j++)
      p->nm1[j] = p->nm2[j] = FL(0.0);
    return OK;
}

int phaser2(ENVIRON *csound, PHASER2 *p)
{
    MYFLT xn = FL(0.0), yn = FL(0.0);
    MYFLT *out, *in;
    MYFLT kbf = *p->kbf, kq = *p->kbw;
    MYFLT ksep = *p->ksep, fbgain = *p->fbgain;
    MYFLT b, a, r, freq;
    MYFLT temp;
    MYFLT *nm1, *nm2, feedback;
    int nsmps = csound->ksmps;
    int j;

    nm1 = p->nm1;
    nm2 = p->nm2;
    feedback = p->feedback;
    out = p->out;
    in = p->in;

    /* frequency of first notch will "fold over" if <= 0 Hz */
    if (kbf <=0)
      kbf = -kbf;

    /* keeps ksep at a positive value. Otherwise, blow ups are
     * almost certain to happen.
     */
    if (ksep <= FL(0.0))
      ksep = -ksep;

    do {
      xn = *in++ + feedback * fbgain;
      /* The following code is used to determine
       * how the frequencies of the notches are calculated.
       * If imode=1, the notches will be in a harmonic
       * relationship of sorts. If imode=2, the frequencies
       * of the notches will be powers of the first notches.
       */
      for (j=0; j < p->loop; j++) {
        if (p->modetype == 1)
          freq = kbf + (kbf * ksep * j);
        else
          freq = kbf * csound->intpow(ksep,(long)j);
        /* Note similarities of following equations to
         * equations in resonr/resonz. The 2nd-order
         * allpass filter used here is similar to the
         * typical reson filter, with the addition of zeros.
         * The pole angle determines the frequency of the
         * notch, while the pole radius determines the q of
         * the notch.
         */
        r = (MYFLT)exp(-(double)(freq * csound->pidsr / kq));
        b = -FL(2.0) * r * (MYFLT)cos((double)(freq * csound->tpidsr));
        a = r * r;

        /* Difference equations for implementing canonical
         * 2nd order section. (Direct Form II)
         */
        temp = xn - b * p->nm1[j] - a * p->nm2[j];
        yn = a * temp + b * p->nm1[j] + nm2[j];
        p->nm2[j] = p->nm1[j];
        p->nm1[j] = temp;
        xn = yn;
      }
      *out++ = yn;
      feedback = yn;
    } while (--nsmps);
    p->feedback = feedback;
    return OK;
}

/* initialization for 2nd-order lowpass filter */
int lp2_set(ENVIRON *csound, LP2 *p)
{
    if (!(*p->istor))
      p->ynm1 = p->ynm2 = FL(0.0);
    return OK;
}

/* k-time code for 2nd-order lowpass filter. Derived from code in
Hal Chamberlin's "Musical Applications of Microprocessors." */
int lp2(ENVIRON *csound, LP2 *p)
{
    MYFLT a, b, c, temp;
    MYFLT *out, *in, yn, ynm1, ynm2;
    MYFLT kfco = *p->kfco, kres = *p->kres;
    int nsmps = csound->ksmps;
    int n;

    temp = csound->mpidsr * kfco / kres;
      /* (-PI_F * kfco / (kres * csound->esr)); */
    a = FL(2.0) * (MYFLT) (cos((double) (kfco * csound->tpidsr))
                           * exp((double) temp));
    b = (MYFLT)exp((double)(temp+temp));
    c = FL(1.0) - a + b;

    out  = p->out;
    in   = p->in;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    for (n=0; n<nsmps; n++) {
      out[n] = yn = a * ynm1 - b * ynm2 + c * in[n];
      ynm2 = ynm1;
      ynm1 = yn;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
{ "svfilter", S(SVF),    5, "aaa", "akko", (SUBR)svfset, NULL, (SUBR)svf     },
{ "hilbert", S(HILBERT), 5, "aa",  "a",    (SUBR)hilbertset, NULL, (SUBR)hilbert },
{ "resonr", S(RESONZ),   5, "a",   "akkoo", (SUBR)resonzset, NULL, (SUBR)resonr  },
{ "resonz", S(RESONZ),   5, "a",   "akkoo", (SUBR)resonzset, NULL, (SUBR)resonz  },
{ "lowpass2", S(LP2),    5, "a",   "akko",  (SUBR)lp2_set, NULL, (SUBR)lp2       },
{ "phaser2", S(PHASER2), 5, "a", "akkkkkk",(SUBR)phaser2set, NULL, (SUBR)phaser2 },
{ "phaser1", S(PHASER1), 5, "a",   "akkko", (SUBR)phaser1set, NULL, (SUBR)phaser1 }
};

LINKAGE

