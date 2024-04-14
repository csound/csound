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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
} PVSGENDY;


static int32_t pvsgendyinit(CSOUND *csound, PVSGENDY *p)
{
    int32_t     N = p->fin->N;

    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, "%s", Str("Unsafe to have same fsig as in and out"));

    if (UNLIKELY(p->fin->sliding)) {
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

static int32_t pvsgendy(CSOUND *csound, PVSGENDY *p)
{
    int32_t     i, N = p->fin->N;
    MYFLT   mrate = *p->kmrate;
    MYFLT   frate = *p->kfrate;
    float   *finf = (float *) p->fin->frame.auxp;
    float   *foutf = (float *) p->fout->frame.auxp;

    if (UNLIKELY(foutf == NULL)) goto err1;

    if (p->fin->sliding) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      int32_t NB  = p->fout->NB;
      for (n=0; n<offset; n+=2) foutf[n] = foutf[n+1] = FL(0.0);
      for (n=nsmps-early; n<nsmps; n+=2) foutf[n] = foutf[n+1] = FL(0.0);
      nsmps -= early;
      for (n=offset; n<nsmps; n++) {
        //int32_t change = 0;
        CMPLX *fin = (CMPLX *) p->fin->frame.auxp + n*NB;
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        for (i = 0; i < NB-1; i++) {
          MYFLT x = (MYFLT)(rand()-RAND_MAX/2)/(MYFLT)RAND_MAX;
          //          printf("%f\n", x);
          fout[i].re = fin[i].re + mrate * x;
          fout[i].im = fin[i].im +
            frate * (MYFLT)(rand()-RAND_MAX/2)/(MYFLT)RAND_MAX/(MYFLT)(i+1);
        }
      }
      return OK;
    }
    if (p->lastframe < p->fin->framecount) {
      for (i = 0; i < N; i += 2) {
        MYFLT x = frate * (MYFLT)(rand()-RAND_MAX/2)/(MYFLT)RAND_MAX/(MYFLT)(i+1);
        foutf[i+1] = finf[i+1] + x;
        foutf[i] = finf[i] ;//+ mrate * (MYFLT)(rand()-RAND_MAX/2)/(MYFLT)RAND_MAX;
      }
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



