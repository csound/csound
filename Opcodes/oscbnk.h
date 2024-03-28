/*
    oscbnk.h:

    Copyright (C) 2002, 2005 Istvan Varga

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

#ifndef CSOUND_OSCBNK_H
#define CSOUND_OSCBNK_H

#include "stdopcod.h"

/*
#ifdef  B64BIT
#define MAXLEN     0x40000000
#define FMAXLEN    ((MYFLT)(MAXLEN))
#define PHMASK     0x3fffffff
#else
#define MAXLEN     0x1000000L
#define FMAXLEN    ((MYFLT)(MAXLEN))
#define PHMASK     0x0FFFFFFL
#endif
*/


/* ---- oscbnk, grain2, and grain3 - written by Istvan Varga, 2001 ---- */

#define OSCBNK_PHSMAX   0x80000000UL    /* max. phase   */
#define OSCBNK_PHSMSK   0x7FFFFFFFUL    /* phase mask   */
#define OSCBNK_RNDPHS   0               /* 31 bit rand -> phase bit shift */

/* convert floating point phase value to integer */

#define OSCBNK_PHS2INT(x)                                                     \
    ((uint32) MYFLT2LRND((x) * (MYFLT) OSCBNK_PHSMAX) & OSCBNK_PHSMSK)

/* oscbnk types */

typedef struct {
        uint32  LFO1phs;                /* LFO 1 phase                  */
        MYFLT   LFO1frq;                /* LFO 1 frequency (0-1)        */
        uint32  LFO2phs;                /* LFO 2 phase                  */
        MYFLT   LFO2frq;                /* LFO 2 frequency (0-1)        */
        uint32  osc_phs;                /* main oscillator phase        */
        MYFLT   osc_phm;                /* phase mod.                   */
        MYFLT   osc_frq, osc_amp;       /* osc. freq. / sr, amplitude   */
        MYFLT   xnm1, xnm2, ynm1, ynm2; /* EQ tmp data                  */
        MYFLT   a1, a2, b0, b1, b2;     /* EQ coeffs saved for interp.  */
        double  osc_phsf, LFO1phsf, LFO2phsf;
} OSCBNK_OSC;

typedef struct {
        OPDS    h;
        MYFLT   *args[27];              /* opcode args (see manual)     */
        int32_t     init_k;                 /* 1st k-cycle (0: no, 1: yes)  */
        int32_t     nr_osc;                 /* number of oscillators        */
        int32   seed;                   /* random seed                  */
        int32_t     ilfomode, ieqmode;      /* LFO and EQ mode              */
        int32_t     eq_interp;              /* enable filter coeff. interp. */
        MYFLT   frq_scl;                /* constants for calculating    */
        MYFLT   lf1_scl, lf1_ofs;       /* k-rate parameters            */
        MYFLT   lf2_scl, lf2_ofs;
        MYFLT   eqo_scl, eqo_ofs;
        MYFLT   eql_scl, eql_ofs;
        MYFLT   eqq_scl, eqq_ofs;
        MYFLT   *l1t, l1t_pfrac;        /* LFO 1 ftable                 */
        uint32   l1t_mask, l1t_lobits;
        MYFLT   *l2t, l2t_pfrac;        /* LFO 2 ftable                 */
        uint32   l2t_mask, l2t_lobits;
        MYFLT   *eqft;                  /* EQ frequency table           */
        int32    eqft_len;
        MYFLT   *eqlt;                  /* EQ level table               */
        int32    eqlt_len;
        MYFLT   *eqqt;                  /* EQ Q table                   */
        int32    eqqt_len;
        MYFLT   *tabl;                  /* parameter input table        */
        int32    tabl_len;               /* (optional)                   */
        MYFLT   *outft;                 /* parameter output table       */
        int32    outft_len;              /* (optional)                   */
        int32    tabl_cnt;               /* current param in table       */
        int32    floatph, flen1, flen2;
        AUXCH   auxdata;
        OSCBNK_OSC      *osc;           /* oscillator array             */
} OSCBNK;

/* grain2 types */

