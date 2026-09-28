/*
    shaker.c:

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

/********************************************************/
/*    Maracha SImulation by Perry R. Cook, 1996         */
/********************************************************/
/********************************************************/
/*    In real life, each grain has an individual
        sound and envelope, but if you buy the
        notion that each sound is independent
        noise, the sum of a bunch of independent
        exponentially decaying enveloped noises
        is a single exponentially decaying enveloped
        noise.  shakeEnergy is an exponentially
        decaying, but reexcitable by shaking, energy
        expressing how loud a single collision will be.

This code would implement individual grain envelopes

            if (random(8) < 1) {
                noises[which] = 1024 * shakeEnergy;
                which += 1;
                if (which==MAX) which = 0;
            }
            input = 0.0;
            for (i=0;i<MAX;i++)     {
                input += noises[i] * noise_tick();
                noises[i] *= COLL_DECAY;
            }

But we're smarter than that!!!  See below
*/
#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "shaker.h"

/* Clamp counts before converting them to integers. */
static int32_t shaker_count(cs_double value, int32_t minimum, int32_t maximum)
{
    if (!(value > minimum)) return minimum;
    if (value >= maximum) return maximum;
    return (int32_t)value;
}

int32_t shakerset(CSOUND *csound, SHAKER *p)
{
    cs_float       amp = (*p->amp)*AMP_RSCALE; /* Normalise */

    p->shake_speed = FL(0.0008) + (amp * FL(0.0004));
    make_BiQuad(&p->filter);
    make_ADSR(&p->envelope, CS_ESR);
    p->res_freq = FL(3200.0);
    BiQuad_setFreqAndReson(p->filter, p->res_freq, FL(0.96));
    BiQuad_setEqualGainZeroes(p->filter);
    BiQuad_setGain(p->filter, FL(1.0));
    p->shakeEnergy = FL(0.0);
    p->noiseGain = FL(0.0);
    p->coll_damp = FL(0.95);
/*     p->shake_damp = 0.999f; */
/*     p->num_beans = 8; */
    ADSR_setAll(csound, &p->envelope,
                p->shake_speed,  p->shake_speed, FL(0.0),  p->shake_speed);
    p->num_beans = shaker_count(*p->beancount, 1, INT32_MAX);
    p->wait_time = 0x7FFFFFFE / p->num_beans;
    p->gain_norm = FL(0.0005);
    /* Keep ktimes latched at note initialization, as in the legacy code.
       Values of 64 or more repeat without a limit. */
    p->shake_num = shaker_count(*p->times, 0, 64);
    ADSR_keyOn(&p->envelope);
    p->kloop = trunc(p->h.insdshead->offtim * CS_EKR)
               - trunc(CS_EKR * *p->dettack);
    p->freq = -FL(1.0);        /* So will get changed */
    return OK;
}

int32_t shaker(CSOUND *csound, SHAKER *p)
{
    cs_float *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float fullscale = AMP_SCALE;
    cs_float amp = *p->amp * (FL(1.0) / fullscale); /* Normalise */
    cs_float shake = amp + amp;
    cs_float damp = *p->shake_damp;
    cs_float gain = p->gain_norm;
    cs_float ngain = p->noiseGain;
    cs_float sEnergy = p->shakeEnergy;
    cs_float shake_speed = FL(0.0008) + amp * FL(0.0004);

    if (p->freq != *p->kfreq)
      BiQuad_setFreqAndReson(p->filter, p->freq = *p->kfreq, FL(0.96));
    int32_t beans = shaker_count(*p->beancount, 1, INT32_MAX);
    if (p->num_beans != beans) {
      p->num_beans = beans;
      p->wait_time = 0x7FFFFFFE / p->num_beans;
    }
    if (shake_speed != p->shake_speed) {
      p->shake_speed = shake_speed;
      ADSR_setAll(csound,
                  &p->envelope, shake_speed, shake_speed, FL(0.0), shake_speed);
    }
    if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
    if (p->kloop > 0 && (--p->kloop) == 0) {
      p->shake_num = 0;
    }
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    gain *= p->num_beans;       /* Save work in loop */
    for (n=offset; n<nsmps; n++) {
        cs_float   lastOutput;
        cs_float   temp;

        ADSR_tick(&p->envelope);
        temp = p->envelope.value * shake;
        if (p->shake_num>0) {
          if (p->envelope.state==SUSTAIN) {
            if (p->shake_num < 64)
              p->shake_num -= 1;
            ADSR_keyOn(&p->envelope);
          }
        }
        if (temp > sEnergy)
          sEnergy = temp;
        sEnergy *= damp;   /*   Exponential System Decay */

    /* There's Roughly Constant Probablity of a Collision, and */
    /* Energy of Each Collision is Weighted by Exponentially   */
    /* Decaying System Energy.  All add together for total     */
    /* exponentially decaying sound energy.                    */
        if (csound->Rand31(csound->RandSeed31(csound)) <= p->wait_time) {
          ngain += gain * sEnergy;
        }
        /* Actual Sound is Random */
        lastOutput = ngain * ((cs_float) csound->Rand31(csound->RandSeed31(csound))
                              - FL(1073741823.5))
                           * (cs_float) (1.0 / 1073741823.0);
        /* Each (all) event(s) decay(s) exponentially */
        ngain *= p->coll_damp;

        lastOutput = BiQuad_tick(&p->filter, lastOutput);
        ar[n] = lastOutput * fullscale * FL(7.0); /* As too quiet */
    }
    p->noiseGain = ngain;
    p->shakeEnergy = sEnergy;
    return OK;
}

