/*
    modal4.c:

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

/*******************************************/
/*  4 Resonance Modal Synthesis Instrument */
/*  by Perry R. Cook, 1995-96              */
/*  This instrument contains an excitation */
/*  wavetable, an envelope, and four reso- */
/*  nances (Non-Sweeping BiQuad Filters).  */
/*******************************************/
#include "csdl.h"
#include "modal4.h"
#include "marimba.h"
#include "vibraphn.h"
#include <math.h>

static int make_Modal4(ENVIRON *csound,
                       Modal4 *m, MYFLT *ifn, MYFLT vgain, MYFLT vrate)
{
    FUNC        *ftp;

    if ((ftp = csound->FTFind(csound,ifn)) != NULL)
      m->vibr = ftp;
    else {                                              /* Expect sine wave */
      csound->perferror_(csound->LocalizeString("No table for Modal4 case"));
      return NOTOK;
    }
    make_Envelope(&m->envelope);
        /*  We don't make the excitation wave here yet,   */
        /*  because we don't know what it's going to be.  */
    make_BiQuad(&m->filters[0]);
    make_BiQuad(&m->filters[1]);
    make_BiQuad(&m->filters[2]);
    make_BiQuad(&m->filters[3]);
    make_OnePole(&m->onepole);

    m->v_rate = vrate; /* 6.0; */
    m->vibrGain = vgain; /* 0.05; */

/*     m->directGain = 0.0; */
    m->masterGain = FL(1.0);
/*     m->baseFreq = 440.0; */
/*     Modal4_setRatioAndReson(m, 0, 1.00, 0.9997); */    /*  Set some      */
/*     Modal4_setRatioAndReson(m, 1, 1.30, 0.9997); */    /*  silly         */
/*     Modal4_setRatioAndReson(m, 2, 1.77, 0.9997); */    /*  default       */
/*     Modal4_setRatioAndReson(m, 3, 2.37, 0.9997); */    /*  values here   */
/*     Modal4_setFiltGain(m, 0, 0.01); */
/*     Modal4_setFiltGain(m, 1, 0.01); */
/*     Modal4_setFiltGain(m, 2, 0.01); */
/*     Modal4_setFiltGain(m, 3, 0.01); */
/*     OnePole_clear(&m->onepole); */
    BiQuad_clear(&m->filters[0]);
    BiQuad_clear(&m->filters[1]);
    BiQuad_clear(&m->filters[2]);
    BiQuad_clear(&m->filters[3]);
    BiQuad_setEqualGainZeroes(m->filters[0]);
    BiQuad_setEqualGainZeroes(m->filters[1]);
    BiQuad_setEqualGainZeroes(m->filters[2]);
    BiQuad_setEqualGainZeroes(m->filters[3]);
/*     stickHardness = 0.5; */
/*     strikePosition = 0.561; */
    return OK;
}

/* void Modal4_clear(Modal4 *m) */
/* {     */
/*     OnePole_clear(&m->onepole); */
/*     BiQuad_clear(&m->filters[0]); */
/*     BiQuad_clear(&m->filters[1]); */
/*     BiQuad_clear(&m->filters[2]); */
/*     BiQuad_clear(&m->filters[3]); */
/* } */

void Modal4_setFreq(ENVIRON *csound, Modal4 *m, MYFLT frequency)
{
    m->baseFreq = frequency;
    Modal4_setRatioAndReson(csound, m, 0,m->ratios[0],m->resons[0]);
    Modal4_setRatioAndReson(csound, m, 1,m->ratios[1],m->resons[1]);
    Modal4_setRatioAndReson(csound, m, 2,m->ratios[2],m->resons[2]);
    Modal4_setRatioAndReson(csound, m, 3,m->ratios[3],m->resons[3]);
}

#include <stdio.h>

