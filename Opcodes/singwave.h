/*
    singwave.h:

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

/*******************************************/
/*  "Singing" Looped Soundfile Class,      */
/*  by Perry R. Cook, 1995-96              */
/*  This Object contains all that's needed */
/*  to make a pitched musical sound, like  */
/*  a simple voice or violin.  In general, */
/*  it will not be used alone (because of  */
/*  of munchinification effects from pitch */
/*  shifting.  It will be used as an       */
/*  excitation source for other instruments*/
/*******************************************/

#if !defined(__SingWave_h)
#define __SingWave_h



extern MYFLT phonGains[32][2];
extern MYFLT phonParams[32][4][3];
extern char phonemes[32][4];

/*******************************************/
/*  Modulator Class, Perry R. Cook, 1995-96*/
/*  This Object combines random and        */
/*  periodic modulations to give a nice    */
/*  natural human modulation function.     */
/*******************************************/

#include "physutil.h"
#include "clarinet.h"
#include "moog1.h"

typedef struct SubNoise {
     Noise      lastOutput;
     int32_t        counter;
     int32_t        howOften;
} SubNoise;

typedef struct Modulatr {
    FUNC     *wave;
    MYFLT    v_rate;
    MYFLT    v_time;
    MYFLT    v_phase;
    MYFLT    v_lastOutput;
    SubNoise noise;
    OnePole  onepole;
    MYFLT    vibAmt;
    MYFLT    lastOutput;
} Modulatr;

typedef struct SingWave {
  OPDS        h;
    Modulatr    modulator;
    Envelope    envelope;
    Envelope    pitchEnvelope;
    FUNC        *wave;
    MYFLT       rate;
    MYFLT       sweepRate;
    MYFLT       mytime;
    MYFLT       lastOutput;
} SingWave;

/*******************************************/
/*  4 Formant Synthesis Instrument         */
/*  by Perry R. Cook, 1995-96              */
/*  This instrument contains an excitation */
/*  singing wavetable (looping wave with   */
/*  random and periodic vibrato, smoothing */
/*  on frequency, etc.), excitation noise, */
/*  and four sweepable complex resonances. */
/*                                         */
/*  Measured Formant data (from me) is     */
/*  included, and enough data is there to  */
/*  support either parallel or cascade     */
/*  synthesis.  In the floating point case */
/*  cascade synthesis is the most natural  */
/*  so that's what you'll find here.       */
/*                                         */
/*  For right now, there's a simple command*/
/*  line score interface consisting of 3   */
/*  letter symbols for the phonemes, =xx   */
/*  sets the pitch to x, + and - add and   */
/*  subtract a half step, and ... makes it */
/*  keep doing what it's doing for longer. */
/*******************************************/

typedef struct VOICF {
    OPDS         h;
    MYFLT       *ar;                  /* Output */
    MYFLT       *amp, *frequency;
    MYFLT       *phoneme, *formant;
    MYFLT       *vibf, *vibAmt;
    MYFLT       *ifn, *ivfn;

    MYFLT       oldform;
    int32_t
    ph;
    MYFLT       basef;
    MYFLT       lastGain;
    SingWave    voiced;
    Noise       noise;
    Envelope    noiseEnv;
    FormSwep    filters[4];
    OnePole     onepole;
    OneZero     onezero;
} VOICF;

#endif

