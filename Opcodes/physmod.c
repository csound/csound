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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

/* Collection of physical modelled instruments */

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "clarinet.h"
#include "flute.h"
#include "bowed.h"
#include "brass.h"
#include <math.h>
#include "interlocks.h"


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

static inline MYFLT ReedTabl_LookUp(ReedTabl *r, MYFLT deltaP)
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
  if (z->zeroCoeff > FL(0.0))               /*  Normalize gain to 1.0 max  */
    z->sgain = z->gain / (FL(1.0) + z->zeroCoeff);
  else
    z->sgain = z->gain / (FL(1.0) - z->zeroCoeff);
}

/* void OneZero_print(CSOUND *csound, OneZero *p) */
/* { */
/*     csound->Message(csound, */
/*                     "OneZero: gain=%f inputs=%f zeroCoeff=%f sgain=%f\n", */
/*                     p->gain, p->inputs, p->zeroCoeff, p->sgain); */
/* } */

/* *********************************************************************** */
int32_t clarinset(CSOUND *csound, CLARIN *p)
{
  FUNC        *ftp;

  if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) p->vibr = ftp;
  else {                                      /* Expect sine wave */
    return csound->InitError(csound, "%s", Str("No table for Clarinet"));
  }
  if (*p->lowestFreq>=FL(0.0)) {      /* Skip initialisation */
    if (*p->lowestFreq)
      p->length = (int32_t) (CS_ESR / *p->lowestFreq + FL(1.0));
    else if (LIKELY(*p->frequency))
      p->length = (int32_t) (CS_ESR / *p->frequency + FL(1.0));
    else {
      csound->Warning(csound, "%s", Str("No base frequency for clarinet "
                                        "-- assuming 50Hz\n"));
      p->length = (int32_t) (CS_ESR / FL(50.0) + FL(1.0));
    }
    make_DLineL(csound, &p->delayLine, p->length);
    p->reedTable.offSet = FL(0.7);
    p->reedTable.slope = -FL(0.3);
    make_OneZero(&(p->filter));
    make_Envelope(&p->envelope);
    make_Noise(p->noise);
    /*    p->noiseGain = 0.2f; */       /* Arguemnts; suggested values? */
    /*    p->vibrGain = 0.1f; */
    {
      int32_t relestim = (int32_t)(CS_EKR * FL(0.1));
      /* 1/10th second decay extention */
      if (relestim > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = relestim;
    }
    p->kloop = (int32_t) ((int32_t) (p->h.insdshead->offtim * CS_EKR)
                          - (int32_t) (CS_EKR * *p->attack));
#ifdef BETA
    csound->Message(csound, "offtim=%f  kloop=%d\n",
                    p->h.insdshead->offtim, p->kloop);
#endif
    p->envelope.rate = FL(0.0);
    p->v_time = 0;
  }
  return OK;
}

int32_t clarin(CSOUND *csound, CLARIN *p)
{
  MYFLT *ar = p->ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT amp = (*p->amp)*AMP_RSCALE; /* Normalise */
  MYFLT nGain = *p->noiseGain;
  int32_t v_len = (int32_t)p->vibr->flen;
  MYFLT *v_data = p->vibr->ftable;
  MYFLT vibGain = *p->vibAmt;
  MYFLT vTime = p->v_time;

  if (p->envelope.rate==FL(0.0)) {
    p->envelope.rate =  amp /(*p->attack*CS_ESR);
    p->envelope.value = p->envelope.target = FL(0.55) + amp*FL(0.30);
  }
  p->outputGain = amp + FL(0.001);
  DLineL_setDelay(&p->delayLine, /* length - approx filter delay */
                  (CS_ESR/ *p->frequency) * FL(0.5) - FL(1.5));
  p->v_rate = *p->vibFreq * p->vibr->flen * CS_ONEDSR;
  /* Check to see if into decay yet */
  if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    p->envelope.state = 1;  /* Start change */
    p->envelope.rate = p->envelope.value / (*p->dettack * CS_ESR);
    p->envelope.target =  FL(0.0);
#ifdef BETA
    csound->Message(csound, "Set off phase time = %f Breath v,r = %f, %f\n",
                    (MYFLT) CS_KCNT * CS_ONEDKR,
                    p->envelope.value, p->envelope.rate);
#endif
  }
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    MYFLT   pressureDiff;
    MYFLT   breathPressure;
    int32    temp;
    MYFLT   temp_time, alpha;
    MYFLT   nextsamp;
    MYFLT   v_lastOutput;
    MYFLT   lastOutput;

    breathPressure = Envelope_tick(&p->envelope);
    breathPressure += breathPressure * nGain * Noise_tick(csound,&p->noise);
    /* Tick on vibrato table   */
    vTime += p->v_rate;            /*  Update current time    */
    while (vTime >= v_len)         /*  Check for end of sound */
      vTime -= v_len;              /*  loop back to beginning */
    while (vTime < FL(0.0))        /*  Check for end of sound */
      vTime += v_len;              /*  loop back to beginning */

    temp_time = vTime;

