/*
    pan2.c:

    Copyright (C) 2007 John ffitch

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
#include "interlocks.h"

#include <math.h>

typedef struct {
    OPDS h;
    MYFLT *aleft;                /* Left output  */
    MYFLT *aright;               /* Right output   */
    MYFLT *asig;
    MYFLT *pan;                  /* pan position */
    MYFLT *itype;                /* type of panning */
    int   type;
} PAN2;
#define SQRT2 FL(1.41421356237309504880)

static int pan2set(CSOUND *csound, PAN2 *p)
{
    int type = p->type = MYFLT2LRND(*p->itype);
    if (UNLIKELY(type <0 || type > 3))
      return csound->InitError(csound, Str("Unknown panning type"));
    return OK;
}

static int pan2run(CSOUND *csound, PAN2 *p)
{
    int type = p->type;
    MYFLT *ain = p->asig;
    MYFLT *al = p->aleft, *ar = p->aright;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) {
      memset(ar, '\0', offset*sizeof(MYFLT));
      memset(al, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
      memset(&al[nsmps], '\0', early*sizeof(MYFLT));
    }
    switch (type) {
    case 0:
      {
        MYFLT kangl = PI_F*FL(0.5) * *p->pan;
        for (n=offset; n<nsmps; n++) {
          if (XINARG2) kangl = PI_F*FL(0.5) * p->pan[n];
          ar[n] = ain[n] * SIN(kangl);
        al[n] = ain[n] * COS(kangl);
        }
        break;
      }
    case 1:
      {
        MYFLT kangl = *p->pan;
        for (n=offset; n<nsmps; n++) {
          if (XINARG2) kangl = p->pan[n];
          ar[n] = ain[n] * SQRT(kangl);
          al[n] = ain[n] * SQRT(FL(1.0)-kangl);
        }
        break;
      }
    case 2:
      {
        MYFLT kangl = *p->pan;
        for (n=offset; n<nsmps; n++) {
          if (XINARG2) kangl = p->pan[n];
          ar[n] = ain[n] * kangl;
          al[n] = ain[n] * (FL(1.0)-kangl);
        }
        break;
      }
    case 3:
      {
        MYFLT kangl = *p->pan, cc, ss, l, r;
        for (n=offset; n<nsmps; n++) {
          if (XINARG2) kangl = p->pan[n];
          cc = COS(PI*kangl*FL(0.5));
          ss = SIN(PI*kangl*FL(0.5));
          l = SQRT2*(cc+ss)*0.5;
          r = SQRT2*(cc-ss)*0.5;
          al[n] = ain[n] * l;
          ar[n] = ain[n] * r;
        }
      }
    }
    return OK;
}


static OENTRY pan2_localops[] =
{
  { "pan2", sizeof(PAN2), 0, 5, "aa", "axo", (SUBR) pan2set, 0, (SUBR) pan2run },
};

LINKAGE_BUILTIN(pan2_localops)
