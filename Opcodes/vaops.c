/*
    vaops.c:

    Copyright (C) 2006 Steven Yi

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

#include "csoundCore.h"
#include "interlocks.h"

#define MYFLOOR(x) (x >= FL(0.0) ? (int32)x : (int32)((double)x - 0.99999999))

typedef struct {
        OPDS    h;
        MYFLT   *kout, *kindx, *avar;
} VA_GET;

typedef struct {
        OPDS    h;
        MYFLT   *kval, *kindx, *avar;
} VA_SET;

static int vaget(CSOUND *csound, VA_GET *p)
{
    int32 ndx = (int32) MYFLOOR((double)*p->kindx);
    if (UNLIKELY(ndx<0 || ndx>=csound->ksmps))
      return csound->PerfError(csound, Str("Out of range in vaget (%d)"), ndx);
    *p->kout = *(p->avar + ndx);
    return OK;
}

static int vaset(CSOUND *csound, VA_SET *p)
{
    int32 ndx = (int32) MYFLOOR((double)*p->kindx);
    if (UNLIKELY(ndx<0 || ndx>=csound->ksmps))
      return csound->PerfError(csound, Str("Out of range in vaset (%d)"), ndx);
    *(p->avar + ndx) = *p->kval;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY vaops_localops[] = {
  { "vaget", S(VA_GET),    2,      "k", "ka",  NULL, (SUBR)vaget },
  { "vaset", S(VA_SET),    2,      "",  "kka", NULL, (SUBR)vaset }
};


LINKAGE1(vaops_localops)

