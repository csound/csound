/*
    reverbsc.c:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"
#include <math.h>

#define DEFAULT_SRATE   44100.0
#define MIN_SRATE       (DEFAULT_SRATE * 0.125)
#define MAX_SRATE       (DEFAULT_SRATE * 8.0)
#define MAX_PITCHMOD    10.0
#define DELAYPOS_SHIFT  16
#define DELAYPOS_MASK   65535
#define DELAYPOS_SCALE  65536

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
    unsigned long readPos;
    unsigned long readPos_inc;
    unsigned long readPos_max;
    unsigned long dummy;
    int           writePos;
    int           writePos_max;
    int           seedVal;
    int           randLine_cnt;
    double        filterState;
    MYFLT         buf[1];
} delayLine;

typedef struct {
    OPDS        h;
    MYFLT       *aoutL, *aoutR, *ainL, *ainR, *kFeedBack, *kLPFreq;
    MYFLT       *iSampleRate, *iPitchMod, *iSkipInit;
    double      sampleRate;
    double      jpState;
    double      dampFact;
    MYFLT       prv_LPFreq;
    int         initDone;
    delayLine   *delayLines[8];
    AUXCH       auxData;
} SC_REVERB;

static int delay_line_max_samples(SC_REVERB *p, int n)
{
    double  maxDel;

    maxDel = reverbParams[n][0];
    maxDel += (reverbParams[n][1] * (double) *(p->iPitchMod) * 1.25);
    return (int) (maxDel * p->sampleRate + 16.5);
}

static int delay_line_bytes_alloc(SC_REVERB *p, int n)
{
    int nBytes;

    nBytes = (int) sizeof(delayLine) - (int) sizeof(MYFLT);
    nBytes += (delay_line_max_samples(p, n) * (int) sizeof(MYFLT));
    nBytes = (nBytes + 15) & (~15);
    return nBytes;
}

static void next_random_lineseg(SC_REVERB *p, delayLine *lp, int n)
{
    double  prvDel, nxtDel, phs_incVal;

    /* update random seed */
    if (lp->seedVal < 0)
      lp->seedVal += 0x10000;
    lp->seedVal = (lp->seedVal * 15625 + 1) & 0xFFFF;
    if (lp->seedVal >= 0x8000)
      lp->seedVal -= 0x10000;
    /* length of next segment in samples */
    lp->randLine_cnt = (int) ((p->sampleRate / reverbParams[n][2]) + 0.5);
    prvDel = (double) lp->writePos;
    prvDel -= ((double) lp->readPos / (double) DELAYPOS_SCALE);
    if (prvDel < 0.0)
      prvDel += (double) lp->writePos_max;
    prvDel = prvDel / p->sampleRate;    /* previous delay time in seconds */
    nxtDel = (double) lp->seedVal * reverbParams[n][1] / 32768.0;
    /* next delay time in seconds */
    nxtDel = reverbParams[n][0] + (nxtDel * (double) *(p->iPitchMod));
    /* calculate phase increment per sample */
    phs_incVal = (prvDel - nxtDel) / (double) lp->randLine_cnt;
    phs_incVal = phs_incVal * p->sampleRate + 1.0;
    lp->readPos_inc = (unsigned long) (phs_incVal * DELAYPOS_SCALE + 0.5);
}

static void init_delay_line(SC_REVERB *p, delayLine *lp, int n)
{
    double  readPos;
    int     i;

    /* calculate length of delay line */
    lp->writePos_max = delay_line_max_samples(p, n);
    lp->readPos_max = (unsigned long) lp->writePos_max << DELAYPOS_SHIFT;
    lp->dummy = 0UL;
    lp->writePos = 0;
    /* set random seed */
    lp->seedVal = (int) (reverbParams[n][3] + 0.5);
    /* set initial delay time */
    readPos = (double) lp->seedVal * reverbParams[n][1] / 32768;
    readPos = reverbParams[n][0] + (readPos * (double) *(p->iPitchMod));
    readPos = (double) lp->writePos_max - (readPos * p->sampleRate);
    lp->readPos = (unsigned long) (readPos * DELAYPOS_SCALE + 0.5);
    /* initialise first random line segment */
    next_random_lineseg(p, lp, n);
    /* clear delay line to zero */
    lp->filterState = 0.0;
    for (i = 0; i < lp->writePos_max; i++)
      lp->buf[i] = FL(0.0);
}

