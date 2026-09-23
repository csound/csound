/*
    sterrain.c:
    Copyright (C) 2002 Matt Gilliard, John ffitch
    for the original file wave-terrain.c from the csound distribution
    Modifications and enhancements (C) 2020 Christian Bacher

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

/*  Wave-terrain synthesis opcode
 *
 *  author: m gilliard
 *          en6mjg@bath.ac.uk
 *
 *  Christian Bacher docb22@googlemail.com
 *  Changes to the original:
 *  - uses the superformula for generating the curves
 *
 *  - tables are krate
 *  - added k parameter for rotating the curve arround the current x,y
 */
typedef struct {

  OPDS h;

  cs_float *aout;
  cs_float *kamp;
  cs_float *kpch;
  cs_float *kx;
  cs_float *ky;
  cs_float *krx;
  cs_float *kry;
  cs_float *krot; // rotation of the curve
  cs_float *ktabx, *ktaby;       /* Table numbers */
  cs_float *sy;
  cs_float *sz;
  cs_float *sn1;
  cs_float *sn2;
  cs_float *sn3;
  cs_float *sa;
  cs_float *sb;
  cs_float *speriod;


/* Internals */
  cs_float oldfnx;  // storage of the current table for k-rate table change
  cs_float oldfny;  // storage of the current table for k-rate table change

  cs_float *xarr, *yarr;           /* Actual tables */

  cs_float sizx, sizy;
  cs_double theta;

} SUPERTER;

static void rotate_point(cs_float  cx, cs_float  cy, cs_float  angle, cs_float *x, cs_float *y)
{
  if(angle == 0) return;
  cs_float s = SIN(angle);
  cs_float c = COS(angle);

  *x -= cx;
  *y -= cy;

  float xnew = *x * c - *y * s;
  float ynew = *x * s + *y * c;

  *x = xnew + cx;
  *y = ynew + cy;
}

typedef struct superparams {
  cs_double y;
  cs_double z;
  cs_double n1;
  cs_double n2;
  cs_double n3;
  cs_double a;
  cs_double b;
} SUPERPARAMS;

static void superformula(cs_float t, cs_float kx, cs_float ky, cs_float krx, cs_float kry,
                         SUPERPARAMS *sp, cs_float *outX, cs_float *outY) {
    if(sp->n1 == 0) return;
    if(sp->a == 0) return;
    if(sp->b == 0) return;
    cs_float y = sp->y;
    cs_float z = sp->z;
    cs_float n1 = sp->n1;
    cs_float n2 = sp->n2;
    cs_float n3 = sp->n3;
    cs_float a = sp->a;
    cs_float b = sp->b;
    cs_float t1 = COS(y*t/4);
    cs_float t2 = SIN(z*t/4);
    cs_float r0 = POWER(FABS(t1/a),n2) + POWER(FABS(t2/b),n3);
    cs_float r = POWER(r0, -1/n1);
    *outX = kx + krx*COS(r);
    *outY = ky + kry*SIN(r);
}

static int32_t wtinit(CSOUND *csound, SUPERTER *p)
{
    p->xarr = NULL;
    p->yarr = NULL;

    p->oldfnx = -1;
    p->oldfny = -1;
    p->sizx = 0;
    p->sizy = 0;
    p->theta = 0.0;
    return OK;
}

static int32_t wtPerf(CSOUND *csound, SUPERTER *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t xloc, yloc;
    cs_float xc = FL(0.0), yc = FL(0.0);
    cs_float amp = *p->kamp;
    cs_float pch = *p->kpch;

    if (*(p->ktabx) != p->oldfnx || p->xarr == NULL) {
      p->oldfnx = *(p->ktabx);
      FUNC *ftp = csound->FTFind(csound, p->ktabx);    /* new table parameters */
      if (UNLIKELY((ftp == NULL) || ((p->xarr = ftp->ftable) == NULL)))
        return csound->PerfError(csound, &(p->h), Str("no table %g\n"), *p->ktabx);
      p->sizx = (cs_float)ftp->flen;
    }
    if (*(p->ktaby) != p->oldfny || p->yarr == NULL) {
      p->oldfny = *(p->ktaby);
      FUNC *ftp = csound->FTFind(csound, p->ktaby);    /* new table parameters */
      if (UNLIKELY((ftp == NULL) || ((p->yarr = ftp->ftable) == NULL)))
        return csound->PerfError(csound, &(p->h), Str("no table %g\n"), *p->ktaby);
      p->sizy = (cs_float)ftp->flen;
    }

    SUPERPARAMS s;
    s.y = FLOOR(*p->sy);
    s.z = FLOOR(*p->sz);
    s.n1 = *p->sn1;
    s.n2 = *p->sn2;
    s.n3 = *p->sn3;
    s.a = *p->sa;
    s.b = *p->sb;
    cs_float period = 1;
    if(*p->speriod != 0) period = 1/(*p->speriod);

    cs_float sizx = p->sizx, sizy = p->sizy;
    cs_float theta = p->theta;
    cs_float *aout = p->aout;

    if (UNLIKELY(offset)) memset(aout, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&aout[nsmps], '\0', early*sizeof(cs_float));
    }
    for (i=offset; i<nsmps; i++) {

      /* COMPUTE LOCATION OF SCANNING POINT */
      superformula(theta,*p->kx,*p->ky,*p->krx,*p->kry,&s,&xc,&yc);
      rotate_point(*p->kx,*p->ky,*p->krot,&xc,&yc);
      /* MAP SCANNING POINT TO BE IN UNIT SQUARE */
      xc = xc-FLOOR(xc);
      yc = yc-FLOOR(yc);

      /* SCALE TO TABLE-SIZE SQUARE */
      xloc = (int32_t)(xc * sizx);
      yloc = (int32_t)(yc * sizy);

      /* OUTPUT AM OF TABLE VALUES * KAMP */
      aout[i] = p->xarr[xloc] * p->yarr[yloc] * amp;

      /* MOVE SCANNING POINT ROUND THE ELLIPSE */
      theta += pch*((period*TWOPI_F) / CS_ESR);
    }

    p->theta = theta;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY sterrain_localops[] = {
  { "sterrain", S(SUPERTER), TR,   "a", "kkkkkkkkkkkkkkkkk",
    (SUBR)wtinit, (SUBR)wtPerf },
};

LINKAGE_BUILTIN(sterrain_localops)
