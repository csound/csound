/*

   beosc.c

   beosc - Bandwidth enhanced oscillator
   beadsynt - Bandwidth enhanced oscillator bank for additive synthesis

   (C) 2017 Eduardo Moguillansky

   The beosc library is free software; you can redistribute it
   and/or modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the gab library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA
   beosc and beadsynt are based on the algorithm implemented by
   Loris.

   Loris is Copyright (c) 1999-2016 by Kelly Fitz and Lippold Haken

*/

/*

  Overview

  opcodes:

  beosc: band-enhanced oscillator
  beadsynt: band-enhanced additive synthesis (a bank of beosc)
  tabrowlin: copy a row of a 2D-table to another table or to an array
             (possibly selecting a slice and interpolating between two rows)
  getrowlin: copy a row of a 2D-array to another array
             (possibly selecting a slice and interpolating between two rows)


*/

#include "csdl.h"
#include "arrays.h"
#include "emugens_common.h"

// -------------------------------------------------------------------------

#define UInt32toFlt(x) ((double)(x) * (1.0 / 4294967295.03125))

#define unirand(c) ((MYFLT) UInt32toFlt(csoundRandMT(&((c)->randState_))))

#define unirand2(cs,seed) ((MYFLT) (cs->Rand31(&seed)-1) / FL(2147483645.0))

// 1 / 2pi
#define RTWOPI 0.1591549430918953357688837634

#define SAMPLE_ACCURATE \
    uint32_t n, nsmps = CS_KSMPS;                                    \
    MYFLT *out = p->out;                                             \
    uint32_t offset = p->h.insdshead->ksmps_offset;                  \
    uint32_t early  = p->h.insdshead->ksmps_no_end;                  \
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));   \
    if (UNLIKELY(early)) {                                           \
        nsmps -= early;                                              \
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));              \
    }                                                                \


/*

  Helpers

*/

static inline float
fastlog2 (float x) {
    union { float f; uint32_t i; } vx = { x };
    union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
    float y = vx.i;
    y *= 1.1920928955078125e-7f;

    return y - 124.22551499f
           - 1.498030302f * mx.f
           - 1.72587999f / (0.3520887068f + mx.f);
}

#ifndef USE_DOUBLE
static inline float
fastlogf (float x) {
    return 0.69314718f * fastlog2 (x);
}
#endif

static inline MYFLT
fastlog(MYFLT x) {
    return FL(0.6931471805599453) * fastlog2(x);
}


// uniform noise, taken from csoundRand31, returns floats between 0-1
static inline MYFLT
FastRandFloat(uint32_t *seedptr) {
    uint64_t tmp1;
    uint32_t tmp2;
    /* x = (742938285 * x) % 0x7FFFFFFF */
    tmp1  = (uint64_t) ((int32_t) (*seedptr) * (int64_t) 742938285);
    tmp2  = (uint32_t) tmp1 & (uint32_t) 0x7FFFFFFF;
    tmp2 += (uint32_t) (tmp1 >> 31);
    tmp2  = (tmp2 & (uint32_t) 0x7FFFFFFF) + (tmp2 >> 31);
    (*seedptr) = tmp2;
    return (MYFLT)(tmp2 - 1) / FL(2147483648.0);
}


/*

  Gaussian Noise

  gaussian_normal: the canonical implementation

  gaussian_normal is used to calculate a table of gaussian numbers
  which is then accessed using uniform noise. This renders an acceptable
  result for our purpose, while reducing the performance cost to simple
  uniform noise, without any logs or sqrts

*/

typedef struct {
    MYFLT gset;
    int iset;
    uint32_t seed;
} GaussianState;

static inline MYFLT
gaussian_normal(GaussianState *gs) {
    if(gs->iset) {
      gs->iset = 0;
      return gs->gset;
    }
    gs->iset = 1;
    MYFLT v1 = FL(2.0) * FastRandFloat(&(gs->seed)) - FL(1.0);
    MYFLT v2 = FL(2.0) * FastRandFloat(&(gs->seed)) - FL(1.0);
    MYFLT r  = v1*v1 + v2*v2;
    while(r >= 1.0) {
      v1 = v2;
      v2 = FL(2.0) * FastRandFloat(&(gs->seed)) - FL(1.0);
      r  = v1*v1 + v2*v2;
    }
    MYFLT fac = r == FL(0) ? FL(0) : sqrt(FL(-2) * fastlog(r)/r);
    gs->gset = v1*fac;
    return v2*fac;
}

static MYFLT* gaussians = NULL;

#define GAUSSIANS_SIZE 65536

static void
gaussians_init(uint32_t seed) {
    GaussianState gs;
    if(gaussians == NULL) {
      uint32_t size = GAUSSIANS_SIZE;
      uint32_t i;
      gs.gset = 0;
      gs.iset = 0;
      gs.seed = seed;
      MYFLT *g = malloc(sizeof(MYFLT)*size);
      for(i=0; i<size; i++) {
        g[i] = gaussian_normal(&gs);
      }
      gaussians = g;
    }
}

#define GAUSSIANS_GET(seed) \
  (gaussians[(uint32_t)(FastRandFloat(seed)*(GAUSSIANS_SIZE-1))])


/*

   Integer phase oscillator with/without interpolation,
   adapted from Supercollider. Not used now, included as
   a reference

 */

#define xlobits 14
#define xlobits1 13

static inline float
PhaseFrac(uint32_t inPhase) {
    union { uint32_t itemp; float ftemp; } u;
    u.itemp = 0x3F800000 | (0x007FFF80 & ((inPhase)<<7));
    return u.ftemp - 1.f;
}

#ifndef USE_DOUBLE
static inline float
PhaseFrac1(uint32_t inPhase) {
    union { uint32_t itemp; float ftemp; } u;
    u.itemp = 0x3F800000 | (0x007FFF80 & ((inPhase)<<7));
    return u.ftemp;
}
#endif

static inline MYFLT
lookupi1(const MYFLT* table0, const MYFLT* table1,
         int32_t pphase, int32_t lomask) {
    MYFLT pfrac    = PhaseFrac(pphase);
    uint32_t index = ((pphase >> xlobits1) & lomask);
    MYFLT val1 = *(const MYFLT*)((const char*)table0 + index);
    MYFLT val2 = *(const MYFLT*)((const char*)table1 + index);
    MYFLT out  = val1 + (val2 - val1) * pfrac;
    return out;
}

