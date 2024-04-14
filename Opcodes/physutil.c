/*
    physutil.c:

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

/* This file contains a collection of utilities for the Physical Model
   opcodes, in no particular order
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include <stdlib.h>
#include "physutil.h"

/*******************************************/
/*  Noise Generator Class,                 */
/*  by Perry R. Cook, 1995-96              */
/*  White noise as often as you like.      */
/*******************************************/

/* Return random MYFLT float between -1.0 and 1.0 */

MYFLT Noise_tick(CSOUND *csound, Noise *n)
{
    MYFLT temp = (MYFLT) csound->Rand31(csound->RandSeed1(csound)) - FL(1073741823.5);
    temp *= (MYFLT) (1.0 / 1073741823.0);
    *n = (Noise) temp;
    return temp;
}

/*******************************************/
/*  Linearly Interpolating Delay Line      */
/*  Object by Perry R. Cook 1995-96        */
/*  This one uses a delay line of maximum  */
/*  length specified on creation, and      */
/*  linearly interpolates fractional       */
/*  length.  It is designed to be more     */
/*  efficient if the delay length is not   */
/*  changed very often.                    */
/*******************************************/

void make_DLineL(CSOUND *csound, DLineL *p, int32 max_length)
{
    p->length = max_length;
    csound->AuxAlloc(csound, max_length * sizeof(MYFLT), &p->inputs);
    p->outPoint = 0;
    p->lastOutput = FL(0.0);
    p->inPoint = max_length >> 1;
}

void DLineL_setDelay(DLineL *p, MYFLT lag)
{
    MYFLT outputPointer = p->inPoint - lag; /* read chases write, +1 for interp. */
    while (outputPointer<FL(0.0))
      outputPointer += (MYFLT)p->length;           /* modulo maximum length */
    while (outputPointer>=(MYFLT)p->length)
      outputPointer -= (MYFLT)p->length;           /* modulo maximum length */
    p->outPoint = (int32) outputPointer;           /* integer part */
    p->alpha = outputPointer - (MYFLT)p->outPoint; /* fractional part */
    p->omAlpha = FL(1.0) - p->alpha;               /* 1.0 - fractional part */
}

MYFLT DLineL_tick(DLineL *p, MYFLT sample) /* Take one, yield one */
{
    MYFLT lastOutput;

    ((MYFLT*)p->inputs.auxp)[p->inPoint++] = sample; /*  Input next sample */
    if (UNLIKELY( p->inPoint ==  p->length))         /* Check for end condition */
      p->inPoint -= p->length;
                                /* first 1/2 of interpolation */
    lastOutput = ((MYFLT*)p->inputs.auxp)[p->outPoint++] * p->omAlpha;
    if ( p->outPoint< p->length) {         /*  Check for end condition */
                                /* second 1/2 of interpolation    */
      lastOutput += ((MYFLT*)p->inputs.auxp)[p->outPoint] * p->alpha;
    }
    else {                      /*  if at end . . .  */
                                /* second 1/2 of interpolation */
      lastOutput +=  ((MYFLT*)p->inputs.auxp)[0]*p->alpha;
      p->outPoint -=  p->length;
    }
    return (p->lastOutput = lastOutput);
}

/*******************************************/
/*  Envelope Class, Perry R. Cook, 1995-96 */
/*  This is the base class for envelopes.  */
/*  This one is capable of ramping state   */
/*  from where it is to a target value by  */
/*  a rate.                                */
/*******************************************/

void make_Envelope(Envelope *e)
{
    e->target = FL(0.0);
    e->value = FL(0.0);
    e->rate = FL(0.001);
    e->state = 1;
}

void Envelope_keyOn(Envelope *e)
{
    e->target = FL(1.0);
    if (e->value != e->target) e->state = 1;
}

void Envelope_keyOff(Envelope *e)
{
    e->target = FL(0.0);
    if (e->value != e->target) e->state = 1;
}

void Envelope_setRate(CSOUND *csound, Envelope *e, MYFLT aRate)
{
    if (UNLIKELY(aRate < FL(0.0))) {
        csound->Warning(csound, "%s", Str("negative rates not "
                                    "allowed!!, correcting\n"));
        e->rate = -aRate;
    }
    else
      e->rate = aRate;
    //    printf("Env setRate: %p rate=%f value=%f target=%f\n", e,
    //           e->rate, e->value, e->target);
}