void Modal4_setRatioAndReson(ENVIRON *csound,
                             Modal4 *m, int whichOne, MYFLT ratio,MYFLT reson)
{
    MYFLT temp;
    if (ratio* m->baseFreq < csound->esr_ * FL(0.5)) {
      m->ratios[whichOne] = ratio;
    }
    else {
      temp = ratio;
      while (temp* m->baseFreq > FL(0.5)*csound->esr_) temp *= FL(0.5);
      m->ratios[whichOne] = temp;
/*     printf("Modal4 : Aliasing would occur here, correcting.\n"); */
    }
    m->resons[whichOne] = reson;
    if (ratio<0)
      temp = -ratio;
    else
      temp = ratio * m->baseFreq;
    BiQuad_setFreqAndReson(m->filters[whichOne], temp,reson);
}

void Modal4_strike(ENVIRON *csound, Modal4 *m, MYFLT amplitude)
{
    int i;
    MYFLT temp;
    Envelope_setRate(csound, &m->envelope, FL(1.0));
    Envelope_setTarget(&m->envelope, amplitude);
    OnePole_setPole(&m->onepole, FL(1.0) - amplitude);
    Envelope_tick(&m->envelope);
    m->w_time = FL(0.0);
    m->w_lastOutput = FL(0.0);
    m->w_allDone = 0;
                                /*     wave->reset(); */
    for (i=0;i<4;i++)   {
      if (m->ratios[i] < 0)
        temp = - m->ratios[i];
      else
        temp = m->ratios[i] * m->baseFreq;
      BiQuad_setFreqAndReson(m->filters[i], temp, m->resons[i]);
    }
}


void Modal4_damp(ENVIRON *csound, Modal4 *m, MYFLT amplitude)
{
    int i;
    MYFLT temp;
    for (i=0;i<4;i++)   {
      if (m->ratios[i] < 0)
        temp = - m->ratios[i];
      else
        temp = m->ratios[i] * m->baseFreq;
      BiQuad_setFreqAndReson(m->filters[i], temp, m->resons[i]*amplitude);
    }
}

MYFLT Modal4_tick(Modal4 *m)
{
    MYFLT temp,temp2;
    long itemp;
    MYFLT temp_time, alpha;
    int length = (int)m->wave->flen;

    m->w_time += m->w_rate;                  /*  Update current time          */
/* printf("Modal4 tick: time=%f rate=%f\n", m->w_time, m->w_rate);   */
    if (m->w_time >= length)  {              /*  Check for end of sound       */
      m->w_time = (MYFLT)(length-1);         /*  stick at end                 */
      m->w_allDone = 1;                      /*  Information for one-shot use */
    }
    else if (m->w_time < FL(0.0))            /*  Check for end of sound       */
      m->w_time = FL(0.0);                   /*  stick at beg                 */

    temp_time = m->w_time;

#ifdef phase_offset
    if (m->w_phaseOffset != FL(0.0)) {
      temp_time += m->w_phaseOffset;         /*  Add phase offset             */
      if (temp_time >= length)               /*  Check for end of sound       */
        temp_time = length-1;                /*  stick at end                 */
      else if (temp_time < FL(0.0))          /*  check for end of sound       */
        temp_time = FL(0.0);                 /*  stick at beg                 */
    }
#endif

    itemp = (long) temp_time;                /* Integer part of time address  */
/* printf("temp_time=%f\t", temp_time); */
    alpha = temp_time - (MYFLT)itemp;     /*  fractional part of time address */
    m->w_lastOutput = m->wave->ftable[itemp]; /*  Do linear interpolation     */
/* printf("w_last1=%f\t", m->w_lastOutput); */
    m->w_lastOutput = m->w_lastOutput +      /*  same as alpha*data[temp+1]   */
        (alpha * (m->wave->ftable[itemp+1] -
                m->w_lastOutput));            /*  + (1-alpha)data[temp]       */
/* printf("w_last2=%f\n", m->w_lastOutput); */

    temp   = m->masterGain *
      OnePole_tick(&m->onepole, m->w_lastOutput * Envelope_tick(&m->envelope));
/* printf("onepole->%f\n", temp); */
    temp2  = BiQuad_tick(&m->filters[0], temp);
/* printf("BiQ_tick0: %f\n", temp2); */
    temp2 += BiQuad_tick(&m->filters[1], temp);
/* printf("BiQ_tick1: %f\n", temp2); */
    temp2 += BiQuad_tick(&m->filters[2], temp);
/* printf("BiQ_tick2: %f\n", temp2); */
    temp2 += BiQuad_tick(&m->filters[3], temp);
/* printf("BiQ_tick3: %f\n", temp2); */
    temp2  = temp2 - (temp2 * m->directGain);
    temp2 += m->directGain * temp;
/* printf("Temp2: %f\n", temp2); */

    if (m->vibrGain != 0.0) {
/* printf("Vibrato gain=%f\n", m->vibrGain); */
                                /* Tick on vibrato table */
      m->v_time += m->v_rate;              /*  Update current time    */
      while (m->v_time >= m->vibr->flen)   /*  Check for end of sound */
        m->v_time -= m->vibr->flen;        /*  loop back to beginning */
      while (m->v_time < FL(0.0))          /*  Check for end of sound */
        m->v_time += m->vibr->flen;        /*  loop back to beginning */

      temp_time = m->v_time;

#ifdef phase_offset
      if (m->v_phaseOffset != FL(0.0)) {
        temp_time += m->v_phaseOffset;     /*  Add phase offset       */
        while (temp_time >= m->vibr->flen) /*  Check for end of sound */
          temp_time -= m->vibr->flen;      /*  loop back to beginning */
        while (temp_time < FL(0.0))            /*  Check for end of sound */
          temp_time += m->vibr->flen;      /*  loop back to beginning */
      }
#endif

      itemp = (long) temp_time;    /*  Integer part of time address    */
                                   /*  fractional part of time address */
      alpha = temp_time - (MYFLT)itemp;
      m->v_lastOutput = m->vibr->ftable[itemp]; /* Do linear interpolation */
      /*  same as alpha*data[itemp+1] + (1-alpha)data[temp] */
      m->v_lastOutput = m->v_lastOutput +
        (alpha * (m->vibr->ftable[itemp+1] - m->v_lastOutput));
      /* End of vibrato tick */
      temp = FL(1.0) + (m->v_lastOutput * m->vibrGain);   /*  Calculate AM      */
      temp2 = temp * temp2;                          /* and apply to master out */
    }

    return (temp2 + temp2);
}

