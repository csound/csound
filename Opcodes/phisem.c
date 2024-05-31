/*
  phisem.c:

  Copyright (C) 1997, 2000 Perry Cook, John ffitch

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General ublic
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

/**********************************************************/
/*  PhISEM (Physically Informed Stochastic Event Modeling */
/*    by Perry R. Cook, Princeton, February 1997          */
/*                                                        */
/*  Meta-model that simulates all of:                     */
/*  Maraca Simulation by Perry R. Cook, Princeton, 1996-7 */
/*  Sekere Simulation by Perry R. Cook, Princeton, 1996-7 */
/*  Cabasa Simulation by Perry R. Cook, Princeton, 1996-7 */
/*  Bamboo Windchime Simulation, by Perry R. Cook, 1996-7 */
/*  Water Drops Simulation, by Perry R. Cook, 1996-7      */
/*  Tambourine Simulation, by Perry R. Cook, 1996-7       */
/*  Sleighbells Simulation, by Perry R. Cook, 1996-7      */
/*  Guiro Simulation, by Perry R. Cook, 1996-7            */
/*                                                        */
/**********************************************************/
/*  PhOLIES (Physically-Oriented Library of               */
/*    Imitated Environmental Sounds), Perry Cook, 1997-9  */
/*                                                        */
/*  Stix1 (walking on brittle sticks)                     */
/*  Crunch1 (like new fallen snow, or not)                */
/*  Wrench (basic socket wrench, friend of guiro)         */
/*  Sandpapr (sandpaper)                                  */
/**********************************************************/


#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "interlocks.h"
#include "phisem.h"
#include <math.h>

/* To do
   "10: Wrench", "12: CokeCan"};
*/

static inline int32_t my_random(CSOUND *csound, int32_t max)
{                                   /* Return Random Int Between 0 and max */
  return (csound->Rand31(csound->RandSeed1(csound)) % (max + 1));
}

static MYFLT noise_tick(CSOUND *csound)
{                         /* Return random MYFLT float between -1.0 and 1.0 */
  MYFLT rnd = (MYFLT) csound->Rand31(csound->RandSeed1(csound)) - FL(1073741823.5);
  return (rnd * (MYFLT) (1.0 / 1073741823.0));
}

/************************* MARACA *****************************/
#define MARA_SOUND_DECAY  FL(0.95)
#define MARA_SYSTEM_DECAY FL(0.999)
#define MARA_GAIN         FL(20.0)
#define MARA_NUM_BEANS    25
#define MARA_CENTER_FREQ  FL(3200.0)
#define MARA_RESON        FL(0.96)
/***********************  SEKERE *****************************/
#define SEKE_SOUND_DECAY  FL(0.96)
#define SEKE_SYSTEM_DECAY FL(0.999)
#define SEKE_GAIN         FL(20.0)
#define SEKE_NUM_BEANS    FL(64.0)
#define SEKE_CENTER_FREQ  FL(5500.0)
#define SEKE_RESON        FL(0.6)
/***********************  SANDPAPER **************************/
#define SANDPAPR_SOUND_DECAY FL(0.999)
#define SANDPAPR_SYSTEM_DECAY FL(0.999)
#define SANDPAPR_GAIN     FL(0.5)
#define SANDPAPR_NUM_GRAINS FL(128.0)
#define SANDPAPR_CENTER_FREQ FL(4500.0)
#define SANDPAPR_RESON    FL(0.6)
/*(********************** CABASA *****************************/
#define CABA_SOUND_DECAY  FL(0.96)
#define CABA_SYSTEM_DECAY FL(0.997)
#define CABA_GAIN         FL(40.0)
#define CABA_NUM_BEADS    512
#define CABA_CENTER_FREQ  FL(3000.0)
#define CABA_RESON        FL(0.7)
/************************ Bamboo Wind Chimes *****************/
#define BAMB_SOUND_DECAY  FL(0.95)
#define BAMB_SYSTEM_DECAY FL(0.9999)
#define BAMB_GAIN         FL(2.0)
#define BAMB_NUM_TUBES    FL(1.25)
#define BAMB_CENTER_FREQ0 FL(2800.0)
#define BAMB_CENTER_FREQ1 (FL(0.8) * FL(2800.0))
#define BAMB_CENTER_FREQ2 (FL(1.2) * FL(2800.0))
#define BAMB_RESON        FL(0.995)
/******************* Water Drops  ****************************/
#define WUTR_SOUND_DECAY  FL(0.95)
#define WUTR_SYSTEM_DECAY FL(0.996)
#define WUTR_GAIN         FL(1.0)
#define WUTR_NUM_SOURCES  FL(10.0)
#define WUTR_CENTER_FREQ0 FL(450.0)
#define WUTR_CENTER_FREQ1 FL(600.0)
#define WUTR_CENTER_FREQ2 FL(750.0)
#define WUTR_RESON        FL(0.9985)
#define WUTR_FREQ_SWEEP   FL(1.0001)
/****************** TAMBOURINE  *****************************/
#define TAMB_SOUND_DECAY  FL(0.95)
#define TAMB_SYSTEM_DECAY FL(0.9985)
#define TAMB_GAIN         FL(5.0)
#define TAMB_NUM_TIMBRELS 32
#define TAMB_SHELL_FREQ   FL(2300.0)
#define TAMB_SHELL_GAIN   FL(0.1)
#define TAMB_SHELL_RESON  FL(0.96)
#define TAMB_CYMB_FREQ1   FL(5600.0)
#define TAMB_CYMB_FREQ2   FL(8100.0)
#define TAMB_CYMB_RESON   FL(0.99)
/********************** SLEIGHBELLS *************************/
#define SLEI_SOUND_DECAY  FL(0.97)
#define SLEI_SYSTEM_DECAY FL(0.9994)
#define SLEI_GAIN         FL(1.0)
#define SLEI_NUM_BELLS    32
#define SLEI_CYMB_FREQ0   FL(2500.0)
#define SLEI_CYMB_FREQ1   FL(5300.0)
#define SLEI_CYMB_FREQ2   FL(6500.0)
#define SLEI_CYMB_FREQ3   FL(8300.0)
#define SLEI_CYMB_FREQ4   FL(9800.0)
#define SLEI_CYMB_RESON   FL(0.99)
/***************************  GUIRO  ***********************/
#define GUIR_SOUND_DECAY  FL(0.95)
#define GUIR_GAIN         FL(10.0)
#define GUIR_NUM_PARTS    128
#define GUIR_GOURD_FREQ   FL(2500.0)
#define GUIR_GOURD_RESON  FL(0.97)
#define GUIR_GOURD_FREQ2  FL(4000.0)
#define GUIR_GOURD_RESON2 FL(0.97)
/**************************  WRENCH  ***********************/
#define WRENCH_SOUND_DECAY FL(0.95)
#define WRENCH_GAIN       5
#define WRENCH_NUM_PARTS  FL(128.0)
#define WRENCH_FREQ       FL(3200.0)
#define WRENCH_RESON      FL(0.99)
#define WRENCH_FREQ2      FL(8000.)
#define WRENCH_RESON2     FL(0.992)
/************************ COKECAN **************************/
#define COKECAN_SOUND_DECAY FL(0.97)
#define COKECAN_SYSTEM_DECAY FL(0.999)
#define COKECAN_GAIN      FL(0.8)
#define COKECAN_NUM_PARTS 48
#define COKECAN_HELMFREQ  FL(370.0)
#define COKECAN_HELM_RES  FL(0.99)
#define COKECAN_METLFREQ0 FL(1025.0)
#define COKECAN_METLFREQ1 FL(1424.0)
#define COKECAN_METLFREQ2 FL(2149.0)
#define COKECAN_METLFREQ3 FL(3596.0)
#define COKECAN_METL_RES  FL(0.992)
/************************************************************/
/*  PhOLIES (Physically-Oriented Library of                 */
/*    Imitated Environmental Sounds), Perry Cook, 1997-8    */
/************************************************************/

