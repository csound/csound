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
#include "uggab.h"

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

#define MAXPTL 10
typedef struct {
        OPDS    h;
        MYFLT   *koct, *kamp;
        MYFLT   *asig;
        MYFLT   *iprd, *ilo, *ihi, *idbthresh;
                                /* Optional */
        MYFLT   *ifrqs, *iconf, *istrt, *iocts, *iq, *inptls, *irolloff, *istor;
        double  c1, c2, prvq;
#define MAXFRQS 120
        SPECDAT wsig;
        int32_t     nfreqs, ncoefs, dbout, scountdown, timcount;
        MYFLT   curq, *sinp, *cosp, *linbufp;
        int32_t     winlen[MAXFRQS], offset[MAXFRQS];
        DOWNDAT downsig;
        WINDAT  sinwindow, octwindow;
        AUXCH   auxch1, auxch2;
        int32_t     pdist[MAXPTL], nptls, rolloff;
        MYFLT   pmult[MAXPTL], confact, kvalsav, kval, kavl, kinc, kanc;
        MYFLT   *flop, *fhip, *fundp, *oct0p, threshon, threshoff;
        int32_t     winpts, jmpcount, playing;
        SPECDAT wfund;
} PITCH;

typedef struct {
        OPDS    h;
        MYFLT   *cnt;
        void    *clk;
        int32_t     c;
} CLOCK;

typedef struct {
        OPDS    h;
        MYFLT   *r;
        MYFLT   *a;
        void    *clk;
} CLKRD;

typedef struct {
        OPDS    h;
        MYFLT   *val;
        MYFLT   *index;
} SCRATCHPAD;


typedef struct {
    OPDS    h;
    MYFLT   *sr, *kamp, *kcps, *ifn, *ifreqtbl, *iamptbl, *icnt, *iphs;
    FUNC    *ftp;
    FUNC    *freqtp;
    FUNC    *amptp;
    uint32_t     count;
    int32_t     inerr;
    AUXCH   lphs;
    int32_t floatph;
} ADSYNT;

typedef struct {
    OPDS        h;
    MYFLT       *sr, *kamp, *ktona, *kbrite, *ibasef, *ifn;
    MYFLT       *imixtbl, *ioctcnt, *iphs;
    int32       lphs[10];
    int32_t         octcnt;
    MYFLT       prevamp;
    FUNC        *ftp;
    FUNC        *mixtp;
} HSBOSC;

typedef struct {
    OPDS    h;
    MYFLT   *kcps, *krms, *asig, *imincps, *imaxcps, *icps,
            *imedi, *idowns, *iexcps, *irmsmedi;
    MYFLT   srate;
    MYFLT   lastval;
    int32   downsamp;
    int32   upsamp;
    int32   minperi;
    int32   maxperi;
    int32   index;
    int32   readp;
    int32   size;
    int32   peri;
    int32   medisize;
    int32   mediptr;
    int32   rmsmedisize;
    int32   rmsmediptr;
    int32_t     inerr;
    AUXCH   median;
    AUXCH   rmsmedian;
    AUXCH   buffer;
} PITCHAMDF;

typedef struct {
        OPDS    h;
        MYFLT   *sr, *xcps, *kindx, *icnt, *iphs;
        AUXCH   curphs;
} PHSORBNK;

/* pinkish opcode... Two methods for generating pink noise */

/* Gardner method space req */
#define GRD_MAX_RANDOM_ROWS   (32)

typedef struct {
    OPDS        h;
    MYFLT       *aout;
    MYFLT       *xin, *imethod, *iparam1, *iseed, *iskip;
    int32       ampinc;         /* Scale output to range */
    uint32      randSeed;     /* Used by local random generator */
                                /* for Paul Kellet's filter bank */
    double      b0, b1, b2, b3, b4, b5, b6;
                                /* for Gardner method */
    int32       grd_Rows[GRD_MAX_RANDOM_ROWS];
    int32       grd_NumRows;    /* Number of rows (octave bands of noise) */
    int32       grd_RunningSum; /* Used to optimize summing of generators. */
    int32_t         grd_Index;      /* Incremented each sample. */
    int32_t         grd_IndexMask;  /* Index wrapped by ANDing with this mask. */
    MYFLT       grd_Scalar;     /* Used to scale to normalize generated noise. */
} PINKISH;

typedef struct {
        OPDS    h;
        MYFLT   *aout;
        MYFLT   *ain, *imethod, *limit, *iarg;
        MYFLT   arg, lim, k1, k2;
        int32_t     meth;
} CLIP;

typedef struct {
        OPDS    h;
        MYFLT   *ar;
        MYFLT   *amp, *freq, *offset;
        uint32_t     next;
} IMPULSE;

