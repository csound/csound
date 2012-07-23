/*  shape.c

    A Csound opcode library implementing ugens
    for various waveshaping methods.

    Anthony Kozar
    January 5, 2004
    November 2, 2007: Ported to Csound 5.

    Copyright (C) 2004,2007  Anthony M. Kozar Jr.

    Code for syncphasor based on phasor from ugens2.c.
    Copyright (C) 1991-2007 Barry Vercoe, John ffitch, Robin Whittle

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

#include "csoundCore.h"
#include "interlocks.h"

#include <math.h>


typedef struct {
    OPDS    h;
    MYFLT   *aout, *ain, *kshapeamount, *ifullscale;
    MYFLT   maxamplitude, one_over_maxamp;
} POWER_SHAPE;

static int PowerShapeInit(CSOUND* csound, POWER_SHAPE* data)
{
    data->maxamplitude = *data->ifullscale;
    if (UNLIKELY(data->maxamplitude<= 0.0))
      return
        csound->InitError(csound,
                          Str("powershape: ifullscale must be strictly positive"));
    data->one_over_maxamp = FL(1.0) / data->maxamplitude;
    return OK;
}

static int PowerShape(CSOUND* csound, POWER_SHAPE* data)
{
    MYFLT     cur, amt, maxampl, invmaxampl;
    int       n, nsmps = csound->ksmps;
    MYFLT*    out = data->aout;
    MYFLT*    in = data->ain;

    amt = *(data->kshapeamount);
    invmaxampl = data->one_over_maxamp;
    if (amt == FL(0.0)) {                    /* treat zero-power with care */
      for (n=0; n<nsmps; n++) {
        cur = in[n];
        if (cur == FL(0.0))
          out[n] = FL(0.0);               /* make 0^0 = 0 for continuity */
        else
          out[n] = FL(1.0)/invmaxampl;    /* otherwise, x^0 = 1.0 */
      }
    }
    else {
      maxampl = data->maxamplitude;
      for (n=0; n<nsmps; n++) {
        cur = in[n] * invmaxampl;
        if (cur < FL(0.0))                /* treat negatives with care */
          /* make output negative to avoid DC offsets and preserve continuity */
          /* with even powers */
          out[n] = - POWER(-cur,amt) * maxampl;
        else
          out[n] = POWER(cur,amt) * maxampl;
      }
    }
    return OK;
}

typedef struct {
    OPDS    h;
    MYFLT   *aout, *ain, *kcoefficients[VARGMAX];
} POLYNOMIAL;

/* Efficiently evaluates a polynomial of arbitrary order --   */
/* coefficients are k-rate and in this order: a0, a1, a2, ... */
static int Polynomial(CSOUND* csound, POLYNOMIAL* data)
{
    int   i;
    int   n, nsmps = csound->ksmps;
    int   ncoeff =    /* index of the last coefficient */
                   csound->GetInputArgCnt(data) - 2;
    MYFLT *out = data->aout;
    MYFLT *in = data->ain;
    MYFLT **coeff = data->kcoefficients;
    MYFLT sum, x;

    for (n=0; n<nsmps; n++) {
      x = in[n];
      sum = *coeff[ncoeff];
      for (i = ncoeff-1; i >= 0; --i) {
        sum *= x;
        sum += *coeff[i];
      }
      out[n] = sum;
    }

    return OK;
}

typedef struct {
    OPDS    h;
    MYFLT   *aout, *ain, *kcoefficients[VARGMAX];
    MYFLT   *chebn;
    AUXCH   coeff;
} CHEBPOLY;

static int ChebyshevPolyInit(CSOUND* csound, CHEBPOLY* data)
{
    int     ncoeff = csound->GetInputArgCnt(data) - 1;

    /* Need two MYFLT arrays of length ncoeff: first for the coefficients
       of the sum of polynomials, and the second for the coefficients of
       the individual chebyshev polynomials as we are adding them up. */
    csound->AuxAlloc(csound, (2*ncoeff + 1)*sizeof(MYFLT), &(data->coeff));
    data->chebn = ((MYFLT*)data->coeff.auxp) + ncoeff;
    return OK;
}

