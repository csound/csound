/*  
    physmod.c:

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

/* Collection of physical modelled instruments */

#include "csdl.h"
#include "clarinet.h"
#include "flute.h"
#include "bowed.h"
#include "brass.h"
#include <math.h>

/* ************************************** */
/*  Waveguide Clarinet model ala Smith    */
/*  after McIntyre, Schumacher, Woodhouse */
/*  by Perry Cook, 1995-96                */
/*  Recoded for Csound by John ffitch     */
/*  November 1997                         */
/*                                        */
/*  This is a waveguide model, and thus   */
/*  relates to various Stanford Univ.     */
/*  and possibly Yamaha and other patents.*/
/*                                        */
/* ************************************** */

/**********************************************/
/*  One break point linear reed table object  */
/*  by Perry R. Cook, 1995-96                 */
/*  Consult McIntyre, Schumacher, & Woodhouse */
/*        Smith, Hirschman, Cook, Scavone,    */
/*        more for information.               */
/**********************************************/

static MYFLT ReedTabl_LookUp(ReedTabl *r, MYFLT deltaP)
    /*   Perform "Table Lookup" by direct clipped  */
    /*   linear function calculation               */
{   /*   deltaP is differential reed pressure      */
    MYFLT lastOutput = r->offSet + (r->slope * deltaP); /* basic non-lin */
    if (lastOutput > FL(1.0))
      lastOutput = FL(1.0);      /* if other way, reed slams shut */
    if (lastOutput < -FL(1.0))
      lastOutput = -FL(1.0);     /* if all the way open, acts like open end */
    return lastOutput;
}


/*******************************************/
/*  One Zero Filter Class,                 */
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
/*  gain / (1.0 - fabs(alpha)).            */
/*******************************************/

void make_OneZero(OneZero* z)
{
    z->gain = FL(1.0);
    z->zeroCoeff = FL(1.0);
    z->sgain = FL(0.5);
    z->inputs = FL(0.0);
}

MYFLT OneZero_tick(OneZero* z, MYFLT sample) /*   Perform Filter Operation  */
{
    MYFLT temp, lastOutput;
    temp = z->sgain * sample;
    lastOutput = (z->inputs * z->zeroCoeff) + temp;
    z->inputs = temp;
    return lastOutput;
}

void OneZero_setCoeff(OneZero* z, MYFLT aValue)
{
    z->zeroCoeff = aValue;
    if (z->zeroCoeff > FL(0.0))                  /*  Normalize gain to 1.0 max  */
      z->sgain = z->gain / (FL(1.0) + z->zeroCoeff);
    else
      z->sgain = z->gain / (FL(1.0) - z->zeroCoeff);
}

void OneZero_print(OneZero *p)
{
    printf("OneZero: gain=%f inputs=%f zeroCoeff=%f sgain=%f\n",
           p->gain, p->inputs, p->zeroCoeff, p->sgain);
}

/* *********************************************************************** */
int clarinset(CLARIN *p)
{
    FUNC        *ftp;

    if ((ftp = ftfind(p->ifn)) != NULL) p->vibr = ftp;
    else {
      return perferror(Str(X_376,"No table for Clarinet")); /* Expect sine wave */
    }
    if (*p->lowestFreq>=FL(0.0)) {      /* Skip initialisation */
      if (*p->lowestFreq)
        p->length = (long) (esr_ / *p->lowestFreq + FL(1.0));
      else if (*p->frequency)
        p->length = (long) (esr_ / *p->frequency + FL(1.0));
      else {
        err_printf(Str(X_362,"No base frequency for clarinet -- assuming 50Hz\n"));
        p->length = (long) (esr_ / FL(50.0) + FL(1.0));
      }
      make_DLineL(&p->delayLine, p->length);
      p->reedTable.offSet = FL(0.7);
      p->reedTable.slope = -FL(0.3);
      make_OneZero(&(p->filter));
      make_Envelope(&p->envelope);
      make_Noise(p->noise);
    /*    p->noiseGain = 0.2f; */       /* Arguemnts; suggested values? */
    /*    p->vibrGain = 0.1f; */
      {
        int relestim = (int)(ekr_ * FL(0.1)); /* 1/10th second decay extention */
        if (relestim > p->h.insdshead->xtratim)
          p->h.insdshead->xtratim = relestim;
      }
      p->kloop = (int)(p->h.insdshead->offtim * ekr_) - (int)(ekr_* *p->attack);
      printf("offtim=%f  kloop=%d\n",
             p->h.insdshead->offtim, p->kloop);
      p->envelope.rate = FL(0.0);
      p->v_time = 0;
    }
    return OK;
}