#ifdef have_phase
    if (p->v_phaseOffset != FL(0.0)) {
      temp_time += p->v_phaseOffset;   /*  Add phase offset       */
      while (temp_time >= v_len)       /*  Check for end of sound */
        temp_time -= v_len;            /*  loop back to beginning */
      while (temp_time < FL(0.0))      /*  Check for end of sound */
        temp_time += v_len;            /*  loop back to beginning */
    }
#endif
    temp = (int32_t) temp_time;    /*  Integer part of time address    */
                                   /*  fractional part of time address */
    alpha = temp_time - (MYFLT)temp;
    v_lastOutput = v_data[temp]; /* Do linear interpolation */
    /*  same as alpha*data[temp+1] + (1-alpha)data[temp] */
    v_lastOutput += (alpha * (v_data[temp+1] - v_lastOutput));
    /* End of vibrato tick */
    breathPressure += breathPressure * vibGain * v_lastOutput;
    pressureDiff = OneZero_tick(&p->filter, /* differential pressure  */
                                DLineL_lastOut(&p->delayLine));
    pressureDiff = (-FL(0.95)*pressureDiff) - breathPressure;
    /* of reflected and mouth */
    nextsamp = pressureDiff * ReedTabl_LookUp(&p->reedTable,pressureDiff);
    nextsamp =  breathPressure + nextsamp;
    /* perform scattering in economical way */
    lastOutput = DLineL_tick(&p->delayLine, nextsamp);
    lastOutput *= p->outputGain;
    ar[n] = lastOutput*AMP_SCALE;
  }
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

static inline MYFLT JetTabl_lookup(MYFLT sample) /* Perform "Table Lookup"  */
{                                  /* By Polynomial Calculation */
                                   /* (x^3 - x) approximates sigmoid of jet */
  MYFLT j = sample * (sample*sample - FL(1.0));
  if (j > FL(1.0)) j = FL(1.0);        /* Saturation at +/- 1.0       */
  else if (j < -FL(1.0)) j = -FL(1.0);
  return j;
}

int32_t fluteset(CSOUND *csound, FLUTE *p)
{
  FUNC        *ftp;
  int32        length;

  if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) p->vibr = ftp;
  else {                                   /* Expect sine wave */
    return csound->InitError(csound, "%s", Str("No table for Flute"));
  }
  if (*p->lowestFreq>=FL(0.0)) {      /* Skip initialisation?? */
    if (*p->lowestFreq!=FL(0.0)) {
      length = (int32_t) (CS_ESR / *p->lowestFreq + FL(1.0));
      p->limit = *p->lowestFreq;
    }
    else if (*p->frequency!=FL(0.0)) {
      length = (int32_t) (CS_ESR / *p->frequency + FL(1.0));
      p->limit = *p->frequency;
    }
    else {
      csound->Warning(csound, "%s", Str("No base frequency for flute "
                                        "-- assumed to be 50Hz\n"));
      length = (int32_t) (CS_ESR / FL(50.0) + FL(1.0));
      p->limit = FL(50.0);
    }
    make_DLineL(csound, &p->boreDelay, length);
    length = length >> 1;        /* ??? really; yes from later version */
    make_DLineL(csound, &p->jetDelay, length);
    make_OnePole(&p->filter);
    make_DCBlock(&p->dcBlock);
    make_Noise(p->noise);
    make_ADSR(&p->adsr, CS_ESR);
    /* Clear */
    /*     OnePole_clear(&p->filter); */
    /*     DCBlock_clear(&p->dcBlock); */
    /* End Clear */
    /*       DLineL_setDelay(&p->boreDelay, 100.0f); */
    /*       DLineL_setDelay(&p->jetDelay, 49.0f); */

    OnePole_setPole(&p->filter, FL(0.7) - (FL(0.1) * RATE_NORM));
    OnePole_setGain(&p->filter, -FL(1.0));
    ADSR_setAllTimes(csound, &p->adsr, FL(0.005), FL(0.01), FL(0.8), FL(0.010));
    /*        ADSR_setAll(&p->adsr, 0.02f, 0.05f, 0.8f, 0.001f); */
    /* Suggested values */
    /*    p->endRefl = 0.5; */
    /*    p->jetRefl = 0.5; */
    /*    p->noiseGain = 0.15; */ /* Breath pressure random component   */
    /*    p->vibrGain = 0.05;  */ /* breath periodic vibrato component  */
    /*    p->jetRatio = 0.32;  */
    p->lastamp = FL(1.0);       /* Remember */
    /* This should be controlled by attack */
    ADSR_setAttackRate(csound, &p->adsr, FL(0.02));
    p->maxPress = FL(2.3) / FL(0.8);
    p->outputGain = FL(1.001);
    ADSR_keyOn(&p->adsr);
    p->kloop = (MYFLT)((int32_t)(p->h.insdshead->offtim*CS_EKR -
                                 CS_EKR*(*p->dettack)));

    p->lastFreq = FL(0.0);
    p->lastJet = -FL(1.0);
    /* freq = (2/3)*p->frequency as we're overblowing here */
    /* but 1/(2/3) is 1.5 so multiply for speed */
  }
  return OK;
}