/* Efficiently evaluates a sum of Chebyshev polynomials of arbitrary order.
   Coefficients (k0, k1, k2, ... ) are k-rate multipliers of each Chebyshev
   polynomial starting with the zero-order polynomial T0(x) = 1, then
   T1(x) = x, T2(x) = 2x^2 - 1, etc. */
static int ChebyshevPolynomial(CSOUND* csound, CHEBPOLY* data)
{
    int     i, j;
    int     n, nsmps = csound->ksmps;
    int     ncoeff =            /* index of the last coefficient */
                     csound->GetInputArgCnt(data) - 2;
    MYFLT   *out = data->aout;
    MYFLT   *in = data->ain;
    MYFLT   **chebcoeff = data->kcoefficients;
    MYFLT   *chebn = data->chebn;
    MYFLT   *coeff = (MYFLT*)data->coeff.auxp;
    MYFLT   sum, x;

    /* Every other coefficient in a Cheb. poly. is 0, and
    these zero coeff. alternate positions in successive
    polynomials. So we store two consecutive Cheb. poly.
    in the same array with the odd coeff. making up one,
    and the even coeff. the other. */
    /* Initialize the Cheb. poly. array with first two C.P. */
    chebn[0] = chebn[1] = FL(1.0);
    for (i = 2; i <= ncoeff; ++i)  chebn[i] = FL(0.0);
    /* Calculate the final coefficients array */
    coeff[0] = chebcoeff[0][0];
    if (ncoeff > 0) coeff[1] = chebcoeff[1][0];
    if (ncoeff > 1) {
      for (i = 2; i <= ncoeff; ++i)  coeff[i] = FL(0.0);
      /* Calculate successive Cheb. poly. while accumulating
         our final poly. coefficients based on input Cheb. coeff. */
      for (i = 2; i <= ncoeff; i+=2) {
        /* do the next even Cheb. poly. */
        chebn[0] = -chebn[0];
        coeff[0] += chebn[0] * chebcoeff[i][0];
        for (j = 2; j <= ncoeff; j+=2) {
          chebn[j] = 2*chebn[j-1] - chebn[j];
          coeff[j] += chebn[j] * chebcoeff[i][0];
        }
        /* do the next odd Cheb. poly. if needed */
        if (ncoeff > i) {
          for (j = 1; j <= ncoeff; j+=2) {
            chebn[j] = 2*chebn[j-1] - chebn[j];
            coeff[j] += chebn[j] * chebcoeff[i+1][0];
          }
        }
      }
    }

    /* Use our final coeff. to evaluate the poly. for each input sample */
    for (n=0; n<nsmps; n++) {
      x = in[n];
      sum = coeff[ncoeff];
      for (i = ncoeff-1; i >= 0 ; --i) {
        sum *= x;
        sum += coeff[i];
      }
      out[n] = sum;
    }

    return OK;
}


/* Phase distortion opcodes */

typedef struct {
    OPDS    h;
    MYFLT   *aout, *ain, *kwidth, *kcenter, *ibipolar, *ifullscale;
} PD_CLIP;

