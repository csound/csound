/*
  biquad.c:

  Copyright (C) 1998, 1999, 2001 by Hans Mikelson, Matt Gerassimoff,
  Jens Groh, John ffitch, Steven Yi

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

/***************************************************************/
/* biquad, moogvcf, rezzy                                      */
/* Biquadratic digital filter with K-Rate coeff.               */
/*      Moog VCF Simulation by Comajuncosas/Stilson/Smith      */
/*      Rezzy Filter by Hans Mikelson                          */
/* Nested allpass and Lorenz added                             */
/* October 1998 by Hans Mikelson                               */
/***************************************************************/
//#include "csdl.h"
#include <math.h>
#include "biquad.h"
#include "csound_standard_types.h"

/***************************************************************************/
/* The biquadratic filter computes the digital filter two x components and */
/* two y values.                                                           */
/* Coded by Hans Mikelson October 1998                                     */
/***************************************************************************/

static int32_t biquadset(CSOUND *csound, BIQUAD *p)
{
  IGN(csound);
  /* The biquadratic filter is initialised to zero.    */
  if (*p->reinit==FL(0.0)) {      /* Only reset in in non-legato mode */
    p->xnm1 = p->xnm2 = p->ynm1 = p->ynm2 = 0.0;
  }
  return OK;
} /* end biquadset(p) */

static int32_t biquad(CSOUND *csound, BIQUAD *p)
{
  IGN(csound);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_double xn, yn;
  cs_double xnm1 = p->xnm1, xnm2 = p->xnm2, ynm1 = p->ynm1, ynm2 = p->ynm2;
  cs_double a0 = 1.0 / *p->a0, a1 = a0 * *p->a1, a2 = a0 * *p->a2;
  cs_double b0 = a0 * *p->b0, b1 = a0 * *p->b1, b2 = a0 * *p->b2;

  if (UNLIKELY(offset)) memset(p->out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&p->out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    xn = (cs_double)p->in[n];
    yn = b0*xn + b1*xnm1 + b2*xnm2 - a1*ynm1 - a2*ynm2;
    xnm2 = xnm1;
    xnm1 = xn;
    ynm2 = ynm1;
    ynm1 = yn;
    p->out[n] = (cs_float)yn;
  }
  p->xnm1 = xnm1; p->xnm2 = xnm2; p->ynm1 = ynm1; p->ynm2 = ynm2;
  return OK;
}

/* A-rate version of above -- JPff August 2001 */

static int32_t biquada(CSOUND *csound, BIQUAD *p)
{
  IGN(csound);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float *out, *in;
  cs_double xn, yn;
  cs_float *a0 = p->a0, *a1 = p->a1, *a2 = p->a2;
  cs_float *b0 = p->b0, *b1 = p->b1, *b2 = p->b2;
  cs_double xnm1 = p->xnm1, xnm2 = p->xnm2, ynm1 = p->ynm1, ynm2 = p->ynm2;
  in   = p->in;
  out  = p->out;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    xn = (cs_double)in[n];
    yn = ( (cs_double)b0[n] * xn + (cs_double)b1[n] * xnm1 + (cs_double)b2[n] * xnm2 -
           a1[n] * ynm1 - (cs_double)a2[n] * ynm2)/ (cs_double)a0[n];
    xnm2 = xnm1;
    xnm1 = xn;
    ynm2 = ynm1;
    ynm1 = yn;
    out[n] = (cs_float)yn;
  }
  p->xnm1 = xnm1; p->xnm2 = xnm2; p->ynm1 = ynm1; p->ynm2 = ynm2;
  return OK;
}

/***************************************************************************/
/* begin MoogVCF by Stilson & Smith of CCRMA,  *****************************/
/* translated to Csound by Josep Comajuncosas, *****************************/
/* translated to C by Hans Mikelson            *****************************/
/***************************************************************************/

/* moogvcf historically applies 0dBFS scaling in addition to iscale.
   Keep this known defect for compatibility with existing scores; moogvcf2
   uses iscale alone. Do not change the legacy scaling to match moogvcf2. */
static int32_t moogvcfset(CSOUND *csound, MOOGVCF *p)
{
  if (*p->iskip==FL(0.0)) {
    p->xnm1 = p->y1nm1 = p->y2nm1 = p->y3nm1 = 0.0;
    p->y1n  = p->y2n   = p->y3n   = p->y4n   = 0.0;
  }
  p->fcocod = IS_ASIG_ARG(p->fco) ? 1 : 0;
  p->rezcod = IS_ASIG_ARG(p->res) ? 1 : 0;
  p->fullscale = csound->Get0dBFS(csound);
  if ((p->maxint = *p->max)==FL(0.0)) p->maxint = p->fullscale;

  return OK;
}

static int32_t moogvcf2set(CSOUND *csound, MOOGVCF *p)
{
  moogvcfset(csound, p);
  p->fullscale = FL(1.0);
  return OK;
}

static int32_t moogvcf(CSOUND *csound, MOOGVCF *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float *out, *in;
  cs_double xn;
  cs_float *fcoptr, *resptr;
  /* Fake initialisations to stop compiler warnings!! */
  cs_double fco, res, kp=0.0, pp1d2=0.0, scale=0.0, k=0.0;
  cs_double max = (cs_double)p->maxint;
  cs_double dmax = 1.0/max;
  cs_double xnm1 = p->xnm1, y1nm1 = p->y1nm1, y2nm1 = p->y2nm1, y3nm1 = p->y3nm1;
  cs_double y1n  = p->y1n, y2n = p->y2n, y3n = p->y3n, y4n = p->y4n;
  cs_float zerodb = p->fullscale;

  in      = p->in;
  out     = p->out;
  fcoptr  = p->fco;
  resptr  = p->res;
  fco     = (cs_double)*fcoptr;
  res     = (cs_double)*resptr;

  /* Only need to calculate once */
  if (UNLIKELY((p->rezcod==0) && (p->fcocod==0))) {
    cs_double fcon;
    fcon  = 2.0*fco*(cs_double)CS_ONEDSR; /* normalised freq. 0 to Nyquist */
    kp    = 3.6*fcon-1.6*fcon*fcon-1.0;     /* Emperical tuning   */
    pp1d2 = (kp+1.0)*0.5;                   /* Timesaver          */
    scale = exp((1.0-pp1d2)*1.386249);      /* Scaling factor     */
    k     = res*scale;
  }
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    /* Handle a-rate modulation of fco & res. */
    if (p->fcocod) {
      fco = (cs_double)fcoptr[n];
    }
    if (p->rezcod) {
      res = (cs_double)resptr[n];
    }
    if ((p->rezcod!=0) || (p->fcocod!=0)) {
      cs_double fcon;
      fcon  = 2.0*fco*(cs_double)CS_ONEDSR; /* normalised frq. 0 to Nyquist */
      kp    = 3.6*fcon-1.6*fcon*fcon-1.0;     /* Emperical tuning */
      pp1d2 = (kp+1.0)*0.5;                   /* Timesaver */
      scale = exp((1.0-pp1d2)*1.386249);      /* Scaling factor */
      k     = res*scale;
    }
    xn = (cs_double)in[n] * dmax/zerodb;
    xn = xn - k * y4n; /* Inverted feed back for corner peaking */

    /* Four cascaded onepole filters (bilinear transform) */
    /*       y1n   = xn  * pp1d2 + xnm1  * pp1d2 - kp * y1n; */
    /*       y2n   = y1n * pp1d2 + y1nm1 * pp1d2 - kp * y2n; */
    /*       y3n   = y2n * pp1d2 + y2nm1 * pp1d2 - kp * y3n; */
    /*       y4n   = y3n * pp1d2 + y3nm1 * pp1d2 - kp * y4n; */
    /* why not */
    y1n   = (xn  + xnm1 ) * pp1d2 - kp * y1n;
    y2n   = (y1n + y1nm1) * pp1d2 - kp * y2n;
    y3n   = (y2n + y2nm1) * pp1d2 - kp * y3n;
    y4n   = (y3n + y3nm1) * pp1d2 - kp * y4n;
    /* ?? Algebraically the same and 4 fewer multiplies per sample.
     */
    /* Clipper band limited sigmoid */
    y4n   = y4n - y4n * y4n * y4n / 6.0;
    xnm1  = xn;       /* Update Xn-1  */
    y1nm1 = y1n;      /* Update Y1n-1 */
    y2nm1 = y2n;      /* Update Y2n-1 */
    y3nm1 = y3n;      /* Update Y3n-1 */
    out[n]   = (cs_float)(y4n * max * zerodb);
  }
  p->xnm1 = xnm1; p->y1nm1 = y1nm1; p->y2nm1 = y2nm1; p->y3nm1 = y3nm1;
  p->y1n  = y1n;  p->y2n  = y2n; p->y3n = y3n; p->y4n = y4n;
  return OK;
}

