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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/********************************************/
/* wrap and mirror UGs by Gabriel Maldonado */
/********************************************/

typedef struct {
    OPDS  h;
    cs_float *xdest, *xsig, *xlow, *xhigh;
} WRAP;

typedef struct {
    OPDS  h;
    cs_float *kout, *ksig, *kthreshold, *kmode;
    cs_float old_sig;
} TRIG;

typedef struct {
    OPDS  h;
    cs_float *r, *val1, *val2, *point, *imin, *imax;
    cs_float point_factor;
} INTERPOL;

typedef struct {
    OPDS        h;
    cs_float       *ar, *argums[VARGMAX];
    AUXCH       aux;
} SUM;

typedef struct {
    OPDS        h;
    cs_float       *ar, *asig, *kcf, *kbw, *ord, *sep, *iflag, *iscl, *istor, *icorrect;
    int32_t     scale, loop;
    AUXCH       aux;
    AUXCH       buffer;
    cs_float       *yt1, *yt2;
} RESONY;

typedef struct {
    OPDS        h;
    cs_float       *ar, *asig, *kincr ;
    cs_double      index;
    cs_float       value;
} FOLD;

typedef struct {
        OPDS    h;
        cs_float   *out, *freq, *retrig, *iphase, *argums[VARGMAX];
        cs_float   args[VARGMAX];
        cs_double  phs;
        int32_t     nsegs;
} LOOPSEG;

/* Complexity of args leads to confusion */
typedef struct {
  cs_float *start;
  cs_float *type;
  cs_float *time;
} T3SEG;

typedef struct {
        OPDS    h;
        cs_float   *out, *freq, *retrig, *iphase;
        T3SEG   argums[VARGMAX/3];
        cs_double  phs;
        int32_t nsegs;
} LOOPTSEG;

typedef struct {
        OPDS    h;
        cs_float   *out, *kphase, *argums[VARGMAX];
        cs_float   args[VARGMAX];
        int32_t nsegs;
} LOOPSEGP;

typedef struct {  /* gab f1 */
        OPDS    h;
        cs_float   *kr, *ksig, *ktime;
        cs_float   current_val, incr, val_incremented;
        cs_double  remaining;
        int32_t flag;
} LINETO;

typedef struct {  /* gab f1 */
        OPDS    h;
        cs_float   *kr, *ksig, *ktime, *ktrig;
        cs_float   current_val, incr, val_incremented;
        cs_double  remaining;
        int32_t flag;
} LINETO2;

typedef struct {
        OPDS    h;
        cs_float   *out, *AverageAmp,*AverageFreq, *randAmountAmp, *randAmountFreq;
        cs_float   *ampMinRate, *ampMaxRate, *cpsMinRate, *cpsMaxRate, *ifn, *iphs;
        cs_float   xcpsAmpRate, xcpsFreqRate;
        cs_double  lphs, tablenUPkr;
        int32   tablen;
        uint32_t phsAmpRate, phsFreqRate;
        cs_float   num1amp, num2amp, num1freq, num2freq, dfdmaxAmp, dfdmaxFreq;
        FUNC    *ftp;
} VIBRATO;

typedef struct {
        OPDS    h;
        cs_float   *out, *AverageAmp,*AverageFreq,*ifn;
        cs_float   xcpsAmpRate, xcpsFreqRate;
        cs_double  lphs, tablenUPkr;
        int32   tablen;
        uint32_t phsAmpRate, phsFreqRate;
        cs_float   num1amp, num2amp, num1freq, num2freq, dfdmaxAmp, dfdmaxFreq;
        FUNC    *ftp;
} VIBR;

typedef struct {
        OPDS    h;
        cs_float   *out, *gamp, *amp1, *cps1, *amp2, *cps2, *amp3, *cps3, *option;
        uint32_t phs1,phs2,phs3;
        cs_float   num1a,num2a, dfdmax1, num1b,num2b, dfdmax2, num1c,num2c, dfdmax3;
} JITTER2;