static int PDClip(CSOUND* csound, PD_CLIP* data)
{
    MYFLT     cur, low, high, maxampl, width, unwidth, center, outscalar;
    int       bipolarMode, n, nsmps = csound->ksmps;
    MYFLT*    out = data->aout;
    MYFLT*    in = data->ain;

    bipolarMode = (int) *(data->ibipolar);
    maxampl = *(data->ifullscale);
    width = (*(data->kwidth) > FL(1.0) ? FL(1.0) :
             (*(data->kwidth) < FL(0.0) ? FL(0.0) : *(data->kwidth)));
    unwidth = FL(1.0) - width;           /* width of 1/2 unclipped region */

    if (bipolarMode) {
      /* the unclipped region can only shift left or right by up to width */
      if      (*(data->kcenter) < - width) center = - width * maxampl;
      else if (*(data->kcenter) > width)   center = width * maxampl;
      else                                 center = *(data->kcenter) * maxampl;
    }
    else {
      width = FL(0.5) * width;   /* range reduced by 1/2 in unipolar mode */
      unwidth = FL(0.5) * unwidth;
      center = *(data->kcenter) * FL(0.5) + FL(0.5);     /* make unipolar */
      if      (center < (FL(0.5) - width)) center = (FL(0.5) - width) * maxampl;
      else if (center > (FL(0.5) + width)) center = (FL(0.5) + width) * maxampl;
      else                                 center = center * maxampl;
    }
    low = center - unwidth*maxampl;       /* min value of unclipped input */
    high = unwidth*maxampl + center;      /* max value of unclipped input */

    if (bipolarMode) {
      outscalar = (unwidth == FL(0.0)) ? FL(0.0) : (FL(1.0) / unwidth);
      for (n=0; n<nsmps; n++) {
        cur = in[n];
        out[n] = (cur <= low ? -maxampl :
                  (cur >= high ? maxampl : (outscalar * (cur-center))));
      }
    }
    else {
      outscalar = (unwidth == FL(0.0)) ? FL(0.0) : (FL(0.5) / unwidth);
      for (n=0; n<nsmps; n++) {
        cur = in[n];
        out[n] = (cur <= low ? FL(0.0) :
                  (cur >= high ? maxampl : (outscalar * (cur-low))));
      }
    }

    return OK;
}

typedef struct {
    OPDS    h;
    MYFLT   *aout, *ain, *kamount, *ibipolar, *ifullscale;
} PD_HALF;

/* Casio-style phase distortion with "pivot point" on the X axis */
static int PDHalfX(CSOUND* csound, PD_HALF* data)
{
    MYFLT     cur, maxampl, midpoint, leftslope, rightslope;
    int       n, nsmps = csound->ksmps;
    MYFLT*    out = data->aout;
    MYFLT*    in = data->ain;

    maxampl = *(data->ifullscale);
    if (maxampl == FL(0.0))  maxampl = FL(1.0);

    if (*(data->ibipolar) != FL(0.0)) {    /* bipolar mode */
      /* clamp kamount in range [-1,1] */
      midpoint = (*(data->kamount) >= FL(1.0) ? maxampl :
                  (*(data->kamount) <= FL(-1.0) ? -maxampl :
                   (*(data->kamount) * maxampl)));

      if (midpoint != -maxampl) leftslope  = maxampl / (midpoint + maxampl);
      else                      leftslope  = FL(0.0);
      if (midpoint != maxampl)  rightslope = maxampl / (maxampl - midpoint);
      else                      rightslope = FL(0.0);

      for (n=0; n<nsmps; n++) {
        cur = in[n];
        if (cur < midpoint) out[n] = leftslope * (cur - midpoint);
        else                out[n] = rightslope * (cur - midpoint);
      }
    }
    else {  /* unipolar mode */
      MYFLT  halfmaxampl = FL(0.5) * maxampl;

      /* clamp kamount in range [-1,1] and make unipolar */
      midpoint = (*(data->kamount) >= FL(1.0) ? maxampl :
                  (*(data->kamount) <= FL(-1.0) ? FL(0.0) :
                   ((*(data->kamount) + FL(1.0)) * halfmaxampl)));

      if (midpoint != FL(0.0)) leftslope  = halfmaxampl / midpoint;
      else                     leftslope  = FL(0.0);
      if (midpoint != maxampl) rightslope = halfmaxampl / (maxampl - midpoint);
      else                     rightslope = FL(0.0);

      for (n=0; n<nsmps; n++) {
        cur = in[n];
        if (cur < midpoint) out[n] = leftslope * cur;
        else                out[n] = rightslope*(cur - midpoint) + halfmaxampl;
      }
    }

    return OK;
}

