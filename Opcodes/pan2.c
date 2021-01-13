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

// #include "csdl.h"
#include "csoundCore.h"
#include "interlocks.h"

#include <math.h>

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
      return csound->InitError(csound, Str("Unknown panning type"));
    p->lastpan = -FL(1.0);
    return OK;
}

static int32_t pan2run(CSOUND *csound, PAN2 *p)
{
    IGN(csound);
    int32_t type = p->type;
    MYFLT *ain = p->asig;
    MYFLT *al = p->aleft, *ar = p->aright;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t asgp = IS_ASIG_ARG(p->pan);
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
            MYFLT kangl = HALFPI_F * p->pan[n];
            ar[n] = ain[n] * SIN(kangl);
            al[n] = ain[n] * COS(kangl);
          }
        }
        else {
          if (*p->pan != p->lastpan) {
            MYFLT kangl = HALFPI_F * (p->lastpan = *p->pan);
            p->s = s = SIN(kangl); p->c = c = COS(kangl);
          }
          else {
            s = p->s; c = p->c;
          }
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
            MYFLT kangl = p->pan[n];
            ar[n] = ain[n] * SQRT(kangl);
            al[n] = ain[n] * SQRT(FL(1.0)-kangl);
          }
        }
        else {
          MYFLT kangl = *p->pan;
          if (kangl != p->lastpan) {
            p->s = s = SQRT(kangl);
            p->c = c = SQRT(FL(1.0)-kangl);
            p->lastpan = kangl;
          }
          else {
            s = p->s; c = p->c;
          }
          for (n=offset; n<nsmps; n++) {
            ar[n] = ain[n] * s;
            al[n] = ain[n] * c;
          }
        }
        break;
      }
    case 2:
      {
        MYFLT kangl = *p->pan;
        for (n=offset; n<nsmps; n++) {
          if (asgp) kangl = p->pan[n];
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
            kangl = p->pan[n];
            c = COS(HALFPI*kangl);
            s = SIN(HALFPI*kangl);
            l = ROOT2*(c+s)*0.5;
            r = ROOT2*(c-s)*0.5;
            al[n] = ain[n] * l;
            ar[n] = ain[n] * r;
          }
        }
        else {
          kangl = *p->pan;
          if (kangl != p->lastpan) {
            MYFLT cc = COS(HALFPI*kangl);
            MYFLT ss = SIN(HALFPI*kangl);
            p->s = s = ROOT2*(cc+ss)*0.5;
            p->c = c = ROOT2*(cc-ss)*0.5;
            p->lastpan = kangl;
          }
          else {
             s = p->s; c = p->c;
          }
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


static OENTRY pan2_localops[] =
{
 { "pan2", sizeof(PAN2), 0, 3, "aa", "axo", (SUBR) pan2set, (SUBR) pan2run },
};

LINKAGE_BUILTIN(pan2_localops)
