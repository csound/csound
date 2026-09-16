/*
    metro.c:

    Copyright (C) 2000 Gabriel Maldonado, (C) 2019 Gleb Rogozinsky

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

#include "stdopcod.h"
#include <math.h>

typedef struct {
        OPDS    h;
        MYFLT   *sr, *xcps, *iphs, *kgate;
        double  curphs;
        double  gate;
        int32_t flag;
} METRO;

// METRO2 ADDED BY GLEB ROGOZINSKY Oct 2019
typedef struct {
        OPDS    h;
        MYFLT   *sr, *xcps, *kswng, *iamp, *iphs, *icorrect;
        double  amp2, curphs, curphs2, swng_init;
        int32_t flag, flag2;
} METRO2;
//

typedef struct  {
        OPDS    h;
        MYFLT   *trig, *ndx, *maxtics, *ifn, *outargs[VARGMAX];
        int32_t             numouts, currtic, old_ndx;
        int32_t max_tics;
        uint32_t flen, numseq;
        uint64_t stride;
        MYFLT *table;
} SPLIT_TRIG;

typedef struct  {
        OPDS    h;
        MYFLT   *ktrig, *kphs, *ifn, *args[VARGMAX];
        MYFLT endSeq, *table, oldPhs;
        int32_t numParm, endIndex;
        int32_t initFlag;

} TIMEDSEQ;

static int32_t metro_set(CSOUND *csound, METRO *p)
{
    double phs = *p->iphs;
    int32  longphs;

    if (phs >= 0.0) {
      if (UNLIKELY((longphs = (int32)phs)))
        csound->Warning(csound, "%s", Str("metro:init phase truncation"));
      p->curphs = (MYFLT)phs - (MYFLT)longphs;
    }
    p->flag=1;
    return OK;
}

static int32_t metro(CSOUND *csound, METRO *p)
{
    double      phs= p->curphs;
    IGN(csound);
    if (phs == 0.0 && p->flag) {
      *p->sr = FL(1.0);
      p->flag = 0;
    }
    else if ((phs += *p->xcps * CS_ONEDKR) >= 1.0) {
      *p->sr = FL(1.0);
      phs -= 1.0;
      p->flag = 0;
    }
    else
      *p->sr = FL(0.0);
    p->curphs = phs;
    return OK;
}

/* John ffitch Oct 2021; for beginers */
static int32_t metrobpm(CSOUND *csound, METRO *p)
{
    double      phs= p->curphs;
    IGN(csound);
    p->gate = *p->kgate;
    if (phs == 0.0 && p->flag) {
      *p->sr = FL(1.0);
      p->flag = 0;
    }
    else if ((phs += *p->xcps * CS_ONEDKR/60) >= 1.0) {
      *p->sr = FL(1.0);
      phs -= 1.0;
      p->flag = 0;
    }
    else if (phs>= p->gate)
      *p->sr = FL(0.0);
    p->curphs = phs;
    return OK;
}

/* GLEB ROGOZINSKY Oct 2019
   Opcode metro2 in addition to 'classic' metro opcode,
   allows swinging with possibiliy of setting its own amplitude value
*/
static int32_t metro2_legacy_set(CSOUND *csound, METRO2 *p)
{
    double phs = *p->iphs;
    double swng = *p->kswng;
    int32  longphs;
    p->amp2 = *p->iamp;

    if (phs >= 0.0) {
      if (UNLIKELY((longphs = (int32)phs)))
        csound->Warning(csound, "%s", Str("metro2:init phase truncation"));
      p->curphs = (MYFLT)phs - (MYFLT)longphs;
      p->curphs2 = (MYFLT)phs - (MYFLT)longphs + 1.0 - (MYFLT)swng;
    }
    p->flag = 1;
    p->flag2 = 1;
    p->swng_init = (MYFLT)swng;
    return OK;
}