int32_t flute(CSOUND *csound, FLUTE *p)
{
  MYFLT       *ar = p->ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       amp = (*p->amp)*AMP_RSCALE; /* Normalise */
  MYFLT       temp;
  int32_t     v_len = (int32_t)p->vibr->flen;
  MYFLT       *v_data = p->vibr->ftable;
  MYFLT       v_time = p->v_time;
  MYFLT       vibGain = *p->vibAmt;
  MYFLT       jetRefl, endRefl, noisegain;

  if (amp!=p->lastamp) {      /* If amplitude has changed */
    /* This should be controlled by attack */
    ADSR_setAttackRate(csound, &p->adsr, amp * FL(0.02));
    p->maxPress = (FL(1.1) + (amp * FL(0.20))) / FL(0.8);
    p->outputGain = amp + FL(0.001);
    p->lastamp = amp;
  }
  p->v_rate = *p->vibFreq * v_len * CS_ONEDSR;
  /* Start SetFreq */
  if (p->lastFreq != *p->frequency) { /* It changed */
    p->lastFreq = *p->frequency;
    if (p->limit>p->lastFreq) {
      p->lastFreq = p->limit;
      csound->Warning(csound, "%s", Str("frequency too low, set to minimum"));
    }
    p->lastJet = *p->jetRatio;
    /* freq = (2/3)*p->frequency as we're overblowing here */
    /* but 1/(2/3) is 1.5 so multiply for speed */
    /* Length - approx. filter delay */
    temp = FL(1.5)* CS_ESR / p->lastFreq - FL(2.0);
    DLineL_setDelay(&p->boreDelay, temp); /* Length of bore tube */
    DLineL_setDelay(&p->jetDelay, temp * p->lastJet); /* jet delay shorter */
  }
  else if (*p->jetRatio != p->lastJet) { /* Freq same but jet changed */
    p->lastJet = *p->jetRatio;
    /* Length - approx. filter delay */
    temp = FL(1.5)* CS_ESR / p->lastFreq - FL(2.0);
    DLineL_setDelay(&p->jetDelay, temp * p->lastJet); /* jet delay shorter */
  }
  /* End SetFreq */

  if (p->kloop>FL(0.0) && p->h.insdshead->relesing) p->kloop=FL(1.0);
  if ((--p->kloop) == 0) {
    p->adsr.releaseRate = p->adsr.value / (*p->dettack * CS_ESR);
    p->adsr.target = FL(0.0);
    p->adsr.rate = p->adsr.releaseRate;
    p->adsr.state = RELEASE;
  }
  noisegain = *p->noiseGain; jetRefl = *p->jetRefl; endRefl = *p->endRefl;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    int32     temp;
    MYFLT     temf;
    MYFLT     temp_time, alpha;
    MYFLT     pressDiff;
    MYFLT     randPress;
    MYFLT     breathPress;
    MYFLT     lastOutput;
    MYFLT     v_lastOutput;

    breathPress = p->maxPress * ADSR_tick(&p->adsr); /* Breath Pressure */
    randPress = noisegain*Noise_tick(csound,&p->noise); /* Random Deviation */
    /* Tick on vibrato table */
    v_time += p->v_rate;            /*  Update current time    */
    while (v_time >= v_len)         /*  Check for end of sound */
      v_time -= v_len;              /*  loop back to beginning */
    while (v_time < FL(0.0))        /*  Check for end of sound */
      v_time += v_len;              /*  loop back to beginning */

    temp_time = v_time;

#ifdef phase_offset
    if (p->v_phaseOffset != FL(0.0)) {
      temp_time += p->v_phaseOffset;/*  Add phase offset       */
      while (temp_time >= v_len)    /*  Check for end of sound */
        temp_time -= v_len;         /*  loop back to beginning */
      while (temp_time < FL(0.0))   /*  Check for end of sound */
        temp_time += v_len;         /*  loop back to beginning */
    }
#endif

    temp = (int32_t) temp_time;        /*  Integer part of time address    */
                                       /*  fractional part of time address */
    alpha = temp_time - (MYFLT)temp;
    v_lastOutput = v_data[temp];    /* Do linear interpolation */
    /*  same as alpha*data[temp+1] + (1-alpha)data[temp] */
    v_lastOutput += (alpha * (v_data[temp+1] - v_lastOutput));
    /* End of vibrato tick */
    randPress += vibGain * v_lastOutput; /* + breath vibrato       */
    randPress *= breathPress;            /* All scaled by Breath Pressure */
    temf = OnePole_tick(&p->filter, DLineL_lastOut(&p->boreDelay));
    temf = DCBlock_tick(&p->dcBlock, temf);   /* Block DC on reflection */
    pressDiff = breathPress + randPress       /* Breath Pressure   */
      - (jetRefl * temf);        /*  - reflected      */
    pressDiff = DLineL_tick(&p->jetDelay, pressDiff);  /* Jet Delay Line */
    pressDiff = JetTabl_lookup(pressDiff)     /* Non-Lin Jet + reflected */
      + (endRefl * temf);
    /* Bore Delay and "bell" filter  */
    lastOutput = FL(0.3) * DLineL_tick(&p->boreDelay, pressDiff);

    lastOutput *= p->outputGain;
    ar[n] = lastOutput*AMP_SCALE*FL(1.4);
  }

  p->v_time = v_time;
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

