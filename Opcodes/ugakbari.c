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
  cs_double c1, c2, yt1, correction;
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

/* A convex combination must not overflow when the endpoints are finite. */
#define SCALE2_LERP(result, low, high, t) do {                              \
    const cs_double scale2_low = (low);                                   \
    const cs_double scale2_high = (high);                                 \
    const cs_double scale2_t = (t);                                       \
    if (scale2_t <= FL(0.0))                                              \
      (result) = scale2_low;                                              \
    else if (scale2_t >= FL(1.0))                                         \
      (result) = scale2_high;                                             \
    else {                                                               \
      /* Step from the nearer endpoint, retaining small displacements. */ \
      const cs_double scale2_start =                                     \
        scale2_t <= FL(0.5) ? scale2_low : scale2_high;                    \
      const cs_double scale2_end =                                       \
        scale2_t <= FL(0.5) ? scale2_high : scale2_low;                    \
      const cs_double scale2_fraction =                                  \
        scale2_t <= FL(0.5) ? scale2_t : FL(1.0) - scale2_t;               \
      cs_double scale2_difference = scale2_end - scale2_start;            \
      volatile cs_double scale2_step;                                    \
      if (isfinite(scale2_difference))                                   \
        scale2_step = scale2_fraction * scale2_difference;                \
      else {                                                             \
        volatile cs_double scale2_halfend = scale2_end * FL(0.5);         \
        volatile cs_double scale2_halfstart = scale2_start * FL(0.5);     \
        volatile cs_double scale2_halfdiff =                             \
          scale2_halfend - scale2_halfstart;                             \
        volatile cs_double scale2_halfstep =                             \
          scale2_fraction * scale2_halfdiff;                             \
        /* The fraction is at most 0.5, so restoring the scale is safe. */\
        scale2_step = scale2_halfstep * FL(2.0);                          \
      }                                                                  \
      (result) = scale2_start + scale2_step;                              \
    }                                                                    \
  } while (0)

static int32_t scale2_init(CSOUND *csound, SCALE2 *p)
{
    if (*p->ihtim < FL(0.0))
      return csound->InitError(csound, "%s\n",
                               Str("scale2: smoothing time must be nonnegative"));
    p->yt1 = p->correction = 0.0;
    /* Compute both coefficients directly, retaining small updates and poles. */
    if (*p->ihtim == FL(0.0)) {
      p->c1 = FL(1.0);
      p->c2 = FL(0.0);
    }
    else {
#ifdef USE_FLOAT
      cs_double exponent = -logf(FL(2.0))*CS_ONEDKR/(cs_double)*p->ihtim;
      p->c1 = -expm1f(exponent);
      p->c2 = expf(exponent);
#else
      cs_double exponent = -log(2.0)*CS_ONEDKR/(cs_double)*p->ihtim;
      p->c1 = -expm1(exponent);
      p->c2 = exp(exponent);
#endif
    }
    return OK;
}

static int32_t scale2_process(CSOUND *csound, SCALE2 *p)
{
    cs_double max = *p->imax;
    cs_double min = *p->imin;
    cs_double kmax = *p->kmax;
    cs_double kmin = *p->kmin;
    cs_double val = *p->kinval;
    cs_double span, position;
    if (UNLIKELY(!(max > min)))
      return csound->PerfError(csound, &p->h, "%s\n",
                               Str("scale2: input maximum must exceed minimum"));
    if (val > max) val = max;
    else if (val < min) val = min;

    span = max - min;
    /* Halving all terms preserves the ratio when the full span overflows. */
    if (isfinite(span))
      position = (val - min) / span;
    else {
      volatile cs_double halfval = val * FL(0.5);
      volatile cs_double halfmin = min * FL(0.5);
      volatile cs_double halfmax = max * FL(0.5);
      position = (halfval - halfmin) / (halfmax - halfmin);
    }
    SCALE2_LERP(val, kmin, kmax, position);
    if (p->c2 == FL(0.0)) {
      p->yt1 = val;
      p->correction = 0.0;
    }
    else if (p->c1 > FL(0.5)) {
      /* Interpolate from the target so a small pole survives c1 rounding. */
      SCALE2_LERP(p->yt1, val, p->yt1, p->c2);
      p->correction = FL(0.0);
    }
    else {
      cs_double difference = val - p->yt1;
      volatile cs_double update;
      if (UNLIKELY(!isfinite(difference))) {
        /* c1 <= 0.5: halve before subtracting, then multiply before
           restoring the scale. The full increment now fits. */
        volatile cs_double halfval = val * FL(0.5);
        volatile cs_double halfstate = p->yt1 * FL(0.5);
        volatile cs_double halfdiff = halfval - halfstate;
        volatile cs_double halfupdate =
          p->c1 * (halfdiff + p->correction * FL(0.5));
        update = halfupdate * FL(2.0);
      }
      else
        update = p->c1 * (difference + p->correction);
      /* Keep updates smaller than one ULP in a second cs_double. */
      /* Force the rounding steps even in a fast-math build. */
      {
        volatile cs_double step = update - p->correction;
        volatile cs_double next = p->yt1 + step;
        volatile cs_double applied = next - p->yt1;
        p->correction = applied - step;
        p->yt1 = next;
      }
    }
    *p->koutval = (cs_float)p->yt1;
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