/***********************  STIX1 *****************************/
#define STIX1_SOUND_DECAY FL(0.96)
#define STIX1_SYSTEM_DECAY FL(0.998)
#define STIX1_GAIN        FL(30.0)
#define STIX1_NUM_BEANS   FL(2.0)
#define STIX1_CENTER_FREQ FL(5500.0)
#define STIX1_RESON       FL(0.6)
/************************ Crunch1 ***************************/
#define CRUNCH1_SOUND_DECAY FL(0.95)
#define CRUNCH1_SYSTEM_DECAY FL(0.99806)
#define CRUNCH1_GAIN      FL(20.0)
#define CRUNCH1_NUM_BEADS 7
#define CRUNCH1_CENTER_FREQ FL(800.0)
#define CRUNCH1_RESON     FL(0.95)

#define MAX_SHAKE FL(2000.0)
#define MIN_ENERGY FL(0.0)          /* 0.1 or 0.3?? */

static int32_t cabasaset(CSOUND *csound, CABASA *p)
{
  p->sndLevel = FL(0.0);
  p->kloop = (int32_t)(p->h.insdshead->offtim * CS_EKR)
    - (int32_t)(CS_EKR * *p->dettack);
  p->outputs0 = FL(0.0);
  p->outputs1 = FL(0.0);
  p->shake_maxSave = FL(0.0);
  p->totalEnergy  = FL(0.0);
  p->num_objects = CABA_NUM_BEADS;
  p->soundDecay = CABA_SOUND_DECAY;
  p->systemDecay = CABA_SYSTEM_DECAY;
  p->gain = LOG((MYFLT)CABA_NUM_BEADS)*CABA_GAIN/(MYFLT)CABA_NUM_BEADS;
  p->resons = CABA_RESON;
  p->coeffs1 = CABA_RESON * CABA_RESON;
  p->coeffs0 = - CABA_RESON * FL(2.0) * COS(CABA_CENTER_FREQ * CS_TPIDSR);
  /* Note On */
  p->shakeEnergy = *p->amp * MAX_SHAKE * FL(0.1);
  if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  p->last_num = FL(0.0);
  return OK;
}

static int32_t cabasa(CSOUND *csound, CABASA *p)
{
  MYFLT *ar = p->ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT data;
  /* Use locals for speed */
  MYFLT shakeEnergy = p->shakeEnergy;
  MYFLT systemDecay = p->systemDecay;
  MYFLT sndLevel    = p->sndLevel;
  MYFLT soundDecay  = p->soundDecay;
  MYFLT input;
  MYFLT outputs0    = p->outputs0;
  MYFLT outputs1    = p->outputs1;
  MYFLT coeff0      = p->coeffs0;
  MYFLT coeff1      = p->coeffs1;
  MYFLT gain        = p->gain;

  if (*p->num_beads != p->last_num) { /* # beans has changed */
    p->last_num = *p->num_beads;
    if ((int32)(*p->num_beads+FL(0.5)) != p->num_objects) {
      p->num_objects = (int32)(*p->num_beads+FL(0.5));
      if (p->num_objects >= 1) {
        gain = p->gain = LOG((MYFLT)p->num_objects) /
          FL(1.38629436111989061883) /* (MYFLT)log(4.0)*/ * FL(40.0) /
          (MYFLT) p->num_objects;
      }
    }
  }

  if (*p->damp != FL(0.0)) {
    systemDecay = p->systemDecay = FL(0.998) + (*p->damp * FL(0.002));
  }

  if (*p->shake_max != FL(0.0)) {
    shakeEnergy = p->shakeEnergy +=
      CS_KSMPS * *p->shake_max * MAX_SHAKE * FL(0.1);
    if (shakeEnergy > MAX_SHAKE) shakeEnergy = MAX_SHAKE;
  }

  if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    shakeEnergy = FL(0.0);
  }

  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    /*        if (shakeEnergy > MIN_ENERGY) { */
    shakeEnergy *= systemDecay;               /* Exponential system decay */
    if (my_random(csound, 1024) < p->num_objects) {
      sndLevel += gain * shakeEnergy;
    }
    input = sndLevel * noise_tick(csound);    /* Actual Sound is Random */
    sndLevel *= soundDecay;                   /* Exponential Sound decay  */
    input -= outputs0*coeff0; /* Do */
    input -= outputs1*coeff1; /* resonant */
    outputs1 = outputs0;      /* filter */
    outputs0 = input;         /* calculations */
    data =  outputs0 - outputs1;
    /*          if (data > 10000.0f)        data = 10000.0f; */
    /*          if (data < -10000.0f) data = -10000.0f; */
    ar[n] = data * FL(0.0005) * csound->Get0dBFS(csound) ;
    /*        } */
    /*        else { */
    /*          *ar++ = 0.0f; */
    /*        } */
  }
  p->shakeEnergy = shakeEnergy;
  p->sndLevel = sndLevel;
  p->outputs0 = outputs0;
  p->outputs1 = outputs1;
  return OK;
}

static int32_t sekereset(CSOUND *csound, SEKERE *p)
{
  p->sndLevel = FL(0.0);
  p->kloop = (int32_t)(p->h.insdshead->offtim * CS_EKR)
    - (int32_t)(CS_EKR * *p->dettack);
  p->outputs0 = FL(0.0);
  p->outputs1 = FL(0.0);
  p->finalZ2 = FL(0.0);
  p->finalZ1 = FL(0.0);
  p->finalZ0 = FL(0.0);
  p->shake_maxSave = FL(0.0);
  p->totalEnergy  = FL(0.0);
  p->num_objects = SEKE_NUM_BEANS;
  p->soundDecay = SEKE_SOUND_DECAY;
  p->systemDecay = SEKE_SYSTEM_DECAY;
  p->gain = LOG(SEKE_NUM_BEANS)*SEKE_GAIN/SEKE_NUM_BEANS;
  p->resons = SEKE_RESON;
  p->coeffs1 = SEKE_RESON * SEKE_RESON;
  p->coeffs0 = - SEKE_RESON * FL(2.0) *
    COS(SEKE_CENTER_FREQ * CS_TPIDSR);
  /* Note On */
  p->shakeEnergy = *p->amp * MAX_SHAKE * FL(0.1);
  if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  p->last_num = FL(0.0);
  return OK;
}