/*******************************************/
/*  Marimba SubClass of Modal4 Instrument, */
/*  by Perry R. Cook, 1995-96              */
/*                                         */
/*   Controls:    stickHardness            */
/*                strikePosition           */
/*                vibFreq                  */
/*                vibAmt                   */
/*******************************************/

int marimbaset(ENVIRON *csound, MARIMBA *p)
{
    Modal4      *m = &(p->m4);
    MYFLT       temp,temp2;
    int         itemp;
    FUNC        *ftp;

    if ((ftp = csound->FTFind(csound, p->ifn)) != NULL)
      p->m4.wave = ftp;
    else {
      perferror
        (Str(
             "No table for Marimba strike")); /* Expect an impulslything */
      return NOTOK;
    }

/*     { int i; */
/*      for (i=0; i<256; i++) printf("%d:%f\t", i, p->m4.wave->ftable[i]); */
/*     } */
    if (make_Modal4(csound,
                    m, p->ivfn, *p->vibAmt, *p->vibFreq)==NOTOK) return NOTOK;
    p->m4.w_phaseOffset = FL(0.0);
/*     p->m4.w_rate = 0.5; */
    Modal4_setRatioAndReson(csound,m, 0, FL(1.00), FL(0.9996)); /* Set all 132.0 */
    Modal4_setRatioAndReson(csound,m, 1, FL(3.99), FL(0.9994)); /* of our  523.0 */
    Modal4_setRatioAndReson(csound,m, 2,FL(10.65), FL(0.9994)); /* default 1405.0 */
    Modal4_setRatioAndReson(csound,m, 3,-FL(18.50), FL(0.999)); /* resonances 2443.0 */
    Modal4_setFiltGain(m, 0, FL(0.04));               /*  and        */
    Modal4_setFiltGain(m, 1, FL(0.01));               /*  gains      */
    Modal4_setFiltGain(m, 2, FL(0.01));               /*  for each   */
    Modal4_setFiltGain(m, 3, FL(0.008));              /*  resonance  */
    p->m4.directGain = FL(0.1);
    p->multiStrike = 0;
    p->strikePosition = *p->spos;
                                /* Set Stick hardness stuff */
    p->stickHardness = *p->hardness;
/*     printf("Stickhardness=%f\n", p->stickHardness); */
    p->m4.w_rate = (FL(0.25) * (MYFLT)pow(4.0,(double)p->stickHardness));
    p->m4.masterGain = (FL(0.1) + (FL(1.8) * p->stickHardness));
/* printf("Hardness=%f, w_rate=%f masterGain=%f [%f]\n", */
/*        *p->hardness, p->m4.w_rate, p->m4.masterGain, */
/*        pow(4.0,(double)p->stickHardness)); */
                                /* Set Strike position */
    temp2 = p->strikePosition * PI_F;
    temp = (MYFLT)sin((double)temp2);
    BiQuad_setGain(p->m4.filters[0], FL(0.12)*temp); /* 1st mode function of pos.*/
    temp = (MYFLT)sin(0.05 + (3.9 * (double)temp2));
    BiQuad_setGain(p->m4.filters[1], -FL(0.03)*temp); /* 2nd mode function of pos.*/
    temp = (MYFLT)sin(-0.05 + (11.0 * (double)temp2));
    BiQuad_setGain(p->m4.filters[2], FL(0.11)*temp); /* 3rd mode function of pos.*/
                                /* Strike */
    {
      int triples = (*p->triples<=FL(0.0) ? 20 : (int)*p->triples);
      int doubles = (*p->doubles<=FL(0.0) ? 40 : triples + (int)*p->doubles);
      itemp = rand() % 100;
      if (itemp < triples) {
        p->multiStrike = 2;
        if (csound->oparms_->msglevel & 02)
          printf(Str("striking three times here!!!\n"));
      }
      else if (itemp < doubles) {
        p->multiStrike = 1;
        if (csound->oparms_->msglevel & 02)
          printf(Str("striking twice here!!\n"));
      }
      else p->multiStrike = 0;
    }
    Modal4_strike(csound, m, *p->amplitude * AMP_RSCALE);
    Modal4_setFreq(csound, m, *p->frequency);
    p->first = 1;
    {
      int relestim = (int)(ekr * *p->dettack); /* 0.1 second decay extention */
      if (relestim > p->h.insdshead->xtratim)
        p->h.insdshead->xtratim = relestim;
    }
    p->kloop = (int)((p->h.insdshead->offtim * ekr) - (int)(ekr* *p->dettack));
    return OK;
}


