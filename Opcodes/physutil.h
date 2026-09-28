/*
    physutil.h:

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
#if !defined(__PhysUtil_h)
#define __PhysUtil_h

/* Various filters etc for Physical models */

#include <math.h>

#define AMP_SCALE (csound->Get0dBFS(csound))
#define AMP_RSCALE (FL(1.)/AMP_SCALE)

/*******************************************/
/*  Noise Generator Class,                 */
/*  by Perry R. Cook, 1995-96              */
/*  White noise as often as you like.      */
/*  Recoded by John ffitch 1997            */
/*******************************************/

typedef cs_float Noise;

#define make_Noise(n) n = FL(0.0)

cs_float Noise_tick(CSOUND *, Noise *);
#define Noise_lastOut(n) (n)

/*******************************************/
/*  Linearly Interpolating Delay Line      */
/*  Object by Perry R. Cook 1995-96        */
/*  Recoded by John ffitch 1997            */
/*******************************************/

typedef struct DLineL{
    AUXCH       inputs;
    cs_float       lastOutput;
    int32        inPoint;
    int32        outPoint;
    int32        length;
    cs_float       alpha;
    cs_float       omAlpha;
} DLineL;

#define DLineL_lastOut(d)       ((d)->lastOutput)
void make_DLineL(CSOUND *,DLineL *, int32);
void DLineL_setDelay(DLineL *, cs_float);
cs_float DLineL_tick(DLineL *, cs_float);

/*******************************************/
/*  Envelope Class, Perry R. Cook, 1995-96 */
/*  This is the base class for envelopes.  */
/*  This one is capable of ramping state   */
/*  from where it is to a target value by  */
/*  a rate.  It also responds to simple    */
/*  KeyOn and KeyOff messages, ramping to  */
/*  1.0 on keyon and to 0.0 on keyoff.     */
/*  There are two tick (update value)      */
/*  methods, one returns the value, and    */
/*  other returns 0 if the envelope is at  */
/*  the target value (the state bit).      */
/*******************************************/

#define RATE_NORM       (FL(22050.0)/CS_ESR)

typedef struct Envelope {
    cs_float       value;
    cs_float       target;
    cs_float       rate;
    int32_t         state;
} Envelope;

void make_Envelope(Envelope*);
void Envelope_keyOn(Envelope*);
void Envelope_keyOff(Envelope*);
void Envelope_setRate(CSOUND *,Envelope*, cs_float);
void Envelope_setTarget(Envelope*, cs_float);
void Envelope_setValue(Envelope*,cs_float);
cs_float Envelope_tick(Envelope*);
void Envelope_print(CSOUND *,Envelope*);

/*******************************************/
/*  One Pole Filter Class,                 */
/*  by Perry R. Cook, 1995-96              */
/*  The parameter gain is an additional    */
/*  gain parameter applied to the filter   */
/*  on top of the normalization that takes */
/*  place automatically.  So the net max   */
/*  gain through the system equals the     */
/*  value of gain.  sgain is the combina-  */
/*  tion of gain and the normalization     */
/*  parameter, so if you set the poleCoeff */
/*  to alpha, sgain is always set to       */
/*  gain * (1.0 - fabs(alpha)).            */
/*******************************************/

typedef struct OnePole {
    cs_float gain;                 /* Start Filter subclass */
    cs_float outputs;
    /*    cs_float *inputs; */
/*     cs_float lastOutput;  */          /* End */
    cs_float poleCoeff;
    cs_float sgain;
} OnePole;

void make_OnePole(OnePole*);
/* void OnePole_clear(OnePole*); */
void OnePole_setPole(OnePole*, cs_float aValue);
void OnePole_setGain(OnePole*, cs_float aValue);
cs_float OnePole_tick(OnePole*, cs_float sample);
void OnePole_print(CSOUND*, OnePole*);

/*******************************************/
/*  DC Blocking Filter                     */
/*  by Perry R. Cook, 1995-96              */
/*  This guy is very helpful in, uh,       */
/*  blocking DC.  Needed because a simple  */
/*  low-pass reflection filter allows DC   */
/*  to build up inside recursive           */
/*  structures.                            */
/*******************************************/