/***************************************************************/
/* This filter is the Mikelson low pass resonant 2-pole filter */
/* Coded by Hans Mikelson October 1998                         */
/***************************************************************/

static int32_t rezzyset(CSOUND *csound, REZZY *p)
{
  IGN(csound);
  if (*p->iskip==FL(0.0)) {
    p->xnm1 = p->xnm2 = p->ynm1 = p->ynm2 = 0.0; /* Initialize to zero */
  }
  p->fcocod = IS_ASIG_ARG(p->fco) ? 1 : 0;
  p->rezcod = IS_ASIG_ARG(p->rez) ? 1 : 0;
  p->warn = 1;
  return OK;
} /* end rezzyset(p) */

#define NEARONE (0.99999)

static int32_t rezzy(CSOUND *csound, REZZY *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float *out, *fcoptr, *rezptr, *in;
  cs_double fco, rez, xn, yn;
  cs_double fqcadj, a=0.0, /* Initialisations fake */
    csq=0.0, invb=0.0, tval=0.0; /* Temporary variables for the filter */
  cs_double xnm1 = p->xnm1, xnm2 = p->xnm2, ynm1 = p->ynm1, ynm2 = p->ynm2;
  cs_double b1 = 0.0, b2 = 0.0;
  int32_t warn = p->warn;

  in     = p->in;
  out    = p->out;
  fcoptr = p->fco;
  rezptr = p->rez;
  fco    = (cs_double)*fcoptr;
  rez    = (cs_double)*rezptr;

  /* Freq. is adjusted based on sample rate */
  fqcadj = 0.149659863*(cs_double)CS_ESR;
  /* Try to keep the resonance under control     */
  if (rez < 1.0) rez = 1.0;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  if (*p->mode == FL(0.0)) {    /* Low Pass */
    if (UNLIKELY((p->rezcod==0) && (p->fcocod==0))) {
      /* Only need to calculate once */
      cs_double c = fqcadj/fco;    /* Filter constant c=1/Fco * adjustment */
      cs_double rez2 = rez/(1.0 + exp(fco/11000.0));
      cs_double b;
      a    = c/rez2 - 1.0;      /* a depends on both Fco and Rez */
      csq  = c*c;               /* Precalculate c^2 */
      b    = 1.0 + a + csq;     /* Normalization constant */
      invb = 1.0/b;
      b1 = (-a-2.0*csq)*invb;
      b2 = csq*invb;
      // Stabalise
      {    // POLES
        //Note that csq cannot be zero
        cs_double  p0, p1, pi, disc;
        disc=b1*b1-(4*b2);
        if (disc<0.0) {
          pi = sqrt(-disc)/2.0;
          p0 = p1 = (-b1)/2.0;
          if (p0*p0+pi*pi>=1.0) {
            cs_double theta = ATAN2(pi, p0);
            if (warn) csound->Warning(csound, "%s", Str("rezzy instability corrected"));
            p0 = NEARONE * COS(theta);
            //pi = NEARONE * sin(theta);
            b1 = -2*p0; b2 = NEARONE*NEARONE; warn = 0;
          }
        }
        else {
          //pi = 0;
          p0=(sqrt(disc)-b1)/2.0;
          p1=(-sqrt(disc)-b1)/2.0;
          if (p0*p0>=1.0 || p1*p1>=1) {
            if (warn) csound->Warning(csound, "%s", Str("rezzy instability corrected"));
            if (p0*p0>=1) p0 = NEARONE*(p0>0.0?1:(-1));
            if (p1*p1>=1) p1 = NEARONE*(p1>0.0?1:(-1));
            b1 = -(p0+p1); b2 = p0*p1; warn = 0;
          }
        }
      }
      //printf("Poles: (%f,%f) and (%f,%f) ", p0, pi, p1, -pi);
    }
    for (n=offset; n<nsmps; n++) { /* do ksmp times   */
      /* Handle a-rate modulation of fco and rez */
      if (p->fcocod) {
        fco = (cs_double)fcoptr[n];
      }
      if (p->rezcod) {
        rez = (cs_double)rezptr[n];
      }
      if ((p->rezcod!=0) || (p->fcocod!=0)) {
        cs_double c = fqcadj/fco;
        cs_double rez2 = rez/(1.0 + exp(fco/11000.0));
        cs_double b;
        a    = c/rez2 - 1.0;  /* a depends on both Fco and Rez */
        csq  = c*c;           /* Precalculate c^2 */
        b    = 1.0 + a + csq; /* Normalization constant */
        invb = 1.0/b;
        b1 = (-a-2.0*csq)*invb;
        b2 = csq*invb;
        // Stabalise
        {    // POLES
          cs_double disc, p0, p1, pi;
          disc=b1*b1-(4*b2);
          if (disc<0.0) {
            pi = sqrt(-disc)/2.0;
            p0=p1=(-b1)/2.0;
            if (p0*p0+pi*pi>=1.0) {
              cs_double theta = ATAN2(pi, p0);
              if (warn) csound->Warning(csound,
                                        "%s", Str("rezzy instability corrected"));
              //printf("b1, b2 = %f, %f ->", b1,b2);
              p0 = NEARONE * COS(theta);
              //pi = NEARONE * sin(theta);
              b1 = -2*p0; b2 = NEARONE*NEARONE; warn = 0;
              //printf(" b1, b2 = %f, %f\n", b1, b2);
            }
          }
          else {
            //pi = 0;
            p0=(sqrt(disc)-b1)/2.0;
            p1=(-sqrt(disc)-b1)/2.0;
            if (p0*p0>=1.0 || p1*p1>=1) {
              if (warn) csound->Warning(csound,
                                        "%s", Str("rezzy instability corrected"));
              //printf("b1, b2= %f, %f ", b1, b2);
              if (p0*p0>=1) p0 = NEARONE*(p0>0.0?1:(-1));
              if (p1*p1>=1) p1 = NEARONE*(p1>0.0?1:(-1));
              b1 = -(p0+p1); b2 = p0*p1; warn = 0;
              //printf("b1, b2= %f, %f ", b1, b2);
            }
          }
        }
        //printf("Poles: (%f,%f) and (%f,%f) ", p0, pi, p1, -pi);
      }
      xn = (cs_double)in[n];             /* Get the next sample */
      /* Mikelson Biquad Filter Guts*/
      //yn = (1.0/sqrt(1.0+rez)*xn - (-a-2.0*csq)*ynm1 - csq*ynm2)*invb;
      yn = invb/sqrt(1.0+rez)*xn - b1*ynm1 - b2*ynm2;

      xnm2 = xnm1; /* Update Xn-2 */
      xnm1 = xn;   /* Update Xn-1 */
      ynm2 = ynm1; /* Update Yn-2 */
      ynm1 = yn;   /* Update Yn-1 */
      out[n] = (cs_float)yn; /* Generate the output sample */

    }
  }
  else { /* High Pass Rezzy */
    cs_double c=0.0, rez2=0.0, cdrez2 = 0.0;
    if (UNLIKELY(p->fcocod==0 && p->rezcod==0)) {
      /* Only need to calculate once */
      cs_double b;
      c = fqcadj/fco;    /* Filter constant c=1/Fco * adjustment */
      rez2 = rez/(1.0 + sqrt(sqrt(1.0/c)));
      tval = 0.75/sqrt(1.0 + rez);
      csq  = c*c;
      cdrez2 = c/rez2;
      b    = (cdrez2 + csq);
      invb = 1.0/b;
      b1 = (1.0-cdrez2-2.0*csq)*invb;
      b2 = csq*invb;
      /* printf("c=%f rez2=%f tval=%f csq=%f b=%f ibvb=%f\n", */
      /*        c,rez2,tval,csq,b,invb); */

      {    // POLES
        cs_double p0, p1, pi;
        cs_double disc=b1*b1-(4*b2);
        if (disc<0.0) {
          pi = sqrt(-disc)/2.0;
          p0=p1=(-b1)/2.0;
          if (p0*p0+pi*pi>=1.0) {
            cs_double theta = ATAN2(pi, p0);
            //printf("b1, b2= %f, %f ", b1, b2);
            if (warn) csound->Warning(csound, "%s", Str("rezzy instability corrected"));
            b1 = -p0*cos(theta); b2 = NEARONE*NEARONE; warn = 0;
            //printf("-> b1, b2= %f, %f\n", b1, b2);
          }
        }
        else {
          //pi = 0;
          p0=(sqrt(disc)-b1)/2.0;
          p1=(-sqrt(disc)-b1)/2.0;
          if (p0*p0>=1.0 || p1*p1>=1) {
            //printf("b1, b2= %f, %f ", b1, b2);
            if (warn) csound->Warning(csound, "%s", Str("rezzy instability corrected"));
            if (p0*p0>=1.0) p0 = NEARONE*(p0>0?1:(-1));
            if (p1*p1>=1.0) p1 = NEARONE*(p1>0?1:(-1));
            b1 = -(p0+p1); b2 = p0*p1; warn = 0;
            //printf("-> b1, b2= %f, %f\n", b1, b2);
          }
        }
      }
      //printf("Poles: (%f,%f) and (%f,%f) ", p0, pi, p1, -pi);

    }
    for (n=offset; n<nsmps; n++) { /* do ksmp times   */
      /* Handle a-rate modulation of fco and rez */
      if (p->fcocod) {
        fco = (cs_double)fcoptr[n];
      }
      if (p->rezcod) {
        rez = (cs_double)rezptr[n];
      }
      if (p->fcocod!=0 || p->rezcod!=0) {
        cs_double b;
        c = fqcadj/fco;
        rez2 = rez/(1.0 + sqrt(sqrt(1.0/c)));
        tval   = 0.75/sqrt(1.0 + rez);
        csq    = c*c;
        cdrez2 = c/rez2;
        b      = (cdrez2 + csq);
        invb   = 1.0/b;
        b1 = (1.0-cdrez2-2.0*csq)*invb;
        b2 = csq*invb;

        {    // POLES
          cs_double  p0, p1, pi;
          cs_double disc=b1*b1-(4*b2);
          if (disc<0.0) {
            pi = sqrt(-disc)/2.0;
            p0=p1=(-b1)/2.0;
            if (p0*p0+pi*pi >=1.0) {
              cs_double theta = ATAN2(pi,p0);
              //printf("b1, b2= %f, %f ", b1, b2);
              if (warn) csound->Warning(csound,
                                        "%s", Str("rezzy instability corrected"));
              b1 = -p0*cos(theta); b2 = NEARONE*NEARONE; warn = 0;
              //printf("-> b1, b2= %f, %f\n", b1, b2);
            }
          }
          else {
            //pi = 0;
            p0=(sqrt(disc)-b1)/2.0;
            p1=(-sqrt(disc)-b1)/2.0;
            if (p0*p0>=1.0 || p1*p1>=1) {
              if (warn) csound->Warning(csound,
                                        "%s", Str("rezzy instability corrected"));
              if (p0*p0>=1.0) p0 = NEARONE*(p0>0?1:(-1));
              if (p1*p1>=1.0) p1 = NEARONE*(p1>0?1:(-1));
              b1 = -(p0+p1); b2 = p0*p1; warn = 0;
            }
          }
          //printf("Poles: (%f,%f) and (%f,%f)\n", p0, pi, p1, -pi);
        }

      }
      xn = (cs_double)in[n];            /* Get the next sample */
      /* Mikelson Biquad Filter Guts*/
      yn = -b1*ynm1 - b2*ynm2
        + (( cdrez2 + csq)*tval*xn + (-cdrez2 - 2.0*csq)*tval*xnm1
           + csq*tval*xnm2)*invb;
      /* printf("ynm1=%f ynm2=%f xn=%f b1=%f, b2=%f => %f\n", */
      /*        ynm1, ynm2, xn, b1,b2, yn); */
      /* printf("y..=%f x..= %f %f %f\n", */
      /*        -b1*ynm1 - b2*ynm2, (cdrez2 + csq)*tval*invb, */
      /*        (-cdrez2 - 2.0*csq)*tval*invb, csq*tval*invb); */
      xnm2 = xnm1;            /* Update Xn-2 */
      xnm1 = xn;              /* Update Xn-1 */
      ynm2 = ynm1;            /* Update Yn-2 */
      ynm1 = yn;              /* Update Yn-1 */
      out[n] = (cs_float)yn;     /* Generate the output sample */
    }
  }
  p->xnm1 = xnm1; p->xnm2 = xnm2; p->ynm1 = ynm1; p->ynm2 = ynm2;
  p->warn = warn;
  return OK;
}
/***************************************************************************/
/* The distortion opcode uses modified hyperbolic tangent distortion.      */
/* Coded by Hans Mikelson November 1998                                    */
/***************************************************************************/

