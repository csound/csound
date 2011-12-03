/*
    ugakbari.c:

    Copyright (C) 2006 by David Akbari

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
#include <math.h>

#define LOGCURVE(x,y) ((LOG(x * (y-FL(1.0))+FL(1.0)))/(LOG(y)))
#define EXPCURVE(x,y) ((EXP(x * LOG(y))-FL(1.0))/(y-FL(1.0)))
#define GAINSLIDER(x) (FL(0.000145) * EXP(x * FL(0.06907)))

typedef struct _scale {
  OPDS  h;
  MYFLT *koutval;
  MYFLT *kinval, *kmax, *kmin;
} scale;

typedef struct _expcurve {
  OPDS  h;
  MYFLT *kout;
  MYFLT *kin, *ksteepness;
} expcurve;

typedef struct _logcurve {
  OPDS  h;
  MYFLT *kout;
  MYFLT *kin, *ksteepness;
} logcurve;

typedef struct _gainslider {
  OPDS  h;
  MYFLT *koutsig;
  MYFLT *kindex;
} gainslider;

/*  scale opcode  */

static int scale_process(CSOUND *csound, scale *p)
{
    if (*p->kmin != *p->kmax) {
      *p->koutval = (*p->kinval * (*p->kmax - *p->kmin) + *p->kmin);
    }

    return OK;
}

/*  expcurve opcode  */

static int expcurve_perf(CSOUND *csound, expcurve *p)
{
    MYFLT ki = *p->kin;
    MYFLT ks = *p->ksteepness;
    *p->kout = EXPCURVE(ki, ks);

    return OK;
}

/*  logcurve opcode  */

static int logcurve_perf(CSOUND *csound, logcurve *p)
{
    MYFLT ki = *p->kin;
    MYFLT ks = *p->ksteepness;
    *p->kout = LOGCURVE(ki, ks);

    return OK;
}

/*  gainslider opcode  */

static int gainslider_perf(CSOUND *csound, gainslider *p)
{
    if (*p->kindex <= FL(0.0)) {
      *p->koutsig = FL(0.0);
    }
    else {
      *p->koutsig = GAINSLIDER(*p->kindex);
    }

    return OK;
}

/* opcode library entries */

static OENTRY ugakbari_localops[] = {
  { "scale", sizeof(scale), 2, "k", "kkk", NULL, (SUBR)scale_process, NULL },
  { "expcurve", sizeof(expcurve), 2, "k", "kk", NULL,
    (SUBR)expcurve_perf, NULL },
  { "logcurve", sizeof(logcurve), 2, "k", "kk", NULL,
    (SUBR)logcurve_perf, NULL },
  { "gainslider", sizeof(gainslider), 2, "k", "k", NULL,
    (SUBR)gainslider_perf, NULL }
};

LINKAGE1(ugakbari_localops)
