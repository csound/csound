/*
    clfilt.c:

    Copyright (C) 2002 Erik Spjut

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

/***************************************************************/
/* clfilt -The Classical Filters                               */
/* Lowpass, Highpass, Bandpass, Bandstop                       */
/* Butterworth, Chebycheff I, Chebycheff II, Elliptical        */
/* Multi-section biquadratic digital filter with K-Rate coeff. */
/*      Based on biquad by Hans Mikelson                       */
/*      and reson by Barry Vercoe                              */
/* Transformation from continuous time to discrete time by     */
/* bilinear z-transform.                                       */
/* April/May 2002 by Erik Spjut                                */
/***************************************************************/

#include <math.h>
#include "stdopcod.h"
#include "clfilt.h"

static int32_t clfiltset(CSOUND *csound, CLFILT *p)
{
    MYFLT tanfpi, tanfpi2, cotfpi, cotfpi2;
    double eps, bethe, aleph, zee;
    int32_t m, nsec;
    MYFLT pbr = *p->pbr, sbr = *p->sbr;        /* As cannot change */
    p->prvfreq = *p->freq;
    tanfpi = (MYFLT)tan(-CS_MPIDSR*(*p->freq));
    tanfpi2 = tanfpi*tanfpi;
    cotfpi = FL(1.0)/tanfpi;
    cotfpi2 = cotfpi*cotfpi;
    p->ilohi = (int32_t)*p->lohi;
    if (UNLIKELY((p->ilohi < 0) || (p->ilohi > 1))) {
      return csound->InitError(csound,
                               "%s", Str("filter type not lowpass or "
                                   "highpass in clfilt"));
    }
    p->ikind = (int32_t)*p->kind;
    if (UNLIKELY((p->ikind < 0) || (p->ikind > 3))) {
      return csound->InitError(csound,
                               Str("filter kind, %d, out of range in clfilt"),
                               p->ikind);
    }
    if (UNLIKELY((*p->npol < FL(1.0)) || (*p->npol > 2*CL_LIM))) {
      return csound->InitError(csound, Str("number of poles, %f, out of range "
                                           "in clfilt"), *p->npol);
/*       p->nsec = nsec = 1; */
    }
    else if (UNLIKELY(fmod((double)*p->npol,2.0) != 0.0)) {
      p->nsec = nsec = (int32_t)((*p->npol+FL(1.0))*FL(0.5));
      csound->Warning(csound, Str("odd number of poles chosen in clfilt,"
                                  " rounded to %d"), 2*nsec);
    }
    else p->nsec = nsec = (int32_t)((*p->npol)*FL(0.5));
    switch (p->ilohi) {
    case 0: /* Lowpass filters */
      switch (p->ikind) {
      case 0: /* Lowpass Butterworth */
        for (m=0;m<=nsec-1;m++) {
          p->alpha[m] = (MYFLT)cos(PI*((m + 0.5)/(2.0*nsec) + 0.5));
          p->beta[m]  = (MYFLT)sin(PI*((m + 0.5)/(2.0*nsec) + 0.5));
          p->a0[m] = (p->alpha[m])*(p->alpha[m]) +
            (p->beta[m])*(p->beta[m]) + cotfpi*(cotfpi-FL(2.0)*(p->alpha[m]));
          p->a1[m] = FL(2.0)*((p->alpha[m])*(p->alpha[m]) +
                              (p->beta[m])*(p->beta[m]) - cotfpi2);
          p->a2[m] = (p->alpha[m])*(p->alpha[m]) +
            (p->beta[m])*(p->beta[m]) + cotfpi*(cotfpi+FL(2.0)*(p->alpha[m]));
          p->b0[m] = FL(1.0);
          p->b1[m] = FL(2.0);
          p->b2[m] = FL(1.0);
        }
        break;
      case 1: /* Lowpass Chebyshev type I */
        if (UNLIKELY( pbr < FL(0.0) )) {
          pbr = -pbr;
          csound->Warning(csound, Str("passband ripple must be positive "
                                      "in clfilt. Set to %f"), pbr);
        }
        else if (UNLIKELY( pbr == FL(0.0) )) {
          pbr = FL(1.0);
          csound->Warning(csound, Str("passband ripple must be non-zero in "
                                      "clfilt. Set to %f"), pbr);
        }
        eps = sqrt(pow(10.0,(pbr/10.0))-1.0);
        aleph = 0.5/nsec*log(1.0/eps + sqrt(1.0/eps/eps +1.0));
        for (m=0;m<=nsec-1;m++) {
          bethe = PI*((m + 0.5)/(2.0*nsec) + 0.5);
          p->alpha[m] = (MYFLT)(sinh(aleph)*cos(bethe));
          p->beta[m] = (MYFLT)(cosh(aleph)*sin(bethe));
          p->a0[m] = (p->alpha[m])*(p->alpha[m]) + (p->beta[m])*(p->beta[m])
            + cotfpi*(cotfpi-2*(p->alpha[m]));
          p->a1[m] = FL(2.0)*((p->alpha[m])*(p->alpha[m]) +
                              (p->beta[m])*(p->beta[m]) - cotfpi2);
          p->a2[m] = (p->alpha[m])*(p->alpha[m]) + (p->beta[m])*(p->beta[m])
            + cotfpi*(cotfpi+FL(2.0)*(p->alpha[m]));
          if (m==0) { /* H0 and pole magnitudes */
            p->b0[m] = ((p->alpha[m])*(p->alpha[m]) +
                        (p->beta[m])*(p->beta[m]))*FL(1.0)
              /(MYFLT)sqrt(1.0+eps*eps);
            p->b1[m] = ((p->alpha[m])*(p->alpha[m]) +
                        (p->beta[m])*(p->beta[m]))*FL(2.0)
              /(MYFLT)sqrt(1.0+eps*eps);
            p->b2[m] = ((p->alpha[m])*(p->alpha[m]) +
                        (p->beta[m])*(p->beta[m]))*FL(1.0)
              /(MYFLT)sqrt(1.0+eps*eps);
          }
          else { /* pole magnitudes */
            p->b0[m] = ((p->alpha[m])*(p->alpha[m]) +
                        (p->beta[m])*(p->beta[m]))*FL(1.0);
            p->b1[m] = ((p->alpha[m])*(p->alpha[m]) +
                        (p->beta[m])*(p->beta[m]))*FL(2.0);
            p->b2[m] = ((p->alpha[m])*(p->alpha[m]) +
                        (p->beta[m])*(p->beta[m]))*FL(1.0);
          }
        }
        break;
      case 2: /* Lowpass Chebyshev type II */
        if ( sbr == FL(1.0) ) {
          sbr = FL(-60.0);
        }
        else if (UNLIKELY( sbr > FL(0.0) )) {
          sbr = -sbr;
          csound->Warning(csound, Str("stopband attenuation must be negative "
                                      "in clfilt. Set to %f"), sbr);
        }
        else if (UNLIKELY( sbr == FL(0.0) )) {
          sbr = FL(-60.0);
          csound->Warning(csound, Str("stopband attenuation must be non-zero "
                                      "in clfilt. Set to %f"), sbr);
        }
        eps = sqrt(1.0/(pow(10.0,-(sbr/10.0))-1.0));
        aleph = 0.5/nsec*log(1.0/eps + sqrt(1.0/eps/eps +1.0));
        for (m=0;m<=nsec-1;m++) {
          zee = PI*(m + 0.5)/(2.0*nsec);
          bethe = PI*((m + 0.5)/(2.0*nsec) + 0.5);
          p->alpha[m] = (MYFLT)(sinh(aleph)*cos(bethe));
          p->beta[m] = (MYFLT)(cosh(aleph)*sin(bethe));
          p->odelta2[m] = (MYFLT)(cos(zee)*cos(zee));
          p->a0[m] = (p->alpha[m])*(p->alpha[m]) + (p->beta[m])*(p->beta[m]) +
            tanfpi*(tanfpi - FL(2.0)*(p->alpha[m]));
          p->a1[m] = FL(2.0)*(tanfpi2 - ((p->alpha[m])*(p->alpha[m]) +
                                         (p->beta[m])*(p->beta[m])));
          p->a2[m] = (p->alpha[m])*(p->alpha[m]) + (p->beta[m])*(p->beta[m]) +
            tanfpi*(tanfpi + FL(2.0)*(p->alpha[m]));
          p->b0[m] = p->odelta2[m] + tanfpi2;
          p->b1[m] = FL(2.0)*(tanfpi2 - p->odelta2[m]);
          p->b2[m] = p->odelta2[m] + tanfpi2;
                                }
        break;
      case 3: /* Lowpass Elliptical */
        return
          csound->InitError(csound,
                            "%s", Str("Lowpass Elliptical not implemented yet. Sorry!"));
        break;
      default: /* Because of earlier conditionals, should never get here. */
        return csound->InitError(csound, "%s", Str("code error, ikind out of range"));
      }
      break;
    case 1: /* Highpass filters */
      switch (p->ikind) {
      case 0: /* Highpass Butterworth */
        for (m=0;m<=nsec-1;m++) {
          p->alpha[m] = (MYFLT)cos(PI*((m + 0.5)/(2.0*nsec) + 0.5));
          p->beta[m]  = (MYFLT)sin(PI*((m + 0.5)/(2.0*nsec) + 0.5));
          p->a0[m] = (p->alpha[m])*(p->alpha[m]) + (p->beta[m])*(p->beta[m])
            + tanfpi*(tanfpi - FL(2.0)*(p->alpha[m]));
          p->a1[m] = FL(2.0)*(tanfpi2-((p->alpha[m])*(p->alpha[m]) +
                                       (p->beta[m])*(p->beta[m])));
          p->a2[m] = (p->alpha[m])*(p->alpha[m]) + (p->beta[m])*(p->beta[m])
            + tanfpi*(tanfpi + FL(2.0)*(p->alpha[m]));
          p->b0[m] = FL(1.0);
          p->b1[m] = -FL(2.0);
          p->b2[m] = FL(1.0);
        }
        break;
      case 1: /* Highpass Chebyshev type I */
        if (UNLIKELY( pbr < FL(0.0) )) {
          pbr = -pbr;
          csound->Warning(csound,
                          Str("passband ripple must be positive in clfilt. "
                              "Set to %f"), pbr);
        }
        else if (UNLIKELY( pbr == FL(0.0) )) {
          pbr = FL(1.0);
          csound->Warning(csound, Str("passband ripple must be non-zero "
                                      "in clfilt. Set to %f"), pbr);
        }
        eps = sqrt((pow(10.0,(pbr/10.0))-1.0));
        aleph = 0.5/nsec*log(1.0/eps + sqrt(1.0/eps/eps +1.0));
        for (m=0;m<=nsec-1;m++) {
          bethe = PI*((m + 0.5)/(2.0*nsec) + 0.5);
          p->alpha[m] = (MYFLT)(sinh(aleph)*cos(bethe));
          p->beta[m] = (MYFLT)(cosh(aleph)*sin(bethe));
          p->a0[m] = (p->alpha[m])*(p->alpha[m]) + (p->beta[m])*(p->beta[m])
            + tanfpi*(tanfpi - FL(2.0)*(p->alpha[m]));
          p->a1[m] = FL(2.0)*(tanfpi2-((p->alpha[m])*(p->alpha[m]) +
                                       (p->beta[m])*(p->beta[m])));
          p->a2[m] = (p->alpha[m])*(p->alpha[m]) + (p->beta[m])*(p->beta[m]) +
            tanfpi*(tanfpi + FL(2.0)*(p->alpha[m]));
          if (m==0) { /* H0 and pole magnitudes */
            p->b0[m] = ((p->alpha[m])*(p->alpha[m]) +
                        (p->beta[m])*(p->beta[m]))*FL(1.0)
              /(MYFLT)sqrt(1.0+eps*eps);
            p->b1[m] = -((p->alpha[m])*(p->alpha[m]) +
                         (p->beta[m])*(p->beta[m]))*FL(2.0)
              /(MYFLT)sqrt(1.0+eps*eps);
            p->b2[m] = ((p->alpha[m])*(p->alpha[m]) +
                        (p->beta[m])*(p->beta[m]))*FL(1.0)
              /(MYFLT)sqrt(1.0+eps*eps);
          }
          else { /* pole magnitudes */
            p->b0[m] = ((p->alpha[m])*(p->alpha[m]) +
                        (p->beta[m])*(p->beta[m]))*FL(1.0);
            p->b1[m] = -((p->alpha[m])*(p->alpha[m]) +
                         (p->beta[m])*(p->beta[m]))*FL(2.0);
            p->b2[m] = ((p->alpha[m])*(p->alpha[m]) +
                        (p->beta[m])*(p->beta[m]))*FL(1.0);
          }
        }
        break;
      case 2: /* Highpass Chebyshev type II */
        if ( sbr == FL(1.0) ) {
          sbr = FL(-60.0);
        }
        else if (UNLIKELY( sbr > FL(0.0) )) {
          sbr = -sbr;
          csound->Warning(csound, Str("stopband attenuation must be negative "
                                      "in clfilt. Set to %f"), sbr);
        }
        else if (UNLIKELY( sbr == FL(0.0) )) {
          sbr = FL(-60.0);
          csound->Warning(csound, Str("stopband attenuation must be non-zero "
                                      "in clfilt. Set to %f"), sbr);
        }
        eps = sqrt(1.0/(pow(10.0,-(sbr/10.0))-1.0));
        aleph = 0.5/nsec*log(1.0/eps + sqrt(1.0/eps/eps +1.0));
        for (m=0;m<=nsec-1;m++) {
          zee = PI*(m + 0.5)/(2.0*nsec);
          bethe = PI*((m + 0.5)/(2.0*nsec) + 0.5);
          p->alpha[m] = (MYFLT)(sinh(aleph)*cos(bethe));
          p->beta[m] = (MYFLT)(cosh(aleph)*sin(bethe));
          p->odelta2[m] = (MYFLT)(cos(zee)*cos(zee));
          p->a0[m] = (p->alpha[m])*(p->alpha[m]) + (p->beta[m])*(p->beta[m]) +
            cotfpi*(cotfpi - FL(2.0)*(p->alpha[m]));
          p->a1[m] = FL(2.0)*(-cotfpi2 + ((p->alpha[m])*(p->alpha[m]) +
                                          (p->beta[m])*(p->beta[m])));
          p->a2[m] = (p->alpha[m])*(p->alpha[m]) + (p->beta[m])*(p->beta[m]) +
            cotfpi*(cotfpi + FL(2.0)*(p->alpha[m]));
          p->b0[m] = p->odelta2[m] + cotfpi2;
          p->b1[m] = FL(2.0)*(-cotfpi2 + p->odelta2[m]);
          p->b2[m] = p->odelta2[m] + cotfpi2;
        }
        break;
      case 3: /* Highpass Elliptical */
        return csound->InitError(csound, "%s", Str("Highpass Elliptical "
                                             "not implemented yet. Sorry!"));
        break;
      default: /* Because of earlier conditionals, should never get here. */
        return csound->InitError(csound, "%s", Str("code error, ikind out of range"));
      }
      break;
    default: /* Because of earlier conditionals, should never get here. */
      return csound->InitError(csound, "%s", Str("code error, ihilo out of range"));
    }
    if (*p->reinit==FL(0.0)) {      /* Only reset in in non-legato mode */
      for (m=0;m<=nsec-1;m++) {
        p->xnm1[m] = p->xnm2[m] = p->ynm1[m] = p->ynm2[m] = FL(0.0);
      }
    }
    return OK;
} /* end clfiltset(p) */

