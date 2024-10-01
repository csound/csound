/*
    minmax.c

    A Csound opcode library implementing ugens
    for finding minimum and maximum values.

    Anthony Kozar
    April 5, 2005

    Jan 26, 2006:  Added non-accumulating and abs value variations.
    Mar 06, 2006:  Converted to Csound 5 API.

    Copyright (C) 2005-6  Anthony M. Kozar Jr.

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

// #include "csdl.h"
#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include <math.h>

typedef struct {
    OPDS    h;
    MYFLT   *accum, *ain;
} MINMAXACCUM;

typedef struct {
    OPDS    h;
    MYFLT   *xout, *xin1, *xin2toN[VARGMAX-1];
} MINMAX;

/* Which implementation is faster ?? */
static int32_t MaxAccumulate(CSOUND *csound, MINMAXACCUM *p)
{
    IGN(csound);
    MYFLT   cur;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *out = p->accum;
    MYFLT   *in = p->ain;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      cur = in[n];
      if (UNLIKELY(cur > out[n]))
        out[n] = cur;
    }

    return OK;
}

static int32_t MinAccumulate(CSOUND *csound, MINMAXACCUM *p)
{
    IGN(csound);
    MYFLT   cur;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *out = p->accum;
    MYFLT   *in = p->ain;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      cur = in[n];
      if (UNLIKELY(cur < out[n]))
        out[n] = cur;
    }

    return OK;
}

/* Absolute value versions of the above */
static int32_t MaxAbsAccumulate(CSOUND *csound, MINMAXACCUM *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *out = p->accum;
    MYFLT   *in = p->ain;
    MYFLT   inabs;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      inabs = FABS(in[n]);
      if (UNLIKELY(inabs > out[n]))
        out[n] = inabs;
    }

    return OK;
}

static int32_t MinAbsAccumulate(CSOUND *csound, MINMAXACCUM *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *out = p->accum;
    MYFLT   *in = p->ain;
    MYFLT   inabs;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      inabs = FABS(in[n]);
      if (UNLIKELY(inabs < out[n]))
        out[n] = inabs;
    }

    return OK;
}

/* Multiple input min and max opcodes */
static int32_t Max_arate(CSOUND *csound, MINMAX *p)
{
    IGN(csound);
    int32_t     i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     nargs = ((int32_t) p->INOCOUNT) - 1;
    MYFLT   *out = p->xout;
    MYFLT   *in1 = p->xin1;
    MYFLT   **in2 = p->xin2toN;
    MYFLT   max, temp;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      max = in1[n];
      for (i = 0; i < nargs; ++i) {
        temp = in2[i][n];
        if (UNLIKELY(temp > max))
          max = temp;
      }
      out[n] = max;
    }

    return OK;
}

static int32_t Min_arate(CSOUND *csound, MINMAX *p)
{
    IGN(csound);
    int32_t     i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     nargs = ((int32_t) p->INOCOUNT) - 1;
    MYFLT   *out = p->xout;
    MYFLT   *in1 = p->xin1;
    MYFLT   **in2 = p->xin2toN;
    MYFLT   min, temp;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      min = in1[n];
      for (i = 0; i < nargs; ++i) {
        temp = in2[i][n];
        if (UNLIKELY(temp < min))
          min = temp;
      }
      out[n] = min;
    }

    return OK;
}

/* Absolute value versions of multiple input opcodes */
static int32_t MaxAbs_arate(CSOUND *csound, MINMAX *p)
{
    IGN(csound);
    int32_t     i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     nargs = ((int32_t) p->INOCOUNT) - 1;
    MYFLT   *out = p->xout;
    MYFLT   *in1 = p->xin1;
    MYFLT   **in2 = p->xin2toN;
    MYFLT   max, temp;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      max = FABS(in1[n]);
      for (i = 0; i < nargs; ++i) {
        temp = FABS(in2[i][n]);
        if (UNLIKELY(temp > max))
          max = temp;
      }
      out[n] = max;
    }

    return OK;
}

static int32_t MinAbs_arate(CSOUND *csound, MINMAX *p)
{
    IGN(csound);
    int32_t     i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     nargs = ((int32_t) p->INOCOUNT) - 1;
    MYFLT   *out = p->xout;
    MYFLT   *in1 = p->xin1;
    MYFLT   **in2 = p->xin2toN;
    MYFLT   min, temp;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      min = FABS(in1[n]);
      for (i = 0; i < nargs; ++i) {
        temp = FABS(in2[i][n]);
        if (UNLIKELY(temp < min))
          min = temp;
      }
      out[n] = min;
    }

    return OK;
}

