/*
    pan2.c:

    Copyright (C) 2007 John ffitch

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

#include <math.h>
#include "arrays.h"

typedef struct {
    OPDS h;
    cs_float *aleft;                /* Left output  */
    cs_float *aright;               /* Right output   */
    cs_float *asig;
    cs_float *pan;                  /* pan position */
    cs_float *itype;                /* type of panning */
    int32_t   type;
    cs_float lastpan, s, c;         /* Cached values */
} PAN2;
//#define SQRT2 FL(1.41421356237309504880)

static int32_t pan2set(CSOUND *csound, PAN2 *p)
{
    int32_t type = p->type = CS_FLOAT2LRND(*p->itype);
    if (UNLIKELY(type <0 || type > 3))
      return csound->InitError(csound, "%s", Str("Unknown panning type"));
    p->lastpan = -FL(1.0);
    return OK;
}

static int32_t pan2run_common(CSOUND *csound, OPDS *opds, cs_float *pan, int32_t type, cs_float *ain, cs_float *al, cs_float *ar) {
    IGN(csound);

    uint32_t offset = opds->insdshead->ksmps_offset;
    uint32_t early  = opds->insdshead->ksmps_no_end;
    uint32_t n, nsmps = opds->insdshead->ksmps;
    int32_t asgp = IS_ASIG_ARG(pan);
    cs_float s, c;
    if (UNLIKELY(offset)) {
      memset(ar, '\0', offset*sizeof(cs_float));
      memset(al, '\0', offset*sizeof(cs_float));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
      memset(&al[nsmps], '\0', early*sizeof(cs_float));
    }
    /* Either output may share the input buffer. Read each sample first. */
    switch (type) {
    case 0:
      {
        if (asgp) {
          for (n=offset; n<nsmps; n++) {
            cs_float sample = ain[n];
            cs_float kangl = HALFPI_F * pan[n];
            ar[n] = sample * SIN(kangl);
            al[n] = sample * COS(kangl);
          }
        }
        else {
          cs_float kangl = HALFPI_F * *pan;
          s = SIN(kangl); c = COS(kangl);
          for (n=offset; n<nsmps; n++) {
            cs_float sample = ain[n];
            ar[n] = sample * s;
            al[n] = sample * c;
          }
        }
        break;
      }
    case 1:
      {
        if (asgp) {
          for (n=offset; n<nsmps; n++) {
            cs_float sample = ain[n];
            cs_float kangl = pan[n];
            ar[n] = sample * SQRT(kangl);
            al[n] = sample * SQRT(FL(1.0)-kangl);
          }
        }
        else {
          cs_float kangl = *pan;
          s = SQRT(kangl);
          c = SQRT(FL(1.0)-kangl);
          for (n=offset; n<nsmps; n++) {
            cs_float sample = ain[n];
            ar[n] = sample * s;
            al[n] = sample * c;
          }
        }
        break;
      }
    case 2:
      {
        cs_float kangl = *pan;
        for (n=offset; n<nsmps; n++) {
          cs_float sample = ain[n];
          if (asgp) kangl = pan[n];
          ar[n] = sample * kangl;
          al[n] = sample * (FL(1.0)-kangl);
        }
        break;
      }
    case 3:
      {
        cs_float kangl, l, r;
        /* This formula takes +0.5 for hard left and -0.5 for hard right. */
        if (asgp) {
          for (n=offset; n<nsmps; n++) {
            cs_float sample = ain[n];
            kangl = FL(0.5) - pan[n];
            c = COS(HALFPI*kangl);
            s = SIN(HALFPI*kangl);
            l = ROOT2*(c+s)*0.5;
            r = ROOT2*(c-s)*0.5;
            al[n] = sample * l;
            ar[n] = sample * r;
          }
        }
        else {
          kangl = FL(0.5) - *pan;
          cs_float cc = COS(HALFPI*kangl);
          cs_float ss = SIN(HALFPI*kangl);
          s = ROOT2*(cc+ss)*0.5;
          c = ROOT2*(cc-ss)*0.5;
          for (n=offset; n<nsmps; n++) {
            cs_float sample = ain[n];
            al[n] = sample * s;
            ar[n] = sample * c;
          }
        }
        break;
      }
    }
    return OK;
}

static int32_t pan2run(CSOUND *csound, PAN2 *p)
{
    return pan2run_common(csound, &(p->h), p->pan, p->type, p->asig, p->aleft, p->aright);
}

typedef struct {
    OPDS h;
    ARRAYDAT *out;
    cs_float *asig;
    cs_float *pan;
    cs_float *itype;
    int32_t type;
} PAN2ARR;

static int32_t pan2arr_set(CSOUND *csound, PAN2ARR *p) {
    int32_t type = p->type = CS_FLOAT2LRND(*p->itype);
    if (UNLIKELY(type <0 || type > 3))
      return csound->InitError(csound, "%s", Str("Unknown panning type"));
    // p->lastpan = -FL(1.0);
    if (UNLIKELY(tabinit(csound, p->out, 2, p->h.insdshead) != OK))
      return csound_array_init_resize_error(csound);
    return OK;
}

static int32_t pan2arr_run(CSOUND *csound, PAN2ARR *p) {
    cs_float *al = p->out->data;
    cs_float *ar = p->out->data + CS_KSMPS;
    return pan2run_common(csound, &(p->h), p->pan, p->type, p->asig, al, ar);
}


static OENTRY pan2_localops[] =
{
 { "pan2", sizeof(PAN2), 0,  "aa", "axo", (SUBR) pan2set, (SUBR) pan2run },
 { "pan2", sizeof(PAN2ARR), 0,  "a[]", "axo", (SUBR) pan2arr_set, (SUBR) pan2arr_run },

};

LINKAGE_BUILTIN(pan2_localops)
