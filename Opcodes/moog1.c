/*  
    moog1.c:

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

#include "csdl.h"
#include "moog1.h"

extern void make_TwoZero(TwoZero *);
extern void TwoZero_setZeroCoeffs(TwoZero *, MYFLT*);
extern MYFLT TwoZero_tick(TwoZero *, MYFLT);

/********************************************/
/*  Sweepable Formant (2-pole)              */
/*  Filter Class, by Perry R. Cook, 1995-96 */
/*  See books on filters to understand      */
/*  more about how this works.  This drives */
/*  to a target at speed set by rate.       */
/********************************************/

void make_FormSwep(FormSwep *p)
{
    p->poleCoeffs[0] = p->poleCoeffs[1] = FL(0.0);
    p->gain          = FL(1.0);
    p->freq          = p->reson         = FL(0.0);
    p->currentGain   = FL(1.0);
    p->currentFreq   = p->currentReson  = FL(0.0);
    p->targetGain    = FL(1.0);
    p->targetFreq    = p->targetReson   = FL(0.0);
    p->deltaGain     = FL(0.0);
    p->deltaFreq     = p->deltaReson    = FL(0.0);
    p->sweepState    = FL(0.0);
    p->sweepRate     = FL(0.002);
    p->dirty         = 0;
    p->outputs[0]    = p->outputs[1] = FL(0.0);
}

/* void FormSwep_setFreqAndReson(FormSwep *p, MYFLT aFreq, MYFLT aReson) */
/* { */
/*     p->dirty = 0; */
/*     p->reson = p->currentReson = aReson; */
/*     p->freq = p->currentFreq = aFreq; */
/*     p->poleCoeffs[1] = - (aReson * aReson); */
/*     p->poleCoeffs[0] = 2.0*aReson*(MYFLT)cos((double)(twopi*aFreq/esr)); */
/* } */

void FormSwep_setStates(FormSwep *p, MYFLT aFreq, MYFLT aReson, MYFLT aGain)
{
    p->dirty = 0;
    p->freq  = p->targetFreq  = p->currentFreq  = aFreq;
    p->reson = p->targetReson = p->currentReson = aReson;
    p->gain  = p->targetGain  = p->currentGain  = aGain;
}

void FormSwep_setTargets(FormSwep *p, MYFLT aFreq, MYFLT aReson, MYFLT aGain)
{
    p->dirty = 1;
    p->targetFreq  = aFreq;
    p->targetReson = aReson;
    p->targetGain  = aGain;
    p->deltaFreq   = aFreq - p->currentFreq;
    p->deltaReson  = aReson - p->currentReson;
    p->deltaGain   = aGain - p->currentGain;
    p->sweepState  = FL(0.0);
}

MYFLT FormSwep_tick(FormSwep *p, MYFLT sample) /* Perform Filter Operation */
{
    MYFLT temp;

    if (p->dirty) {
      p->sweepState += p->sweepRate;
      if (p->sweepState>= FL(1.0)) {
        p->sweepState   = FL(1.0);
        p->dirty        = 0;
        p->currentReson = p->targetReson;
        p->reson        = p->targetReson;
        p->currentFreq  = p->targetFreq;
        p->freq         = p->targetFreq;
        p->currentGain  = p->targetGain;
        p->gain         = p->targetGain;
      }
      else {
        p->currentReson = p->reson + (p->deltaReson * p->sweepState);
        p->currentFreq  = p->freq + (p->deltaFreq * p->sweepState);
        p->currentGain  = p->gain + (p->deltaGain * p->sweepState);
      }
      p->poleCoeffs[1] = - (p->currentReson * p->currentReson);
      p->poleCoeffs[0] = FL(2.0) * p->currentReson *
        (MYFLT)cos(tpidsr * (double)p->currentFreq);
    }

    temp = p->currentGain * sample;
    temp += p->poleCoeffs[0] * p->outputs[0];
    temp += p->poleCoeffs[1] * p->outputs[1];
    p->outputs[1] = p->outputs[0];
    p->outputs[0] = temp;
    return temp;
}

