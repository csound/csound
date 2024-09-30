/*

    scugens.c

    Copyright (C) 2017  Eduardo Moguillansky

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

#include <math.h>
#include "emugens_common.h"

#define LOG001 FL(-6.907755278982137)
#define CALCSLOPE(next,prev,nsmps) ((next - prev)/nsmps)

#define SAMPLE_ACCURATE \
    uint32_t n, nsmps = CS_KSMPS;                                    \
    MYFLT *out = p->out;                                             \
    uint32_t offset = p->h.insdshead->ksmps_offset;                  \
    uint32_t early = p->h.insdshead->ksmps_no_end;                   \
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));   \
    if (UNLIKELY(early)) {                                           \
        nsmps -= early;                                              \
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));              \
    }                                                                \

/* #define ZXP(z) (*(z)++) */

#define register_deinit(csound, p, func) \
    csound->RegisterDeinitCallback(csound, p, (int32_t(*)(CSOUND*, void*))(func))

static inline MYFLT
zapgremlins(MYFLT x) {
    MYFLT absx = fabs(x);
    // very small numbers fail the first test, eliminating denormalized numbers
    //    (zero also fails the first test, but that is OK since it returns
    // zero.)
    // very large numbers fail the second test, eliminating infinities
    // Not-a-Numbers fail both tests and are eliminated.
    return (absx > (MYFLT)1e-15 && absx < (MYFLT)1e15) ? x : (MYFLT)0.0;
}

static inline MYFLT
sc_wrap(MYFLT in, MYFLT lo, MYFLT hi) {
    MYFLT range;
    // avoid the divide if possible
    if(in >= hi) {
        range = hi - lo;
        in -= range;
        if (in < hi) return in;
    } else if(in < lo) {
        range = hi - lo;
        in += range;
        if(in >= lo) return in;
    } else return in;

    if (hi == lo) return lo;
    return in - range * FLOOR((in - lo) / range);
}




/*

  lag

  This is essentially the same as OnePole except that instead of
  supplying the coefficient directly, it is calculated from a 60 dB lag time.
  This is the time required for the filter to converge to within 0.01%
  of a value. This is useful for smoothing out control signals.

  ksmooth = lag(kx, klagtime, [initialvalue])
  asmooth = lag(ain, klagtime, [initialvalue])

  NB: if no initialvalue is given, the first value of the input signal
      is used. To reproduce the previous behaviour, use 0 as initialvalue

*/

typedef struct {
    OPDS  h;
    MYFLT *out, *in, *lagtime, *initial_value;
    int32_t started;
    MYFLT lag, b1, y1;
    MYFLT sr;
} LAG0;

static int32_t lag0_init_no_initial_value(CSOUND *csound, LAG0 *p) {
    IGN(csound);
    p->lag = -1;
    p->started = 0;
    p->b1 = FL(0.0);
    p->sr = CS_EKR;
    p->y1 = 0;
    return OK;
}

static int32_t lag0_init_initial_value(CSOUND *csound, LAG0 *p) {
    IGN(csound);
    p->lag = -1;
    p->started = 1;
    p->y1 = *(p->initial_value);
    p->b1 = FL(0.0);
    p->sr = CS_EKR;
    return OK;
}


static int32_t lag0k_next(CSOUND *csound, LAG0 *p) {
    IGN(csound);
    MYFLT y1, b1;
    MYFLT y0 = *p->in;

    if(UNLIKELY(em_isinfornan(y0))) {
        return PERFERRF("Non-finite or nan value detected: %f", y0);
    }

    MYFLT lag = *p->lagtime;

    if(LIKELY(p->started))
        y1 = p->y1;
    else {
        p->started = 1;
        y1 = y0;
    }

    if (lag == p->lag) {
        b1 = p->b1;
        p->y1   = y1 = y0 + b1 * (y1 - y0);
        *p->out = y1;
    } else {
        // faust uses tau2pole = exp(-1 / (lag*sr))
        b1 = lag == FL(0.0) ? FL(0.0) : exp(LOG001 / (lag * p->sr));
        *p->out = y0 + b1 * (y1 - y0);
        p->lag = lag;
        p->y1 = y1;
        p->b1 = b1;
    }
    MYFLT out = *(p->out);
    if (UNLIKELY(em_isnan(out))) {
        return PERFERR("Output should not be nan!");
    }
    return OK;
}

