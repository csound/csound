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
    MYFLT  *klowest;
    MYFLT  *khighest;
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
            p->fout->frame.size < sizeof(float) * (N + 2))  /* RWD MUST be 32bit */
          csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
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
    int     i, N = p->fout->N;
    MYFLT   lowest = *p->klowest;
    MYFLT   highest = *p->khighest;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;

    if (fout == NULL)
      return csound->PerfError(csound, Str("pvsband: not initialised"));

    if (lowest<FL(0.0)) lowest = FL(0.0);
    if (highest>FL(0.5)*csound->esr) highest = FL(0.5)*csound->esr;
#ifdef SDFT
    if (p->fin->sliding) {
      int n, nsmps = csound->ksmps;
      int NB  = p->fout->NB;

      for (n=0; n<nsmps; n++) {
        CMPLX *fin = (CMPLX *) p->fin->frame.auxp + n*NB;
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        if (XINARG2) {
          lowest = p->klowest[n];
          if (lowest<FL(0.0)) lowest = FL(0.0);
        }
        if (XINARG3) {
          highest = p->khighest[n];
          if (highest>FL(0.5)*csound->esr)
            highest = FL(0.5)*csound->esr;
        }
        for (i = 0; i < NB-1; i++) {
          if (fout[i].im < lowest || fout[i].im>highest) {
            fout[i].re = FL(0.0);
            fout[i].im = -FL(1.0);
          }
          else {
            fout[i] = fin[i];
          }
        }
      }
      return OK;
    }
#endif
    if (p->lastframe < p->fin->framecount) {

      for (i = 0; i < N; i += 2) {
        if (fout[i + 1] < lowest || fout[i + 1] > highest) {
          fout[i] = 0.0f;
          fout[i + 1] = -1.0f;
        }
        else {
          fout[i] = fin[i];
          fout[i + 1] = fin[i + 1];
        }
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  {"pvsband", S(PVSBAND), 3, "f", "fxx", (SUBR) pvsbandinit, (SUBR) pvsband }
};

int pvsband_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}


