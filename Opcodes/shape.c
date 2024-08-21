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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
 */

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "interlocks.h"

#include <math.h>


typedef struct {
    OPDS    h;
    MYFLT   *aout, *ain, *kshapeamount, *ifullscale;
    MYFLT   maxamplitude, one_over_maxamp;
} POWER_SHAPE;

static int32_t PowerShapeInit(CSOUND* csound, POWER_SHAPE* p)
{
    p->maxamplitude = *p->ifullscale;
    if (UNLIKELY(p->maxamplitude<= 0.0))
      return
        csound->InitError(csound,
                          "%s", Str("powershape: ifullscale must be strictly positive"));
    p->one_over_maxamp = FL(1.0) / p->maxamplitude;
    return OK;
}

static int32_t PowerShape(CSOUND* csound, POWER_SHAPE* p)
{
    IGN(csound);
    MYFLT     cur, amt, maxampl, invmaxampl;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t  early  = p->h.insdshead->ksmps_no_end;
    uint32_t  n, nsmps = CS_KSMPS;
    MYFLT*    out = p->aout;
    MYFLT*    in = p->ain;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    amt = *(p->kshapeamount);
    invmaxampl = p->one_over_maxamp;
    if (amt == FL(0.0)) {                    /* treat zero-power with care */
      for (n=offset; n<nsmps; n++) {
        cur = in[n];
        if (cur == FL(0.0))
          out[n] = FL(0.0);               /* make 0^0 = 0 for continuity */
        else
          out[n] = FL(1.0)/invmaxampl;    /* otherwise, x^0 = 1.0 */
      }
    }
    else {
      maxampl = p->maxamplitude;
      for (n=offset; n<nsmps; n++) {
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
    MYFLT   *aout, *ain, *kcoefficients[VARGMAX-1];
} POLYNOMIAL;

/* Efficiently evaluates a polynomial of arbitrary order --   */
/* coefficients are k-rate and in this order: a0, a1, a2, ... */
static int32_t Polynomial(CSOUND* csound, POLYNOMIAL* p)
{
    int32_t   i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   ncoeff =    /* index of the last coefficient */
                   GetInputArgCnt((OPDS *)p) - 2;
    MYFLT *out = p->aout;
    MYFLT *in = p->ain;
    MYFLT **coeff = p->kcoefficients;
    MYFLT sum, x;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
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
    MYFLT   *aout, *ain, *kcoefficients[VARGMAX-1];
    MYFLT   *chebn;
    AUXCH   coeff;
} CHEBPOLY;

static int32_t ChebyshevPolyInit(CSOUND* csound, CHEBPOLY* p)
{
    int32_t     ncoeff = GetInputArgCnt((OPDS *)p) - 1;

    /* Need two MYFLT arrays of length ncoeff: first for the coefficients
       of the sum of polynomials, and the second for the coefficients of
       the individual chebyshev polynomials as we are adding them up. */
    csound->AuxAlloc(csound, (2*ncoeff + 1)*sizeof(MYFLT), &(p->coeff));
    p->chebn = ((MYFLT*)p->coeff.auxp) + ncoeff;
    return OK;
}

/* Efficiently evaluates a sum of Chebyshev polynomials of arbitrary order.
   Coefficients (k0, k1, k2, ... ) are k-rate multipliers of each Chebyshev
   polynomial starting with the zero-order polynomial T0(x) = 1, then
   T1(x) = x, T2(x) = 2x^2 - 1, etc. */
static int32_t ChebyshevPolynomial(CSOUND* csound, CHEBPOLY* p)
{
    int32_t     i, j;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     ncoeff =            /* index of the last coefficient */
                     GetInputArgCnt((OPDS *)p) - 2;
    MYFLT   *out = p->aout;
    MYFLT   *in = p->ain;
    MYFLT   **chebcoeff = p->kcoefficients;
    MYFLT   *chebn = p->chebn;
    MYFLT   *coeff = (MYFLT*)p->coeff.auxp;
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
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
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

static int32_t PDClip(CSOUND* csound, PD_CLIP* p)
{
    IGN(csound);
    MYFLT     cur, low, high, maxampl, width, unwidth, center, outscalar;
    int32_t       bipolarMode;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT*    out = p->aout;
    MYFLT*    in = p->ain;

    bipolarMode = (int32_t) *(p->ibipolar);
    maxampl = *(p->ifullscale);
    width = (*(p->kwidth) > FL(1.0) ? FL(1.0) :
             (*(p->kwidth) < FL(0.0) ? FL(0.0) : *(p->kwidth)));
    unwidth = FL(1.0) - width;           /* width of 1/2 unclipped region */

    if (bipolarMode) {
      /* the unclipped region can only shift left or right by up to width */
      if      (*(p->kcenter) < - width) center = - width * maxampl;
      else if (*(p->kcenter) > width)   center = width * maxampl;
      else                                 center = *(p->kcenter) * maxampl;
    }
    else {
      width = FL(0.5) * width;   /* range reduced by 1/2 in unipolar mode */
      unwidth = FL(0.5) * unwidth;
      center = *(p->kcenter) * FL(0.5) + FL(0.5);     /* make unipolar */
      if      (center < (FL(0.5) - width)) center = (FL(0.5) - width) * maxampl;
      else if (center > (FL(0.5) + width)) center = (FL(0.5) + width) * maxampl;
      else                                 center = center * maxampl;
    }
    low = center - unwidth*maxampl;       /* min value of unclipped input */
    high = unwidth*maxampl + center;      /* max value of unclipped input */

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }

    if (bipolarMode) {
      outscalar = (unwidth == FL(0.0)) ? FL(0.0) : (FL(1.0) / unwidth);
      for (n=offset; n<nsmps; n++) {
        cur = in[n];
        out[n] = (cur <= low ? -maxampl :
                  (cur >= high ? maxampl : (outscalar * (cur-center))));
      }
    }
    else {
      outscalar = (unwidth == FL(0.0)) ? FL(0.0) : (FL(0.5) / unwidth);
      for (n=offset; n<nsmps; n++) {
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
static int32_t PDHalfX(CSOUND* csound, PD_HALF* p)
{
    IGN(csound);
    MYFLT     cur, maxampl, midpoint, leftslope, rightslope;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT*    out = p->aout;
    MYFLT*    in = p->ain;

    maxampl = *(p->ifullscale);
    if (maxampl == FL(0.0))  maxampl = FL(1.0);

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (*(p->ibipolar) != FL(0.0)) {    /* bipolar mode */
      /* clamp kamount in range [-1,1] */
      midpoint = (*(p->kamount) >= FL(1.0) ? maxampl :
                  (*(p->kamount) <= FL(-1.0) ? -maxampl :
                   (*(p->kamount) * maxampl)));

      if (midpoint != -maxampl) leftslope  = maxampl / (midpoint + maxampl);
      else                      leftslope  = FL(0.0);
      if (midpoint != maxampl)  rightslope = maxampl / (maxampl - midpoint);
      else                      rightslope = FL(0.0);

      for (n=offset; n<nsmps; n++) {
        cur = in[n];
        if (cur < midpoint) out[n] = leftslope * (cur - midpoint);
        else                out[n] = rightslope * (cur - midpoint);
      }
    }
    else {  /* unipolar mode */
      MYFLT  halfmaxampl = FL(0.5) * maxampl;

      /* clamp kamount in range [-1,1] and make unipolar */
      midpoint = (*(p->kamount) >= FL(1.0) ? maxampl :
                  (*(p->kamount) <= FL(-1.0) ? FL(0.0) :
                   ((*(p->kamount) + FL(1.0)) * halfmaxampl)));

      if (midpoint != FL(0.0)) leftslope  = halfmaxampl / midpoint;
      else                     leftslope  = FL(0.0);
      if (midpoint != maxampl) rightslope = halfmaxampl / (maxampl - midpoint);
      else                     rightslope = FL(0.0);

      for (n=offset; n<nsmps; n++) {
        cur = in[n];
        if (cur < midpoint) out[n] = leftslope * cur;
        else                out[n] = rightslope*(cur - midpoint) + halfmaxampl;
      }
    }

    return OK;
}

/* Casio-style phase distortion with "pivot point" on the Y axis */
static int32_t PDHalfY(CSOUND* csound, PD_HALF* p)
{
    IGN(csound);
    MYFLT     cur, maxampl, midpoint, leftslope, rightslope;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT*    out = p->aout;
    MYFLT*    in = p->ain;

    maxampl = *(p->ifullscale);
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (maxampl == FL(0.0))  maxampl = FL(1.0);

    if (*(p->ibipolar) != FL(0.0)) {    /* bipolar mode */
      /* clamp kamount in range [-1,1] */
      midpoint = (*(p->kamount) > FL(1.0) ? maxampl :
                  (*(p->kamount) < FL(-1.0) ? -maxampl :
                   (*(p->kamount) * maxampl)));

      leftslope  = (midpoint + maxampl) / maxampl;
      rightslope = (maxampl - midpoint) / maxampl;

      for (n=offset; n<nsmps; n++) {
        cur = in[n];
        if (cur < FL(0.0)) out[n] = leftslope*cur + midpoint;
        else               out[n] = rightslope*cur + midpoint;
      }
    }
    else {  /* unipolar mode */
      MYFLT  halfmaxampl = FL(0.5) * maxampl;

      /* clamp kamount in range [-1,1] and make unipolar */
      midpoint = (*(p->kamount) >= FL(1.0) ? maxampl :
                  (*(p->kamount) <= FL(-1.0) ? FL(0.0) :
                   ((*(p->kamount) + FL(1.0)) * halfmaxampl)));

      leftslope  = midpoint / halfmaxampl;
      rightslope = (maxampl - midpoint) / halfmaxampl;

      for (n=offset; n<nsmps; n++) {
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

int32_t SyncPhasorInit(CSOUND *csound, SYNCPHASOR *p)
{
    MYFLT  phs;
    int32   longphs;

    if ((phs = *p->initphase) >= FL(0.0)) {
      if (UNLIKELY((longphs = (int32)phs))) {
        csound->Warning(csound, "%s", Str("init phase truncation\n"));
      }
      p->curphase = phs - (MYFLT)longphs;
    }
    return OK;
}

int32_t SyncPhasor(CSOUND *csound, SYNCPHASOR *p)
{
    double      phase;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *out, *syncout, *syncin;
    double      incr;
    int32_t         cpsIsARate;

    out = p->aphase;
    syncout = p->asyncout;
    syncin = p->asyncin;
    phase = p->curphase;
    cpsIsARate = IS_ASIG_ARG(p->xcps); /* check first input arg rate */
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (cpsIsARate) {
      MYFLT *cps = p->xcps;
      for (n=offset; n<nsmps; n++) {
        if (syncin[n] != FL(0.0)) {        /* non-zero triggers reset */
          phase = 0.0;
          out[n] = (MYFLT)phase;
          syncout[n] = FL(1.0);        /* send sync whenever syncin */
        }
        else {
          incr = (double)(cps[n] * CS_ONEDSR);
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
      incr = (double)(*p->xcps * CS_ONEDSR);
      for (n=offset; n<nsmps; n++) {
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

static int32_t PhasineInit(CSOUND* csound, PHASINE* p)
{
    p->lastin = FL(0.0);
    p->maxamplitude = *p->ifullscale;
    return OK;
}

static int32_t Phasine(CSOUND* csound, PHASINE* p)
{
    MYFLT     last, cur, phase, adjust, maxampl;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT*    out = p->aout;
    MYFLT*    in = p->ain;

    adjust = *(p->kphaseadjust);
    last = p->lastin;
    maxampl = p->maxamplitude;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
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

    p->lastin = last;
    return OK;

}
#endif

/* code for linking dynamic libraries under Csound 5 */

#define S(x)    sizeof(x)

static OENTRY shape_localops[] =
  {
  /* { "phasine", S(PHASINE), 0, "a", "akp",
     (SUBR)PhasineInit, (SUBR)Phasine }, */
   { "powershape", S(POWER_SHAPE), 0, "a", "akp",
     (SUBR)PowerShapeInit, (SUBR)PowerShape },
   { "polynomial", S(POLYNOMIAL), 0,  "a", "az", NULL, (SUBR)Polynomial },
   { "chebyshevpoly", S(CHEBPOLY), 0, "a", "az",
     (SUBR)ChebyshevPolyInit, (SUBR)ChebyshevPolynomial },
   { "pdclip", S(PD_CLIP), 0,  "a", "akkop", NULL, (SUBR)PDClip },
   { "pdhalf", S(PD_HALF), 0,  "a", "akop", NULL, (SUBR)PDHalfX },
   { "pdhalfy", S(PD_HALF), 0,  "a", "akop", NULL, (SUBR)PDHalfY },
   { "syncphasor", S(SYNCPHASOR), 0, "aa", "xao",
     (SUBR)SyncPhasorInit, (SUBR)SyncPhasor },
};

LINKAGE_BUILTIN(shape_localops)
