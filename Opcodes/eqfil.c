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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

// #include "csdl.h"
#include "csoundCore.h"
#ifdef PARCS
#include "interlocks.h"
#endif

typedef struct _equ {
  OPDS h;
  MYFLT *out;
  MYFLT *sig, *fr, *bw, *g, *ini;  /* in, freq, bw, gain, ini */
  double z1,z2;              /* delay memory */
  MYFLT frv, bwv;            /* bandwidth and frequency */
  double c,d;                /* filter vars */

} equ;

static int equ_init(CSOUND *csound, equ *p)
{
    if (*p->ini==0) {
      double sr = (double)csound->GetSr(csound);
      p->z1 = p->z2 = 0.0;
      p->frv = *p->fr; p->bwv = *p->bw;
      p->d = cos(2*PI*p->frv/sr);
      p->c = tan(PI*p->bwv/sr);
    }

    return OK;
}

static int equ_process(CSOUND *csound, equ *p)
{
    double z1 = p->z1, z2 = p->z2,c,d,w,a,y;
    MYFLT  *in= p->sig,*out=p->out,g;
    int i, ksmps = csound->GetKsmps(csound);

    if (*p->bw != p->bwv || *p->fr != p->frv){
      double sr = (double)csound->GetSr(csound);
      p->frv = *p->fr; p->bwv = *p->bw;
      p->d = cos(2*PI*p->frv/sr);
      p->c = tan(PI*p->bwv/sr);
    }
    c = p->c;
    d = p->d;
    a = (1.0-c)/(1.0+c);
    g = *p->g;

    for(i=0; i < ksmps; i++){
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
  {"eqfil", sizeof(equ), 5,
   "a", "akkko", (SUBR)equ_init, NULL, (SUBR)equ_process},
};

LINKAGE1(eqfil_localops)
