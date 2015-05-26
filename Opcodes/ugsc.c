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

#include "stdopcod.h"
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

static int svfset(CSOUND *csound, SVF *p)
{
    if (*p->iskip) {
      /* set initial delay states to 0 */
      p->ynm1 = p->ynm2 = FL(0.0);
    }
    return OK;
}

static int svf(CSOUND *csound, SVF *p)
{
    MYFLT f1 = FL(0.0), q1 = FL(1.0), scale = FL(1.0),
          lfco = -FL(1.0), lq = -FL(1.0);
    MYFLT *low, *high, *band, *in, ynm1, ynm2;
    MYFLT low2, high2, band2;
    MYFLT *kfco = p->kfco, *kq = p->kq;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    in   = p->in;
    low  = p->low;
    band = p->band;
    high = p->high;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;


    /* equations derived from Hal Chamberlin, "Musical Applications
     * of Microprocessors.
     */
    if (UNLIKELY(offset)) {
      memset(low,  '\0', offset*sizeof(MYFLT));
      memset(high, '\0', offset*sizeof(MYFLT));
      memset(band, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&low[nsmps],  '\0', early*sizeof(MYFLT));
      memset(&high[nsmps], '\0', early*sizeof(MYFLT));
      memset(&band[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT fco = IS_ASIG_ARG(p->kfco) ? kfco[n] : *kfco;
      MYFLT q = IS_ASIG_ARG(p->kq) ? kq[n] : *kq;
      if (fco != lfco || q != lq) {
        lfco = fco; lq = q;
        /* calculate frequency and Q coefficients */
        f1 = FL(2.0) * (MYFLT)sin((double)(fco * csound->pidsr));
        /* Protect against division by zero */
        if (UNLIKELY(q<FL(0.000001))) q = FL(1.0);
        q1 = FL(1.0) / q;
        /* if there is a non-zero value for iscl, set scale to be
         * equal to the Q coefficient.
         */
        if (*p->iscl) scale = q1;
      }
      low[n]  = low2 = ynm2 + f1 * ynm1;
      high[n] = high2 = scale * in[n] - low2 - q1 * ynm1;
      band[n] = band2 = f1 * high2 + ynm1;
      ynm1    = band2;
      ynm2    = low2;
    }
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

static int hilbertset(CSOUND *csound, HILBERT *p)
{
    int j;  /* used to increment for loop */

    /* pole values taken from Bernie Hutchins, "Musical Engineer's Handbook" */
    double poles[12] = {0.3609, 2.7412, 11.1573, 44.7581, 179.6242, 798.4578,
                        1.2524, 5.5671, 22.3423, 89.6271, 364.7914, 2770.1114};
    double polefreq, rc, alpha, beta;
    /* calculate coefficients for allpass filters, based on sampling rate */
    for (j=0; j<12; j++) {
      /*      p->coef[j] = (1 - (15 * PI * pole[j]) / CS_ESR) /
              (1 + (15 * PI * pole[j]) / CS_ESR); */
      polefreq = poles[j] * 15.0;
      rc = 1.0 / (2.0 * PI * polefreq);
      alpha = 1.0 / rc;
      alpha = alpha * 0.5 * (double)csound->onedsr;
      beta = (1.0 - alpha) / (1.0 + alpha);
      p->xnm1[j] = p->ynm1[j] = FL(0.0);
      p->coef[j] = -(MYFLT)beta;
    }
    return OK;
}

static int hilbert(CSOUND *csound, HILBERT *p)
{
    MYFLT xn1, yn1, xn2, yn2;
    MYFLT *out1, *out2, *in;
    MYFLT *coef;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int j;

    coef = p->coef;
    out1 = p->out1;
    out2 = p->out2;
    in = p->in;

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      xn1 = in[n];
      /* 6th order allpass filter for sine output. Structure is
       * 6 first-order allpass sections in series. Coefficients
       * taken from arrays calculated at i-time.
       */
      for (j=0; j < 6; j++) {
        yn1 = coef[j] * (xn1 - p->ynm1[j]) + p->xnm1[j];
        p->xnm1[j] = xn1;
        p->ynm1[j] = yn1;
        xn1 = yn1;
      }
      xn2 = in[n];
      /* 6th order allpass filter for cosine output. Structure is
       * 6 first-order allpass sections in series. Coefficients
       * taken from arrays calculated at i-time.
       */
      for (j=6; j < 12; j++) {
        yn2 = coef[j] * (xn2 - p->ynm1[j]) + p->xnm1[j];
        p->xnm1[j] = xn2;
        p->ynm1[j] = yn2;
        xn2 = yn2;
      }
      out1[n] = yn2;
      out2[n] = yn1;
    }
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

static int resonzset(CSOUND *csound, RESONZ *p)
{
    /* error message code derived from code for reson in ugens5.c */
    int scaletype;
    p->scaletype = scaletype = (int)*p->iscl;
    if (UNLIKELY(UNLIKELY(scaletype && scaletype != 1 && scaletype != 2))) {
      return csound->InitError(csound, Str("illegal reson iscl value, %f"),
                               (float)*p->iscl);
    }
    if (!(*p->istor))
      p->xnm1 = p->xnm2 = p->ynm1 = p->ynm2 = 0.0;
    return OK;
}

static int resonr(CSOUND *csound, RESONZ *p)
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

    double r = 0.0, scale = 1.0; /* radius & scaling factor */
    double c1=0.0, c2=0.0;   /* filter coefficients */
    MYFLT *out, *in;
    double xn, yn, xnm1, xnm2, ynm1, ynm2;
    MYFLT *kcf = p->kcf, *kbw = p->kbw;
    MYFLT lcf = -FL(1.0), lbw = -FL(1.0);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    out = p->out;
    in = p->in;
    xnm1 = p->xnm1;
    xnm2 = p->xnm2;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT cf = IS_ASIG_ARG(p->kcf) ? kcf[n] : *kcf;
      MYFLT bw = IS_ASIG_ARG(p->kbw) ? kbw[n] : *kbw;
      if (cf != lcf || bw != lbw) {
        lcf = cf; lbw = bw;
        r = exp((double)(bw * csound->mpidsr));
        c1 = 2.0 * r * cos((double)(cf * csound->tpidsr));
        c2 = r * r;
        if (p->scaletype == 1)
          scale = 1.0 - r;
        else if (p->scaletype == 2)
          scale = sqrt(1.0 - r);
      }
      xn = (double)in[n];
      out[n] = (MYFLT)(yn = scale * (xn - r * xnm2) + c1 * ynm1 - c2 * ynm2);
      xnm2 = xnm1;
      xnm1 = xn;
      ynm2 = ynm1;
      ynm1 = yn;
    }
    p->xnm1 = xnm1;
    p->xnm2 = xnm2;
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;
    return OK;
}

static int resonz(CSOUND *csound, RESONZ *p)
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

    double r = 0.0, scale = 1.0; /* radius & scaling factor */
    double c1=0.0, c2=0.0;   /* filter coefficients */
    MYFLT *out, *in;
    double xn, yn, xnm1, xnm2, ynm1, ynm2;
    MYFLT *kcf = p->kcf, *kbw = p->kbw;
    MYFLT lcf = -FL(1.0), lbw = -FL(1.0);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    /* Normalizing factors derived from equations in Ken Steiglitz,
     * "A Note on Constant-Gain Digital Resonators," Computer
     * Music Journal, vol. 18, no. 4, pp. 8-10, Winter 1982.
     */

    out  = p->out;
    in   = p->in;
    xnm1 = p->xnm1;
    xnm2 = p->xnm2;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT cf = IS_ASIG_ARG(p->kcf) ? kcf[n] : *kcf;
      MYFLT bw = IS_ASIG_ARG(p->kbw) ? kbw[n] : *kbw;
      if (cf != lcf || bw != lbw) {
        lcf = cf; lbw = bw;
        r = exp(-(double)(bw * csound->pidsr));
        c1 = 2.0 * r * cos((double)(csound->tpidsr*cf));
        c2 = r * r;
        if (p->scaletype == 1)
          scale = (1.0 - c2) * 0.5;
        else if (p->scaletype == 2)
          scale = sqrt((1.0 - c2) * 0.5);
      }
      xn = (double)in[n];
      out[n] = (MYFLT)(yn = scale * (xn - xnm2) + c1 * ynm1 - c2 * ynm2);
      xnm2 = xnm1;
      xnm1 = xn;
      ynm2 = ynm1;
      ynm1 = yn;
    }

    p->xnm1 = xnm1;
    p->xnm2 = xnm2;
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;
    return OK;
}