static void delay_line_perform(SC_REVERB *p, delayLine *lp, MYFLT inSig, int n)
{
    double  vm1, v0, v1, v2, am1, a0, a1, a2, frac, x;
    int     readPos;

    if (lp->writePos >= lp->writePos_max)
      lp->writePos -= lp->writePos_max;
    /* send input signal and feedback to delay line */
    lp->buf[lp->writePos] = (MYFLT) ((double) inSig + p->jpState
                                     - lp->filterState);
    lp->writePos++;
    /* read from delay line */
    if (lp->readPos >= lp->readPos_max)
      lp->readPos -= lp->readPos_max;
    readPos = (int) (lp->readPos >> DELAYPOS_SHIFT);
    frac = (double) ((int) (lp->readPos & (unsigned long) DELAYPOS_MASK));
    frac *= (1.0 / (double) DELAYPOS_SCALE);
    if (readPos > 0 && readPos < (lp->writePos_max - 2)) {
      vm1 = (double) (lp->buf[readPos - 1]);
      v0 = (double) (lp->buf[readPos]);
      v1 = (double) (lp->buf[readPos + 1]);
      v2 = (double) (lp->buf[readPos + 2]);
    }
    else {
      /* at buffer wrap-around, need to check index */
      if (--readPos < 0) readPos += lp->writePos_max;
      vm1 = (double) lp->buf[readPos];
      if (++readPos >= lp->writePos_max) readPos -= lp->writePos_max;
      v0 = (double) lp->buf[readPos];
      if (++readPos >= lp->writePos_max) readPos -= lp->writePos_max;
      v1 = (double) lp->buf[readPos];
      if (++readPos >= lp->writePos_max) readPos -= lp->writePos_max;
      v2 = (double) lp->buf[readPos];
    }
    lp->readPos += lp->readPos_inc;
    /* with cubic interpolation */
    a2 = frac * frac; a2 -= 1.0; a2 *= (1.0 / 6.0);
    a1 = frac; a1 += 1.0; a1 *= 0.5; am1 = a1 - 1.0;
    a0 = 3.0 * a2; a1 -= a0; am1 -= a2; a0 -= frac;
    x = (am1 * vm1 + a0 * v0 + a1 * v1 + a2 * v2) * frac + v0;
    /* lowpass filter */
    x *= (double) *(p->kFeedBack);
    x = (lp->filterState - x) * p->dampFact + x;
    lp->filterState = x;
    /* start next random line segment if current one has reached endpoint */
    if (--(lp->randLine_cnt) <= 0)
      next_random_lineseg(p, lp, n);
}

