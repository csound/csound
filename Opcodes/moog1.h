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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
    MYFLT       gain;
    MYFLT       outputs[2];
    MYFLT       poleCoeffs[2];
    MYFLT       freq;
    MYFLT       reson;
    int32_t
    dirty;
    MYFLT       targetFreq;
    MYFLT       targetReson;
    MYFLT       targetGain;
    MYFLT       currentFreq;
    MYFLT       currentReson;
    MYFLT       currentGain;
    MYFLT       deltaFreq;
    MYFLT       deltaReson;
    MYFLT       deltaGain;
    MYFLT       sweepState;
    MYFLT       sweepRate;
} FormSwep;

#define FormSwep_setSweepRate(p,aRate)  (p.sweepRate = aRate)
#define FormSwep_clear(p)               (p.outputs[0]=p.outputs[1]=FL(0.0))
void FormSwep_setTargets(FormSwep *, MYFLT, MYFLT, MYFLT);
MYFLT FormSwep_tick(OPDS *, FormSwep *, MYFLT);

typedef struct Wave {
    FUNC        *wave;
    MYFLT       rate;
    MYFLT       time;
    MYFLT       phase;
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
    MYFLT       *ar;                  /* Output */
    MYFLT       *amp, *frequency;
    MYFLT       *filterQ, *filterRate, *vibf, *vibAmt;
    MYFLT       *iatt, *ifn, *ivfn;

    ADSR        adsr;
    Wave        attk;      /* Not looped */
    Wave        loop;      /* Looped */
    Wave        vibr;      /* Looped */
    OnePole     filter;
    MYFLT       baseFreq;
    MYFLT       attackRatio;
    MYFLT       loopRatio;
    MYFLT       attackGain;
    MYFLT       loopGain;
    MYFLT       oldfilterQ;
    MYFLT       oldfilterRate;
    FormSwep    filters[2];
    TwoZero     twozeroes[2];
} MOOG1;

#endif

