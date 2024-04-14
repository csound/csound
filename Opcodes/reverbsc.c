/*
    reverbsc.c:

    Copyright 1999, 2005 Sean Costello and Istvan Varga

    8 delay line FDN reverb, with feedback matrix based upon
    physical modeling scattering junction of 8 lossless waveguides
    of equal characteristic impedance. Based on Julius O. Smith III,
    "A New Approach to Digital Reverberation using Closed Waveguide
    Networks," Proceedings of the International Computer Music
    Conference 1985, p. 47-53 (also available as a seperate
    publication from CCRMA), as well as some more recent papers by
    Smith and others.

    Csound orchestra version coded by Sean Costello, October 1999

    C implementation (C) 2005 Istvan Varga

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

#include "stdopcod.h"
#include <math.h>

#define DEFAULT_SRATE   44100.0
#define MIN_SRATE       5000.0
#define MAX_SRATE       1000000.0
#define MAX_PITCHMOD    20.0
#define DELAYPOS_SHIFT  28
#define DELAYPOS_SCALE  0x10000000
#define DELAYPOS_MASK   0x0FFFFFFF

/* reverbParams[n][0] = delay time (in seconds)                     */
/* reverbParams[n][1] = random variation in delay time (in seconds) */
/* reverbParams[n][2] = random variation frequency (in 1/sec)       */
/* reverbParams[n][3] = random seed (0 - 32767)                     */

static const double reverbParams[8][4] = {
    { (2473.0 / DEFAULT_SRATE), 0.0010, 3.100,  1966.0 },
    { (2767.0 / DEFAULT_SRATE), 0.0011, 3.500, 29491.0 },
    { (3217.0 / DEFAULT_SRATE), 0.0017, 1.110, 22937.0 },
    { (3557.0 / DEFAULT_SRATE), 0.0006, 3.973,  9830.0 },
    { (3907.0 / DEFAULT_SRATE), 0.0010, 2.341, 20643.0 },
    { (4127.0 / DEFAULT_SRATE), 0.0011, 1.897, 22937.0 },
    { (2143.0 / DEFAULT_SRATE), 0.0017, 0.891, 29491.0 },
    { (1933.0 / DEFAULT_SRATE), 0.0006, 3.221, 14417.0 }
};

static const double outputGain  = 0.35;
static const double jpScale     = 0.25;

typedef struct {
    int32_t         writePos;
    int32_t         bufferSize;
    int32_t         readPos;
    int32_t         readPosFrac;
    int32_t         readPosFrac_inc;
    int32_t         dummy;
    int32_t         seedVal;
    int32_t         randLine_cnt;
    double      filterState;
    MYFLT       buf[1];
} delayLine;

typedef struct {
    OPDS        h;
    MYFLT       *aoutL, *aoutR, *ainL, *ainR, *kFeedBack, *kLPFreq;
    MYFLT       *iSampleRate, *iPitchMod, *iSkipInit;
    double      sampleRate;
    double      dampFact;
    MYFLT       prv_LPFreq;
    int32_t         initDone;
    delayLine   *delayLines[8];
    AUXCH       auxData;
} SC_REVERB;

static int32_t delay_line_max_samples(SC_REVERB *p, int32_t n)
{
    double  maxDel;

    maxDel = reverbParams[n][0];
    maxDel += (reverbParams[n][1] * (double) *(p->iPitchMod) * 1.125);
    return (int32_t) (maxDel * p->sampleRate + 16.5);
}

static int32_t delay_line_bytes_alloc(SC_REVERB *p, int32_t n)
{
    int32_t nBytes;

    nBytes = (int32_t) sizeof(delayLine) - (int32_t) sizeof(MYFLT);
    nBytes += (delay_line_max_samples(p, n) * (int32_t) sizeof(MYFLT));
    nBytes = (nBytes + 15) & (~15);
    return nBytes;
}

