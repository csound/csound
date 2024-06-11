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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

/*              Butterworth filters coded by Paris Smaragdis 1994       */
/*              Berklee College of Music Csound development team        */
/*              Copyright (c) May 1994.  All rights reserved            */

#include "stdopcod.h"

typedef struct  {
        OPDS    h;
        MYFLT   *sr, *ain, *kfc, *istor;
        MYFLT   lkf;
        double  a[8];
} BFIL;

typedef struct  {
        OPDS    h;
        MYFLT   *sr, *ain, *kfo, *kbw, *istor;
        MYFLT   lkf, lkb;
        double  a[8];
} BBFIL;

#include <math.h>
//#define ROOT2 (1.4142135623730950488)

static void butter_filter(uint32_t, uint32_t, MYFLT *, MYFLT *, double *);

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
    MYFLT       *out, *in;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    in = p->ain;
    out = p->sr;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }

    if (*p->kfc <= FL(0.0))     {
      memcpy(&out[offset], &in[offset], (nsmps-offset)*sizeof(MYFLT));
      return OK;
    }

    if (*p->kfc != p->lkf)      {
      double    *a, c;

      a = p->a;
      p->lkf = *p->kfc;
      c = tan((double)(CS_PIDSR * p->lkf));

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
    MYFLT       *out, *in;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    in = p->ain;
    out = p->sr;

    if (*p->kfc <= FL(0.0))     {
      memset(out, 0, nsmps*sizeof(MYFLT));
      return OK;
    }

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }

    if (*p->kfc != p->lkf) {
      double     *a, c;
      a = p->a;
      p->lkf = *p->kfc;
      c = 1.0 / tan((double)(CS_PIDSR * p->lkf));
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
                          MYFLT *in, MYFLT *out, double *a)
{
    double t, y;
    uint32_t nn;

    for (nn=offset; nn<n; nn++) {
      t = (double)in[nn] - a[4] * a[6] - a[5] * a[7];
      t = csoundUndenormalizeDouble(t); /* Not needed on AMD */
      y = t * a[1] + a[2] * a[6] + a[3] * a[7];
      a[7] = a[6];
      a[6] = t;
      out[nn] = (MYFLT)y;
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
