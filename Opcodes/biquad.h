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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

                                                        /* biquad.h */

#pragma once

#include "stdopcod.h"

                                /* Structure for biquadratic filter */
typedef struct {
    OPDS    h;
    MYFLT   *out, *in, *b0, *b1, *b2, *a0, *a1, *a2, *reinit;
    double  xnm1, xnm2, ynm1, ynm2;
} BIQUAD;

                                /* Structure for moogvcf filter */
typedef struct {
    OPDS    h;
    MYFLT   *out, *in, *fco, *res, *max, *iskip;
    double  xnm1, y1nm1, y2nm1, y3nm1, y1n, y2n, y3n, y4n;
    MYFLT   maxint;
    int16   fcocod, rezcod;
} MOOGVCF;

                                /* Structure for rezzy filter */
typedef struct {
    OPDS    h;
    MYFLT   *out, *in, *fco, *rez, *mode, *iskip;
    double  xnm1, xnm2, ynm1, ynm2;
    int16   fcocod, rezcod;
    int16   warn;
} REZZY;

                                /* Structure for distortion */
typedef struct {
    OPDS    h;
    MYFLT   *out, *in, *pregain, *postgain, *shape1, *shape2, *imode;
} DISTORT;

                                /* Structure for vco, analog modeling opcode */
typedef struct {
    OPDS    h;
    MYFLT   *ar,
            *xamp, *xcps, *wave, *pw, *sine, *maxd, *leak, *inyq, *iphs, *iskip;
   MYFLT   ynm1, ynm2, leaky, nyq, fphs;
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
    MYFLT   *outx, *outy, *outz, *mass1, *mass2, *sep, *xval, *yval, *zval;
    MYFLT   *vxval, *vyval, *vzval, *delta, *fric, *iskip;
    MYFLT   s1z, s2z, friction;
    MYFLT   x, y, z, vx, vy, vz, ax, ay, az, hstep;
} PLANET;

typedef struct {
    OPDS   h;
    MYFLT  *out, *in, *fc, *v, *q, *mode, *iskip;
    double xnm1, xnm2, ynm1, ynm2;
    MYFLT  prv_fc, prv_v, prv_q;
    double b0, b1, b2, a1, a2;
    int32_t imode;
} PAREQ;

typedef struct {
    OPDS    h;
    MYFLT   *out, *in, *mode, *maxdel, *del1, *gain1, *del2, *gain2;
    MYFLT   *del3, *gain3, *istor;
    MYFLT   *curp, out1, out2, out3;
    MYFLT   *beg1p, *beg2p, *beg3p, *end1p, *end2p, *end3p;
    MYFLT   *del1p, *del2p, *del3p;
    int32   npts;
    AUXCH   auxch;
} NESTEDAP;

typedef struct {
    OPDS    h;
    MYFLT   *outx, *outy, *outz,
            *s, *r, *b, *hstep, *inx, *iny, *inz, *skip, *iskip;
    MYFLT   valx, valy, valz;
} LORENZ;

/* And also opcodes of  Jens Groh, Munich, Germany.   mail: groh@irt.de */

/* Structure for tbvcf filter */
typedef struct {
    OPDS    h;
    MYFLT   *out, *in, *fco, *res, *dist, *asym, *iskip;
    double  y, y1, y2;
    int16   fcocod, rezcod;
} TBVCF;

/* Structure for mode opcode */
typedef struct {
    OPDS    h;
    MYFLT   *aout, *ain, *kfreq, *kq, *reinit;
    double  xnm1, ynm1, ynm2, a0, a1, a2, d;
    MYFLT   lfq,lq;
    MYFLT   limit;
} MODE;

typedef struct {
  OPDS h;
  MYFLT *out;
  MYFLT *in, *f0, *tau, *reinit;
  MYFLT x, y;
} MVMFILT;