/*  Perform Table Lookup    */
MYFLT BowTabl_lookup(CSOUND *csound, BowTabl *b, MYFLT sample)
{                                              /*  sample is differential  */
  MYFLT lastOutput;                          /*  string vs. bow velocity */
  MYFLT input;
  input = sample /* + b->offSet*/ ;          /*  add bias to sample      */
  input *= b->slope;                         /*  scale it                */
  lastOutput = FABS(input) + FL(0.75); /*  below min delta, frict = 1 */
  lastOutput = intpow(lastOutput,-4L);
  /* if (lastOutput < FL(0.0) ) lastOutput = FL(0.0); */ /* minimum frict is 0.0 */
  if (lastOutput > FL(1.0)) lastOutput = FL(1.0); /*  maximum friction is 1.0 */
  return lastOutput;
}

int32_t bowedset(CSOUND *csound, BOWED *p)
{
  int32        length;
  FUNC        *ftp;
  MYFLT       amp = (*p->amp)*AMP_RSCALE; /* Normalise */

  if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) p->vibr = ftp;
  else {                                      /* Expect sine wave */
    return csound->InitError(csound, "%s", Str("No table for wgbow vibrato"));
  }
  if (*p->lowestFreq>=FL(0.0)) {      /* If no init skip */
    if (*p->lowestFreq!=FL(0.0)) {
      length = (int32_t) (CS_ESR / *p->lowestFreq + FL(1.0));
      p->limit = *p->lowestFreq;
    }
    else if (*p->frequency!=FL(0.0)) {
      length = (int32_t) (CS_ESR / *p->frequency + FL(1.0));
      p->limit = *p->frequency;
    }
    else {
      csound->Warning(csound, "%s", Str("unknown lowest frequency for bowed string "
                                        "-- assuming 50Hz\n"));
      length = (int32_t) (CS_ESR / FL(50.0) + FL(1.0));
      p->limit = FL(50.0);
    }
    make_DLineL(csound, &p->neckDelay, length);
    length = length >> 1; /* Unsure about this; seems correct in later code */
    make_DLineL(csound, &p->bridgeDelay, length);

    /*  p->bowTabl.offSet = FL(0.0);*/
    /* offset is a bias, really not needed unless */
    /* friction is different in each direction    */

    /* p->bowTabl.slope contrls width of friction pulse, related to bowForce */
    p->bowTabl.slope = FL(3.0);
    make_OnePole(&p->reflFilt);
    make_BiQuad(&p->bodyFilt);
    make_ADSR(&p->adsr, CS_ESR);

    DLineL_setDelay(&p->neckDelay, FL(100.0));
    DLineL_setDelay(&p->bridgeDelay, FL(29.0));

    OnePole_setPole(&p->reflFilt, FL(0.6) - (FL(0.1) * RATE_NORM));
    OnePole_setGain(&p->reflFilt, FL(0.95));

    BiQuad_setFreqAndReson(p->bodyFilt, FL(500.0), FL(0.85));
    BiQuad_setEqualGainZeroes(p->bodyFilt);
    BiQuad_setGain(p->bodyFilt, FL(0.2));

    ADSR_setAllTimes(csound, &p->adsr, FL(0.02), FL(0.005), FL(0.9), FL(0.01));
    /*        ADSR_setAll(&p->adsr, 0.002f,0.01f,0.9f,0.01f); */

    p->adsr.target = FL(1.0);
    p->adsr.rate = p->adsr.attackRate;
    p->adsr.state = ATTACK;
    p->maxVelocity = FL(0.03) + (FL(0.2) * amp);

    p->lastpress = FL(0.0);   /* Set unknown state */
    p->lastfreq = FL(0.0);
    p->lastbeta = FL(0.0);    /* Remember states */
    p->lastamp = amp;
  }
  return OK;
}