typedef struct {
        int32   cnt,acnt;
        MYFLT   alpha;
        MYFLT   val, nxtpt;
        MYFLT   c1;
} NSEG;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *argums[VARGMAX];
        NSEG    *cursegp;
        int32   nsegs;
        int32   segsrem, curcnt;
        MYFLT   curval, curinc, alpha;
        MYFLT   curx;
        AUXCH   auxch;
        int32   xtra;
        MYFLT   finalval, lastalpha;
} TRANSEG;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *kamp, *beta;
        MYFLT   last, lastbeta, sq1mb2, ampmod;
        int32_t     ampinc;
} VARI;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *ain, *fco, *res, *dist, *istor;
        MYFLT   ay1, ay2, aout, lastin;
} LPF18;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *ain, *rep, *len;
        AUXCH   auxch;
        int32_t     length;         /* Length of buffer */
        int32_t     cnt;            /* Repetions of current cycle */
        int32_t     start;          /* Start of current cycle */
        int32_t     current;        /* takeout point */
        int32_t     direction;      /* Need to check direction of crossing */
        int32_t     end;            /* Insert point */
        MYFLT   lastsamp;       /* So we can test changes */
        int32_t     noinsert;       /* Flag to say we are losing input */
} BARRI;

typedef struct {
        OPDS    h;
        MYFLT   *sr, *xamp, *xcps, *ifn, *iphs;
        MYFLT   lphs;
        FUNC    *ftp;
} XOSC;



typedef struct {
        OPDS    h;
        MYFLT   *ans;
        MYFLT   *asig;
        MYFLT   *kwind;
        MYFLT   *imaxsize;
        MYFLT   *iskip;
        AUXCH   b;
        MYFLT   *buff;
        MYFLT   *med;
        int32_t     ind;
        int32_t     maxwind;
} MEDFILT;


extern void DOWNset(CSOUND *, DOWNDAT *, int32);
extern void SPECset(CSOUND *, SPECDAT *, int32);
int32_t Foscaa(CSOUND *, XOSC *p);
int32_t Foscak(CSOUND *, XOSC *p);
int32_t Foscka(CSOUND *, XOSC *p);
int32_t Fosckk(CSOUND *, XOSC *p);
int32_t Foscset(CSOUND *, XOSC *p);
int32_t GardnerPink_init(CSOUND *, PINKISH *p);
int32_t GardnerPink_perf(CSOUND *, PINKISH *p);
int32_t adsynt(CSOUND *, ADSYNT *p);
int32_t adsyntset(CSOUND *, ADSYNT *p);
int32_t clip(CSOUND *, CLIP *p);
int32_t clip_set(CSOUND *, CLIP *p);
int32_t clockoff(CSOUND *, CLOCK *p);
int32_t clockon(CSOUND *, CLOCK *p);
int32_t clockread(CSOUND *, CLKRD *p);
int32_t clockset(CSOUND *, CLOCK *p);
int32_t scratchread(CSOUND *, SCRATCHPAD *p);
int32_t scratchwrite(CSOUND *, SCRATCHPAD *p);

int32_t hsboscil(CSOUND *, HSBOSC *p);
int32_t hsboscset(CSOUND *, HSBOSC *p);
int32_t impulse(CSOUND *, IMPULSE *p);
int32_t impulse_set(CSOUND *, IMPULSE *p);

//int32_t totalcount(CSOUND *, INSTCNT *p);
int32_t kphsorbnk(CSOUND *, PHSORBNK *p);
int32_t ktrnseg(CSOUND *, TRANSEG *p);
int32_t ktrnsegr(CSOUND *csound, TRANSEG *p);
int32_t lpf18db(CSOUND *, LPF18 *p);
int32_t lpf18set(CSOUND *, LPF18 *p);
int32_t mac(CSOUND *, SUM *p);
int32_t maca(CSOUND *, SUM *p);
int32_t macset(CSOUND *, SUM *p);

int32_t phsbnkset(CSOUND *, PHSORBNK *p);
int32_t phsorbnk(CSOUND *, PHSORBNK *p);
int32_t pinkish(CSOUND *, PINKISH *p);
int32_t pinkset(CSOUND *, PINKISH *p);
int32_t pitch(CSOUND *, PITCH *p);
int32_t pitchamdf(CSOUND *, PITCHAMDF *p);
int32_t pitchamdfset(CSOUND *, PITCHAMDF *p);
int32_t pitchset(CSOUND *, PITCH *p);
int32_t trnseg(CSOUND *, TRANSEG *p);
int32_t trnsegr(CSOUND *csound, TRANSEG *p);
int32_t trnset(CSOUND *, TRANSEG *p);
int32_t trnset_bkpt(CSOUND *, TRANSEG *p);
int32_t trnsetr(CSOUND *csound, TRANSEG *p);
int32_t varicol(CSOUND *, VARI *p);
int32_t varicolset(CSOUND *, VARI *p);
int32_t waveset(CSOUND *, BARRI *p);
int32_t wavesetset(CSOUND *, BARRI *p);
int32_t medfiltset(CSOUND *, MEDFILT *p);
int32_t medfilt(CSOUND *, MEDFILT *p);
int32_t kmedfilt(CSOUND *, MEDFILT *p);

#endif