static int32_t laga_init_no_initial_value(CSOUND *csound, LAG0 *p) {
    IGN(csound);
    p->lag = -1;
    p->b1 = FL(0.0);
    p->sr = CS_ESR;
    p->started = 0;
    p->y1 = -INF;
    return OK;
}

static int32_t laga_init_initial_value(CSOUND *csound, LAG0 *p) {
    IGN(csound);
    p->lag = -1;
    p->b1 = FL(1.0);
    p->sr = CS_ESR;
    p->started = 1;
    p->y1 = *p->initial_value;
    return OK;
}

static int32_t laga_next(CSOUND *csound, LAG0 *p) {
    IGN(csound);

    SAMPLE_ACCURATE

    MYFLT *in = p->in;
    MYFLT lag = *p->lagtime;
    MYFLT y0, y1;
    MYFLT b1 = p->b1;

    if(LIKELY(p->started))
        y1 = p->y1;
    else {
        p->started = 1;
        y1 = in[0];
    }

    if (lag == p->lag) {
        for (n=offset; n<nsmps; n++) {
            y0 = in[n];
            y1 = y0 + b1 * (y1 - y0);
            out[n] = y1;
        }
    } else {
        // faust uses tau2pole = exp(-1 / (lag*sr))
        p->b1 = lag == FL(0.0) ? FL(0.0) : exp(LOG001 / (lag * p->sr));
        MYFLT b1_slope = CALCSLOPE(p->b1, b1, nsmps);
        p->lag = lag;
        for (n=offset; n<nsmps; n++) {
            b1 += b1_slope;
            y0  = in[n];
            y1  = y0 + b1 * (y1 - y0);
            out[n] = y1;
        }
    }
    p->y1 = y1;
    return OK;
}

// ------------------------- LagUD ---------------------------

/*

  klagged lagud ksrc, klagtime_up, klagtime_down [, initialvalue]
  alagged lagud asrc, klagtime_up, klagtime_down [, initialvalue]

  NB: if not initialvalue is given, the first value of the src (ksrc or asrc)
      is used.

*/


typedef struct {
    OPDS h;
    MYFLT *out, *in, *lagtimeU, *lagtimeD, *first;
    MYFLT  lagu, lagd, b1u, b1d, y1;
    MYFLT sr;
    int32_t started;
} LagUD;


static int32_t _lagud_init(CSOUND *csound, LagUD *p, int32_t started) {
    IGN(csound);
    p->lagu = -1;
    p->lagd = -1;
    p->y1 = started ? *p->first : FL(0.0);
    p->b1u = started ? FL(1.0) : FL(0.0);
    p->b1d = p->b1u;
    p->started = started;
    p->sr = CS_EKR;
    return OK;
}


static int32_t lagud_init_initial_value(CSOUND *csound, LagUD *p) {
    return _lagud_init(csound, p, 1);
}


static int32_t lagud_init_no_initial_value(CSOUND *csound, LagUD *p) {
    return _lagud_init(csound, p, 0);
}


static int
lagud_k(CSOUND *csound, LagUD *p) {
    MYFLT y0  = *p->in;
    MYFLT lagu = *p->lagtimeU;
    MYFLT lagd = *p->lagtimeD;
    MYFLT y1;

    if(UNLIKELY(em_isinfornan(y0))) {
        return PERFERRF("Non-finite value detected: %f", y0);
    }

    if(LIKELY(p->started))
        y1 = p->y1;
    else {
        p->started = 1;
        y1 = y0;
    }

    if ((lagu == p->lagu) && (lagd == p->lagd)) {
        if (y0 > y1)
            p->y1 = y1 = y0 + p->b1u * (y1 - y0);
        else
            p->y1 = y1 = y0 + p->b1d * (y1 - y0);
        *(p->out) = y1;
    } else {
        MYFLT sr = p->sr;
        // faust uses tau2pole = exp(-1 / (lag*sr)), sc uses log(0.01)
        p->b1u  = lagu == FL(0.0) ? FL(0.0) : exp(LOG001 / (lagu * sr));
        p->lagu = lagu;
        p->b1d  = lagd == FL(0.0) ? FL(0.0) : exp(LOG001 / (lagd * sr));
        p->lagd = lagd;
        if (y0 > y1)
            y1 = y0 + p->b1u * (y1 - y0);
        else
            y1 = y0 + p->b1d * (y1 - y0);
        *(p->out) = y1;
    }
    p->y1 = y1;
    return OK;
}