int clarin(CLARIN *p)
{
    MYFLT *ar = p->ar;
    long nsmps = ksmps_;
    MYFLT amp = (*p->amp)*AMP_RSCALE; /* Normalise */
    MYFLT nGain = *p->noiseGain;
    int v_len = (int)p->vibr->flen;
    MYFLT *v_data = p->vibr->ftable;
    MYFLT vibGain = *p->vibAmt;
    MYFLT vTime = p->v_time;

    if (p->envelope.rate==FL(0.0)) {
      p->envelope.rate =  amp /(*p->attack*esr_);
      p->envelope.value = p->envelope.target = FL(0.55) + amp*FL(0.30);
    }
    p->outputGain = amp + FL(0.001);
    DLineL_setDelay(&p->delayLine, /* length - approx filter delay */
        (esr_/ *p->frequency) * FL(0.5) - FL(1.5));
    p->v_rate = *p->vibFreq * p->vibr->flen / esr_;
                                /* Check to see if into decay yet */
    if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
    if ((--p->kloop) == 0) {
      p->envelope.state = 1;  /* Start change */
      p->envelope.rate = p->envelope.value / (*p->dettack * esr_);
      p->envelope.target =  FL(0.0);
      printf("Set off phase time = %f Breath v,r = %f, %f\n",
             (MYFLT)kcounter/ekr_, p->envelope.value, p->envelope.rate);
    }

    do {
        MYFLT   pressureDiff;
        MYFLT   breathPressure;
        long    temp;
        MYFLT   temp_time, alpha;
        MYFLT   nextsamp;
        MYFLT   v_lastOutput;
        MYFLT   lastOutput;

        breathPressure = Envelope_tick(&p->envelope);
        breathPressure += breathPressure * nGain* Noise_tick(&p->noise);
                                /* Tick on vibrato table */
        vTime += p->v_rate;            /*  Update current time    */
        while (vTime >= v_len)         /*  Check for end of sound */
          vTime -= v_len;              /*  loop back to beginning */
        while (vTime < FL(0.0))            /*  Check for end of sound */
          vTime += v_len;              /*  loop back to beginning */

        temp_time = vTime;

#ifdef have_phase
        if (p->v_phaseOffset != FL(0.0)) {
          temp_time += p->v_phaseOffset;   /*  Add phase offset       */
          while (temp_time >= v_len)       /*  Check for end of sound */
            temp_time -= v_len;            /*  loop back to beginning */
          while (temp_time < FL(0.0))          /*  Check for end of sound */
            temp_time += v_len;            /*  loop back to beginning */
        }
#endif
        temp = (long) temp_time;    /*  Integer part of time address    */
                                /*  fractional part of time address */
        alpha = temp_time - (MYFLT)temp;
        v_lastOutput = v_data[temp]; /* Do linear interpolation */
                        /*  same as alpha*data[temp+1] + (1-alpha)data[temp] */
        v_lastOutput += (alpha * (v_data[temp+1] - v_lastOutput));
                                /* End of vibrato tick */
        breathPressure += breathPressure * vibGain * v_lastOutput;
        pressureDiff = OneZero_tick(&p->filter, /* differential pressure  */
                                   DLineL_lastOut(&p->delayLine));
        pressureDiff = (-FL(0.95)*pressureDiff) - breathPressure;  /* of reflected and mouth */
          nextsamp = pressureDiff * ReedTabl_LookUp(&p->reedTable,pressureDiff);
        nextsamp =  breathPressure + nextsamp;
        lastOutput =
          DLineL_tick(&p->delayLine, nextsamp); /* perform scattering in economical way */
        lastOutput *= p->outputGain;
        *ar++ = lastOutput*AMP_SCALE;
    } while (--nsmps);
    p->v_time = vTime;

    return OK;
}

/******************************************/
/*  WaveGuide Flute ala Karjalainen,      */
/*  Smith, Waryznyk, etc.                 */
/*  with polynomial Jet ala Cook          */
/*  by Perry Cook, 1995-96                */
/*  Recoded for Csound by John ffitch     */
/*  November 1997                         */
/*                                        */
/*  This is a waveguide model, and thus   */
/*  relates to various Stanford Univ.     */
/*  and possibly Yamaha and other patents.*/
/*                                        */
/******************************************/


/**********************************************/
/* Jet Table Object by Perry R. Cook, 1995-96 */
/* Consult Fletcher and Rossing, Karjalainen, */
/*       Cook, more, for information.         */
/* This, as with many other of my "tables",   */
/* is not a table, but is computed by poly-   */
/* nomial calculation.                        */
/**********************************************/

static MYFLT JetTabl_lookup(MYFLT sample) /* Perform "Table Lookup"  */
{                                  /* By Polynomial Calculation */
                                   /* (x^3 - x) approximates sigmoid of jet */
    MYFLT j = sample * (sample*sample - FL(1.0));
    if (j > FL(1.0)) j = FL(1.0);        /* Saturation at +/- 1.0       */
    else if (j < -FL(1.0)) j = -FL(1.0);
    return j;
}