static int32_t metro2_legacy(CSOUND *csound, METRO2 *p)
{
    double      phs= p->curphs;
    double      phs2= p->curphs2;
    double      phs2_init = p->swng_init;
    double      amp2= p->amp2;
    double      swng= *p->kswng;
    IGN(csound);
// MAIN TICK
    if (phs == 0.0 && p->flag) {
      *p->sr = FL(1.0);
      p->flag = 0;
    }
    else if ((phs += *p->xcps * CS_ONEDKR * 0.5) >= 1.0 ) {
      *p->sr = FL(1.0);
      phs -= 1.0;
      p->flag = 0;
    }
    else
      *p->sr = FL(0.0);
    p->curphs = phs;

// SWINGING TICK
    if (phs2 == 0.0 && p->flag2) {
      *p->sr = FL(amp2);
      p->flag2 = 0;
    }
    else if ((phs2 += *p->xcps * CS_ONEDKR * 0.5) >= (1.0 + swng - phs2_init) ) {
      *p->sr = FL(amp2);
      phs2 -= 1.0;
      p->flag2 = 0;
    }
    p->curphs2 = phs2;

    return OK;
}
//

static int32_t metro2_correct_set(CSOUND *csound, METRO2 *p)
{
    double phs = *p->iphs;

    if (UNLIKELY(!isfinite(phs) || phs < 0.0))
      return csound->InitError(csound, "%s", Str("metro2: invalid initial phase"));
    if (UNLIKELY(phs >= 1.0)) {
      csound->Warning(csound, "%s", Str("metro2:init phase truncation"));
      phs -= floor(phs);
    }
    p->amp2 = *p->iamp;
    p->curphs = phs;
    p->curphs2 = 0.0;
    p->flag = 1;
    p->swng_init = 0.0;
    return OK;
}

static int32_t metro2_correct(CSOUND *csound, METRO2 *p)
{
    double phs = p->curphs, phs2 = p->curphs2;
    double swng = *p->kswng;
    double frequency = *p->xcps, increment, threshold;

    if (UNLIKELY(!(swng >= 0.0 && swng <= 1.0)))
      return csound->PerfError(csound, &(p->h), "%s",
                              Str("metro2: swing must be between 0 and 1"));
    if (UNLIKELY(!isfinite(frequency) || frequency < 0.0))
      return csound->PerfError(csound, &(p->h), "%s",
                              Str("metro2: frequency must be finite and nonnegative"));

    /* An exact initial tick must hold both clocks for the same cycle.
       At coincident endpoints, retain the documented initial main tick. */
    if (p->flag) {
      /* k-rate expressions may not have a value during initialization. */
      p->swng_init = swng;
      phs2 = phs - swng;
      if (phs2 < 0.0) phs2 += 1.0;
      p->curphs2 = phs2;
      p->flag = 0;
      if (phs == 0.0 || phs2 == 0.0) {
        *p->sr = phs == 0.0 ? FL(1.0) : (MYFLT)p->amp2;
        return OK;
      }
    }

    /* At most one output tick fits in a control cycle. Keep two whole
       periods so even a swing change across its full range crosses a tick. */
    if (UNLIKELY(frequency >= 6.0 * CS_EKR))
      increment = 2.0 + fmod(frequency, 2.0 * CS_EKR) / (2.0 * CS_EKR);
    else
      increment = frequency * (0.5 * CS_ONEDKR);
    phs += increment;
    phs2 += increment;
    *p->sr = FL(0.0);
    if (phs >= 1.0) {
      *p->sr = FL(1.0);
      phs -= floor(phs);
    }

    threshold = 1.0 + swng - p->swng_init;
    if (phs2 >= threshold) {
      *p->sr = (MYFLT)p->amp2;
      phs2 -= floor(phs2 - threshold) + 1.0;
    }
    p->curphs = phs;
    p->curphs2 = phs2;
    return OK;
}

/* Preserve existing scores unless corrected timing is requested. */
static int32_t metro2_set(CSOUND *csound, METRO2 *p)
{
    return *p->icorrect == FL(0.0) ? metro2_legacy_set(csound, p)
                                 : metro2_correct_set(csound, p);
}

