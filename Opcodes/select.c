/*
    select.c:

    Copyright (C) 2016 John ffitch

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

typedef struct Select {
  OPDS        h;
  MYFLT       *ar, *in1, *in2, *less, *equal, *more;
} Selecter;


static int selecter(CSOUND *csound, Selecter* p)
{
    MYFLT       *ar = p->ar;

    MYFLT       *a1=p->in1, *a2=p->in2, *al=p->less, *ae=p->equal, *am=p->more;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      ar[n] = a1[n]<a2[n] ? al[n] : a1[n]==a2[n] ? ae[n] : am[n];
    }
    return OK;
}




#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "select", S(Selecter), 0, 4, "a", "aaaaa",
                                   NULL, NULL, (SUBR)selecter}
};

LINKAGE