static int32_t
lagud_a(CSOUND *csound, LagUD *p) {
    IGN(csound);

    SAMPLE_ACCURATE

    MYFLT *in  = p->in;
    MYFLT lagu = *p->lagtimeU;
    MYFLT lagd = *p->lagtimeD;
    // MYFLT y1   = p->y1;
    MYFLT y1;
    MYFLT b1u  = p->b1u;
    MYFLT b1d  = p->b1d;

    if (UNLIKELY(offset)) memset(p->out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early))  {
      nsmps -= early;
      memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
    }
    if(LIKELY(p->started))
        y1 = p->y1;
    else {
        p->started = 1;
        y1 = in[0];
    }


    if ((lagu == p->lagu) && (lagd == p->lagd)) {
        for (n=offset; n<nsmps; n++) {
            MYFLT y0 = in[n];
            if (y0 > y1)
                y1 = y0 + b1u * (y1 - y0);
            else
                y1 = y0 + b1d * (y1 - y0);
            out[n]= y1;
        }
    } else {
        MYFLT sr = CS_ESR;
        // faust uses tau2pole = exp(-1 / (lag*sr))
        p->b1u = lagu == FL(0.0) ? FL(0.0) : exp(LOG001 / (lagu * sr));
        MYFLT b1u_slope = CALCSLOPE(p->b1u, b1u, nsmps);
        p->lagu = lagu;
        p->b1d  = lagd == FL(0.0) ? FL(0.0) : exp(LOG001 / (lagd * sr));
        MYFLT b1d_slope = CALCSLOPE(p->b1d, b1d, nsmps);
        p->lagd = lagd;
        for (n=offset; n<nsmps; n++) {
            MYFLT y0 = in[n];

            b1u += b1u_slope;
            b1d += b1d_slope;
            if (y0 > y1)
                y1 = y0 + b1u * (y1-y0);
            else
                y1 = y0 + b1d * (y1-y0);
            out[n] = y1;
        }
    }
    p->y1 = zapgremlins(y1);
    return OK;
}



/* ------------------ Trig -------------------------

   Returns 1 for a given duration whenever signal crosses from
     non-positive to positive

   kout    trig kin, kduration
   aout    trig ain, kduration

*/

typedef struct {
    OPDS h;
    MYFLT *out, *in, *dur;
    MYFLT  level, prevtrig;
    long counter;
} Trig;

static int
trig_a(CSOUND *csound, Trig *p) {

    SAMPLE_ACCURATE

    MYFLT *in = p->in;
    MYFLT dur = *p->dur;
    MYFLT sr = CS_ESR;
    MYFLT prevtrig = p->prevtrig;
    MYFLT level = p->level;
    unsigned long counter = p->counter;

    for(n=offset; n<nsmps; n++) {
        MYFLT curtrig = in[n];
        MYFLT zout;
        if (counter > 0) {
            zout = --counter ? level : FL(0.0);
        } else {
            if (curtrig > FL(0.0) && prevtrig <= FL(0.0)) {
                counter = (long)(dur * sr + FL(0.5));
                if (counter < 1) counter = 1;
                level = curtrig;
                zout  = level;
            } else {
                zout = FL(0.0);
            }
        }
        prevtrig = curtrig;
        out[n]   = zout;
    }
    p->prevtrig = prevtrig;
    p->counter  = counter;
    p->level    = level;
    return OK;
}

static int
trig_k(CSOUND *csound, Trig *p) {
    MYFLT curtrig = *p->in;
    MYFLT dur = *p->dur;
    MYFLT kr = CS_EKR;
    MYFLT prevtrig = p->prevtrig;
    MYFLT level = p->level;
    uint64_t counter = p->counter;
    if (counter > 0) {
        *p->out = --counter ? level : FL(0.0);
    } else {
        if (curtrig > FL(0.0) && prevtrig <= FL(0.0)) {
            counter = (int64_t)(dur * kr + FL(0.5));
            if (counter < 1)
                counter = 1;
            level   = curtrig;
            *p->out = level;
        } else {
            *p->out = FL(0.0);
        }
    }
    p->prevtrig = curtrig;
    p->counter = counter;
    p->level = level;
    return OK;
}

static int32_t trig_init(CSOUND *csound, Trig *p) {
    p->counter = 0;
    p->prevtrig = FL(0.0);
    p->level = FL(0.0);
    trig_k(csound, p);
    return OK;
}


