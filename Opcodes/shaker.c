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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
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

#include "csdl.h"
#include "shaker.h"

int shakerset(SHAKER *p)
{
    MYFLT       amp = (*p->amp)*AMP_RSCALE; /* Normalise */
    ENVIRON     *csound = p->h.insdshead->csound;

    p->shake_speed = FL(0.0008) + (amp * FL(0.0004));
    make_BiQuad(&p->filter);
    make_ADSR(&p->envelope);
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
    p->num_beans = (int)*p->beancount;
    if (p->num_beans<1) p->num_beans = 1;
    p->wait_time = RAND_MAX / p->num_beans;
    p->gain_norm = FL(0.0005);
    p->shake_num = (int)*p->times;
    ADSR_keyOn(&p->envelope);
    p->kloop = (int)(p->h.insdshead->offtim * ekr) - (int)(ekr* *p->dettack);
    p->freq = -FL(1.0);        /* So will get changed */
/*     printf("Shaker_set: num_beans=%ld\twait_time=%ld\tshake_num=%ld\n" */
/*            "\tshake_speed=%f\tres_freq=%f\n" */
/*            "\tcoll_damp=%f\tshakeEnergy=%f\tnoiseGain=%f\n" */
/*            "\tgain_norm=%f\tkloop=%d\n", */
/*            p->num_beans,p->wait_time,p->shake_num, */
/*            p->shake_speed,p->res_freq,p->coll_damp,p->shakeEnergy, */
/*            p->noiseGain,p->gain_norm,p->kloop); */
/* printf("Env:\tvalue=%f\ttarget=%f\trate=%f\n" */
/*        "state=%d\tattackRate=%f\tdecayRate=%f\n" */
/*        "sustainLevel=%f\treleaseRate=%f\n", */
/*        p->envelope.value,p->envelope.target,p->envelope.rate,p->envelope.state, */
/*        p->envelope.attackRate,p->envelope.decayRate,p->envelope.sustainLevel, */
/*        p->envelope.releaseRate); */
/* printf("BiQ:\tzeroCoeffs[0] = %f\tzeroCoeffs[1] = %f\npoleCoeffs[0] = %f\t" */
/*        X_1131,"poleCoeffs[1] = %f\ngain = %f\tinputs[0] = %f\t" */
/*        "inputs[1] = %f\nlastOutput = %f\n",     */
/*        p->filter.zeroCoeffs[0], p->filter.zeroCoeffs[1], */
/*        p->filter.poleCoeffs[0], p->filter.poleCoeffs[1], */
/*        p->filter.gain, p->filter.inputs[0], */
/*        p->filter.inputs[1], p->filter.lastOutput); */
    return OK;
}

int shaker(SHAKER *p)
{
    MYFLT *ar = p->ar;
    long nsmps = ksmps;
    MYFLT amp = (*p->amp)*AMP_RSCALE; /* Normalise */
    MYFLT shake = amp + amp;
    MYFLT damp = *p->shake_damp;
    MYFLT gain = p->gain_norm;
    MYFLT ngain = p->noiseGain;
    MYFLT sEnergy = p->shakeEnergy;
    MYFLT shake_speed = FL(0.0008) + amp * FL(0.0004);
    ENVIRON *csound = p->h.insdshead->csound;

    if (p->freq != *p->kfreq)
      BiQuad_setFreqAndReson(p->filter, p->freq = *p->kfreq, FL(0.96));
    if (p->num_beans != (int)*p->beancount) { /* Bean Count */
      p->num_beans = (int)*p->beancount;
      p->wait_time = RAND_MAX / p->num_beans;
    }
    if (shake_speed != p->shake_speed) {
      p->shake_speed = shake_speed;
      ADSR_setAll(csound,
                  &p->envelope, shake_speed, shake_speed, FL(0.0), shake_speed);
    }
    if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
    if ((--p->kloop) == 0) {
      p->shake_num = 0;
    }
    gain *= p->num_beans;       /* Save work in loop */
    do {
        MYFLT   lastOutput;
        MYFLT   temp;

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

        if (rand() < p->wait_time) {
          ngain += gain * sEnergy;
        }
        lastOutput = ngain *
                     ((MYFLT) rand() - (MYFLT)(RAND_MAX/2)) /
                   (MYFLT)(RAND_MAX/2);  /* Actual Sound is Random   */
        ngain *= p->coll_damp;               /* Each (all) event(s)      */
                                             /* decay(s) exponentially   */
        lastOutput = BiQuad_tick(&p->filter, lastOutput);
        *ar++ = lastOutput*AMP_SCALE* FL(7.0); /* As too quiet */
    } while (--nsmps);
    p->noiseGain = ngain;
    p->shakeEnergy = sEnergy;
    return OK;
}


