/*
    biquad.h:

    Copyright (C) 1998, 1999, 2001 by Hans Mikelson,
                                      Matt Gerassimoff, John ffitch,
                                      Steven Yi

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

                                                        /* biquad.h */

#pragma once

#include "stdopcod.h"

                                /* Structure for biquadratic filter */
typedef struct {
    OPDS    h;
    cs_float   *out, *in, *b0, *b1, *b2, *a0, *a1, *a2, *reinit;
    cs_double  xnm1, xnm2, ynm1, ynm2;
} BIQUAD;

                                /* Structure for moogvcf filter */
typedef struct {
    OPDS    h;
    cs_float   *out, *in, *fco, *res, *max, *iskip;
    cs_double  xnm1, y1nm1, y2nm1, y3nm1, y1n, y2n, y3n, y4n;
    cs_float   maxint, fullscale;
    int16   fcocod, rezcod;
} MOOGVCF;

                                /* Structure for rezzy filter */
typedef struct {
    OPDS    h;
    cs_float   *out, *in, *fco, *rez, *mode, *iskip;
    cs_double  xnm1, xnm2, ynm1, ynm2;
    int16   fcocod, rezcod;
    int16   warn;
} REZZY;

                                /* Structure for distortion */
typedef struct {
    OPDS    h;
    cs_float   *out, *in, *pregain, *postgain, *shape1, *shape2, *imode;
} DISTORT;

                                /* Structure for vco, analog modeling opcode */
typedef struct {
    OPDS    h;
    cs_float   *ar,
            *xamp, *xcps, *wave, *pw, *sine, *maxd, *leak, *inyq, *iphs, *iskip;
   cs_float   ynm1, ynm2, leaky, nyq, fphs;
    int16   ampcod, cpscod;
  int32   lphs,  floatph;
    FUNC    *ftp;
 /* Insert VDelay here */
    AUXCH   aux;
 /* AUXCH   auxd; */
    int32   left;
 /* End VDelay insert  */
} VCO;

typedef struct {
    OPDS    h;
    cs_float   *outx, *outy, *outz, *mass1, *mass2, *sep, *xval, *yval, *zval;
    cs_float   *vxval, *vyval, *vzval, *delta, *fric, *iskip;
    cs_float   s1z, s2z, friction;
    cs_float   x, y, z, vx, vy, vz, ax, ay, az, hstep;
} PLANET;

typedef struct {
    OPDS   h;
    cs_float  *out, *in, *fc, *v, *q, *mode, *iskip;
    cs_double xnm1, xnm2, ynm1, ynm2;
    cs_float  prv_fc, prv_v, prv_q;
    cs_double b0, b1, b2, a1, a2;
    int32_t imode, initialized;
} PAREQ;

typedef struct {
    OPDS    h;
    cs_float   *out, *in, *mode, *maxdel, *del1, *gain1, *del2, *gain2;
    cs_float   *del3, *gain3, *istor;
    cs_float   *curp, out1, out2, out3;
    cs_float   *beg1p, *beg2p, *beg3p, *end1p, *end2p, *end3p;
    cs_float   *del1p, *del2p, *del3p;
    int32   npts, imode;
    AUXCH   auxch;
} NESTEDAP;

typedef struct {
    OPDS    h;
    cs_float   *outx, *outy, *outz,
            *s, *r, *b, *hstep, *inx, *iny, *inz, *skip, *iskip;
    cs_float   valx, valy, valz;
} LORENZ;

/* And also opcodes of  Jens Groh, Munich, Germany.   mail: groh@irt.de */

/* Structure for tbvcf filter */
typedef struct {
    OPDS    h;
    cs_float   *out, *in, *fco, *res, *dist, *asym, *iskip;
    cs_double  y, y1, y2;
    int16   fcocod, rezcod;
} TBVCF;

/* Structure for mode opcode */
typedef struct {
    OPDS    h;
    cs_float   *aout, *ain, *kfreq, *kq, *reinit;
    cs_double  xnm1, ynm1, ynm2, a0, a1, a2, d;
    cs_float   lfq,lq;
    cs_float   limit;
} MODE;

typedef struct {
  OPDS h;
  cs_float *out;
  cs_float *in, *f0, *tau, *reinit;
  cs_float x, y;
} MVMFILT;