static int32_t sekere(CSOUND *csound, SEKERE *p)
{
  MYFLT *ar = p->ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT data;
  /* Use locals for speed */
  MYFLT shakeEnergy = p->shakeEnergy;
  MYFLT systemDecay = p->systemDecay;
  MYFLT sndLevel = p->sndLevel;
  MYFLT soundDecay = p->soundDecay;
  MYFLT input;
  MYFLT outputs0 = p->outputs0;
  MYFLT outputs1 = p->outputs1;
  MYFLT coeff0   = p->coeffs0;
  MYFLT coeff1   = p->coeffs1;
  MYFLT gain     = p->gain;

  if (*p->num_beads != p->last_num) {
    p->last_num = *p->num_beads;
    if ((int32)(*p->num_beads+FL(0.5)) != p->num_objects) {
      p->num_objects = *p->num_beads;
      if (p->num_objects >= 1) {
        gain = p->gain = LOG((MYFLT)p->num_objects) /
          FL(1.38629436111989061883) /* (MYFLT)log(4.0)*/ * FL(120.0) /
          (MYFLT) p->num_objects;
      }
    }
  }

  if (*p->damp != FL(0.0)) {
    systemDecay = p->systemDecay = FL(0.998) + (*p->damp * FL(0.002));
  }

  if (*p->shake_max != FL(0.0)) {
    shakeEnergy = p->shakeEnergy +=
      CS_KSMPS * *p->shake_max * MAX_SHAKE * FL(0.1);
    if (shakeEnergy > MAX_SHAKE) shakeEnergy = MAX_SHAKE;
  }

  if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    shakeEnergy = FL(0.0);
  }

  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    /*        if (shakeEnergy > MIN_ENERGY) { */
    shakeEnergy *= systemDecay;           /* Exponential system decay */
    if (my_random(csound, 1024) < p->num_objects) {
      sndLevel += gain * shakeEnergy;
    }
    input = sndLevel * noise_tick(csound);  /* Actual Sound is Random */
    sndLevel *= soundDecay;               /* Exponential Sound decay  */
    input -= outputs0*coeff0;             /* Do */
    input -= outputs1*coeff1;             /* resonant */
    outputs1 = outputs0;                  /* filter */
    outputs0 = input;                     /* calculations */
    p->finalZ2 = p->finalZ1;
    p->finalZ1 = p->finalZ0;
    p->finalZ0 = p->outputs1;
    data = p->finalZ0 - p->finalZ2;
    /*          if (data > 10000.0f)        data = 10000.0f; */
    /*          if (data < -10000.0f) data = -10000.0f; */
    ar[n] = data * FL(0.0005) * csound->Get0dBFS(csound) ;
    /*        } */
    /*        else { */
    /*          ar[n] = 0.0f; */
    /*        } */
  }
  //printf("%d/%d:\n", offset, early);
  p->shakeEnergy = shakeEnergy;
  p->sndLevel = sndLevel;
  p->outputs0 = outputs0;
  p->outputs1 = outputs1;
  return OK;
}

static int32_t sandset(CSOUND *csound, SEKERE *p)
{
  p->sndLevel = FL(0.0);
  p->kloop = (int32_t)(p->h.insdshead->offtim * CS_EKR)
    - (int32_t)(CS_EKR * *p->dettack);
  p->outputs0 = FL(0.0);
  p->outputs1 = FL(0.0);
  p->finalZ2 = FL(0.0);
  p->finalZ1 = FL(0.0);
  p->finalZ0 = FL(0.0);
  p->shake_maxSave = FL(0.0);
  p->totalEnergy  = FL(0.0);
  p->num_objects = SANDPAPR_NUM_GRAINS;
  p->soundDecay = SANDPAPR_SOUND_DECAY;
  p->systemDecay = SANDPAPR_SYSTEM_DECAY;
  p->gain = LOG(SANDPAPR_NUM_GRAINS) *
    SANDPAPR_GAIN / SANDPAPR_NUM_GRAINS;
  p->resons = SANDPAPR_RESON;
  p->coeffs1 = SANDPAPR_RESON * SANDPAPR_RESON;
  p->coeffs0 = - SANDPAPR_RESON * FL(2.0) *
    COS(SANDPAPR_CENTER_FREQ * CS_TPIDSR);
  /* Note On */
  p->shakeEnergy = *p->amp * CS_ONEDDBFS * MAX_SHAKE * FL(0.1);
  if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  p->last_num = FL(128.0);
  return OK;
}

static int32_t stixset(CSOUND *csound, SEKERE *p)
{
  p->sndLevel = FL(0.0);
  p->kloop = (int32_t)(p->h.insdshead->offtim * CS_EKR)
    - (int32_t)(CS_EKR * *p->dettack);
  p->outputs0 = FL(0.0);
  p->outputs1 = FL(0.0);
  p->finalZ2 = FL(0.0);
  p->finalZ1 = FL(0.0);
  p->finalZ0 = FL(0.0);
  p->shake_maxSave = FL(0.0);
  p->totalEnergy  = FL(0.0);
  p->num_objects = STIX1_NUM_BEANS;
  p->soundDecay = STIX1_SOUND_DECAY;
  p->systemDecay = STIX1_SYSTEM_DECAY;
  p->gain = LOG(STIX1_NUM_BEANS) * STIX1_GAIN / STIX1_NUM_BEANS;
  p->resons = STIX1_RESON;
  p->coeffs1 = STIX1_RESON * STIX1_RESON;
  p->coeffs0 = - STIX1_RESON * FL(2.0) *
    COS(STIX1_CENTER_FREQ * CS_TPIDSR);
  /* Note On */
  p->shakeEnergy = *p->amp * CS_ONEDDBFS * MAX_SHAKE * FL(0.1);
  if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  p->last_num = FL(30.0);
  return OK;
}

static int32_t crunchset(CSOUND *csound, CABASA *p)
{
  p->sndLevel = FL(0.0);
  p->kloop = (int32_t)(p->h.insdshead->offtim * CS_EKR)
    - (int32_t)(CS_EKR * *p->dettack);
  p->outputs0 = FL(0.0);
  p->outputs1 = FL(0.0);
  p->shake_maxSave = FL(0.0);
  p->totalEnergy  = FL(0.0);
  p->num_objects = CRUNCH1_NUM_BEADS;
  p->soundDecay = CRUNCH1_SOUND_DECAY;
  p->systemDecay = CRUNCH1_SYSTEM_DECAY;
  p->gain = LOG((MYFLT)CRUNCH1_NUM_BEADS) *
    CRUNCH1_GAIN / (MYFLT) CRUNCH1_NUM_BEADS;
  p->resons = CRUNCH1_RESON;
  p->coeffs1 = CRUNCH1_RESON * CRUNCH1_RESON;
  p->coeffs0 = - CRUNCH1_RESON * FL(2.0) *
    COS(CRUNCH1_CENTER_FREQ * CS_TPIDSR);
  /* Note On */
  p->shakeEnergy = *p->amp * CS_ONEDDBFS * MAX_SHAKE * FL(0.1);
  if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  p->last_num = FL(0.0);
  return OK;
}

static int32_t guiroset(CSOUND *csound, GUIRO *p)
{
  MYFLT temp;

  p->res_freqSave = FL(0.0);
  p->shake_maxSave = FL(0.0);
  p->res_freq2 = FL(0.0);

  p->baseGain = FL(0.0);

  p->sndLevel = FL(0.0);
  p->kloop = (int32_t)(p->h.insdshead->offtim * CS_EKR)
    - (int32_t)(CS_EKR * *p->dettack);

  p->outputs00    = FL(0.0);
  p->outputs01    = FL(0.0);
  p->outputs10    = FL(0.0);
  p->outputs11    = FL(0.0);

  p->totalEnergy  = FL(0.0);
  p->ratchetDelta = FL(0.0005);
  p->finalZ0      = FL(0.0);
  p->finalZ1      = FL(0.0);
  p->finalZ2      = FL(0.0);

  p->num_objects = (MYFLT)GUIR_NUM_PARTS;
  p->soundDecay = GUIR_SOUND_DECAY;
  p->systemDecay = FL(1.0);
  temp = LOG((MYFLT)GUIR_NUM_PARTS) * GUIR_GAIN /
    (MYFLT) GUIR_NUM_PARTS;
  p->gains0=temp;
  p->gains1=temp;

  p->coeffs01 = GUIR_GOURD_RESON * GUIR_GOURD_RESON;
  p->coeffs00 = -GUIR_GOURD_RESON * FL(2.0) *
    COS(GUIR_GOURD_FREQ * CS_TPIDSR);

  p->coeffs11 = GUIR_GOURD_RESON2 * GUIR_GOURD_RESON2;
  p->coeffs10 = -GUIR_GOURD_RESON2 * FL(2.0) *
    COS(GUIR_GOURD_FREQ2 * CS_TPIDSR);

  p->ratchet = FL(0.0);
  p->ratchetPos = 10;
  /* Note On */
  p->shakeEnergy = MAX_SHAKE * FL(0.1);
  p->shake_damp = FL(0.0);
  if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  p->ratchetPos += 1;
  return OK;
}

