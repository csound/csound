/*
    freeverb.c:

    Copyright (C) 2005 Istvan Varga (based on public domain C++ code by Jezar)

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

#define DEFAULT_SRATE   44100.0
#define STEREO_SPREAD   23.0
#define MIN_SRATE       FL(1000.0)

#define NR_COMB         8
#define NR_ALLPASS      4

static const cs_double comb_delays[NR_COMB][2] = {
    { 1116.0 / DEFAULT_SRATE, (1116.0 + STEREO_SPREAD) / DEFAULT_SRATE },
    { 1188.0 / DEFAULT_SRATE, (1188.0 + STEREO_SPREAD) / DEFAULT_SRATE },
    { 1277.0 / DEFAULT_SRATE, (1277.0 + STEREO_SPREAD) / DEFAULT_SRATE },
    { 1356.0 / DEFAULT_SRATE, (1356.0 + STEREO_SPREAD) / DEFAULT_SRATE },
    { 1422.0 / DEFAULT_SRATE, (1422.0 + STEREO_SPREAD) / DEFAULT_SRATE },
    { 1491.0 / DEFAULT_SRATE, (1491.0 + STEREO_SPREAD) / DEFAULT_SRATE },
    { 1557.0 / DEFAULT_SRATE, (1557.0 + STEREO_SPREAD) / DEFAULT_SRATE },
    { 1617.0 / DEFAULT_SRATE, (1617.0 + STEREO_SPREAD) / DEFAULT_SRATE }
};

static const cs_double allpass_delays[NR_ALLPASS][2] = {
    { 556.0 / DEFAULT_SRATE, (556.0 + STEREO_SPREAD) / DEFAULT_SRATE },
    { 441.0 / DEFAULT_SRATE, (441.0 + STEREO_SPREAD) / DEFAULT_SRATE },
    { 341.0 / DEFAULT_SRATE, (341.0 + STEREO_SPREAD) / DEFAULT_SRATE },
    { 225.0 / DEFAULT_SRATE, (225.0 + STEREO_SPREAD) / DEFAULT_SRATE }
};

static const cs_double fixedGain   = 0.015;
static const cs_double scaleDamp   = 0.4;
static const cs_double scaleRoom   = 0.28;
static const cs_double offsetRoom  = 0.7;

static const cs_double allPassFeedBack = 0.5;

typedef struct {
    int32_t     nSamples;
    int32_t     bufPos;
    cs_double  filterState;
    cs_float   buf[1];
} freeVerbComb;

typedef struct {
    int32_t     nSamples;
    int32_t     bufPos;
    cs_float   buf[1];
} freeVerbAllPass;

typedef struct {
    OPDS            h;
    cs_float           *aOutL;
    cs_float           *aOutR;
    cs_float           *aInL;
    cs_float           *aInR;
    cs_float           *kRoomSize;
    cs_float           *kDampFactor;
    cs_float           *iSampleRate;
    cs_float           *iSkipInit;
    freeVerbComb    *Comb[NR_COMB][2];
    freeVerbAllPass *AllPass[NR_ALLPASS][2];
    cs_float           *tmpBuf;
    AUXCH           auxData;
    cs_float           prvDampFactor;
    cs_double          dampValue;
    cs_double          srFact;
} FREEVERB;

static int32_t calc_nsamples(FREEVERB *p, cs_double delTime)
{
    cs_double  sampleRate;
    sampleRate = (cs_double) *(p->iSampleRate);
    if (sampleRate < MIN_SRATE)
      sampleRate = DEFAULT_SRATE;
    return (int32_t) (delTime * sampleRate + 0.5);
}

static int32_t comb_nbytes(FREEVERB *p, cs_double delTime)
{
    int32_t nbytes;
    nbytes = (int32_t) sizeof(freeVerbComb) - (int32_t) sizeof(cs_float);
    nbytes += ((int32_t) sizeof(cs_float) * calc_nsamples(p, delTime));
    return ((nbytes + 15) & (~15));
}

static int32_t allpass_nbytes(FREEVERB *p, cs_double delTime)
{
    int32_t nbytes;
    nbytes = (int32_t) sizeof(freeVerbAllPass) - (int32_t) sizeof(cs_float);
    nbytes += ((int32_t) sizeof(cs_float) * calc_nsamples(p, delTime));
    return ((nbytes + 15) & (~15));
}

static int32_t freeverb_init(CSOUND *csound, FREEVERB *p)
{
    int32_t             i, j, k, nbytes;
    freeVerbComb    *combp;
    freeVerbAllPass *allpassp;
    /* calculate the total number of bytes to allocate */
    nbytes = 0;
    for (i = 0; i < NR_COMB; i++) {
      nbytes += comb_nbytes(p, comb_delays[i][0]);
      nbytes += comb_nbytes(p, comb_delays[i][1]);
    }
    for (i = 0; i < NR_ALLPASS; i++) {
      nbytes += allpass_nbytes(p, allpass_delays[i][0]);
      nbytes += allpass_nbytes(p, allpass_delays[i][1]);
    }
    nbytes += 2 * (int32_t) sizeof(cs_float) * (int32_t) CS_KSMPS;
    /* allocate space if size has changed */
    if (nbytes != (int32_t) p->auxData.size)
      csound->AuxAlloc(csound, (int32) nbytes, &(p->auxData));
    else if (*(p->iSkipInit) != FL(0.0))    /* skip initialisation */
      return OK;                            /*   if requested      */
    /* set up comb and allpass filters */
    nbytes = 0;
    for (i = 0; i < (NR_COMB << 1); i++) {
      combp = (freeVerbComb*)((unsigned char*)p->auxData.auxp + (int32_t) nbytes);
      p->Comb[i >> 1][i & 1] = combp;
      k = calc_nsamples(p, comb_delays[i >> 1][i & 1]);
      combp->nSamples = k;
      combp->bufPos = 0;
      combp->filterState = 0.0;
      for (j = 0; j < k; j++)
        combp->buf[j] = FL(0.0);
      nbytes += comb_nbytes(p, comb_delays[i >> 1][i & 1]);
    }
    for (i = 0; i < (NR_ALLPASS << 1); i++) {
      allpassp = (freeVerbAllPass*) ((unsigned char*) p->auxData.auxp
                                     + (int32_t) nbytes);
      p->AllPass[i >> 1][i & 1] = allpassp;
      k = calc_nsamples(p, allpass_delays[i >> 1][i & 1]);
      allpassp->nSamples = k;
      allpassp->bufPos = 0;
      //memset(allpassp->buf, '\0', k*sizeof(cs_float));
      for (j = 0; j < k; j++)
        allpassp->buf[j] = FL(0.0);
      nbytes += allpass_nbytes(p, allpass_delays[i >> 1][i & 1]);
    }
    p->tmpBuf = (cs_float*) ((unsigned char*)p->auxData.auxp + (int32_t)nbytes);
    p->prvDampFactor = -FL(1.0);
    if (*(p->iSampleRate) >= MIN_SRATE)
      p->srFact = pow((DEFAULT_SRATE / *(p->iSampleRate)), 0.8);
    else
      p->srFact = 1.0;
    return OK;
}