int marimba(ENVIRON *csound, MARIMBA *p)
{
    Modal4      *m = &(p->m4);
    MYFLT       *ar = p->ar;
    int         n,nsmps = ksmps;
    MYFLT       amp = (*p->amplitude) * AMP_RSCALE; /* Normalise */

    if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
    if ((--p->kloop) == 0) {
/*       printf("Start damp\n"); */
      Modal4_damp(csound, m, FL(1.0) - (amp * FL(0.03)));
    }
    p->m4.v_rate = *p->vibFreq; /* 6.0; */
    p->m4.vibrGain = *p->vibAmt; /* 0.05; */
    if (p->first) {
      Modal4_strike(csound, m, *p->amplitude * AMP_RSCALE);
      Modal4_setFreq(csound, m, *p->frequency);
      p->first = 0;
    }
    for (n=0;n<nsmps;n++) {
      MYFLT     lastOutput;
      if (p->multiStrike>0)
        if (p->m4.w_allDone) {
          p->m4.w_time = FL(0.0);
          p->m4.w_lastOutput = FL(0.0);
          p->m4.w_allDone = 0;
          p->multiStrike -= 1;
        }
      lastOutput = Modal4_tick(m);
/*       printf("Sample=%f\n", lastOutput); */
      ar[n] = lastOutput*AMP_SCALE*FL(0.5);
    }
    return OK;
}

/*******************************************/
/*  Vibraphone SubClass of Modal4          */
/*  Instrument, by Perry R. Cook, 1995-96  */
/*                                         */
/*   Controls:    CONTROL1 = stickHardness */
/*                CONTROL2 = strikePosition*/
/*                CONTROL3 = vibFreq       */
/*                MOD_WHEEL= vibAmt        */
/*******************************************/

