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
        int     c;
} CLOCK;

typedef struct {
        OPDS    h;
        MYFLT   *r;
        MYFLT   *a;
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

int adsynt(ADSYNT *p);
int adsyntset(ADSYNT *p);
int clip(CLIP *p);
int clip_set(CLIP *p);
int clockoff(CLOCK *p);
int clockon(CLOCK *p);
int clockread(CLKRD *p);
int clockset(CLOCK *p);
int cpuperc(CPU_PERC *p);
int Foscaa(XOSC *p);
int Foscak(XOSC *p);
int Foscka(XOSC *p);
int Fosckk(XOSC *p);
int Foscset(XOSC *p);
int GardnerPink_init(PINKISH *p);
int GardnerPink_perf(PINKISH *p);
int hsboscil(HSBOSC *p);
int hsboscset(HSBOSC *p);
int impulse(IMPULSE *p);
int impulse_set(IMPULSE *p);
int instcount(INSTCNT *p);
int isense(KSENSE *p);
int kphsorbnk(PHSORBNK *p);
int ksense(KSENSE *p);
int ktrnseg(TRANSEG *p);
int lpf18db(LPF18 *p);
int lpf18set(LPF18 *p);
int mac(SUM *p);
int maca(SUM *p);
int macset(SUM *p);
int maxalloc(CPU_PERC *p);
int mute_inst(MUTE *p);
int pfun(PFUN *p);
int phsbnkset(PHSORBNK *p);
int phsorbnk(PHSORBNK *p);
int pinkish(PINKISH *p);
int pinkset(PINKISH *p);
int pitch(PITCH *p);
int pitchamdf(PITCHAMDF *p);
int pitchamdfset(PITCHAMDF *p);
int pitchset(PITCH *p);
int prealloc(CPU_PERC *p);
int trnseg(TRANSEG *p);
int trnset(TRANSEG *p);
int varicol(VARI *p);
int varicolset(VARI *p);
int waveset(BARRI *p);
int wavesetset(BARRI *p);

#endif // PITCH_H