static int32_t guiro(CSOUND *csound, GUIRO *p)
{
  MYFLT *ar = p->ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT lastOutput;

  if (*p->num_teeth != FL(0.0) &&
      (int32)(*p->num_teeth+FL(0.5)) != p->num_objects) {
    p->num_objects = *p->num_teeth;
    if (p->num_objects < FL(1.0)) p->num_objects = FL(1.0);
    p->gains0 = p->gains1 = LOG((MYFLT)p->num_objects) * GUIR_GAIN /
      (MYFLT) p->num_objects;
  }

  if (*p->damp != FL(0.0) && *p->damp != p->shake_damp) {
    p->shake_damp = *p->damp;
    /*        p->systemDecay = TAMB_SYSTEM_DECAY + (p->shake_damp * FL(0.002)); */
    /*        p->scrapeVel = p->shake_damp; */
  }
  if (*p->shake_max != FL(0.0) && *p->shake_max != p->shake_maxSave) {
    p->shake_maxSave = *p->shake_max;
    p->shakeEnergy += p->shake_maxSave * MAX_SHAKE * FL(0.1);
    if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  }
  if (*p->freq != FL(0.0) && *p->freq !=  p->res_freqSave) {
    p->res_freqSave = *p->freq;
    p->coeffs00 = -GUIR_GOURD_RESON * FL(2.0) *
      COS(p->res_freqSave * CS_TPIDSR);
  }
  if (*p->freq2 != p->res_freq2) {
    p->res_freq2 = *p->freq2;
    p->coeffs10 = -GUIR_GOURD_RESON2 * FL(2.0) *
      COS(p->res_freq2 * CS_TPIDSR);
  }
  if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    p->shakeEnergy = FL(0.0);
    p->ratchetPos = 0;
  }

  {
    MYFLT sndLevel     = p->sndLevel;
    MYFLT ratchet      = p->ratchet;
    int32_t ratchetPos     = p->ratchetPos;
    MYFLT totalEnergy  = p->totalEnergy;
    MYFLT num_objects  = p->num_objects;
    MYFLT soundDecay   = p->soundDecay;
    MYFLT ratchetDelta = p->ratchetDelta;
    MYFLT inputs0, inputs1;
    MYFLT outputs00    = p->outputs00;
    MYFLT outputs01    = p->outputs01;
    MYFLT outputs10    = p->outputs10;
    MYFLT outputs11    = p->outputs11;
    MYFLT coeffs00     = p->coeffs00;
    MYFLT coeffs01     = p->coeffs01;
    MYFLT coeffs10     = p->coeffs10;
    MYFLT coeffs11     = p->coeffs11;
    MYFLT finalZ0      = p->finalZ0;
    MYFLT finalZ1      = p->finalZ1;
    MYFLT finalZ2      = p->finalZ2;
    MYFLT gains0       = p->gains0;
    MYFLT gains1       = p->gains1;
    MYFLT amp          = *p->amp*csound->Get0dBFS(csound);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      if (ratchetPos > 0) {
        ratchet -= (ratchetDelta + (FL(0.002)*totalEnergy));
        if (ratchet < FL(0.0)) {
          ratchet = FL(1.0);
          ratchetPos -= 1;
        }
        totalEnergy = ratchet;
        if (my_random(csound, 1024) < num_objects) {
          sndLevel += FL(512.0) * ratchet * totalEnergy;
        }
        inputs0     = sndLevel;
        inputs0    *= noise_tick(csound) * ratchet;
        sndLevel   *= soundDecay;

        inputs1     = inputs0;
        inputs0    -= outputs00*coeffs00;
        inputs0    -= outputs01*coeffs01;
        outputs01   = outputs00;
        outputs00   = inputs0;
        inputs1    -= outputs10*coeffs10;
        inputs1    -= outputs11*coeffs11;
        outputs11   = outputs10;
        outputs10   = inputs1;

        finalZ2     = finalZ1;
        finalZ1     = finalZ0;
        finalZ0     = gains0*outputs01 + gains1*outputs11;
        lastOutput  = finalZ0 - finalZ2;
        lastOutput *= FL(0.0001);
      }
      else
        lastOutput = FL(0.0);
      ar[n] = FL(1.33)*lastOutput*amp;
    }
    p->sndLevel    = sndLevel;
    p->ratchet     = ratchet;
    p->ratchetPos  = ratchetPos;
    p->totalEnergy = totalEnergy;
    p->outputs00   = outputs00;
    p->outputs01   = outputs01;
    p->outputs10   = outputs10;
    p->outputs11   = outputs11;
    p->finalZ0     = finalZ0;
    p->finalZ1     = finalZ1;
    p->finalZ2     = finalZ2;

  }
  return OK;
}

static int32_t tambourset(CSOUND *csound, TAMBOURINE *p)
{
  MYFLT temp;

  p->shake_maxSave = FL(0.0);
  p->sndLevel = FL(0.0);
  p->kloop = (int32_t)(p->h.insdshead->offtim * CS_EKR)
    - (int32_t)(CS_EKR * *p->dettack);

  p->outputs00       = FL(0.0);
  p->outputs01       = FL(0.0);
  p->outputs10       = FL(0.0);
  p->outputs11       = FL(0.0);
  p->outputs20       = FL(0.0);
  p->outputs21       = FL(0.0);

  p->totalEnergy     = FL(0.0);
  p->finalZ0         = FL(0.0);
  p->finalZ1         = FL(0.0);
  p->finalZ2         = FL(0.0);

  p->num_objectsSave = p->num_objects = (MYFLT)TAMB_NUM_TIMBRELS;
  p->soundDecay      = TAMB_SOUND_DECAY;
  p->systemDecay     = TAMB_SYSTEM_DECAY;
  p->gain            = FL(24.0) / TAMB_NUM_TIMBRELS;
  p->res_freq1       = TAMB_CYMB_FREQ1;
  p->res_freq2       = TAMB_CYMB_FREQ2;
  temp               = LOG((MYFLT)TAMB_NUM_TIMBRELS) * TAMB_GAIN /
    (MYFLT) TAMB_NUM_TIMBRELS;
  p->gains0          = temp*TAMB_SHELL_GAIN;
  p->gains1          = temp*FL(0.8);
  p->gains2          = temp;
  p->coeffs01        = TAMB_SHELL_RESON * TAMB_SHELL_RESON;
  p->coeffs00        = -TAMB_SHELL_RESON * FL(2.0) *
    COS(TAMB_SHELL_FREQ * CS_TPIDSR);
  p->coeffs11        = TAMB_CYMB_RESON * TAMB_CYMB_RESON;
  p->coeffs10        = -TAMB_CYMB_RESON * FL(2.0) *
    COS(TAMB_CYMB_FREQ1 * CS_TPIDSR);
  p->coeffs21        = TAMB_CYMB_RESON * TAMB_CYMB_RESON;
  p->coeffs20        = -TAMB_CYMB_RESON * FL(2.0) *
    COS(TAMB_CYMB_FREQ2 * CS_TPIDSR);
  /* Note On */
  p->shakeEnergy = *p->amp * CS_ONEDDBFS * MAX_SHAKE * FL(0.1);
  p->shake_damp = FL(0.0);
  if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  return OK;
}

