/*
    moog1.h:

    Copyright (C) 1996, 1997 Perry Cook, John ffitch

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

/******************************************/
/*  Moog1 Subclass of                     */
/*  Sampling Synthesizer Class            */
/*  by Perry R. Cook, 1995-96             */
/*                                        */
/*   Controls:    CONTROL1 = filterQ      */
/*                CONTROL2 = filterRate   */
/*                CONTROL3 = vibFreq      */
/*                MOD_WHEEL= vibAmt       */
/******************************************/

#if !defined(__Moog1_h)
#define __Moog1_h

#include "fm4op.h"

/*******************************************/
/*  Sweepable Formant (2-pole)             */
/*  Filter Class, by Perry R. Cook, 1995-96*/
/*  See books on filters to understand     */
/*  more about how this works.  Nothing    */
/*  out of the ordinary in this version.   */
/*******************************************/

typedef struct FormSwep {
    cs_float       gain;
    cs_float       outputs[2];
    cs_float       poleCoeffs[2];
    cs_float       freq;
    cs_float       reson;
    int32_t
    dirty;
    cs_float       targetFreq;
    cs_float       targetReson;
    cs_float       targetGain;
    cs_float       currentFreq;
    cs_float       currentReson;
    cs_float       currentGain;
    cs_float       deltaFreq;
    cs_float       deltaReson;
    cs_float       deltaGain;
    cs_float       sweepState;
    cs_float       sweepRate;
} FormSwep;

#define FormSwep_setSweepRate(p,aRate)  (p.sweepRate = aRate)
#define FormSwep_clear(p)               (p.outputs[0]=p.outputs[1]=FL(0.0))
void FormSwep_setTargets(FormSwep *, cs_float, cs_float, cs_float);
cs_float FormSwep_tick(OPDS *, FormSwep *, cs_float);

typedef struct Wave {
    FUNC        *wave;
    cs_float       rate;
    cs_float       time;
    cs_float       phase;
} Wave;

/*******************************************/
/*  Master Class for Sampling Synthesizer  */
/*  by Perry R. Cook, 1995-96              */
/*  This instrument contains up to 5       */
/*  attack waves, 5 looped waves, and      */
/*  an ADSR envelope.                      */
/*******************************************/

typedef struct MOOG1 {
    OPDS        h;
    cs_float       *ar;                  /* Output */
    cs_float       *amp, *frequency;
    cs_float       *filterQ, *filterRate, *vibf, *vibAmt;
    cs_float       *iatt, *ifn, *ivfn;

    ADSR        adsr;
    Wave        attk;      /* Not looped */
    Wave        loop;      /* Looped */
    Wave        vibr;      /* Looped */
    OnePole     filter;
    cs_float       baseFreq;
    cs_float       attackRatio;
    cs_float       loopRatio;
    cs_float       attackGain;
    cs_float       loopGain;
    cs_float       oldfilterQ;
    cs_float       oldfilterRate;
    FormSwep    filters[2];
    TwoZero     twozeroes[2];
} MOOG1;

#endif