static int32_t distort(CSOUND *csound, DISTORT *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float *out, *in;
  cs_float pregain = *p->pregain, postgain  = *p->postgain;
  cs_float shape1 = *p->shape1, shape2 = *p->shape2;
  cs_float sig;

  in  = p->in;
  out = p->out;
  if (*p->imode < FL(0.5)) {                  /* IV - Dec 28 2002 */
    pregain   *=  FL(0.0002);
    postgain  *=  FL(20000.0);        /* mode 0: original Mikelson version */
    shape1    *=  FL(0.000125);
    shape2    *=  FL(0.000125);
  }
  else if (*p->imode < FL(1.5)) {     /* mode 1: same with 0dBFS support */
    pregain   *=  (FL(6.5536) * (FL(1.)/csound->Get0dBFS(csound)));
    postgain  *=  (FL(0.61035156) * csound->Get0dBFS(csound));
    shape1    *=  (FL(4.096) * (FL(1.)/csound->Get0dBFS(csound)));
    shape2    *=  (FL(4.096) * (FL(1.)/csound->Get0dBFS(csound)));
  }
  else {                              /* mode 2: "raw" mode (+/- 1 amp.) */
    shape1 *= pregain;
    shape2 *= -pregain;
  }
  /* IV - Dec 28 2002 */
  shape1 += pregain;
  shape2 -= pregain;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    cs_float norm;
    sig    = in[n];
    norm = FABS(sig * pregain);
    /* Scale by the largest denominator exponent to avoid overflow. */
    out[n] = ((EXP(sig * shape1 - norm) - EXP(sig * shape2 - norm))
              / (FL(1.0) + EXP(FL(-2.0) * norm))) * postgain;
  }
  return OK;
}

/***************************************************************************/
/* The vco is an analog modeling opcode                                    */
/* Generates bandlimited saw, square/PWM, triangle/Saw-Ramp-Mod            */
/* Coded by Hans Mikelson November 1998                                    */
/***************************************************************************/



static int32_t vcoset(CSOUND *csound, VCO *p)
{
  /* Number of bytes in the delay */
  uint32 ndel = (uint32)(*p->maxd * CS_ESR);
  FUNC  *ftp;    /* Pointer to a sine function */
  //cs_float ndsave;

  //ndsave = (cs_float) ndel;
  if (UNLIKELY((ftp = csound->FTFind(csound, p->sine)) == NULL)) {
    return NOTOK;
  } else {
    p->ftp = ftp;
  }
  p->floatph = !(IS_POW_TWO(p->ftp->flen));

  if (LIKELY(*p->iphs >= FL(0.0))) {
    p->lphs = (int32)(*p->iphs * FL(0.5) * FMAXLEN);
    p->fphs = *p->iphs;
    /* Does it need this? */
  }
  else {
    p->fphs = FL(0.0);
    p->lphs = 0;  
  }

  p->ampcod = IS_ASIG_ARG(p->xamp) ? 1 : 0;
  p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;

  if (*p->iskip==FL(0.0)) {
    p->ynm1 = (*p->wave == FL(1.0)) ? -FL(0.5) : FL(0.0);
    p->ynm2 = FL(0.0);
  }

  /* finished setting up buzz now set up internal vdelay */

  if (UNLIKELY(ndel == 0)) ndel = 1;    /* fix due to Troxler */
  if (p->aux.auxp == NULL ||
      (uint32_t)(ndel*sizeof(cs_float)) > p->aux.size)
    /* allocate space for delay buffer */
    csound->AuxAlloc(csound, ndel * sizeof(cs_float), &p->aux);
  else if (*p->iskip==FL(0.0)) {
    memset(p->aux.auxp, 0, ndel*sizeof(cs_float));
  }
  p->left = 0;
  if (*p->leak <= FL(0.0) || *p->leak >= FL(1.0)) {
    p->leaky = (*p->wave == FL(3.0)) ? FL(0.995) : FL(0.999);
  }
  else {
    p->leaky = *p->leak;
  }
  p->nyq = *p->inyq;

  return OK;
} /* end vcoset(p) */

/* This code modified from Csound's buzz, integ, & vdelay opcodes */