static int32_t metro2(CSOUND *csound, METRO2 *p)
{
    return *p->icorrect == FL(0.0) ? metro2_legacy(csound, p)
                                 : metro2_correct(csound, p);
}

static int32_t split_trig_set(CSOUND *csound,   SPLIT_TRIG *p)
{

    /* syntax of each table element:
       numtics_elem1,
       tic1_out1, tic1_out2, ... , tic1_outN,
       tic2_out1, tic2_out2, ... , tic2_outN,
       tic3_out1, tic3_out2, ... , tic3_outN,
       .....
       ticN_out1, ticN_out2, ... , ticN_outN,

       numtics_elem2,
       tic1_out1, tic1_out2, ... , tic1_outN,
       tic2_out1, tic2_out2, ... , tic2_outN,
       tic3_out1, tic3_out2, ... , tic3_outN,
       .....
       ticN_out1, ticN_out2, ... , ticN_outN,

    */

    FUNC *ftp;
    double maxtics = (double)*p->maxtics;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL)) {
      return csound->InitError(csound, "%s", Str("splitrig: incorrect table number"));
    }
    p->table = ftp->ftable;
    p->numouts =  p->INOCOUNT-4;
    if (UNLIKELY(p->numouts < 1 || ftp->flen < (uint32_t)p->numouts))
      return csound->InitError(csound, "%s",
                               Str("splitrig: table cannot hold one tick"));
    if (UNLIKELY(!(maxtics >= 1.0 && maxtics < (double)INT32_MAX + 1.0)))
      return csound->InitError(csound, "%s",
                               Str("splitrig: invalid maximum tick count"));
    p->max_tics = (int32_t)maxtics;
    p->stride = (uint64_t)p->numouts * p->max_tics + 1;
    /* The allocated guard point may hold the final tick value. */
    p->flen = ftp->flen;
    p->numseq = (uint32_t)(p->flen / p->stride + 1);
    p->currtic = 0;
    p->old_ndx = -1;
    return OK;
}

static int32_t split_trig(CSOUND *csound, SPLIT_TRIG *p)
{
    int32_t j;
    int32_t numouts =  p->numouts;
    MYFLT **outargs = p->outargs;

    if (*p->trig) {
      double index = (double)*p->ndx;
      double ticks;
      uint32_t ndx, available;
      int32_t kndx, numtics, currtic;
      MYFLT *table;

      /* Preserve truncation toward zero, but check before converting. */
      if (UNLIKELY(!(index > -1.0 && index < (double)p->numseq)))
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("splitrig: sequence index out of range"));
      kndx = (int32_t)index;
      ndx = (uint32_t)(kndx * p->stride);
      ticks = (double)p->table[ndx];
      available = (p->flen - ndx) / numouts;
      if (UNLIKELY(!(ticks >= 1.0 &&
                     ticks < (double)p->max_tics + 1.0 &&
                     ticks < (double)available + 1.0)))
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("splitrig: invalid sequence tick count"));
      numtics = (int32_t)ticks;
      table = &p->table[ndx+1];

      if (kndx != p->old_ndx) {
        p->currtic = 0;
        p->old_ndx = kndx;
      }
      /* A table write may shorten the selected sequence between triggers. */
      if (UNLIKELY(p->currtic >= numtics)) p->currtic = 0;
      currtic = p->currtic;

      for (j = 0; j < numouts; j++)
        *outargs[j] = table[j +  currtic * numouts ];

      p->currtic = (currtic + 1 == numtics ? 0 : currtic + 1);

    }

    else { // Maybe a memset?
      for(j =0; j< numouts; j++)
        *outargs[j] = FL(0.0);
    }
    return OK;
}