int32_t bowed(CSOUND *csound, BOWED *p)
{
  MYFLT       *ar = p->ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       amp = (*p->amp)*AMP_RSCALE; /* Normalise */
  MYFLT       maxVel;
  int32_t         freq_changed = 0;

  if (amp != p->lastamp) {
    p->maxVelocity = FL(0.03) + (FL(0.2) * amp);
    p->lastamp = amp;
  }
  maxVel = p->maxVelocity;
  if (p->lastpress != *p->bowPress)
    p->bowTabl.slope = p->lastpress = *p->bowPress;

  /* Set Frequency if changed */
  if (p->lastfreq != *p->frequency) {
    /* delay - approx. filter delay */
    if (p->limit<=*p->frequency)
      p->lastfreq = *p->frequency;
    else {
      p->lastfreq = p->limit;
      csound->Warning(csound, "%s", Str("frequency too low, set to minimum"));
    }
    p->baseDelay = CS_ESR / p->lastfreq - FL(4.0);
    freq_changed = 1;
  }
  if (p->lastbeta != *p->betaRatio ||
      freq_changed) {         /* Reset delays if changed */
    p->lastbeta = *p->betaRatio;
    DLineL_setDelay(&p->bridgeDelay, /* bow to bridge length */
                    p->baseDelay * p->lastbeta);
    DLineL_setDelay(&p->neckDelay, /* bow to nut (finger) length */
                    p->baseDelay *(FL(1.0) - p->lastbeta));
  }
  p->v_rate = *p->vibFreq * p->vibr->flen * CS_ONEDSR;
  if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    ADSR_setDecayRate(csound, &p->adsr, (FL(1.0) - p->adsr.value) * FL(0.005));
    p->adsr.target = FL(0.0);
    p->adsr.rate = p->adsr.releaseRate;
    p->adsr.state = RELEASE;
  }

  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    MYFLT     bowVelocity;
    MYFLT     bridgeRefl=FL(0.0), nutRefl=FL(0.0);
    MYFLT     newVel=FL(0.0), velDiff=FL(0.0), stringVel=FL(0.0);
    MYFLT     lastOutput;

    bowVelocity = maxVel * ADSR_tick(&p->adsr);

    /* Bridge Reflection      */
    bridgeRefl = - OnePole_tick(&p->reflFilt, p->bridgeDelay.lastOutput);
    nutRefl = - p->neckDelay.lastOutput;       /* Nut Reflection  */
    stringVel = bridgeRefl + nutRefl;          /* Sum is String Velocity */
    velDiff = bowVelocity - stringVel;         /* Differential Velocity  */
    /* Non-Lin Bow Function   */
    newVel = velDiff * BowTabl_lookup(csound, &p->bowTabl, velDiff);
    DLineL_tick(&p->neckDelay, bridgeRefl + newVel);  /* Do string       */
    DLineL_tick(&p->bridgeDelay, nutRefl + newVel);   /*   propagations  */

    if (*p->vibAmt > FL(0.0)) {
      int32    temp;
      MYFLT   temp_time, alpha;
      /* Tick on vibrato table */
      p->v_time += p->v_rate;              /*  Update current time    */
      while (p->v_time >= p->vibr->flen)   /*  Check for end of sound */
        p->v_time -= p->vibr->flen;        /*  loop back to beginning */
      while (p->v_time < FL(0.0))          /*  Check for end of sound */
        p->v_time += p->vibr->flen;        /*  loop back to beginning */

      temp_time = p->v_time;

#ifdef phase_offset
      if (p->v_phaseOffset != FL(0.0)) {
        temp_time += p->v_phaseOffset;     /*  Add phase offset       */
        while (temp_time >= p->vibr->flen) /*  Check for end of sound */
          temp_time -= p->vibr->flen;      /*  loop back to beginning */
        while (temp_time < FL(0.0))        /*  Check for end of sound */
          temp_time += p->vibr->flen;      /*  loop back to beginning */
      }
#endif
      temp = (int32_t) temp_time;    /*  Integer part of time address    */
      /*  fractional part of time address */
      alpha = temp_time - (MYFLT)temp;
      p->v_lastOutput = p->vibr->ftable[temp]; /* Do linear interpolation */
      /*  same as alpha*data[temp+1] + (1-alpha)data[temp] */
      p->v_lastOutput = p->v_lastOutput +
        (alpha * (p->vibr->ftable[temp+1] - p->v_lastOutput));
      /* End of vibrato tick */

      DLineL_setDelay(&p->neckDelay,
                      (p->baseDelay * (FL(1.0) - p->lastbeta)) +
                      (p->baseDelay * *p->vibAmt * p->v_lastOutput));
    }
    else
      DLineL_setDelay(&p->neckDelay,
                      (p->baseDelay * (FL(1.0) - p->lastbeta)));

    lastOutput = BiQuad_tick(&p->bodyFilt, p->bridgeDelay.lastOutput);

    ar[n] = lastOutput*AMP_SCALE * amp *FL(1.8);
  }
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