static int phaser1set(CSOUND *csound, PHASER1 *p)
{
    int   loop = (int) MYFLT2LONG(*p->iorder);
    int32  nBytes = (int32) loop * (int32) sizeof(MYFLT);

    if (*p->istor == FL(0.0) || p->auxx.auxp == NULL ||
        (int)p->auxx.size<nBytes || p->auxy.auxp == NULL ||
        (int)p->auxy.size<nBytes) {
      csound->AuxAlloc(csound, nBytes, &p->auxx);
      csound->AuxAlloc(csound, nBytes, &p->auxy);
      p->xnm1 = (MYFLT *) p->auxx.auxp;
      p->ynm1 = (MYFLT *) p->auxy.auxp;
    }
    else if ((int32) p->auxx.size < nBytes || (int32) p->auxy.size < nBytes) {
      /* Existing arrays too small so copy */
      void    *tmp1, *tmp2;
      size_t  oldSize1 = (size_t) p->auxx.size;
      size_t  oldSize2 = (size_t) p->auxy.size;
      tmp1 = malloc(oldSize1 + oldSize2);
      tmp2 = (char*) tmp1 + (int) oldSize1;
      memcpy(tmp1, p->auxx.auxp, oldSize1);
      memcpy(tmp2, p->auxy.auxp, oldSize2);
      csound->AuxAlloc(csound, nBytes, &p->auxx);
      csound->AuxAlloc(csound, nBytes, &p->auxy);
      memcpy(p->auxx.auxp, tmp1, oldSize1);
      memcpy(p->auxy.auxp, tmp2, oldSize2);
      free(tmp1);
      p->xnm1 = (MYFLT *) p->auxx.auxp;
      p->ynm1 = (MYFLT *) p->auxy.auxp;
    }
    p->loop = loop;
    return OK;
}

