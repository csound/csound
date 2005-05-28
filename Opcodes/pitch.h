#ifndef PITCH_H
#define PITCH_H
/*
    pitch.h:

    Copyright (C) 1999 John ffitch, Istvan Varga, Peter Neubäcker,
                       rasmus ekman, Phil Burk

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

                        /*                                      PITCH.H */
#include "spectra.h"
#include "uggab.h"

typedef struct {
        OPDS    h;
        MYFLT   *koct, *kamp;
        MYFLT   *asig;
        MYFLT   *iprd, *ilo, *ihi, *idbthresh;
                                /* Optional */
        MYFLT   *ifrqs, *iconf, *istrt, *iocts, *iq, *inptls, *irolloff, *istor;
        MYFLT   c1, c2, prvq;
#define MAXFRQS 120
        SPECDAT wsig;
        int     nfreqs, ncoefs, dbout, scountdown, timcount;
        MYFLT   curq, *sinp, *cosp, *linbufp;
        int     winlen[MAXFRQS], offset[MAXFRQS];
        DOWNDAT downsig;
        WINDAT  sinwindow, octwindow;
        AUXCH   auxch1, auxch2;
        int     pdist[MAXPTL], nptls, rolloff;
        MYFLT   pmult[MAXPTL], confact, kvalsav, kval, kavl, kinc, kanc;
        MYFLT   *flop, *fhip, *fundp, *oct0p, threshon, threshoff;
        int     winpts, jmpcount, playing;
        SPECDAT wfund;
} PITCH;

typedef struct {
        OPDS    h;
        MYFLT   *cnt;
        void    *clk;
        int     c;
} CLOCK;

typedef struct {
        OPDS    h;
        MYFLT   *r;
        MYFLT   *a;
        void    *clk;
} CLKRD;

typedef struct {
        OPDS    h;
        MYFLT   *ins;
        MYFLT   *onoff;
} MUTE;

typedef struct {
        OPDS    h;
        MYFLT   *cnt;
        MYFLT   *ins;
} INSTCNT;

typedef struct {
    OPDS        h;
    MYFLT       *instrnum, *ipercent, *iopc;    /* IV - Oct 31 2002 */
} CPU_PERC;

typedef struct {
    OPDS    h;
    MYFLT   *sr, *kamp, *kcps, *ifn, *ifreqtbl, *iamptbl, *icnt, *iphs;
    FUNC    *ftp;
    FUNC    *freqtp;
    FUNC    *amptp;
    int     count;
    int     inerr;
    AUXCH   lphs;
} ADSYNT;

typedef struct {
    OPDS        h;
    MYFLT       *sr, *kamp, *ktona, *kbrite, *ibasef, *ifn;
    MYFLT       *imixtbl, *ioctcnt, *iphs;
    long        lphs[10];
    int         octcnt;
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
    long    downsamp;
    long    upsamp;
    long    minperi;
    long    maxperi;
    long    index;
    long    readp;
    long    size;
    long    peri;
    long    medisize;
    long    mediptr;
    long    rmsmedisize;
    long    rmsmediptr;
    int     inerr;
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
    long        ampinc;         /* Scale output to range */
    unsigned long randSeed;     /* Used by local random generator */
                                /* for Paul Kellet's filter bank */
    double      b0, b1, b2, b3, b4, b5, b6;
                                /* for Gardner method */
    long        grd_Rows[GRD_MAX_RANDOM_ROWS];
    long        grd_NumRows;    /* Number of rows (octave bands of noise) */
    long        grd_RunningSum; /* Used to optimize summing of generators. */
    int         grd_Index;      /* Incremented each sample. */
    int         grd_IndexMask;  /* Index wrapped by ANDing with this mask. */
    MYFLT       grd_Scalar;     /* Used to scale to normalize generated noise. */
} PINKISH;

typedef struct {
        OPDS    h;
        MYFLT   *aout;
        MYFLT   *ain, *imethod, *limit, *iarg;
        MYFLT   arg, lim, k1, k2;
        int     meth;
} CLIP;

typedef struct {
        OPDS    h;
        MYFLT   *ar;
        MYFLT   *amp, *freq, *offset;
        int     next;
} IMPULSE;