typedef struct {
        OPDS    h;
        cs_float   *ar, *amp, *cpsMin, *cpsMax;
        cs_float   xcps;
        int32   phs;
        int32_t initflag;
        cs_float   num1, num2, dfdmax;
} JITTER;

typedef struct {
        OPDS    h;
        cs_float   *ar, *amp, *cpsMin, *cpsMax;
        cs_double  si;
        cs_double  phs;
        int32_t initflag, cod;
        cs_float   num0, num1, num2, df0, df1,c3, c2;
} JITTERS;

#define oneUp31Bit      (4.656612875245796924105750827168e-10)

static inline cs_float randGab(CSOUND *csound) {
  int32_t *holdrand = (int32_t *) csound->QueryGlobalVariable(csound, "::HOLDRAND::");
  uint32_t tmp = (uint32_t)*holdrand * 214013U + 2531011U;
  *holdrand = (int32_t)tmp;
  return (cs_float) ((cs_double)((tmp >> 1) & 0x7fffffff) * oneUp31Bit);
}

static inline cs_float BiRandGab(CSOUND *csound) {
  int32_t *holdrand = (int32_t *) csound->QueryGlobalVariable(csound, "::HOLDRAND::");
  uint32_t tmp = (uint32_t)*holdrand * (uint32_t)(-214013) + 2531011U;
  *holdrand = (int32_t)tmp;
  return (cs_float) ((cs_double)(int32_t)tmp * oneUp31Bit);
}


/* randGab can return 1, and cs_float rounding can reach the table length.
   Continuous lookup may return the guard point; discrete lookup must select
   a real table element. Callers supply a nonempty table and position >= 0. */
#define USER_RAND_LOOKUP(result, table, length, position, interpolate) do { \
  const cs_float *ur_table = (table);                                         \
  uint32_t ur_length = (length);                                          \
  cs_float ur_position = (position);                                         \
  if ((cs_double)ur_position >= (cs_double)ur_length)                            \
    (result) = ur_table[(interpolate) ? ur_length : ur_length - 1];         \
  else {                                                                 \
    uint32_t ur_index = (uint32_t)ur_position;                             \
    cs_float ur_value = ur_table[ur_index];                                  \
    if (interpolate)                                                      \
      ur_value += (ur_table[ur_index + 1] - ur_value) *                    \
                  (ur_position - ur_index);                              \
    (result) = ur_value;                                                  \
  }                                                                      \
} while (0)

typedef struct  {
        OPDS    h;
        cs_float   *out, *tableNum;
        int32_t pfn;
        FUNC    *ftp;
} DURAND;

typedef struct  {
        OPDS    h;
        cs_float   *out, *min, *max, *tableNum;
        int32_t pfn;
        FUNC    *ftp;
} CURAND;

typedef struct  {
        OPDS    h;
        cs_float   *out, *min, *max;
} RANGERAND;

/* mode and fstval arguments added */
/* by Francois Pinot, jan. 2011    */
typedef struct {
        OPDS    h;
        cs_float   *ar, *min, *max, *xcps, *mode, *fstval;
        int16   cpscod;
        uint32_t phs;
        cs_float   num1, num2, dfdmax;
} RANDOMI;

/* mode and fstval arguments added */
/* by Francois Pinot, jan. 2011    */
typedef struct {
        OPDS    h;
        cs_float   *ar, *min, *max, *xcps, *mode, *fstval;
        int16   cpscod;
        uint32_t phs;
        cs_float   num1;
} RANDOMH;

typedef struct {
        OPDS    h;
        cs_float   *ar, *rangeMin, *rangeMax, *cpsMin, *cpsMax;
        cs_double  si;
        cs_double  phs;
        int32_t initflag, rangeMin_cod, rangeMax_cod;
        cs_float   num0, num1, num2, df0, df1,c3, c2;
} RANDOM3;

#endif /* UGGAB_H */