void make_DLineA(CSOUND *csound, DLineA *p, int32 max_length)
{
  p->length = max_length;
  csound->AuxAlloc(csound, max_length * sizeof(MYFLT), &p->inputs);
  p->lastIn = FL(0.0);
  p->lastOutput = FL(0.0);
  p->inPoint = 0;
  p->outPoint = max_length >> 1;
}

int32_t DLineA_setDelay(CSOUND *csound, DLineA *p, MYFLT lag)
{
  MYFLT outputPointer;
  /* outPoint chases inpoint + 2 for interp and other        */
  outputPointer = (MYFLT)p->inPoint - lag + FL(2.0);

  if (UNLIKELY(p->length<=0)) goto err1;
  while (outputPointer<0)
    outputPointer += p->length;        /* modulo table length            */
  p->outPoint = (int32_t) outputPointer;    /* Integer part of delay          */
  p->alpha = FL(1.0) + p->outPoint - outputPointer;/* fractional part of delay */
  if (p->alpha<FL(0.1)) {
    ///outputPointer += FL(1.0);          /*  Hack to avoid pole/zero       */
    p->outPoint++;                        /*  cancellation.  Keeps allpass  */
    p->alpha += FL(1.0);                  /*  delay in range of .1 to 1.1   */
  }
  p->coeff = (FL(1.0)-p->alpha)/(FL(1.0)+p->alpha); /* coefficient for all pass*/
  return 0;
 err1:
  csound->ErrorMsg(csound, "%s", Str("DlineA not initialised"));
  return NOTOK;
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

void LipFilt_setFreq(BRASS *p, LipFilt *pp, MYFLT frequency)
{
  MYFLT coeffs[2];
  coeffs[0] = FL(2.0) * FL(0.997) *
    (MYFLT)cos(CS_TPIDSR * (double)frequency);   /* damping should  */
  coeffs[1] = -FL(0.997) * FL(0.997);                 /* change with lip */
  BiQuad_setPoleCoeffs(pp, coeffs);                    /* parameters, but */
  BiQuad_setGain(*pp, FL(0.03));                       /* not yet.        */
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
  output += (FL(1.0)-temp) * boreSample; /* and Bore reflection is compliment */
  return output;
}

/* ====================================================================== */

int32_t brassset(CSOUND *csound, BRASS *p)
{
  FUNC        *ftp;
  MYFLT amp = (*p->amp)*AMP_RSCALE; /* Normalise */

  if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) p->vibr = ftp;
  else {                                      /* Expect sine wave */
    return csound->InitError(csound, "%s", Str("No table for Brass"));
  }
  p->frq = *p->frequency;     /* Remember */
  if (*p->lowestFreq>=FL(0.0)) {
    if (*p->lowestFreq!=FL(0.0)) {
      p->length = (int32_t) (CS_ESR / *p->lowestFreq + FL(1.0));
      p->limit = *p->lowestFreq;
    }
    else if (p->frq!=FL(0.0)) {
      p->length = (int32_t) (CS_ESR / p->frq + FL(1.0));
      p->limit = p->frq;
    }
    else {
      csound->Warning(csound, "%s", Str("No base frequency for brass "
                                        "-- assumed to be 50Hz\n"));
      p->length = (int32_t) (CS_ESR / FL(50.0) + FL(1.0));
      p->limit = FL(50.0);
    }
    make_DLineA(csound, &p->delayLine, p->length);
    make_LipFilt(&p->lipFilter);
    make_DCBlock(&p->dcBlock);
    make_ADSR(&p->adsr, CS_ESR);
    ADSR_setAllTimes(csound, &p->adsr, FL(0.005), FL(0.001), FL(1.0), FL(0.010));
    /*        ADSR_setAll(&p->adsr, 0.02f, 0.05f, FL(1.0), 0.001f); */

    ADSR_setAttackRate(csound, &p->adsr, amp * FL(0.001));

    p->maxPressure = amp;
    ADSR_keyOn(&p->adsr);

    /* Set frequency */
    /*      p->slideTarget = (CS_ESR / p->frq * FL(2.0)) + 3.0f; */
    /* fudge correction for filter delays */
    /*      DLineA_setDelay(&p->delayLine, p->slideTarget);*/
    /* we'll play a harmonic  */
    p->lipTarget = FL(0.0);
    /*        LipFilt_setFreq(csound, &p->lipFilter, p->frq); */
    /* End of set frequency */
    p->frq = FL(0.0);         /* to say we do not know */
    p->lipT = FL(0.0);
    /*     LipFilt_setFreq(csound, &p->lipFilter, */
    /*                     p->lipTarget * (MYFLT)pow(4.0,
                           (2.0* p->lipT) -1.0)); */
    {
      int32_t relestim = (int32_t)(CS_EKR * FL(0.1));
      /* 1/10th second decay extention */
      if (relestim > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = relestim;
    }
    p->kloop = (int32_t) ((int32_t) (p->h.insdshead->offtim * CS_EKR)
                          - (int32_t) (CS_EKR * *p->dettack));
  }
  return OK;
}

int32_t brass(CSOUND *csound, BRASS *p)
{
  MYFLT *ar = p->ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT amp = (*p->amp)*AMP_RSCALE; /* Normalise */
  MYFLT maxPressure = p->maxPressure = amp;
  int32_t v_len = (int32_t)p->vibr->flen;
  MYFLT *v_data = p->vibr->ftable;
  MYFLT vibGain = *p->vibAmt;
  MYFLT vTime = p->v_time;

  p->v_rate = *p->vibFreq * v_len * CS_ONEDSR;
  /*   vibr->setFreq(6.137); */
  /* vibrGain = 0.05; */            /* breath periodic vibrato component  */
  if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    ADSR_setReleaseRate(csound, &p->adsr, amp * FL(0.005));
    ADSR_keyOff(&p->adsr);
  }
  if (p->frq != *p->frequency) {             /* Set frequency if changed */
    p->frq = *p->frequency;
    if (p->limit > p->frq) {
      p->frq =p->limit;
      csound->Warning(csound, "%s", Str("frequency too low, set to minimum"));
    }
    p->slideTarget = (CS_ESR / p->frq * FL(2.0)) + FL(3.0);
    /* fudge correction for filter delays */
    /*  we'll play a harmonic */
    if (DLineA_setDelay(csound, &p->delayLine, p->slideTarget)) return OK;
    p->lipTarget = p->frq;
    p->lipT = FL(0.0);                /* So other part is set */
  } /* End of set frequency */
  if (*p->liptension != p->lipT) {
    p->lipT = *p->liptension;
    LipFilt_setFreq(p, &p->lipFilter,
                    p->lipTarget * (MYFLT)pow(4.0,(2.0* p->lipT) -1.0));
  }

  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    MYFLT     breathPressure;
    MYFLT     lastOutput;
    int32_t       temp;
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

    temp = (int32_t) temp_time;            /*  Integer part of time address    */
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
                                            /* and bore reflection */
                                            FL(0.85) * p->delayLine.lastOutput)));
    ans = lastOutput*AMP_SCALE*FL(3.5);
    ar[n] = ans;
  }

  p->v_time = vTime;
  return OK;
}