/* Casio-style phase distortion with "pivot point" on the Y axis */
static int PDHalfY(CSOUND* csound, PD_HALF* data)
{
    MYFLT     cur, maxampl, midpoint, leftslope, rightslope;
    int       n, nsmps = csound->ksmps;
    MYFLT*    out = data->aout;
    MYFLT*    in = data->ain;

    maxampl = *(data->ifullscale);
    if (maxampl == FL(0.0))  maxampl = FL(1.0);

    if (*(data->ibipolar) != FL(0.0)) {    /* bipolar mode */
      /* clamp kamount in range [-1,1] */
      midpoint = (*(data->kamount) > FL(1.0) ? maxampl :
                  (*(data->kamount) < FL(-1.0) ? -maxampl :
                   (*(data->kamount) * maxampl)));

      leftslope  = (midpoint + maxampl) / maxampl;
      rightslope = (maxampl - midpoint) / maxampl;

      for (n=0; n<nsmps; n++) {
        cur = in[n];
        if (cur < FL(0.0)) out[n] = leftslope*cur + midpoint;
        else               out[n] = rightslope*cur + midpoint;
      }
    }
    else {  /* unipolar mode */
      MYFLT  halfmaxampl = FL(0.5) * maxampl;

      /* clamp kamount in range [-1,1] and make unipolar */
      midpoint = (*(data->kamount) >= FL(1.0) ? maxampl :
                  (*(data->kamount) <= FL(-1.0) ? FL(0.0) :
                   ((*(data->kamount) + FL(1.0)) * halfmaxampl)));

      leftslope  = midpoint / halfmaxampl;
      rightslope = (maxampl - midpoint) / halfmaxampl;

      for (n=0; n<nsmps; n++) {
        cur = in[n];
        if (cur < halfmaxampl)
              out[n] = leftslope * cur;
        else  out[n] = rightslope*(cur - halfmaxampl) + midpoint;
      }
    }
    return OK;
}


/* syncphasor: a phasor opcode with sync input and output */
/* Code for syncphasor based on phasor from ugens2.c in Csound */

typedef struct {
    OPDS    h;
    MYFLT   *aphase, *asyncout, *xcps, *asyncin, *initphase;
    double  curphase;
} SYNCPHASOR;

int SyncPhasorInit(CSOUND *csound, SYNCPHASOR *p)
{
    MYFLT  phs;
    int32   longphs;

    if ((phs = *p->initphase) >= FL(0.0)) {
      if (UNLIKELY((longphs = (int32)phs))) {
        csound->Warning(csound, Str("init phase truncation\n"));
      }
      p->curphase = phs - (MYFLT)longphs;
    }
    return OK;
}