int fluteset(FLUTE *f)
{
    FUNC        *ftp;
    long        length;

    if ((ftp = ftfind(f->ifn)) != NULL) f->vibr = ftp;
    else {
      return perferror(Str(X_378,"No table for Flute")); /* Expect sine wave */
    }
    if (*f->lowestFreq>=FL(0.0)) {      /* Skip initialisation?? */
      if (*f->lowestFreq!=FL(0.0))
        length = (long) (esr_ / *f->lowestFreq + FL(1.0));
      else if (*f->frequency!=FL(0.0))
        length = (long) (esr_ / *f->frequency + FL(1.0));
      else {
        err_printf(Str(X_363,"No base frequency for flute -- assumed to be 50Hz\n"));
        length = (long) (esr_ / FL(50.0) + FL(1.0));
      }
      make_DLineL(&f->boreDelay, length);
      length = length >> 1;        /* ??? really; yes from later version */
      make_DLineL(&f->jetDelay, length);
      make_OnePole(&f->filter);
      make_DCBlock(&f->dcBlock);
      make_Noise(f->noise);
      make_ADSR(&f->adsr);
                                /* Clear */
/*     OnePole_clear(&f->filter); */
/*     DCBlock_clear(&f->dcBlock); */
                                /* End Clear */
/*       DLineL_setDelay(&f->boreDelay, 100.0f); */
/*       DLineL_setDelay(&f->jetDelay, 49.0f); */

      OnePole_setPole(&f->filter, FL(0.7) - (FL(0.1) * RATE_NORM));
      OnePole_setGain(&f->filter, -FL(1.0));
      ADSR_setAllTimes(&f->adsr, FL(0.005), FL(0.01), FL(0.8), FL(0.010));
/*        ADSR_setAll(&f->adsr, 0.02f, 0.05f, 0.8f, 0.001f); */
    /* Suggested values */
    /*    f->endRefl = 0.5; */
    /*    f->jetRefl = 0.5; */
    /*    f->noiseGain = 0.15; */ /* Breath pressure random component   */
    /*    f->vibrGain = 0.05;  */ /* breath periodic vibrato component  */
    /*    f->jetRatio = 0.32;  */
      f->lastamp = FL(1.0);             /* Remember */
      ADSR_setAttackRate(&f->adsr, FL(0.02));/* This should be controlled by attack */
      f->maxPress = FL(2.3) / FL(0.8);
      f->outputGain = FL(1.001);
      ADSR_keyOn(&f->adsr);
      f->kloop = (MYFLT)((int)(f->h.insdshead->offtim*ekr_ - ekr_*(*f->dettack)));

      f->lastFreq = FL(0.0);
      f->lastJet = -FL(1.0);
      /* freq = (2/3)*f->frequency as we're overblowing here */
      /* but 1/(2/3) is 1.5 so multiply for speed */
    }
/*     printf("offtim=%f  kloop=%d\n", f->h.insdshead->offtim, f->kloop); */
/*     printf("Flute : NoteOn: Freq=%f\n",*f->frequency); */
/*  printf("%f %f %f %f \n%f %f %f %f \n",  */
/*        f->v_rate, f->v_time, f->lastFreq, f->lastJet, */
/*        f->maxPress, f->vibrGain, f->kloop, f->lastamp); */
    return OK;
}

int flute(FLUTE *f)
{
    MYFLT       *ar = f->ar;
    long        nsmps = ksmps_;
    MYFLT       amp = (*f->amp)*AMP_RSCALE; /* Normalise */
    MYFLT       temp;
    int         v_len = (int)f->vibr->flen;
    MYFLT       *v_data = f->vibr->ftable;
    MYFLT       v_time = f->v_time;
    MYFLT       vibGain = *f->vibAmt;
    MYFLT       jetRefl, endRefl, noisegain;
    if (amp!=f->lastamp) {      /* If amplitude has changed */
      /*       printf("Amp changed to %f\n", amp); */
      ADSR_setAttackRate(&f->adsr, amp * FL(0.02));/* This should be controlled by attack */
      f->maxPress = (FL(1.1) + (amp * FL(0.20))) / FL(0.8);
      f->outputGain = amp + FL(0.001);
      f->lastamp = amp;
    }
    f->v_rate = *f->vibFreq * v_len * onedsr;
                                /* Start SetFreq */
    if (f->lastFreq != *f->frequency) { /* It changed */
      f->lastFreq = *f->frequency;
      f->lastJet = *f->jetRatio;
      /* freq = (2/3)*f->frequency as we're overblowing here */
      /* but 1/(2/3) is 1.5 so multiply for speed */
      temp = FL(1.5)* esr_ / f->lastFreq - FL(2.0);/* Length - approx. filter delay */
      DLineL_setDelay(&f->boreDelay, temp); /* Length of bore tube */
      DLineL_setDelay(&f->jetDelay, temp * f->lastJet); /* jet delay shorter */
    }
    else if (*f->jetRatio != f->lastJet) { /* Freq same but jet changed */
      /*      printf("Jet changed to %f\n", *f->jetRatio); */
      f->lastJet = *f->jetRatio;
      temp = FL(1.5)* esr_ / f->lastFreq - FL(2.0);      /* Length - approx. filter delay */
      DLineL_setDelay(&f->jetDelay, temp * f->lastJet); /* jet delay shorter */
    }
                                /* End SetFreq */

    if (f->kloop>FL(0.0) && f->h.insdshead->relesing) f->kloop=FL(1.0);
    if ((--f->kloop) == 0) {
      f->adsr.releaseRate = f->adsr.value / (*f->dettack * esr_);
      f->adsr.target = FL(0.0);
      f->adsr.rate = f->adsr.releaseRate;
      f->adsr.state = RELEASE;
/*       printf("Set off phase time = %f\n", (MYFLT)kcounter/ekr_); */
    }
/*     printf("Flute : NoteOn: Freq=%f Amp=%f\n",*f->frequency,amp); */
/*     printf("%f %f %f %f \n%f %f %f %f \n",  */
/*        f->v_rate, f->v_time, f->lastFreq, f->lastJet,  */
/*        f->maxPress, f->vibrGain, f->kloop, f->lastamp); */
    noisegain = *f->noiseGain; jetRefl = *f->jetRefl; endRefl = *f->endRefl;
    do {
      long      temp;
      MYFLT     temf;
      MYFLT     temp_time, alpha;
      MYFLT     pressDiff;
      MYFLT     randPress;
      MYFLT     breathPress;
      MYFLT     lastOutput;
      MYFLT     v_lastOutput;

      breathPress = f->maxPress * ADSR_tick(&f->adsr); /* Breath Pressure */
      randPress = noisegain * Noise_tick(&f->noise);   /* Random Deviation */
                                /* Tick on vibrato table */
      v_time += f->v_rate;            /*  Update current time    */
      while (v_time >= v_len)         /*  Check for end of sound */
        v_time -= v_len;              /*  loop back to beginning */
      while (v_time < FL(0.0))           /*  Check for end of sound */
        v_time += v_len;              /*  loop back to beginning */

      temp_time = v_time;

#ifdef phase_offset
      if (f->v_phaseOffset != FL(0.0)) {
        temp_time += f->v_phaseOffset;   /*  Add phase offset       */
        while (temp_time >= v_len)    /*  Check for end of sound */
          temp_time -= v_len;         /*  loop back to beginning */
        while (temp_time < FL(0.0))      /*  Check for end of sound */
          temp_time += v_len;         /*  loop back to beginning */
      }
#endif

      temp = (long) temp_time;        /*  Integer part of time address    */
                                      /*  fractional part of time address */
      alpha = temp_time - (MYFLT)temp;
      v_lastOutput = v_data[temp];    /* Do linear interpolation */
      /*  same as alpha*data[temp+1] + (1-alpha)data[temp] */
      v_lastOutput += (alpha * (v_data[temp+1] - v_lastOutput));
/*       printf("Vibrato %f\n", v_lastOutput); */
                                      /* End of vibrato tick */
      randPress += vibGain * v_lastOutput; /* + breath vibrato       */
      randPress *= breathPress;            /* All scaled by Breath Pressure */
      temf = OnePole_tick(&f->filter, DLineL_lastOut(&f->boreDelay));
      temf = DCBlock_tick(&f->dcBlock, temf);        /* Block DC on reflection */
      pressDiff = breathPress + randPress    /* Breath Pressure   */
                     - (jetRefl * temf); /*  - reflected      */
      pressDiff = DLineL_tick(&f->jetDelay, pressDiff);  /* Jet Delay Line */
      pressDiff = JetTabl_lookup(pressDiff)  /* Non-Lin Jet + reflected */
                     + (endRefl * temf);
      lastOutput = FL(0.3) * DLineL_tick(&f->boreDelay, pressDiff);  /* Bore Delay and "bell" filter  */

      lastOutput *= f->outputGain;
/*       printf("sample=%f\n", lastOutput); */
      *ar++ = lastOutput*AMP_SCALE*FL(1.4);
    } while (--nsmps);

    f->v_time = v_time;
    return OK;
}

