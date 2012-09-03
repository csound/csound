/*
    pvsgendy.c:
    gendy style transformation in frequency domain

    (c) John ffitch, 2009

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
    MYFLT  *kmrate;
    MYFLT  *kfrate;
    int    lastframe;
} PVSGENDY;


static int pvsgendyinit(CSOUND *csound, PVSGENDY *p)
{
    int     N = p->fin->N;

    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));

    if (p->fin->sliding) {
      if (p->fout->frame.auxp==NULL ||
          csound->ksmps*(N+2)*sizeof(MYFLT) > (unsigned int)p->fout->frame.size)
        csound->AuxAlloc(csound, csound->ksmps*(N+2)*sizeof(MYFLT),&p->fout->frame);
      else memset(p->fout->frame.auxp, 0, csound->ksmps*(N+2)*sizeof(MYFLT));
    }
    else
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
    p->fout->sliding = p->fin->sliding;
    p->fout->NB = p->fin->NB;
    return OK;
}

static int pvsgendy(CSOUND *csound, PVSGENDY *p)
{
    int     i, N = p->fin->N;
    MYFLT   mrate = *p->kmrate;
    MYFLT   frate = *p->kfrate;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;

    if (UNLIKELY(fout == NULL)) goto err1;

    if (p->fin->sliding) {
      int n, nsmps = csound->ksmps;
      int NB  = p->fout->NB;

      for (n=0; n<nsmps; n++) {
        int change = 0;
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
        fout[i+1] = fin[i+1] + x;
        fout[i] = fin[i] + mrate * (MYFLT)(rand()-RAND_MAX/2)/(MYFLT)RAND_MAX;
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
 err1:
    return csound->PerfError(csound, Str("pvsgendy: not initialised"));
}

static OENTRY pvsgendy_localops[] = {
  { "pvsgendy", sizeof(PVSGENDY), 3, "f", "fkk",
                (SUBR) pvsgendyinit, (SUBR) pvsgendy, (SUBR) NULL }
};

LINKAGE1(pvsgendy_localops)



