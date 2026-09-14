/*
    tabsum.c:

    Copyright (C) 2009 John ffitch

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "interlocks.h"

typedef struct {
    OPDS    h;
    MYFLT   *kans;
    MYFLT   *itab;
    MYFLT   *kmin, *kmax;
  /* Local */
    FUNC    *ftp;
} TABSUM;


static int32_t tabsuminit(CSOUND *csound, TABSUM *p)
{
    if (UNLIKELY((p->ftp = csound->FTFind(csound, p->itab)) == NULL)) {
      return csound->InitError(csound, "%s", Str("tabsum: No table"));
    }
    return OK;
}



static int32_t tabsum(CSOUND *csound, TABSUM *p)
{
    int32_t i, min, max;
    double min_value = (double)*p->kmin;
    double max_value = (double)*p->kmax;
    MYFLT ans = FL(0.0);
    FUNC  *ftp = p->ftp;
    MYFLT *t;

    if (UNLIKELY(ftp==NULL))

      return csound->PerfError(csound, &(p->h),
                               "%s", Str("tabsum: Not initialised"));
    t = p->ftp->ftable;
    /* Allow rounding to index zero or the allocated guard point. */
    if (UNLIKELY(!(min_value > -1.0 && min_value < (double)ftp->flen + 1.0 &&
                   max_value > -1.0 && max_value < (double)ftp->flen + 1.0)))
      return csound->PerfError(csound, &(p->h), "%s",
                               Str("tabsum: range is outside table bounds"));
    min = MYFLT2LRND(min_value);
    max = MYFLT2LRND(max_value);
    if (UNLIKELY((uint32_t)min > ftp->flen || (uint32_t)max > ftp->flen))
      return csound->PerfError(csound, &(p->h), "%s",
                               Str("tabsum: range is outside table bounds"));
    if (UNLIKELY(min == 0 && max == 0)) max = ftp->flen-1;
    else if (UNLIKELY(min > max)) {
      int32_t k = min; min = max; max = k;
    }
    /* printf("tabsum: min, max = %d, %d\n", min, max); */
    for (i=min; i<=max; i++) ans += t[i];
    *p->kans = ans;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY tabsum_localops[] = {
{ "tabsum",     S(TABSUM),     0,      "k",    "iOO",
                (SUBR)tabsuminit, (SUBR)tabsum },
};

LINKAGE_BUILTIN(tabsum_localops)