static MYFLT Samp_tick(Wave *p)
{
    long    temp, temp1;
    MYFLT   temp_time, alpha;
    MYFLT   lastOutput;

    p->time += p->rate;                  /*  Update current time    */
    while (p->time >= p->wave->flen)     /*  Check for end of sound */
      p->time -= p->wave->flen;          /*  loop back to beginning */
    while (p->time < FL(0.))             /*  Check for end of sound */
      p->time += p->wave->flen;          /*  loop back to beginning */

    temp_time = p->time;

    if (p->phase != FL(0.)) {
      temp_time += p->phase;             /*  Add phase offset       */
      while (temp_time >= p->wave->flen) /*  Check for end of sound */
        temp_time -= p->wave->flen;      /*  loop back to beginning */
      while (temp_time < FL(0.))         /*  Check for end of sound */
        temp_time += p->wave->flen;      /*  loop back to beginning */
    }

    temp = (long) temp_time;    /*  Integer part of time address    */
    temp1 = temp + 1;
    if (temp1==p->wave->flen) temp1 = 0; /* Wrap!! */
#ifdef DEBUG
    printf("Samp_tick: in (%d,%d)\n", temp, temp1);
#endif
    /*  fractional part of time address */
    alpha = temp_time - (MYFLT)temp;
    lastOutput = p->wave->ftable[temp];  /* Do linear interpolation */
#ifdef DEBUG
    printf("Part 1=%f\n", lastOutput);
#endif
    /* same as alpha*data[temp+1] + (1-alpha)data[temp] */
    lastOutput += (alpha * (p->wave->ftable[temp+1] - lastOutput));
    /* End of vibrato tick */
    return lastOutput;
}


int Moog1set(MOOG1 *p)
{
    FUNC        *ftp;
    MYFLT       tempCoeffs[2] = {FL(0.0),-FL(1.0)};

    make_ADSR(&p->adsr);
    make_OnePole(&p->filter);
    make_TwoZero(&p->twozeroes[0]);
    TwoZero_setZeroCoeffs(&p->twozeroes[0], tempCoeffs);
    make_TwoZero(&p->twozeroes[1]);
    TwoZero_setZeroCoeffs(&p->twozeroes[1], tempCoeffs);
    make_FormSwep(&p->filters[0]);
    make_FormSwep(&p->filters[1]);

    if ((ftp = ftfind(p->iatt)) != NULL) p->attk.wave = ftp; /* mandpluk */
    else return NOTOK;
    if ((ftp = ftfind(p->ifn )) != NULL) p->loop.wave = ftp; /* impuls20 */
    else return NOTOK;
    if ((ftp = ftfind(p->ivfn)) != NULL) p->vibr.wave = ftp; /* sinewave */
    else return NOTOK;
    p->attk.time = p->attk.phase = FL(0.0);
    p->loop.time = p->loop.phase = FL(0.0);
    p->vibr.time = p->vibr.phase = FL(0.0);
    p->oldfilterQ = p->oldfilterRate = FL(0.0);
    ADSR_setAllTimes(&p->adsr, FL(0.001), FL(1.5), FL(0.6), FL(0.250));
    ADSR_setAll(&p->adsr, FL(0.05), FL(0.00003), FL(0.6), FL(0.0002));
    ADSR_keyOn(&p->adsr);
    return OK;
}