int vibraphnset(ENVIRON *csound, VIBRAPHN *p)
{
    Modal4      *m = &(p->m4);
    MYFLT       temp;
    FUNC        *ftp;

    if ((ftp = csound->FTFind(csound, p->ifn)) != NULL)
      p->m4.wave = ftp;         /* Expect an impulslything */
    else {
      return perferror(Str(
                    "No table for Vibraphone strike"));
    }

    if (make_Modal4(csound,
                    m, p->ivfn, *p->vibAmt, *p->vibFreq)==NOTOK) return NOTOK;

    p->m4.w_phaseOffset = FL(0.0);
/*     p->m4.w_rate = 13.33; */
    OnePole_setPole(&p->m4.onepole, FL(0.2));
    Modal4_setRatioAndReson(csound, m, 0, FL(1.0), FL(0.99995)); /*  Set         */
    Modal4_setRatioAndReson(csound, m, 1, FL(2.01),FL(0.99991)); /*  our         */
    Modal4_setRatioAndReson(csound, m, 2, FL(3.9), FL(0.99992)); /*  resonance   */
    Modal4_setRatioAndReson(csound, m, 3,FL(14.37),FL(0.99990)); /*  values here */
    Modal4_setFiltGain(m, 0, FL(0.025));
    Modal4_setFiltGain(m, 1, FL(0.015));
    Modal4_setFiltGain(m, 2, FL(0.015));
    Modal4_setFiltGain(m, 3, FL(0.015));
    p->m4.directGain = FL(0.0);
/*     vibrGain = 0.2; */
    p->m4.w_rate = FL(2.0) + (FL(22.66) * *p->hardness);
    p->m4.masterGain = FL(0.2) + (*p->hardness * FL(1.6));
/* printf("Hardness=%f, w_rate=%f masterGain=%f\n", */
/*        *p->hardness, p->m4.w_rate, p->m4.masterGain); */
                                /* Set Strike position */
    temp = (p->strikePosition = *p->spos) * PI_F;
    BiQuad_setGain(p->m4.filters[0], FL(0.025) * (MYFLT)sin((double)temp));
    BiQuad_setGain(p->m4.filters[1], FL(0.015) *
                   (MYFLT)sin(0.1 + (2.01 *(double) temp)));
    BiQuad_setGain(p->m4.filters[2], FL(0.015) * (MYFLT)sin(3.95 * (double)temp));
                                /* Strike */
    Modal4_strike(csound, m, *p->amplitude * AMP_RSCALE);
    Modal4_setFreq(csound, m, *p->frequency);
    p->first = 1;
    return OK;
}

int vibraphn(ENVIRON *csound, VIBRAPHN *p)
{
    Modal4      *m = &(p->m4);
    MYFLT       *ar = p->ar;
    int         n,nsmps = ksmps;
    MYFLT       amp = (*p->amplitude)*AMP_RSCALE; /* Normalise */

    if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
    if ((--p->kloop) == 0) {
/*       printf("Start damp\n"); */
      Modal4_damp(csound, m, FL(1.0) - (amp * FL(0.03)));
    }
    if (p->first) {
      Modal4_strike(csound, m, *p->amplitude * AMP_RSCALE);
      Modal4_setFreq(csound, m, *p->frequency);
      p->first = 0;
    }
    p->m4.v_rate = *p->vibFreq;
    p->m4.vibrGain =*p->vibAmt;
    for (n=0;n<nsmps;n++) {
      MYFLT     lastOutput = Modal4_tick(m);
/*       printf("Sample=%f\n", lastOutput); */
      ar[n] = lastOutput*FL(8.0)*AMP_SCALE;/* Times 8 as seems too quiet */
    }
    return OK;
}


/*******************************************/
/*  AgogoBell SubClass of Modal4 Instrument*/
/*  by Perry R. Cook, 1995-96              */
/*                                         */
/*   Controls:    CONTROL1 = stickHardness */
/*                CONTROL2 = strikePosition*/
/*                CONTROL3 = vibFreq       */
/*                MOD_WHEEL= vibAmt        */
/*******************************************/


