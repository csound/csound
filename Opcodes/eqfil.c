/*
    eqfil.c:

    Copyright (C) 2007 Victor Lazzarini

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

// #include "csdl.h"
#include "csoundCore.h"
#include "interlocks.h"

typedef struct _equ {
  OPDS h;
  MYFLT *out;
  MYFLT *sig, *fr, *bw, *g, *ini;  /* in, freq, bw, gain, ini */
  double z1,z2;              /* delay memory */
  MYFLT frv, bwv;            /* bandwidth and frequency */
  double c,d;                /* filter vars */

} equ;

static int32_t equ_init(CSOUND *csound, equ *p)
{
    if (*p->ini==0) {
      double sr = (double)CS_ESR;
      p->z1 = p->z2 = 0.0;
      p->frv = *p->fr; p->bwv = *p->bw;
      p->d = cos(2*PI*p->frv/sr);
      p->c = tan(PI*p->bwv/sr);
    }

    return OK;
}

static int32_t equ_process(CSOUND *csound, equ *p)
{
    double z1 = p->z1, z2 = p->z2,c,d,w,a,y;
    MYFLT  *in= p->sig,*out=p->out,g;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t
      i, ksmps = CS_KSMPS;

    if (*p->bw != p->bwv || *p->fr != p->frv){
      double sr = (double)CS_ESR;
      p->frv = *p->fr; p->bwv = *p->bw;
      p->d = cos(2*PI*p->frv/sr);
      p->c = tan(PI*p->bwv/sr);
    }
    c = p->c;
    d = p->d;
    a = (1.0-c)/(1.0+c);
    g = *p->g;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      ksmps -= early;
      memset(&out[ksmps], '\0', early*sizeof(MYFLT));
    }
    for (i=0; i < ksmps; i++){
      w = (double)(in[i]) + d*(1.0 + a)*z1 - a*z2;
      y = w*a - d*(1.0 + a)*z1 + z2;
      z2 = z1;
      z1 = w;
      out[i] = (MYFLT) (0.5*(y + in[i] + g*(in[i] - y)));

    }
    p->z1 = z1;
    p->z2 = z2;

    return OK;
}

static OENTRY eqfil_localops[] = {
  {"eqfil", sizeof(equ), 0, 3,
   "a", "akkko", (SUBR)equ_init, (SUBR)equ_process},
};

LINKAGE_BUILTIN(eqfil_localops)