/******************************************/
/*  Bowed String model ala Smith          */
/*  after McIntyre, Schumacher, Woodhouse */
/*  by Perry Cook, 1995-96                */
/*  Recoded for Csound by John ffitch     */
/*  November 1997                         */
/*                                        */
/*  This is a waveguide model, and thus   */
/*  relates to various Stanford Univ.     */
/*  and possibly Yamaha and other patents.*/
/*                                        */
/******************************************/


/******************************************/
/*  Simple Bow Table Object, after Smith  */
/*    by Perry R. Cook, 1995-96           */
/******************************************/

MYFLT BowTabl_lookup(BowTabl *b, MYFLT sample) /*  Perform Table Lookup    */
{                                              /*  sample is differential  */
    MYFLT lastOutput;                          /*  string vs. bow velocity */
    MYFLT input;
    input = sample /* + b->offSet*/ ;          /*  add bias to sample      */
    input *= b->slope;                         /*  scale it                */
    lastOutput = (MYFLT)fabs(input) + FL(0.75); /*  below min delta, frict = 1 */
    lastOutput = intpow(lastOutput,-4L);
/* if (lastOutput < FL(0.0) ) lastOutput = FL(0.0); */ /* minimum frict is 0.0 */
    if (lastOutput > FL(1.0)) lastOutput = FL(1.0); /*  maximum friction is 1.0 */
    return lastOutput;
}

int bowedset(BOWED *b)
{
    long        length;
    FUNC        *ftp;
    MYFLT       amp = (*b->amp)*AMP_RSCALE; /* Normalise */

    if ((ftp = ftfind(b->ifn)) != NULL) b->vibr = ftp;
    else {
      return perferror(Str(X_386,"No table for wgbow vibrato")); /* Expect sine wave */
    }
    if (*b->lowestFreq>=FL(0.0)) {      /* If no init skip */
/*        printf("Initialising lowest = %f\n", *b->lowestFreq); */
      if (*b->lowestFreq!=FL(0.0))
        length = (long) (esr_ / *b->lowestFreq + FL(1.0));
      else if (*b->frequency!=FL(0.0))
        length = (long) (esr_ / *b->frequency + FL(1.0));
      else {
        err_printf(Str(X_512,"unknown lowest frequency for bowed string -- assuming 50Hz\n"));
        length = (long) (esr_ / FL(50.0) + FL(1.0));
      }
      make_DLineL(&b->neckDelay, length);
      length = length >> 1; /* ?Unsure about this; seems correct in later code */
      make_DLineL(&b->bridgeDelay, length);

      /*  b->bowTabl.offSet = FL(0.0);*/
      /* offset is a bias, really not needed unless */
      /* friction is different in each direction    */

      /* b->bowTabl.slope contrls width of friction pulse, related to bowForce */
      b->bowTabl.slope = FL(3.0);
      make_OnePole(&b->reflFilt);
      make_BiQuad(&b->bodyFilt);
      make_ADSR(&b->adsr);

      DLineL_setDelay(&b->neckDelay, FL(100.0));
      DLineL_setDelay(&b->bridgeDelay, FL(29.0));

      OnePole_setPole(&b->reflFilt, FL(0.6) - (FL(0.1) * RATE_NORM));
      OnePole_setGain(&b->reflFilt, FL(0.95));

      BiQuad_setFreqAndReson(b->bodyFilt, FL(500.0), FL(0.85));
      BiQuad_setEqualGainZeroes(b->bodyFilt);
      BiQuad_setGain(b->bodyFilt, FL(0.2));

      ADSR_setAllTimes(&b->adsr, FL(0.02), FL(0.005), FL(0.9), FL(0.01));
/*        ADSR_setAll(&b->adsr, 0.002f,0.01f,0.9f,0.01f); */

      b->adsr.target = FL(1.0);
      b->adsr.rate = b->adsr.attackRate;
      b->adsr.state = ATTACK;
      b->maxVelocity = FL(0.03) + (FL(0.2) * amp);

      b->lastpress = FL(0.0);   /* Set unknown state */
      b->lastfreq = FL(0.0);
      b->lastbeta = FL(0.0);    /* Remember states */
      b->lastamp = amp;
/*        printf("Bowed amp = %f (%f)\n", amp, *b->amp); */
    }
    return OK;
}


