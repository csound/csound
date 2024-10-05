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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

/*******************************************/
/*  4 Resonance Modal Synthesis Instrument */
/*  by Perry R. Cook, 1995-96              */
/*  This instrument contains an excitation */
/*  wavetable, an envelope, and four reso- */
/*  nances (Non-Sweeping BiQuad Filters).  */
/*******************************************/
// #include "csdl.h"
#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif


#include "modal4.h"
#include "marimba.h"
#include "vibraphn.h"
#include <math.h>
#include "interlocks.h"
static int32_t make_Modal4(CSOUND *csound,
                           Modal4 *m, MYFLT *ifn, MYFLT vgain, MYFLT vrate)
{
  FUNC        *ftp;

  if (LIKELY((ftp = csound->FTFind(csound,ifn)) != NULL))
    m->vibr = ftp;
  else {                                              /* Expect sine wave */
    csound->ErrorMsg(csound, "%s", Str("No table for Modal4 case"));
    return NOTOK;
  }
  make_Envelope(&m->envelope);
  /*  We do not make the excitation wave here yet,   */
  /*  because we do not know what it's going to be.  */
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

void Modal4_setFreq(CSOUND *csound, Modal4 *m, MYFLT frequency)
{
  m->baseFreq = frequency;
  Modal4_setRatioAndReson(csound, m, 0,m->ratios[0],m->resons[0]);
  Modal4_setRatioAndReson(csound, m, 1,m->ratios[1],m->resons[1]);
  Modal4_setRatioAndReson(csound, m, 2,m->ratios[2],m->resons[2]);
  Modal4_setRatioAndReson(csound, m, 3,m->ratios[3],m->resons[3]);
}

void Modal4_setRatioAndReson(CSOUND *csound,
                             Modal4 *m, int32_t whichOne, MYFLT ratio,MYFLT reson)
{
  MYFLT temp;
  Modal4 *p = m;
  if (ratio* m->baseFreq < m->sr * FL(0.5)) {
    m->ratios[whichOne] = ratio;
  }
  else {
    temp = ratio;
    while (temp* m->baseFreq > FL(0.5)*m->sr) temp *= FL(0.5);
    m->ratios[whichOne] = temp;
  }
  m->resons[whichOne] = reson;
  if (ratio<0)
    temp = -ratio;
  else
    temp = ratio * m->baseFreq;
  BiQuad_setFreqAndReson(m->filters[whichOne], temp,reson);
}

static void Modal4_strike(CSOUND *csound, Modal4 *m, MYFLT amplitude)
{
  int32_t i;
  MYFLT temp;
  Modal4 *p = m;
  Envelope_setRate(csound, &m->envelope, FL(1.0));
  Envelope_setTarget(&m->envelope, amplitude);
  OnePole_setPole(&m->onepole, FL(1.0) - amplitude);
  Envelope_tick(&m->envelope);
  m->w_time = FL(0.0);
  //m->w_lastOutput = FL(0.0);
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

static void Modal4_damp(CSOUND *csound, Modal4 *m, MYFLT amplitude)
{
  int32_t i;
  MYFLT temp;
  Modal4 *p = m; 
  for (i=0;i<4;i++)   {
    if (m->ratios[i] < 0)
      temp = - m->ratios[i];
    else
      temp = m->ratios[i] * m->baseFreq;
    BiQuad_setFreqAndReson(m->filters[i], temp, m->resons[i]*amplitude);
  }
}

static MYFLT Modal4_tick(Modal4 *m)
{
  MYFLT temp,temp2;
  int32 itemp;
  MYFLT temp_time, alpha, lastOutput;
  int32_t length = (int32_t)m->wave->flen;

  m->w_time += m->w_rate;                  /*  Update current time          */
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

  itemp = (int32) temp_time;               /* Integer part of time address  */
  alpha = temp_time - (MYFLT)itemp;      /* fractional part of time address */
  lastOutput = m->wave->ftable[itemp];     /*  Do linear interpolation      */
  lastOutput = lastOutput +                /*  same as alpha*data[temp+1]   */
    (alpha * (m->wave->ftable[itemp+1] -
              lastOutput));              /*  + (1-alpha)data[temp]        */

  temp   = m->masterGain *
    OnePole_tick(&m->onepole, lastOutput * Envelope_tick(&m->envelope));
  temp2  = BiQuad_tick(&m->filters[0], temp);
  temp2 += BiQuad_tick(&m->filters[1], temp);
  temp2 += BiQuad_tick(&m->filters[2], temp);
  temp2 += BiQuad_tick(&m->filters[3], temp);
  temp2  = temp2 - (temp2 * m->directGain);
  temp2 += m->directGain * temp;

  if (m->vibrGain != 0.0) {
    /*  Tick on vibrato table  */
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
      while (temp_time < FL(0.0))        /*  Check for end of sound */
        temp_time += m->vibr->flen;      /*  loop back to beginning */
    }
#endif

    itemp = (int32) temp_time;    /*  Integer part of time address    */
    /*  fractional part of time address */
    alpha = temp_time - (MYFLT)itemp;
    lastOutput = m->vibr->ftable[itemp]; /* Do linear interpolation */
    /*  same as alpha*data[itemp+1] + (1-alpha)data[temp] */
    lastOutput = /*m->v)*/lastOutput +
      (alpha * (m->vibr->ftable[itemp+1] - lastOutput));
    /* End of vibrato tick */
    temp = FL(1.0) + (lastOutput * m->vibrGain);   /*  Calculate AM      */
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

int32_t marimbaset(CSOUND *csound, MARIMBA *p)
{
  Modal4      *m = &(p->m4);
  MYFLT       temp,temp2;
  int32_t         itemp;
  FUNC        *ftp;
  p->m4.sr = CS_ESR;
  p->m4.h = p->h;

  if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL))
    p->m4.wave = ftp;
  else {                                    /* Expect an impulslything */
    return csound->InitError(csound, "%s", Str("No table for Marimba strike"));
  }

  if (UNLIKELY(make_Modal4(csound,
                           m, p->ivfn, *p->vibAmt, *p->vibFreq)==NOTOK))
    return NOTOK;
  p->m4.w_phaseOffset = FL(0.0);
  /*     p->m4.w_rate = 0.5; */
  Modal4_setRatioAndReson(csound,m,0, FL(1.00), FL(0.9996)); /* Set all 132.0 */
  Modal4_setRatioAndReson(csound,m,1, FL(3.99), FL(0.9994)); /* of our  523.0 */
  Modal4_setRatioAndReson(csound,m,2,FL(10.65), FL(0.9994)); /* default 1405.0 */
  Modal4_setRatioAndReson(csound,m,3,-FL(18.50),FL(0.999)); /* resonances 2443 */
  Modal4_setFiltGain(m, 0, FL(0.04));               /*  and        */
  Modal4_setFiltGain(m, 1, FL(0.01));               /*  gains      */
  Modal4_setFiltGain(m, 2, FL(0.01));               /*  for each   */
  Modal4_setFiltGain(m, 3, FL(0.008));              /*  resonance  */
  p->m4.directGain = FL(0.1);
  p->multiStrike = 0;
  p->strikePosition = *p->spos;
  /* Set Stick hardness stuff */
  p->stickHardness = *p->hardness;
  p->m4.w_rate = (FL(0.25) * (MYFLT)pow(4.0,(double)p->stickHardness));
  p->m4.masterGain = (FL(0.1) + (FL(1.8) * p->stickHardness));
  /* Set Strike position */
  temp2 = p->strikePosition * PI_F;
  temp = SIN(temp2);
  BiQuad_setGain(p->m4.filters[0], FL(0.12)*temp); /* 1st mode function of pos.*/
  temp = SIN(FL(0.05) + (FL(3.9) * temp2));
  BiQuad_setGain(p->m4.filters[1],
                 -FL(0.03)*temp); /* 2nd mode function of pos.*/
  temp = SIN(-FL(0.05) + (FL(11.0) * temp2));
  BiQuad_setGain(p->m4.filters[2], FL(0.11)*temp); /* 3rd mode function of pos.*/
  /* Strike */
  {
    int32_t triples = (*p->triples<=FL(0.0) ? 20 : (int32_t)*p->triples);
    int32_t doubles = (*p->doubles<=FL(0.0) ? 40 : triples + (int32_t)*p->doubles);
   const OPARMS *parm;
    parm =  csound->GetOParms(csound) ;
    itemp = csound->Rand31(csound->RandSeed1(csound)) % 100;
    if (itemp < triples) {
      p->multiStrike = 2;   
      if (parm->msglevel & CS_RNGEMSG)
        csound->Message(csound, "%s", Str("striking three times here!!!\n"));
    }
    else if (itemp < doubles) {
      p->multiStrike = 1;
      if (parm->msglevel & CS_RNGEMSG)
        csound->Message(csound, "%s", Str("striking twice here!!\n"));
    }
    else p->multiStrike = 0;
  }
  Modal4_strike(csound, m, *p->amplitude * AMP_RSCALE);
  Modal4_setFreq(csound, m, *p->frequency);
  p->first = 1;
  {
    int32_t relestim = (int32_t) (CS_EKR * *p->dettack);
    /* 0.1 second decay extention */
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = relestim;
  }
  p->kloop = (int32_t) ((int32_t) (p->h.insdshead->offtim * CS_EKR)
                        - (int32_t) (CS_EKR * *p->dettack));
  return OK;
}

int32_t marimba(CSOUND *csound, MARIMBA *p)
{
  Modal4      *m = &(p->m4);
  MYFLT       *ar = p->ar;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t    n, nsmps = CS_KSMPS;
  MYFLT       amp = (*p->amplitude) * AMP_RSCALE; /* Normalise */

  if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    Modal4_damp(csound, m, FL(1.0) - (amp * FL(0.03)));
  }
  p->m4.v_rate = *p->vibFreq; /* 6.0; */
  p->m4.vibrGain = *p->vibAmt; /* 0.05; */
  if (UNLIKELY(p->first)) {
    Modal4_strike(csound, m, *p->amplitude * AMP_RSCALE);
    Modal4_setFreq(csound, m, *p->frequency);
    p->first = 0;
  }
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    MYFLT     lastOutput;
    if (p->multiStrike>0)
      if (p->m4.w_allDone) {
        p->m4.w_time = FL(0.0);
        //p->m4.w_lastOutput = FL(0.0);
        p->m4.w_allDone = 0;
        p->multiStrike -= 1;
      }
    lastOutput = Modal4_tick(m);
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

int32_t vibraphnset(CSOUND *csound, VIBRAPHN *p)
{
  Modal4      *m = &(p->m4);
  MYFLT       temp;
  FUNC        *ftp;
  p->m4.sr = CS_ESR;
  p->m4.h = p->h;

  if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL))
    p->m4.wave = ftp;         /* Expect an impulslything */
  else {
    return csound->InitError(csound, "%s", Str("No table for Vibraphone strike"));
  }

  if (UNLIKELY(make_Modal4(csound, m, p->ivfn, *p->vibAmt, *p->vibFreq)==NOTOK))
    return NOTOK;

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
  /* Set Strike position */
  temp = (p->strikePosition = *p->spos) * PI_F;
  BiQuad_setGain(p->m4.filters[0], FL(0.025) * SIN(temp));
  BiQuad_setGain(p->m4.filters[1], FL(0.015) *
                 SIN(FL(0.1) + (FL(2.01) * temp)));
  BiQuad_setGain(p->m4.filters[2], FL(0.015) * SIN(FL(3.95) * temp));
  /* Strike */
  Modal4_strike(csound, m, *p->amplitude * AMP_RSCALE);
  Modal4_setFreq(csound, m, *p->frequency);
  p->first = 1;
  return OK;
}