static int32_t vco(CSOUND *csound, VCO *p)
{
  FUNC  *ftp;
  cs_float *ar, *ampp, *cpsp, *ftbl;
  int32  phs, inc, lobits, dwnphs, tnp1, lenmask, maxd, indx;
  cs_float leaky, /*rtfqc,*/ amp, fqc;
  cs_float sicvt2, over2n, scal, num, denom, pulse = FL(0.0), saw = FL(0.0);
  cs_float sqr = FL(0.0), tri = FL(0.0);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  int32_t   knh;
  cs_float  fphs = p->fphs, finc;

  /* VDelay Inserted here */
  cs_float *buf = (cs_float *)p->aux.auxp;
  cs_float fv1, out1;
  int32  v1, v2, flen, floatph = p->floatph;
  /* Save recalculation and also round */
  int32_t wave = (int32_t)CS_FLOAT2LONG(*p->wave);

  leaky = p->leaky;

  ftp = p->ftp;
  flen = ftp->flen;
  if (UNLIKELY(buf==NULL || ftp==NULL)) goto err1;            /* RWD fix */
  maxd = (uint32) (*p->maxd * CS_ESR);
  if (UNLIKELY(maxd == 0)) maxd = 1;    /* Degenerate case */
  indx = p->left;
  /* End of VDelay insert */

  ftbl = ftp->ftable;
  if(!floatph) {
    sicvt2 = CS_SICVT * FL(0.5);  /* for theta/2 */
    lobits = ftp->lobits;
    lenmask = ftp->lenmask;
  }
  ampp = p->xamp;
  cpsp = p->xcps;
  fqc = *cpsp;
  //rtfqc = SQRT(fqc);
  knh = (int32_t)(CS_ESR*p->nyq/fqc);
  if (UNLIKELY((n = (int32_t)knh) <= 0)) {
    csound->Warning(csound, "knh=%x nyq=%f fqc=%f\n"
                    "vco knh (%d) <= 0; taken as 1\n", knh, p->nyq, fqc, n);
    n = 1;
  }
  tnp1 = n + n + 1;           /* calc 2n + 1 */
  over2n = FL(0.5) / n;

  amp  = *ampp;
  scal = over2n;
  if(floatph) finc = fqc * CS_ONEDSR;
  else  inc = (int32)(fqc * sicvt2);
  ar = p->ar;
  phs = p->lphs;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(cs_float));
  }

  /*-----------------------------------------------------*/
  /* PWM Wave                                            */
  /*-----------------------------------------------------*/
  if (wave==2) {
    cs_float pw = *p->pw;
    for (n=offset; n<nsmps; n++) {
      if(floatph) {
        denom =*(ftbl + (size_t) (fphs*flen));
        if (denom > FL(0.00001) || denom < -FL(0.00001)) {
          num = *(ftbl + (size_t) PHMOD1(fphs)*flen*tnp1);
          pulse = (num / denom - FL(1.0)) * scal;
        } else pulse = FL(1.0);         
        fphs = PHMOD1(fphs + finc);
        if (p->cpscod) {
          fqc = cpsp[n];
          finc  = (fqc*CS_ONEDSR);
        }
      } else {
        dwnphs = phs >> lobits;
        denom = *(ftbl + dwnphs);
        if (denom > FL(0.00001) || denom < -FL(0.00001)) {
          num = *(ftbl + (dwnphs * tnp1 & lenmask));
          pulse = (num / denom - FL(1.0)) * scal;
        }
        else pulse = FL(1.0);
        phs += inc;
        phs &= PHMASK;
        if (p->cpscod) {
          fqc = cpsp[n];
          inc  = (int32)(fqc* sicvt2);
        }
      }
      if (p->ampcod) {
        amp  = ampp[n];
        scal = over2n;        /* Why is this needed?? */
      }


      /* VDelay inserted here */
      buf[indx] = pulse;
      fv1 = (cs_float) indx - CS_ESR * pw / fqc;

      v1 = (int32) fv1;
      if (fv1 < FL(0.0)) v1--;
      fv1 -= (cs_float) v1;
      /* Make sure Inside the buffer */
      while (v1 >= maxd)
        v1 -= maxd;
      while (v1 < 0L)
        v1 += maxd;
      /* Find next sample for interpolation      */
      v2 = (v1 < (maxd - 1L) ? v1 + 1L : 0L);
      out1 = buf[v1] + fv1 * (buf[v2] - buf[v1]);

      if (++indx == maxd) indx = 0;             /* Advance current pointer */
      /* End of VDelay */

      sqr  = pulse - out1 + leaky*p->ynm1;
      p->ynm1 = sqr;
      ar[n]  = (sqr + pw - FL(0.5)) * FL(1.9) * amp;
    }
  }

  /*-----------------------------------------------------*/
  /* Triangle Wave                                       */
  /*-----------------------------------------------------*/
  else if (wave==3) {
    cs_float pw = *p->pw;
    for (n=offset; n<nsmps; n++) {
      if(floatph) {
        denom = *(ftbl + (size_t) (fphs*flen));
        if (denom > FL(0.0002) || denom < -FL(0.0002)) {
          num = *(ftbl + (size_t) PHMOD1(fphs)*flen*tnp1);
          pulse = (num / denom - FL(1.0)) * scal;
        } else pulse = FL(1.0);         
        fphs = PHMOD1(fphs + finc);
        if (p->cpscod) {
          fqc = cpsp[n];
          finc  = (fqc*CS_ONEDSR);
        }
      } else {  
          dwnphs = phs >> lobits;
          denom = *(ftbl + dwnphs);
          if (denom > FL(0.0002) || denom < -FL(0.0002)) {
            num = *(ftbl + (dwnphs * tnp1 & lenmask));
            pulse = (num / denom - FL(1.0)) * scal;
          }
          /* else pulse = *ampp; */
          else pulse = FL(1.0);
          phs += inc;
          phs &= PHMASK;
          if (p->cpscod) {
            fqc = cpsp[n];
            inc  = (int32)(fqc* sicvt2);
        
          }
        }
        if (p->ampcod) {
          amp = ampp[n];
        }
  

        /* VDelay inserted here */
        buf[indx] = pulse;
        fv1 = (cs_float) indx - CS_ESR * pw / fqc;
        v1 = (int32) fv1;
        if (fv1 < FL(0.0)) v1--;
        fv1 -= (cs_float) v1;
        /* Make sure Inside the buffer */
        while (v1 >= maxd)
          v1 -= maxd;
        while (v1 < 0L)
          v1 += maxd;
        /* Find next sample for interpolation      */
        v2 = (v1 < (maxd - 1L) ? v1 + 1L : 0L);
        out1 = buf[v1] + fv1 * (buf[v2] - buf[v1]);

        if (UNLIKELY(++indx == maxd)) indx = 0;  /* Advance current pointer */
        /* End of VDelay */

        /* Integrate twice and ouput */
        sqr  = pulse - out1 + leaky*p->ynm1;
        tri  = sqr + leaky*p->ynm2;
        p->ynm1 = sqr;
        p->ynm2 = tri;
        ar[n] =  tri * amp * fqc
          / (CS_ESR * FL(0.42) * (FL(0.05) + pw - pw * pw));
      }
    }
    /*-----------------------------------------------------*/
    /* Sawtooth Wave                                       */
    /*-----------------------------------------------------*/
    else {
      for (n=offset; n<nsmps; n++) {
        if(floatph) {
          denom = *(ftbl + (size_t) (fphs*flen));
          if (denom > FL(0.0002) || denom < -FL(0.0002)) {
            num = *(ftbl + (size_t) PHMOD1(fphs)*flen*tnp1);
            pulse = (num / denom - FL(1.0)) * scal;
          } else pulse = FL(1.0);         
          fphs = PHMOD1(fphs + finc);
          if (p->cpscod) {
            fqc = cpsp[n];
            finc  = (fqc*CS_ONEDSR);
          }
        } else {              
            dwnphs = phs >> lobits;
            denom = *(ftbl + dwnphs);
            if (denom > FL(0.0002) || denom < -FL(0.0002)) {
              num = *(ftbl + (dwnphs * tnp1 & lenmask));
              pulse = (num / denom - FL(1.0)) * scal;
            }
            /* else pulse = *ampp; */
            else pulse = FL(1.0);
            phs += inc;
            phs &= PHMASK;
            if (p->cpscod) {
              fqc = cpsp[n];
              inc  = (int32)(fqc*sicvt2);
            }
          }
          if (p->ampcod) {
            /* scal = *(++ampp) * over2n; */
            /*          scal = over2n;        *//* Why is this needed?? */
            amp = ampp[n];
          }  

          /* Leaky Integration */
          saw  = pulse + leaky*p->ynm1;
          p->ynm1 = saw;
          ar[n] = saw*FL(1.5)*amp;
        }
      }

      p->left = indx;             /*      and keep track of where you are */
      p->lphs = phs;
      p->fphs = fphs;
      return OK;
    err1:
      return csound->PerfError(csound, &(p->h), "%s", Str("vco: not initialised"));
    }

    /***************************************************************************/
    /* This is a simplified model of a planet orbiting in a binary star system */
    /* Coded by Hans Mikelson December 1998                                    */
    /***************************************************************************/

    static int32_t planetset(CSOUND *csound, PLANET *p)
    {
      IGN(csound);
      if (*p->iskip==FL(0.0)) {
        p->x  = *p->xval;  p->y  = *p->yval;  p->z  = *p->zval;
        p->vx = *p->vxval; p->vy = *p->vyval; p->vz = *p->vzval;
        p->ax = FL(0.0); p->ay = FL(0.0); p->az = FL(0.0);
        p->hstep = *p->delta;
        p->friction = FL(1.0) - *p->fric/FL(10000.0);
      }
      return OK;
    } /* end planetset(p) */

      /* Planet orbiting in a binary star system coded by Hans Mikelson */

    static int32_t planet(CSOUND *csound, PLANET *p)
    {
      IGN(csound);
      cs_float *outx, *outy, *outz;
      cs_float   sqradius1, sqradius2, radius1, radius2, fric;
      cs_float xxpyy, dz1, dz2, mass1, mass2, msqror1, msqror2;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;

      fric = p->friction;

      outx = p->outx;
      outy = p->outy;
      outz = p->outz;

      p->s1z = *p->sep*FL(0.5);
      p->s2z = -p->s1z;

      mass1 = *p->mass1;
      mass2 = *p->mass2;

      if (UNLIKELY(offset)) {
        memset(outx, '\0', offset*sizeof(cs_float));
        memset(outy, '\0', offset*sizeof(cs_float));
        memset(outz, '\0', offset*sizeof(cs_float));
      }
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&outx[nsmps], '\0', early*sizeof(cs_float));
        memset(&outy[nsmps], '\0', early*sizeof(cs_float));
        memset(&outz[nsmps], '\0', early*sizeof(cs_float));
      }

      for (n=offset; n<nsmps; n++) {
        xxpyy = p->x * p->x + p->y * p->y;
        dz1 = p->s1z - p->z;

        /* Calculate Acceleration */
        sqradius1 = xxpyy + dz1 * dz1 + FL(1.0);
        radius1 = SQRT(sqradius1);
        msqror1 = mass1/sqradius1/radius1;

        p->ax = msqror1 * -p->x;
        p->ay = msqror1 * -p->y;
        p->az = msqror1 * dz1;

        dz2 = p->s2z - p->z;

        /* Calculate Acceleration */
        sqradius2 = xxpyy + dz2 * dz2 + FL(1.0);
        radius2 = SQRT(sqradius2);
        msqror2 = mass2/sqradius2/radius2;

        p->ax += msqror2 * -p->x;
        p->ay += msqror2 * -p->y;
        p->az += msqror2 * dz2;

        /* Update Velocity */
        p->vx = fric * p->vx + p->hstep * p->ax;
        p->vy = fric * p->vy + p->hstep * p->ay;
        p->vz = fric * p->vz + p->hstep * p->az;

        /* Update Position */
        p->x += p->hstep * p->vx;
        p->y += p->hstep * p->vy;
        p->z += p->hstep * p->vz;

        /* Output the results */
        outx[n] = p->x;
        outy[n] = p->y;
        outz[n] = p->z;
      }
      return OK;
    }

    /* ************************************************** */
    /* ******** Parametric EQ *************************** */
    /* ************************************************** */

    /* Implementation of Zoelzer's Parametric Equalizer Filters */
    static int32_t pareqset(CSOUND *csound, PAREQ *p)
    {
      IGN(csound);
      /* Initialize once before honoring iskip. */
      if (*p->iskip == FL(0.0) || !p->initialized) {
        p->xnm1 = p->xnm2 = p->ynm1 = p->ynm2 = 0.0;
        p->prv_fc = p->prv_v = p->prv_q = FL(-1.0);
        p->imode = (int32_t) CS_FLOAT2LONG(*p->mode);
        p->initialized = 1;
      }
      return OK;
    } /* end pareqset(p) */

    static int32_t pareq(CSOUND *csound, PAREQ *p)
    {
      IGN(csound);
      cs_float xn, yn;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;

      if (*p->fc != p->prv_fc || *p->v != p->prv_v || *p->q != p->prv_q) {
        cs_double omega = (cs_double)(CS_TPIDSR * *p->fc), k, kk, vkk, vk, vkdq, a0;
        p->prv_fc = *p->fc; p->prv_v = *p->v; p->prv_q = *p->q;
        switch (p->imode) {
          /* Low Shelf */
        case 1: {
          cs_double sq = sqrt(2.0 * (cs_double) p->prv_v);
          k = tan(omega * 0.5);
          kk = k * k;
          vkk = (cs_double)p->prv_v * kk;
          p->b0 =  1.0 + sq * k + vkk;
          p->b1 =  2.0 * (vkk - FL(1.0));
          p->b2 =  1.0 - sq * k + vkk;
          a0    =  1.0 + k / (cs_double)p->prv_q + kk;
          p->a1 =  2.0 * (kk - 1.0);
          p->a2 =  1.0 - k / (cs_double)p->prv_q + kk;
        }
          break;
          /* High Shelf */
        case 2: {
          cs_double sq = sqrt(2.0 * (cs_double) p->prv_v);
          k = tan((PI - omega) * 0.5);
          kk = k * k;
          vkk = (cs_double)p->prv_v * kk;
          p->b0 =  1.0 + sq * k + vkk;
          p->b1 = -2.0 * (vkk - 1.0);
          p->b2 =  1.0 - sq * k + vkk;
          a0    =  1.0 + k / (cs_double)p->prv_q + kk;
          p->a1 = -2.0 * (kk - 1.0);
          p->a2 =  1.0 - k / (cs_double)p->prv_q + kk;
        }
          break;
          /* Peaking EQ */
        default: {
          k = tan(omega * 0.5);
          kk = k * k;
          vk = (cs_double)p->prv_v * k;
          vkdq = vk / (cs_double)p->prv_q;
          p->b0 =  1.0 + vkdq + kk;
          p->b1 =  2.0 * (kk - 1.0);
          p->b2 =  1.0 - vkdq + kk;
          a0    =  1.0 + k / (cs_double)p->prv_q + kk;
          p->a1 =  2.0 * (kk - 1.0);
          p->a2 =  1.0 - k / (cs_double)p->prv_q + kk;
        }
        }
        a0 = 1.0 / a0;
        p->a1 *= a0; p->a2 *= a0; p->b0 *= a0; p->b1 *= a0; p->b2 *= a0;
      }
      if (UNLIKELY(offset)) memset(p->out, '\0', offset*sizeof(cs_float));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&p->out[nsmps], '\0', early*sizeof(cs_float));
      }
      {
        cs_double a1 = p->a1, a2 = p->a2;
        cs_double b0 = p->b0, b1 = p->b1, b2 = p->b2;
        cs_double xnm1 = p->xnm1, xnm2 = p->xnm2, ynm1 = p->ynm1, ynm2 = p->ynm2;
        for (n=offset; n<nsmps; n++) {
          xn = (cs_double)p->in[n];
          yn = b0 * xn + b1 * xnm1 + b2 * xnm2 - a1 * ynm1 - a2 * ynm2;
          xnm2 = xnm1;
          xnm1 = xn;
          ynm2 = ynm1;
          ynm1 = yn;
          p->out[n] = (cs_float)yn;
        }
        p->xnm1 = xnm1; p->xnm2 = xnm2; p->ynm1 = ynm1; p->ynm2 = ynm2;
      }
      return OK;
    }

    /* Nested all-pass filters useful for creating reverbs */
    /* Coded by Hans Mikelson January 1999                 */
    /* Derived from Csound's delay opcode                  */
    /* Set up nested all-pass filter                       */

    static int32_t nestedapset(CSOUND *csound, NESTEDAP *p)
    {
      int32_t npts, npts1, npts2 = 0, npts3 = 0;
      cs_double samples;
      size_t size;
      int32_t mode;

      if (*p->istor && p->auxch.auxp != NULL)
        return OK;
      if (UNLIKELY(*p->mode != FL(1.0) && *p->mode != FL(2.0) &&
                   *p->mode != FL(3.0)))
        return csound->InitError(csound, Str("nestedap: mode must be 1, 2 or 3"));
      mode = (int32_t)*p->mode;
      samples = *p->del1 * CS_ESR;
      if (UNLIKELY(!(samples >= 1.0 && samples <= (INT32_MAX + 0.0) &&
                     samples <= SIZE_MAX / sizeof(cs_float))))
        return csound->InitError(csound, Str("nestedap: invalid outer delay"));
      npts = (int32_t)samples;
      if (mode >= 2) {
        samples = *p->del2 * CS_ESR;
        if (UNLIKELY(!(samples >= 1.0 && samples < npts)))
          return csound->InitError(csound, Str("nestedap: invalid second delay"));
        npts2 = (int32_t)samples;
      }
      if (mode == 3) {
        samples = *p->del3 * CS_ESR;
        if (UNLIKELY(!(samples >= 1.0 && samples < npts - npts2)))
          return csound->InitError(csound, Str("nestedap: invalid third delay"));
        npts3 = (int32_t)samples;
      }
      npts1 = npts - npts2 - npts3;
      size = (size_t)npts * sizeof(cs_float);
      if (p->auxch.auxp == NULL || npts != p->npts)
        csound->AuxAlloc(csound, size, &p->auxch);
      else
        memset(p->auxch.auxp, 0, size);
      p->npts = npts;
      p->imode = mode;

      /* Rebuild the layout even when the total allocation size is unchanged. */
      p->beg1p = (cs_float *)p->auxch.auxp;
      p->beg2p = p->beg3p = NULL;
      p->end2p = p->end3p = NULL;
      p->end1p = p->beg1p + npts;
      /* Nested modes historically shorten the outer stages by one sample.
         Keep these endpoints and the filter equations for sound compatibility. */
      if (mode >= 2) {
        p->beg2p = p->beg1p + npts1;
        p->end1p = p->beg2p - 1;
        p->end2p = p->beg1p + npts;
      }
      if (mode == 3) {
        p->beg3p = p->beg2p + npts2;
        p->end2p = p->beg3p - 1;
        p->end3p = p->beg1p + npts;
      }
      p->del1p = p->beg1p;
      p->del2p = p->beg2p;
      p->del3p = p->beg3p;
      p->out1 = FL(0.0);
      p->out2 = FL(0.0);
      p->out3 = FL(0.0);
      return OK;
    }

    static int32_t nestedap(CSOUND *csound, NESTEDAP *p)
    {
      cs_float   *outp, *inp;
      cs_float   *beg1p, *beg2p, *beg3p, *end1p, *end2p, *end3p;
      cs_float   *del1p, *del2p, *del3p;
      cs_float   in1, g1, g2, g3;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;

      if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */

      outp = p->out;
      inp  = p->in;

      if (UNLIKELY(offset)) memset(outp, '\0', offset*sizeof(cs_float));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&outp[nsmps], '\0', early*sizeof(cs_float));
      }
      /* Ordinary All-Pass Filter */
      if (p->imode == 1) {

        del1p = p->del1p;
        end1p = p->end1p;
        beg1p = p->beg1p;
        g1    = *p->gain1;

        for (n=offset; n<nsmps; n++) {
          in1 = inp[n];
          p->out1 = *del1p - g1*in1;

          /* dw1 delay dr1, dt1 */
          *del1p = in1 + g1 * p->out1;
          if (UNLIKELY(++del1p >= end1p))
            del1p = beg1p;

          outp[n] = p->out1;
        }
        p->del1p = del1p;         /* save the new curp */
      }

      /* Single Nested All-Pass Filter */
      else if (p->imode == 2) {

        del1p = p->del1p;
        end1p = p->end1p;
        beg1p = p->beg1p;
        g1    = *p->gain1;

        del2p = p->del2p;
        end2p = p->end2p;
        beg2p = p->beg2p;
        g2    = *p->gain2;

        for (n=offset; n<nsmps; n++) {
          in1 = inp[n];

          p->out2 = *del2p  - g2 * *del1p;
          p->out1 = p->out2 - g1 * in1;

          *del1p = in1    + g1 * p->out1;
          *del2p = *del1p + g2 * p->out2;

          /* delay 2 */
          if (UNLIKELY(++del2p >= end2p))
            del2p = beg2p;

          /* delay 1 */
          if (UNLIKELY(++del1p >= end1p))
            del1p = beg1p;

          outp[n] = p->out1;
        }
        p->del1p = del1p;         /* save the new del1p */
        p->del2p = del2p;         /* save the new del2p */
      }

      /* Double Nested All-Pass Filter */
      else if (p->imode == 3) {

        del1p = p->del1p;
        end1p = p->end1p;
        beg1p = p->beg1p;
        g1    = *p->gain1;

        del2p = p->del2p;
        end2p = p->end2p;
        beg2p = p->beg2p;
        g2    = *p->gain2;

        del3p = p->del3p;
        end3p = p->end3p;
        beg3p = p->beg3p;
        g3    = *p->gain3;

        for (n=offset; n<nsmps; n++) {
          in1 = inp[n];

          p->out2 = *del2p  - g2 * *del1p;
          p->out3 = *del3p  - g3 * p->out2;
          p->out1 = p->out3 - g1 * in1;

          *del1p = in1      + g1 * p->out1;
          *del2p = *del1p   + g2 * p->out2;
          *del3p = p->out2  + g3 * p->out3;

          /* delay 1 */
          if (UNLIKELY(++del1p >= end1p))
            del1p = beg1p;

          /* delay 2 */
          if (UNLIKELY(++del2p >= end2p))
            del2p = beg2p;

          /* delay 3 */
          if (UNLIKELY(++del3p >= end3p))
            del3p = beg3p;

          outp[n] = p->out1;
        }
        p->del1p = del1p;         /* save the new del1p */
        p->del2p = del2p;         /* save the new del2p */
        p->del3p = del3p;         /* save the new del3p */
      }
      return OK;
    err1:
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("delay: not initialised"));
    }

    /***************************************************************************/
    /* The Lorenz System                                                       */
    /* Coded by Hans Mikelson Jauarary 1999                                    */
    /***************************************************************************/

    static int32_t lorenzset(CSOUND *csound, LORENZ *p)
    {
      IGN(csound);
      if (*p->iskip==FL(0.0)) {
        p->valx = *p->inx; p->valy = *p->iny; p->valz = *p->inz;
      }
      return OK;
    }

    /* Lorenz System coded by Hans Mikelson */

    static int32_t lorenz(CSOUND *csound, LORENZ *p)
    {
      IGN(csound);
      cs_float   *outx, *outy, *outz;
      cs_float   x, y, z, xx, yy, s, r, b, hstep;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      int32    skip;

      outx  = p->outx;
      outy  = p->outy;
      outz  = p->outz;

      s     = *p->s;
      r     = *p->r;
      b     = *p->b;
      hstep = *p->hstep;
      skip  = (int32) *p->skip;
      if (skip < 1) skip = 1;
      x     = p->valx;
      y     = p->valy;
      z     = p->valz;

      if (UNLIKELY(offset)) {
        memset(outx, '\0', offset*sizeof(cs_float));
        memset(outy, '\0', offset*sizeof(cs_float));
        memset(outz, '\0', offset*sizeof(cs_float));
      }
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&outx[nsmps], '\0', early*sizeof(cs_float));
        memset(&outy[nsmps], '\0', early*sizeof(cs_float));
        memset(&outz[nsmps], '\0', early*sizeof(cs_float));
      }

      for (n=offset; n<nsmps; n++) {
        int32 remaining = skip;
        do {
          xx   =      x+hstep*s*(y-x);
          yy   =      y+hstep*(-x*z+r*x-y);
          z    =      z+hstep*(x*y-b*z);
          x    =      xx;
          y    =      yy;
        } while (--remaining>0);

        /* Output the results */
        outx[n] = x;
        outy[n] = y;
        outz[n] = z;
      }

      p->valx = x;
      p->valy = y;
      p->valz = z;
      return OK;
    }

    /**************************************************************************/
    /* TBVCF by Hans Mikelson December 2000-January 2001                      */
    /* This opcode attempts to model some of the filter characteristics of    */
    /* a TB303.  Euler's method is used to approximate the system rather      */
    /* than traditional filter methods.  Cut-off frequency, Q and distortion  */
    /* are all coupled.  Empirical methods were used to try to unentwine them */
    /* but frequency is only approximate.                                     */
    /**************************************************************************/

    static int32_t tbvcfset(CSOUND *csound, TBVCF *p)
    {
      IGN(csound);
      if (*p->iskip==FL(0.0)) {
        p->y = p->y1 = p->y2 = 0.0;
      }
      p->fcocod = IS_ASIG_ARG(p->fco) ? 1 : 0;
      p->rezcod = IS_ASIG_ARG(p->res) ? 1 : 0;
      return OK;
    }

    static int32_t tbvcf(CSOUND *csound, TBVCF *p)
    {
      IGN(csound);
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      cs_float *out, *in;
      cs_double x;
      cs_float *fcoptr, *resptr, *distptr, *asymptr;
      cs_double fco, res, dist, asym;
      cs_double y = p->y, y1 = p->y1, y2 = p->y2;
      /* The initialisations are fake to fool compiler warnings */
      cs_double ih, fdbk, d, ad;
      cs_double fc=0.0, fco1=0.0, q=0.0, q1=0.0;

      ih  = 0.001; /* ih is the incremental factor */

      /* Set up the pointers */
      in      = p->in;
      out     = p->out;
      fcoptr  = p->fco;
      resptr  = p->res;
      distptr = p->dist;
      asymptr = p->asym;

      /* Get the values for the k-rate variables */
      fco  = (cs_double)*fcoptr;
      res  = (cs_double)*resptr;
      dist = (cs_double)*distptr;
      asym = (cs_double)*asymptr;

      /* Try to decouple the variables */
      if ((p->rezcod==0) && (p->fcocod==0)) { /* Calc once only */
        q1   = res/(1.0 + sqrt(dist));
        fco1 = pow(fco*260.0/(1.0+q1*0.5),0.58);
        q    = q1*fco1*fco1*0.0005;
        fc   = fco1*(cs_double)CS_ONEDSR*(44100.0/8.0);
      }
      if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&out[nsmps], '\0', early*sizeof(cs_float));
      }
      for (n=offset; n<nsmps; n++) {
        /* Handle a-rate modulation of fco & res. */
        if (p->fcocod) {
          fco = (cs_double)fcoptr[n];
        }
        if (p->rezcod) {
          res = (cs_double)resptr[n];
        }
        if ((p->rezcod!=0) || (p->fcocod!=0)) {
          q1  = res/(1.0 + sqrt(dist));
          fco1 = pow(fco*260.0/(1.0+q1*0.5),0.58);
          q  = q1*fco1*fco1*0.0005;
          fc  = fco1*(cs_double)CS_ONEDSR*(44100.0/8.0);
        }
        x  = (cs_double)in[n];
        fdbk = q*y/(1.0 + exp(-3.0*y)*asym);
        y1  = y1 + ih*((x - y1)*fc - fdbk);
        d  = -0.1*y*20.0;
        ad  = (d*d*d + y2)*100.0*dist;
        y2  = y2 + ih*((y1 - y2)*fc + ad);
        y  = y + ih*((y2 - y)*fc);
        out[n] = (cs_float)(y*fc/1000.0*(1.0 + q1)*3.2);
      }
      p->y = y; p->y1 = y1; p->y2 = y2;
      return OK;
    }

    /* bqrez by Matt Gerassimoff */
    static int32_t bqrezset(CSOUND *csound, REZZY *p)
    {
      IGN(csound);
      if (*p->iskip==FL(0.0)) {
        p->xnm1 = p->xnm2 = p->ynm1 = p->ynm2 = 0.0;  /* Initialise to zero */
      }
      p->fcocod = IS_ASIG_ARG(p->fco) ? 1 : 0;
      p->rezcod = IS_ASIG_ARG(p->rez) ? 1 : 0;

      return OK;
    } /* end rezzyset(p) */

    static int32_t bqrez(CSOUND *csound, REZZY *p)
    {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      cs_float *out, *fcoptr, *rezptr, *in;
      cs_double fco, rez, xn, yn;
      cs_double sin2 = 0.0, cos2 = 0.0, beta=0.0, alpha, gamma=0.0, mu, sigma, chi;
      cs_double theta;
      cs_double xnm1 = p->xnm1, xnm2 = p->xnm2, ynm1 = p->ynm1, ynm2 = p->ynm2;
      int32_t mode = (int32_t)CS_FLOAT2LONG(*p->mode);

      in     = p->in;
      out    = p->out;
      fcoptr = p->fco;
      rezptr = p->rez;
      fco    = (cs_double)*fcoptr;
      rez    = (cs_double)*rezptr;

      if ((p->rezcod == 0) && (p->fcocod == 0)) {
        theta = fco * (cs_double)CS_TPIDSR;
        sin2 = sin(theta) * 0.5;
        cos2 = cos(theta);
        beta = (rez - sin2) / (rez + sin2);
        gamma = (beta + 1.0) * cos2;
      }

      if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&out[nsmps], '\0', early*sizeof(cs_float));
      }

      if (mode < 3) {
        if (mode == 0) {    /* Low Pass */
          chi   = -1.0;
          mu    = 2.0;
          sigma = 1.0;
        }
        else if (mode == 1) { /* High Pass */
          chi   = 1.0;
          mu    = -2.0;
          sigma = 1.0;
        }
        else {                /* Band Pass */
          chi   = 1.0;
          mu    = 0.0;
          sigma = -1.0;
        }
        /* Band-pass gain follows the low/high-pass center gain (2 * rez).
           The high-pass numerator gives an unwanted cot(theta/2) factor. */
        alpha = mode == 2 ? (beta + 1.0) * sin2
                          : (beta + 1.0 + chi*gamma) * 0.5;

        for (n=offset; n<nsmps; n++) {                        /* do ksmp times   */
          /* Handle a-rate modulation of fco and rez */
          if (p->fcocod) {
            fco = (cs_double)fcoptr[n];
          }
          if (p->rezcod) {
            rez = (cs_double)rezptr[n];
          }
          if ((p->rezcod == 1) || (p->fcocod == 1)) {
            theta = fco * (cs_double) CS_TPIDSR;
            sin2 = sin(theta) * 0.5;
            cos2 = cos(theta);
            beta = (rez - sin2) / (rez + sin2);
            gamma = (beta + 1.0) * cos2;
            alpha = mode == 2 ? (beta + 1.0) * sin2
                              : (beta + 1.0 + chi*gamma) * 0.5;
          }
          xn     = (cs_double)in[n];   /* Get the next sample */
          yn     = alpha*(xn + mu*xnm1 + sigma*xnm2) + gamma*ynm1 - beta*ynm2;

          xnm2   = xnm1; /* Update Xn-2 */
          xnm1   = xn;   /* Update Xn-1 */
          ynm2   = ynm1; /* Update Yn-2 */
          ynm1   = yn;   /* Update Yn-1 */
          out[n] = (cs_float)yn;   /* Generate the output sample */

        }
      }
      else if (mode == 3) {   /* Band Stop */
        alpha  = (beta + 1.0) * 0.5;
        for (n=offset; n<nsmps; n++) {                       /* do ksmp times   */
          /* Handle a-rate modulation of fco and rez */
          if (p->fcocod) {
            fco = (cs_double)fcoptr[n];
          }
          if (p->rezcod) {
            rez = (cs_double)rezptr[n];
          }
          if ((p->rezcod == 1) || (p->fcocod == 1)) {
            theta = fco * (cs_double) CS_TPIDSR;
            sin2  = sin(theta) * 0.5;
            cos2  = cos(theta);
            beta  = (rez - sin2) / (rez + sin2);
            gamma = (beta + 1.0) * cos2;
            alpha = (beta + 1.0) * 0.5;
          }
          mu     = -2.0*cos2;
          xn     = (cs_double)in[n];       /* Get the next sample */
          yn     = alpha*(xn + mu*xnm1 + xnm2) + gamma*ynm1 - beta*ynm2;

          xnm2   = xnm1;  /* Update Xn-2 */
          xnm1   = xn;    /* Update Xn-1 */
          ynm2   = ynm1;  /* Update Yn-2 */
          ynm1   = yn;    /* Update Yn-1 */
          out[n] = (cs_float) yn;  /* Generate the output sample */
        }
      }
      else if (mode == 4) {   /* All Pass */
        for (n=offset; n<nsmps; n++) {                        /* do ksmp times   */
          /* Handle a-rate modulation of fco and rez */
          if (p->fcocod) {
            fco = (cs_double)fcoptr[n];
          }
          if (p->rezcod) {
            rez = (cs_double)rezptr[n];
          }
          if ((p->rezcod == 1) || (p->fcocod == 1)) {
            theta = fco * (cs_double) CS_TPIDSR;
            sin2 = sin(theta) * 0.5;
            cos2 = cos(theta);
            beta = (rez - sin2) / (rez + sin2);
            gamma = (beta + 1.0) * cos2;
          }
          chi    = beta;
          mu     = -gamma;
          xn     = (cs_double)in[n];               /* Get the next sample */
          yn     = chi*xn + mu*xnm1 + xnm2 + gamma*ynm1 - beta*ynm2;

          xnm2   = xnm1;  /* Update Xn-2 */
          xnm1   = xn;    /* Update Xn-1 */
          ynm2   = ynm1;  /* Update Yn-2 */
          ynm1   = yn;    /* Update Yn-1 */
          out[n] = (cs_float)yn;  /* Generate the output sample */

        }
      }
      p->xnm1 = xnm1; p->xnm2 = xnm2; p->ynm1 = ynm1; p->ynm2 = ynm2;
      return OK;
    }

    /* mode opcode - original UDO code by François Blanc, rewritten in C by
     * Steven Yi
     */

    static int32_t modeset(CSOUND *csound, MODE *p)
    {
      IGN(csound);
      /* Initialize filter to zero if set to reinitialize.  */
      if (*p->reinit==FL(0.0)) {      /* Only reset in in non-legato mode */
        p->xnm1 = p->ynm1 = p->ynm2 = 0.0;
        p->a0 = p->a1 = p->a2 = p->d = 0.0;
      }
      p->lfq = -FL(1.0); p->lq = -FL(1.0);
      /* set a limit below theoretical sr/pi */
      p->limit = CS_ESR*(FL(1.0)/PI_F-FL(1.0)/FL(100.0));
      //printf("*** limit = %lf\n", p->limit);
      return OK;
    }


    static int32_t mode(CSOUND *csound, MODE *p)
    {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      cs_float lfq = p->lfq, lq = p->lq;
      cs_float kfq = *p->kfreq;
      cs_float kq  = *p->kq;

      cs_double xn, yn, a0=p->a0, a1=p->a1, a2=p->a2,d=p->d;
      cs_double xnm1 = p->xnm1, ynm1 = p->ynm1, ynm2 = p->ynm2;
      int32_t    asgfr = IS_ASIG_ARG(p->kfreq), asgq = IS_ASIG_ARG(p->kq);

      if (UNLIKELY(kfq>p->limit)) {
        //printf("*** freq, limit = %f, %f\n", *p->kfreq, p->limit);
        kfq = p->limit;
      }
      if (UNLIKELY(offset)) memset(p->aout, '\0', offset*sizeof(cs_float));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&p->aout[nsmps], '\0', early*sizeof(cs_float));
      }
      for (n=offset; n<nsmps; n++) {
        if (asgfr) {
          kfq = p->kfreq[n];
          if (UNLIKELY(kfq>p->limit)) {
            //printf("*** freq, limit = %f, %f\n", *p->kfreq, p->limit);
            kfq = p->limit;
          }
        }
        if (asgq) kq = p->kq[n];
        if (lfq != kfq || lq != kq) {
          lq = kq; lfq = kfq;
          if (UNLIKELY(kfq == FL(0.0) || kq == FL(0.0))) {
            /* Zero frequency or Q silences the resonator. Clear feedback
               so a later positive value can start from finite state. */
            a0 = a1 = a2 = d = 0.0;
            ynm1 = ynm2 = 0.0;
          }
          else {
            cs_double kfreq  = kfq*TWOPI;
            cs_double kalpha = (CS_ESR/kfreq);
            cs_double kbeta  = kalpha*kalpha;
            d      = 0.5*kalpha;
            a0     = 1.0/ (kbeta+d/kq);
            a1     = a0 * (1.0-2.0*kbeta);
            a2     = a0 * (kbeta-d/kq);
          }
        }
        xn = (cs_double)p->ain[n];

        yn = a0*xnm1 - a1*ynm1 - a2*ynm2;

        xnm1 = xn;
        ynm2 = ynm1;
        ynm1 = yn;

        yn = yn*d;

        p->aout[n] = (cs_float)yn;
      }
      p->xnm1 = xnm1;  p->ynm1 = ynm1;  p->ynm2 = ynm2;
      p->lfq = lfq;    p->lq = lq;      p->d = d;
      p->a0 = a0;      p->a1 = a1;      p->a2 = a2;
      return OK;
    }

    int32_t mvmfilterset(CSOUND *csound, MVMFILT *p)
    {
      IGN(csound);
      if (*p->reinit==FL(0.0)) {
        p->x = 0;
        p->y = 0;
      }
      return OK;
    }

    int32_t mvmfilter(CSOUND *csound, MVMFILT *p) {
      uint32_t      offset   = p->h.insdshead->ksmps_offset;
      uint32_t      early    = p->h.insdshead->ksmps_no_end;
      cs_float fs       = CS_ESR;
      uint32_t      n, nsmps = CS_KSMPS;
      int32_t       asigtau, asigf0;
      asigtau = IS_ASIG_ARG(p->tau);
      asigf0  = IS_ASIG_ARG(p->f0);

      cs_float *out,*in,*f0,*tau;
      out = p->out;
      in  = p->in;
      f0  = p->f0;
      tau = p->tau;

      cs_float theta,r1,x1,y1,x,y,limit;
      x  = p->x;
      y  = p->y;
      limit = CS_ESR / FL(2.0);

      cs_float f0val=*p->f0;
      f0val = f0val > limit ? limit : f0val;

      if (UNLIKELY(offset)) memset(p->out, '\0', offset*sizeof(cs_float));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&p->out[nsmps], '\0', early*sizeof(cs_float));
      }

      if ((! asigtau) || (! asigf0)) {
        theta  = (TWOPI * f0val) / fs;
        if (*tau > FL(0.0)) {
          r1     = exp(-1 / (*tau*fs));
        } else {
          r1 = 0;
        }
        x1     = cos(theta) * r1;
        y1     = sin(theta) * r1;
      }

      for(n = offset; n < nsmps; n++) {
        if (asigtau) {
          if (tau[n] > FL(0.0)) {
            r1 = exp(-1 / (tau[n]*fs));
          } else {
            r1 = 0;
          }
        }

        if (asigf0) {
          f0val = f0[n];
          f0val = f0val > limit ? limit : f0val;
        }
        if (asigf0 || asigtau) {
          theta  = (TWOPI * f0val) / fs;
          x1     = cos(theta) * r1;
          y1     = sin(theta) * r1;
        }

        cs_float x_  = x;
        x      = (x1 * x)  - (y1 * y) + in[n];
        y      = (y1 * x_) + (x1 * y);
        out[n] = x;
      }

      p->x = x;
      p->y = y;

      return OK;
    }


