/*

beosc.c

beosc - Bandwidth enhanced oscillator
beadsynt - Bandwidth enhanced oscillator bank for additive synthesis

(C) 2017 Eduardo Moguillansky

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as published by
the Free Software Foundation.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
*/

/*
 
Loris UGens adapted from Loris 1.8

Loris is Copyright (c) 1999-2016 by Kelly Fitz and Lippold Haken

Loris is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY, without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
file COPYING or the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

loris@cerlsoundgroup.org
http://www.cerlsoundgroup.org/Loris/
 
*/

/*
 
BEASTmulch UGens - Supercollider
Copyright (C) 2009 Scott Wilson

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 

http://www.beast.bham.ac.uk/research/mulch.shtml
beastmulch-info@contacts.bham.ac.uk

*/

#include <math.h>
// #include "/usr/local/include/csound/csdl.h"
#include "csdl.h"

// -------------------------------------------------------------------------

#define UInt32toFlt(x) ((double)(x) * (1.0 / 4294967295.03125))

#define unirand(c) ((MYFLT) UInt32toFlt(csoundRandMT(&((c)->randState_))))

#define unirand2(cs,seed) ((MYFLT) (cs->Rand31(&seed)-1) / FL(2147483645.0))

// 1 / 2pi
#define RTWOPI 0.1591549430918953357688837634

#define SAMPLE_ACCURATE \
uint32_t offset = p->h.insdshead->ksmps_offset;                   \
uint32_t early  = p->h.insdshead->ksmps_no_end;                   \
if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));    \
if (UNLIKELY(early)) {                                            \
    nsmps -= early;                                               \
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));               \
}                                                                 \

#define MSG(m) (csound->Message(csound, Str(m)))
#define MSGF(m, ...) (csound->Message(csound, Str(m), __VA_ARGS__))

#define INITERR(m) (csound->InitError(csound, Str(m)))

#define PERFERROR(m) (csound->PerfError(csound, p->h.insdshead, Str(m)))

#define RET_INITERR(m) \
p->inerr = 1; \
return csound->InitError(csound, Str(m)) \


// -------------------------------------------------------------------------

/*

    fast log

*/

inline float fastlog2 (float x) {
    union { float f; uint32_t i; } vx = { x };
    union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
    float y = vx.i;
    y *= 1.1920928955078125e-7f;

    return y - 124.22551499f
           - 1.498030302f * mx.f 
           - 1.72587999f / (0.3520887068f + mx.f);
}

inline float fastlogf (float x) {
    return 0.69314718f * fastlog2 (x);
}

inline MYFLT fastlog(MYFLT x) {
    return FL(0.6931471805599453) * fastlog2(x);
}


// uniform noise

inline MYFLT FastRandFloat(uint32_t *seedptr) {
    // taken from csoundRand31, gives floats between 0-1
    uint64_t  tmp1;
    uint32_t  tmp2;
    /* x = (742938285 * x) % 0x7FFFFFFF */
    tmp1 = (uint64_t) ((int32_t) (*seedptr) * (int64_t) 742938285);
    tmp2 = (uint32_t) tmp1 & (uint32_t) 0x7FFFFFFF;
    tmp2 += (uint32_t) (tmp1 >> 31);
    tmp2 = (tmp2 & (uint32_t) 0x7FFFFFFF) + (tmp2 >> 31);
    (*seedptr) = tmp2;
    return (MYFLT)(tmp2 - 1) / FL(2147483648.0);
} 


/*

Gaussian noise

*/

typedef struct {
    MYFLT gset;
    int iset;
    uint32_t seed;
} GaussianState;


inline MYFLT gaussian_normal(GaussianState *gs) {
    if(gs->iset) {
        gs->iset = 0;
        return gs->gset;
    }
    gs->iset = 1;
    MYFLT v1 = FL(2.0) * FastRandFloat(&(gs->seed)) - FL(1.0);
    MYFLT v2 = FL(2.0) * FastRandFloat(&(gs->seed)) - FL(1.0);
    MYFLT r = v1*v1 + v2*v2;
    while(r >= 1.0) {
        v1 = v2;
        v2 = FL(2.0) * FastRandFloat(&(gs->seed)) - FL(1.0);
        r = v1*v1 + v2*v2;
    }
    MYFLT fac = r == FL(0) ? FL(0) : sqrt(FL(-2) * fastlog(r)/r);
    gs->gset = v1*fac;
    return v2*fac;
}

static MYFLT* gaussians = NULL;

// #define GAUSSIANS_SIZE 65536
#define GAUSSIANS_SIZE 32768