int32_t vibraphn(CSOUND *csound, VIBRAPHN *p)
{
  Modal4      *m = &(p->m4);
  MYFLT       *ar = p->ar;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t    n, nsmps = CS_KSMPS;
  MYFLT       amp = (*p->amplitude)*AMP_RSCALE; /* Normalise */

  if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    Modal4_damp(csound, m, FL(1.0) - (amp * FL(0.03)));
  }
  if (UNLIKELY(p->first)) {
    Modal4_strike(csound, m, *p->amplitude * AMP_RSCALE);
    Modal4_setFreq(csound, m, *p->frequency);
    p->first = 0;
  }
  p->m4.v_rate = *p->vibFreq;
  p->m4.vibrGain =*p->vibAmt;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    MYFLT     lastOutput = Modal4_tick(m);
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

int32_t agogobelset(CSOUND *csound, VIBRAPHN *p)
{
  Modal4      *m = &(p->m4);
  FUNC        *ftp;
  MYFLT       temp;
  p->m4.sr = CS_ESR;
  p->m4.h = p->h;

  /* Expect an impulslything */
  if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) p->m4.wave = ftp;
  else {
    return csound->InitError(csound, "%s", Str("No table for Agogobell strike"));
  }

  if (UNLIKELY(make_Modal4(csound, m, p->ivfn, *p->vibAmt, *p->vibFreq)==NOTOK))
    return NOTOK;

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
  BiQuad_setGain(p->m4.filters[0], FL(0.08) * SIN(FL(0.7) * temp));
  BiQuad_setGain(p->m4.filters[1], FL(0.07) * SIN(FL(0.1) + (FL(5.0) * temp)));
  BiQuad_setGain(p->m4.filters[2], FL(0.04) * SIN(FL(0.2) + (FL(7.0) * temp)));
  /* Strike */
  Modal4_strike(csound, m, *p->amplitude*AMP_RSCALE);
  Modal4_setFreq(csound, m, *p->frequency);
  return OK;
}

int32_t agogobel(CSOUND *csound, VIBRAPHN *p)
{
  Modal4      *m = &(p->m4);
  MYFLT       *ar = p->ar;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t    n, nsmps = CS_KSMPS;

  p->m4.v_rate = *p->vibFreq;
  p->m4.vibrGain =*p->vibAmt;
  if (UNLIKELY(p->first)) {
    Modal4_strike(csound, m, *p->amplitude * AMP_RSCALE);
    Modal4_setFreq(csound, m, *p->frequency);
    p->first = 0;
  }
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    MYFLT     lastOutput = Modal4_tick(m);
    ar[n] = lastOutput*AMP_SCALE;
  }
  return OK;
}