/*
   Phasor

   kindex   sc_phasor  ktrig, krate, kstart, kend, kresetPos=kstart
   aindex   sc_phasor  ktrig, krate, kstart, kend, kresetPos=kstart
   aindex   sc_phasor  atrig, krate, kstart, kend, kresetPos=kstart
   aindex   sc_phasor  atrig, arate, kstart, kend, kresetPos=kstart

   Phasor is a linear ramp between start and end values. When its trigger
   input crosses from non-positive to positive, Phasor's output will jump
   to its reset position. Upon reaching the end of its ramp Phasor will wrap
   back to its start.

   NOTE: N.B. Since end is defined as the wrap point, its value is never
   actually output.

   NOTE: If one wants Phasor to output a signal with frequency freq
   oscillating between start and end, then the rate should be
   (end - start) * freq / sr where sr is the sampling rate.

   Phasor is commonly used as an index control.

   aindex phasor atrig, xrate, kstart, kend, kresetPos=kstart
   kindex phasor ktrig, krate, kstart, kend, kresetPos=kstart

   trig:     When triggered, jump to resetPos (default: 0, equivalent to start).
   rate:     The amount of change per sample, i.e at a rate of 1 the value
             of each sample will be 1 greater than the preceding sample.
   start:    Start point of the ramp.
   end:      End point of the ramp.
   resetPos: The value to jump to upon receiving a trigger.

*/

typedef struct {
    OPDS h;
    MYFLT *out, *trig, *rate, *start, *end, *resetPos;
    MYFLT level, previn;
} Phasor;

static int32_t phasor_init(CSOUND *csound, Phasor *p) {
    IGN(csound);
    p->previn = 0;
    p->level = 0;
    return OK;
}

static int32_t
phasor_a_aa(CSOUND *csound, Phasor *p) {
    IGN(csound);

    SAMPLE_ACCURATE

    MYFLT *in = p->trig;
    MYFLT *rate = p->rate;
    MYFLT start = *p->start;
    MYFLT end = *p->end;
    MYFLT resetPos = *p->resetPos;
    MYFLT previn = p->previn;
    MYFLT level = p->level;

    for(n=offset; n<nsmps; n++) {
        MYFLT curin = in[n];
        MYFLT zrate = rate[n];
        if (previn <= FL(0.0) && curin > FL(0.0)) {
            MYFLT frac = FL(1) - previn/(curin-previn);
            level = resetPos + frac * zrate;
        }
        out[n] = level;
        level += zrate;
        level = sc_wrap(level, start, end);
        previn = curin;
    }
    p->previn = previn;
    p->level  = level;
    return OK;
}

static int32_t
phasor_a_ak(CSOUND *csound, Phasor *p) {
    IGN(csound);

    SAMPLE_ACCURATE

    MYFLT *in = p->trig;
    MYFLT rate = *p->rate;
    MYFLT start = *p->start;
    MYFLT end = *p->end;
    MYFLT resetPos = *p->resetPos;
    MYFLT previn = p->previn;
    MYFLT level = p->level;

    for(n=offset; n<nsmps; n++) {
        MYFLT curin = in[n];
        if (previn <= FL(0.0) && curin > FL(0.0)) {
            MYFLT frac = FL(1.0) - previn/(curin-previn);
            level = resetPos + frac * rate;
        }
        out[n] = level;
        level += rate;
        level = sc_wrap(level, start, end);
        previn = curin;
    }
    p->previn = previn;
    p->level = level;
    return OK;
}

static int32_t
phasor_a_kk(CSOUND *csound, Phasor *p) {
    IGN(csound);

    SAMPLE_ACCURATE

    MYFLT curin    = *p->trig;
    MYFLT rate     = *p->rate;
    MYFLT start    = *p->start;
    MYFLT end      = *p->end;
    MYFLT resetPos = *p->resetPos;
    MYFLT previn   = p->previn;
    MYFLT level    = p->level;
    int32_t trig = (previn <= FL(0.0)) && (curin > FL(0.0));
    MYFLT frac = FL(1.0) - previn/(curin-previn);

    for(n=offset; n<nsmps; n++) {
        if (trig)
            level = resetPos + frac * rate;
        out[n] = level;
        level += rate;
        level  = sc_wrap(level, start, end);
    }
    p->previn = curin;
    p->level  = level;
    return OK;
}

