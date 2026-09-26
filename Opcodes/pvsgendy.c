/*
    pvsgendy.c:
    gendy style transformation in frequency domain

    Copyright (c) John ffitch, 2009

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
    MYFLT  *kmrate;
    MYFLT  *kfrate;
    uint32_t lastframe;
    uint32_t seed;
    size_t framebytes;
} PVSGENDY;

/* Rand31's recurrence, with instance-local state and no call for each bin. */
#define PVSGENDY_RANDOM(state, value) do {                                  \
    uint64_t product_ = (uint64_t)(state) * UINT64_C(742938285);             \
    uint32_t next_ = ((uint32_t)product_ & UINT32_C(0x7fffffff)) +           \
                     (uint32_t)(product_ >> 31);                           \
    (state) = (next_ & UINT32_C(0x7fffffff)) + (next_ >> 31);                \
    (value) = (MYFLT)((double)(state) * (1.0 / 2147483647.0) - 0.5);        \
} while (0)

static int32_t pvsgendyinit(CSOUND *csound, PVSGENDY *p)
{
    int32_t     N = p->fin->N;
    size_t stride, samples;

    if (UNLIKELY(p->fin == p->fout))
      return csound->InitError(csound, "%s",
                              Str("pvsgendy: input and output must differ"));
    if (UNLIKELY(p->fin->format != PVS_AMP_FREQ))
      return csound->InitError(csound, "%s",
                              Str("pvsgendy: input format must be amp-freq"));
    if (UNLIKELY(N < 2 || N > INT32_MAX - 2 || (N & 1) ||
                 (p->fin->sliding && p->fin->NB != N / 2 + 1)))
      return csound->InitError(csound, "%s",
                              Str("pvsgendy: invalid input frame size"));

    stride = p->fin->sliding ? sizeof(MYFLT) : sizeof(float);
    samples = p->fin->sliding ? CS_KSMPS : 1;
    if (UNLIKELY((size_t)(N + 2) > SIZE_MAX / stride / samples))
      return csound->InitError(csound, "%s",
                              Str("pvsgendy: input frame is too large"));
    p->framebytes = (size_t)(N + 2) * stride * samples;
    if (UNLIKELY(p->fin->frame.auxp == NULL ||
                 p->fin->frame.size < p->framebytes))
      return csound->InitError(csound, "%s",
                              Str("pvsgendy: input frame is not initialised"));
    csound->AuxAlloc(csound, p->framebytes, &p->fout->frame);
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    p->fout->sliding = p->fin->sliding;
    p->fout->NB = N / 2 + 1;
    p->seed = csound->Rand31(csound->RandSeed31(csound));
    return OK;
}

static int32_t pvsgendy(CSOUND *csound, PVSGENDY *p)
{
    int32_t     i, N = p->fout->N;
    uint32_t seed = p->seed;
    MYFLT   mrate = *p->kmrate;
    MYFLT   frate = *p->kfrate;
    float   *finf = (float *) p->fin->frame.auxp;
    float   *foutf = (float *) p->fout->frame.auxp;

    if (UNLIKELY(finf == NULL || foutf == NULL ||
                 p->fin->frame.size < p->framebytes ||
                 p->fout->frame.size < p->framebytes)) goto err1;
    if (UNLIKELY(p->fin->N != N ||
                 p->fin->sliding != p->fout->sliding ||
                 p->fin->format != p->fout->format ||
                 p->fin->overlap != p->fout->overlap ||
                 p->fin->winsize != p->fout->winsize ||
                 p->fin->wintype != p->fout->wintype ||
                 (p->fin->sliding && p->fin->NB != p->fout->NB)))
      return csound->PerfError(csound, &(p->h), "%s",
                         Str("pvsgendy: analysis settings changed; reinitialise"));

    if (p->fin->sliding) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      int32_t NB  = p->fout->NB;
      CMPLX *output = (CMPLX *) p->fout->frame.auxp;
      nsmps -= early;
      /* Each inactive audio sample contains NB complete complex pairs. */
      if (offset)
        memset(output, 0, (size_t)offset * NB * sizeof(CMPLX));
      if (early)
        memset(output + (size_t)nsmps * NB, 0,
               (size_t)early * NB * sizeof(CMPLX));
      for (n=offset; n<nsmps; n++) {
        CMPLX *fin = (CMPLX *) p->fin->frame.auxp + (size_t)n*NB;
        CMPLX *fout = output + (size_t)n*NB;
        for (i = 0; i < NB; i++) {
          MYFLT x;
          PVSGENDY_RANDOM(seed, x);
          fout[i].re = fin[i].re + mrate * x;
          PVSGENDY_RANDOM(seed, x);
          fout[i].im = fin[i].im + frate * x / (MYFLT)(i+1);
        }
      }
      p->seed = seed;
      p->fout->framecount = p->fin->framecount;
      return OK;
    }
    if (p->lastframe != p->fin->framecount) {
      for (i = 0; i < N + 2; i += 2) {
        MYFLT x;
        PVSGENDY_RANDOM(seed, x);
        foutf[i+1] = finf[i+1] + frate * x / (MYFLT)(i+1);
        /* Ordinary frames retain their input amplitudes. */
        foutf[i] = finf[i];
      }
      p->seed = seed;
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("pvsgendy: not initialised"));
}

static OENTRY pvsgendy_localops[] = {
  { "pvsgendy", sizeof(PVSGENDY), 0,  "f", "fkk",
                (SUBR) pvsgendyinit, (SUBR) pvsgendy, (SUBR) NULL }
};

int32_t pvsgendy_localops_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(pvsgendy_localops[0]),
                               (int32_t) (sizeof(pvsgendy_localops) / sizeof(OENTRY)));
}