static int32_t clfilt(CSOUND *csound, CLFILT *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t m, nsec;
    MYFLT *out, *in;
    MYFLT xn[CL_LIM+1], yn[CL_LIM];
    MYFLT a0[CL_LIM], a1[CL_LIM], a2[CL_LIM];
    MYFLT b0[CL_LIM], b1[CL_LIM], b2[CL_LIM];
    MYFLT xnm1[CL_LIM], xnm2[CL_LIM], ynm1[CL_LIM], ynm2[CL_LIM];
    MYFLT tanfpi, tanfpi2, cotfpi, cotfpi2;
    nsec = p->nsec;
    for (m=0;m<=nsec-1;m++) {
      a0[m] = p->a0[m]; a1[m] = p->a1[m]; a2[m] = p->a2[m];
      b0[m] = p->b0[m]; b1[m] = p->b1[m]; b2[m] = p->b2[m];
      xnm1[m] = p->xnm1[m]; xnm2[m] = p->xnm2[m];
      ynm1[m] = p->ynm1[m]; ynm2[m] = p->ynm2[m];
    }
    if (*p->freq != p->prvfreq) {      /* Only reset if freq changes */
      p->prvfreq = *p->freq;
      tanfpi = (MYFLT)tan(-CS_MPIDSR*(*p->freq));
      tanfpi2 = tanfpi*tanfpi;
      cotfpi = FL(1.0)/tanfpi;
      cotfpi2 = cotfpi*cotfpi;
      switch (p->ilohi) {
      case 0: /* Lowpass filters */
        switch (p->ikind) {
        case 0: /* Lowpass Butterworth */
        case 1: /* Lowpass Chebyshev type I */
          for (m=0;m<=nsec-1;m++) {
            p->a0[m] = a0[m] = (p->alpha[m])*(p->alpha[m]) +
              (p->beta[m])*(p->beta[m]) + cotfpi*(cotfpi-FL(2.0)*(p->alpha[m]));
            p->a1[m] = a1[m] = FL(2.0)*((p->alpha[m])*(p->alpha[m]) +
                                        (p->beta[m])*(p->beta[m]) - cotfpi2);
            p->a2[m] = a2[m] = (p->alpha[m])*(p->alpha[m]) +
              (p->beta[m])*(p->beta[m]) + cotfpi*(cotfpi+FL(2.0)*(p->alpha[m]));
          }
          break;
        case 2: /* Lowpass Chebyshev type II */
          for (m=0;m<=nsec-1;m++) {
            p->a0[m] = a0[m] = (p->alpha[m])*(p->alpha[m]) +
              (p->beta[m])*(p->beta[m]) + tanfpi*(tanfpi - FL(2.0)*(p->alpha[m]));
            p->a1[m] = a1[m] = FL(2.0)*(tanfpi2 - ((p->alpha[m])*(p->alpha[m]) +
                                                   (p->beta[m])*(p->beta[m])));
            p->a2[m] = a2[m] = (p->alpha[m])*(p->alpha[m]) +
              (p->beta[m])*(p->beta[m]) + tanfpi*(tanfpi + FL(2.0)*(p->alpha[m]));
            p->b0[m] = b0[m] = p->odelta2[m] + tanfpi2;
            p->b1[m] = b1[m] = FL(2.0)*(tanfpi2 - p->odelta2[m]);
            p->b2[m] = b2[m] = p->odelta2[m] + tanfpi2;
          }
          break;
        case 3: /* Lowpass Elliptical */
          return csound->PerfError(csound, &(p->h),
                                   "%s", Str("Lowpass Elliptical "
                                               "not implemented yet. Sorry!"));
          break;
        default: /* Because of earlier contditionals, should never get here. */
          return csound->PerfError(csound, &(p->h),
                                   "%s", Str("code error, ikind out of range"));
        }
        break;
      case 1: /* Highpass filters */
        switch (p->ikind) {
        case 0: /* Highpass Butterworth */
        case 1: /* Highpass Chebyshev type I */
          for (m=0;m<=nsec-1;m++) {
            p->a0[m] = a0[m] = (p->alpha[m])*(p->alpha[m]) +
              (p->beta[m])*(p->beta[m]) +
              tanfpi*(tanfpi - FL(2.0)*(p->alpha[m]));
            p->a1[m] = a1[m] = FL(2.0)*(tanfpi2-((p->alpha[m])*(p->alpha[m]) +
                                                 (p->beta[m])*(p->beta[m])));
            p->a2[m] = a2[m] = (p->alpha[m])*(p->alpha[m]) +
              (p->beta[m])*(p->beta[m]) +
              tanfpi*(tanfpi + FL(2.0)*(p->alpha[m]));
          }
          break;
        case 2: /* Highpass Chebyshev type II */
          for (m=0;m<=nsec-1;m++) {
            p->a0[m] = a0[m] = (p->alpha[m])*(p->alpha[m]) +
              (p->beta[m])*(p->beta[m]) + cotfpi*(cotfpi - FL(2.0)*(p->alpha[m]));
            p->a1[m] = a1[m] = FL(2.0)*(-cotfpi2 + ((p->alpha[m])*(p->alpha[m]) +
                                                    (p->beta[m])*(p->beta[m])));
            p->a2[m] = a2[m] = (p->alpha[m])*(p->alpha[m]) +
              (p->beta[m])*(p->beta[m]) + cotfpi*(cotfpi + FL(2.0)*(p->alpha[m]));
            p->b0[m] = b0[m] = p->odelta2[m] + cotfpi2;
            p->b1[m] = b1[m] = FL(2.0)*(-cotfpi2 + p->odelta2[m]);
            p->b2[m] = b2[m] = p->odelta2[m] + cotfpi2;
          }
          break;
        case 3: /* Highpass Elliptical */
          return csound->PerfError(csound, &(p->h),
                                   "%s", Str("Highpass Elliptical "
                                       "not implemented yet. Sorry!"));
          break;
        default: /* Because of earlier contditionals, should never get here. */
          return csound->PerfError(csound, &(p->h),
                                   "%s", Str("code error, ikind out of range"));
        }
        break;
      default: /* Because of earlier conditionals, should never get here. */
        return csound->PerfError(csound, &(p->h),
                                 "%s", Str("code error, ihilo out of range"));
      }
    }
    in   = p->in;
    out  = p->out;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      xn[0] = in[n];
      for (m=0;m<=nsec-1;m++) {
        yn[m] = (b0[m]*xn[m] + b1[m]*xnm1[m] + b2[m]*xnm2[m] -
                 a1[m]*ynm1[m] - a2[m]*ynm2[m])/a0[m];
        xnm2[m] = xnm1[m];
        xnm1[m] = xn[m];
        ynm2[m] = ynm1[m];
        ynm1[m] = yn[m];
        xn[m+1] = yn[m];
      }
      out[n] = yn[nsec-1];
    }
    for (m=0;m<=nsec-1;m++) {
      p->xnm1[m] = xnm1[m]; p->xnm2[m] = xnm2[m];
      p->ynm1[m] = ynm1[m]; p->ynm2[m] = ynm2[m];
    }
    return OK;
} /* end clfilt(p) */

#define S sizeof

static OENTRY localops[] = {
{ "clfilt", S(CLFILT),  0,  "a", "akiioppo",(SUBR)clfiltset, (SUBR)clfilt },
};

int32_t clfilt_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}