void Envelope_setTarget(Envelope *e, MYFLT aTarget)
{
    e->target = aTarget;
    if (e->value != e->target) e->state = 1;
}

void Envelope_setValue(Envelope *e, MYFLT aValue)
{
    e->state = 0;
    e->target = aValue;
    e->value = aValue;
}

MYFLT Envelope_tick(Envelope *e)
{
    //    printf("(Envelope_tick: %p state=%d target=%f, rate=%f, value=%f => ", e,
    //           e->state, e->target, e->rate, e->value);
    if (e->state) {
      if (e->target > e->value) {
        e->value += e->rate;
        if (e->value >= e->target) {
          e->value = e->target;
          e->state = 0;
        }
      }
      else {
        e->value -= e->rate;
        if (e->value <= e->target) {
          e->value = e->target;
          e->state = 0;
        }
      }
    }
    //           printf("%f) ", e->value);
    return e->value;
}

void Envelope_print(CSOUND *csound, Envelope *p)
{
    csound->Message(csound, Str("Envelope: value=%f target=%f"
                                " rate=%f state=%d\n"),
                    p->value, p->target, p->rate, p->state);
}

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

void make_OnePole(OnePole* p)
{
    p->poleCoeff = FL(0.9);
    p->gain = FL(1.0);
    p->sgain = FL(0.1);
    p->outputs = FL(0.0);
}

void OnePole_setPole(OnePole* p, MYFLT aValue)
{
    p->poleCoeff = aValue;
    if (p->poleCoeff > FL(0.0))           /*  Normalize gain to 1.0 max */
      p->sgain = p->gain * (FL(1.0) - p->poleCoeff);
    else
      p->sgain = p->gain * (FL(1.0) + p->poleCoeff);
}

void OnePole_setGain(OnePole* p, MYFLT aValue)
{
    p->gain = aValue;
    if (p->poleCoeff > FL(0.0))
      p->sgain = p->gain * (FL(1.0) - p->poleCoeff);  /* Normalize gain 1.0 max */
    else
      p->sgain = p->gain * (FL(1.0) + p->poleCoeff);
}

MYFLT OnePole_tick(OnePole* p, MYFLT sample)  /*   Perform Filter Operation */
{
    p->outputs = (p->sgain * sample) + (p->poleCoeff * p->outputs);
    return p->outputs;
}

#ifdef BETA
void OnePole_print(CSOUND *csound, OnePole *p)
{
    csound->Message(csound,
                    "OnePole: gain=%f outputs=%f poleCoeff=%f sgain=%f\n",
                    p->gain, p->outputs, p->poleCoeff, p->sgain);
}
#endif

/*******************************************/
/*  DC Blocking Filter                     */
/*  by Perry R. Cook, 1995-96              */
/*  This guy is very helpful in, uh,       */
/*  blocking DC.  Needed because a simple  */
/*  low-pass reflection filter allows DC   */
/*  to build up inside recursive           */
/*  structures.                            */
/*******************************************/

void make_DCBlock(DCBlock* p)
{
    p->outputs = FL(0.0);
    p->inputs = FL(0.0);
}

MYFLT DCBlock_tick(DCBlock* p, MYFLT sample)
{
    p->outputs = sample - p->inputs + FL(0.99) * p->outputs;
    p->inputs = sample;
    return p->outputs;
}

/*******************************************/
/*  ADSR Subclass of the Envelope Class,   */
/*  by Perry R. Cook, 1995-96              */
/*  This is the traditional ADSR (Attack   */
/*  Decay, Sustain, Release) ADSR.         */
/*  It responds to simple KeyOn and KeyOff */
/*  messages, keeping track of it's state. */
/*  There are two tick (update value)      */
/*  methods, one returns the value, and    */
/*  other returns the state (0 = A, 1 = D, */
/*  2 = S, 3 = R)                          */
/*******************************************/