int bowed(BOWED *b)
{
    MYFLT       *ar = b->ar;
    long        nsmps = ksmps_;
    MYFLT       amp = (*b->amp)*AMP_RSCALE; /* Normalise */
    MYFLT       maxVel;
    int         freq_changed = 0;

    if (amp != b->lastamp) {
      b->maxVelocity = FL(0.03) + (FL(0.2) * amp);
      b->lastamp = amp;
/*        printf("Bowed amp = %f\n", amp); */
    }
    maxVel = b->maxVelocity;
    if (b->lastpress != *b->bowPress)
      b->bowTabl.slope = b->lastpress = *b->bowPress;

                                /* Set Frequency if changed */
    if (b->lastfreq != *b->frequency) {
      /* delay - approx. filter delay */
/*        printf("Freq chganged %f %f\n", *b->frequency, b->lastfreq); */
      b->lastfreq = *b->frequency;
      b->baseDelay = esr_ / b->lastfreq - FL(4.0);
/*        printf("Freq set to %f with basedelay %f\n", b->lastfreq, b->baseDelay); */
      freq_changed = 1;
    }
    if (b->lastbeta != *b->betaRatio ||
        freq_changed) {         /* Reset delays if changed */
/*        printf("setDelay %f !=", b->lastbeta); */
      b->lastbeta = *b->betaRatio;
      DLineL_setDelay(&b->bridgeDelay, /* bow to bridge length */
                      b->baseDelay * b->lastbeta);
      DLineL_setDelay(&b->neckDelay, /* bow to nut (finger) length */
                      b->baseDelay *(FL(1.0) - b->lastbeta));
/*        printf(" %f || %d\n", b->lastbeta, freq_changed); */
    }
    b->v_rate = *b->vibFreq * b->vibr->flen * onedsr;
    if (b->kloop>0 && b->h.insdshead->relesing) b->kloop=1;
    if ((--b->kloop) == 0) {
      ADSR_setDecayRate(&b->adsr, (FL(1.0) - b->adsr.value) * FL(0.005));
      b->adsr.target = FL(0.0);
      b->adsr.rate = b->adsr.releaseRate;
      b->adsr.state = RELEASE;
    }

    do {
      MYFLT     bowVelocity;
      MYFLT     bridgeRefl=FL(0.0), nutRefl=FL(0.0);
      MYFLT     newVel=FL(0.0), velDiff=FL(0.0), stringVel=FL(0.0);
      MYFLT     lastOutput;

      bowVelocity = maxVel * ADSR_tick(&b->adsr);
/* printf("bowVelocity=%f\n", bowVelocity); */

      bridgeRefl = - OnePole_tick(&b->reflFilt, b->bridgeDelay.lastOutput);  /* Bridge Reflection      */
/* printf("bridgeRefl=%f\n", bridgeRefl); */
      nutRefl = - b->neckDelay.lastOutput; /* Nut Reflection  */
/* printf("nutRefl=%f\n", nutRefl); */
      stringVel = bridgeRefl + nutRefl; /* Sum is String Velocity */
/* printf("stringVel=%f\n",stringVel ); */
      velDiff = bowVelocity - stringVel; /* Differential Velocity  */
/* printf("velDiff=%f\n",velDiff ); */
      newVel = velDiff * BowTabl_lookup(&b->bowTabl, velDiff);  /* Non-Lin Bow Function   */
/* printf("newVel=%f\n",newVel ); */
      DLineL_tick(&b->neckDelay, bridgeRefl + newVel);  /* Do string       */
      DLineL_tick(&b->bridgeDelay, nutRefl + newVel);   /*   propagations  */

      if (*b->vibAmt > FL(0.0)) {
        long    temp;
        MYFLT   temp_time, alpha;
                                /* Tick on vibrato table */
        b->v_time += b->v_rate;              /*  Update current time    */
        while (b->v_time >= b->vibr->flen)   /*  Check for end of sound */
          b->v_time -= b->vibr->flen;        /*  loop back to beginning */
        while (b->v_time < FL(0.0))          /*  Check for end of sound */
          b->v_time += b->vibr->flen;        /*  loop back to beginning */

        temp_time = b->v_time;

#ifdef phase_offset
        if (b->v_phaseOffset != FL(0.0)) {
          temp_time += b->v_phaseOffset;     /*  Add phase offset       */
          while (temp_time >= b->vibr->flen) /*  Check for end of sound */
            temp_time -= b->vibr->flen;      /*  loop back to beginning */
          while (temp_time < FL(0.0))        /*  Check for end of sound */
            temp_time += b->vibr->flen;      /*  loop back to beginning */
        }
#endif
        temp = (long) temp_time;    /*  Integer part of time address    */
                                /*  fractional part of time address */
        alpha = temp_time - (MYFLT)temp;
        b->v_lastOutput = b->vibr->ftable[temp]; /* Do linear interpolation */
                        /*  same as alpha*data[temp+1] + (1-alpha)data[temp] */
        b->v_lastOutput = b->v_lastOutput +
          (alpha * (b->vibr->ftable[temp+1] - b->v_lastOutput));
                                /* End of vibrato tick */

       DLineL_setDelay(&b->neckDelay,
                       (b->baseDelay * (FL(1.0) - b->lastbeta)) +
                       (b->baseDelay * *b->vibAmt * b->v_lastOutput));
      }
      else
       DLineL_setDelay(&b->neckDelay,
                       (b->baseDelay * (FL(1.0) - b->lastbeta)));

      lastOutput = BiQuad_tick(&b->bodyFilt, b->bridgeDelay.lastOutput);
/* printf("lastOutput=%f\n",lastOutput ); */

      *ar++ = lastOutput*AMP_SCALE * amp *FL(1.8);
    } while (--nsmps);
    return OK;
}

