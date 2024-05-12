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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
    MYFLT  lastframe;
} PVSBAND;


static int32_t pvsbandinit(CSOUND *csound, PVSBAND *p)
{
    int32_t     N = p->fin->N;

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

static int32_t pvsband(CSOUND *csound, PVSBAND *p)
{
    int32_t     i, N = p->fin->N;
    MYFLT   lowcut = *p->klowcut;
    MYFLT   lowbnd = *p->klowbnd;
    MYFLT   higbnd = *p->khigbnd;
    MYFLT   higcut = *p->khigcut;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;
    MYFLT   fade = *p->fade;
    MYFLT   opef = FL(1.0) - EXP(fade);

    if (UNLIKELY(fout == NULL)) goto err1;

    if (lowcut<FL(0.0)) lowcut = FL(0.0);
    if (lowbnd<lowcut) lowbnd = lowcut;
    if (higbnd<lowbnd) higbnd = lowbnd;
    if (higcut<higbnd) higcut = higbnd;
    if (p->fin->sliding) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      int32_t NB  = p->fout->NB;

      if (UNLIKELY(early)) nsmps -= early;
      for (n=offset; n<nsmps; n++) {
        int32_t change = 0;
        CMPLX *fin = (CMPLX *) p->fin->frame.auxp + n*NB;
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        if (IS_ASIG_ARG(p->klowcut)) lowcut = p->klowcut[n], change = 1;
        if (IS_ASIG_ARG(p->klowbnd)) lowbnd = p->klowbnd[n], change = 1;
        if (IS_ASIG_ARG(p->khigbnd)) higbnd = p->khigbnd[n], change = 1;
        if (IS_ASIG_ARG(p->khigcut)) higcut = p->khigcut[n], change = 1;
        if (change) {
          if (lowcut<FL(0.0)) lowcut = FL(0.0);
          if (lowbnd<lowcut) lowbnd = lowcut;
          if (higbnd<lowbnd) higbnd = lowbnd;
          if (higcut<higbnd) higcut = higbnd;
        }
        for (i = 0; i < NB-1; i++) {
          MYFLT frq = fin[i].im;
          MYFLT afrq = (frq<FL(0.0)? -frq : frq);
          if (afrq < lowcut || afrq>higcut) { /* outside band */
            fout[i].re = FL(0.0);
            fout[i].im = -FL(1.0);
          }
          else if (afrq > lowbnd && afrq<higbnd) { /* inside nand */
            fout[i] = fin[i];
          }
          else if (afrq > lowcut && afrq < lowbnd) { /* ramp up */
            if (fade != FL(0.0)) {
              fout[i].re = fin[i].re *
                (FL(1.0) - EXP(fade*(afrq-lowcut)/(lowbnd-lowcut)))/opef;
            }
            else
              fout[i].re = fin[i].re * (afrq - lowcut)/(lowbnd - lowcut);
            fout[i].im = frq;
          }
          else {                /* ramp down */
            if (fade != FL(0.0)) {
              fout[i].re = fin[i].re *
                (FL(1.0) - EXP(fade*(higcut-afrq)/(higcut-higbnd)))/opef;
            }
            else
              fout[i].re = fin[i].re * (higcut - afrq)/(higcut - higbnd);
            fout[i].im = frq;
          }
        }
      }
      return OK;
    }
    if (p->lastframe < p->fin->framecount) {
      for (i = 0; i < N; i += 2) {
        MYFLT frq = fin[i+1];
        MYFLT afrq = (frq<FL(0.0)? -frq : frq);
        if (afrq < lowcut || afrq>higcut) {
            fout[i] = FL(0.0);
            fout[i+1] = -FL(1.0);
          }
          else if (afrq > lowbnd && afrq<higbnd) {
            fout[i] = fin[i];
            fout[i+1] = fin[i+1];
          }
          else if (afrq > lowcut && afrq < lowbnd) {
            if (fade != FL(0.0))
              fout[i] = fin[i] *
                (1.0f - expf(fade*(afrq-lowcut)/(lowbnd-lowcut)))/opef;
            else
              fout[i] = fin[i] * (frq - lowcut)/(lowbnd - lowcut);
            fout[i+1] = frq;
          }
          else {
            if (fade != FL(0.0))
              fout[i] = fin[i] *
                (1.0f - expf(fade*(higcut-afrq)/(higcut-higbnd)))/opef;
            else
              fout[i] = fin[i] * (higcut - frq)/(higcut - higbnd);
            fout[i+1] = frq;
          }
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
    int32_t     i, N = p->fin->N;
    MYFLT   lowcut = *p->klowcut;
    MYFLT   lowbnd = *p->klowbnd;
    MYFLT   higbnd = *p->khigbnd;
    MYFLT   higcut = *p->khigcut;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;
    MYFLT   fade = *p->fade;
    MYFLT   opef = FL(1.0) - EXP(fade);

    if (UNLIKELY(fout == NULL)) goto err1;

    if (lowcut<FL(0.0)) lowcut = FL(0.0);
    if (lowbnd<lowcut) lowbnd = lowcut;
    if (higbnd<lowbnd) higbnd = lowbnd;
    if (higcut<higbnd) higcut = higbnd;
    if (p->fin->sliding) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      int32_t NB  = p->fout->NB;

      if (UNLIKELY(early)) nsmps -= early;
      for (n=offset; n<nsmps; n++) {
        int32_t change = 0;
        CMPLX *fin = (CMPLX *) p->fin->frame.auxp + n*NB;
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        if (IS_ASIG_ARG(p->klowcut)) lowcut = p->klowcut[n], change = 1;
        if (IS_ASIG_ARG(p->klowbnd)) lowbnd = p->klowbnd[n], change = 1;
        if (IS_ASIG_ARG(p->khigbnd)) higbnd = p->khigbnd[n], change = 1;
        if (IS_ASIG_ARG(p->khigcut)) higcut = p->khigcut[n], change = 1;
        if (change) {
          if (lowcut<FL(0.0)) lowcut = FL(0.0);
          if (lowbnd<lowcut) lowbnd = lowcut;
          if (higbnd<lowbnd) higbnd = lowbnd;
          if (higcut<higbnd) higcut = higbnd;
        }
        for (i = 0; i < NB-1; i++) {
          MYFLT frq = fin[i].im;
          MYFLT afrq = (frq<FL(0.0)? -frq : frq);
          if (afrq < lowcut || afrq>higcut) {
            fout[i] = fin[i];
          }
          else if (afrq > lowbnd && afrq<higbnd) {
            fout[i].re = FL(0.0);
            fout[i].im = -FL(1.0);
          }
          else if (afrq > lowcut && afrq < lowbnd) {
            if (fade)
              fout[i].re = fin[i].re *
                (FL(1.0) - EXP(fade*(afrq-lowcut)/(lowbnd-lowcut)))/opef;
            else
              fout[i].re = fin[i].re * (lowbnd - afrq)/(lowbnd - lowcut);
            fout[i].im = frq;
          }
          else {
            if (fade)
              fout[i].re = fin[i].re *
                (FL(1.0) - EXP(fade*(afrq-higbnd)/(higcut-higbnd)))/opef;
            else
              fout[i].re = fin[i].re * (afrq - higbnd)/(higcut - higbnd);
            fout[i].im = frq;
          }
        }
      }
      return OK;
    }
    if (p->lastframe < p->fin->framecount) {
      for (i = 0; i < N; i += 2) {
        MYFLT frq = fin[i+1];
        MYFLT afrq = (frq<FL(0.0)? -frq : frq);
        if (afrq < lowcut || afrq>higcut) {
            fout[i] = fin[i];
            fout[i+1] = fin[i+1];
          }
          else if (afrq > lowbnd && afrq<higbnd) {
            fout[i] = FL(0.0);
            fout[i+1] = -FL(1.0);
          }
          else if (afrq > lowcut && afrq < lowbnd) {
            if (fade != FL(0.0))
              fout[i] = fin[i] *
                (1.0f - expf(fade*(lowbnd - afrq)/(lowbnd - lowcut)))/opef;
            else
              fout[i] = fin[i] * (lowbnd - afrq)/(lowbnd - lowcut);
            fout[i+1] = frq;
          }
          else {
            if (fade != FL(0.0))
              fout[i] = fin[i] *
                (1.0f - expf(fade*(afrq - higbnd)/(higcut - higbnd)))/opef;
            else
              fout[i] = fin[i] * (afrq - higbnd)/(higcut - higbnd);
            fout[i+1] = frq;
          }
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("pvsband: not initialised"));
}

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
