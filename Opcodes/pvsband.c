/*
    pvsband.c:
    bandpass filter transformation of streaming PV signals

    (c) John ffitch, 2007

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

#include "csdl.h"
#include "pstream.h"

typedef struct {
    OPDS h;
    PVSDAT *fout;
    PVSDAT *fin;
    MYFLT  *klowcut;
    MYFLT  *klowbnd;
    MYFLT  *khigbnd;
    MYFLT  *khigcut;
    MYFLT  lastframe;
} PVSBAND;


static int pvsbandinit(CSOUND *csound, PVSBAND *p)
{
    int     N = p->fin->N;

    if (p->fin == p->fout)
      csound->Warning(csound, "Unsafe to have same fsig as in and out");

#ifdef SDFT
    if (p->fin->sliding) {
      if (p->fout->frame.auxp==NULL ||
          csound->ksmps*(N+2)*sizeof(MYFLT) > (unsigned int)p->fout->frame.size)
        csound->AuxAlloc(csound, csound->ksmps*(N+2)*sizeof(MYFLT),&p->fout->frame);
      else memset(p->fout->frame.auxp, 0, csound->ksmps*(N+2)*sizeof(MYFLT));
    }
    else
#endif
      {
        if (p->fout->frame.auxp == NULL ||
            p->fout->frame.size < (N+2)*sizeof(float))  /* RWD MUST be 32bit */
          csound->AuxAlloc(csound, (N+2)*sizeof(float), &p->fout->frame);
        else memset(p->fout->frame.auxp, 0, (N+2)*sizeof(MYFLT));
      }
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
#ifdef SDFT
    p->fout->sliding = p->fin->sliding;
    p->fout->NB = p->fin->NB;
#endif
    return OK;
}

static int pvsband(CSOUND *csound, PVSBAND *p)
{
    int     i, N = p->fin->N;
    MYFLT   lowcut = *p->klowcut;
    MYFLT   lowbnd = *p->klowbnd;
    MYFLT   higbnd = *p->khigbnd;
    MYFLT   higcut = *p->khigcut;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;

    if (fout == NULL)
      return csound->PerfError(csound, Str("pvsband: not initialised"));

    if (lowcut<FL(0.0)) lowcut = FL(0.0);
    if (lowbnd<lowcut) lowbnd = lowcut;
    if (higbnd<lowbnd) higbnd = lowbnd;
    if (higcut<higbnd) higcut = higbnd;
#ifdef SDFT
    if (p->fin->sliding) {
      int n, nsmps = csound->ksmps;
      int NB  = p->fout->NB;

      for (n=0; n<nsmps; n++) {
        int change = 0;
        CMPLX *fin = (CMPLX *) p->fin->frame.auxp + n*NB;
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        if (XINARG2) lowcut = p->klowcut[n], change = 1;
        if (XINARG3) lowbnd = p->klowbnd[n], change = 1;
        if (XINARG4) higbnd = p->khigbnd[n], change = 1;
        if (XINARG5) higcut = p->khigcut[n], change = 1;
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
            fout[i].re = FL(0.0);
            fout[i].im = -FL(1.0);
          }
          else if (afrq > lowbnd && afrq<higbnd) {
            fout[i] = fin[i];
          }
          else if (afrq > lowcut && afrq < lowbnd) {
            fout[i].re = fin[i].re * (afrq - lowcut)/(lowbnd - lowcut);
            fout[i].im = frq;
          }
          else {
            fout[i].re = fin[i].re * (higcut - afrq)/(higcut - higbnd);
            fout[i].im = frq;
          }
        }
      }
      return OK;
    }
#endif
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
            fout[i] = fin[i] * (frq - lowcut)/(lowbnd - lowcut);
            fout[i+1] = frq;
          }
          else {
            fout[i] = fin[i] * (higcut - frq)/(higcut - higbnd);
            fout[i+1] = frq;
          }
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

static int pvsbrej(CSOUND *csound, PVSBAND *p)
{
    int     i, N = p->fin->N;
    MYFLT   lowcut = *p->klowcut;
    MYFLT   lowbnd = *p->klowbnd;
    MYFLT   higbnd = *p->khigbnd;
    MYFLT   higcut = *p->khigcut;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;

    if (fout == NULL)
      return csound->PerfError(csound, Str("pvsband: not initialised"));

    if (lowcut<FL(0.0)) lowcut = FL(0.0);
    if (lowbnd<lowcut) lowbnd = lowcut;
    if (higbnd<lowbnd) higbnd = lowbnd;
    if (higcut<higbnd) higcut = higbnd;
#ifdef SDFT
    if (p->fin->sliding) {
      int n, nsmps = csound->ksmps;
      int NB  = p->fout->NB;

      for (n=0; n<nsmps; n++) {
        int change = 0;
        CMPLX *fin = (CMPLX *) p->fin->frame.auxp + n*NB;
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        if (XINARG2) lowcut = p->klowcut[n], change = 1;
        if (XINARG3) lowbnd = p->klowbnd[n], change = 1;
        if (XINARG4) higbnd = p->khigbnd[n], change = 1;
        if (XINARG5) higcut = p->khigcut[n], change = 1;
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
            fout[i].re = fin[i].re * (lowbnd - afrq)/(lowbnd - lowcut);
            fout[i].im = frq;
          }
          else {
            fout[i].re = fin[i].re * (afrq - higbnd)/(higcut - higbnd);
            fout[i].im = frq;
          }
        }
      }
      return OK;
    }
#endif
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
            fout[i] = fin[i] * (lowbnd - afrq)/(lowbnd - lowcut);
            fout[i+1] = frq;
          }
          else {
            fout[i] = fin[i] * (afrq - higbnd)/(higcut - higbnd);
            fout[i+1] = frq;
          }
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  {"pvsbandp", S(PVSBAND), 3, "f", "fxxxx", (SUBR) pvsbandinit, (SUBR) pvsband },
  {"pvsbandr", S(PVSBAND), 3, "f", "fxxxx", (SUBR) pvsbandinit, (SUBR) pvsbrej }
};

int pvsband_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}