/******************************************/
/*  Waveguide Brass Instrument Model ala  */
/*  Cook (TBone, HosePlayer)              */
/*  by Perry R. Cook, 1995-96             */
/*  Recoded for Csound by John ffitch     */
/*  November 1997                         */
/*                                        */
/*  This is a waveguide model, and thus   */
/*  relates to various Stanford Univ.     */
/*  and possibly Yamaha and other patents.*/
/*                                        */
/******************************************/


/****************************************************************************/
/*                                                                          */
/*  AllPass Interpolating Delay Line Object by Perry R. Cook 1995-96        */
/*  This one uses a delay line of maximum length specified on creation,     */
/*  and interpolates fractional length using an all-pass filter.  This      */
/*  version is more efficient for computing static length delay lines       */
/*  (alpha and coeff are computed only when the length is set, there        */
/*  probably is a more efficient computational form if alpha is changed     */
/*  often (each sample)).                                                   */
/****************************************************************************/

void make_DLineA(DLineA *p, long max_length)
{
    long i;
    p->length = max_length;
    auxalloc(max_length * sizeof(MYFLT), &p->inputs);
    for (i=0;i<max_length;i++) ((MYFLT*)p->inputs.auxp)[i] = FL(0.0);
    p->lastIn = FL(0.0);
    p->lastOutput = FL(0.0);
    p->inPoint = 0;
    p->outPoint = max_length >> 1;
}

/* void DLineA_clear(DLineA *p) */
/* { */
/*     long i; */
/*     for (i=0; i<p->length; i++) ((MYFLT*)p->inputs.auxp)[i] = FL(0.0); */
/*     p->lastIn = FL(0.0); */
/*     p->lastOutput = FL(0.0); */
/* } */

int DLineA_setDelay(DLineA *p, MYFLT lag)
{
    MYFLT outputPointer;
    outputPointer = (MYFLT)p->inPoint - lag + FL(2.0);  /* outPoint chases inpoint        */
                                                    /*   + 2 for interp and other     */
    if (p->length<=0) {
      return perferror(Str(X_247,"DlineA not initialised"));
    }
    while (outputPointer<0)
        outputPointer += p->length;        /* modulo table length            */
    p->outPoint = (long) outputPointer;    /* Integer part of delay          */
    p->alpha = FL(1.0) + p->outPoint - outputPointer;/* fractional part of delay */
    if (p->alpha<FL(0.1)) {
        outputPointer += FL(1.0);             /*  Hack to avoid pole/zero       */
        p->outPoint++;                     /*  cancellation.  Keeps allpass  */
        p->alpha += FL(1.0);                  /*  delay in range of .1 to 1.1   */
    }
    p->coeff = (FL(1.0) - p->alpha)/(FL(1.0) + p->alpha); /* coefficient for all pass*/
    return 0;
}

MYFLT DLineA_tick(DLineA *p, MYFLT sample)   /*   Take sample, yield sample */
{
    MYFLT temp;
    ((MYFLT*)p->inputs.auxp)[p->inPoint++] = sample; /* Write input sample  */
    if (p->inPoint >= p->length)                 /* Increment input pointer */
        p->inPoint -= p->length;                 /* modulo length           */
    temp = ((MYFLT*)p->inputs.auxp)[p->outPoint++]; /* filter input         */
    if (p->outPoint >= p->length)                /* Increment output pointer*/
        p->outPoint -= p->length;                /* modulo length           */
    p->lastOutput = -p->coeff * p->lastOutput;   /* delayed output          */
    p->lastOutput += p->lastIn + (p->coeff * temp); /* input + delayed Input*/
    p->lastIn = temp;
    return p->lastOutput;                        /* save output and return  */
}

/* ====================================================================== */

/****************************************************************************/
/*  Lip Filter Object by Perry R. Cook, 1995-96                             */
/*  The lip of the brass player has dynamics which are controlled by the    */
/*  mass, spring constant, and damping of the lip.  This filter simulates   */
/*  that behavior and the transmission/reflection properties as well.       */
/*  See Cook TBone and HosePlayer instruments and articles.                 */
/****************************************************************************/