typedef struct {
        uint32   grain_phs;      /* grain phase                  */
        uint32   grain_frq_int;  /* grain frequency (integer)    */
        MYFLT           grain_frq_flt;  /* grain frequency (float)      */
        uint32   window_phs;     /* window phase                 */
        MYFLT    grain_frq, grain_phsf, window_phsf; 
} GRAIN2_OSC;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *kcps, *kfmd;      /* opcode args                  */
        MYFLT   *kgdur, *iovrlp;
        MYFLT   *kfn, *iwfn, *irpow;
        MYFLT   *iseed, *imode;
        int32_t     init_k;                 /* 1st k-cycle (0: no, 1: yes)  */
        int32_t     mode;                   /* imode (see manual)           */
        int32_t     nr_osc;                 /* number of oscillators        */
        int32    seed;                   /* random seed                  */
        int32_t     rnd_mode;               /* random distribution params   */
        MYFLT   rnd_pow;
        MYFLT   grain_frq, frq_scl;     /* grain frequency              */
        MYFLT   *wft, wft_pfrac;        /* window table                 */
        uint32   wft_lobits, wft_mask;
        int32   floatph, wflen;
        AUXCH   auxdata;
        GRAIN2_OSC      *osc;           /* oscillator array             */
} GRAIN2;

/* -------- grain3 types -------- */

typedef struct {
        OPDS    h;
        MYFLT   *ar, *kcps, *kphs;      /* opcode args                  */
        MYFLT   *kfmd, *kpmd;
        MYFLT   *kgdur, *kdens;
        MYFLT   *imaxovr, *kfn, *iwfn;
        MYFLT   *kfrpow, *kprpow;
        MYFLT   *iseed, *imode;
        int32_t     init_k;                 /* 1st k-cycle (0: no, 1: yes)  */
        int32_t     mode;                   /* imode (see manual)           */
        int32_t     ovrlap;                 /* max. number of oscillators   */
        int32    seed;                   /* random seed                  */
        int32_t     f_rnd_mode;             /* random distribution (freq.)  */
        MYFLT   f_rnd_pow;
        int32_t     p_rnd_mode, pm_wrap;    /* random distribution (phase)  */
        MYFLT   p_rnd_pow;
        uint32   grain_frq;      /* grain frequency              */
         MYFLT   frq_scl, grain_frqf;
        MYFLT   phs0;                   /* prev. kphs value for interp. */
        uint32   x_phs;
        MYFLT   *wft, wft_pfrac;        /* window table                 */
        uint32   wft_lobits, wft_mask;
  int32   wflen, floatph;
        double  x_phsf, *phasef;
        AUXCH   auxdata;
        uint32   *phase;         /* grain phase offset           */
        GRAIN2_OSC      *osc;           /* oscillator array             */
        GRAIN2_OSC      *osc_start;     /* first active grain           */
        GRAIN2_OSC      *osc_end;       /* last active grain + 1        */
        GRAIN2_OSC      *osc_max;       /* ptr to last osc in array     */
} GRAIN3;

/* -------- rnd31 types -------- */

typedef struct {
        OPDS    h;              /* opcode args          */
        MYFLT   *out;                   /* output signal                */
        MYFLT   *scl;                   /* scale                        */
        MYFLT   *rpow;                  /* distribution                 */
        MYFLT   *iseed;                 /* seed                         */
                                /* internal variables   */
        int32    *rnd31i_seed;           /* global seed for rnd31        */
        int32    seed;                   /* random seed                  */
} RND31;

/* -------- oscilikt types -------- */

typedef struct {
        OPDS    h;
        MYFLT   *sr, *xamp, *xcps, *kfn, *iphs, *istor;
        uint32    phs, lobits, mask;
  MYFLT   pfrac, *ft, oldfn, phsf;
  int32 flen, floatph;
} OSCKT;

typedef struct {
        OPDS    h;
        MYFLT   *sr, *xcps, *kfn, *kphs, *istor;
        uint32    phs, lobits, mask;
  MYFLT   pfrac, *ft, oldfn, old_phs, phsf;
        int32_t     init_k;
          int32 flen, floatph;
} OSCKTP;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *xamp, *xcps, *kfn, *async, *kphs, *istor;
        uint32    phs, lobits, mask;
  MYFLT   pfrac, *ft, oldfn, phsf;
        int32_t     init_k;
            int32 flen, floatph;
} OSCKTS;

/* ---- vco2init, vco2ft, and vco2 opcodes by Istvan Varga, Sep 2002 ---- */

