/*
    bowedbar.c:

    Copyright (C) 1999 Perry Cook, Georg Essl, John ffitch

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

/*********************************************/
/*  Bowed Bar model                          */
/*  by Georg Essl, 1999                      */
/*  For details refer to:                    */
/*    G.Essl, P.R.Cook: "Banded Waveguides:  */
/*    Towards Physical Modelling of Bar      */
/*    Percussion Instruments", ICMC'99       */
/*********************************************/
#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "bowedbar.h"

/* Number of banded waveguide modes */

static void make_DLineN(CSOUND *csound, DLINEN *p, int32 length)
{
    /* Writing before reading allows delays from 0 to length-1.
       Thus, if we want to allow a delay of max_length, we need
       a delay-line of length = max_length+1. */
    p->length = length = length+1;
    csound->AuxAlloc(csound, length * sizeof(MYFLT), &p->inputs);
    p->inPoint = 0;
    p->outPoint = length >> 1;
    p->lastOutput = FL(0.0);
}

static void DLineN_setDelay(CSOUND *csound, DLINEN *p, int32_t lag)
{
    if (UNLIKELY(lag > p->length-1)) {                   /* if delay is too big, */
      csound->Warning(csound, Str("DLineN: Delay length too big ... setting to "
                                  "maximum length of %d.\n"), p->length - 1);
      p->outPoint = p->inPoint + 1;            /* force delay to max_length */
    }
    else
      p->outPoint = p->inPoint - (int32) lag;   /* read chases write */
    while (p->outPoint<0)
      p->outPoint += p->length;                /* modulo maximum length */
}

static void DLineN_tick(DLINEN *p, MYFLT sample) /*  Take one, yield one */
{
    MYFLT *xx = (MYFLT*)p->inputs.auxp;
    xx[p->inPoint++] = sample;   /* Input next sample */
    if (UNLIKELY(p->inPoint == p->length)) /* Check for end condition */
      p->inPoint -= p->length;
    p->lastOutput = xx[p->outPoint++]; /* Read nxt value */
    if (UNLIKELY(p->outPoint>=p->length))        /* Check for end condition */
      p->outPoint -= p->length;
}

int32_t bowedbarset(CSOUND *csound, BOWEDBAR *p)
{
    int32 i;
    MYFLT amplitude = *p->amp * AMP_RSCALE;

    p->modes[0] = FL(1.0);
    p->modes[1] = FL(2.756);
    p->modes[2] = FL(5.404);
    p->modes[3] = FL(8.933);

    make_BiQuad(&p->bandpass[0]);
    make_BiQuad(&p->bandpass[1]);
    make_BiQuad(&p->bandpass[2]);
    make_BiQuad(&p->bandpass[3]);
    make_ADSR(&p->adsr, CS_ESR);
    ADSR_setAllTimes(csound, &p->adsr, FL(0.02), FL(0.005), FL(0.9), FL(0.01));

    if (LIKELY(*p->lowestFreq>=FL(0.0))) {      /* If no init skip */
      if (*p->lowestFreq!=FL(0.0))
        p->length = (int32) (CS_ESR / *p->lowestFreq + FL(1.0));
      else if (*p->frequency!=FL(0.0))
        p->length = (int32) (CS_ESR / *p->frequency + FL(1.0));
      else {
        csound->Warning(csound,
                        "%s", Str("unknown lowest frequency for bowed bar -- "
                            "assuming 50Hz\n"));
        p->length = (int32) (CS_ESR / FL(50.0) + FL(1.0));
      }
    }

    p->nr_modes = NR_MODES;
    for (i = 0; i<NR_MODES; i++) {
      make_DLineN(csound, &p->delay[i], p->length);
      DLineN_setDelay(csound, &p->delay[i], (int32_t)(p->length/p->modes[i]));
      BiQuad_clear(&p->bandpass[i]);
    }
/*     p->gains[0] = FL(0.0); */
/*     p->gains[1] = FL(0.0); */
/*     p->gains[2] = FL(0.0); */
/*     p->gains[3] = FL(0.0); */
    p->adsr.target = FL(0.0);
    p->adsr.value = FL(0.0);
    p->adsr.rate = amplitude * FL(0.001);
    p->adsr.state = ATTACK;
    p->lastBowPos = FL(0.0);
    p->bowTarg = FL(0.0);
    p->freq = -FL(1.0);
    p->lastpos = -FL(1.0);
    p->lastpress = p->bowvel = p->velinput = FL(0.0);
    p->kloop = 0;
    p->bowTabl.offSet = p->bowTabl.slope = FL(0.0);
    return OK;
}

