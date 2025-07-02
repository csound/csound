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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

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
    int32 ndx = (int32) MYFLOOR((double)*p->kindx);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    if(LIKELY(ndx >= 0 && ndx < CS_KSMPS)) {
    if (UNLIKELY(ndx<(int32)offset || ndx>=(int32)(CS_KSMPS-early)))
      csound->Warning(csound, "index %d outside sample-accurate bounds (%d, %d]",
                      ndx, offset, CS_KSMPS-early);
    *p->kout = p->avar[ndx];
    return OK;
    } else
      return csound->PerfError(csound, &(p->h),
                               Str("Out of range in vaget.k (%d)"), ndx);

}

static int32_t vaset(CSOUND *csound, VA_SET *p)
{
    int32 ndx = (int32) MYFLOOR((double)*p->kindx);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    if(LIKELY(ndx >= 0 && ndx < CS_KSMPS)) {
    if (UNLIKELY(ndx<(int32)offset || ndx>=(int32)(CS_KSMPS-early)))
      csound->Warning(csound, "index %d outside sample-accurate bounds (%d, %d]",
                      ndx, offset, CS_KSMPS-early);
     p->avar[ndx] = *p->kval;
     return OK;
    }
    else return csound->PerfError(csound, &(p->h),
                               Str("Out of range in vaset.k (%d)"), ndx);
}


static int32_t vasigget(CSOUND *csound, VASIG_GET *p)
{
    int32 ndx = (int32) MYFLOOR((double)*p->kindx);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    if(LIKELY(ndx >= 0 && ndx < CS_KSMPS)) {
    if (UNLIKELY(ndx<(int32)offset || ndx>=(int32)(CS_KSMPS-early)))
      csound->Warning(csound, "index %d outside sample-accurate bounds (%d, %d]",
                      ndx, offset, CS_KSMPS-early);
    *p->kout = p->avar[ndx];
    return OK;
    } else
      return csound->PerfError(csound, &(p->h),
                               Str("Out of range in vasigget.k (%d)"), ndx);
}

static int32_t vasigset(CSOUND *csound, VASIG_SET *p)
{
    int32 ndx = (int32) MYFLOOR((double)*p->kindx);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    if(LIKELY(ndx >= 0 && ndx < CS_KSMPS)) {
    if (UNLIKELY(ndx<(int32)offset || ndx>=(int32)(CS_KSMPS-early)))
      csound->Warning(csound, "index %d outside sample-accurate bounds (%d, %d]",
                      ndx, offset, CS_KSMPS-early);
     p->avar[ndx] = *p->kval;
     return OK;
    }
    else return csound->PerfError(csound, &(p->h),
                               Str("Out of range in vasigset.k (%d)"), ndx);
    
}

#define S(x)    sizeof(x)

static OENTRY vaops_localops[] = {
  { "vaget", S(VA_GET),    0,       "k", "ka",  NULL, (SUBR)vaget },
  { "vaset", S(VA_SET),    WI,       "",  "kka", NULL, (SUBR)vaset },
  { "##array_get", S(VASIG_GET),    0,       "k", "ak",  NULL, (SUBR)vasigget },
  { "##array_set", S(VASIG_SET),    0,       "",  "akk", NULL, (SUBR)vasigset }
};


LINKAGE_BUILTIN(vaops_localops)