int Moog1(MOOG1 *p)
{
    MYFLT       amp = *p->amp * AMP_RSCALE; /* Normalised */
    MYFLT       *ar = p->ar;
    long        nsmps = ksmps;
    MYFLT       temp;
    MYFLT       vib = *p->vibAmt;

    p->baseFreq = *p->frequency;
    p->attk.rate = p->baseFreq * FL(0.01) * p->attk.wave->flen * onedsr;
    p->loop.rate = p->baseFreq            * p->loop.wave->flen * onedsr;
    p->attackGain = amp * FL(0.5);
    p->loopGain = amp;
    if (*p->filterQ != p->oldfilterQ) {
      p->oldfilterQ = *p->filterQ;
      temp = p->oldfilterQ + FL(0.05);
      FormSwep_setStates(&p->filters[0], FL(2000.0), temp,
                         FL(2.0) * (FL(1.0) - temp));
      FormSwep_setStates(&p->filters[1], FL(2000.0), temp,
                         FL(2.0) * (FL(1.0) - temp));
      temp = p->oldfilterQ + FL(0.099);
      FormSwep_setTargets(&p->filters[0],   FL(0.0), temp,
                          FL(2.0) * (FL(1.0) - temp));
      FormSwep_setTargets(&p->filters[1],   FL(0.0), temp,
                          FL(2.0) * (FL(1.0) - temp));
    }
    if (*p->filterRate != p->oldfilterRate) {
      p->oldfilterRate = *p->filterRate;
      p->filters[0].sweepRate = p->oldfilterRate * RATE_NORM;
      p->filters[1].sweepRate = p->oldfilterRate * RATE_NORM;
    }
    p->vibr.rate = *p->vibf * p->vibr.wave->flen * onedsr;
/*     printf("   (%f): modDepth=%f\tfilterQ=%f\tfilterRate=%f\n", */
/*             last, vib, *p->filterQ, *p->filterRate); */
/*     printf("FormSwep: poleCoeffs=%f, %f\tfreq=%.2f\treson=%f\n", */
/*            p->filters[0].poleCoeffs[0], p->filters[0].poleCoeffs[1], */
/*            p->filters[0].freq, p->filters[0].reson); */
/*     printf("        : dirty=%d\ttargetFreq=%f\ttargetReson=%f\n", */
/*            p->filters[0].dirty, p->filters[0].targetFreq, */
/*            p->filters[0].targetReson); */
/*     printf("        : targetGain=%f\tcurrentFreq=%f\tcurrentReson=%f\n", */
/*            p->filters[0].targetGain, p->filters[0].currentFreq, */
/*            p->filters[0].currentReson); */
/*     printf("        : currentGain=%f\tdeltaFreq=%.2f\tdeltaReson=%f\n", */
/*            p->filters[0].currentGain, p->filters[0].deltaFreq, */
/*            p->filters[0].deltaReson); */
/*     printf("        : deltaGain=%f\tsweepState=%f\tsweepRate=%f\n", */
/*            p->filters[0].deltaGain, p->filters[0].sweepState, */
/*            p->filters[0].sweepRate); */
/*     printf("FormSwep: poleCoeffs=%f, %f\tfreq=%.2f\treson=%f\n", */
/*            p->filters[1].poleCoeffs[0], p->filters[1].poleCoeffs[1], */
/*            p->filters[1].freq, p->filters[1].reson); */
/*     printf("        : dirty=%d\ttargetFreq=%f\ttargetReson=%f\n", */
/*            p->filters[1].dirty, p->filters[1].targetFreq, */
/*            p->filters[1].targetReson); */
/*     printf("        : targetGain=%f\tcurrentFreq=%f\tcurrentReson=%f\n", */
/*            p->filters[1].targetGain, p->filters[1].currentFreq, */
/*            p->filters[1].currentReson); */
/*     printf("        : currentGain=%f\tdeltaFreq=%.2f\tdeltaReson=%f\n", */
/*            p->filters[1].currentGain, p->filters[1].deltaFreq, */
/*            p->filters[1].deltaReson); */
/*     printf("        : deltaGain=%f\tsweepState=%f\tsweepRate=%f\n", */
/*            p->filters[1].deltaGain, p->filters[1].sweepState, */
/*            p->filters[1].sweepRate); */
/*     printf("TwoZero: %f %f\n", */
/*            p->twozeroes[0].zeroCoeffs[0],p->twozeroes[0].zeroCoeffs[1]); */
/*     printf("TwoZero: %f %f\n", */
/*            p->twozeroes[1].zeroCoeffs[0],p->twozeroes[1].zeroCoeffs[1]); */

    do {
      MYFLT     temp;
      MYFLT     output;
      long      itemp;
      MYFLT     temp_time, alpha;

      if (vib != FL(0.0)) {
        temp = vib * Samp_tick(&p->vibr);
        p->loop.rate = p->baseFreq * (FL(1.0) + temp) *
                       (MYFLT)(p->loop.wave->flen) * onedsr;
      }

      p->attk.time += p->attk.rate;           /*  Update current time    */
#ifdef DEBUG
      printf("Attack_time=%f\tAttack_rate=%f\n", p->attk.time, p->attk.rate);
#endif
      temp_time = p->attk.time;
      if (p->attk.time >= (MYFLT)p->attk.wave->flen)
        output = FL(0.0);                                        /* One shot */
      else {
        itemp = (long) temp_time;    /*  Integer part of time address    */
                                     /*  fractional part of time address */
        alpha = temp_time - (MYFLT)itemp;
#ifdef DEBUG
        printf("Attack: (%d, %d), alpha=%f\t", itemp, itemp+1, alpha);
#endif
        output = p->attk.wave->ftable[itemp]; /* Do linear interpolation */
                  /*  same as alpha*data[itemp+1] + (1-alpha)data[itemp] */
#ifdef DEBUG
        printf("->%f+\n", output);
#endif
        output += (alpha * (p->attk.wave->ftable[itemp+1] - output));
        output *= p->attackGain;
                                                   /* End of attack tick */
      }
#ifdef DEBUG
      printf("After Attack: %f\n", output);
#endif
      output += p->loopGain * Samp_tick(&p->loop);
#ifdef DEBUG
      printf("Before OnePole: %f\n", output);
#endif
      output = OnePole_tick(&p->filter, output);
#ifdef DEBUG
      printf("After OnePole: %f\n", output);
#endif
      output *= ADSR_tick(&p->adsr);
#ifdef DEBUG
      printf("Sampler_tick: %f\n", output);
#endif
      output = TwoZero_tick(&p->twozeroes[0], output);
#ifdef DEBUG
      printf("TwoZero0_tick: %f\n", output);
#endif
      output = FormSwep_tick(&p->filters[0], output);
#ifdef DEBUG
      printf("Filters0_tick: %f\n", output);
#endif
      output = TwoZero_tick(&p->twozeroes[1], output);
#ifdef DEBUG
      printf("TwoZero1_tick: %f\n", output);
#endif
      output = FormSwep_tick(&p->filters[1], output);
#ifdef DEBUG
      printf("Filter2_tick: %f\n", output);
#endif
      *ar++ = output*AMP_SCALE*FL(8.0);
    } while (--nsmps);
    return OK;
}