#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "biquad", S(BIQUAD),   0, "a", "akkkkkko",
                                 (SUBR)biquadset,  (SUBR)biquad },
{ "biquada", S(BIQUAD),  0, "a", "aaaaaaao",
                                 (SUBR)biquadset, (SUBR)biquada },
{ "moogvcf", S(MOOGVCF), 0, "a", "axxpo",
                               (SUBR)moogvcfset,  (SUBR)moogvcf },
{ "moogvcf2", S(MOOGVCF),0, "a", "axxoo",
                               (SUBR)moogvcf2set,  (SUBR)moogvcf },
{ "rezzy", S(REZZY),     0, "a", "axxoo", (SUBR)rezzyset,  (SUBR)rezzy },
{ "bqrez", S(REZZY),     0, "a", "axxoo", (SUBR)bqrezset,  (SUBR)bqrez },
{ "distort1", S(DISTORT),TR,  "a", "akkkko",  NULL,      (SUBR)distort   },
{ "vco", S(VCO),      TR, "a", "xxiVppovoo",(SUBR)vcoset,  (SUBR)vco },
{ "tbvcf", S(TBVCF),     0, "a", "axxkkp",
                                 (SUBR)tbvcfset,  (SUBR)tbvcf   },
{ "planet", S(PLANET),0,"aaa","kkkiiiiiiioo",
                                  (SUBR)planetset,  (SUBR)planet},
{ "pareq", S(PAREQ),     0, "a", "akkkoo",(SUBR)pareqset,  (SUBR)pareq },
{ "nestedap", S(NESTEDAP),0,"a", "aiiiiooooo",
                                     (SUBR)nestedapset,  (SUBR)nestedap},
{ "lorenz", S(LORENZ),0,  "aaa", "kkkkiiiio",
                                  (SUBR)lorenzset,  (SUBR)lorenz},
{ "mode",  S(MODE),   0,      "a", "axxo", (SUBR)modeset,  (SUBR)mode   },
{ "mvmfilter", S(MVMFILT), 0, "a", "axxo",
                                  (SUBR) mvmfilterset, (SUBR) mvmfilter },
};

    int32_t biquad_init_(CSOUND *csound)
    {
      return csound->AppendOpcodes(csound, &(localops[0]),
                                   (int32_t
                                    ) (sizeof(localops) / sizeof(OENTRY)));
    }
