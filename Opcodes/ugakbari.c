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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/* scale modified and scale2 written as a replacement by JPff Dec 2020 */


#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include <math.h>

#define LOGCURVE(x,y) (log1p((cs_double)(x) * ((cs_double)(y)-1.0)) / log((cs_double)(y)))
#define EXPCURVE(x,y) (expm1((cs_double)(x) * log((cs_double)(y))) / ((cs_double)(y)-1.0))
#define GAINSLIDER(x) (FL(0.000145) * EXP(x * FL(0.06907)))

typedef struct _scale {
  OPDS  h;
  cs_float *koutval;
  cs_float *kinval, *kmax, *kmin, *imax, *imin;
} scale;

typedef struct _scale2 {
  OPDS  h;
  cs_float *koutval;
  cs_float *kinval, *kmin, *kmax, *imin, *imax, *ihtim;
  cs_float c1, c2, yt1;
} SCALE2;

typedef struct _expcurve {
  OPDS  h;
  cs_float *kout;
  cs_float *kin, *ksteepness;
} expcurve;

typedef struct _logcurve {
  OPDS  h;
  cs_float *kout;
  cs_float *kin, *ksteepness;
} logcurve;

typedef struct _gainslider {
  OPDS  h;
  cs_float *koutsig;
  cs_float *kindex;
} gainslider;

/*  scale opcode  */

static int32_t scale_process(CSOUND *csound, scale *p)
{
    IGN(csound);
    cs_float max = *p->imax;
    cs_float min = *p->imin;
    cs_float kmax = *p->kmax;
    cs_float kmin = *p->kmin;
    cs_float val = *p->kinval;
    /* if (max < min) { max = min ; min = *p->imax; } */
    /* if (kmax < kmin) { kmax = kmin ; kmin = *p->kmax; } */
    /* if (val > max) val = max; */
    /* else if (val < min) val = min; */

    //if (max>min && /* fmax>=fmin && */ *p->kinval<= max && *p->kinval >= min) {
      *p->koutval = ((val - min)/(max-min))* (kmax - kmin) + kmin;
    /* } else { */
    /*   //printf("** input *%g,%ginput (%g,%g) output (%g,%g) val %g\n", */
    /*   //       min, max, kmin, kmax, *p->kinval); */
    /*   //printf("%d %d %d %d\n", */
    /*   //       max>min, fmax>fmin, *p->kinval<= max, *p->kinval >= min); */
    /*   return csound->InitError(csound, Str("Invalid range in scale")); */
    /* } */
    return OK;
}

static int32_t scale2_init(CSOUND *csound, SCALE2 *p)
{
    if (*p->ihtim < FL(0.0))
      return csound->InitError(csound, "%s",
                               Str("scale2: smoothing time must be nonnegative"));
    p->yt1 = FL(0.0);
    if (*p->ihtim != FL(0.0)) {
      p->c2 = POWER(FL(0.5), CS_ONEDKR / *p->ihtim);
      p->c1 = FL(1.0) - p->c2;
    } else {
      p->c2 = FL(0.0); p->c1 = FL(1.0);
    }
    return OK;
}

static int32_t scale2_process(CSOUND *csound, SCALE2 *p)
{
    cs_float max = *p->imax;
    cs_float min = *p->imin;
    cs_float kmax = *p->kmax;
    cs_float kmin = *p->kmin;
    cs_float val = *p->kinval;
    if (UNLIKELY(!(max > min)))
      return csound->PerfError(csound, &p->h, "%s",
                               Str("scale2: input maximum must exceed minimum"));
    if (val > max) val = max;
    else if (val < min) val = min;

    val = ((val - min)/(max-min))* (kmax - kmin) + kmin;
    if (p->c2 == FL(0.0))
      p->yt1 = val;
    else
      p->yt1 = p->c1 * val + p->c2 * p->yt1;
    *p->koutval = p->yt1;
    return OK;
}

/*  expcurve opcode  */

static int32_t expcurve_perf(CSOUND *csound, expcurve *p)
{
    IGN(csound);
    cs_float ki = *p->kin;
    cs_float ks = *p->ksteepness;
    if (ks <= FL(1.0) || ki == FL(0.0) || ki == FL(1.0))
      *p->kout = ki;
    else
      *p->kout = EXPCURVE(ki, ks);

    return OK;
}

/*  logcurve opcode  */

static int32_t logcurve_perf(CSOUND *csound, logcurve *p)
{
    IGN(csound);
    cs_float ki = *p->kin;
    cs_float ks = *p->ksteepness;
    if (ks <= FL(1.0) || ki == FL(0.0) || ki == FL(1.0))
      *p->kout = ki;
    else
      *p->kout = LOGCURVE(ki, ks);

    return OK;
}

/*  gainslider opcode  */

static int32_t
gainslider_perf(CSOUND *csound, gainslider *p)
{
    IGN(csound);
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
  { "scale", sizeof(scale), 0,  "k", "kkkPO", NULL, (SUBR)scale_process, NULL },
  { "scale2", sizeof(SCALE2), 0,  "k", "kkkOPo", (SUBR)scale2_init, (SUBR)scale2_process, NULL },
  { "expcurve", sizeof(expcurve), 0,  "k", "kk", NULL,
    (SUBR)expcurve_perf, NULL },
  { "logcurve", sizeof(logcurve), 0,  "k", "kk", NULL,
    (SUBR)logcurve_perf, NULL },
  { "gainslider", sizeof(gainslider), 0,  "k", "k", NULL,
    (SUBR)gainslider_perf, NULL }
};

LINKAGE_BUILTIN(ugakbari_localops)
