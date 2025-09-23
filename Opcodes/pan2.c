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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
    MYFLT *aleft;                /* Left output  */
    MYFLT *aright;               /* Right output   */
    MYFLT *asig;
    MYFLT *pan;                  /* pan position */
    MYFLT *itype;                /* type of panning */
    int32_t   type;
    MYFLT lastpan, s, c;         /* Cached values */
} PAN2;
//#define SQRT2 FL(1.41421356237309504880)

static int32_t pan2set(CSOUND *csound, PAN2 *p)
{
    int32_t type = p->type = MYFLT2LRND(*p->itype);
    if (UNLIKELY(type <0 || type > 3))
      return csound->InitError(csound, "%s", Str("Unknown panning type"));
    p->lastpan = -FL(1.0);
    return OK;
}

static int32_t pan2run_common(CSOUND *csound, OPDS *opds, MYFLT *pan, int32_t type, MYFLT *ain, MYFLT *al, MYFLT *ar) {
    IGN(csound);

    uint32_t offset = opds->insdshead->ksmps_offset;
    uint32_t early  = opds->insdshead->ksmps_no_end;
    uint32_t n, nsmps = opds->insdshead->ksmps;
    int32_t asgp = IS_ASIG_ARG(pan);
    MYFLT s, c;
    if (UNLIKELY(offset)) {
      memset(ar, '\0', offset*sizeof(MYFLT));
      memset(al, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
      memset(&al[nsmps], '\0', early*sizeof(MYFLT));
    }
    switch (type) {
    case 0:
      {
        if (asgp) {
          for (n=offset; n<nsmps; n++) {
            MYFLT kangl = HALFPI_F * pan[n];
            ar[n] = ain[n] * SIN(kangl);
            al[n] = ain[n] * COS(kangl);
          }
        }
        else {
          MYFLT kangl = HALFPI_F * *pan;
          s = SIN(kangl); c = COS(kangl);
          for (n=offset; n<nsmps; n++) {
            ar[n] = ain[n] * s;
            al[n] = ain[n] * c;
          }
        }
        break;
      }
    case 1:
      {
        if (asgp) {
          for (n=offset; n<nsmps; n++) {
            MYFLT kangl = pan[n];
            ar[n] = ain[n] * SQRT(kangl);
            al[n] = ain[n] * SQRT(FL(1.0)-kangl);
          }
        }
        else {
          MYFLT kangl = *pan;
          s = SQRT(kangl);
          c = SQRT(FL(1.0)-kangl);
          for (n=offset; n<nsmps; n++) {
            ar[n] = ain[n] * s;
            al[n] = ain[n] * c;
          }
        }
        break;
      }
    case 2:
      {
        MYFLT kangl = *pan;
        for (n=offset; n<nsmps; n++) {
          if (asgp) kangl = pan[n];
          ar[n] = ain[n] * kangl;
          al[n] = ain[n] * (FL(1.0)-kangl);
        }
        break;
      }
    case 3:
      {
        MYFLT kangl, l, r;
        if (asgp) {
          for (n=offset; n<nsmps; n++) {
            kangl = pan[n];
            c = COS(HALFPI*kangl);
            s = SIN(HALFPI*kangl);
            l = ROOT2*(c+s)*0.5;
            r = ROOT2*(c-s)*0.5;
            al[n] = ain[n] * l;
            ar[n] = ain[n] * r;
          }
        }
        else {
          kangl = *pan;
          MYFLT cc = COS(HALFPI*kangl);
          MYFLT ss = SIN(HALFPI*kangl);
          s = ROOT2*(cc+ss)*0.5;
          c = ROOT2*(cc-ss)*0.5;
          for (n=offset; n<nsmps; n++) {
            al[n] = ain[n] * s;
            ar[n] = ain[n] * c;
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
    MYFLT *asig;
    MYFLT *pan;
    MYFLT *itype;
    int32_t type;
} PAN2ARR;

static int32_t pan2arr_set(CSOUND *csound, PAN2ARR *p) {
    int32_t type = p->type = MYFLT2LRND(*p->itype);
    if (UNLIKELY(type <0 || type > 3))
      return csound->InitError(csound, "%s", Str("Unknown panning type"));
    // p->lastpan = -FL(1.0);
    tabinit(csound, p->out, 2, p->h.insdshead);
    return OK;
}

static int32_t pan2arr_run(CSOUND *csound, PAN2ARR *p) {
    MYFLT *al = p->out->data;
    MYFLT *ar = p->out->data + CS_KSMPS;
    return pan2run_common(csound, &(p->h), p->pan, p->type, p->asig, al, ar);
}


static OENTRY pan2_localops[] =
{
 { "pan2", sizeof(PAN2), 0,  "aa", "axo", (SUBR) pan2set, (SUBR) pan2run },
 { "pan2", sizeof(PAN2ARR), 0,  "a[]", "axo", (SUBR) pan2arr_set, (SUBR) pan2arr_run },

};

LINKAGE_BUILTIN(pan2_localops)
