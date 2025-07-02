/*
    ugens6.h:

    Copyright (C) 1991-2000 Barry Vercoe, John ffitch, Jens Groh,
                            Hans Mikelson, Istvan Varga

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

/*                                                      UGENS6.H        */

#pragma once

typedef struct {
        OPDS    h;
        MYFLT   *kr, *asig, *ilen;
        uint32_t     len;
} DOWNSAMP;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *ksig;
} UPSAMP;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *xsig, *istor;
        MYFLT   prev;
} INDIFF;

typedef struct {
  OPDS    h;                                             /* JPff Nov 2015 */
  MYFLT   *rslt, *xsig, *istor, *imode, *istart;   /* IV - Sep 5 2002 */
        int32_t     init_k;
        MYFLT   prev;
} INTERP;

typedef struct {
        OPDS    h;
        MYFLT   *xr, *xsig, *xgate, *ival, *istor;
        MYFLT   state;
        int32_t     audiogate;
} SAMPHOLD;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *idlt, *istor;
        MYFLT   *curp;
        int32    npts;
        AUXCH   auxch;
} DELAY;

typedef struct DELAYR {
        OPDS    h;
        MYFLT   *ar, *indx, *idlt, *istor;
        MYFLT   *curp;
        uint32_t npts;
        AUXCH   auxch;
        struct DELAYR  *next_delayr; /* fifo for delayr pointers by Jens Groh */
} DELAYR;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *xdlt, *indx;
        DELAYR  *delayr;
} DELTAP;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *adlt, *iwsize, *indx;
        int32_t     wsize;
        double  d2x;
        DELAYR  *delayr;
} DELTAPX;

typedef struct {
        OPDS    h;
        MYFLT   *asig;
        DELAYR  *delayr;
} DELAYW;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *istor;
        MYFLT   sav1;
} DELAY1;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *krvt, *ilpt, *istor, *insmps;
        MYFLT   coef, prvt, *pntr;
        AUXCH   auxch;
} COMB;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *krvt, *istor;
        MYFLT   c1, c2, c3, c4, c5, c6, prvt;
        MYFLT   *p1, *p2, *p3, *p4, *p5, *p6;
        MYFLT   *adr1, *adr2, *adr3, *adr4, *adr5, *adr6;
        AUXCH   auxch;
        int32   revlpsum;
        int32   revlpsiza[6];
        AUXCH   revlpsiz;
} REVERB;

typedef struct {
        OPDS    h;
        MYFLT   *r1, *r2, *r3, *r4, *asig, *kx, *ky, *ifn, *imode, *ioffset;
        MYFLT   xmul, xoff;
        FUNC    *ftp;
} PAN;

int32_t downset(CSOUND *, DOWNSAMP *p);
int32_t downsamp(CSOUND *, DOWNSAMP *p);
int32_t upsamp(CSOUND *, UPSAMP *p);
int32_t a_k_set(CSOUND *, INTERP *p);
int32_t interpset(CSOUND *, INTERP *p);
int32_t interp(CSOUND *, INTERP *p);
int32_t indfset(CSOUND *, INDIFF *p);
int32_t kntegrate(CSOUND *, INDIFF *p);
int32_t integrate(CSOUND *, INDIFF *p);
int32_t kdiff(CSOUND *, INDIFF *p);
int32_t diff(CSOUND *, INDIFF *p);
int32_t samphset(CSOUND *, SAMPHOLD *p);
int32_t ksmphold(CSOUND *, SAMPHOLD *p);
int32_t samphold(CSOUND *, SAMPHOLD *p);
int32_t delset(CSOUND *, DELAY *p);
int32_t delrset(CSOUND *, DELAYR *p);
int32_t delwset(CSOUND *, DELAYW *p);
int32_t tapset(CSOUND *, DELTAP *p);
int32_t delay(CSOUND *, DELAY *p);
int32_t delayr(CSOUND *, DELAYR *p);
int32_t delayw(CSOUND *, DELAYW *p);
int32_t deltap(CSOUND *, DELTAP *p);
int32_t deltapi(CSOUND *, DELTAP *p);
int32_t deltapn(CSOUND *, DELTAP *p);
int32_t deltap3(CSOUND *, DELTAP *p);
int32_t tapxset(CSOUND *, DELTAPX *p);
int32_t deltapx(CSOUND *, DELTAPX *p);
int32_t deltapxw(CSOUND *, DELTAPX *p);
int32_t del1set(CSOUND *, DELAY1 *p);
int32_t delay1(CSOUND *, DELAY1 *p);
int32_t cmbset(CSOUND *, COMB *p);
int32_t comb(CSOUND *, COMB *p);
int32_t invcomb(CSOUND *, COMB *p);
int32_t alpass(CSOUND *, COMB *p);
int32_t rvbset(CSOUND *, REVERB *p);
int32_t reverb(CSOUND *, REVERB *p);
int32_t panset(CSOUND *, PAN *p);
int32_t pan(CSOUND *, PAN *p);