/* Select algorithm to be used for finding table numbers */
/* Define this macro to use simple table lookup (slower  */
/* at high control rate, due to float->int32_t cast), or     */
/* comment it out to use a search algorithm (slower with */
/* very fast changes in frequency)                       */

#define VCO2FT_USE_TABLE    1

typedef struct {
    int32_t     npart;              /* number of harmonic partials (may be zero) */
    int32_t     size;               /* size of the table (not incl. guard point) */
    uint32               /* parameters needed for reading the table,  */
            lobits, mask;       /*   and interpolation                       */
    MYFLT   pfrac;
    MYFLT   *ftable;            /* table data (size + 1 floats)              */
} VCO2_TABLE;

struct VCO2_TABLE_ARRAY_ {
    int32_t     ntabl;              /* number of tables                          */
    int32_t     base_ftnum;         /* base ftable number (-1: none)             */
#ifdef VCO2FT_USE_TABLE
    VCO2_TABLE  **nparts_tabl;  /* table ptrs for all numbers of partials    */
#else
    MYFLT   *nparts;            /* number of partials list                   */
#endif
    VCO2_TABLE  *tables;        /* array of table structures                 */
};

typedef struct {
    OPDS    h;
    MYFLT   *ift, *iwaveforms, *iftnum, *ipmul, *iminsiz, *imaxsiz, *isrcft;
} VCO2INIT;

typedef struct {
    OPDS    h;
    MYFLT   *ar, *kamp, *kcps, *imode, *kpw, *kphs, *inyx;
    MYFLT   *dummy[9];
#ifdef VCO2FT_USE_TABLE
    VCO2_TABLE  **nparts_tabl;  /* table ptrs for all numbers of partials    */
#endif
    int32_t     init_k;             /* 1 in first k-cycle, 0 otherwise           */
    int32_t     mode;               /* algorithm (0, 1, or 2)                    */
    int32_t     pm_enabled;         /* phase modulation enabled (0: no, 1: yes)  */
#ifdef VCO2FT_USE_TABLE
    MYFLT   f_scl, p_min, p_scl, kphs_old, kphs2_old;
#else
    MYFLT   f_scl, p_min, p_scl, *npart_old, *nparts, kphs_old, kphs2_old;
    VCO2_TABLE  *tables;        /* pointer to array of tables                */
#endif
    uint32  phs, phs2;  /* oscillator phase                          */
    VCO2_TABLE_ARRAY  ***vco2_tables;
    int32_t             *vco2_nr_table_arrays;
} VCO2;

typedef struct {
    OPDS    h;
    MYFLT   *kft, *kcps, *iwave, *inyx;
    MYFLT   p_min, p_scl;
#ifdef VCO2FT_USE_TABLE
    VCO2_TABLE  **nparts_tabl, *tab0;
#else
    MYFLT   *npart_old, *nparts;
#endif
    VCO2_TABLE_ARRAY    ***vco2_tables;
    int32_t                 *vco2_nr_table_arrays;
    int32_t                 base_ftnum;
} VCO2FT;

typedef struct {                /* denorm a1[, a2[, a3[, ... ]]] */
    OPDS    h;
    MYFLT   *ar[256];
    int32_t     *seedptr;
} DENORMS;

typedef struct {                /* kr delayk ksig, idel[, imode] */
    OPDS    h;
    MYFLT   *ar, *ksig, *idel, *imode;
    int32_t     npts, init_k, readp, mode;
    AUXCH   aux;
} DELAYK;

typedef struct {                /* kr vdel_k ksig, kdel, imdel[, imode] */
    OPDS    h;
    MYFLT   *ar, *ksig, *kdel, *imdel, *imode;
    int32_t     npts, init_k, wrtp, mode;
    MYFLT   frstkval;
    AUXCH   aux;
} VDELAYK;

typedef struct {                /* ar rbjeq asig, kfco, klvl, kQ, kS[, imode] */
        OPDS    h;
        MYFLT   *ar, *asig, *kcps, *klvl, *kQ, *kS, *imode;     /* args */
        /* internal variables */
        MYFLT   old_kcps, old_klvl, old_kQ, old_kS;
        double  omega, cs, sn;
        MYFLT   xnm1, xnm2, ynm1, ynm2;
        MYFLT   b0, b1, b2, a1, a2;
        int32_t
        ftype;
} RBJEQ;

#endif          /* CSOUND_OSCBNK_H */