static inline MYFLT
lookup(const MYFLT *table, int32_t phase, int32_t mask) {
    uint32_t index = ((phase >> xlobits1) & mask);
    return *(const MYFLT*)((const char*)table + index);
}

static inline MYFLT
cs_lookupi(const MYFLT* ftbl, int32_t phs, int32_t lobits, int32_t lomask,
           MYFLT lodiv) {
    MYFLT fract = (MYFLT)((phs & lomask) * lodiv);
    const MYFLT* ftbl0 = ftbl + (phs >> lobits);
    MYFLT v1 = ftbl0[0];
    return v1 + (ftbl0[1] - v1)*fract;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*

    beosc

    Band-Enhanced oscil, adapted from the supercollider port (BEOsc)
    Loris original implementation uses gaussian normal noise,
    the supercollider port uses uniform noise. I included
    both implementations. Normal noise is generally more expensive, but this
    implementation is efficient, using uniform noise reshaped
    with a gaussian distribution

    aout beosc xfreq, kbw [, ifn=-1, iphase=0, iflags=1 ]

    xfreq: freq. of the oscillator
    kbw: bandwidth of the oscillator (0=pure, 1=noise)
    ifn: like oscil, default is -1, a sine wave.
    iphase: like oscil, a float value between 0 - 2pi (default=0)
    iflags: 0-1 = uniform or gaussian noise (default=gaussian)
            +2  = table lookup with linear interpolation

 */

typedef struct {
    OPDS h;
    MYFLT *out, *xfreq, *kbw, *ifn, *iphs, *iflags;
    MYFLT  lastfreq;
    int32_t  phase;
    int32_t  lomask;
    MYFLT  cpstoinc, radtoinc;
    FUNC * ftp;
    MYFLT  x1, x2, x3; // MA
    MYFLT  y1, y2, y3; // AR
    int flags;
    GaussianState gs;
    uint32_t seed;
} BEOSC;

static int
beosc_init(CSOUND *csound, BEOSC *p) {
    FUNC *ftp;
    MYFLT sampledur = 1 / csound->GetSr(csound);
    ftp = csound->FTFind(csound, p->ifn);
    if (UNLIKELY(ftp == NULL))
      return NOTOK;
    p->ftp = ftp;
    uint32_t tabsize = ftp->flen;
    p->radtoinc = tabsize * (RTWOPI * 65536.);
    p->cpstoinc = tabsize * sampledur * 65536;
    p->lomask   = (tabsize - 1) << 3;
    p->phase    = fabs(fmod(*p->iphs, TWOPI)) * p->radtoinc;
    p->flags    = (int)(*p->iflags);
    p->lastfreq = *p->xfreq;
    p->gs.seed  = p->seed = csound->GetRandomSeedFromTime();
    p->gs.iset  = 0;
    gaussians_init(csound->GetRandomSeedFromTime());
    return OK;
}

static int
beosc_kkiii(CSOUND *csound, BEOSC *p) {
    IGN(csound);

    SAMPLE_ACCURATE

    FUNC *ftp     = p->ftp;
    MYFLT freqin  = *p->xfreq;
    MYFLT bwin    = *p->kbw;
    MYFLT *table0 = ftp->ftable;
    MYFLT *table1 = table0 + 1;

    int32_t phase  = p->phase;
    int32_t lomask = p->lomask;

    int32_t phaseinc = (int32_t)(p->cpstoinc * freqin);

    MYFLT x0,
          x1 = p->x1,
          x2 = p->x2,
          x3 = p->x3,
          y0,
          y1 = p->y1,
          y2 = p->y2,
          y3 = p->y3;

    // bw coefficients
    MYFLT bw1 = sqrt( FL(1.0) - bwin );
    MYFLT bw2 = sqrt( FL(2.0) * bwin );

    // uint32_t seed = p->gs.seed;
    uint32_t seed = p->seed;

    switch (p->flags) {
    case 0:    // uniform noise, no interp.
      for (n=offset; n<nsmps; n++) {
        x0 = x1; x1 = x2; x2 = x3;
        // kelly uses 6. / GAIN
        x3 = (FastRandFloat(&seed) * FL(2.0) - FL(1.0)) *
          FL(0.00012864661681256);
        y0 = y1; y1 = y2; y2 = y3;
        y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
          (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
        out[n] = lookup(table0, phase, lomask)
          * (bw1 + ( y3 * bw2 ));
        phase += phaseinc;
      }

      break;
    case 1:     // gaussian noise, no interp
      for (n=offset; n<nsmps; n++) {
        x0 = x1; x1 = x2; x2 = x3;
        x3  = GAUSSIANS_GET(&seed);
        x3 *= FL(0.00012864661681256);  // kelly uses 6. / GAIN
        y0  = y1; y1 = y2; y2 = y3;
        y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
          (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
        out[n] = lookup(table0, phase, lomask)  \
          * (bw1 + ( y3 * bw2 ));
            phase += phaseinc;
      }
      break;
    case 2:
      for (n=offset; n<nsmps; n++) {
        x0  = x1; x1 = x2; x2 = x3;
        x3  = (FastRandFloat(&seed) * FL(2.0) - FL(1.0));
        x3 *= FL(0.00012864661681256);  // kelly uses 6. / GAIN
        y0  = y1; y1 = y2; y2 = y3;
        y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
          (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
        out[n] = lookupi1(table0, table1, phase, lomask)
          * (bw1 + ( y3 * bw2 ));
        phase += phaseinc;
      }
      break;
    case 3:
      for (n=offset; n<nsmps; n++) {
        x0  = x1; x1 = x2; x2 = x3;
        x3  = GAUSSIANS_GET(&seed);
        x3 *= FL(0.00012864661681256); // kelly uses 6. / GAIN
        y0  = y1; y1 = y2; y2 = y3;
        y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
          (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
        out[n] = lookupi1(table0, table1, phase, lomask)        \
          * (bw1 + ( y3 * bw2 ));
        phase += phaseinc;
      }
      break;
    }
    // p->gs.seed = seed;
    p->seed = seed;
    p->phase = phase;
    p->x1    = x1; p->x2 = x2; p->x3 = x3;
    p->y1    = y1; p->y2 = y2; p->y3 = y3;
    return OK;
}

static int
beosc_akiii(CSOUND *csound, BEOSC *p) {
    SAMPLE_ACCURATE

    FUNC *ftp  = p->ftp;
    MYFLT *freqptr = p->xfreq;
    MYFLT bwin    = *p->kbw;
    MYFLT *table0 = ftp->ftable;
    MYFLT *table1 = table0 + 1;
    // MYFLT noise;

    int32_t phase  = p->phase;
    int32_t lomask = p->lomask;

    MYFLT x0,
          x1 = p->x1,
          x2 = p->x2,
          x3 = p->x3,
          y0,
          y1 = p->y1,
          y2 = p->y2,
          y3 = p->y3;

    // bw coefficients
    MYFLT bw1 = sqrt( FL(1.0) - bwin );
    MYFLT bw2 = sqrt( FL(2.0) * bwin );

    uint32_t seed = p->gs.seed;

    MYFLT freq,
          cpstoinc = p->cpstoinc;

    // GaussianState *gsptr;

    switch (p->flags) {
    case 0:
      for (n=offset; n<nsmps; n++) {
        x0  = x1; x1 = x2; x2 = x3;
        x3  = FastRandFloat(&seed) * FL(2.0) - FL(1.0);
        x3 *= FL(0.00012864661681256); // kelly uses 6. / GAIN
        y0  = y1; y1 = y2; y2 = y3;
        y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
          (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
        out[n] = lookup(table0, phase, lomask)
          * (bw1 + ( y3 * bw2 ));
        freq   = freqptr[n];
        phase += (int32_t)(cpstoinc * freq);
      }

      break;
    case 1:
      for (n=offset; n<nsmps; n++) {

        x0 = x1; x1 = x2; x2 = x3;
        // x3 = gaussian_normal(gsptr);
        x3  = GAUSSIANS_GET(&seed);
        x3 *= FL(0.00012864661681256); // kelly uses 6. / GAIN
        y0  = y1; y1 = y2; y2 = y3;
        y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
          (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
        out[n] = lookup(table0, phase, lomask)  \
          * (bw1 + ( y3 * bw2 ));
        freq   = freqptr[n];
        phase += (int32_t)(cpstoinc * freq);
      }
      break;
    case 2:    // + interp
      for (n=offset; n<nsmps; n++) {
        x0  = x1; x1 = x2; x2 = x3;
        x3  = FastRandFloat(&seed) * FL(2.0) - FL(1.0);
        x3 *= FL(0.00012864661681256); // kelly uses 6. / GAIN
        y0  = y1; y1 = y2; y2 = y3;
        y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
          (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
        out[n] = lookupi1(table0, table1, phase, lomask)
          * (bw1 + ( y3 * bw2 ));
        freq   = freqptr[n];
        phase += (int32_t)(cpstoinc * freq);
      }
      p->gs.seed = seed;
      break;
    case 3:    // + interp
      for (n=offset; n<nsmps; n++) {
        x0 = x1; x1 = x2; x2 = x3;
        // x3 = gaussian_normal(gsptr);
        x3  = GAUSSIANS_GET(&seed);
        x3 *= FL(0.00012864661681256); // kelly uses 6. / GAIN
        y0  = y1; y1 = y2; y2 = y3;
        y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
          (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
        out[n] = lookupi1(table0, table1, phase, lomask)        \
          * (bw1 + ( y3 * bw2 ));
        freq   = freqptr[n];
        phase += (int32_t)(cpstoinc * freq);
      }
      break;
    }
    p->gs.seed = seed;
    p->phase = phase;
    p->x1    = x1; p->x2 = x2; p->x3 = x3;
    p->y1    = y1; p->y2 = y2; p->y3 = y3;
    return OK;
}


/*
   ----------------------------------------------------------------

   beadsynt - Band enhanced oscillator bank

   aout   beadsynt ifreqfn, iampfn, ibwfn, icnt, \
                  kfreq=1, kbw=1, iwfn=-1, iphs=-1, iflags=0
   aout   beadsynt kFreq[], kAmp[], kBw[], icnt, \
                  kfreq=1, kbw=1, iwfn=-1, iphs=-1, iflags=0

   ifreqfn: a table containing frequencies for each oscillator.
   iampfn:  a table containing amplitudes for each oscillator.
   ibwfn:   a table containing bandwidths for each oscillator.
   icnt:    number of oscillators. All three tables or arrays must be at
            least this big
   kfreq:   frequency scaling, all frequencies are multiplied by this factor
   kbw:     bandwidth scaling, bandwidths are multiplied by this factor
   iwfn:    a table containing one wave cycle to be used for the oscillators.
            -1 to use the default sine table
   iphs:    Initial phase for the oscillators, indicated as follows:
            *  -1: randomize phases
            * 0-1: set the initial phase of all oscillators to this value
            * >=1: the table number (an int) containing the initial phase
                   for each oscillator (must contain at least icnt values)
   iflags:  0-1  => noise type (0=uniform, 1=gaussian)
            +2   => table lookup interpolation
            +4   => freq interpolation

   ----------------------------------------------------------------
 */

typedef struct {
    MYFLT x1, x2, x3;
    MYFLT y1, y2, y3;
} FILTCOEFS;

static void
befilter_init(FILTCOEFS *filt) {
    filt->x1 = 0;
    filt->x2 = 0;
    filt->x3 = 0;
    filt->y1 = 0;
    filt->y2 = 0;
    filt->y3 = 0;
}

typedef struct {
    OPDS h;
    MYFLT *out;
    void *ifreqtbl, *iamptbl, *ibwtbl;
    MYFLT *icnt,  *iflags, *kfreq, *kbw, *ifn, *iphs;
    GaussianState gs;
    FUNC * ftp;
    MYFLT *freqs;
    MYFLT *amps;
    MYFLT *bws;
    unsigned int count;
    int inerr;
    AUXCH lphs;
    AUXCH pamp;
    AUXCH filtcoefs;
    AUXCH pfreq;
    MYFLT cpstoinc;
    uint32_t seed;
    int updatearrays;
} BEADSYNT;

static int32_t
beadsynt_init_common(CSOUND *csound, BEADSYNT *p) {
    FILTCOEFS *filtcoefs;
    int32_t *lphs;
    unsigned int c, count = p->count;
    MYFLT iphs = *p->iphs;
    MYFLT sr   = csound->GetSr(csound);
    p->inerr = 0;
    // corresponds to csound->sicvt. FMAXLEN depends on B64BIT being defined
    p->cpstoinc = FMAXLEN / sr;
    p->gs.seed  = p->seed = csound->GetRandomSeedFromTime();
    p->gs.iset  = 0;  p->gs.gset = 0;
    gaussians_init(csound->GetRandomSeedFromTime());

    if (p->lphs.auxp==NULL || p->lphs.size < sizeof(int32_t)*count)
      csound->AuxAlloc(csound, sizeof(int32_t)*count, &p->lphs);
    lphs = (int32*)p->lphs.auxp;

    if (iphs < 0) {
      // init phase with random values
      uint32_t seed = csound->GetRandomSeedFromTime();
      for (c=0; c<count; c++) {
        lphs[c] = (int32_t)(FastRandFloat(&seed) * FMAXLEN) & PHMASK;
      }
    } else if (iphs <= 1) {
      // between 0 and 1, use this number as phase
      for (c=0; c<count; c++) {
        lphs[c] = ((int32_t)(iphs * FMAXLEN)) & PHMASK;
      }
    } else {  // iphs is the number of a table containing the phases
      FUNC *phasetp = csound->FTnp2Finde(csound, p->iphs);
      if (phasetp == NULL) {
        p->inerr = 1;
        return INITERR(Str("beadsynt: phasetable not found"));
      }
      for (c=0; c<count; c++) {
        MYFLT ph = phasetp->ftable[c];
        lphs[c] = ((int32_t)(ph * FMAXLEN)) & PHMASK;
      }
    }

    if (p->pamp.auxp==NULL || p->pamp.size < (uint32_t)(sizeof(MYFLT)*p->count))
      csound->AuxAlloc(csound, sizeof(MYFLT)*p->count, &p->pamp);
    else if (iphs >= 0)        /* AuxAlloc clear anyway */
      memset(p->pamp.auxp, 0, sizeof(MYFLT)*p->count);

    if (p->filtcoefs.auxp==NULL || p->filtcoefs.size < sizeof(FILTCOEFS)*count)
      csound->AuxAlloc(csound, sizeof(FILTCOEFS)*count, &p->filtcoefs);
    filtcoefs = (FILTCOEFS *)(p->filtcoefs.auxp);
    for (c=0; c<count; c++) {
      befilter_init(filtcoefs++);
    }
    // freq. interpolation
    if ((int)*p->iflags & 4) {
      if (p->pfreq.auxp==NULL ||
          p->pfreq.size < (uint32_t)(sizeof(MYFLT)*p->count))
        csound->AuxAlloc(csound, sizeof(MYFLT)*p->count, &p->pfreq);
      // init freqs to current table contents
      MYFLT *prevfreqs = (MYFLT*)p->pfreq.auxp;
      MYFLT *freqs  = p->freqs;
      MYFLT freqmul = *p->kfreq;
      for (c=0; c<p->count; c++) {
        prevfreqs[c] = freqs[c] * freqmul;
      }
    }
    return OK;
}

static int32_t
beadsynt_init(CSOUND *csound, BEADSYNT *p) {
    FUNC *ftp;
    int count = (int)*p->icnt;
    p->inerr = 1;
    p->ftp = ftp = csound->FTFind(csound, p->ifn);
    if (ftp == NULL) {
      return INITERR(Str("beadsynt: wavetable not found"));
    }
    ftp = csound->FTnp2Find(csound, (MYFLT *)p->iamptbl);
    if (ftp == NULL) {
      return INITERR("beadsynt: amptable not found!");
    }
    if( count < 0) {
      // count not specified, set it to the length of the amps table
      count = ftp->flen;
    }
    if (ftp->flen < (unsigned int)count) {
      return INITERR(Str("beadsynt: partial count > amptable size"));
    }
    p->amps = ftp->ftable;

    ftp = csound->FTnp2Find(csound, (MYFLT *)p->ifreqtbl);
    if (ftp == NULL) {
      return INITERR(Str("beadsynt: freqtable not found!"));
    }
    if (ftp->flen < (unsigned int)count) {
      return INITERR(Str("beadsynt: partial count > freqtable size"));
    }
    p->freqs = ftp->ftable;

    ftp = csound->FTnp2Find(csound, (MYFLT *)p->ibwtbl);
    if (ftp == NULL) {
      return INITERR(Str("beadsynt: bandwidth table not found"));
    }
    if (ftp->flen < (unsigned int)count) {
      return INITERR(Str("beadsynt: partial count > bandwidth size"));
    }
    p->bws = ftp->ftable;
    p->updatearrays = 0;
    p->inerr = 0;
    p->count = count < 1 ? 1 : count;

    return beadsynt_init_common(csound, p);
}

static int32_t
beadsynt_init_array(CSOUND *csound, BEADSYNT *p) {
    FUNC *ftp;
    p->ftp = ftp = csound->FTFind(csound, p->ifn);
    if (ftp == NULL) {
      p->inerr = 1;
      return INITERR(Str("beadsynt: wavetable not found!"));
    }

    ARRAYDAT *ampsarr  = (ARRAYDAT *)p->iamptbl;
    ARRAYDAT *freqsarr = (ARRAYDAT *)p->ifreqtbl;
    ARRAYDAT *bwsarr   = (ARRAYDAT *)p->ibwtbl;
    // check sizes and dimensions
    if(ampsarr->dimensions != 1 || freqsarr->dimensions != 1 ||
       bwsarr->dimensions != 1) {
      return INITERR(Str("The arrays should have 1 dimension"));
    }

    int count = (int)*p->icnt;
    if (count < 0) {
      // count not specified: set it to the size of the amps array
      count = ampsarr->sizes[0];
    }
    p->count = count;

    if(ampsarr->sizes[0] < count) {
      return INITERR(Str("Amplitudes array is too small"));
    }
    if(freqsarr->sizes[0] < count) {
      return INITERR(Str("Frequencies array is too small"));
    }
    if(bwsarr->sizes[0] < count) {
      return INITERR(Str("bandwidths array is too small"));
    }

    p->amps  = ampsarr->data;
    p->freqs = freqsarr->data;
    p->bws   = bwsarr->data;

    p->updatearrays = 1;
    return beadsynt_init_common(csound, p);
}

// FMAXLEN = MYFLT 0x40000000
// PHMASK = 0x3fffffff

static int32_t
beadsynt_perf(CSOUND *csound, BEADSYNT *p) {
    FUNC *ftp;
    MYFLT *out, *ftpdata, *freqs, *amps, *bws, *prevamps, *prevfreqs;
    MYFLT freq, freqmul, freqnow, freqinc;
    MYFLT amp, ampnow, ampinc, bwmul, bwin, bw1, bw2;
    MYFLT cpstoinc, sample, lodiv;
    int32_t phs, inc, lobits, lomask;
    int32_t *lphs;
    int flags;
    unsigned int c, count;
    MYFLT x0, x1, x2, x3, y0, y1, y2, y3;
    FILTCOEFS *coefs;
    uint32_t seed,
             offset = p->h.insdshead->ksmps_offset,
             early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(p->inerr))
      return INITERR(Str("beadsynt: not initialised"));

    ftp = p->ftp;
    ftpdata  = ftp->ftable;
    lobits   = ftp->lobits;
    lodiv    = ftp->lodiv;
    lomask   = ftp->lomask;
    cpstoinc = p->cpstoinc;
    freqmul  = *p->kfreq;
    bwmul    = *p->kbw;
    count    = p->count;
    out      = p->out;
    flags    = (int)*p->iflags;

    if(p->updatearrays) {
      freqs = ((ARRAYDAT *)p->ifreqtbl)->data;
      amps  = ((ARRAYDAT *)p->iamptbl)->data;
      bws   = ((ARRAYDAT *)p->ibwtbl)->data;
    } else {
      freqs = p->freqs;
      amps  = p->amps;
      bws   = p->bws;
    }

    lphs = (int32*)p->lphs.auxp;
    prevamps  = (MYFLT*)p->pamp.auxp;
    prevfreqs = (MYFLT*)p->pfreq.auxp;

    // clear output before adding partials
    memset(out, 0, nsmps*sizeof(MYFLT));

    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }

    coefs = (FILTCOEFS *)(p->filtcoefs.auxp);
    // GaussianState *gsptr = &(p->gs);
    seed = p->seed;

    for (c=0; c<count; c++) {
      ampnow = prevamps[c];
      amp    = amps[c];
      if(ampnow == 0 && amp == 0) {
        // skip silent partials
        coefs++;
        continue;
      }
      freq = freqs[c] * freqmul;
      inc  = (int32_t) (freq * cpstoinc);

      bwin = bws[c] * bwmul;
      bwin = bwin < 0 ? 0 : (bwin > 1 ? 1 : bwin);
      bw1  = sqrt(FL(1.0) - bwin);
      bw2  = sqrt(FL(2.0) * bwin);

      phs    = lphs[c];
      ampinc = (amp - ampnow) * CS_ONEDKSMPS;

      if(LIKELY(bwin != 0)) {
        x1 = coefs->x1; x2 = coefs->x2; x3 = coefs->x3;
        y1 = coefs->y1; y2 = coefs->y2; y3 = coefs->y3;
        switch(flags) {
          // 0-1=uniform | gauss. noise,
          //  +2=osc lookup with linear interp
          //  +4=freq. interp
        case 0:  // 000
          for (n=offset; n<nsmps; n++) {
            x0  = x1; x1 = x2; x2 = x3;
            x3  = FastRandFloat(&seed) * FL(2) - FL(1);
            x3 *= FL(0.00012864661681256);
            y0  = y1; y1 = y2; y2 = y3;
            y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
              (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            sample  = *(ftpdata + (phs >> lobits)) * ampnow;
            out[n] += sample * (bw1 + (y3*bw2));
            phs    += inc;
            phs    &= PHMASK;
            ampnow += ampinc;
          }
          break;
        case 1:  // 001
          for (n=offset; n<nsmps; n++) {
            x0 = x1; x1 = x2; x2 = x3;
            // x3 = gaussian_normal(gsptr);
            x3  = GAUSSIANS_GET(&seed);
            x3 *= FL(0.00012864661681256);
            y0  = y1; y1 = y2; y2 = y3;
            y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
              (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            sample  = *(ftpdata + (phs >> lobits)) * ampnow;
            out[n] += sample * (bw1 + (y3*bw2));
            phs    += inc; phs &= PHMASK;
            ampnow += ampinc;
          }
          break;
        case 2:  // 010
          for (n=offset; n<nsmps; n++) {
            x0  = x1; x1 = x2; x2 = x3;
            x3  = FastRandFloat(&seed) * FL(2) - FL(1);
            x3 *= FL(0.00012864661681256);
            y0  = y1; y1 = y2; y2 = y3;
            y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
              (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            sample =
              cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
            out[n] += sample * (bw1 + (y3*bw2));
            phs    += inc; phs &= PHMASK;
            ampnow += ampinc;
          }
          break;
        case 3:  // 011
          // seed = p->gs.seed;
          for (n=offset; n<nsmps; n++) {
            x0  = x1; x1 = x2; x2 = x3;
            x3  = GAUSSIANS_GET(&seed);
            x3 *= FL(0.00012864661681256);
            y0  = y1; y1 = y2; y2 = y3;
            y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
              (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            sample =
              cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
            out[n] += sample * (bw1 + (y3*bw2));
            phs    += inc; phs &= PHMASK;
            ampnow += ampinc;
          }
          break;
        case 4:  // 100
          freqnow = prevfreqs[c];
          freqinc = (freq - freqnow) * CS_ONEDKSMPS;
          // seed = p->gs.seed;
          for (n=offset; n<nsmps; n++) {
            x0  = x1; x1 = x2; x2 = x3;
            x3  = FastRandFloat(&seed) * FL(2) - FL(1);
            x3 *= FL(0.00012864661681256);
            y0  = y1; y1 = y2; y2 = y3;
            y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
              (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            sample   = *(ftpdata + (phs >> lobits)) * ampnow;
            out[n]  += sample * (bw1 + (y3*bw2));
            freqnow += freqinc;
            phs    += (int32_t)(cpstoinc * freqnow); phs &= PHMASK;
            ampnow += ampinc;
          }
          prevfreqs[c] = freq;
          break;
        case 5:  // 101
          freqnow = prevfreqs[c];
          freqinc = (freq - freqnow) * CS_ONEDKSMPS;
          for (n=offset; n<nsmps; n++) {
            x0 = x1; x1 = x2; x2 = x3;
            // x3 = gaussian_normal(gsptr);
            x3  = GAUSSIANS_GET(&seed);
            x3 *= FL(0.00012864661681256);
            y0  = y1; y1 = y2; y2 = y3;
            y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
              (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            sample   = *(ftpdata + (phs >> lobits)) * ampnow;
            out[n]  += sample * (bw1 + (y3*bw2));
            freqnow += freqinc;
            phs    += (int32_t)(cpstoinc * freqnow); phs &= PHMASK;
            ampnow += ampinc;
          }
          prevfreqs[c] = freq;
          break;
        case 6:  // 110
          freqnow = prevfreqs[c];
          freqinc = (freq - freqnow) * CS_ONEDKSMPS;
          for (n=offset; n<nsmps; n++) {
            x0  = x1; x1 = x2; x2 = x3;
            x3  = FastRandFloat(&seed) * FL(2) - FL(1);
            x3 *= FL(0.00012864661681256);
            y0  = y1; y1 = y2; y2 = y3;
            y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
              (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            sample =
              cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
            out[n]  += sample * (bw1 + (y3*bw2));
            freqnow += freqinc;
            phs    += (int32_t)(cpstoinc * freqnow); phs &= PHMASK;
            ampnow += ampinc;
          }
          prevfreqs[c] = freq;
          break;
        case 7:  // 111
          freqnow = prevfreqs[c];
          freqinc = (freq - freqnow) * CS_ONEDKSMPS;
          for (n=offset; n<nsmps; n++) {
            x0 = x1; x1 = x2; x2 = x3;
            // x3 = gaussian_normal(gsptr);
            x3  = GAUSSIANS_GET(&seed);
            x3 *= FL(0.00012864661681256);
            y0  = y1; y1 = y2; y2 = y3;
            y3  = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
              (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            sample = cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
            out[n]  += sample * (bw1 + (y3*bw2));
            freqnow += freqinc;
            phs    += (int32_t)(cpstoinc * freqnow); phs &= PHMASK;
            ampnow += ampinc;
          }
          prevfreqs[c] = freq;
          break;
        }
        coefs->x1 = x1; coefs->x2 = x2; coefs->x3 = x3;
        coefs->y1 = y1; coefs->y2 = y2; coefs->y3 = y3;
      } else {
        // simplified loops when there is no bw
        switch(flags) {
        case 0:  // 000
        case 1:  // 001
          for (n=offset; n<nsmps; n++) {
            out[n] += *(ftpdata + (phs >> lobits)) * ampnow;
            phs    += inc; phs &= PHMASK; ampnow += ampinc;
          }
          break;
        case 2:  // 010
        case 3:  // 011
          for (n=offset; n<nsmps; n++) {
            out[n] +=
              cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
            phs += inc; phs &= PHMASK; ampnow += ampinc;
          }
          break;
        case 4:  // 100
        case 5:  // 101
          freqnow = prevfreqs[c];
          freqinc = (freq - freqnow) * CS_ONEDKSMPS;
          for (n=offset; n<nsmps; n++) {
            out[n]  += *(ftpdata + (phs >> lobits)) * ampnow;
            freqnow += freqinc;
            phs    += (int32_t)(cpstoinc * freqnow); phs &= PHMASK;
            ampnow += ampinc;
          }
          prevfreqs[c] = freq;
          break;
        case 6:  // 110
        case 7:  // 111
          freqnow = prevfreqs[c];
          freqinc = (freq - freqnow) * CS_ONEDKSMPS;
          for (n=offset; n<nsmps; n++) {
            out[n] +=
              cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
            freqnow += freqinc;
            phs    += (int32_t)(cpstoinc * freqnow); phs &= PHMASK;
            ampnow += ampinc;
          }
          prevfreqs[c] = freq;
          break;
        }
      }
      prevamps[c] = amp;
      lphs[c] = phs;
      coefs++;
    }
    p->seed = seed;
    return OK;
}

/////////////////////////////////////////////////////


/*

    tabrowlin

    Assuming a 2D table containing multiple rows of sampled streams
    (for instance, the amplitudes of a set of oscilators, sampled at a
    regular interval): extract one row of that data with linear
    interpolation between adjacent rows (if row is not a whole number)

    tabrowlin krow, ifnsrc, ifndest, inumcols, ioffset=0, istart=0, iend=0,
       istep=1

    If reading out of bounds a PerformanceError will be raised. Because we
    interpolate between rows, the last row that can be read is

    maxrow = (ftlen(ifnsrc)-ioffset)/inumcols - 2

    krow     : the row to read (can be a fractional number, in which case
             : interpolation with the next row is performed)
    ifnsrc   : index of the source table
    ifndest  : index of the dest table
    inumcols : the number of columns a row has, in the source table
    ioffset  : an offset to where the data starts (used to skip a header,
             : if present)
    istart   : start index to read from the row
    iend     : end index to read from the row (not inclusive)
    istep    : step used to read the along the row

    The use case is as follows: a bank of oscillators is driven by a
    table containing the data. The bank has a fixed number of
    oscillators, each oscillator is sampled regularly and for each
    instant, the frequency, amplitude and bandwidth are recorded. All
    information is put into a table with following layout

    row0: f0 amp0 bw0 f1 amp1 bw1 f2 amp2 bw2 ...
    row1: f0 amp0 bw0 f1 amp1 bw1 f2 amp2 bw2 ...
    ...

    In order to get the frequency of the oscillators at any given time,
    krow = ktime / ksampleperiod

    Put the (interpolated) frequencies in another table

    ioffset = 0
    istart = 0
    iend = 0
    istep = 3
    tabrowlin krow, ifnsrc, ifndest, inumoscil*istep, ioffset, istart, iend, istep

 */

typedef struct {
    OPDS h;
    MYFLT *krow, *ifnsrc, *ifndest, *inumcols, *ioffset, *istart, *iend, *istep;
    MYFLT* tabsource;
    MYFLT* tabdest;
    int maxrow;
    int tabsourcelen;
    int tabdestlen;
    int end;
} TABROWCOPY;

// idx = ioffset + row * inumcols + n*step, while idx < iend

static int32_t
tabrowcopy_init(CSOUND* csound, TABROWCOPY* p){
    FUNC* ftp;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifnsrc)) == NULL))
      return INITERR(Str("tabrowcopy: incorrect table number"));
    p->tabsource    = ftp->ftable;
    p->tabsourcelen = ftp->flen;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifndest)) == NULL))
      return INITERR(Str("tabrowcopy: incorrect table number"));
    p->tabdest    = ftp->ftable;
    p->tabdestlen = ftp->flen;

    int end = *p->iend;
    if(end > *p->inumcols)
        return INITERR(Str("tabrowcopy: iend cannot be bigger than numcols"));

    if(end == 0)
        end = *p->inumcols;

    p->end = end;

    int numcols_to_copy = (int)((end - *p->istart) / *p->istep);
    if (numcols_to_copy > p->tabdestlen)
        return INITERR(Str("tabrowcopy: Destination table too small"));

    p->maxrow = (int)((p->tabsourcelen - *p->ioffset) / *p->inumcols) - 1;
    return OK;
}

static int32_t
tabrowcopyk(CSOUND* csound, TABROWCOPY* p) {
    int i;
    MYFLT x0, x1;
    MYFLT row   = *p->krow;
    if(row > p->maxrow) {
      csound->Message(csound, Str(">>>> tabrowlin: row %.4f > maxrow %d! "
                                  "It will be clipped\n"),
                      row, p->maxrow);
       row = p->maxrow;
    }
    row = row < p->maxrow ? row : p->maxrow;
    int row0    = (int)row;
    MYFLT delta = row - row0;
    int numcols = *p->inumcols;
    int offset  = *p->ioffset;
    int start   = *p->istart;
    int end  = p->end;
    int step = *p->istep;
    int tabsourcelen = p->tabsourcelen;

    MYFLT* tabsource = p->tabsource;
    MYFLT* tabdest   = p->tabdest;

    int idx0 = offset + numcols * row0 + start;
    int idx1 = idx0 + (end-start);
    int j    = 0;

    if(UNLIKELY(row < 0))
      return PERFERR(Str("tabrowcopy: krow cannot be negative"));

    if (LIKELY(delta != 0)) {
      if (UNLIKELY(idx1+numcols > tabsourcelen)) {
        csound->Message(csound,
                       "krow: %f   row0: %d  idx1: %d  numcols: %d   "
                        "tabsourcelen: %d\n",
                        row, row0, idx1, numcols, tabsourcelen);
        return PERFERR(Str("tabrowcopy: tab off end"));
      }
      for (i=idx0; i<idx1; i+=step) {
        x0 = tabsource[i];
        x1 = tabsource[i + numcols];
        tabdest[j++] = x0 + (x1-x0)*delta;
      }
    } else {
      if (UNLIKELY(idx1 > tabsourcelen))
        return PERFERR(Str("tabrowcopy: tab off end"));
      for (i=idx0; i<idx1; i+=step) {
        tabdest[j++] = tabsource[i];
      }
    }
    return OK;
}

typedef struct {
    OPDS h;
    ARRAYDAT *outarr;
    MYFLT *krow, *ifnsrc, *inumcols, *ioffset, *istart, *iend, *istep;
    MYFLT* tabsource;
    MYFLT  maxrow;
    uint32_t tabsourcelen;
    uint32_t end;
    uint32_t numitems;
} TABROWCOPYARR;

static int32_t
tabrowcopyarr_init(CSOUND *csound, TABROWCOPYARR *p) {
    FUNC* ftp;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifnsrc)) == NULL))
        return INITERR(Str("tabrowlin: incorrect table number"));
    p->tabsource = ftp->ftable;
    p->tabsourcelen = ftp->flen;
    uint32_t start = (uint32_t)*p->istart;
    uint32_t end   = (uint32_t)*p->iend;
    uint32_t step  = (uint32_t)*p->istep;
    if(end > *p->inumcols)
        return INITERR(Str("tabrowlin: iend cannot be bigger than numcols"));
    if(end == 0)
        end = (uint32_t) *p->inumcols;
    if(end <= start) {
        return INITERR(Str("tabrowlin: end must be bigger than start"));
    }
    p->end = end;
    uint32_t numitems = (uint32_t) (ceil((end - start) / (MYFLT)step));
    if(numitems <= 0) {
        return INITERR(Str("tabrowlin: no items to copy"));
    }
    tabinit(csound, p->outarr, numitems);
    p->numitems = numitems;
    p->maxrow = (p->tabsourcelen - *p->ioffset) / *p->inumcols - FL(2);
    return OK;
}

static int32_t
tabrowcopyarr_k(CSOUND *csound, TABROWCOPYARR *p) {
    uint32_t start = (uint32_t)*p->istart;
    uint32_t end   = (uint32_t)p->end;
    uint32_t step  = (uint32_t)*p->istep;
    uint32_t offset = (uint32_t)*p->ioffset;
    //uint32_t numitems = (uint32_t)ceil((end - start) / (float)step);
    uint32_t numcols = (uint32_t)*p->inumcols;
    MYFLT row = *p->krow;
    uint32_t row0 = (uint32_t)row;
    MYFLT delta = row - row0;
    uint32_t tabsourcelen = p->tabsourcelen;
    MYFLT x0, x1;

    if(UNLIKELY(row < 0)) {
      return PERFERR(Str("krow cannot be negative"));
    }
    // TODO : check maxrow
    uint32_t idx0 = offset + numcols * row0 + start;
    uint32_t idx1 = idx0 + (end-start);
    uint32_t numitems = (uint32_t) (ceil((end - start) / (MYFLT)step));
    ARRAY_ENSURESIZE_PERF(csound, p->outarr, numitems);

    MYFLT *out = p->outarr->data;
    MYFLT *tabsource = p->tabsource;

    uint32_t i, j = 0;
    if (LIKELY(delta != 0)) {
    if (UNLIKELY(idx1+numcols >= tabsourcelen))
        return PERFERR(Str("tab off end"));
      for (i=idx0; i<idx1; i+=step) {
        x0 = tabsource[i];
        x1 = tabsource[i + numcols];
        out[j++] = x0 + (x1-x0)*delta;
      }
    } else {
      if (UNLIKELY(idx1 >= tabsourcelen))
        return PERFERR(Str("tab off end"));
      for (i=idx0; i<idx1; i+=step) {
        out[j++] = tabsource[i];
      }
    }
    return OK;
}


typedef struct {
    OPDS h;
    // kOut[] rowlin kMtrx[], krow, kstart=0, kend=0, kstep=1
    ARRAYDAT *outarr, *inarr;
    MYFLT *krow, *kstart, *kend, *kstep;
    int numitems;
} GETROWLIN;

/*

  getrowlin: the same as tabrowlin, but with arrays instead of tables

  kOut[] getrowlin kMtrx[], krow, kstart=0, kend=0, kstep=1

  Given a 2D array kMtrx, get a row of this array (possibly a slice
  [kstart:kend:kstep]).  If krow is not an integer, the values are the
  result of the interpolation between two rows

*/

static int32_t
getrowlin_init(CSOUND *csound, GETROWLIN *p) {
    int start = (int)*p->kstart;
    int end   = (int)*p->kend;
    int step  = (int)*p->kstep;
    if (end < 1)
      end = p->inarr->sizes[1];
    int numitems = (int) (ceil((end - start) / (MYFLT)step));
    tabinit(csound, p->outarr, numitems);
    p->numitems = numitems;
    return OK;
}

static int32_t
getrowlin_k(CSOUND *csound, GETROWLIN *p) {
    if(p->inarr->dimensions != 2)
        return PERFERR(Str("The input array should be a 2D array"));
    int start = (int)*p->kstart;
    int end   = (int)*p->kend;
    int step  = (int)*p->kstep;
    if (end <= 0) {
        end = p->inarr->sizes[1];
    }
    int numitems = (int) (ceil((end - start) / (MYFLT)step));
    int numcols  = p->inarr->sizes[1];
    if(numitems > numcols)
        return PERFERR(Str("Asked to read too many items from a row"));
    ARRAY_ENSURESIZE_PERF(csound, p->outarr, numitems);

    p->numitems = numitems;
    MYFLT row = *p->krow;
    int maxrow = p->inarr->sizes[0] - 1;
    if(UNLIKELY(row < 0))
        return PERFERR(Str("getrowlin: krow cannot be negative"));
    if(UNLIKELY(row > maxrow)) {
        csound->Message(csound, Str("getrowlin: row %.4f > maxrow %d, clipping\n"),
                        row, maxrow);
        row = maxrow;
        // return PERFERR(Str("getrowlin: exceeded maximum row"));
    }
    int row0    = (int)row;
    MYFLT delta = row - row0;

    MYFLT *out = p->outarr->data;
    MYFLT *in  = p->inarr->data;

    int idx0 = numcols * row0 + start;
    int idx1 = idx0 + numitems;
    MYFLT x0, x1;
    int i, j = 0;
    if (LIKELY(delta != 0)) {
        for (i=idx0; i<idx1; i+=step) {
            x0 = in[i];
            x1 = in[i + numcols];
            out[j++] = x0 + (x1-x0)*delta;
        }
    }
    else {
        for (i=idx0; i<idx1; i+=step) {
            out[j++] = in[i];
        }
    }
    return OK;
}

////////////////////////////////////////////////////////////////////////////////


/*

Input types:

 * a, k, s, i, w, f,
 * o (optional i-rate, default to 0), O optional krate=0
 * p (opt, default to 1), P optional krate=1
 * q (opt, 10),
 * v(opt, 0.5),
 * j(opt, -1), J optional krate=-1
 * h(opt, 127),
 * y (multiple inputs, a-type),
 * z (multiple inputs, k-type),
 * Z (multiple inputs, alternating k- and a-types),
 * m (multiple inputs, i-type),
 * M (multiple inputs, any type)
 * n (multiple inputs, odd number of inputs, i-type).
 * . anytype
 * ? optional

 */

#define S(x)    sizeof(x)

static OENTRY localops[] = {
    // aout beosc xfreq, kbw, ifn=-1, iphase=0, iflags=1
    {"beosc", S(BEOSC), TR, 3, "a", "kkjop", (SUBR)beosc_init, (SUBR)beosc_kkiii },
    {"beosc", S(BEOSC), TR, 3, "a", "akjop", (SUBR)beosc_init, (SUBR)beosc_akiii },

    // aout beadsynt ifreqft, iampft, ibwft, inumosc,
    //               iflags=1, kfreq=1, kbw=1, ifn=-1, iphs=-1
    {"beadsynt", S(BEADSYNT), TR, 3, "a", "iiijpPPjj",
             (SUBR)beadsynt_init, (SUBR)beadsynt_perf },

    // aout beadsynt kFreq[], kAmp[], kBw[],
    //               inumosc=-1, iflags=1, kfreq=1, kbw=1, ifn=-1, iphs=-1
    {"beadsynt", S(BEADSYNT), TR, 3, "a", ".[].[].[]jpPPjj",
     (SUBR)beadsynt_init_array, (SUBR)beadsynt_perf },


    // tabrowlin krow, ifnsrc, ifndest, inumcols,
    //                 ioffset=0, istart=0, iend=0, istep=1
    {"tabrowlin", S(TABROWCOPY), 0, 3, "", "kiiiooop",
     (SUBR)tabrowcopy_init, (SUBR)tabrowcopyk },

    // kOut[]  tabrowlin krow, ifnsrc, inumcols,
    //                   ioffset=0, istart=0, iend=0, istep=1
    {"getrowlin", S(TABROWCOPY), 0, 3, "k[]", "kiiooop",
     (SUBR)tabrowcopyarr_init, (SUBR)tabrowcopyarr_k},

    // kOut[] getrowlin kMtrx[], krow, kstart=0, kend=0, kstep=1
    {"getrowlin", S(GETROWLIN), 0, 3, "k[]", "k[]kOOP",
     (SUBR)getrowlin_init, (SUBR)getrowlin_k },
};

LINKAGE