static int phaser1(CSOUND *csound, PHASER1 *p)
{
    MYFLT xn = FL(0.0), yn = FL(0.0);
    MYFLT *out, *in;
    MYFLT feedback;
    MYFLT coef = FABS(*p->kcoef), fbgain = *p->fbgain;
    MYFLT beta, wp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int j;

    feedback = p->feedback;
    out = p->out;
    in = p->in;

    //if (coef<=FL(0.0)) coef = -coef; /* frequency will "fold over" if <= 0 Hz */
    /* next two lines implement bilinear z-transform, to convert
     * frequency value into a useable coefficient for the
     * allpass filters.
     */
    wp = csound->pidsr * coef;
    beta = (FL(1.0) - wp)/(FL(1.0) + wp);

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i=offset; i<nsmps; i++) {
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

static int phaser2set(CSOUND *csound, PHASER2 *p)
{
    int modetype;
    int loop;

    p->modetype = modetype = (int)*p->mode;
    if (UNLIKELY(UNLIKELY(modetype && modetype != 1 && modetype != 2))) {
      return csound->InitError(csound,
                               Str("Phaser mode must be either 1 or 2"));
    }

    loop = p->loop = (int) MYFLT2LONG(*p->order);
    csound->AuxAlloc(csound, (size_t)loop*sizeof(MYFLT), &p->aux1);
    csound->AuxAlloc(csound, (size_t)loop*sizeof(MYFLT), &p->aux2);
    p->nm1 = (MYFLT *) p->aux1.auxp;
    p->nm2 = (MYFLT *) p->aux2.auxp;
    /* *** This is unnecessary as AuxAlloc zeros *** */
    /* for (j=0; j< loop; j++) */
    /*   p->nm1[j] = p->nm2[j] = FL(0.0); */
    return OK;
}

static int phaser2(CSOUND *csound, PHASER2 *p)
{
    MYFLT xn = FL(0.0), yn = FL(0.0);
    MYFLT *out, *in;
    MYFLT kbf = *p->kbf, kq = *p->kbw;
    MYFLT ksep = *p->ksep, fbgain = *p->fbgain;
    MYFLT b, a, r, freq;
    MYFLT temp;
    MYFLT *nm1, *nm2, feedback;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
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

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT kk = FL(1.0);
      xn = in[n] + feedback * fbgain;
      /* The following code is used to determine
       * how the frequencies of the notches are calculated.
       * If imode=1, the notches will be in a harmonic
       * relationship of sorts. If imode=2, the frequencies
       * of the notches will be powers of the first notches.
       */
      for (j=0; j < p->loop; j++) {
        if (p->modetype == 1)
          freq = kbf + (kbf * ksep * j);
        else {
          freq = kbf * kk;
          kk *= ksep;
          //freq = kbf * csound->intpow(ksep,(int32)j);
        }
        /* Note similarities of following equations to
         * equations in resonr/resonz. The 2nd-order
         * allpass filter used here is similar to the
         * typical reson filter, with the addition of zeros.
         * The pole angle determines the frequency of the
         * notch, while the pole radius determines the q of
         * the notch.
         */
        r = EXP(-(freq * csound->pidsr / kq));
        b = -FL(2.0) * r * COS(freq * csound->tpidsr);
        a = r * r;

        /* Difference equations for implementing canonical
         * 2nd order section. (Direct Form II)
         */
        temp = xn - b * nm1[j] - a * nm2[j];
        yn = a * temp + b * nm1[j] + nm2[j];
        nm2[j] = nm1[j];
        nm1[j] = temp;
        xn = yn;
      }
      out[n] = yn;
      feedback = yn;
    }
    p->feedback = feedback;
    return OK;
}

