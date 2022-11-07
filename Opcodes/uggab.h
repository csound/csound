#ifndef UGGAB_H
#define UGGAB_H
/*
    uggab.h:

    Copyright (C) 1998 Gabriel Maldonado, John ffitch

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

/********************************************/
/* wrap and mirror UGs by Gabriel Maldonado */
/********************************************/

typedef struct {
    OPDS  h;
    MYFLT *xdest, *xsig, *xlow, *xhigh;
} WRAP;

typedef struct {
    OPDS  h;
    MYFLT *kout, *ksig, *kthreshold, *kmode;
    MYFLT old_sig;
} TRIG;

typedef struct {
    OPDS  h;
    MYFLT *r, *val1, *val2, *point, *imin, *imax;
    MYFLT point_factor;
} INTERPOL;

typedef struct  {
    OPDS        h;
    MYFLT       *out, *amp, *freq, *ift, *iphs;
    FUNC        *ftp;
    int32       tablen;
    double      tablenUPsr;
    double      phs;
} POSC;

typedef struct  {
    OPDS        h;
    MYFLT       *out, *amp, *freq, *kloop, *kend, *ift, *iphs;
    FUNC        *ftp;
    int32        tablen;
    MYFLT       fsr;
    double      phs, looplength;
} LPOSC;

typedef struct {
    OPDS        h;
    MYFLT       *ar, *argums[VARGMAX];
    AUXCH       aux;
} SUM;

typedef struct {
    OPDS        h;
    MYFLT       *ar, *asig, *kcf, *kbw, *ord, *sep, *iflag, *iscl, *istor;
    int32_t     scale, loop;
    AUXCH       aux;
    AUXCH       buffer;
    MYFLT       *yt1, *yt2;
} RESONY;

typedef struct {
    OPDS        h;
    MYFLT       *ar, *asig, *kincr ;
    double      index;
    int32       sample_index;
    MYFLT       value;
} FOLD;

typedef struct {
        OPDS    h;
        MYFLT   *out, *freq, *retrig, *iphase, *argums[VARGMAX];
        MYFLT   args[VARGMAX];
        double  phs;
        int32_t     nsegs;
} LOOPSEG;

/* Complexity of args leads to confusion */
typedef struct {
  MYFLT *start;
  MYFLT *type;
  MYFLT *time;
} T3SEG;

typedef struct {
        OPDS    h;
        MYFLT   *out, *freq, *retrig, *iphase;
        T3SEG   argums[VARGMAX/3];
        double  phs;
        int32_t nsegs;
} LOOPTSEG;

typedef struct {
        OPDS    h;
        MYFLT   *out, *kphase, *argums[VARGMAX];
        MYFLT   args[VARGMAX];
        int32_t nsegs;
} LOOPSEGP;

typedef struct {  /* gab f1 */
        OPDS    h;
        MYFLT   *kr, *ksig, *ktime;
        MYFLT   current_val, current_time, incr, val_incremented, old_time;
        int32_t flag;
} LINETO;

typedef struct {  /* gab f1 */
        OPDS    h;
        MYFLT   *kr, *ksig, *ktime, *ktrig;
        MYFLT   current_val, current_time, incr, val_incremented, old_time;
        int32_t flag;
} LINETO2;

typedef struct {
        OPDS    h;
        MYFLT   *out, *AverageAmp,*AverageFreq, *randAmountAmp, *randAmountFreq;
        MYFLT   *ampMinRate, *ampMaxRate, *cpsMinRate, *cpsMaxRate, *ifn, *iphs;
        MYFLT   xcpsAmpRate, xcpsFreqRate;
        double  lphs, tablenUPkr;
        int32   tablen, phsAmpRate, phsFreqRate;
        MYFLT   num1amp, num2amp, num1freq, num2freq, dfdmaxAmp, dfdmaxFreq;
        FUNC    *ftp;
} VIBRATO;

typedef struct {
        OPDS    h;
        MYFLT   *out, *AverageAmp,*AverageFreq,*ifn;
        MYFLT   xcpsAmpRate, xcpsFreqRate;
        double  lphs, tablenUPkr;
        int32   tablen, phsAmpRate, phsFreqRate;
        MYFLT   num1amp, num2amp, num1freq, num2freq, dfdmaxAmp, dfdmaxFreq;
        FUNC    *ftp;
} VIBR;

typedef struct {
        OPDS    h;
        MYFLT   *out, *gamp, *amp1, *cps1, *amp2, *cps2, *amp3, *cps3, *option;
        int32_t flag;
        int32   phs1,phs2,phs3;
        MYFLT   num1a,num2a, dfdmax1, num1b,num2b, dfdmax2, num1c,num2c, dfdmax3;
} JITTER2;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *amp, *cpsMin, *cpsMax;
        MYFLT   xcps;
        int32   phs;
        int32_t initflag;
        MYFLT   num1, num2, dfdmax;
} JITTER;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *amp, *cpsMin, *cpsMax;
        double  si;
        double  phs;
        int32_t initflag, cod;
        MYFLT   num0, num1, num2, df0, df1,c3, c2;
} JITTERS;

#define oneUp31Bit      (4.656612875245796924105750827168e-10)

#define randGab   (MYFLT) ((double)                                       \
        (((csound->holdrand = csound->holdrand * 214013 + 2531011) >> 1)  \
         & 0x7fffffff) * oneUp31Bit)
#define BiRandGab (MYFLT) ((double)                                       \
        (csound->holdrand = csound->holdrand * -214013 + 2531011) * oneUp31Bit)

typedef struct  {
        OPDS    h;
        MYFLT   *out, *tableNum;
        int32_t pfn;
        FUNC    *ftp;
} DURAND;

typedef struct  {
        OPDS    h;
        MYFLT   *out, *min, *max, *tableNum;
        int32_t pfn;
        FUNC    *ftp;
} CURAND;

typedef struct  {
        OPDS    h;
        MYFLT   *out, *min, *max;
} RANGERAND;

/* mode and fstval arguments added */
/* by Francois Pinot, jan. 2011    */
typedef struct {
        OPDS    h;
        MYFLT   *ar, *min, *max, *xcps, *mode, *fstval;
        int16   cpscod;
        int32   phs;
        MYFLT   num1, num2, dfdmax;
} RANDOMI;

/* mode and fstval arguments added */
/* by Francois Pinot, jan. 2011    */
typedef struct {
        OPDS    h;
        MYFLT   *ar, *min, *max, *xcps, *mode, *fstval;
        int16   cpscod;
        int32   phs;
        MYFLT   num1;
} RANDOMH;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *rangeMin, *rangeMax, *cpsMin, *cpsMax;
        double  si;
        double  phs;
        int32_t initflag, rangeMin_cod, rangeMax_cod;
        MYFLT   num0, num1, num2, df0, df1,c3, c2;
} RANDOM3;

#endif /* UGGAB_H */
