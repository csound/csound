/*

    gammatone.c:

    Copyright (C) 2019 John ffitch

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

typedef struct {
        OPDS    h;
        MYFLT   *ans;
        MYFLT   *x;
        MYFLT   *freq;
        MYFLT   *decay;
        MYFLT   *order;
        MYFLT   *phase;
        int32_t n;
        MYFLT   expmbt;
        MYFLT   cosft;
        MYFLT   sinft;
        MYFLT   oldf;
        MYFLT   yr[10];
        MYFLT   yi[10];
        AUXCH   aux;
        MYFLT   *xxr, *xxi;
} GAMMA;

static int32_t gammatone_init(CSOUND *csound, GAMMA *p)
{
    p->n = MYFLT2LRND(*p->order);
    if (p->n<0 || p->n>10)
      return csound->InitError(csound, Str("Invalid order %d\n"), p->n);
    else if (p->n==0) p->n = 4;
    p->expmbt = EXP(-2.0*PI_F* *p->decay/*/CS_ESR*/);
    p->cosft = FL(1.0);
    p->sinft = FL(0.0);
    p->oldf = FL(0.0);
    memset(p->yr, '\0', 10*sizeof(MYFLT));
    memset(p->yi, '\0', 10*sizeof(MYFLT));
    csound->AuxAlloc(csound, 2*CS_KSMPS*sizeof(MYFLT), &p->aux);
    p->xxr = (MYFLT*)p->aux.auxp;
    p->xxi  = & p->xxr[CS_KSMPS];
    return OK;
}

static int32_t gammatone_perf(CSOUND *csound, GAMMA *p)
{
    MYFLT freq = p->oldf;
    MYFLT cc, ss, yrm1, yim1;
    MYFLT *xxr = p->xxr;
    MYFLT *xxi = p->xxr;
    int32_t nsmps = CS_KSMPS, i, k;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    if (UNLIKELY(offset)) memset(p->ans, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&(p->ans[nsmps]), '\0', early*sizeof(MYFLT));
    }
    if (*p->freq != freq) {
      freq = p->oldf = *p->freq;
      p->cosft = COS(2.0*PI_F*freq/CS_ESR);
      p->sinft = SIN(2.0*PI_F*freq/CS_ESR);
      //printf("**** cos/sin = %f / %f\n", p->cosft, p->sinft);
      //printf("**** expmbt = %f\n", p->expmbt);
    }
    cc = p->cosft;
    ss = p->sinft;
    yrm1 = p->yr[0];     yim1 = p->yi[0];
    for (i = offset; i<nsmps; i++) {
      MYFLT real = p->expmbt*(cc*yrm1 - ss*yim1) + (1.0-p->expmbt)*(p->x[i]);
      MYFLT imag = p->expmbt*(ss*yrm1 + cc*yim1);
      //printf("*** y[] = %f + i*%f x[%d] = %f\n", real, imag, i, p->x[i]);
      //printf("*** xpart = %f\n", (1.0-p->expmbt)*p->x[i]);
      yrm1 = real; yim1 = imag;
      xxr[i] = real; xxi[i]= imag;
    }
    p->yr[0] = yrm1;  p->yi[0] = yim1;
    for (k =1; k<p->n; k++) {
      yrm1 = p->yr[k];     yim1 = p->yi[k];
      //printf("--- iteration %d\n", k);
      for (i = offset; i<nsmps; i++) {
        MYFLT real = p->expmbt*(cc*yrm1 - ss*yim1 - xxr[i]) + xxr[i];
        MYFLT imag = p->expmbt*(ss*yrm1 + cc*yim1 - xxi[i]) + xxi[i];
        //printf("*** y[%d] = %f + i*%f xx[] = %f + i*%f\n",
        //       i, real, imag, xxr[i], xxi[i]);
        yrm1 = real;  yim1 = imag;
        xxr[i] = real; xxi[i] = imag;
      }
      p->yr[k] = yrm1;  p->yi[k] = yim1;
    }
    if (*p->phase == FL(0.0))
      for (i = offset; i<nsmps; i++)
        p->ans[i] =FL(2.0)*xxr[i];
    else {
      MYFLT cc = COS(*p->phase), ss = SIN(*p->phase);
      for (i=offset; i<nsmps; i++)
        p->ans[i] = FL(2.0)*(xxr[i]*cc - xxi[i]*ss);
    }
    return OK;
}


#define S(x) sizeof(x)

static OENTRY gamma_localops[] = {
{ "gtf", S(GAMMA), 0, "a", "akioo", (SUBR)gammatone_init, (SUBR)gammatone_perf }
};

LINKAGE_BUILTIN(gamma_localops)
