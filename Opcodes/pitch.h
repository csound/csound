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
        double  c1, c2, prvq;
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
        MYFLT   *val;
        MYFLT   *index;
} SCRATCHPAD;

typedef struct {
        OPDS    h;
        MYFLT   *ins;
        MYFLT   *onoff;
} MUTE;

typedef struct {
        OPDS    h;
        MYFLT   *cnt;
        MYFLT   *ins;
        MYFLT   *opt;
        MYFLT   *norel;
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
    unsigned int     count;
    int     inerr;
    AUXCH   lphs;
} ADSYNT;

typedef struct {
    OPDS        h;
    MYFLT       *sr, *kamp, *ktona, *kbrite, *ibasef, *ifn;
    MYFLT       *imixtbl, *ioctcnt, *iphs;
    int32       lphs[10];
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
    int32       ampinc;         /* Scale output to range */
    uint32      randSeed;     /* Used by local random generator */
                                /* for Paul Kellet's filter bank */
    double      b0, b1, b2, b3, b4, b5, b6;
                                /* for Gardner method */
    int32       grd_Rows[GRD_MAX_RANDOM_ROWS];
    int32       grd_NumRows;    /* Number of rows (octave bands of noise) */
    int32       grd_RunningSum; /* Used to optimize summing of generators. */
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
        unsigned int     next;
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
        int     ampinc;
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

typedef struct {
        OPDS    h;
        MYFLT   *ans;
        MYFLT   *pnum;
        AUXCH   pfield;
} PFUNK;

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
        int     ind;
        int     maxwind;
} MEDFILT;

int Foscaa(CSOUND *, XOSC *p);
int Foscak(CSOUND *, XOSC *p);
int Foscka(CSOUND *, XOSC *p);
int Fosckk(CSOUND *, XOSC *p);
int Foscset(CSOUND *, XOSC *p);
int GardnerPink_init(CSOUND *, PINKISH *p);
int GardnerPink_perf(CSOUND *, PINKISH *p);
int adsynt(CSOUND *, ADSYNT *p);
int adsyntset(CSOUND *, ADSYNT *p);
int clip(CSOUND *, CLIP *p);
int clip_set(CSOUND *, CLIP *p);
int clockoff(CSOUND *, CLOCK *p);
int clockon(CSOUND *, CLOCK *p);
int clockread(CSOUND *, CLKRD *p);
int clockset(CSOUND *, CLOCK *p);
int scratchread(CSOUND *, SCRATCHPAD *p);
int scratchwrite(CSOUND *, SCRATCHPAD *p);
int cpuperc(CSOUND *, CPU_PERC *p);
int cpuperc_S(CSOUND *, CPU_PERC *p);
int hsboscil(CSOUND *, HSBOSC *p);
int hsboscset(CSOUND *, HSBOSC *p);
int impulse(CSOUND *, IMPULSE *p);
int impulse_set(CSOUND *, IMPULSE *p);
int instcount(CSOUND *, INSTCNT *p);
int instcount_S(CSOUND *, INSTCNT *p);
int totalcount(CSOUND *, INSTCNT *p);
int kphsorbnk(CSOUND *, PHSORBNK *p);
int ktrnseg(CSOUND *, TRANSEG *p);
int ktrnsegr(CSOUND *csound, TRANSEG *p);
int lpf18db(CSOUND *, LPF18 *p);
int lpf18set(CSOUND *, LPF18 *p);
int mac(CSOUND *, SUM *p);
int maca(CSOUND *, SUM *p);
int macset(CSOUND *, SUM *p);
int maxalloc(CSOUND *, CPU_PERC *p);
int mute_inst(CSOUND *, MUTE *p);
int maxalloc_S(CSOUND *, CPU_PERC *p);
int mute_inst_S(CSOUND *, MUTE *p);
int pfun(CSOUND *, PFUN *p);
int pfunk_init(CSOUND *, PFUNK *p);
int pfunk(CSOUND *, PFUNK *p);
int phsbnkset(CSOUND *, PHSORBNK *p);
int phsorbnk(CSOUND *, PHSORBNK *p);
int pinkish(CSOUND *, PINKISH *p);
int pinkset(CSOUND *, PINKISH *p);
int pitch(CSOUND *, PITCH *p);
int pitchamdf(CSOUND *, PITCHAMDF *p);
int pitchamdfset(CSOUND *, PITCHAMDF *p);
int pitchset(CSOUND *, PITCH *p);
int trnseg(CSOUND *, TRANSEG *p);
int trnsegr(CSOUND *csound, TRANSEG *p);
int trnset(CSOUND *, TRANSEG *p);
int trnset_bkpt(CSOUND *, TRANSEG *p);
int trnsetr(CSOUND *csound, TRANSEG *p);
int varicol(CSOUND *, VARI *p);
int varicolset(CSOUND *, VARI *p);
int waveset(CSOUND *, BARRI *p);
int wavesetset(CSOUND *, BARRI *p);
int medfiltset(CSOUND *, MEDFILT *p);
int medfilt(CSOUND *, MEDFILT *p);
int kmedfilt(CSOUND *, MEDFILT *p);
#endif /* PITCH_H */