void make_ADSR(ADSR *a, MYFLT sr)
{
    make_Envelope((Envelope*)a);
    a->target = FL(0.0);
    a->value = FL(0.0);
    a->attackRate = FL(0.001);
    a->decayRate = FL(0.001);
    a->sustainLevel = FL(0.5);
    a->releaseRate = FL(0.01);
    a->state = ATTACK;
    a->sr = sr;
}

void ADSR_keyOn(ADSR *a)
{
    a->target = FL(1.0);
    a->rate = a->attackRate;
    a->state = ATTACK;
}

void ADSR_keyOff(ADSR *a)
{
    a->target = FL(0.0);
    a->rate = a->releaseRate;
    a->state = RELEASE;
}

void ADSR_setAttackRate(CSOUND *csound, ADSR *a, MYFLT aRate)
{
    if (UNLIKELY(aRate < FL(0.0))) {
      csound->Warning(csound, "%s", Str("negative rates not allowed!!,"
                                  " correcting\n"));
      a->attackRate = -aRate;
    }
    else a->attackRate = aRate;
    a->attackRate *= (FL(22050.0)/a->sr);
}

void ADSR_setDecayRate(CSOUND *csound, ADSR *a, MYFLT aRate)
{
    if (UNLIKELY(aRate < FL(0.0))) {
      csound->Warning(csound,
                      "%s", Str("negative rates not allowed!!, correcting\n"));
      a->decayRate = -aRate;
    }
    else a->decayRate = aRate;
    a->decayRate *= (FL(22050.0)/a->sr);
}

void ADSR_setSustainLevel(CSOUND *csound, ADSR *a, MYFLT aLevel)
{
    if (UNLIKELY(aLevel < FL(0.0) )) {
      csound->Warning(csound,
                      "%s", Str("Sustain level out of range!!, correcting\n"));
      a->sustainLevel = FL(0.0);
    }
    else a->sustainLevel = aLevel;
}

void ADSR_setReleaseRate(CSOUND *csound, ADSR *a, MYFLT aRate)
{
    if (UNLIKELY(aRate < FL(0.0))) {
      csound->Warning(csound,
                      "%s", Str("negative rates not allowed!!, correcting\n"));
      a->releaseRate = -aRate;
    }
    else a->releaseRate = aRate;
    a->releaseRate *= (FL(22050.0)/a->sr);
}

void ADSR_setAttackTime(CSOUND *csound, ADSR *a, MYFLT aTime)
{
    if (UNLIKELY(aTime < FL(0.0))) {
      csound->Warning(csound,
                      "%s", Str("negative times not allowed!!, correcting\n"));
      a->attackRate = FL(1.0) /(-aTime*a->sr);
    }
    else a->attackRate = FL(1.0) / (aTime*a->sr);
}

void ADSR_setDecayTime(CSOUND *csound, ADSR *a, MYFLT aTime)
{
    if (UNLIKELY(aTime < FL(0.0))) {
      csound->Warning(csound,
                      "%s", Str("negative times not allowed!!, correcting\n"));
      a->decayRate = FL(1.0) /(-aTime*a->sr);
    }
    else a->decayRate = FL(1.0) / (aTime*a->sr);
}

void ADSR_setReleaseTime(CSOUND *csound, ADSR *a, MYFLT aTime)
{
    if (UNLIKELY(aTime < FL(0.0))) {
      csound->Warning(csound,
                      "%s", Str("negative times not allowed!!, correcting\n"));
      a->releaseRate = FL(1.0) /(-aTime*a->sr);
    }
    else a->releaseRate = FL(1.0) / (aTime*a->sr);
}

void ADSR_setAllTimes(CSOUND *csound, ADSR *a, MYFLT attTime, MYFLT decTime,
                      MYFLT susLevel, MYFLT relTime)
{
    ADSR_setAttackTime(csound, a, attTime);
    ADSR_setDecayTime(csound, a, decTime);
    ADSR_setSustainLevel(csound, a, susLevel);
    ADSR_setReleaseTime(csound, a, relTime);
}