static void next_random_lineseg(SC_REVERB *p, delayLine *lp, int32_t n)
{
    double  prvDel, nxtDel, phs_incVal;

    /* update random seed */
    if (lp->seedVal < 0)
      lp->seedVal += 0x10000;
    lp->seedVal = (lp->seedVal * 15625 + 1) & 0xFFFF;
    if (lp->seedVal >= 0x8000)
      lp->seedVal -= 0x10000;
    /* length of next segment in samples */
    lp->randLine_cnt = (int32_t) ((p->sampleRate / reverbParams[n][2]) + 0.5);
    prvDel = (double) lp->writePos;
    prvDel -= ((double) lp->readPos
               + ((double) lp->readPosFrac / (double) DELAYPOS_SCALE));
    while (prvDel < 0.0)
      prvDel += (double) lp->bufferSize;
    prvDel = prvDel / p->sampleRate;    /* previous delay time in seconds */
    nxtDel = (double) lp->seedVal * reverbParams[n][1] / 32768.0;
    /* next delay time in seconds */
    nxtDel = reverbParams[n][0] + (nxtDel * (double) *(p->iPitchMod));
    /* calculate phase increment per sample */
    phs_incVal = (prvDel - nxtDel) / (double) lp->randLine_cnt;
    phs_incVal = phs_incVal * p->sampleRate + 1.0;
    lp->readPosFrac_inc = (int32_t) (phs_incVal * DELAYPOS_SCALE + 0.5);
}

static void init_delay_line(SC_REVERB *p, delayLine *lp, int32_t n)
{
    double  readPos;
    /* int32_t     i; */

    /* calculate length of delay line */
    lp->bufferSize = delay_line_max_samples(p, n);
    lp->dummy = 0;
    lp->writePos = 0;
    /* set random seed */
    lp->seedVal = (int32_t) (reverbParams[n][3] + 0.5);
    /* set initial delay time */
    readPos = (double) lp->seedVal * reverbParams[n][1] / 32768;
    readPos = reverbParams[n][0] + (readPos * (double) *(p->iPitchMod));
    readPos = (double) lp->bufferSize - (readPos * p->sampleRate);
    lp->readPos = (int32_t) readPos;
    readPos = (readPos - (double) lp->readPos) * (double) DELAYPOS_SCALE;
    lp->readPosFrac = (int32_t) (readPos + 0.5);
    /* initialise first random line segment */
    next_random_lineseg(p, lp, n);
    /* clear delay line to zero */
    lp->filterState = 0.0;
    memset(lp->buf, 0, sizeof(MYFLT)*lp->bufferSize);
    /* for (i = 0; i < lp->bufferSize; i++) */
    /*   lp->buf[i] = FL(0.0); */
}

static int32_t sc_reverb_init(CSOUND *csound, SC_REVERB *p)
{
    int32_t i;
    int32_t nBytes;

    /* check for valid parameters */
    if (UNLIKELY(*(p->iSampleRate) <= FL(0.0)))
      p->sampleRate = (double) CS_ESR;
    else
      p->sampleRate = (double) *(p->iSampleRate);
    if (UNLIKELY(p->sampleRate < MIN_SRATE || p->sampleRate > MAX_SRATE)) {
      return csound->InitError(csound,
                               "%s", Str("reverbsc: sample rate is out of range"));
    }
    if (UNLIKELY(*(p->iPitchMod) < FL(0.0) ||
                 *(p->iPitchMod) > (MYFLT) MAX_PITCHMOD)) {
      return csound->InitError(csound,
                               "%s", Str("reverbsc: invalid pitch modulation factor"));
    }
    /* calculate the number of bytes to allocate */
    nBytes = 0;
    for (i = 0; i < 8; i++)
      nBytes += delay_line_bytes_alloc(p, i);
    if (nBytes != (int32_t)p->auxData.size)
      csound->AuxAlloc(csound, (size_t) nBytes, &(p->auxData));
    else if (p->initDone && *(p->iSkipInit) != FL(0.0))
      return OK;    /* skip initialisation if requested */
    /* set up delay lines */
    nBytes = 0;
    for (i = 0; i < 8; i++) {
      p->delayLines[i] = (delayLine*) ((unsigned char*) (p->auxData.auxp)
                                       + (int32_t) nBytes);
      init_delay_line(p, p->delayLines[i], i);
      nBytes += delay_line_bytes_alloc(p, i);
    }
    p->dampFact = 1.0;
    p->prv_LPFreq = FL(0.0);
    p->initDone = 1;

    return OK;
}