/* k-rate multiple input min and max opcodes */
static int32_t Max_krate(CSOUND *csound, MINMAX *p)
{
    IGN(csound);
    int32_t     i;
    int32_t     nargs = ((int32_t) p->INOCOUNT) - 1;
    MYFLT   *out = p->xout;
    MYFLT   *in1 = p->xin1;
    MYFLT   **in2 = p->xin2toN;
    MYFLT   max, temp;

    max = *in1;
    for (i = 0; i < nargs; ++i) {
      temp = in2[i][0];
      if (UNLIKELY(temp > max))
        max = temp;
    }
    *out = max;

    return OK;
}

static int32_t Min_krate(CSOUND *csound, MINMAX *p)
{
    IGN(csound);
    int32_t     i;
    int32_t     nargs = ((int32_t) p->INOCOUNT) - 1;
    MYFLT   *out = p->xout;
    MYFLT   *in1 = p->xin1;
    MYFLT   **in2 = p->xin2toN;
    MYFLT   min, temp;

    min = *in1;
    for (i = 0; i < nargs; ++i) {
      temp = in2[i][0];
      if (UNLIKELY(temp < min))
        min = temp;
    }
    *out = min;

    return OK;
}

/* Absolute value versions of k-rate opcodes */
static int32_t MaxAbs_krate(CSOUND *csound, MINMAX *p)
{
    IGN(csound);
    int32_t     i;
    int32_t     nargs = ((int32_t) p->INOCOUNT) - 1;
    MYFLT   *out = p->xout;
    MYFLT   *in1 = p->xin1;
    MYFLT   **in2 = p->xin2toN;
    MYFLT   max, temp;

    max = FABS(*in1);
    for (i = 0; i < nargs; ++i) {
      temp = FABS(in2[i][0]);
      if (UNLIKELY(temp > max))
        max = temp;
    }
    *out = max;

    return OK;
}

static int32_t MinAbs_krate(CSOUND *csound, MINMAX *p)
{
    IGN(csound);
    int32_t     i;
    int32_t     nargs = ((int32_t) p->INOCOUNT) - 1;
    MYFLT   *out = p->xout;
    MYFLT   *in1 = p->xin1;
    MYFLT   **in2 = p->xin2toN;
    MYFLT   min, temp;

    min = FABS(*in1);
    for (i = 0; i < nargs; ++i) {
      temp = FABS(in2[i][0]);
      if (UNLIKELY(temp < min))
        min = temp;
    }
    *out = min;

    return OK;
}

/* code for linking dynamic libraries under Csound 5 */

#define S(x)    sizeof(x)

static OENTRY minmax_localops[] = {
    {"maxaccum", S(MINMAXACCUM), WI, "", "aa", NULL, (SUBR) MaxAccumulate},
    {"minaccum", S(MINMAXACCUM), WI, "", "aa", NULL, (SUBR) MinAccumulate},
    {"maxabsaccum", S(MINMAXACCUM), WI, "", "aa", NULL,
     (SUBR) MaxAbsAccumulate},
    {"minabsaccum", S(MINMAXACCUM), WI, "", "aa", NULL,
     (SUBR) MinAbsAccumulate},
    {"max.a", S(MINMAX), 0, "a", "ay", NULL, (SUBR) Max_arate},
    {"min.a", S(MINMAX), 0, "a", "ay", NULL, (SUBR) Min_arate},
    {"maxabs.a", S(MINMAX), 0, "a", "ay", NULL, (SUBR) MaxAbs_arate},
    {"minabs.a", S(MINMAX), 0, "a", "ay", NULL, (SUBR) MinAbs_arate},
    {"max.i", S(MINMAX), 0,  "i", "im", (SUBR) Max_krate, NULL, NULL},
    {"max.k", S(MINMAX), 0, "k", "kz", NULL, (SUBR) Max_krate, NULL},
    {"min.i", S(MINMAX), 0,  "i", "im", (SUBR) Min_krate, NULL, NULL},
    {"min.k", S(MINMAX), 0, "k", "kz", NULL, (SUBR) Min_krate, NULL},
    {"maxabs.k", S(MINMAX), 0, "k", "kz", NULL, (SUBR) MaxAbs_krate, NULL},
    {"minabs.k", S(MINMAX), 0, "k", "kz", NULL, (SUBR) MinAbs_krate, NULL}
};

LINKAGE_BUILTIN(minmax_localops)