typedef struct {
        OPDS    h;
        MYFLT   *ans;
} KSENSE;

typedef struct {
        long    cnt;
        MYFLT   alpha;
        MYFLT   val, nxtpt;
        MYFLT   c1;
} NSEG;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *argums[VARGMAX];
        NSEG    *cursegp;
        long    nsegs;
        long    segsrem, curcnt;
        MYFLT   curval, curinc, alpha;
        MYFLT   curx;
        AUXCH   auxch;
        long    xtra;
} TRANSEG;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *kamp, *beta;
        MYFLT   last, lastbeta, sq1mb2, ampmod;
        int     ampinc;
} VARI;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *ain, *fco, *res, *dist;
        MYFLT   ay1, ay2, aout, lastin;
} LPF18;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *ain, *rep, *len;
        AUXCH   auxch;
        int     length;         /* Length of buffer */
        int     cnt;            /* Repetions of current cycle */
        int     start;          /* Start of current cycle */
        int     current;        /* takeout point */
        int     direction;      /* Need to check direction of crossing */
        int     end;            /* Insert point */
        MYFLT   lastsamp;       /* So we can test changes */
        int     noinsert;       /* Flag to say we are losing input */
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
        MYFLT   *pnum;
} PFUN;

int adsynt(ENVIRON*,ADSYNT *p);
int adsyntset(ENVIRON*,ADSYNT *p);
int clip(ENVIRON*,CLIP *p);
int clip_set(ENVIRON*,CLIP *p);
int clockoff(ENVIRON*,CLOCK *p);
int clockon(ENVIRON*,CLOCK *p);
int clockread(ENVIRON*,CLKRD *p);
int clockset(ENVIRON*,CLOCK *p);
int cpuperc(ENVIRON*,CPU_PERC *p);
int Foscaa(ENVIRON*,XOSC *p);
int Foscak(ENVIRON*,XOSC *p);
int Foscka(ENVIRON*,XOSC *p);
int Fosckk(ENVIRON*,XOSC *p);
int Foscset(ENVIRON*,XOSC *p);
int GardnerPink_init(ENVIRON*,PINKISH *p);
int GardnerPink_perf(ENVIRON*,PINKISH *p);
int hsboscil(ENVIRON*,HSBOSC *p);
int hsboscset(ENVIRON*,HSBOSC *p);
int impulse(ENVIRON*,IMPULSE *p);
int impulse_set(ENVIRON*,IMPULSE *p);
int instcount(ENVIRON*,INSTCNT *p);
int isense(ENVIRON*,KSENSE *p);
int kphsorbnk(ENVIRON*,PHSORBNK *p);
int ksense(ENVIRON*,KSENSE *p);
int ktrnseg(ENVIRON*,TRANSEG *p);
int lpf18db(ENVIRON*,LPF18 *p);
int lpf18set(ENVIRON*,LPF18 *p);
int mac(ENVIRON*,SUM *p);
int maca(ENVIRON*,SUM *p);
int macset(ENVIRON*,SUM *p);
int maxalloc(ENVIRON*,CPU_PERC *p);
int mute_inst(ENVIRON*,MUTE *p);
int pfun(ENVIRON*,PFUN *p);
int phsbnkset(ENVIRON*,PHSORBNK *p);
int phsorbnk(ENVIRON*,PHSORBNK *p);
int pinkish(ENVIRON*,PINKISH *p);
int pinkset(ENVIRON*,PINKISH *p);
int pitch(ENVIRON*,PITCH *p);
int pitchamdf(ENVIRON*,PITCHAMDF *p);
int pitchamdfset(ENVIRON*,PITCHAMDF *p);
int pitchset(ENVIRON*,PITCH *p);
int trnseg(ENVIRON*,TRANSEG *p);
int trnset(ENVIRON*,TRANSEG *p);
int varicol(ENVIRON*,VARI *p);
int varicolset(ENVIRON*,VARI *p);
int waveset(ENVIRON*,BARRI *p);
int wavesetset(ENVIRON*,BARRI *p);

#endif /* PITCH_H */