static int32_t tambourine(CSOUND *csound, TAMBOURINE *p)
{
  MYFLT *ar = p->ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT data;
  MYFLT temp_rand;
  MYFLT lastOutput;

  if (*p->num_timbrels != FL(0.0) && *p->num_timbrels != p->num_objects) {
    p->num_objects = *p->num_timbrels;
    if (p->num_objects < FL(1.0)) p->num_objects = FL(1.0);
    p->gain = FL(24.0) / p->num_objects;
  }
  if (*p->freq != FL(0.0) && *p->freq != p->res_freq) {
    p->res_freq = *p->freq;
    p->coeffs00 = -TAMB_SHELL_RESON * FL(2.0) *
      COS(p->res_freq * CS_TPIDSR);
  }
  if (*p->damp != FL(0.0) && *p->damp != p->shake_damp) {
    p->shake_damp = *p->damp;
    p->systemDecay = TAMB_SYSTEM_DECAY + (p->shake_damp * FL(0.002));
  }
  if (*p->shake_max != FL(0.0) && *p->shake_max != p->shake_maxSave) {
    p->shake_maxSave = *p->shake_max;
    p->shakeEnergy += p->shake_maxSave * MAX_SHAKE * FL(0.1);
    if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  }
  if (*p->freq1 != FL(0.0) && *p->freq1 != p->res_freq1) {
    p->res_freq1 = *p->freq1;
    p->coeffs10 = -TAMB_CYMB_RESON * FL(2.0) *
      COS(p->res_freq1 * CS_TPIDSR);
  }
  if (*p->freq2 != FL(0.0) && *p->freq2 != p->res_freq2) {
    p->res_freq2 = *p->freq2;
    p->coeffs20 = -TAMB_CYMB_RESON * FL(2.0) *
      COS(p->res_freq2 * CS_TPIDSR);
  }
  if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    p->shakeEnergy = FL(0.0);
  }

  {
    MYFLT shakeEnergy = p->shakeEnergy;
    MYFLT systemDecay = p->systemDecay;
    MYFLT sndLevel = p->sndLevel;
    MYFLT soundDecay = p->soundDecay;
    MYFLT inputs0, inputs1, inputs2;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      shakeEnergy *= systemDecay; /* Exponential system decay */
      if (my_random(csound, 1024) < p->num_objects) {
        sndLevel += p->gain * shakeEnergy;
        temp_rand = p->res_freq1 * (FL(1.0) + (FL(0.05)*noise_tick(csound)));
        p->coeffs10 = -TAMB_CYMB_RESON * FL(2.0) *
          COS(temp_rand * CS_TPIDSR);
        temp_rand = p->res_freq2 * (FL(1.0) + (FL(0.05)*noise_tick(csound)));
        p->coeffs20 = -TAMB_CYMB_RESON * FL(2.0) *
          COS(temp_rand * CS_TPIDSR);
      }
      inputs0 = sndLevel * noise_tick(csound);  /* Actual Sound is Random */
      inputs1 = inputs0;
      inputs2 = inputs0;
      sndLevel *= soundDecay;                 /* Exponential Sound decay  */
      p->finalZ2 = p->finalZ1;
      p->finalZ1 = p->finalZ0;
      p->finalZ0 = FL(0.0);
      inputs0 -= p->outputs00*p->coeffs00;    /* Do */
      inputs0 -= p->outputs01*p->coeffs01;    /* resonant */
      p->outputs01 = p->outputs00;            /* filter */
      p->outputs00 = inputs0;                 /* calculations */
      p->finalZ0 += p->gains0 * p->outputs01;
      inputs1 -= p->outputs10*p->coeffs10;    /* Do */
      inputs1 -= p->outputs11*p->coeffs11;    /* resonant */
      p->outputs11 = p->outputs10;            /* filter */
      p->outputs10 = inputs1;                 /* calculations */
      p->finalZ0 += p->gains1 * p->outputs11;
      inputs2 -= p->outputs20*p->coeffs20;    /* Do */
      inputs2 -= p->outputs21*p->coeffs21;    /* resonant */
      p->outputs21 = p->outputs20;            /* filter */
      p->outputs20 = inputs2;                 /* calculations */
      p->finalZ0 += p->gains2 * p->outputs21;
      data = p->finalZ0 - p->finalZ2;         /* Extra zero(s) for shape */
      lastOutput = data * FL(0.0009);
      ar[n] = lastOutput*csound->Get0dBFS(csound);
    }
    p->shakeEnergy = shakeEnergy;
    p->sndLevel = sndLevel;
  }
  return OK;
}

static int32_t bambooset(CSOUND *csound, BAMBOO *p)
{
  MYFLT temp;
  p->shake_maxSave = FL(0.0);

  p->sndLevel = FL(0.0);
  p->kloop = (int32_t)(p->h.insdshead->offtim * CS_EKR)
    - (int32_t)(CS_EKR * *p->dettack);

  p->outputs00       = FL(0.0);
  p->outputs01       = FL(0.0);
  p->outputs10       = FL(0.0);
  p->outputs11       = FL(0.0);
  p->outputs20       = FL(0.0);
  p->outputs21       = FL(0.0);

  p->totalEnergy     = FL(0.0);

  p->res_freq0       = BAMB_CENTER_FREQ0;
  p->res_freq1       = BAMB_CENTER_FREQ1;
  p->res_freq2       = BAMB_CENTER_FREQ2;
  p->num_objectsSave = p->num_objects = BAMB_NUM_TUBES;
  p->soundDecay      = BAMB_SOUND_DECAY;
  p->systemDecay     = BAMB_SYSTEM_DECAY;
  temp               = LOG((MYFLT)BAMB_NUM_TUBES) * BAMB_GAIN /
    (MYFLT) BAMB_NUM_TUBES;
  p->gain            = temp;
  p->coeffs01        = BAMB_RESON * BAMB_RESON;
  p->coeffs00        = -BAMB_RESON * FL(2.0) *
    COS(BAMB_CENTER_FREQ0 * CS_TPIDSR);
  p->coeffs11        = BAMB_RESON * BAMB_RESON;
  p->coeffs10        = -BAMB_RESON * FL(2.0) *
    COS(BAMB_CENTER_FREQ1 * CS_TPIDSR);
  p->coeffs21        = BAMB_RESON * BAMB_RESON;
  p->coeffs20        = -BAMB_RESON * FL(2.0) *
    COS(BAMB_CENTER_FREQ2 * CS_TPIDSR);
  /* Note On */
  p->shakeEnergy     = *p->amp * CS_ONEDDBFS * MAX_SHAKE * FL(0.1);
  p->shake_damp      = FL(0.0);
  if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  return OK;
}