void ADSR_setAll(CSOUND *csound, ADSR *a, MYFLT attRate, MYFLT decRate,
                 MYFLT susLevel, MYFLT relRate)
{
    ADSR_setAttackRate(csound, a, attRate);
    ADSR_setDecayRate(csound, a, decRate);
    ADSR_setSustainLevel(csound, a, susLevel);
    ADSR_setReleaseRate(csound, a, relRate);
}

void ADSR_setTarget(CSOUND *csound, ADSR *a, MYFLT aTarget)
{
    a->target = aTarget;
    if (a->value <a-> target) {
      a->state = ATTACK;
      ADSR_setSustainLevel(csound, a, a->target);
      a->rate = a->attackRate;
    }
    if (a->value > a->target) {
      ADSR_setSustainLevel(csound, a, a->target);
      a->state = DECAY;
      a->rate = a->decayRate;
    }
}

void ADSR_setValue(CSOUND *csound, ADSR *a, MYFLT aValue)
{
    a->state = SUSTAIN;
    a->target = aValue;
    a->value = aValue;
    ADSR_setSustainLevel(csound, a, aValue);
    a->rate = FL(0.0);
}

MYFLT ADSR_tick(ADSR *a)
{
    if (a->state==ATTACK) {
      a->value += a->rate;
      if (a->value >= a->target) {
        a->value = a->target;
        a->rate = a->decayRate;
        a->target = a->sustainLevel;
        a->state = DECAY;
      }
    }
    else if (a->state==DECAY) {
      a->value -= a->decayRate;
      if (a->value <= a->sustainLevel) {
        a->value = a->sustainLevel;
        a->rate = FL(0.0);
        a->state = SUSTAIN;
      }
    }
    else if (a->state==RELEASE)  {
      a->value -= a->releaseRate;
      if (a->value <= FL(0.0)) {
        a->value = FL(0.0);
        a->state = CLEAR;
      }
    }
    return a->value;
}

/*******************************************/
/*  BiQuad (2-pole, 2-zero) Filter Class,  */
/*  by Perry R. Cook, 1995-96              */
/*  See books on filters to understand     */
/*  more about how this works.  Nothing    */
/*  out of the ordinary in this version.   */
/*******************************************/

void make_BiQuad(BiQuad *b)
{
    b->zeroCoeffs[0] = FL(0.0);
    b->zeroCoeffs[1] = FL(0.0);
    b->poleCoeffs[0] = FL(0.0);
    b->poleCoeffs[1] = FL(0.0);
    b->gain = FL(1.0);
/*     BiQuad_clear(b); */
    b->inputs[0] = FL(0.0);
    b->inputs[1] = FL(0.0);
    b->lastOutput = FL(0.0);
}

void BiQuad_clear(BiQuad *b)
{
    b->inputs[0] = FL(0.0);
    b->inputs[1] = FL(0.0);
    b->lastOutput = FL(0.0);
}

void BiQuad_setPoleCoeffs(BiQuad *b, MYFLT *coeffs)
{
    b->poleCoeffs[0] = coeffs[0];
    b->poleCoeffs[1] = coeffs[1];
}

void BiQuad_setZeroCoeffs(BiQuad *b, MYFLT *coeffs)
{
    b->zeroCoeffs[0] = coeffs[0];
    b->zeroCoeffs[1] = coeffs[1];
}

MYFLT BiQuad_tick(BiQuad *b, MYFLT sample) /*   Perform Filter Operation   */
{                               /*  Biquad is two pole, two zero filter  */
    MYFLT temp;                 /*  Look it up in your favorite DSP text */

    temp = sample * b->gain;                     /* Here's the math for the  */
    temp += b->inputs[0] * b->poleCoeffs[0];     /* version which implements */
    temp += b->inputs[1] * b->poleCoeffs[1];     /* only 2 state variables.  */

    b->lastOutput = temp;                               /* This form takes   */
    b->lastOutput += (b->inputs[0] * b->zeroCoeffs[0]); /* 5 multiplies and  */
    b->lastOutput += (b->inputs[1] * b->zeroCoeffs[1]); /* 4 adds            */
    b->inputs[1] = b->inputs[0];                        /* and 3 moves       */
    b->inputs[0] = temp;                        /* like the 2 state-var form */

    return b->lastOutput;

}

