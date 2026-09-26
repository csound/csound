/*
    pvsband.c:
    bandpass filter transformation of streaming PV signals

    Copyright (c) John ffitch, 2007

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

#include "pvs_ops.h"
#include "pstream.h"

typedef struct {
    OPDS h;
    PVSDAT *fout;
    PVSDAT *fin;
    MYFLT  *klowcut;
    MYFLT  *klowbnd;
    MYFLT  *khigbnd;
    MYFLT  *khigcut;
    MYFLT  *fade;
    uint32 lastframe;
} PVSBAND;


static int32_t pvsbandinit(CSOUND *csound, PVSBAND *p)
{
    int32_t     N = p->fin->N;

    if (UNLIKELY(p->fin->format != PVS_AMP_FREQ))
      return csound->InitError(csound, "%s",
                               Str("pvsband: input must be amp-freq"));

    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, "%s", Str("Unsafe to have same fsig as in and out"));

    if (p->fin->sliding) {
      if (p->fout->frame.auxp==NULL ||
          CS_KSMPS*(N+2)*sizeof(MYFLT) > (uint32_t)p->fout->frame.size)
        csound->AuxAlloc(csound, CS_KSMPS*(N+2)*sizeof(MYFLT),&p->fout->frame);
      else memset(p->fout->frame.auxp, 0, CS_KSMPS*(N+2)*sizeof(MYFLT));
    }
    else
      {
        if (p->fout->frame.auxp == NULL ||
            p->fout->frame.size < (N+2)*sizeof(float))  /* RWD MUST be 32bit */
          csound->AuxAlloc(csound, (N+2)*sizeof(float), &p->fout->frame);
        else memset(p->fout->frame.auxp, 0, (N+2)*sizeof(float));
      }
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    p->fout->sliding = p->fin->sliding;
    p->fout->NB = p->fin->NB;
    return OK;
}

/* Use the same boundaries and curves for FFT and sliding spectra.
   Inner band boundaries take precedence over cutoffs when a ramp has no width. */
#define PVSBAND_LIMITS() do {                                            \
    if (lowcut < FL(0.0)) lowcut = FL(0.0);                               \
    if (lowbnd < lowcut) lowbnd = lowcut;                                 \
    if (higbnd < lowbnd) higbnd = lowbnd;                                 \
    if (higcut < higbnd) higcut = higbnd;                                 \
  } while (0)

/* These macros use the current cutoffs and the block's curve constants.
   Keep exponents nonpositive for large positive curves; expm1 avoids
   cancellation for curves near zero. Only ramps need a transcendental call. */
#define PVSBAND_GAIN(freq, reject, gain) do {                            \
    MYFLT afrq = FABS(freq), position;                                   \
    if (afrq < lowcut || afrq > higcut)                                  \
      position = (reject) ? FL(1.0) : FL(0.0);                           \
    else if (afrq >= lowbnd && afrq <= higbnd)                            \
      position = (reject) ? FL(0.0) : FL(1.0);                           \
    else if (afrq < lowbnd)                                              \
      position = ((reject) ? lowbnd-afrq : afrq-lowcut) / (lowbnd-lowcut); \
    else                                                                \
      position = ((reject) ? afrq-higbnd : higcut-afrq) / (higcut-higbnd); \
    if (position <= FL(0.0))                                             \
      (gain) = FL(0.0);                                                  \
    else if (position >= FL(1.0))                                        \
      (gain) = FL(1.0);                                                  \
    else if (fade == 0.0)                                                \
      (gain) = position;                                                 \
    else if (fade > 1.0)                                                 \
      (gain) = (exp(fade*((double)position-1.0))-curvebase) / curveden;    \
    else                                                                \
      (gain) = expm1(fade*position) / curveden;                           \
  } while (0)