int32_t bowedbar(CSOUND *csound, BOWEDBAR *p)
{
    MYFLT       *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       amp = (*p->amp)*AMP_RSCALE; /* Normalise */
    int32 k;
    int32_t i;
    MYFLT       maxVelocity;
    MYFLT       integration_const = *p->integration_const;

    if (p->lastpress != *p->bowPress)
      p->bowTabl.slope = p->lastpress = *p->bowPress;
    if (p->freq != *p->frequency) {
      p->freq = *p->frequency;
      if (p->freq > FL(1568.0)) p->freq = FL(1568.0);

      p->length = (int32_t)(CS_ESR/p->freq);
      p->nr_modes = NR_MODES;   /* reset for frequency shift */
      for (i = 0; i<NR_MODES; i++) {
        if ((int32_t)(p->length/p->modes[i]) > 4)
          DLineN_setDelay(csound, &p->delay[i], (int32_t)(p->length/p->modes[i]));
        else    {
          p->nr_modes = i;
          break;
        }
      }
      if (UNLIKELY(p->nr_modes==0))
        return csound->InitError(csound,
                                 "%s", Str("Bowedbar: cannot have zero modes\n"));
      for (i=0; i<p->nr_modes; i++) {
        MYFLT R = FL(1.0) - p->freq * p->modes[i] * CS_PIDSR;
        BiQuad_clear(&p->bandpass[i]);
        BiQuad_setFreqAndReson(p->bandpass[i], p->freq * p->modes[i], R);
        BiQuad_setEqualGainZeroes(p->bandpass[i]);
        BiQuad_setGain(p->bandpass[i], (FL(1.0)-R*R)*FL(0.5));
      }
    }
                                /* Bow position as well */
    if (*p->position != p->lastpos) {
      MYFLT temp2 = *p->position * PI_F;
      p->gains[0] = FABS(SIN(temp2 * FL(0.5))) /*  * pow(0.9,0))*/;
      p->gains[1] = FABS(SIN(temp2) * FL(0.9));
      p->gains[2] = FABS(SIN(temp2 * FL(1.5)) * FL(0.9)*FL(0.9));
      p->gains[3] = FABS(SIN(temp2 * FL(2.0)) * FL(0.9)*FL(0.9)*FL(0.9));
      p->lastpos = *p->position;
    }
    if (*p->bowposition != p->lastBowPos) { /* Not sure what this control is? */
      p->bowTarg += FL(0.02) * (*p->bowposition - p->lastBowPos);
      p->lastBowPos = *p->bowposition;
      ADSR_setTarget(csound, &p->adsr, p->lastBowPos);
      p->lastBowPos = *p->bowposition;
    }
    if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;
    if ((--p->kloop) == 0) {
      ADSR_setReleaseRate(csound, &p->adsr, (FL(1.0) - amp) * FL(0.005));
      p->adsr.target = FL(0.0);
      p->adsr.rate = p->adsr.releaseRate;
      p->adsr.state = RELEASE;
    }
    maxVelocity = FL(0.03) + (FL(0.5) * amp);

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT data = FL(0.0);
      MYFLT input = FL(0.0);
      if (integration_const == FL(0.0))
        p->velinput = FL(0.0);
      else
        p->velinput = integration_const * p->velinput;

      for (k=0; k<p->nr_modes; k++) {
        p->velinput += *p->GAIN * p->delay[k].lastOutput;
      }

      if (*p->trackVel) {
        p->bowvel *= FL(0.9995);
        p->bowvel += p->bowTarg;
        p->bowTarg *= FL(0.995);
      }
      else
        p->bowvel = ADSR_tick(&p->adsr)*maxVelocity;

      input = p->bowvel - p->velinput;
      input = input * BowTabl_lookup(csound, &p->bowTabl, input);
      input = input/(MYFLT)p->nr_modes;

      for (k=0; k<p->nr_modes; k++) {
        BiQuad_tick(&p->bandpass[k],
                    input*p->gains[k] + *p->GAIN * p->delay[k].lastOutput);
        DLineN_tick(&p->delay[k], p->bandpass[k].lastOutput);
        data += p->bandpass[k].lastOutput;
      }

      ar[n] = data * AMP_SCALE * FL(20.0); /* 20 is an experimental value */
    }
    return OK;
}


