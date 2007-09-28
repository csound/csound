/*
    pan.h:

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

#include "csdl.h"

typedef struct {
    OPDS h;

    MYFLT *aleft;                /* Left output  */
    MYFLT *aright;               /* Right output   */
    MYFLT *asig;
    MYFLT *pan;                  /* pan position */
    MYFLT *itype;                /* type of panning */
    int   type;
} PAN2;

#include <math.h>

static int pan2init(CSOUND *csound, PAN2 *p)
{
    int type = p->type = MYFLT2LRND(*p->itype);
    if (type <0 || type > 2) csound->InitError(csound, "Unknown panning type");
    return OK;
}

static int pan2(CSOUND *csound, PAN2 *p)
{
    int type = p->type;
    MYFLT *ain = p->asig;
    MYFLT *al = p->aleft, *ar = p->aright;
    int n, nsmps=csound->ksmps;
    switch (type) {
    case 0: {
      double kangl = PI*0.5 * (*p->pan + 0.5);
      for (n=0; n<nsmps; n++) {
        if (XINARG2) kangl = PI*0.5 * (p->pan[n] + 0.5);
        al[n] = ain[n] * (MYFLT)sin(kangl);
        ar[n] = ain[n] * (MYFLT)cos(kangl);
      }
      break;
    }
    case 1: {
      double kangl = *p->pan;
      for (n=0; n<nsmps; n++) {
        if (XINARG2) kangl = p->pan[n];
        al[n] = ain[n] * (MYFLT)sqrt(kangl);
        ar[n] = ain[n] * (MYFLT)sqrt(1-kangl);
      }
      break;
    }
    case 2: {
      MYFLT kangl = *p->pan;
      for (n=0; n<nsmps; n++) {
        if (XINARG2) kangl = p->pan[n];
        al[n] = ain[n] * kangl;
        ar[n] = ain[n] * (1-kangl);
      }
      break;
    }
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "pan2",     S(PAN2),     5,     "aa",    "axo",(SUBR)pan2init, NULL, (SUBR)pan2 },
};

LINKAGE



