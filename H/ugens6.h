/*  
    ugens6.h:

    Copyright (C) 1991-2000 Barry Vercoe, John ffitch, Jens Groh, Hans Mikelson, Istvan Varga

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

/*                                                                      UGENS6.H        */

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
        MYFLT   *ar, *idlt, *istor;
        MYFLT   *curp;
        long    npts;
        AUXCH   auxch;
        struct DELAYR  *next_delayr; /* fifo for delayr pointers by Jens Groh */
} DELAYR;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *xdlt;
        DELAYR  *delayr;
} DELTAP;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *adlt, *iwsize;
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

int downset(DOWNSAMP *p);
int downsamp(DOWNSAMP *p);
int upsamp(UPSAMP *p);
int a_k_set(INTERP *p);
int interpset(INTERP *p);
int interp(INTERP *p);
int indfset(INDIFF *p);
int kntegrate(INDIFF *p);
int integrate(INDIFF *p);
int kdiff(INDIFF *p);
int diff(INDIFF *p);
int samphset(SAMPHOLD *p);
int ksmphold(SAMPHOLD *p);
int samphold(SAMPHOLD *p);
int delset(DELAY *p);
int delrset(DELAYR *p);
int delwset(DELAYW *p);
int tapset(DELTAP *p);
int delay(DELAY *p);
int delayr(DELAYR *p);
int delayw(DELAYW *p);
int deltap(DELTAP *p);
int deltapi(DELTAP *p);
int deltapn(DELTAP *p);
int deltap3(DELTAP *p);
int tapxset(DELTAPX *p);
int deltapx(DELTAPX *p);
int deltapxw(DELTAPX *p);
int del1set(DELAY1 *p);
int delay1(DELAY1 *p);
int cmbset(COMB *p);
int comb(COMB *p);
int alpass(COMB *p);
void reverbinit(void);
int rvbset(REVERB *p);
int reverb(REVERB *p);
int panset(PAN *p);
int pan(PAN *p);