static int32_t sc_reverb_perf(CSOUND *csound, SC_REVERB *p)
{
    double    ainL, ainR, aoutL, aoutR;
    double    vm1, v0, v1, v2, am1, a0, a1, a2, frac;
    delayLine *lp;
    int32_t       readPos;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, n, nsmps = CS_KSMPS;
    int32_t       bufferSize; /* Local copy */
    double    dampFact = p->dampFact;

    if (UNLIKELY(p->initDone <= 0)) goto err1;
    /* calculate tone filter coefficient if frequency changed */
    if (*(p->kLPFreq) != p->prv_LPFreq) {
      p->prv_LPFreq = *(p->kLPFreq);
      dampFact = 2.0 - cos(p->prv_LPFreq * TWOPI / p->sampleRate);
      dampFact = p->dampFact = dampFact - sqrt(dampFact * dampFact - 1.0);
    }
    if (UNLIKELY(offset)) {
      memset(p->aoutL, '\0', offset*sizeof(MYFLT));
      memset(p->aoutR, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->aoutL[nsmps], '\0', early*sizeof(MYFLT));
      memset(&p->aoutR[nsmps], '\0', early*sizeof(MYFLT));
    }
    /* update delay lines */
    for (i = offset; i < nsmps; i++) {
      /* calculate "resultant junction pressure" and mix to input signals */
      ainL = aoutL = aoutR = 0.0;
      for (n = 0; n < 8; n++)
        ainL += p->delayLines[n]->filterState;
      ainL *= jpScale;
      ainR = ainL + (double) p->ainR[i];
      ainL = ainL + (double) p->ainL[i];
      /* loop through all delay lines */
      for (n = 0; n < 8; n++) {
        lp = p->delayLines[n];
        bufferSize = lp->bufferSize;
        /* send input signal and feedback to delay line */
        lp->buf[lp->writePos] = (MYFLT) ((n & 1 ? ainR : ainL)
                                         - lp->filterState);
        if (UNLIKELY(++lp->writePos >= bufferSize))
          lp->writePos -= bufferSize;
        /* read from delay line with cubic interpolation */
        if (lp->readPosFrac >= DELAYPOS_SCALE) {
          lp->readPos += (lp->readPosFrac >> DELAYPOS_SHIFT);
          lp->readPosFrac &= DELAYPOS_MASK;
        }
        if (UNLIKELY(lp->readPos >= bufferSize))
          lp->readPos -= bufferSize;
        readPos = lp->readPos;
        frac = (double) lp->readPosFrac * (1.0 / (double) DELAYPOS_SCALE);
        /* calculate interpolation coefficients */
        a2 = frac * frac; a2 -= 1.0; a2 *= (1.0 / 6.0);
        a1 = frac; a1 += 1.0; a1 *= 0.5; am1 = a1 - 1.0;
        a0 = 3.0 * a2; a1 -= a0; am1 -= a2; a0 -= frac;
        /* read four samples for interpolation */
        if (LIKELY(readPos > 0 && readPos < (bufferSize - 2))) {
          vm1 = (double) (lp->buf[readPos - 1]);
          v0  = (double) (lp->buf[readPos]);
          v1  = (double) (lp->buf[readPos + 1]);
          v2  = (double) (lp->buf[readPos + 2]);
        }
        else {
          /* at buffer wrap-around, need to check index */
          if (--readPos < 0) readPos += bufferSize;
          vm1 = (double) lp->buf[readPos];
          if (++readPos >= bufferSize) readPos -= bufferSize;
          v0 = (double) lp->buf[readPos];
          if (++readPos >= bufferSize) readPos -= bufferSize;
          v1 = (double) lp->buf[readPos];
          if (++readPos >= bufferSize) readPos -= bufferSize;
          v2 = (double) lp->buf[readPos];
        }
        v0 = (am1 * vm1 + a0 * v0 + a1 * v1 + a2 * v2) * frac + v0;
        /* update buffer read position */
        lp->readPosFrac += lp->readPosFrac_inc;
        /* apply feedback gain and lowpass filter */
        v0 *= (double) *(p->kFeedBack);
        v0 = (lp->filterState - v0) * dampFact + v0;
        lp->filterState = v0;
        /* mix to output */
        if (n & 1)
          aoutR += v0;
        else
          aoutL += v0;
        /* start next random line segment if current one has reached endpoint */
        if (--(lp->randLine_cnt) <= 0)
          next_random_lineseg(p, lp, n);
      }
      p->aoutL[i] = (MYFLT) (aoutL * outputGain);
      p->aoutR[i] = (MYFLT) (aoutR * outputGain);
    }

    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("reverbsc: not initialised"));
}

/* module interface functions */

int32_t reverbsc_init_(CSOUND *csound)
{
    return csound->AppendOpcode(csound, "reverbsc",
                                (int32_t) sizeof(SC_REVERB), 0,  "aa", "aakkjpo",
                                (int32_t (*)(CSOUND *, void *)) sc_reverb_init,
                                (int32_t (*)(CSOUND *, void *)) sc_reverb_perf,
                                NULL);
}