/*   Modes measured from my Agogo Bell by FFT:  */
/*   360, 1470, 2401, 4600                      */


int agogobelset(ENVIRON *csound, VIBRAPHN *p)
{
    Modal4      *m = &(p->m4);
    FUNC        *ftp;
    MYFLT       temp;

    /* Expect an impulslything */
    if ((ftp = csound->FTFind(csound, p->ifn)) != NULL) p->m4.wave = ftp;
    else {
      return perferror(Str(
                    "No table for Agogobell strike"));
    }

    if (make_Modal4(csound,
                    m, p->ivfn, *p->vibAmt, *p->vibFreq)==NOTOK) return NOTOK;

    p->m4.w_phaseOffset = FL(0.0);
/*     p->m4.w_rate = 7.0; */
    OnePole_setPole(&p->m4.onepole, FL(0.2));
    Modal4_setRatioAndReson(csound, m, 0, FL(1.00), FL(0.999));   /* Set         */
    Modal4_setRatioAndReson(csound, m, 1, FL(4.08), FL(0.999));   /* our         */
    Modal4_setRatioAndReson(csound, m, 2, FL(6.669),FL(0.999));   /* resonance   */
    Modal4_setRatioAndReson(csound, m, 3,-FL(3725.0), FL(0.999)); /* values here */
    Modal4_setFiltGain(m, 0, FL(0.06));
    Modal4_setFiltGain(m, 1, FL(0.05));
    Modal4_setFiltGain(m, 2, FL(0.03));
    Modal4_setFiltGain(m, 3, FL(0.02));
    p->m4.directGain = FL(0.25);
/*     vibrGain = 0.2; */
    p->m4.w_rate = FL(3.0) + (FL(8.0) * *p->hardness);
    p->m4.masterGain = FL(1.0);
                                /* Set Strike position */
    temp = (p->strikePosition = *p->spos) * PI_F;
/*     printf("gain0=%f\n", 0.08 * sin(0.7 * temp)); */
    BiQuad_setGain(p->m4.filters[0], FL(0.08) * (MYFLT)sin(0.7 * temp));
/*     printf("gain1=%f\n" ,0.07 * sin(0.1 + (5.0 * temp))); */
    BiQuad_setGain(p->m4.filters[1], FL(0.07) * (MYFLT)sin(0.1 + (5.0 * temp)));
/*     printf("gain2=%f\n", 0.04 * sin(0.2 + (7.0 * temp))); */
    BiQuad_setGain(p->m4.filters[2], FL(0.04) * (MYFLT)sin(0.2 + (7.0 * temp)));
                                /* Strike */
    Modal4_strike(csound, m, *p->amplitude*AMP_RSCALE);
    Modal4_setFreq(csound, m, *p->frequency);
    return OK;
}

int agogobel(ENVIRON *csound, VIBRAPHN *p)
{
    Modal4      *m = &(p->m4);
    MYFLT       *ar = p->ar;
    int         n,nsmps = ksmps;

    p->m4.v_rate = *p->vibFreq;
    p->m4.vibrGain =*p->vibAmt;
    if (p->first) {
      Modal4_strike(csound, m, *p->amplitude * AMP_RSCALE);
      Modal4_setFreq(csound, m, *p->frequency);
      p->first = 0;
    }
    for (n=0;n<nsmps;n++) {
      MYFLT     lastOutput = Modal4_tick(m);
/*       printf("Sample=%f\n", lastOutput); */
      ar[n] = lastOutput*AMP_SCALE;
    }
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
{ "marimba", S(MARIMBA), 5, "a", "kkiiikkiijj", (SUBR)marimbaset, NULL, (SUBR)marimba},
{ "vibes", S(VIBRAPHN),  5, "a", "kkiiikkii", (SUBR)vibraphnset,NULL,(SUBR)vibraphn},
{ "gogobel",S(VIBRAPHN), 5, "a", "kkiiikki", (SUBR)agogobelset,NULL, (SUBR)agogobel},
};

LINKAGE