#define S       sizeof
#include "mandolin.h"
#include "singwave.h"
#include "shaker.h"
#include "fm4op.h"
#include "bowedbar.h"
#include "marimba.h"
#include "vibraphn.h"

int32_t tubebellset(void*,void*);
int32_t tubebell(void*,void*);
int32_t rhodeset(void*,void*);
int32_t wurleyset(void*,void*);
int32_t wurley(void*,void*);
int32_t heavymetset(void*,void*);
int32_t heavymet(void*,void*);
int32_t b3set(void*,void*);
int32_t hammondB3(void*,void*);
int32_t FMVoiceset(void*,void*);
int32_t FMVoice(void*,void*);
int32_t percfluteset(void*,void*);
int32_t percflute(void*,void*);
int32_t Moog1set(void*,void*);
int32_t Moog1(void*,void*);
int32_t mandolinset(void*,void*);
int32_t mandolin(void*,void*);
int32_t voicformset(void*,void*);
int32_t voicform(void*,void*);
int32_t shakerset(void*,void*);
int32_t shaker(void*,void*);
int32_t bowedbarset(void*,void*);
int32_t bowedbar(void*,void*);
int32_t marimbaset(void*,void*);
int32_t marimba(void*,void*);
int32_t vibraphnset(void*,void*);
int32_t vibraphn(void*,void*);
int32_t agogobelset(void*,void*);
int32_t agogobel(void*,void*);