int SyncPhasor(CSOUND *csound, SYNCPHASOR *p)
{
    double      phase;
    int         n, nsmps=csound->ksmps;
    MYFLT       *out, *syncout, *syncin;
    double      incr;
    int         cpsIsARate;

    out = p->aphase;
    syncout = p->asyncout;
    syncin = p->asyncin;
    phase = p->curphase;
    cpsIsARate = (csound->GetInputArgAMask(p) & 1); /* check first input arg rate */
    if (cpsIsARate) {
      MYFLT *cps = p->xcps;
      for (n=0; n<nsmps; n++) {
        if (syncin[n] != FL(0.0)) {        /* non-zero triggers reset */
          phase = 0.0;
          out[n] = (MYFLT)phase;
          syncout[n] = FL(1.0);        /* send sync whenever syncin */
        }
        else {
          incr = (double)(cps[n] * csound->onedsr);
          out[n] = (MYFLT)phase;
          phase += incr;
          if (phase >= 1.0) {
            phase -= 1.0;
            syncout[n] = FL(1.0);        /* send sync when phase wraps */
          }
          else if (phase < 0.0) {
            phase += 1.0;
            syncout[n] = FL(1.0);
          }
          else syncout[n] = FL(0.0);
        }
      }
    }
    else {
      incr = (double)(*p->xcps * csound->onedsr);
      for (n=0; n<nsmps; n++) {
        if (syncin[n] != FL(0.0)) {        /* non-zero triggers reset */
          phase = 0.0;
          out[n] = (MYFLT)phase;
          syncout[n] = FL(1.0);        /* send sync whenever syncin */
        }
        else {
          out[n] = (MYFLT)phase;
          phase += incr;
          if (phase >= 1.0) {
            phase -= 1.0;
            syncout[n] = FL(1.0);        /* send sync when phase wraps */
          }
          else if (phase < 0.0) {
            phase += 1.0;
            syncout[n] = FL(1.0);
          }
          else syncout[n] = FL(0.0);
        }
      }
    }
    p->curphase = phase;
    return OK;
}


/* phasine is an experimental opcode that needs to be reconceptualized
   because it produces many discontinuities. */

#if 0
typedef struct {
    OPDS    h;
    MYFLT   *aout, *ain, *kphaseadjust, *ifullscale;
    MYFLT   lastin, maxamplitude;
} PHASINE;

static int PhasineInit(CSOUND* csound, PHASINE* data)
{
    data->lastin = FL(0.0);
    data->maxamplitude = *data->ifullscale;
    return OK;
}

static int Phasine(CSOUND* csound, PHASINE* data)
{
    MYFLT     last, cur, phase, adjust, maxampl;
    int       n, nsmps = csound->ksmps;
    MYFLT*    out = data->aout;
    MYFLT*    in = data->ain;

    adjust = *(data->kphaseadjust);
    last = data->lastin;
    maxampl = data->maxamplitude;
    for (n=0; n<nsmps; n++) {
      cur = in[n];
      phase = ASIN(cur/maxampl) / PI_F; /* current "phase" value */
      if (last < cur) {
        if (phase < 0)  phase = (FL(-1.0) - phase);
        else            phase = (FL(1.0) - phase);
      }
      last = cur;

      phase += adjust;                        /* new phase value */
      /* wrap phase if using a lookup table */
      out[n] = SIN(phase*PI_F) * maxampl;
    }

    data->lastin = last;
    return OK;

}
#endif

/* code for linking dynamic libraries under Csound 5 */

#define S(x)    sizeof(x)

static OENTRY shape_localops[] = {
  /* { "phasine", S(PHASINE), 5, "a", "akp",
                        (SUBR)PhasineInit, NULL, (SUBR)Phasine }, */
  { "powershape", S(POWER_SHAPE), 5, "a", "akp",
                        (SUBR)PowerShapeInit, NULL, (SUBR)PowerShape },
  { "polynomial", S(POLYNOMIAL), 4, "a", "az", NULL, NULL, (SUBR)Polynomial },
  { "chebyshevpoly", S(CHEBPOLY), 5, "a", "az",
                     (SUBR)ChebyshevPolyInit, NULL, (SUBR)ChebyshevPolynomial },
  { "pdclip", S(PD_CLIP), 4, "a", "akkop", NULL, NULL, (SUBR)PDClip },
  { "pdhalf", S(PD_HALF), 4, "a", "akop", NULL, NULL, (SUBR)PDHalfX },
  { "pdhalfy", S(PD_HALF), 4, "a", "akop", NULL, NULL, (SUBR)PDHalfY },
  { "syncphasor", S(SYNCPHASOR), 5, "aa", "xao",
                  (SUBR)SyncPhasorInit, NULL, (SUBR)SyncPhasor },
};

LINKAGE1(shape_localops)