#define make_LipFilt(p) make_BiQuad(p)

void LipFilt_setFreq(LipFilt *p, MYFLT frequency)
{
    MYFLT coeffs[2];
    coeffs[0] = FL(2.0) * FL(0.997) *
      (MYFLT)cos(tpidsr * (double)frequency);        /* damping should  */
    coeffs[1] = -FL(0.997) * FL(0.997);              /* change with lip */
    BiQuad_setPoleCoeffs(p, coeffs);                 /* parameters, but */
    BiQuad_setGain(*p, FL(0.03));                    /* not yet.        */
}

/*  NOTE:  Here we should add lip tension                 */
/*              settings based on Mass/Spring/Damping     */
/*              Maybe in TookKit97                        */

MYFLT LipFilt_tick(LipFilt *p, MYFLT mouthSample, MYFLT boreSample)
                /*   Perform "Table Lookup" By Polynomial Calculation */
{
    MYFLT temp;
    MYFLT output;
    temp = mouthSample - boreSample;     /* Differential pressure        */
    temp = BiQuad_tick(p, temp);         /* Force -> position            */
    temp = temp*temp;                    /* Simple position to area mapping */
    if (temp > FL(1.0)) temp = FL(1.0);  /* Saturation at + 1.0          */
    output = temp * mouthSample;         /* Assume mouth input = area    */
    output += (FL(1.0) - temp) * boreSample; /* and Bore reflection is compliment */
    return output;
}

/* ====================================================================== */

int brassset(BRASS *p)
{
    FUNC        *ftp;
    MYFLT amp = (*p->amp)*AMP_RSCALE; /* Normalise */

    if ((ftp = ftfind(p->ifn)) != NULL) p->vibr = ftp;
    else {
      return perferror(Str(X_375,"No table for Brass")); /* Expect sine wave */
    }
    p->frq = *p->frequency;     /* Remember */
    if (*p->lowestFreq>=FL(0.0)) {
      if (*p->lowestFreq!=FL(0.0))
        p->length = (long) (esr_ / *p->lowestFreq + FL(1.0));
      else if (p->frq!=FL(0.0))
        p->length = (long) (esr_ / p->frq + FL(1.0));
      else {
        err_printf(Str(X_361,
                       "No base frequency for brass -- assumed to be 50Hz\n"));
        p->length = (long) (esr_ / FL(50.0) + FL(1.0));
      }
      make_DLineA(&p->delayLine, p->length);
      make_LipFilt(&p->lipFilter);
      make_DCBlock(&p->dcBlock);
      make_ADSR(&p->adsr);
      ADSR_setAllTimes(&p->adsr, FL(0.005), FL(0.001), FL(1.0), FL(0.010));
/*        ADSR_setAll(&p->adsr, 0.02f, 0.05f, FL(1.0), 0.001f); */

      ADSR_setAttackRate(&p->adsr, amp * FL(0.001));

      p->maxPressure = amp;
      ADSR_keyOn(&p->adsr);

      /* Set frequency */
      /*      p->slideTarget = (esr_ / p->frq * FL(2.0)) + 3.0f; */
      /* fudge correction for filter delays */
      /*      DLineA_setDelay(&p->delayLine, p->slideTarget);*/ /* we'll play a harmonic  */
      p->lipTarget = FL(0.0);
/*        LipFilt_setFreq(&p->lipFilter, p->frq); */
      /* End of set frequency */
      p->frq = FL(0.0);         /* to say we do not know */
      p->lipT = FL(0.0);
      /*     LipFilt_setFreq(&p->lipFilter, */
      /*                     p->lipTarget * (MYFLT)pow(4.0,(2.0* p->lipT) -1.0)); */
      {
        int relestim = (int)(ekr_ * FL(0.1)); /* 1/10th second decay extention */
        if (relestim > p->h.insdshead->xtratim)
          p->h.insdshead->xtratim = relestim;
      }
      p->kloop = (int)(p->h.insdshead->offtim * ekr_) - (int)(ekr_* *p->dettack);
/*        printf("offtim=%f  kloop=%d\n",  */
/*               p->h.insdshead->offtim, p->kloop);  */
    }
    return OK;
}