static OENTRY physmod_localops[] =
  {
    { "wgclar",  S(CLARIN),TR, "a", "kkkiikkkjo",(SUBR)clarinset, (SUBR)clarin },
    { "wgflute", S(FLUTE), TR, "a", "kkkiikkkjovv",(SUBR)fluteset, (SUBR)flute },
    { "wgbow",   S(BOWED), TR, "a", "kkkkkkjo", (SUBR)bowedset,   (SUBR)bowed },
    { "wgbrass", S(BRASS), TR, "a", "kkkikkjo", (SUBR)brassset,    (SUBR)brass},
    { "mandol", S(MANDOL), TR, "a", "kkkkkkio",(SUBR)mandolinset,(SUBR)mandolin},
    { "voice", S(VOICF),   TR, "a", "kkkkkkii",(SUBR)voicformset,(SUBR)voicform},
    { "fmbell",  S(FM4OP), TR, "a", "kkkkkkjjjjjo",
      (SUBR)tubebellset,(SUBR)tubebell},
    { "fmrhode", S(FM4OP), TR, "a", "kkkkkkiiiii",(SUBR)rhodeset,(SUBR)tubebell},
    { "fmwurlie", S(FM4OP),TR, "a", "kkkkkkiiiii",(SUBR)wurleyset,(SUBR)wurley },
    { "fmmetal", S(FM4OP), TR, "a", "kkkkkkiiiii",
      (SUBR)heavymetset, (SUBR)heavymet},
    { "fmb3", S(FM4OP),    TR, "a", "kkkkkkjjjjj", (SUBR)b3set,(SUBR)hammondB3 },
    { "fmvoice", S(FM4OPV),TR, "a", "kkkkkkjjjjj",
      (SUBR)FMVoiceset,(SUBR)FMVoice},
    { "fmpercfl", S(FM4OP),TR, "a", "kkkkkkjjjjj",
      (SUBR)percfluteset, (SUBR)percflute},
    { "moog", S(MOOG1),    TR, "a", "kkkkkkiii", (SUBR)Moog1set, (SUBR)Moog1  },
    { "shaker", S(SHAKER), 0, "a", "kkkkko",  (SUBR)shakerset,   (SUBR)shaker},
    { "wgbowedbar", S(BOWEDBAR), 0, "a","kkkkkoooo",
      (SUBR)bowedbarset,(SUBR) bowedbar },
    { "marimba", S(MARIMBA), TR,  "a", "kkiiikkiijj",
      (SUBR)marimbaset, (SUBR)marimba},
    { "vibes", S(VIBRAPHN),  TR,  "a", "kkiiikkii",
      (SUBR)vibraphnset,(SUBR)vibraphn},
    { "gogobel",S(VIBRAPHN), TR,  "a", "kkiiikki",
      (SUBR)agogobelset, (SUBR)agogobel},
  };



LINKAGE_BUILTIN(physmod_localops)