void gaussians_init(uint32_t seed) {
    if(gaussians != NULL)
        return;
    int size = GAUSSIANS_SIZE;
    unsigned int i;
    GaussianState gs = { .gset=0, .iset=0, .seed=seed };
    MYFLT *g = malloc(sizeof(MYFLT)*size);
    for(i=0; i<size; i++) {
        g[i] = gaussian_normal(&gs);
    }
    gaussians = g;
}

inline MYFLT gaussians_get(uint32_t *seedp) {
    const int maxidx = GAUSSIANS_SIZE - 1;
    unsigned int idx = (unsigned int)(FastRandFloat(seedp) * maxidx);
    return gaussians[idx];
}

/* from Opcodes/arrays.c, original name: tabensure */
static inline void arrayensure(CSOUND *csound, ARRAYDAT *p, int size)
{
    if (p->data==NULL || p->dimensions == 0 || (p->dimensions==1 && p->sizes[0] < size)) {
        size_t ss;
        if (p->data == NULL) {
            CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
            p->arrayMemberSize = var->memBlockSize;
        }
        ss = p->arrayMemberSize*size;
        if (p->data==NULL) {
            p->data = (MYFLT*)csound->Calloc(csound, ss);
        }
        else {
            p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
        }
        p->dimensions = 1;
        p->sizes = (int*)csound->Malloc(csound, sizeof(int));
        p->sizes[0] = size;
    }
}

/*

Integer phase oscillator with/without interpolation,
adapted from Supercollider

*/

#define xlobits 14
#define xlobits1 13

inline float PhaseFrac(uint32_t inPhase) {
    union { uint32_t itemp; float ftemp; } u;
    u.itemp = 0x3F800000 | (0x007FFF80 & ((inPhase)<<7));
    return u.ftemp - 1.f;
}

inline float PhaseFrac1(uint32_t inPhase) {
    union { uint32_t itemp; float ftemp; } u;
    u.itemp = 0x3F800000 | (0x007FFF80 & ((inPhase)<<7));
    return u.ftemp;
}

inline MYFLT lookupi1(const MYFLT* table0, const MYFLT* table1, 
                      int32_t pphase, int32_t lomask) {
    MYFLT pfrac = PhaseFrac(pphase);
    uint32_t index = ((pphase >> xlobits1) & lomask);
    MYFLT val1 = *(const MYFLT*)((const char*)table0 + index);
    MYFLT val2 = *(const MYFLT*)((const char*)table1 + index);
    MYFLT out = val1 + (val2 - val1) * pfrac;
    return out;
}

inline MYFLT lookup(const MYFLT *table, int32_t phase, int32_t mask) {
    uint32_t index = ((phase >> xlobits1) & mask);
    return  *(const MYFLT*)((const char*)table + index);
}

inline MYFLT 
cs_lookupi(const MYFLT* ftbl, int32_t phs, int32_t lobits, int32_t lomask, MYFLT lodiv) {
    MYFLT fract = (MYFLT)((phs & lomask) * lodiv);
    const MYFLT* ftbl0 = ftbl + (phs >> lobits);
    MYFLT v1 = ftbl0[0];
    return v1 + (ftbl0[1] - v1)*fract;
}

// -------------------------------------------------------------

/*

    beoscil

    aout beoscil xfreq, kbw [, ifn=-1, iphase=0, iflags=1 ]
    
    ifn: like oscil, default is -1, a sine wave. 
    iphase: like oscil, a float value between 0 - 2pi (default=0)
    iflags: 0-1 = uniform or gaussian noise (default=gaussian)
            +2  = table lookup with linear interpolation

    The original Loris implementation uses gaussian normal noise,
    the Supercollider port (BEOsc) uses uniform noise.

*/

typedef struct {
    OPDS h;
    MYFLT *out, *xfreq, *kbw, *ifn, *iphs, *iflags;
    MYFLT lastfreq;
    int32 phase;
    int32 lomask;
    MYFLT cpstoinc, radtoinc;
    FUNC *ftp;
    MYFLT x1, x2, x3; // MA
    MYFLT y1, y2, y3; // AR
    int flags;
    GaussianState gs;
} BEOSC;