static int32_t pvsband(CSOUND *csound, PVSBAND *p)
{
    int32_t i, N = p->fin->N;
    MYFLT lowcut = *p->klowcut, lowbnd = *p->klowbnd;
    MYFLT higbnd = *p->khigbnd, higcut = *p->khigcut;
    float *fin = (float *) p->fin->frame.auxp;
    float *fout = (float *) p->fout->frame.auxp;
    double fade = *p->fade;
    double curvebase = fade > 1.0 ? exp(-fade) : 0.0;
    double curveden = fade > 1.0 ? 1.0-curvebase :
      (fade != 0.0 ? expm1(fade) : 1.0);

    if (UNLIKELY(fout == NULL)) goto err1;

    PVSBAND_LIMITS();
    if (p->fin->sliding) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      int32_t NB = p->fout->NB;
      uint32_t lowcutstep = IS_ASIG_ARG(p->klowcut) ? 1 : 0;
      uint32_t lowbndstep = IS_ASIG_ARG(p->klowbnd) ? 1 : 0;
      uint32_t higbndstep = IS_ASIG_ARG(p->khigbnd) ? 1 : 0;
      uint32_t higcutstep = IS_ASIG_ARG(p->khigcut) ? 1 : 0;

      if (UNLIKELY(offset))
        memset(p->fout->frame.auxp, 0, (size_t)offset*NB*sizeof(CMPLX));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset((CMPLX *) p->fout->frame.auxp + (size_t)nsmps*NB, 0,
               (size_t)early*NB*sizeof(CMPLX));
      }
      for (n=offset; n<nsmps; n++) {
        CMPLX *fin = (CMPLX *) p->fin->frame.auxp + (size_t)n*NB;
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + (size_t)n*NB;
        /* Reload k-rate limits too: clamping one sample must not change
           a later sample's limits when an a-rate control crosses them. */
        lowcut = p->klowcut[n*lowcutstep];
        lowbnd = p->klowbnd[n*lowbndstep];
        higbnd = p->khigbnd[n*higbndstep];
        higcut = p->khigcut[n*higcutstep];
        PVSBAND_LIMITS();
        for (i = 0; i < NB; i++) {
          MYFLT gain;
          PVSBAND_GAIN(fin[i].im, 0, gain);
          fout[i].re = fin[i].re * gain;
          fout[i].im = gain == FL(0.0) ? -FL(1.0) : fin[i].im;
        }
      }
      return OK;
    }
    if (p->lastframe < p->fin->framecount) {
      for (i = 0; i <= N; i += 2) {
        MYFLT gain;
        PVSBAND_GAIN(fin[i+1], 0, gain);
        fout[i] = fin[i] * gain;
        fout[i+1] = gain == FL(0.0) ? -FL(1.0) : fin[i+1];
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("pvsband: not initialised"));
}

static int32_t pvsbrej(CSOUND *csound, PVSBAND *p)
{
    int32_t i, N = p->fin->N;
    MYFLT lowcut = *p->klowcut, lowbnd = *p->klowbnd;
    MYFLT higbnd = *p->khigbnd, higcut = *p->khigcut;
    float *fin = (float *) p->fin->frame.auxp;
    float *fout = (float *) p->fout->frame.auxp;
    double fade = *p->fade;
    double curvebase = fade > 1.0 ? exp(-fade) : 0.0;
    double curveden = fade > 1.0 ? 1.0-curvebase :
      (fade != 0.0 ? expm1(fade) : 1.0);

    if (UNLIKELY(fout == NULL)) goto err1;

    PVSBAND_LIMITS();
    if (p->fin->sliding) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      int32_t NB = p->fout->NB;
      uint32_t lowcutstep = IS_ASIG_ARG(p->klowcut) ? 1 : 0;
      uint32_t lowbndstep = IS_ASIG_ARG(p->klowbnd) ? 1 : 0;
      uint32_t higbndstep = IS_ASIG_ARG(p->khigbnd) ? 1 : 0;
      uint32_t higcutstep = IS_ASIG_ARG(p->khigcut) ? 1 : 0;

      if (UNLIKELY(offset))
        memset(p->fout->frame.auxp, 0, (size_t)offset*NB*sizeof(CMPLX));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset((CMPLX *) p->fout->frame.auxp + (size_t)nsmps*NB, 0,
               (size_t)early*NB*sizeof(CMPLX));
      }
      for (n=offset; n<nsmps; n++) {
        CMPLX *fin = (CMPLX *) p->fin->frame.auxp + (size_t)n*NB;
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + (size_t)n*NB;
        /* Reload k-rate limits too: clamping one sample must not change
           a later sample's limits when an a-rate control crosses them. */
        lowcut = p->klowcut[n*lowcutstep];
        lowbnd = p->klowbnd[n*lowbndstep];
        higbnd = p->khigbnd[n*higbndstep];
        higcut = p->khigcut[n*higcutstep];
        PVSBAND_LIMITS();
        for (i = 0; i < NB; i++) {
          MYFLT gain;
          PVSBAND_GAIN(fin[i].im, 1, gain);
          fout[i].re = fin[i].re * gain;
          fout[i].im = gain == FL(0.0) ? -FL(1.0) : fin[i].im;
        }
      }
      return OK;
    }
    if (p->lastframe < p->fin->framecount) {
      for (i = 0; i <= N; i += 2) {
        MYFLT gain;
        PVSBAND_GAIN(fin[i+1], 1, gain);
        fout[i] = fin[i] * gain;
        fout[i+1] = gain == FL(0.0) ? -FL(1.0) : fin[i+1];
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("pvsband: not initialised"));
}

#undef PVSBAND_GAIN
#undef PVSBAND_LIMITS

static OENTRY localops[] = {
  {"pvsbandp", sizeof(PVSBAND), 0,  "f", "fxxxxO",
                    (SUBR) pvsbandinit, (SUBR) pvsband, (SUBR) NULL },
  {"pvsbandr", sizeof(PVSBAND), 0,  "f", "fxxxxO",
                    (SUBR) pvsbandinit, (SUBR) pvsbrej, (SUBR) NULL }
};

int32_t pvsband_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int32_t
                                ) (sizeof(localops) / sizeof(OENTRY)));
}