static int32_t freeverb_perf(CSOUND *csound, FREEVERB *p)
{
    cs_double          feedback, damp1, damp2, x;
    freeVerbComb    *combp;
    freeVerbAllPass *allpassp;
    int32_t             i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS - early;
    cs_float *tmpBuf = p->tmpBuf, *left = p->tmpBuf;

    /* check if opcode was correctly initialised */
    if (UNLIKELY(p->auxData.size <= 0L || p->auxData.auxp == NULL)) goto err1;
    /* calculate reverb parameters */
    feedback = (cs_double) *(p->kRoomSize) * scaleRoom + offsetRoom;
    if (*(p->kDampFactor) != p->prvDampFactor) {
      p->prvDampFactor = *(p->kDampFactor);
      damp1 = (cs_double) *(p->kDampFactor) * scaleDamp;
      /* hack to correct high frequency attenuation for sample rate */
      if (*(p->iSampleRate) >= MIN_SRATE)
        damp1 = pow(damp1, p->srFact);
      p->dampValue = damp1;
    }
    else
      damp1 = p->dampValue;
    damp2 = 1.0 - damp1;
    /* comb filters (left channel) */
    memset(tmpBuf,0, sizeof(cs_float)*nsmps);
    for (i = 0; i < NR_COMB; i++) {
      combp = p->Comb[i][0];
      for (n = offset; n < nsmps; n++) {
        tmpBuf[n] += combp->buf[combp->bufPos];
        x = (cs_double) combp->buf[combp->bufPos];
        combp->filterState = (combp->filterState * damp1) + (x * damp2);
        x = combp->filterState * feedback + (cs_double) p->aInL[n];
        combp->buf[combp->bufPos] = (cs_float) x;
        if (UNLIKELY(++(combp->bufPos) >= combp->nSamples))
          combp->bufPos = 0;
      }
    }
    /* allpass filters (left channel) */
    for (i = 0; i < NR_ALLPASS; i++) {
      allpassp = p->AllPass[i][0];
      for (n = offset; n < nsmps; n++) {
        x = (cs_double) allpassp->buf[allpassp->bufPos] - (cs_double) tmpBuf[n];
        allpassp->buf[allpassp->bufPos] *= (cs_float) allPassFeedBack;
        allpassp->buf[allpassp->bufPos] += tmpBuf[n];
        if (UNLIKELY(++(allpassp->bufPos) >= allpassp->nSamples))
          allpassp->bufPos = 0;
        tmpBuf[n] = (cs_float) x;
      }
    }

    /* Keep the left result until both inputs have been consumed. */
    tmpBuf += CS_KSMPS;
    /* comb filters (right channel) */
    memset(tmpBuf, 0, sizeof(cs_float)*nsmps);
    for (i = 0; i < NR_COMB; i++) {
      combp = p->Comb[i][1];
      for (n = offset; n < nsmps; n++) {
        tmpBuf[n] += combp->buf[combp->bufPos];
        x = (cs_double) combp->buf[combp->bufPos];
        combp->filterState = (combp->filterState * damp1) + (x * damp2);
        x = combp->filterState * feedback + (cs_double) p->aInR[n];
        combp->buf[combp->bufPos] = (cs_float) x;
        if (UNLIKELY(++(combp->bufPos) >= combp->nSamples))
          combp->bufPos = 0;
      }
    }
    /* allpass filters (right channel) */
    for (i = 0; i < NR_ALLPASS; i++) {
      allpassp = p->AllPass[i][1];
      for (n = offset; n < nsmps; n++) {
        x = (cs_double) allpassp->buf[allpassp->bufPos] - (cs_double) tmpBuf[n];
        allpassp->buf[allpassp->bufPos] *= (cs_float) allPassFeedBack;
        allpassp->buf[allpassp->bufPos] += tmpBuf[n];
        if (UNLIKELY(++(allpassp->bufPos) >= allpassp->nSamples))
          allpassp->bufPos = 0;
        tmpBuf[n] = (cs_float) x;
      }
    }
    if (UNLIKELY(offset)) {
      memset(p->aOutL, '\0', offset*sizeof(cs_float));
      memset(p->aOutR, '\0', offset*sizeof(cs_float));
    }
    if (UNLIKELY(early)) {
      memset(&p->aOutL[nsmps], '\0', early*sizeof(cs_float));
      memset(&p->aOutR[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n = offset; n < nsmps; n++) {
      p->aOutL[n] = left[n] * (cs_float) fixedGain;
      p->aOutR[n] = tmpBuf[n] * (cs_float) fixedGain;
    }

    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("freeverb: not initialised"));
}

/* module interface functions */

int32_t freeverb_init_(CSOUND *csound)
{
    return csound->AppendOpcode(csound, "freeverb",
                                (int32_t) sizeof(FREEVERB), 0,  "aa", "aakkjo",
                                (int32_t (*)(CSOUND*, void*)) freeverb_init,
                                (int32_t (*)(CSOUND*, void*)) freeverb_perf,
                                NULL);
}