typedef struct DCBlock {
    cs_float       gain;
    cs_float       outputs;
    cs_float       inputs;
/*     cs_float    lastOutput; */
} DCBlock;

void make_DCBlock(DCBlock*);
/* void DCBlock_clear(DCBlock*); */
cs_float DCBlock_tick(DCBlock*, cs_float);

/*******************************************/
/*  ADSR Subclass of the Envelope Class,   */
/*  by Perry R. Cook, 1995-96              */
/*  This is the traditional ADSR (Attack   */
/*  Decay, Sustain, Release) envelope.     */
/*  It responds to simple KeyOn and KeyOff */
/*  messages, keeping track of it's state. */
/*  There are two tick (update value)      */
/*  methods, one returns the value, and    */
/*  other returns the state (0 = A, 1 = D, */
/*  2 = S, 3 = R)                          */
/*******************************************/

#define ATTACK  (0)
#define DECAY   (1)
#define SUSTAIN (2)
#define RELEASE (3)
#define CLEAR   (4)

typedef struct ADSR {
    cs_float       value;                /* Envelope subclass */
    cs_float       target;
    cs_float       rate;
    int32_t         state;                  /* end */
    cs_float       attackRate;
    cs_float       decayRate;
    cs_float       sustainLevel;
    cs_float       releaseRate;
    cs_float       sr;
} ADSR;

void make_ADSR(ADSR*, cs_float sr);
void dest_ADSR(ADSR*);
void ADSR_keyOn(ADSR*);
void ADSR_keyOff(ADSR*);
void ADSR_setAttackRate(CSOUND *,ADSR*, cs_float);
void ADSR_setDecayRate(CSOUND *,ADSR*, cs_float);
void ADSR_setSustainLevel(CSOUND *,ADSR*, cs_float);
void ADSR_setReleaseRate(CSOUND *,ADSR*, cs_float);
void ADSR_setAll(CSOUND *,ADSR*, cs_float, cs_float, cs_float, cs_float);
void ADSR_setAllTimes(CSOUND *,ADSR*, cs_float, cs_float, cs_float, cs_float);
void ADSR_setTarget(CSOUND *,ADSR*, cs_float);
void ADSR_setValue(CSOUND *,ADSR*, cs_float);
cs_float ADSR_tick(ADSR*);
int32_t
ADSR_informTick(ADSR*);
cs_float ADSR_lastOut(ADSR*);

/*******************************************/
/*  BiQuad (2-pole, 2-zero) Filter Class,  */
/*  by Perry R. Cook, 1995-96              */
/*  See books on filters to understand     */
/*  more about how this works.  Nothing    */
/*  out of the ordinary in this version.   */
/*******************************************/

typedef struct BiQuad {
    cs_float       gain;                 /* Start if filter subclass */
    cs_float       inputs[2];
    cs_float       lastOutput;           /* End */
    cs_float       poleCoeffs[2];
    cs_float       zeroCoeffs[2];
} BiQuad;

void make_BiQuad(BiQuad*);
void dest_BiQuad(BiQuad*);
void BiQuad_clear(BiQuad*);
void BiQuad_setPoleCoeffs(BiQuad*, cs_float *);
void BiQuad_setZeroCoeffs(BiQuad*, cs_float *);
#define BiQuad_setGain(b,aValue)        ((b).gain = aValue)
#define BiQuad_setEqualGainZeroes(b)    \
        { (b).zeroCoeffs[1] = -FL(1.0); (b).zeroCoeffs[0] = FL(0.0); }
#define BiQuad_setFreqAndReson(b,freq,reson)    \
        { (b).poleCoeffs[1]= -((reson)*(reson)); \
          (b).poleCoeffs[0]= FL(2.0)*(reson)*\
            (cs_float)cos((cs_double)(freq)*CS_TPIDSR); }
cs_float BiQuad_tick(BiQuad*, cs_float);
#define BiQuad_lastOut(x)       (x)->lastOutput

#endif

