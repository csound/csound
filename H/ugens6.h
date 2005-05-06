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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/*                                                      UGENS6.H        */

typedef struct {
        OPDS    h;
        MYFLT   *kr, *asig, *ilen;
        int     len;
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
        OPDS    h;
        MYFLT   *rslt, *xsig, *istor, *imode;   /* IV - Sep 5 2002 */
        int     init_k;
        MYFLT   prev;
} INTERP;

typedef struct {
        OPDS    h;
        MYFLT   *xr, *xsig, *xgate, *ival, *istor;
        MYFLT   state;
        int     audiogate;
} SAMPHOLD;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *idlt, *istor;
        MYFLT   *curp;
        long    npts;
        AUXCH   auxch;
} DELAY;

typedef struct DELAYR {
        OPDS    h;
        MYFLT   *ar, *indx, *idlt, *istor;
        MYFLT   *curp;
        long    npts;
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
        int     wsize;
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
        long    revlpsum;
        AUXCH   revlpsiz;
} REVERB;

typedef struct {
        OPDS    h;
        MYFLT   *r1, *r2, *r3, *r4, *asig, *kx, *ky, *ifn, *imode, *ioffset;
        MYFLT   xmul;
        long    xoff;
        FUNC    *ftp;
} PAN;

int downset(ENVIRON*,DOWNSAMP *p);
int downsamp(ENVIRON*,DOWNSAMP *p);
int upsamp(ENVIRON*,UPSAMP *p);
int a_k_set(ENVIRON*,INTERP *p);
int interpset(ENVIRON*,INTERP *p);
int interp(ENVIRON*,INTERP *p);
int indfset(ENVIRON*,INDIFF *p);
int kntegrate(ENVIRON*,INDIFF *p);
int integrate(ENVIRON*,INDIFF *p);
int kdiff(ENVIRON*,INDIFF *p);
int diff(ENVIRON*,INDIFF *p);
int samphset(ENVIRON*,SAMPHOLD *p);
int ksmphold(ENVIRON*,SAMPHOLD *p);
int samphold(ENVIRON*,SAMPHOLD *p);
int delset(ENVIRON*,DELAY *p);
int delrset(ENVIRON*,DELAYR *p);
int delwset(ENVIRON*,DELAYW *p);
int tapset(ENVIRON*,DELTAP *p);
int delay(ENVIRON*,DELAY *p);
int delayr(ENVIRON*,DELAYR *p);
int delayw(ENVIRON*,DELAYW *p);
int deltap(ENVIRON*,DELTAP *p);
int deltapi(ENVIRON*,DELTAP *p);
int deltapn(ENVIRON*,DELTAP *p);
int deltap3(ENVIRON*,DELTAP *p);
int tapxset(ENVIRON*,DELTAPX *p);
int deltapx(ENVIRON*,DELTAPX *p);
int deltapxw(ENVIRON*,DELTAPX *p);
int del1set(ENVIRON*,DELAY1 *p);
int delay1(ENVIRON*,DELAY1 *p);
int cmbset(ENVIRON*,COMB *p);
int comb(ENVIRON*,COMB *p);
int alpass(ENVIRON*,COMB *p);
void reverbinit(ENVIRON *);
int rvbset(ENVIRON*,REVERB *p);
int reverb(ENVIRON*,REVERB *p);
int panset(ENVIRON*,PAN *p);
int pan(ENVIRON*,PAN *p);

