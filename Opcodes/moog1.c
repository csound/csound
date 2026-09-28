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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "moog1.h"

extern void make_TwoZero(TwoZero *);
extern void TwoZero_setZeroCoeffs(TwoZero *, cs_float*);
extern cs_float TwoZero_tick(TwoZero *, cs_float);

/********************************************/
/*  Sweepable Formant (2-pole)              */
/*  Filter Class, by Perry R. Cook, 1995-96 */
/*  See books on filters to understand      */
/*  more about how this works.  This drives */
/*  to a target at speed set by rate.       */
/********************************************/

static void make_FormSwep(FormSwep *p)
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

/* void FormSwep_setFreqAndReson(FormSwep *p, cs_float aFreq, cs_float aReson) */
/* { */
/*     p->dirty = 0; */
/*     p->reson = p->currentReson = aReson; */
/*     p->freq = p->currentFreq = aFreq; */
/*     p->poleCoeffs[1] = - (aReson * aReson); */
/*     p->poleCoeffs[0] = 2.0*aReson*(cs_float)cos((double)(twopi*aFreq/esr)); */
/* } */

static void FormSwep_setStates(OPDS *pp, FormSwep *p, cs_float aFreq,
                               cs_float aReson, cs_float aGain)
{
    p->dirty = 0;
    p->freq  = p->targetFreq  = p->currentFreq  = aFreq;
    p->reson = p->targetReson = p->currentReson = aReson;
    p->gain  = p->targetGain  = p->currentGain  = aGain;
    p->poleCoeffs[1] = -(aReson * aReson);
    p->poleCoeffs[0] = FL(2.0) * aReson *
      COS(2 * pp->insdshead->pidsr * aFreq);
}

void FormSwep_setTargets(FormSwep *p, cs_float aFreq, cs_float aReson, cs_float aGain)
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

cs_float FormSwep_tick(OPDS *pp,
                    FormSwep *p, cs_float sample) /* Perform Filter Operation */
{
    cs_float temp;

    /* Keep the pending target when a zero rate pauses the sweep. */
    if (p->dirty && p->sweepRate != FL(0.0)) {
      p->sweepState += p->sweepRate;
      if (p->sweepState>= FL(1.0)) {
        p->sweepState   = FL(1.0);
        p->dirty        = 0;
        p->currentReson = p->reson = p->targetReson;
        p->currentFreq  = p->freq  = p->targetFreq;
        p->currentGain  = p->gain  = p->targetGain;
      }
      else {
        p->currentReson = p->reson + (p->deltaReson * p->sweepState);
        p->currentFreq  = p->freq + (p->deltaFreq * p->sweepState);
        p->currentGain  = p->gain + (p->deltaGain * p->sweepState);
      }
      p->poleCoeffs[1] = - (p->currentReson * p->currentReson);
      p->poleCoeffs[0] = FL(2.0) * p->currentReson *
      COS(2*pp->insdshead->pidsr * p->currentFreq);

    }

    temp = p->currentGain * sample;
    temp += p->poleCoeffs[0] * p->outputs[0];
    temp += p->poleCoeffs[1] * p->outputs[1];
    p->outputs[1] = p->outputs[0];
    p->outputs[0] = temp;
    return temp;
}

static inline cs_float Samp_tick(Wave *p)
{
    int32    temp, temp1;
    cs_float   temp_time, alpha;
    cs_float   lastOutput;

    p->time += p->rate;                  /*  Update current time    */
    while (p->time >= p->wave->flen)     /*  Check for end of sound */
      p->time -= p->wave->flen;          /*  loop back to beginning */
    while (p->time < FL(0.0))            /*  Check for end of sound */
      p->time += p->wave->flen;          /*  loop back to beginning */

    temp_time = p->time;

    if (p->phase != FL(0.0)) {
      temp_time += p->phase;             /*  Add phase offset       */
      while (temp_time >= p->wave->flen) /*  Check for end of sound */
        temp_time -= p->wave->flen;      /*  loop back to beginning */
      while (temp_time < FL(0.0))        /*  Check for end of sound */
        temp_time += p->wave->flen;      /*  loop back to beginning */
    }

    temp = (int32) temp_time;    /*  Integer part of time address    */
    temp1 = temp + 1;
    if (UNLIKELY(temp1==(int32_t)p->wave->flen)) temp1 = 0; /* Wrap!! */
    /*  fractional part of time address */
    alpha = temp_time - (cs_float)temp;
    lastOutput = p->wave->ftable[temp];  /* Do linear interpolation */
    /* same as alpha*data[temp+1] + (1-alpha)data[temp] */
    lastOutput += (alpha * (p->wave->ftable[temp1] - lastOutput));
    /* End of vibrato tick */
    return lastOutput;
}