static int32_t bamboo(CSOUND *csound, BAMBOO *p)
{
  MYFLT *ar = p->ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT data;
  MYFLT temp_rand;
  MYFLT lastOutput;

  if (*p->num_tubes != FL(0.0) && *p->num_tubes != p->num_objects) {
    p->num_objects = *p->num_tubes;
    if (p->num_objects < FL(1.0)) p->num_objects = FL(1.0);
  }
  if (*p->freq != FL(0.0) && *p->freq != p->res_freq0) {
    p->res_freq0 = *p->freq;
    p->coeffs00 = -BAMB_RESON * FL(2.0) *
      COS(p->res_freq0 * CS_TPIDSR);
  }
  if (*p->damp != FL(0.0) && *p->damp != p->shake_damp) {
    p->shake_damp = *p->damp;
    p->systemDecay = BAMB_SYSTEM_DECAY + (p->shake_damp * FL(0.002));
  }
  if (*p->shake_max != FL(0.0) && *p->shake_max != p->shake_maxSave) {
    p->shake_maxSave = *p->shake_max;
    p->shakeEnergy += p->shake_maxSave * MAX_SHAKE * FL(0.1);
    if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  }
  if (*p->freq1 != FL(0.0) && *p->freq1 != p->res_freq1) {
    p->res_freq1 = *p->freq1;
    p->coeffs10 = -BAMB_RESON * FL(2.0) *
      COS(p->res_freq1 * CS_TPIDSR);
  }
  if (*p->freq2 != FL(0.0) && *p->freq2 != p->res_freq2) {
    p->res_freq2 = *p->freq2;
    p->coeffs20 = -BAMB_RESON * FL(2.0) *
      COS(p->res_freq2 * CS_TPIDSR);
  }
  if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    p->shakeEnergy = FL(0.0);
  }

  {
    MYFLT shakeEnergy = p->shakeEnergy;
    MYFLT systemDecay = p->systemDecay;
    MYFLT sndLevel    = p->sndLevel;
    MYFLT soundDecay  = p->soundDecay;
    MYFLT inputs0, inputs1, inputs2;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      shakeEnergy *= systemDecay; /* Exponential system decay */
      if (my_random(csound, 1024) < p->num_objects) {
        sndLevel += shakeEnergy;
        temp_rand = p->res_freq0 * (FL(1.0) + (FL(0.2) * noise_tick(csound)));
        p->coeffs00 = -BAMB_RESON * FL(2.0) *
          COS(temp_rand * CS_TPIDSR);
        temp_rand = p->res_freq1 * (FL(1.0) +
                                    (FL(0.2) * noise_tick(csound)));
        p->coeffs10 = -BAMB_RESON * FL(2.0) *
          COS(temp_rand * CS_TPIDSR);
        temp_rand = p->res_freq2 * (FL(1.0) +
                                    (FL(0.2) * noise_tick(csound)));
        p->coeffs20 = -BAMB_RESON * FL(2.0) *
          COS(temp_rand * CS_TPIDSR);
      }
      inputs0 = sndLevel * noise_tick(csound); /* Actual Sound is Random */
      inputs1      = inputs0;
      inputs2      = inputs0;
      sndLevel    *= soundDecay;               /* Exponential Sound decay  */
      inputs0     -= p->outputs00*p->coeffs00; /* Do */
      inputs0     -= p->outputs01*p->coeffs01; /* resonant */
      p->outputs01 = p->outputs00;             /* filter */
      p->outputs00 = inputs0;                  /* calculations */
      data         = p->gain * p->outputs01;
      inputs1     -= p->outputs10*p->coeffs10; /* Do */
      inputs1     -= p->outputs11*p->coeffs11; /* resonant */
      p->outputs11 = p->outputs10;             /* filter */
      p->outputs10 = inputs1;                  /* calculations */
      data        += p->gain * p->outputs11;
      inputs2     -= p->outputs20*p->coeffs20; /* Do */
      inputs2     -= p->outputs21*p->coeffs21; /* resonant */
      p->outputs21 = p->outputs20;             /* filter */
      p->outputs20 = inputs2;                  /* calculations */
      data        += p->gain * p->outputs21;
      /*            if (data > 10000.0f)      data = 10000.0f; */
      /*            if (data < -10000.0f) data = -10000.0f; */
      lastOutput   = data * FL(0.00051);
      ar[n]        = lastOutput*csound->Get0dBFS(csound);
    }
    p->shakeEnergy = shakeEnergy;
    p->sndLevel    = sndLevel;
  }
  return OK;
}

static int32_t wuterset(CSOUND *csound, WUTER *p)
{
  MYFLT temp;

  p->sndLevel = FL(0.0);
  p->kloop = (int32_t)(p->h.insdshead->offtim * CS_EKR)
    - (int32_t)(CS_EKR * *p->dettack);

  p->outputs00       = FL(0.0);
  p->outputs01       = FL(0.0);
  p->outputs10       = FL(0.0);
  p->outputs11       = FL(0.0);
  p->outputs20       = FL(0.0);
  p->outputs21       = FL(0.0);

  p->totalEnergy     = FL(0.0);

  p->center_freqs0   = p->res_freq0 = WUTR_CENTER_FREQ0;
  p->center_freqs1   = p->res_freq1 = WUTR_CENTER_FREQ1;
  p->center_freqs2   = p->res_freq2 = WUTR_CENTER_FREQ2;
  p->num_objectsSave = p->num_objects = WUTR_NUM_SOURCES;
  p->soundDecay      = WUTR_SOUND_DECAY;
  p->systemDecay     = WUTR_SYSTEM_DECAY;
  temp               = LOG(WUTR_NUM_SOURCES) * WUTR_GAIN / WUTR_NUM_SOURCES;
  p->gains0          = p->gains1 = p->gains2 = temp;
  p->coeffs01        = WUTR_RESON * WUTR_RESON;
  p->coeffs00        = -WUTR_RESON * FL(2.0) *
    COS(WUTR_CENTER_FREQ0 * CS_TPIDSR);
  p->coeffs11        = WUTR_RESON * WUTR_RESON;
  p->coeffs10        = -WUTR_RESON * FL(2.0) *
    COS(WUTR_CENTER_FREQ1 * CS_TPIDSR);
  p->coeffs21        = WUTR_RESON * WUTR_RESON;
  p->coeffs20        = -WUTR_RESON * FL(2.0) *
    COS(WUTR_CENTER_FREQ2 * CS_TPIDSR);
  /* Note On */
  p->shakeEnergy     = *p->amp * CS_ONEDDBFS * MAX_SHAKE * FL(0.1);
  p->shake_damp      = FL(0.0);
  if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  p->shake_maxSave = FL(0.0);
  p->num_objects = 10;        /* Bug fix by JPff 2014/08/27 */
  p->finalZ0 = p->finalZ1 = p->finalZ2 = FL(0.0);
  return OK;
}