static int32_t timeseq_set(CSOUND *csound, TIMEDSEQ *p)
{
    FUNC *ftp = csound->FTFind(csound, p->ifn);
    uint32_t row, rows;
    MYFLT previous = FL(0.0);
    if (UNLIKELY(ftp == NULL)) return NOTOK;
    p->numParm = p->INOCOUNT - 2;
    if (UNLIKELY(p->numParm < 2))
      return csound->InitError(csound, "%s",
                              Str("timedseq: rows need at least an event and time"));
    p->table = ftp->ftable;
    rows = ftp->flen / p->numParm;
    for (row = 0; row < rows; row++) {
      MYFLT *event = p->table + (size_t)row * p->numParm;
      if (event[0] < 0) {
        if (UNLIKELY(row == 0 || !(event[1] > 0) ||
                     !isfinite(event[1]) || event[1] < previous))
          return csound->InitError(csound, "%s",
                                  Str("timedseq: invalid sequence end"));
        p->endSeq = event[1];
        p->endIndex = row;
        p->initFlag = 1;
        *p->ktrig = FL(0.0);
        return OK;
      }
      if (UNLIKELY(!(event[1] >= previous) || !isfinite(event[1])))
        return csound->InitError(csound, "%s",
                                Str("timedseq: event times must be sorted and nonnegative"));
      previous = event[1];
    }
    return csound->InitError(csound, "%s",
                            Str("timedseq: missing complete end row"));
}

static int32_t timeseq(CSOUND *csound, TIMEDSEQ *p)
{
    MYFLT phs = *p->kphs, delta, distance;
    MYFLT endseq = p->endSeq;
    int32_t lo = 0, hi = p->endIndex, index, j;
    int32_t reverse;

    *p->ktrig = FL(0.0);
    if (phs < 0 || phs >= endseq) {
      phs = FMOD(phs, endseq);
      if (phs < 0) phs += endseq;
    }
    if (UNLIKELY(!(phs >= 0 && phs < endseq)))
      return csound->PerfError(csound, &p->h, "%s",
                              Str("timedseq: invalid time pointer"));
    delta = p->initFlag ? FL(0.0) : phs - p->oldPhs;
    p->oldPhs = phs;
    /* A wrapped phase cannot distinguish a large jump from a loop crossing.
       Use the shorter path, as for a forward or reverse phasor. */
    if (delta > endseq * FL(0.5)) delta -= endseq;
    else if (delta < -endseq * FL(0.5)) delta += endseq;
    if (!p->initFlag && delta == 0) return OK;
    reverse = delta < 0;

    /* Find the last crossed row in the direction of travel. The end marker
       is never an event. Only one row can be returned per control cycle. */
    while (lo < hi) {
      int32_t mid = lo + (hi - lo) / 2;
      MYFLT time = p->table[(size_t)mid * p->numParm + 1];
      if (time < phs || (!reverse && time == phs)) lo = mid + 1;
      else hi = mid;
    }
    if (reverse) {
      index = lo == p->endIndex ? 0 : lo;
      distance = p->table[(size_t)index * p->numParm + 1] - phs;
      if (lo == p->endIndex) distance += endseq;
    }
    else {
      index = lo == 0 ? p->endIndex - 1 : lo - 1;
      distance = phs - p->table[(size_t)index * p->numParm + 1];
      if (lo == 0) distance += endseq;
    }
    if ((p->initFlag && distance == 0) ||
        (!p->initFlag && distance < (reverse ? -delta : delta))) {
      MYFLT *event = p->table + (size_t)index * p->numParm;
      for (j = 0; j < p->numParm; j++) *p->args[j] = event[j];
      *p->ktrig = FL(1.0);
    }
    p->initFlag = 0;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "metro",  S(METRO),  0,        "k", "ko",  (SUBR)metro_set, (SUBR)metro    },
  { "metro2", S(METRO2), 0,        "k", "kkpoo", (SUBR)metro2_set, (SUBR)metro2  },
  { "metrobpm",S(METRO), 0,        "k", "koO",  (SUBR)metro_set, (SUBR)metrobpm },
  { "splitrig", S(SPLIT_TRIG), 0,  "",  "kkiiz",
                                        (SUBR)split_trig_set, (SUBR)split_trig },
  { "timedseq",S(TIMEDSEQ), TR,  "k", "kiz", (SUBR)timeseq_set, (SUBR)timeseq }
};

int32_t metro_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
