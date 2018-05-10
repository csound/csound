/*
    spectra.h:

    Copyright (C) 1995 Barry Vercoe

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
                        /*                              SPECTRA.H       */
#ifndef __SPECTRA_H
#define __SPECTRA_H

#define MAXFRQS 120

typedef struct {
        OPDS    h;
        SPECDAT *wsig;
        MYFLT   *signal,*iprd,*iocts,*ifrqs,*iq,*ihann;
        MYFLT   *idbout,*idisprd,*idsines;
        int32_t     nfreqs, hanning, ncoefs, dbout, nsmps, scountdown;
        uint32_t timcount;
        MYFLT   curq, *sinp, *cosp, *linbufp;
        int32_t     disprd, dcountdown, winlen[MAXFRQS], offset[MAXFRQS];
        DOWNDAT downsig;
        WINDAT  sinwindow, octwindow;
        AUXCH   auxch1, auxch2;
} SPECTRUM;

#if 0
typedef struct {
        OPDS    h;
        SPECDAT *wsig;
        DOWNDAT *dsig;
        MYFLT   *iprd, *ifrqs, *iq, *ihann, *idbout, *idsines;
        int32_t     nfreqs, hanning, ncoefs, dbout;
        MYFLT   curq, *sinp, *cosp, *linbufp;
        int32_t     countdown, timcount, winlen[MAXFRQS];
        WINDAT  dwindow;
        AUXCH   auxch;
} NOCTDFT;
#endif

typedef struct {
        OPDS    h;
        SPECDAT *wsig;
        MYFLT   *iprd, *iwtflg;
        int32_t     countdown, timcount;
        WINDAT  dwindow;
} SPECDISP;

#define MAXPTL 10

typedef struct {
        OPDS    h;
        MYFLT   *koct, *kamp;
        SPECDAT *wsig;
        MYFLT   *kvar, *ilo, *ihi, *istrt, *idbthresh, *inptls, *irolloff;
        MYFLT   *iodd, *iconf, *interp, *ifprd, *iwtflg;
        int32_t     pdist[MAXPTL], nptls, rolloff, kinterp, ftimcnt;
        MYFLT   pmult[MAXPTL], confact, kvalsav, kval, kavl, kinc, kanc;
        MYFLT   *flop, *fhip, *fundp, *oct0p, threshon, threshoff;
        int32_t     winpts, jmpcount, playing;
        SPECDAT wfund;
        SPECDISP fdisplay;
} SPECPTRK;

typedef struct {
        OPDS    h;
        MYFLT   *ksum;
        SPECDAT *wsig;
        MYFLT   *interp;
        int32_t     kinterp;
        MYFLT   kval, kinc;
} SPECSUM;

typedef struct {
        OPDS    h;
        SPECDAT *waddm;
        SPECDAT *wsig1, *wsig2;
        MYFLT   *imul2;
        MYFLT   mul2;
} SPECADDM;

typedef struct {
        OPDS    h;
        SPECDAT *wdiff;
        SPECDAT *wsig;
        SPECDAT specsave;
} SPECDIFF;

typedef struct {
        OPDS    h;
        SPECDAT *wscaled;
        SPECDAT *wsig;
        MYFLT   *ifscale, *ifthresh;
        int32_t     thresh;
        MYFLT   *fscale, *fthresh;
        AUXCH   auxch;
} SPECSCAL;

typedef struct {
        OPDS    h;
        SPECDAT *wacout;
        SPECDAT *wsig;
        SPECDAT accumer;
} SPECHIST;

typedef struct {
        OPDS    h;
        SPECDAT *wfil;
        SPECDAT *wsig;
        MYFLT   *ifhtim;
        MYFLT   *coefs, *states;
        AUXCH   auxch;
} SPECFILT;

extern void DOWNset(CSOUND *, DOWNDAT *, int32);
extern void SPECset(CSOUND *, SPECDAT *, int32);

#endif