int brass(BRASS *p)
{
    MYFLT *ar = p->ar;
    long nsmps = ksmps_;
    MYFLT amp = (*p->amp)*AMP_RSCALE; /* Normalise */
    MYFLT maxPressure = p->maxPressure = amp;
    int v_len = (int)p->vibr->flen;
    MYFLT *v_data = p->vibr->ftable;
    MYFLT vibGain = *p->vibAmt;
    MYFLT vTime = p->v_time;

    p->v_rate = *p->vibFreq * v_len * onedsr;
    /*   vibr->setFreq(6.137); */
    /* vibrGain = 0.05; */            /* breath periodic vibrato component  */
    if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
    if ((--p->kloop) == 0) {
      ADSR_setReleaseRate(&p->adsr, amp * FL(0.005));
      ADSR_keyOff(&p->adsr);
    }
    if (p->frq != *p->frequency) {             /* Set frequency if changed */
      p->frq = *p->frequency;
      p->slideTarget = (esr_ / p->frq * FL(2.0)) + FL(3.0);
                        /* fudge correction for filter delays */
       /*  we'll play a harmonic */
      if (DLineA_setDelay(&p->delayLine, p->slideTarget)) return OK;
      p->lipTarget = p->frq;
      p->lipT = FL(0.0);                /* So other part is set */
    } /* End of set frequency */
    if (*p->liptension != p->lipT) {
      p->lipT = *p->liptension;
      LipFilt_setFreq(&p->lipFilter,
                      p->lipTarget * (MYFLT)pow(4.0,(2.0* p->lipT) -1.0));
    }

    do {
      MYFLT     breathPressure;
      MYFLT     lastOutput;
      int       temp;
      MYFLT     temp_time, alpha;
      MYFLT     v_lastOutput;
      MYFLT     ans;

      breathPressure = maxPressure * ADSR_tick(&p->adsr);
                                /* Tick on vibrato table */
      vTime += p->v_rate;            /*  Update current time    */
      while (vTime >= v_len)         /*  Check for end of sound */
        vTime -= v_len;              /*  loop back to beginning */
      while (vTime < FL(0.0))        /*  Check for end of sound */
        vTime += v_len;              /*  loop back to beginning */

      temp_time = vTime;

#ifdef phase_offset
      if (p->v_phaseOffset != FL(0.0)) {
        temp_time += p->v_phaseOffset;   /*  Add phase offset       */
        while (temp_time >= v_len)       /*  Check for end of sound */
          temp_time -= v_len;            /*  loop back to beginning */
        while (temp_time < FL(0.0))      /*  Check for end of sound */
          temp_time += v_len;            /*  loop back to beginning */
      }
#endif

      temp = (int) temp_time;            /*  Integer part of time address    */
                                         /*  fractional part of time address */
      alpha = temp_time - (MYFLT)temp;
      v_lastOutput = v_data[temp];  /* Do linear interpolation, same as */
      v_lastOutput +=               /*alpha*data[temp+1]+(1-alpha)data[temp] */
        (alpha * (v_data[temp+1] - v_lastOutput));
                                /* End of vibrato tick */
      breathPressure += vibGain * v_lastOutput;
      lastOutput =
        DLineA_tick(&p->delayLine,        /* bore delay  */
             DCBlock_tick(&p->dcBlock,    /* block DC    */
                  LipFilt_tick(&p->lipFilter,
                               FL(0.3) * breathPressure, /* mouth input */
                               FL(0.85) * p->delayLine.lastOutput))); /* and bore reflection */
      ans = lastOutput*AMP_SCALE*FL(3.5);
      *ar++ = ans;
    } while (--nsmps);

    p->v_time = vTime;
    return OK;
}

#define S       sizeof
#include "mandolin.h"
#include "singwave.h"
#include "shaker.h"
#include "fm4op.h"

int tubebellset(void*);
int tubebell(void*);
int rhodeset(void*);
int wurleyset(void*);
int wurley(void*);
int heavymetset(void*);
int heavymet(void*);
int b3set(void*);
int hammondB3(void*);
int FMVoiceset(void*);
int FMVoice(void*);
int percfluteset(void*);
int percflute(void*);
int Moog1set(void*);
int Moog1(void*);
int mandolinset(void*);
int mandolin(void*);
int voicformset(void*);
int voicform(void*);
int shakerset(void*);
int shaker(void*);
static OENTRY localops[] = {
{ "wgclar",  S(CLARIN),5, "a", "kkkiikkkio",(SUBR)clarinset,NULL,   (SUBR)clarin},
{ "wgflute", S(FLUTE), 5, "a", "kkkiikkkiovv",(SUBR)fluteset,NULL,  (SUBR)flute },
{ "wgbow",   S(BOWED), 5, "a", "kkkkkkio", (SUBR)bowedset, NULL,    (SUBR)bowed },
{ "wgbrass", S(BRASS), 5, "a", "kkkikkio", (SUBR)brassset, NULL,     (SUBR)brass},
{ "mandol", S(MANDOL), 5, "a", "kkkkkkio", (SUBR)mandolinset, NULL,(SUBR)mandolin},
{ "voice", S(VOICF),   5, "a", "kkkkkkii", (SUBR)voicformset, NULL,(SUBR)voicform},
{ "fmbell",  S(FM4OP), 5, "a", "kkkkkkiiiii",(SUBR)tubebellset,NULL,(SUBR)tubebell},
{ "fmrhode", S(FM4OP), 5, "a", "kkkkkkiiiii",(SUBR)rhodeset,NULL,  (SUBR)tubebell},
{ "fmwurlie", S(FM4OP),5, "a", "kkkkkkiiiii",(SUBR)wurleyset, NULL,(SUBR) wurley},
{ "fmmetal", S(FM4OP), 5, "a", "kkkkkkiiiii",(SUBR)heavymetset, NULL, (SUBR)heavymet},
{ "fmb3", S(FM4OP),    5, "a", "kkkkkkiiiii", (SUBR)b3set, NULL, (SUBR)hammondB3},
{ "fmvoice", S(FM4OPV),5, "a", "kkkkkkiiiii",(SUBR)FMVoiceset, NULL, (SUBR)FMVoice},
{ "fmpercfl", S(FM4OP),5, "a", "kkkkkkiiiii",(SUBR)percfluteset, NULL, (SUBR)percflute},
{ "moog", S(MOOG1),    5, "a", "kkkkkkiii", (SUBR)Moog1set, NULL, (SUBR)Moog1  },
{ "shaker", S(SHAKER), 5, "a", "kkkkko",  (SUBR)shakerset, NULL,   (SUBR)shaker},
};

LINKAGE
