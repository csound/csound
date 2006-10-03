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

#include "csdl.h"
#include <math.h>

#define LOGCURVE(x,y) ((log(x * (y-1)+1))/(log(y)))
#define EXPCURVE(x,y) (exp(x * log(y))-1/(y-1))
#define GAINSLIDER(x) (0.000145 * exp(x * 0.06907))

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

int scale_init(CSOUND *csound, scale *p)
{
    /* This does little as k values not available yet */
    *p->koutval = (*p->kinval * (*p->kmax - *p->kmin) + *p->kmin);

    return OK;
}

int scale_process(CSOUND *csound, scale *p)
{

    if (*p->kmin != *p->kmax)
      {
        *p->koutval = (*p->kinval * (*p->kmax - *p->kmin) + *p->kmin);
      }

    return OK;
}

/*  expcurve opcode  */

int expcurve_init(CSOUND *csound, expcurve *p)
{
    *p->kout = EXPCURVE((MYFLT) *p->kin, (MYFLT) *p->ksteepness);

    return OK;
}

int expcurve_perf(CSOUND *csound, expcurve *p)
{
    *p->kout = EXPCURVE((MYFLT) *p->kin, (MYFLT) *p->ksteepness);

    return OK;
}

/*  logcurve opcode  */

int logcurve_init(CSOUND *csound, logcurve *p)
{
    *p->kout = LOGCURVE((MYFLT) *p->kin, (MYFLT) *p->ksteepness);

    return OK;
}

int logcurve_perf(CSOUND *csound, logcurve *p)
{
    *p->kout = LOGCURVE((MYFLT) *p->kin, (MYFLT) *p->ksteepness);

    return OK;
}

/*  gainslider opcode  */

int gainslider_init(CSOUND *csound, gainslider *p)
{
    *p->koutsig = GAINSLIDER((MYFLT) *p->kindex);
 
    return OK;
}

int gainslider_perf(CSOUND *csound, gainslider *p)
{

    if (*p->kindex >= FL(0.0) && *p->kindex <= FL(152.0)) {
      *p->koutsig = GAINSLIDER((MYFLT) *p->kindex);
  }

  return OK;
}

/* opcode library entries */

static OENTRY localops[] = {
  { "scale", sizeof(scale), 3, "k", "kkk", (SUBR)scale_init, (SUBR)scale_process, NULL },
  { "expcurve", sizeof(expcurve), 3, "k", "kk", (SUBR)expcurve_init, (SUBR)expcurve_perf, NULL },
  { "logcurve", sizeof(logcurve), 3, "k", "kk", (SUBR)logcurve_init, (SUBR)logcurve_perf, NULL },
  { "gainslider", sizeof(gainslider), 3, "k", "k", (SUBR)gainslider_init, (SUBR)gainslider_perf, NULL }
};

LINKAGE
