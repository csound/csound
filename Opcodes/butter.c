/*
    butter.c:

    Copyright (C) 1994 Paris Smaragdis, John ffitch

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

/*              Butterworth filters coded by Paris Smaragdis 1994       */
/*              Berklee College of Music Csound development team        */
/*              Copyright (c) May 1994.  All rights reserved            */

#include "stdopcod.h"

typedef struct  {
        OPDS    h;
        cs_float   *sr, *ain, *kfc, *istor;
        cs_float   lkf;
        cs_double  a[8];
} BFIL;

typedef struct  {
        OPDS    h;
        cs_float   *sr, *ain, *kfo, *kbw, *istor;
        cs_float   lkf, lkb;
        cs_double  a[8];
} BBFIL;

#include <math.h>
//#define ROOT2 (1.4142135623730950488)

static void butter_filter(uint32_t, uint32_t, cs_float *, cs_float *, cs_double *);

int32_t butset(CSOUND *csound, BFIL *p)      /*      Hi/Lo pass set-up   */
{
     IGN(csound);
    if (*p->istor==FL(0.0)) {
      p->a[6] = p->a[7] = 0.0;
      p->lkf = FL(0.0);
    }
    return OK;
}

static int32_t hibut(CSOUND *csound, BFIL *p)       /*      Hipass filter       */
{
    cs_float       *out, *in;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    in = p->ain;
    out = p->sr;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(cs_float));
    }

    if (*p->kfc <= FL(0.0))     {
      if (out != in)
        memcpy(&out[offset], &in[offset], (nsmps-offset)*sizeof(cs_float));
      return OK;
    }

    if (*p->kfc != p->lkf)      {
      cs_double    *a, c;

      a = p->a;
      p->lkf = *p->kfc;
      c = tan((cs_double)(CS_PIDSR * p->lkf));

      a[1] = 1.0 / ( 1.0 + ROOT2 * c + c * c);
      a[2] = -(a[1] + a[1]);
      a[3] = a[1];
      a[4] = 2.0 * ( c*c - 1.0) * a[1];
      a[5] = ( 1.0 - ROOT2 * c + c * c) * a[1];
    }
    butter_filter(nsmps, offset, in, out, p->a);
    return OK;
}

static int32_t lobut(CSOUND *csound, BFIL *p)       /*      Lopass filter       */
{
    cs_float       *out, *in;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    in = p->ain;
    out = p->sr;

    if (*p->kfc <= FL(0.0))     {
      memset(out, 0, nsmps*sizeof(cs_float));
      return OK;
    }

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(cs_float));
    }

    if (*p->kfc != p->lkf) {
      cs_double     *a, c;
      a = p->a;
      p->lkf = *p->kfc;
      c = 1.0 / tan((cs_double)(CS_PIDSR * p->lkf));
      a[1] = 1.0 / ( 1.0 + ROOT2 * c + c * c);
      a[2] = a[1] + a[1];
      a[3] = a[1];
      a[4] = 2.0 * ( 1.0 - c*c) * a[1];
      a[5] = ( 1.0 - ROOT2 * c + c * c) * a[1];
    }

    butter_filter(nsmps, offset, in, out, p->a);
    return OK;
}

/* Filter loop */

static void butter_filter(uint32_t n, uint32_t offset,
                          cs_float *in, cs_float *out, cs_double *a)
{
    cs_double t, y;
    uint32_t nn;

    for (nn=offset; nn<n; nn++) {
      t = (cs_double)in[nn] - a[4] * a[6] - a[5] * a[7];
      t = csoundUndenormalizeDouble(t); /* Not needed on AMD */
      y = t * a[1] + a[2] * a[6] + a[3] * a[7];
      a[7] = a[6];
      a[6] = t;
      out[nn] = (cs_float)y;
    }
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "butterhp.k", S(BFIL), 0,  "a",    "ako",  (SUBR)butset,   (SUBR)hibut  },
{ "butterlp.k", S(BFIL), 0,  "a",    "ako",  (SUBR)butset,   (SUBR)lobut  },
{ "buthp.k",    S(BFIL),  0,  "a",   "ako",  (SUBR)butset,   (SUBR)hibut  },
{ "butlp.k",    S(BFIL),  0,  "a",   "ako",  (SUBR)butset,   (SUBR)lobut  },
};

int32_t butter_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