static int32_t wuter(CSOUND *csound, WUTER *p)
{
  MYFLT *ar = p->ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT data;
  MYFLT lastOutput;

  if (*p->num_tubes != FL(0.0) && *p->num_tubes != p->num_objects) {
    p->num_objects = *p->num_tubes;
    if (p->num_objects < FL(1.0)) p->num_objects = FL(1.0);
  }
  if (*p->freq != FL(0.0) && *p->freq != p->res_freq0) {
    p->res_freq0 = *p->freq;
    p->coeffs00 = -WUTR_RESON * FL(2.0) *
      COS(p->res_freq0 * CS_TPIDSR);
  }
  if (*p->damp != FL(0.0) && *p->damp != p->shake_damp) {
    p->shake_damp = *p->damp;
    p->systemDecay = WUTR_SYSTEM_DECAY + (p->shake_damp * FL(0.002));
  }
  if (*p->shake_max != FL(0.0) && *p->shake_max != p->shake_maxSave) {
    p->shake_maxSave = *p->shake_max;
    p->shakeEnergy += p->shake_maxSave * MAX_SHAKE * FL(0.1);
    if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  }
  if (*p->freq1 != FL(0.0) && *p->freq1 != p->res_freq1) {
    p->res_freq1 = *p->freq1;
    p->coeffs10 = -WUTR_RESON * FL(2.0) *
      COS(p->res_freq1 * CS_TPIDSR);
  }
  if (*p->freq2 != FL(0.0) && *p->freq2 != p->res_freq2) {
    p->res_freq2 = *p->freq2;
    p->coeffs20 = -WUTR_RESON * FL(2.0) *
      COS(p->res_freq2 * CS_TPIDSR);
  }
  //if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    p->shakeEnergy = FL(0.0);
  }

  {
    MYFLT shakeEnergy = p->shakeEnergy;
    MYFLT systemDecay = p->systemDecay;
    MYFLT sndLevel = p->sndLevel;
    MYFLT num_objects = p->num_objects;
    MYFLT soundDecay = p->soundDecay;
    MYFLT inputs0, inputs1, inputs2;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {

      shakeEnergy *= systemDecay;               /* Exponential system decay */
      if (my_random(csound, 32767) < num_objects) {
        int32_t j;
        sndLevel = shakeEnergy;
        j = my_random(csound, 3);
        /* ******** Stange that there is no use of freq0 and freq2  */
        if (j == 0)   {
          p->center_freqs0 = p->res_freq1 *
            (FL(0.75) + (FL(0.25) * noise_tick(csound)));
          p->gains0 = FABS(noise_tick(csound));
        }
        else if (j == 1)      {
          p->center_freqs1 = p->res_freq1 *
            (FL(1.0) + (FL(0.25) * noise_tick(csound)));
          p->gains1 = FABS(noise_tick(csound));
        }
        else  {
          p->center_freqs2 = p->res_freq1 *
            (FL(1.25) + (FL(0.25) * noise_tick(csound)));
          p->gains2 = FABS(noise_tick(csound));
        }
      }

      p->gains0 *= WUTR_RESON;
      if (p->gains0 >  FL(0.001)) {
        p->center_freqs0  *= WUTR_FREQ_SWEEP;
        p->coeffs00 = -WUTR_RESON * FL(2.0) *
          COS(p->center_freqs0 * CS_TPIDSR);
      }
      p->gains1 *= WUTR_RESON;
      if (p->gains1 > FL(0.001)) {
        p->center_freqs1 *= WUTR_FREQ_SWEEP;
        p->coeffs10 = -WUTR_RESON * FL(2.0) *
          COS(p->center_freqs1 * CS_TPIDSR);
      }
      p->gains2 *= WUTR_RESON;
      if (p->gains2 > FL(0.001)) {
        p->center_freqs2 *= WUTR_FREQ_SWEEP;
        p->coeffs20 = -WUTR_RESON * FL(2.0) *
          COS(p->center_freqs2 * CS_TPIDSR);
      }

      sndLevel    *= soundDecay;          /* Each (all) event(s)  */
      /* decay(s) exponentially  */
      inputs0      = sndLevel;
      inputs0     *= noise_tick(csound);  /* Actual Sound is Random */
      inputs1      = inputs0 * p->gains1;
      inputs2      = inputs0 * p->gains2;
      inputs0     *= p->gains0;
      inputs0     -= p->outputs00*p->coeffs00;
      inputs0     -= p->outputs01*p->coeffs01;
      p->outputs01 = p->outputs00;
      p->outputs00 = inputs0;
      data         = p->gains0*p->outputs00;
      inputs1     -= p->outputs10*p->coeffs10;
      inputs1     -= p->outputs11*p->coeffs11;
      p->outputs11 = p->outputs10;
      p->outputs10 = inputs1;
      data        += p->gains1*p->outputs10;
      inputs2     -= p->outputs20*p->coeffs20;
      inputs2     -= p->outputs21*p->coeffs21;
      p->outputs21 = p->outputs20;
      p->outputs20 = inputs2;
      data        += p->gains2*p->outputs20;

      p->finalZ2   = p->finalZ1;
      p->finalZ1   = p->finalZ0;
      p->finalZ0   = data * FL(4.0);

      lastOutput   = p->finalZ2 - p->finalZ0;
      lastOutput  *= FL(0.005);
      ar[n]        = lastOutput*csound->Get0dBFS(csound);
    }
    p->shakeEnergy = shakeEnergy;
    p->sndLevel = sndLevel;
  }
  return OK;
}

static int32_t sleighset(CSOUND *csound, SLEIGHBELLS *p)
{
  MYFLT temp;

  p->shake_maxSave = FL(0.0);
  p->sndLevel = FL(0.0);
  p->kloop = (int32_t)(p->h.insdshead->offtim * CS_EKR)
    - (int32_t)(CS_EKR * *p->dettack);

  p->outputs00 = FL(0.0);
  p->outputs01 = FL(0.0);
  p->outputs10 = FL(0.0);
  p->outputs11 = FL(0.0);
  p->outputs20 = FL(0.0);
  p->outputs21 = FL(0.0);
  p->outputs30 = FL(0.0);
  p->outputs31 = FL(0.0);
  p->outputs40 = FL(0.0);
  p->outputs41 = FL(0.0);
  p->totalEnergy  = FL(0.0);

  p->res_freq0 = SLEI_CYMB_FREQ0;
  p->res_freq1 = SLEI_CYMB_FREQ1;
  p->res_freq2 = SLEI_CYMB_FREQ2;
  p->res_freq3 = SLEI_CYMB_FREQ3;
  p->res_freq4 = SLEI_CYMB_FREQ4;
  p->num_objectsSave = p->num_objects = (MYFLT)SLEI_NUM_BELLS;
  p->soundDecay      = SLEI_SOUND_DECAY;
  p->systemDecay     = SLEI_SYSTEM_DECAY;
  temp               = LOG((MYFLT)SLEI_NUM_BELLS)/(MYFLT)SLEI_NUM_BELLS;
  p->gain            = temp;
  p->coeffs01        = SLEI_CYMB_RESON * SLEI_CYMB_RESON;
  p->coeffs00        = -SLEI_CYMB_RESON * FL(2.0) *
    COS(SLEI_CYMB_FREQ0 * CS_TPIDSR);
  p->coeffs11        = SLEI_CYMB_RESON * SLEI_CYMB_RESON;
  p->coeffs10        = -SLEI_CYMB_RESON * FL(2.0) *
    COS(SLEI_CYMB_FREQ1 * CS_TPIDSR);
  p->coeffs21        = SLEI_CYMB_RESON * SLEI_CYMB_RESON;
  p->coeffs20        = -SLEI_CYMB_RESON * FL(2.0) *
    COS(SLEI_CYMB_FREQ2 * CS_TPIDSR);
  p->coeffs31        = SLEI_CYMB_RESON * SLEI_CYMB_RESON;
  p->coeffs30        = -SLEI_CYMB_RESON * FL(2.0) *
    COS(SLEI_CYMB_FREQ3 * CS_TPIDSR);
  p->coeffs41        = SLEI_CYMB_RESON * SLEI_CYMB_RESON;
  p->coeffs40        = -SLEI_CYMB_RESON * FL(2.0) *
    COS(SLEI_CYMB_FREQ4 * CS_TPIDSR);
  /* Note On */
  p->shakeEnergy = *p->amp * CS_ONEDDBFS * MAX_SHAKE * FL(0.1);
  p->shake_damp = FL(0.0);
  if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  return OK;
}