static int sc_reverb_init(ENVIRON *csound, SC_REVERB *p)
{
    int i, nBytes;

    /* check for valid parameters */
    if (*(p->iSampleRate) <= FL(0.0))
      p->sampleRate = (double) csound->esr_;
    else if (*(p->iSampleRate) >= MIN_SRATE && *(p->iSampleRate) <= MAX_SRATE)
      p->sampleRate = (double) *(p->iSampleRate);
    else {
      initerror(Str("reverbsc: sample rate is out of range"));
      return NOTOK;
    }
    if (*(p->iPitchMod) < FL(0.0) || *(p->iPitchMod) > (MYFLT) MAX_PITCHMOD) {
      initerror(Str("reverbsc: invalid pitch modulation factor"));
      return NOTOK;
    }
    /* calculate the number of bytes to allocate */
    nBytes = 0;
    for (i = 0; i < 8; i++)
      nBytes += delay_line_bytes_alloc(p, i);
    if (nBytes != (int) p->auxData.size)
      auxalloc(csound, (long) nBytes, &(p->auxData));
    else if (p->initDone && *(p->iSkipInit) != FL(0.0))
      return OK;    /* skip initialisation if requested */
    /* set up delay lines */
    nBytes = 0;
    for (i = 0; i < 8; i++) {
      p->delayLines[i] = (delayLine*) ((unsigned char*) (p->auxData.auxp)
                                       + (int) nBytes);
      init_delay_line(p, p->delayLines[i], i);
      nBytes += delay_line_bytes_alloc(p, i);
    }
    p->jpState = 0.0;
    p->dampFact = 0.0;
    p->prv_LPFreq = FL(-1.0);
    p->initDone = 1;

    return OK;
}

static int sc_reverb_perf(ENVIRON *csound, SC_REVERB *p)
{
    double  aoutL, aoutR;
    int     i, j;

    if (p->initDone <= 0) {
      perferror(Str("reverbsc: not initialised"));
      return NOTOK;
    }
    /* calculate tone filter coefficient if frequency changed */
    if (*(p->kLPFreq) != p->prv_LPFreq) {
      p->prv_LPFreq = *(p->kLPFreq);
      p->dampFact = 2.0 - cos(p->prv_LPFreq * atan(1.0) * 8.0 / p->sampleRate);
      p->dampFact = p->dampFact - sqrt(p->dampFact * p->dampFact - 1.0);
    }
    /* update delay lines */
    for (i = 0; i < csound->ksmps_; i++) {
      /* calculate "resultant junction pressure" */
      p->jpState = 0.0;
      for (j = 0; j < 8; j++)
        p->jpState += p->delayLines[j]->filterState;
      p->jpState *= jpScale;
      delay_line_perform(p, p->delayLines[0], p->ainL[i], 0);
      aoutL = p->delayLines[0]->filterState;
      delay_line_perform(p, p->delayLines[1], p->ainR[i], 1);
      aoutR = p->delayLines[1]->filterState;
      delay_line_perform(p, p->delayLines[2], p->ainL[i], 2);
      aoutL += (MYFLT) p->delayLines[2]->filterState;
      delay_line_perform(p, p->delayLines[3], p->ainR[i], 3);
      aoutR += (MYFLT) p->delayLines[3]->filterState;
      delay_line_perform(p, p->delayLines[4], p->ainL[i], 4);
      aoutL += (MYFLT) p->delayLines[4]->filterState;
      delay_line_perform(p, p->delayLines[5], p->ainR[i], 5);
      aoutR += (MYFLT) p->delayLines[5]->filterState;
      delay_line_perform(p, p->delayLines[6], p->ainL[i], 6);
      aoutL += (MYFLT) p->delayLines[6]->filterState;
      delay_line_perform(p, p->delayLines[7], p->ainR[i], 7);
      aoutR += (MYFLT) p->delayLines[7]->filterState;
      p->aoutL[i] = (MYFLT) (aoutL * outputGain);
      p->aoutR[i] = (MYFLT) (aoutR * outputGain);
    }

    return OK;
}

/* module interface functions */

int csoundModuleCreate(void *csound)
{
    return 0;
}

int csoundModuleInit(void *csound_)
{
    ENVIRON *csound;
    csound = (ENVIRON*) csound_;
    return (csound->AppendOpcode(csound, "reverbsc",
                                 (int) sizeof(SC_REVERB), 5, "aa", "aakkjpo",
                                 (int (*)(void*, void*)) sc_reverb_init,
                                 (int (*)(void*, void*)) NULL,
                                 (int (*)(void*, void*)) sc_reverb_perf,
                                 (int (*)(void*, void*)) NULL));
}

