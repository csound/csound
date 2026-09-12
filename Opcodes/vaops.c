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
        MYFLT   *kout, *kindx, *avar;
} VA_GET;

typedef struct {
        OPDS    h;
        MYFLT   *kval, *kindx, *avar;
} VA_SET;


typedef struct {
        OPDS    h;
        MYFLT   *kout, *avar, *kindx;
} VASIG_GET;

typedef struct {
        OPDS    h;
        MYFLT   *avar, *kval, *kindx;
} VASIG_SET;

static int32_t vaget(CSOUND *csound, VA_GET *p)
{
    double index = *p->kindx;
    uint32_t ndx;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    if (UNLIKELY(!(index >= 0.0 && index < (double)CS_KSMPS)))
      return csound->PerfError(csound, &(p->h),
                              Str("Out of range in vaget.k (%g)"), index);
    /* A checked nonnegative index truncates to the same sample as floor. */
    ndx = (uint32_t)index;
    if (UNLIKELY(ndx < offset || ndx >= CS_KSMPS-early))
      csound->Warning(csound, "index %u outside sample-accurate bounds [%u, %u)",
                      ndx, offset, CS_KSMPS-early);
    *p->kout = p->avar[ndx];
    return OK;
}

static int32_t vaset(CSOUND *csound, VA_SET *p)
{
    double index = *p->kindx;
    uint32_t ndx;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    if (UNLIKELY(!(index >= 0.0 && index < (double)CS_KSMPS)))
      return csound->PerfError(csound, &(p->h),
                              Str("Out of range in vaset.k (%g)"), index);
    ndx = (uint32_t)index;
    if (UNLIKELY(ndx < offset || ndx >= CS_KSMPS-early))
      csound->Warning(csound, "index %u outside sample-accurate bounds [%u, %u)",
                      ndx, offset, CS_KSMPS-early);
    p->avar[ndx] = *p->kval;
    return OK;
}


static int32_t vasigget(CSOUND *csound, VASIG_GET *p)
{
    double index = *p->kindx;
    uint32_t ndx;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;

    if (UNLIKELY(!(index >= 0.0 && index < (double)CS_KSMPS)))
      return csound->PerfError(csound, &(p->h),
                              Str("Out of range in vasigget.k (%g)"), index);
    ndx = (uint32_t)index;
    if (UNLIKELY(ndx < offset || ndx >= CS_KSMPS-early))
      csound->Warning(csound, "index %u outside sample-accurate bounds [%u, %u)",
                      ndx, offset, CS_KSMPS-early);
    *p->kout = p->avar[ndx];
    return OK;
}

static int32_t vasigset(CSOUND *csound, VASIG_SET *p)
{
    double index = *p->kindx;
    uint32_t ndx;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;

    if (UNLIKELY(!(index >= 0.0 && index < (double)CS_KSMPS)))
      return csound->PerfError(csound, &(p->h),
                              Str("Out of range in vasigset.k (%g)"), index);
    ndx = (uint32_t)index;
    if (UNLIKELY(ndx < offset || ndx >= CS_KSMPS-early))
      csound->Warning(csound, "index %u outside sample-accurate bounds [%u, %u)",
                      ndx, offset, CS_KSMPS-early);
    p->avar[ndx] = *p->kval;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY vaops_localops[] = {
  { "vaget", S(VA_GET),    0,       "k", "ka",  NULL, (SUBR)vaget },
  { "vaset", S(VA_SET),    WI,       "",  "kka", NULL, (SUBR)vaset },
  { "##array_get", S(VASIG_GET),    0,       "k", "ak",  NULL, (SUBR)vasigget },
  { "##array_set", S(VASIG_SET),    0,       "",  "akk", NULL, (SUBR)vasigset }
};


LINKAGE_BUILTIN(vaops_localops)