static int
phasor_k_kk(CSOUND *csound, Phasor *p) {
    IGN(csound);
    MYFLT curin    = *p->trig;
    MYFLT rate     = *p->rate;
    MYFLT start    = *p->start;
    MYFLT end      = *p->end;
    MYFLT resetPos = *p->resetPos;
    MYFLT previn   = p->previn;
    MYFLT level    = p->level;

    if (UNLIKELY(previn <= FL(0.0) && curin > FL(0.0))) {
        level = resetPos;
    }
    level = sc_wrap(level, start, end);
    *p->out = level;
    level += rate;
    p->previn = curin;
    p->level = level;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY scugens_localops[] = {
    {"sc_lag",    S(LAG0),   0,  "k", "kk",
     (SUBR)lag0_init_no_initial_value, (SUBR)lag0k_next},
    {"lag",    S(LAG0),   0,  "k", "kk",
     (SUBR)lag0_init_no_initial_value, (SUBR)lag0k_next},

    {"sc_lag",    S(LAG0),   0,  "k", "kki",
     (SUBR)lag0_init_initial_value, (SUBR)lag0k_next},
    {"lag",    S(LAG0),   0,  "k", "kki",
     (SUBR)lag0_init_initial_value, (SUBR)lag0k_next},

    {"sc_lag",    S(LAG0),    0,  "a", "aki",
     (SUBR)laga_init_initial_value, (SUBR)laga_next},
    {"lag",    S(LAG0),    0,  "a", "aki",
     (SUBR)laga_init_initial_value, (SUBR)laga_next},

    {"sc_lag",    S(LAG0),    0,  "a", "ak",
     (SUBR)laga_init_no_initial_value, (SUBR)laga_next},
    {"lag",    S(LAG0),    0,  "a", "ak",
     (SUBR)laga_init_no_initial_value, (SUBR)laga_next},

    {"sc_lagud",  S(LagUD),  0,  "k", "kkki",
     (SUBR)lagud_init_initial_value, (SUBR)lagud_k },
    {"lagud",  S(LagUD),  0,  "k", "kkki",
     (SUBR)lagud_init_initial_value, (SUBR)lagud_k },

    {"sc_lagud",  S(LagUD),  0,  "a", "akki",
     (SUBR)lagud_init_initial_value, (SUBR)lagud_a },
    {"lagud",  S(LagUD),  0,  "a", "akki",
     (SUBR)lagud_init_initial_value, (SUBR)lagud_a },

    {"sc_lagud",  S(LagUD),  0,  "k", "kkk",
     (SUBR)lagud_init_no_initial_value, (SUBR)lagud_k },
    {"lagud",  S(LagUD),  0,  "k", "kkk",
     (SUBR)lagud_init_no_initial_value, (SUBR)lagud_k },

    {"sc_lagud",  S(LagUD),  0,  "a", "akk",
     (SUBR)lagud_init_no_initial_value, (SUBR)lagud_a },
    {"lagud",  S(LagUD),  0,  "a", "akk",
     (SUBR)lagud_init_no_initial_value, (SUBR)lagud_a },


    {"sc_trig",   S(Trig),   0,  "k", "kk",    (SUBR)trig_init, (SUBR)trig_k },
    {"trighold",   S(Trig),   0,  "k", "kk",    (SUBR)trig_init, (SUBR)trig_k },

    {"sc_trig",   S(Trig),   0,  "a", "ak",    (SUBR)trig_init, (SUBR)trig_a },
    {"trighold",   S(Trig),   0,  "a", "ak",    (SUBR)trig_init, (SUBR)trig_a },

    {"sc_phasor", S(Phasor), 0,  "k", "kkkkO",
     (SUBR)phasor_init, (SUBR)phasor_k_kk },
    {"trigphasor", S(Phasor), 0,  "k", "kkkkO",
     (SUBR)phasor_init, (SUBR)phasor_k_kk },

    {"sc_phasor", S(Phasor), 0,  "a", "akkkO",
     (SUBR)phasor_init, (SUBR)phasor_a_ak },
    {"trigphasor", S(Phasor), 0,  "a", "akkkO",
     (SUBR)phasor_init, (SUBR)phasor_a_ak },

    {"sc_phasor", S(Phasor), 0,  "a", "aakkO",
     (SUBR)phasor_init, (SUBR)phasor_a_aa },
    {"trigphasor", S(Phasor), 0,  "a", "aakkO",
     (SUBR)phasor_init, (SUBR)phasor_a_aa },

    {"sc_phasor", S(Phasor), 0,  "a", "kkkkO",
     (SUBR)phasor_init, (SUBR)phasor_a_kk },

    {"trigphasor", S(Phasor), 0,  "a", "kkkkO",
     (SUBR)phasor_init, (SUBR)phasor_a_kk }
};

LINKAGE_BUILTIN(scugens_localops)