static int beosc_init(CSOUND *csound, BEOSC *p) {
    FUNC        *ftp;
    MYFLT sampledur = 1 / csound->GetSr(csound);
    ftp = csound->FTFind(csound, p->ifn);
    if (UNLIKELY(ftp == NULL)) {
        return NOTOK;
    }
    p->ftp = ftp;
    uint32_t tabsize = ftp->flen;
    p->radtoinc = tabsize * (RTWOPI * 65536.);
    p->cpstoinc = tabsize * sampledur * 65536;
    p->lomask = (tabsize - 1) << 3; 
    p->phase = fabs(fmod(*p->iphs, TWOPI)) * p->radtoinc;
    p->flags = (int)(*p->iflags);
    p->lastfreq = *p->xfreq;
    p->gs.seed = csound->GetRandomSeedFromTime();
    p->gs.iset = 0;
    return OK; 
}

static int beosc_kkiii(CSOUND *csound, BEOSC *p) {
    uint32_t n, nsmps = CS_KSMPS;
    FUNC *ftp = p->ftp;
    MYFLT *out = p->out;
    MYFLT freqin = *p->xfreq;
    MYFLT bwin = *p->kbw;
    MYFLT *table0 = ftp->ftable;
    MYFLT *table1 = table0 + 1;

    int32 phase = p->phase;
    int32 lomask = p->lomask;
    
    int32 phaseinc = (int32)(p->cpstoinc * freqin); 

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
    
    SAMPLE_ACCURATE

    uint32_t seed;
    GaussianState *gsptr;

    switch (p->flags) {
    case 0:    // gaussian noise, no interp.
        seed = p->gs.seed;
        for (n=offset; n<nsmps; n++) {
            x0 = x1; x1 = x2; x2 = x3;
            x3 = (FastRandFloat(&seed) * FL(2.0) - FL(1.0)) * FL(0.00012864661681256); // kelly uses 6. / GAIN
            y0 = y1; y1 = y2; y2 = y3;
            y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                 (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            out[n] = lookup(table0, phase, lomask)
                     * (bw1 + ( y3 * bw2 ));
            phase += phaseinc;
        }
        p->gs.seed = seed;
        break;
    case 1:     // uniform noise, no interp
        gsptr = &(p->gs);
        for (n=offset; n<nsmps; n++) {
            x0 = x1; x1 = x2; x2 = x3;
            x3 = gaussian_normal(gsptr) * FL(0.00012864661681256); // kelly uses 6. / GAIN
            y0 = y1; y1 = y2; y2 = y3;
            y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                 (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            out[n] = lookup(table0, phase, lomask) \
                     * (bw1 + ( y3 * bw2 ));
            phase += phaseinc;
        }
        break;
    case 2:    
        seed = p->gs.seed;
        for (n=offset; n<nsmps; n++) {
            x0 = x1; x1 = x2; x2 = x3;
            x3 = (FastRandFloat(&seed) * FL(2.0) - FL(1.0)) * FL(0.00012864661681256); // kelly uses 6. / GAIN
            y0 = y1; y1 = y2; y2 = y3;
            y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                 (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            out[n] = lookupi1(table0, table1, phase, lomask)
                     * (bw1 + ( y3 * bw2 ));
            phase += phaseinc;
        }
        p->gs.seed = seed;
        break;
    case 3:    // uniform noise, interp
        gsptr = &(p->gs);
        for (n=offset; n<nsmps; n++) {
            x0 = x1; x1 = x2; x2 = x3;
            x3 = gaussian_normal(gsptr) * FL(0.00012864661681256); // kelly uses 6. / GAIN
            y0 = y1; y1 = y2; y2 = y3;
            y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                 (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            out[n] = lookupi1(table0, table1, phase, lomask) \
                     * (bw1 + ( y3 * bw2 ));
            phase += phaseinc;
        }
        break;
    }
    p->phase = phase;
    p->x1 = x1; p->x2 = x2; p->x3 = x3;
    p->y1 = y1; p->y2 = y2; p->y3 = y3; 
    return OK;
}

static int beosc_akiii(CSOUND *csound, BEOSC *p) {
    uint32_t n, nsmps = CS_KSMPS;
    FUNC *ftp = p->ftp;
    MYFLT *out = p->out;
    MYFLT *freqptr = p->xfreq;
    MYFLT bwin = *p->kbw;
    MYFLT *table0 = ftp->ftable;
    MYFLT *table1 = table0 + 1;
    MYFLT noise;

    int32 phase = p->phase;
    int32 lomask = p->lomask;
    
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
    
    SAMPLE_ACCURATE

    uint32_t seed;

    MYFLT freq, 
          cpstoinc = p->cpstoinc;

    GaussianState *gsptr;

    switch (p->flags) {
    case 0:    // uniform noise, no interp.
        seed = p->gs.seed;
        for (n=offset; n<nsmps; n++) {
            x0 = x1; x1 = x2; x2 = x3;
            noise = FastRandFloat(&seed) * FL(2.0) - FL(1.0);
            x3 = noise * FL(0.00012864661681256); // kelly uses 6. / GAIN
            y0 = y1; y1 = y2; y2 = y3;
            y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                 (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            out[n] = lookup(table0, phase, lomask)
                     * (bw1 + ( y3 * bw2 ));
            freq = freqptr[n];
            phase += (int32)(cpstoinc * freq);
        }
        p->gs.seed = seed;
        break;
    case 1:     // gaussian noise, no interp
        gsptr = &(p->gs);
        for (n=offset; n<nsmps; n++) {
            x0 = x1; x1 = x2; x2 = x3;
            noise = gaussian_normal(gsptr);
            x3 = noise * FL(0.00012864661681256); // kelly uses 6. / GAIN
            y0 = y1; y1 = y2; y2 = y3;
            y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                 (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            out[n] = lookup(table0, phase, lomask) \
                     * (bw1 + ( y3 * bw2 ));
            freq = freqptr[n];
            phase += (int32)(cpstoinc * freq);
        }
        break;
    case 2:    // + interp
        seed = p->gs.seed;
        for (n=offset; n<nsmps; n++) {
            x0 = x1; x1 = x2; x2 = x3;
            noise = FastRandFloat(&seed) * FL(2.0) - FL(1.0);
            x3 = noise * FL(0.00012864661681256); // kelly uses 6. / GAIN
            y0 = y1; y1 = y2; y2 = y3;
            y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                 (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            out[n] = lookupi1(table0, table1, phase, lomask)
                     * (bw1 + ( y3 * bw2 ));
            freq = freqptr[n];
            phase += (int32)(cpstoinc * freq);
        }
        p->gs.seed = seed;
        break;
    case 3:    // + interp
        gsptr = &(p->gs);
        for (n=offset; n<nsmps; n++) {
            x0 = x1; x1 = x2; x2 = x3;
            noise = gaussian_normal(gsptr);
            x3 = noise * FL(0.00012864661681256); // kelly uses 6. / GAIN
            y0 = y1; y1 = y2; y2 = y3;
            y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                 (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
            out[n] = lookupi1(table0, table1, phase, lomask) \
                     * (bw1 + ( y3 * bw2 ));
            freq = freqptr[n];
            phase += (int32)(cpstoinc * freq);
        }
        break;
    }

    p->phase = phase;
    p->x1 = x1; p->x2 = x2; p->x3 = x3;
    p->y1 = y1; p->y2 = y2; p->y3 = y3; 
    return OK;
}


/*
----------------------------------------------------------------

beadsynt - Band enhanced oscillator bank

aout   beadsynt  kfreq, kbw, ifreqfn, iampfn, ibwfn, icnt, iwfn=-1, iphs=-1, iflags=0

kfreq:   frequency scaling, all frequencies are multiplied by this factor
kbw:     bandwidth scaling, bandwidths are multiplied by this factor
ifreqfn: a table containing frequencies for each oscillator.
iampfn:  a table containing amplitudes for each oscillator.
ibwfn:   a table containing bandwidths for each oscillator.
icnt:    number of oscillators. All three tables must be at least this big
iwfn:    a table containing one wave cycle to be used for the oscillators.
         -1 to use the default sine table
iphs:    Phase for the oscillators, indicated as follows:
         *  -1: randomize phases
         * 0-1: set the initial phase of all oscillators to this value
         * >=1: the table number (an int) containing the initial phase
                for each oscillator (must contain at least icnt values)
iflags:  0-1  => noise type (0=uniform, 1=gaussian) 
         +2   => table lookup interpolation
         +4   => freq interpolation 

aout   beadsynt kreq, kbw, kFreq[], kAmp[], kBw[], icnt, iwfn=-1, iphs=-1, iflags=0

----------------------------------------------------------------
*/

typedef struct {
    MYFLT x1, x2, x3;
    MYFLT y1, y2, y3;
} FILTCOEFS;

void befilter_init(FILTCOEFS *filt) {
    filt->x1 = 0;
    filt->x2 = 0;
    filt->x3 = 0;
    filt->y1 = 0;
    filt->y2 = 0;
    filt->y3 = 0;
}

typedef struct {
    OPDS    h;
    MYFLT   *out, *kfreq, *kbw;
    void    *ifreqtbl, *iamptbl, *ibwtbl;
    MYFLT   *icnt, *ifn, *iphs, *iflags;
    GaussianState gs;
    FUNC    *ftp;
    MYFLT *freqs;
    MYFLT *amps;
    MYFLT *bws;
    int     count;
    int     inerr;
    AUXCH   lphs;
    AUXCH   pamp;
    AUXCH   filtcoefs;
    AUXCH   pfreq;
    MYFLT cpstoinc;
    uint32_t seed;
} BEADSYNT;

static int beadsynt_init_common(CSOUND *csound, BEADSYNT *p) {
    FILTCOEFS *filtcoefs;
    int32 *lphs;
    unsigned int c, count;

    MYFLT iphs = *p->iphs;
    MYFLT sr = csound->GetSr(csound);
    p->inerr = 0;
    count = (unsigned int)*p->icnt;
    p->count = count = count < 1 ? 1 : count;
    // corresponds to csound->sicvt. FMAXLEN depends on B64BIT being defined
    p->cpstoinc = FMAXLEN / sr;
    p->gs.seed = p->seed = csound->GetRandomSeedFromTime();
    p->gs.iset = 0;  p->gs.gset = 0;
    gaussians_init(csound->GetRandomSeedFromTime());

    if (p->lphs.auxp==NULL || p->lphs.size < sizeof(int32)*count)
        csound->AuxAlloc(csound, sizeof(int32)*count, &p->lphs);
    lphs = (int32*)p->lphs.auxp;

    if (iphs < 0) {
        uint32_t seed = csound->GetRandomSeedFromTime();
        for (c=0; c<count; c++) {
            lphs[c] = (int32)(FastRandFloat(&seed) * FMAXLEN) & PHMASK;
        }
        MSG("beadsynt: init phase with random values\n");
    } else if (iphs <= 1) {   // between 0 and 1, use this number as phase
        for (c=0; c<count; c++) {
            lphs[c] = ((int32)(iphs * FMAXLEN)) & PHMASK;
        }
        MSG("beadsynt: init phase with fixed value\n");
    } else {  // iphs is the number of a table containing the phases
        FUNC *phasetp = csound->FTFind(csound, p->iphs);
        if (phasetp == NULL) {
            RET_INITERR("beadsynt: phasetable not found");
        }
        for (c=0; c<count; c++) {
            MYFLT ph = phasetp->ftable[c];
            lphs[c] = ((int32)(ph * FMAXLEN)) & PHMASK;
        }
        MSG("beadsynt: init phase with func table\n");
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
        if (p->pfreq.auxp==NULL || p->pfreq.size < (uint32_t)(sizeof(MYFLT)*p->count))
            csound->AuxAlloc(csound, sizeof(MYFLT)*p->count, &p->pfreq);
        // init freqs to current table contents
        MYFLT *prevfreqs = (MYFLT*)p->pfreq.auxp;
        MYFLT *freqs = p->freqs;
        MYFLT freqmul = *p->kfreq;
        for (c=0; c<p->count;c++) {
            prevfreqs[c] = freqs[c] * freqmul;
        }
    }
    return OK;
}

static int beadsynt_init(CSOUND *csound, BEADSYNT *p) {
    FUNC *ftp;
    unsigned int count = (unsigned int)*p->icnt;

    p->ftp = ftp = csound->FTFind(csound, p->ifn);
    if (ftp == NULL) {
        RET_INITERR("beadsynt: wavetable not found!");
    }
    ftp = csound->FTnp2Find(csound, (MYFLT *)p->iamptbl);
    if (ftp == NULL) {
        RET_INITERR("beadsynt: amptable not found!");
    }
    if (ftp->flen < count) {
        RET_INITERR("beadsynt: partial count is greater than amptable size!");
    }
    p->amps = ftp->ftable;

    ftp = csound->FTnp2Find(csound, (MYFLT *)p->ifreqtbl);
    if (ftp == NULL) {
        RET_INITERR("beadsynt: freqtable not found!");
    }
    if (ftp->flen < count) {
        RET_INITERR("beadsynt: partial count is greater than freqtable size!");
    }
    p->freqs = ftp->ftable;

    ftp = csound->FTnp2Find(csound, (MYFLT *)p->ibwtbl);
    if (ftp == NULL) {
        RET_INITERR("beadsynt: bandwidth table not found!");
    }
    if (ftp->flen < count) {
        RET_INITERR("beadsynt: partial count is greater than bandwidthe size!");
    }
    p->bws = ftp->ftable;

    return beadsynt_init_common(csound, p);
}

// FMAXLEN = MYFLT 0x40000000
// PHMASK = 0x3fffffff

static int beadsynt_perf(CSOUND *csound, BEADSYNT *p) {
    FUNC  *ftp;
    MYFLT *out, *ftpdata, *freqs, *amps, *bws, *prevamps, *prevfreqs;
    MYFLT freq, freqmul, freqnow, freqinc;
    MYFLT amp, ampnow, ampinc, bwmul, bwin, bw1, bw2;
    MYFLT cpstoinc, sample, lodiv;
    int32 phs, inc, lobits, lomask;
    int32 *lphs;
    int flags;
    unsigned int c, count;
    MYFLT x0, x1, x2, x3, y0, y1, y2, y3;
    FILTCOEFS *coefs;
    uint32_t seed,
             offset = p->h.insdshead->ksmps_offset,
             early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(p->inerr)) {
        return INITERR("beadsynt: not initialised");
    }

    ftp      = p->ftp;
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
    
    freqs = p->freqs;
    amps  = p->amps;
    bws   = p->bws;
    
    lphs      = (int32*)p->lphs.auxp;
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
        amp = amps[c];
        if(ampnow == 0 && amp == 0)   // skip silent partials
            continue; 
        bwin = bws[c] * bwmul;
        bwin = bwin < 0 ? 0 : (bwin > 1 ? 1 : bwin);  
        bw1  = sqrt(FL(1.0) - bwin);
        bw2  = sqrt(FL(2.0) * bwin);
        
        freq = freqs[c] * freqmul;
        inc  = (int32) (freq * cpstoinc);
        phs  = lphs[c];
        ampinc = (amp - ampnow) * CS_ONEDKSMPS;

        if(LIKELY(bwin != 0)) {
            x1 = coefs->x1; x2 = coefs->x2; x3 = coefs->x3;
            y1 = coefs->y1; y2 = coefs->y2; y3 = coefs->y3;
            switch(flags) {
            // 0-1=uniform | gauss. noise, +2=osc lookup with linear interp, +4=freq. interp
            case 0:  // 000
                for (n=offset; n<nsmps; n++) {
                    x0 = x1; x1 = x2; x2 = x3;
                    x3 = FastRandFloat(&seed) * FL(2) - FL(1);
                    x3 *= FL(0.00012864661681256);
                    y0 = y1; y1 = y2; y2 = y3;
                    y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                             (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
                    sample = *(ftpdata + (phs >> lobits)) * ampnow;
                    out[n] += sample * (bw1 + (y3*bw2));
                    phs += inc;
                    phs &= PHMASK;
                    ampnow += ampinc;
                }
                break;
            case 1:  // 001
                for (n=offset; n<nsmps; n++) {
                    x0 = x1; x1 = x2; x2 = x3;
                    // x3 = gaussian_normal(gsptr);
                    x3 = gaussians_get(&seed);
                    x3 *= FL(0.00012864661681256);
                    y0 = y1; y1 = y2; y2 = y3;
                    y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                             (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
                    sample = *(ftpdata + (phs >> lobits)) * ampnow;
                    out[n] += sample * (bw1 + (y3*bw2));
                    phs += inc; phs &= PHMASK;
                    ampnow += ampinc;
                }
                break;
            case 2:  // 010
                for (n=offset; n<nsmps; n++) {
                    x0 = x1; x1 = x2; x2 = x3;
                    x3 = FastRandFloat(&seed) * FL(2) - FL(1);
                    x3 *= FL(0.00012864661681256);
                    y0 = y1; y1 = y2; y2 = y3;
                    y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                             (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
                    sample = cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
                    out[n] += sample * (bw1 + (y3*bw2));
                    phs += inc; phs &= PHMASK;
                    ampnow += ampinc;
                }
                break;
            case 3:  // 011
                // seed = p->gs.seed;
                for (n=offset; n<nsmps; n++) {
                    x0 = x1; x1 = x2; x2 = x3;
                    x3 = gaussians_get(&seed);
                    x3 *= FL(0.00012864661681256);
                    y0 = y1; y1 = y2; y2 = y3;
                    y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                             (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
                    sample = cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
                    out[n] += sample * (bw1 + (y3*bw2));
                    phs += inc; phs &= PHMASK;
                    ampnow += ampinc;
                }
                break;
            case 4:  // 100
                freqnow = prevfreqs[c];
                freqinc = (freq - freqnow) * CS_ONEDKSMPS;
                // seed = p->gs.seed;
                for (n=offset; n<nsmps; n++) {
                    x0 = x1; x1 = x2; x2 = x3;
                    x3 = FastRandFloat(&seed) * FL(2) - FL(1);
                    x3 *= FL(0.00012864661681256);
                    y0 = y1; y1 = y2; y2 = y3;
                    y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                             (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
                    sample = *(ftpdata + (phs >> lobits)) * ampnow;
                    out[n] += sample * (bw1 + (y3*bw2));
                    freqnow += freqinc;
                    phs += (int32)(cpstoinc * freqnow); phs &= PHMASK;
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
                    x3 = gaussians_get(&seed);
                    x3 *= FL(0.00012864661681256);
                    y0 = y1; y1 = y2; y2 = y3;
                    y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                             (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
                    sample = *(ftpdata + (phs >> lobits)) * ampnow;
                    out[n] += sample * (bw1 + (y3*bw2));
                    freqnow += freqinc;
                    phs += (int32)(cpstoinc * freqnow); phs &= PHMASK;
                    ampnow += ampinc;
                }
                prevfreqs[c] = freq;
                break;
            case 6:  // 110
                freqnow = prevfreqs[c];
                freqinc = (freq - freqnow) * CS_ONEDKSMPS;
                for (n=offset; n<nsmps; n++) {
                    x0 = x1; x1 = x2; x2 = x3;
                    x3 = FastRandFloat(&seed) * FL(2) - FL(1);
                    x3 *= FL(0.00012864661681256);
                    y0 = y1; y1 = y2; y2 = y3;
                    y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                             (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
                    sample = cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
                    out[n] += sample * (bw1 + (y3*bw2));
                    freqnow += freqinc;
                    phs += (int32)(cpstoinc * freqnow); phs &= PHMASK;
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
                    x3 = gaussians_get(&seed);
                    x3 *= FL(0.00012864661681256);
                    y0 = y1; y1 = y2; y2 = y3;
                    y3 = (x0 + x3) + (FL(3) * (x1 + x2)) + (FL(0.9320209047) * y0) + \
                             (FL(-2.8580608588) * y1) + (FL(2.9258684253) * y2);
                    sample = cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
                    out[n] += sample * (bw1 + (y3*bw2));
                    freqnow += freqinc;
                    phs += (int32)(cpstoinc * freqnow); phs &= PHMASK;
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
                    phs += inc; phs &= PHMASK; ampnow += ampinc;
                }
                break;
            case 2:  // 010
            case 3:  // 011
                for (n=offset; n<nsmps; n++) {
                    out[n] += cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
                    phs += inc; phs &= PHMASK; ampnow += ampinc;
                }
                break;
            case 4:  // 100
            case 5:  // 101
                freqnow = prevfreqs[c];
                freqinc = (freq - freqnow) * CS_ONEDKSMPS;
                for (n=offset; n<nsmps; n++) {
                    out[n] += *(ftpdata + (phs >> lobits)) * ampnow;
                    freqnow += freqinc;
                    phs += (int32)(cpstoinc * freqnow); phs &= PHMASK; ampnow += ampinc;
                }
                prevfreqs[c] = freq;
                break;
            case 6:  // 110
            case 7:  // 111
                freqnow = prevfreqs[c];
                freqinc = (freq - freqnow) * CS_ONEDKSMPS;
                for (n=offset; n<nsmps; n++) {
                    out[n] += cs_lookupi(ftpdata, phs, lobits, lomask, lodiv) * ampnow;
                    freqnow += freqinc;
                    phs += (int32)(cpstoinc * freqnow); phs &= PHMASK; ampnow += ampinc;
                }
                prevfreqs[c] = freq;
                break;
            }
        }
        prevamps[c] = amp;
        lphs[c]     = phs;
        coefs++;
    }
    p->seed = seed;    
    return OK;
}


/////////////////////////////////////////////////////
/*

    tabrowlin

    Assuming a 2d (flattened) function table containing
    multiple rows of sampled streams (for instance, the amplitudes
    of a set of oscilators), extract one row of that data
    with a possible linear interpolation between adjacent rows, if
    row is not a whole number.

    tabrowlin krow, ifnsrc, ifndest, inumcols, ioffset=0, istart=0, iend=0, istep=1

    If reading out of bounds a PerformanceError will be raised. Because we interpolate
    between rows, the last row that can be read is 

    maxrow = (ftlen(ifnsrc)-ioffset)/inumcols - 2
*/

typedef struct {
    OPDS h;
    MYFLT *krow, *ifnsrc, *ifndest, *inumcols, *ioffset, *istart, *iend, *istep, *iclip;
    MYFLT* tabsource;
    MYFLT* tabdest;
    MYFLT maxrow;
    int tabsourcelen;
    int tabdestlen;
    int end;
} TABROWCOPY;

// idx = ioffset + row * inumcols + n*step, while idx < iend

static int tabrowcopy_init(CSOUND* csound, TABROWCOPY* p)
{
    FUNC* ftp;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifnsrc)) == NULL)) {
        return INITERR("tabrowcopy: incorrect table number");
    }
    p->tabsource = ftp->ftable;
    p->tabsourcelen = ftp->flen;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifndest)) == NULL)) {
        return INITERR("tabrowcopy: incorrect table number");
    }
    p->tabdest = ftp->ftable;
    p->tabdestlen = ftp->flen;

    int end = *p->iend;
    if(end > *p->inumcols)
        return INITERR("tabrowcopy: iend can't be bigger than numcols");

    if(end == 0)
        end = *p->inumcols;

    p->end = end;
    
    int numcols_to_copy = (int)((end - *p->istart) / *p->istep);
    if (numcols_to_copy > p->tabdestlen)
        return INITERR("tabrowcopy: Destination table too small");

    p->maxrow = (p->tabsourcelen - *p->ioffset) / *p->inumcols - FL(2);

    MSGF("tabrowcopy: Max. Row = %f \n", p->maxrow);
    return OK;
}


static int tabrowcopyk(CSOUND* csound, TABROWCOPY* p) {
    int i;
    MYFLT x0, x1;
    MYFLT row = *p->krow;
    int row0 = (int)row;
    MYFLT delta = row - row0;
    int numcols = *p->inumcols;
    int offset = *p->ioffset;
    int start = *p->istart;
    int end = p->end;
    int step = *p->istep;
    int tabsourcelen = p->tabsourcelen;

    MYFLT* tabsource = p->tabsource;
    MYFLT* tabdest = p->tabdest;
    
    int idx0 = offset + numcols * row0 + start;
    int idx1 = idx0 + (end-start);
    int j = 0;

    if(UNLIKELY(row < 0)) {
        return PERFERROR("tabrowcopy: krow can't be negative");       
    }
    
    if (LIKELY(delta != 0)) {
        if (UNLIKELY(idx1+numcols >= tabsourcelen)) {
            return PERFERROR("tabrowcopy: tab off end");       
        }
        for (i=idx0; i<idx1; i+=step) {
            x0 = tabsource[i];
            x1 = tabsource[i + numcols];
            tabdest[j++] = x0 + (x1-x0)*delta;
        }
    } else {
        if (UNLIKELY(idx1 >= tabsourcelen)) {
            return PERFERROR("tabrowcopy: tab off end");       
        }
        for (i=idx0; i<idx1; i+=step) {
            tabdest[j++] = tabsource[i];
        }
    }
    return OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////

/*

Input types: 
  * a, k, s, i, w, f, 
  * o (optional i-rate, default to 0), 
  * p (opt, default to 1), 
  * q (opt, 10),  
  * v(opt, 0.5), 
  * j(opt, -1), 
  * h(opt, 127), 
  * y (multiple inputs, a-type),
  * z (multiple inputs, k-type), 
  * Z (multiple inputs, alternating k- and a-types), 
  * m (multiple inputs, i-type), 
  * M (multiple inputs, any type) 
  * n (multiple inputs, odd number of inputs, i-type).
*/

#define S(x)    sizeof(x)

static OENTRY localops[] = {
    {"beosc",  S(BEOSC),      0, 5, "a", "kkjop",     (SUBR)beosc_init, NULL, (SUBR)beosc_kkiii },
    {"beosc",  S(BEOSC),      0, 5, "a", "akjop",     (SUBR)beosc_init, NULL, (SUBR)beosc_akiii },

    // aout beadsynt kfreq, kbw, ifreqft, iampft, ibwft, inumosc, ifn=-1, iphs=-1, iflags=0
    {"beadsynt", S(BEADSYNT), 0, 5, "a", "kkiiiijjp", (SUBR)beadsynt_init, NULL, (SUBR)beadsynt_perf },

    // aout beadsynt kfreq, kbw, kFreq[], kAmp[], kBw[], inumosc, ifn=-1, iphs=-1, iflags=0
    // {"beadsynt", S(BEADSYNT), 0, 5, "a", "kkk[]k[]k[]ijjp", (SUBR)beadsynt_init, NULL, (SUBR)beadsynt_perf },

    // tabrowlin krow, ifnsrc, ifndest, inumcols, ioffset=0, istart=0, iend=0, istep=1
    {"tabrowlin", S(TABROWCOPY), 0, 3, "",  "kiiiooop",  (SUBR)tabrowcopy_init, (SUBR)tabrowcopyk },

};

LINKAGE