int32_t Moog1set(CSOUND *csound, MOOG1 *p)
{
    FUNC        *ftp;
    cs_float       tempCoeffs[2] = {FL(0.0),-FL(1.0)};

    make_ADSR(&p->adsr, CS_ESR);
    make_OnePole(&p->filter);
    make_TwoZero(&p->twozeroes[0]);
    TwoZero_setZeroCoeffs(&p->twozeroes[0], tempCoeffs);
    make_TwoZero(&p->twozeroes[1]);
    TwoZero_setZeroCoeffs(&p->twozeroes[1], tempCoeffs);
    make_FormSwep(&p->filters[0]);
    make_FormSwep(&p->filters[1]);

    if (LIKELY((ftp = csound->FTFind(csound, p->iatt)) != NULL))
      p->attk.wave = ftp; /* mandpluk */
    else return NOTOK;
    if (LIKELY((ftp = csound->FTFind(csound, p->ifn )) != NULL))
      p->loop.wave = ftp; /* impuls20 */
    else return NOTOK;
    if (LIKELY((ftp = csound->FTFind(csound, p->ivfn)) != NULL))
      p->vibr.wave = ftp; /* sinewave */
    else return NOTOK;
    p->attk.time = p->attk.phase = FL(0.0);
    p->loop.time = p->loop.phase = FL(0.0);
    p->vibr.time = p->vibr.phase = FL(0.0);
    /* Force the first control block to apply zero values too. */
    p->oldfilterQ = p->oldfilterRate = -FL(1.0);
    ADSR_setAll(csound, &p->adsr, FL(0.05), FL(0.00003), FL(0.6), FL(0.0002));
    ADSR_keyOn(&p->adsr);
    {
      /* The release takes at most 1 / (0.0002 * 22050) seconds.
         Allow 250 ms for it and the filter tail. */
      int32_t relestim = (int32_t)ceil(FL(0.25) * CS_EKR);
      if (relestim > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = relestim;
    }
    return OK;
}

int32_t Moog1(CSOUND *csound, MOOG1 *p)
{
    cs_float       fullscale = AMP_SCALE;
    cs_float       amp = *p->amp * (FL(1.0) / fullscale);
    cs_float       *ar = p->ar;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    cs_float       temp;
    cs_float       vib = *p->vibAmt;

    if (p->h.insdshead->relesing &&
        p->adsr.state != RELEASE && p->adsr.state != CLEAR)
      ADSR_keyOff(&p->adsr);

    p->baseFreq = *p->frequency;
    p->attk.rate = p->baseFreq * FL(0.01) * p->attk.wave->flen * CS_ONEDSR;
    p->loop.rate = p->baseFreq            * p->loop.wave->flen * CS_ONEDSR;
    p->attackGain = amp * FL(0.5);
    p->loopGain = amp;
    if (*p->filterQ != p->oldfilterQ) {
      p->oldfilterQ = *p->filterQ;
      temp = p->oldfilterQ + FL(0.05);
      FormSwep_setStates(&p->h, &p->filters[0], FL(2000.0), temp,
                         FL(2.0) * (FL(1.0) - temp));
      FormSwep_setStates(&p->h, &p->filters[1], FL(2000.0), temp,
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
    p->vibr.rate = *p->vibf * p->vibr.wave->flen * CS_ONEDSR;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n = offset; n<nsmps; n++) {
      cs_float     temp;
      cs_float     output;
      int32     itemp;
      cs_float     temp_time, alpha;

      if (vib != FL(0.0)) {
        temp = vib * Samp_tick(&p->vibr);
        p->loop.rate = p->baseFreq * (FL(1.0) + temp) *
                       (cs_float)(p->loop.wave->flen) * CS_ONEDSR;
      }

      p->attk.time += p->attk.rate;           /*  Update current time    */
#ifdef DEBUG
      csound->Message(csound, "Attack_time=%f\tAttack_rate=%f\n",
                              p->attk.time, p->attk.rate);
#endif
      temp_time = p->attk.time;
      if (p->attk.time >= (cs_float)p->attk.wave->flen)
        output = FL(0.0);                                    /* One shot */
      else {
        itemp = (int32) temp_time;   /*  Integer part of time address    */
                                     /*  fractional part of time address */
        alpha = temp_time - (cs_float)itemp;
#ifdef DEBUG
        csound->Message(csound, "Attack: (%d, %d), alpha=%f\t",
                                itemp, itemp+1, alpha);
#endif
        output = p->attk.wave->ftable[itemp]; /* Do linear interpolation */
                  /*  same as alpha*data[itemp+1] + (1-alpha)data[itemp] */
#ifdef DEBUG
        csound->Message(csound, "->%f+\n", output);
#endif
        output += (alpha * (p->attk.wave->ftable[itemp+1] - output));
        output *= p->attackGain;
                                                   /* End of attack tick */
      }
#ifdef DEBUG
      csound->Message(csound, "After Attack: %f\n", output);
#endif
      output += p->loopGain * Samp_tick(&p->loop);
#ifdef DEBUG
      csound->Message(csound, "Before OnePole: %f\n", output);
#endif
      output = OnePole_tick(&p->filter, output);
#ifdef DEBUG
      csound->Message(csound, "After OnePole: %f\n", output);
#endif
      output *= ADSR_tick(&p->adsr);
#ifdef DEBUG
      csound->Message(csound, "Sampler_tick: %f\n", output);
#endif
      output = TwoZero_tick(&p->twozeroes[0], output);
#ifdef DEBUG
      csound->Message(csound, "TwoZero0_tick: %f\n", output);
#endif
      output = FormSwep_tick((OPDS *)p, &p->filters[0], output);
#ifdef DEBUG
      csound->Message(csound, "Filters0_tick: %f\n", output);
#endif
      output = TwoZero_tick(&p->twozeroes[1], output);
#ifdef DEBUG
      csound->Message(csound, "TwoZero1_tick: %f\n", output);
#endif
      output = FormSwep_tick((OPDS *)p, &p->filters[1], output);
#ifdef DEBUG
      csound->Message(csound, "Filter2_tick: %f\n", output);
#endif
      ar[n] = output*fullscale*FL(8.0);
    }
    return OK;
}