static int32_t sleighbells(CSOUND *csound, SLEIGHBELLS *p)
{
  MYFLT *ar = p->ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT data;
  MYFLT temp_rand;
  MYFLT lastOutput;

  if (*p->num_bells != FL(0.0) && *p->num_bells != p->num_objects) {
    p->num_objects = *p->num_bells;
    if (p->num_objects < FL(1.0)) p->num_objects = FL(1.0);
  }
  if (*p->freq != FL(0.0) && *p->freq != p->res_freq0) {
    p->res_freq0 = *p->freq;
    p->coeffs00 = -SLEI_CYMB_RESON * FL(2.0) *
      COS(p->res_freq0 * CS_TPIDSR);
  }
  if (*p->damp != FL(0.0) && *p->damp != p->shake_damp) {
    p->shake_damp = *p->damp;
    p->systemDecay = SLEI_SYSTEM_DECAY + (p->shake_damp * FL(0.002));
  }
  if (*p->shake_max != FL(0.0) && *p->shake_max != p->shake_maxSave) {
    p->shake_maxSave = *p->shake_max;
    p->shakeEnergy += p->shake_maxSave * MAX_SHAKE * FL(0.1);
    if (p->shakeEnergy > MAX_SHAKE) p->shakeEnergy = MAX_SHAKE;
  }
  if (*p->freq1 != FL(0.0) && *p->freq1 != p->res_freq1) {
    p->res_freq1 = *p->freq1;
    p->coeffs10 = -SLEI_CYMB_RESON * FL(2.0) *
      COS(p->res_freq1 * CS_TPIDSR);
  }
  if (*p->freq2 != FL(0.0) && *p->freq2 != p->res_freq2) {
    p->res_freq2 = *p->freq2;
    p->coeffs20 = -SLEI_CYMB_RESON * FL(2.0) *
      COS(p->res_freq2 * CS_TPIDSR);
  }
  if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
  if ((--p->kloop) == 0) {
    p->shakeEnergy = FL(0.0);
  }

  {
    MYFLT shakeEnergy = p->shakeEnergy;
    MYFLT systemDecay = p->systemDecay;
    MYFLT sndLevel = p->sndLevel;
    MYFLT soundDecay = p->soundDecay;
    MYFLT inputs0, inputs1, inputs2, inputs3, inputs4;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      shakeEnergy *= systemDecay; /* Exponential system decay */
      if (my_random(csound, 1024) < p->num_objects) {
        sndLevel += p->gain * shakeEnergy;
        temp_rand = p->res_freq0 * (FL(1.0) + (FL(0.03)*noise_tick(csound)));
        p->coeffs00 = -SLEI_CYMB_RESON * FL(2.0) *
          COS(temp_rand * CS_TPIDSR);
        temp_rand = p->res_freq1 * (FL(1.0) + (FL(0.03)*noise_tick(csound)));
        p->coeffs10 = -SLEI_CYMB_RESON * FL(2.0) *
          COS(temp_rand * CS_TPIDSR);
        temp_rand = p->res_freq2 * (FL(1.0) + (FL(0.03)*noise_tick(csound)));
        p->coeffs20 = -SLEI_CYMB_RESON * FL(2.0) *
          COS(temp_rand * CS_TPIDSR);
        temp_rand = p->res_freq3 * (FL(1.0) + (FL(0.03)*noise_tick(csound)));
        p->coeffs30 = -SLEI_CYMB_RESON * FL(2.0) *
          COS(temp_rand * CS_TPIDSR);
        temp_rand = p->res_freq4 * (FL(1.0) + (FL(0.03)*noise_tick(csound)));
        p->coeffs40 = -SLEI_CYMB_RESON * FL(2.0) *
          COS(temp_rand * CS_TPIDSR);
      }
      inputs0 = sndLevel * noise_tick(csound);  /* Actual Sound is Random */
      inputs1      = inputs0;
      inputs2      = inputs0;
      inputs3      = inputs0 * FL(0.5);
      inputs4      = inputs0 * FL(0.3);
      sndLevel    *= soundDecay;                /* ExponentialSound decay */
      inputs0     -= p->outputs00*p->coeffs00;  /* Do */
      inputs0     -= p->outputs01*p->coeffs01;  /* resonant */
      p->outputs01 = p->outputs00;              /* filter */
      p->outputs00 = inputs0;                   /* calculations */
      data         = p->outputs01;
      inputs1     -= p->outputs10*p->coeffs10;    /* Do */
      inputs1     -= p->outputs11*p->coeffs11;    /* resonant */
      p->outputs11 = p->outputs10;                /* filter */
      p->outputs10 = inputs1;                     /* calculations */
      data        += p->outputs11;
      inputs2     -= p->outputs20*p->coeffs20;    /* Do */
      inputs2     -= p->outputs21*p->coeffs21;    /* resonant */
      p->outputs21 = p->outputs20;                /* filter */
      p->outputs20 = inputs2;                     /* calculations */
      data        += p->outputs21;
      inputs3     -= p->outputs30*p->coeffs30;    /* Do */
      inputs3     -= p->outputs31*p->coeffs31;    /* resonant */
      p->outputs31 = p->outputs30;                /* filter */
      p->outputs30 = inputs3;                     /* calculations */
      data        += p->outputs31;
      inputs4     -= p->outputs40*p->coeffs40;    /* Do */
      inputs4     -= p->outputs41*p->coeffs41;    /* resonant */
      p->outputs41 = p->outputs40;                /* filter */
      p->outputs40 = inputs4;                     /* calculations */
      data        += p->outputs41;
      p->finalZ2   = p->finalZ1;
      p->finalZ1   = p->finalZ0;
      p->finalZ0   = data;
      data         = p->finalZ2 - p->finalZ0;
      lastOutput   = data * FL(0.001);
      ar[n]        = lastOutput*csound->Get0dBFS(csound);
    }
    p->shakeEnergy = shakeEnergy;
    p->sndLevel = sndLevel;
  }
  return OK;
}

#define S(x)    sizeof(x)

static OENTRY phisem_localops[] =
  {
   { "cabasa",  S(CABASA),  0, "a", "iiooo",  (SUBR)cabasaset, (SUBR)cabasa},
   { "crunch",  S(CABASA),  0, "a", "iiooo",  (SUBR)crunchset, (SUBR)cabasa},
   { "sekere",  S(SEKERE),  0, "a", "iiooo",  (SUBR)sekereset, (SUBR)sekere},
   { "sandpaper", S(SEKERE),0, "a", "iiooo",  (SUBR)sandset,   (SUBR)sekere},
   { "stix", S(SEKERE),     0, "a", "iiooo",  (SUBR)stixset,   (SUBR)sekere},
   { "guiro", S(GUIRO),     0, "a", "kiooooo",(SUBR)guiroset,  (SUBR)guiro },
   { "tambourine", S(TAMBOURINE),0,"a", "kioooooo",
                                        (SUBR)tambourset, (SUBR)tambourine},
   { "bamboo", S(BAMBOO),   0, "a", "kioooooo",
                                        (SUBR)bambooset, (SUBR)bamboo },
   { "dripwater", S(WUTER), 0, "a", "kioooooo", (SUBR)wuterset, (SUBR)wuter },
   { "sleighbells", S(SLEIGHBELLS), 0, "a","kioooooo",
                                       (SUBR)sleighset, (SUBR)sleighbells }
};


LINKAGE_BUILTIN(phisem_localops)