/* initialization for 2nd-order lowpass filter */
static int lp2_set(CSOUND *csound, LP2 *p)
{
    if (!(*p->istor))
      p->ynm1 = p->ynm2 = 0.0;
    return OK;
}

/* k-time code for 2nd-order lowpass filter. Derived from code in
Hal Chamberlin's "Musical Applications of Microprocessors." */
static int lp2(CSOUND *csound, LP2 *p)
{
    double a, b, c, temp;
    MYFLT *out, *in;
    double yn, ynm1, ynm2;
    MYFLT kfco = *p->kfco, kres = *p->kres;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    temp = (double)(csound->mpidsr * kfco / kres);
      /* (-PI_F * kfco / (kres * CS_ESR)); */
    a = 2.0 * cos((double) (kfco * csound->tpidsr)) * exp(temp);
    b = exp(temp+temp);
    c = 1.0 - a + b;

    out  = p->out;
    in   = p->in;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      out[n] = (MYFLT)(yn = a * ynm1 - b * ynm2 + c * (double)in[n]);
      ynm2 = ynm1;
      ynm1 = yn;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;
    return OK;
}

static int lp2aa(CSOUND *csound, LP2 *p)
{
    double a, b, c, temp;
    MYFLT *out, *in;
    double yn, ynm1, ynm2;
    MYFLT *fcop = p->kfco, *resp = p->kres;
    MYFLT fco = fcop[0], res = resp[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    temp = (double)(csound->mpidsr * fco / res);
      /* (-PI_F * kfco / (kres * CS_ESR)); */
    a = 2.0 * cos((double) (fco * csound->tpidsr)) * exp(temp);
    b = exp(temp+temp);
    c = 1.0 - a + b;

    out  = p->out;
    in   = p->in;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      if (res!=resp[n] || fco!=fcop[n]) {
        res=resp[n]; fco=fcop[n];
        temp = (double)(csound->mpidsr * fco / res);
        /* (-PI_F * kfco / (kres * CS_ESR)); */
        a = 2.0 * cos((double) (fco * csound->tpidsr)) * exp(temp);
        b = exp(temp+temp);
        c = 1.0 - a + b;
      }
      out[n] = (MYFLT)(yn = a * ynm1 - b * ynm2 + c * (double)in[n]);
      ynm2 = ynm1;
      ynm1 = yn;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;
    return OK;
}

static int lp2ka(CSOUND *csound, LP2 *p)
{
    double a, b, c, temp;
    MYFLT *out, *in;
    double yn, ynm1, ynm2;
    MYFLT *resp = p->kres;
    MYFLT fco = *p->kfco, res = resp[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    temp = (double)(csound->mpidsr * fco / res);
      /* (-PI_F * kfco / (kres * CS_ESR)); */
    a = 2.0 * cos((double) (fco * csound->tpidsr)) * exp(temp);
    b = exp(temp+temp);
    c = 1.0 - a + b;

    out  = p->out;
    in   = p->in;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      if (res!=resp[n]) {
        res=resp[n];
        temp = (double)(csound->mpidsr * fco / res);
        /* (-PI_F * kfco / (kres * CS_ESR)); */
        a = 2.0 * cos((double) (fco * csound->tpidsr)) * exp(temp);
        b = exp(temp+temp);
        c = 1.0 - a + b;
      }
      out[n] = (MYFLT)(yn = a * ynm1 - b * ynm2 + c * (double)in[n]);
      ynm2 = ynm1;
      ynm1 = yn;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;
    return OK;
}

static int lp2ak(CSOUND *csound, LP2 *p)
{
    double a, b, c, temp;
    MYFLT *out, *in;
    double yn, ynm1, ynm2;
    MYFLT *fcop = p->kfco;
    MYFLT fco = fcop[0], res = *p->kres;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    temp = (double)(csound->mpidsr * fco / res);
      /* (-PI_F * kfco / (kres * CS_ESR)); */
    a = 2.0 * cos((double) (fco * csound->tpidsr)) * exp(temp);
    b = exp(temp+temp);
    c = 1.0 - a + b;

    out  = p->out;
    in   = p->in;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      if (fco!=fcop[n]) {
        fco=fcop[n];
        temp = (double)(csound->mpidsr * fco / res);
        /* (-PI_F * kfco / (kres * CS_ESR)); */
        a = 2.0 * cos((double) (fco * csound->tpidsr)) * exp(temp);
        b = exp(temp+temp);
        c = 1.0 - a + b;
      }
      out[n] = (MYFLT)(yn = a * ynm1 - b * ynm2 + c * (double)in[n]);
      ynm2 = ynm1;
      ynm1 = yn;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "svfilter", S(SVF),    0, 5, "aaa", "axxoo", (SUBR)svfset, NULL, (SUBR)svf    },
{ "hilbert", S(HILBERT), 0,5, "aa", "a", (SUBR)hilbertset, NULL, (SUBR)hilbert },
{ "resonr", S(RESONZ),   0,5, "a", "axxoo", (SUBR)resonzset, NULL, (SUBR)resonr},
{ "resonz", S(RESONZ),   0,5, "a", "axxoo", (SUBR)resonzset, NULL, (SUBR)resonz},
{ "lowpass2.kk", S(LP2), 0,5, "a", "akko",  (SUBR)lp2_set, NULL, (SUBR)lp2     },
{ "lowpass2.aa", S(LP2), 0,5, "a", "aaao",  (SUBR)lp2_set, NULL, (SUBR)lp2aa   },
{ "lowpass2.ak", S(LP2), 0,5, "a", "aakao", (SUBR)lp2_set, NULL, (SUBR)lp2ak   },
{ "lowpass2.ka", S(LP2), 0,5, "a", "akao",  (SUBR)lp2_set, NULL, (SUBR)lp2ka   },
{ "phaser2", S(PHASER2), 0,5, "a", "akkkkkk",(SUBR)phaser2set,NULL,(SUBR)phaser2},
{ "phaser1", S(PHASER1), 0,5, "a", "akkko", (SUBR)phaser1set, NULL,(SUBR)phaser1}
};

int ugsc_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

