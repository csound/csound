/*
  fm4op.c:

  Copyright (C) 1998 Perry Cook, John ffitch

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

/*********************************************************/
/*  Master Class for 4 Operator FM Synth                 */
/*  by Perry R. Cook, 1995-96; recoded John ffitch 97/98 */
/*  This instrument contains 4 waves, 4 adsr, and        */
/*  various state vars.                                  */
/*                                                       */
/*  The basic Chowning/Stanford FM patent expired April  */
/*  1995, but there exist follow-on patents, mostly      */
/*  assigned to Yamaha.  If you are of the type who      */
/*  should worry about this (making money) worry away.   */
/*                                                       */
/*********************************************************/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "fm4op.h"

/***********************************************************/
/*  Two Zero Filter Class,                                 */
/*  by Perry R. Cook, 1995-96; recoded John ffitch 97/98   */
/*  See books on filters to understand more about how this */
/*  works.  Nothing out of the ordinary in this version.   */
/***********************************************************/

/* Used by moog1.c as well */
void make_TwoZero(TwoZero *p)
{
    p->zeroCoeffs[0] = FL(0.0);
    p->zeroCoeffs[1] = FL(0.0);
    p->gain = FL(1.0);
    p->inputs[0] = FL(0.0);
    p->inputs[1] = FL(0.0);
    p->lastOutput = FL(0.0);
}

void TwoZero_setZeroCoeffs(TwoZero* p, MYFLT *coeffs)
{
    p->zeroCoeffs[0] = coeffs[0];
    p->zeroCoeffs[1] = coeffs[1];
}

MYFLT TwoZero_tick(TwoZero *p, MYFLT sample)
/*   Perform Filter Operation            */
{ /*  TwoZero is a two zero filter (duh!)  */
  /*  Look it up in your favorite DSP text */
    MYFLT lastOutput = p->zeroCoeffs[0] * p->inputs[0] +
                       p->zeroCoeffs[1] * p->inputs[1];
    p->inputs[1] = p->inputs[0];
    p->inputs[0] = p->gain * sample;
    p->lastOutput = (lastOutput += p->inputs[0]);
    return lastOutput;
}

MYFLT Wave_tick(MYFLT *vTime, int32_t len, MYFLT *data, MYFLT rate, MYFLT phase)
{                                /* Tick on vibrato table */
    int32   temp;
    MYFLT   alpha;
    MYFLT   lastOutput;
    MYFLT   vvTime = *vTime;

    vvTime += rate;            /*  Update current time    */
    while (vvTime >= len)      /*  Check for end of sound */
      vvTime -= len;           /*  loop back to beginning */
    while (vvTime < FL(0.0))       /*  Check for end of sound */
      vvTime += len;           /*  loop back to beginning */

    *vTime = vvTime;

    if (phase != FL(0.0)) {
      vvTime += phase;      /*  Add phase offset       */
      while (vvTime >= len) /*  Check for end of sound */
        vvTime -= len;      /*  loop back to beginning */
      while (vvTime < FL(0.0))  /*  Check for end of sound */
        vvTime += len;      /*  loop back to beginning */
    }
    temp = (int32) vvTime;  /*  Integer part of time address    */
    /*  fractional part of time address */
    alpha = vvTime - (MYFLT)temp;
    lastOutput = data[temp];   /* Do linear interpolation */
    /* same as alpha*data[temp+1] + (1-alpha)data[temp] */
    lastOutput += (alpha * (data[temp+1] - lastOutput));
    /* End of vibrato tick */
    return lastOutput;
}

/* ---------------------------------------------------------------------- */

static int32_t      FM_tabs_built = 0;
static MYFLT    FM4Op_gains[100];
static MYFLT    FM4Op_susLevels[16];
static MYFLT    FM4Op_attTimes[32];

void build_FM(void)
{                                /* The following tables are pre-built */
    MYFLT       temp = FL(1.0);
    int32_t         i;

    for (i=99; i>=0; i--) {
      FM4Op_gains[i] = temp;
      temp *= FL(0.933033);
    }
    temp = FL(1.0);
    for (i=15; i>=0; i--) {
      FM4Op_susLevels[i] = temp;
      temp *= FL(0.707106781186547524400844362104849);
    }
    temp = FL(8.498186);
    for (i=0; i<32; i++) {
      FM4Op_attTimes[i] = temp;
      temp *= FL(0.707106781186547524400844362104849);
    }
    FM_tabs_built = 1;
}

int32_t make_FM4Op(CSOUND *csound, FM4OP *p)
{
    MYFLT       tempCoeffs[2] = {FL(0.0), -FL(1.0)};
    FUNC        *ftp;

    if (!FM_tabs_built) build_FM(); /* Ensure tables exist */

    make_ADSR(&p->adsr[0], CS_ESR);
    make_ADSR(&p->adsr[1], CS_ESR);
    make_ADSR(&p->adsr[2], CS_ESR);
    make_ADSR(&p->adsr[3], CS_ESR);
    make_TwoZero(&p->twozero);
    if (UNLIKELY((ftp = csound->FTFind(csound, p->vifn)) == NULL))
      goto err1;
    p->vibWave = ftp;
    p->baseFreq = csound->GetA4(csound);
    p->ratios[0] = FL(1.0);
    p->ratios[1] = FL(1.0);
    p->ratios[2] = FL(1.0);
    p->ratios[3] = FL(1.0);
    p->gains[0] = FL(1.0);
    p->gains[1] = FL(1.0);
    p->gains[2] = FL(1.0);
    p->gains[3] = FL(1.0);
    TwoZero_setZeroCoeffs(&p->twozero, tempCoeffs);
    p->twozero.gain = FL(0.0);
    p->w_phase[3] = 0;          /* *** FIDDLE???? *** */
    return OK;
 err1:
/* Expect sine wave */
    return csound->InitError(csound, "%s", Str("No table for VibWaveato"));
}

static int32_t FM4Op_loadWaves(CSOUND *csound, FM4OP *p)
{
    FUNC        *ftp;

    if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn0)) == NULL)) goto err1;
    p->waves[0] = ftp;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn1)) == NULL)) goto err1;
    p->waves[1] = ftp;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn2)) == NULL)) goto err1;
    p->waves[2] = ftp;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn3)) == NULL)) goto err1;
    p->waves[3] = ftp;
    p->w_time[0] = p->w_time[1] = p->w_time[2] = p->w_time[3] = FL(0.0);
    return OK;
 err1:
    return csound->InitError(csound, "%s", Str("No table for FM4Op"));
}

void FM4Op_setRatio(FM4OP *p, int32_t whichOne, MYFLT ratio)
{
    p->ratios[whichOne] = ratio;
    if (ratio>FL(0.0))
      p->w_rate[whichOne] = p->baseFreq * ratio;
    else
      p->w_rate[whichOne] = ratio;
}

void FM4Op_keyOff(FM4OP *p)
{
    ADSR_keyOff(&p->adsr[0]);
    ADSR_keyOff(&p->adsr[1]);
    ADSR_keyOff(&p->adsr[2]);
    ADSR_keyOff(&p->adsr[3]);
}

/*********************************************************/
/*  Algorithm 5 (TX81Z) Subclass of 4 Operator FM Synth  */
/*  by Perry R. Cook, 1995-96; recoded John ffitch 97/98 */
/*  This connection topology is 2 simple FM Pairs summed */
/*  together, like:                                      */
/*                                                       */
/*   Alg 5 is :      4->3--\                             */
/*                          + --> Out                    */
/*                   2->1--/                             */
/*                                                       */
/*  Controls: control1 = mod index 1                     */
/*            control2 = crossfade of two outputs        */
/*                                                       */
/*********************************************************/

MYFLT FM4Alg5_tick(FM4OP *p, MYFLT c1, MYFLT c2)
{
    MYFLT       temp,temp2;
    MYFLT       lastOutput;

    temp = p->gains[1] * ADSR_tick(&p->adsr[1]) *
      Wave_tick(&p->w_time[1], (int32_t)p->waves[1]->flen, p->waves[1]->ftable,
                p->w_rate[1], p->w_phase[1]);
    temp = temp * c1;
    p->w_phase[0] = p->waves[0]->flen * temp; /* addPhaseOffset */
    p->w_phase[3] = p->waves[0]->flen * p->twozero.lastOutput;
    temp =  p->gains[3] * ADSR_tick(&p->adsr[3]) *
      Wave_tick(&p->w_time[3], (int32_t)p->waves[3]->flen, p->waves[3]->ftable,
                p->w_rate[3], p->w_phase[3]);
    TwoZero_tick(&p->twozero, temp);
    p->w_phase[2] = p->waves[2]->flen * temp; /* addPhaseOffset */
    temp = (FL(1.0) - ( c2 * FL(0.5))) *  p->gains[0] *
      ADSR_tick(&p->adsr[0]) *
      Wave_tick(&p->w_time[0], (int32_t)p->waves[0]->flen, p->waves[0]->ftable,
                p->w_rate[0], p->w_phase[0]);
    temp +=  c2 * FL(0.5) *  p->gains[2] * ADSR_tick(&p->adsr[2]) *
      Wave_tick(&p->w_time[2], (int32_t)p->waves[2]->flen, p->waves[2]->ftable,
                p->w_rate[2], p->w_phase[2]);

    temp2 = Wave_tick(&p->v_time, (int32_t)p->vibWave->flen,
                      p->vibWave->ftable, p->v_rate, FL(0.0)) *
      *p->modDepth; /* Calculate amplitude mod */
    temp = temp * (FL(1.0) + temp2); /*  and apply it to output */

    lastOutput = temp * FL(0.5);
    return  lastOutput;
}

/***************************************************************/
/*  Tubular Bell (Orch. Chime) Subclass of Algorithm 5 (TX81Z) */
/*  Subclass of 4 Operator FM Synth by Perry R. Cook, 1995-96  */
/*  Recoded in C by John ffitch 1997-98                        */
/***************************************************************/

int32_t tubebellset(CSOUND *csound, FM4OP *p)
{
    MYFLT       amp = *p->amp * AMP_RSCALE; /* Normalised */
    MYFLT       opt = *p->opt;

    if (UNLIKELY(make_FM4Op(csound,p))) return NOTOK;
    if (UNLIKELY(FM4Op_loadWaves(csound,p)))
      return NOTOK; /* 4 x "rawwaves/sinewave.raw" */

    FM4Op_setRatio(p, 0, FL(1.0)   * FL(0.995));
    FM4Op_setRatio(p, 1, FL(1.414) * FL(0.995));
    FM4Op_setRatio(p, 2, FL(1.0)   * FL(1.005));
    FM4Op_setRatio(p, 3, FL(1.414)            );
    p->gains[0] = amp * FM4Op_gains[94];
    p->gains[1] = amp * FM4Op_gains[76];
    p->gains[2] = amp * FM4Op_gains[99];
    p->gains[3] = amp * FM4Op_gains[71];
    if (opt<= FL(0.0)) opt = FL(4.0);
    ADSR_setAllTimes(csound, &p->adsr[0], FL(0.005), opt, FL(0.0), FL(0.04));
    ADSR_setAllTimes(csound, &p->adsr[1], FL(0.005), opt, FL(0.0), FL(0.04));
    ADSR_setAllTimes(csound, &p->adsr[2], FL(0.001),FL(0.5)*opt,FL(0.0), FL(0.04));
    ADSR_setAllTimes(csound, &p->adsr[3], FL(0.004), opt, FL(0.0), FL(0.04));
    /*      ADSR_setAll(csound, &p->adsr[0], 0.03f,0.00001f,FL(0.0),0.02f); */
    /*      ADSR_setAll(csound, &p->adsr[1], 0.03f,0.00001f,FL(0.0),0.02f); */
    /*      ADSR_setAll(csound, &p->adsr[2], 0.07f,0.00002f,FL(0.0),0.02f); */
    /*      ADSR_setAll(csound, &p->adsr[3], FL(0.04),0.00001f,FL(0.0),0.02f); */
    p->twozero.gain = FL(0.5);
    p->v_rate = FL(2.0) * p->vibWave->flen * CS_ONEDSR; /* Vib rate */
    /* Set Freq */
    p->baseFreq = *p->frequency;
    p->w_rate[0] = p->baseFreq * p->ratios[0] * p->waves[0]->flen * CS_ONEDSR;
    p->w_rate[1] = p->baseFreq * p->ratios[1] * p->waves[1]->flen * CS_ONEDSR;
    p->w_rate[2] = p->baseFreq * p->ratios[2] * p->waves[2]->flen * CS_ONEDSR;
    p->w_rate[3] = p->baseFreq * p->ratios[3] * p->waves[3]->flen * CS_ONEDSR;
    ADSR_keyOn(&p->adsr[0]);
    ADSR_keyOn(&p->adsr[1]);
    ADSR_keyOn(&p->adsr[2]);
    ADSR_keyOn(&p->adsr[3]);
    return OK;
}

int32_t tubebell(CSOUND *csound, FM4OP *p)
{
    MYFLT       amp = *p->amp * AMP_RSCALE; /* Normalised */
    MYFLT       *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       c1 = *p->control1;
    MYFLT       c2 = *p->control2;

    /* Set Freq */
    p->baseFreq = *p->frequency;
    p->gains[0] = amp * FM4Op_gains[94];
    p->gains[1] = amp * FM4Op_gains[76];
    p->gains[2] = amp * FM4Op_gains[99];
    p->gains[3] = amp * FM4Op_gains[71];
    p->w_rate[0] = p->baseFreq * p->ratios[0] * p->waves[0]->flen * CS_ONEDSR;
    p->w_rate[1] = p->baseFreq * p->ratios[1] * p->waves[1]->flen * CS_ONEDSR;
    p->w_rate[2] = p->baseFreq * p->ratios[2] * p->waves[2]->flen * CS_ONEDSR;
    p->w_rate[3] = p->baseFreq * p->ratios[3] * p->waves[3]->flen * CS_ONEDSR;
    p->v_rate = *p->vibFreq * p->vibWave->flen * CS_ONEDSR;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT   lastOutput = FM4Alg5_tick(p, c1, c2);
      ar[n] = lastOutput*AMP_SCALE*FL(1.8);
    }
    return OK;
}

/*****************************************************************/
/*  Fender Rhodes Electric Piano Subclass of Algorithm 5 (TX81Z) */
/*  Subclass of 4 Operator FM Synth by Perry R. Cook, 1995-96    */
/*  Recoded in C by John ffitch 1997-98                          */
/*****************************************************************/

int32_t rhodeset(CSOUND *csound, FM4OP *p)
{
    MYFLT       amp = *p->amp * AMP_RSCALE; /* Normalised */

    if (UNLIKELY(make_FM4Op(csound,p))) return NOTOK;
    if (UNLIKELY(FM4Op_loadWaves(csound,p))) return NOTOK; /* 3 times "sinewave.raw";
                                                    1 x fwavblnk.raw */

    FM4Op_setRatio(p, 0, FL(1.0));
    FM4Op_setRatio(p, 1, FL(0.5));
    FM4Op_setRatio(p, 2, FL(1.0));
    FM4Op_setRatio(p, 3, FL(15.0));
    p->gains[0] = amp * FM4Op_gains[99];
    p->gains[1] = amp * FM4Op_gains[90];
    p->gains[2] = amp * FM4Op_gains[99];
    p->gains[3] = amp * FM4Op_gains[67];
    ADSR_setAllTimes(csound, &p->adsr[0], FL(0.001), FL(1.50), FL(0.0), FL(0.04));
    ADSR_setAllTimes(csound, &p->adsr[1], FL(0.001), FL(1.50), FL(0.0), FL(0.04));
    ADSR_setAllTimes(csound, &p->adsr[2], FL(0.001), FL(1.00), FL(0.0), FL(0.04));
    ADSR_setAllTimes(csound, &p->adsr[3], FL(0.001), FL(0.25), FL(0.0), FL(0.04));
    /*      ADSR_setAll(&p->adsr[0], 0.05f,0.00003f,FL(0.0),0.02f); */
    /*      ADSR_setAll(&p->adsr[1], 0.05f,0.00003f,FL(0.0),0.02f); */
    /*      ADSR_setAll(&p->adsr[2], 0.05f,0.00005f,FL(0.0),0.02f); */
    /*      ADSR_setAll(&p->adsr[3], 0.05f,0.0002f,FL(0.0),0.02f); */
    p->twozero.gain = FL(1.0);
    p->v_rate = FL(2.0) * p->vibWave->flen * CS_ONEDSR; /* Vib rate */
    /* Set Freq */
    p->baseFreq = *p->frequency;
    p->w_rate[0] = p->baseFreq * p->ratios[0] * p->waves[0]->flen * CS_ONEDSR;
    p->w_rate[1] = p->baseFreq * p->ratios[1] * p->waves[1]->flen * CS_ONEDSR;
    p->w_rate[2] = p->baseFreq * p->ratios[2] * p->waves[2]->flen * CS_ONEDSR;
    p->w_rate[3] = p->baseFreq * p->ratios[3] * p->waves[3]->flen * CS_ONEDSR;
    ADSR_keyOn(&p->adsr[0]);
    ADSR_keyOn(&p->adsr[1]);
    ADSR_keyOn(&p->adsr[2]);
    ADSR_keyOn(&p->adsr[3]);
    return OK;
}

/***************************************************************/
/*  Wurlitzer Electric Piano Subclass of Algorithm 5 (TX81Z)   */
/*  Subclass of 4 Operator FM Synth by Perry R. Cook, 1995-96  */
/*  Recoded in C by John ffitch 1997-98                        */
/***************************************************************/

int32_t wurleyset(CSOUND *csound, FM4OP *p)
{
    MYFLT       amp = *p->amp * AMP_RSCALE; /* Normalised */

    if (UNLIKELY(make_FM4Op(csound,p))) return NOTOK;
    if (UNLIKELY(FM4Op_loadWaves(csound,p))) return NOTOK; /* 3 x "sinewave.raw";
                                                    1 x fwavblnk.raw */

    FM4Op_setRatio(p, 0, FL(1.0));
    FM4Op_setRatio(p, 1, FL(4.05));
    FM4Op_setRatio(p, 2, -FL(510.0));
    FM4Op_setRatio(p, 3, -FL(510.0));
    p->gains[0] = amp * FM4Op_gains[99];
    p->gains[1] = amp * FM4Op_gains[82];
    p->gains[2] = amp * FM4Op_gains[82];
    p->gains[3] = amp * FM4Op_gains[68];
    ADSR_setAllTimes(csound, &p->adsr[0], FL(0.001), FL(1.50), FL(0.0), FL(0.04));
    ADSR_setAllTimes(csound, &p->adsr[1], FL(0.001), FL(1.50), FL(0.0), FL(0.04));
    ADSR_setAllTimes(csound, &p->adsr[2], FL(0.001), FL(0.25), FL(0.0), FL(0.04));
    ADSR_setAllTimes(csound, &p->adsr[3], FL(0.001), FL(0.15), FL(0.0), FL(0.04));
    /*      ADSR_setAll(&p->adsr[0], 0.05f,0.00003f,FL(0.0),0.02f); */
    /*      ADSR_setAll(&p->adsr[1], 0.05f,0.00003f,FL(0.0),0.02f); */
    /*      ADSR_setAll(&p->adsr[2], 0.05f,0.0002f,FL(0.0),0.02f); */
    /*      ADSR_setAll(&p->adsr[3], 0.05f,0.0003f,FL(0.0),0.02f); */
    p->twozero.gain = FL(2.0);
    /* Set Freq */
    p->baseFreq = *p->frequency;
    p->w_rate[0] = p->baseFreq * p->ratios[0] * p->waves[0]->flen * CS_ONEDSR;
    p->w_rate[1] = p->baseFreq * p->ratios[1] * p->waves[1]->flen * CS_ONEDSR;
    p->w_rate[2] =               p->ratios[2] * p->waves[2]->flen * CS_ONEDSR;
    p->w_rate[3] =               p->ratios[3] * p->waves[3]->flen * CS_ONEDSR;
    ADSR_keyOn(&p->adsr[0]);
    ADSR_keyOn(&p->adsr[1]);
    ADSR_keyOn(&p->adsr[2]);
    ADSR_keyOn(&p->adsr[3]);
    return OK;
}

int32_t wurley(CSOUND *csound, FM4OP *p)
{
    MYFLT       amp = *p->amp * AMP_RSCALE; /* Normalised */
    MYFLT       *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       c1 = *p->control1;
    MYFLT       c2 = *p->control2;

    /* Set Freq */
    p->baseFreq = *p->frequency;
    p->gains[0] = amp * FM4Op_gains[99];
    p->gains[1] = amp * FM4Op_gains[82];
    p->gains[2] = amp * FM4Op_gains[82];
    p->gains[3] = amp * FM4Op_gains[68];
    p->w_rate[0] = p->baseFreq * p->ratios[0] * p->waves[0]->flen * CS_ONEDSR;
    p->w_rate[1] = p->baseFreq * p->ratios[1] * p->waves[1]->flen * CS_ONEDSR;
    p->w_rate[2] =               p->ratios[2] * p->waves[2]->flen * CS_ONEDSR;
    p->w_rate[3] =               p->ratios[3] * p->waves[3]->flen * CS_ONEDSR;
    p->v_rate = *p->vibFreq * p->vibWave->flen * CS_ONEDSR;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT   lastOutput = FM4Alg5_tick(p, c1, c2);
      ar[n] = lastOutput*AMP_SCALE*FL(1.9);
    }
    return OK;
}

/*********************************************************/
/*  Algorithm 3 (TX81Z) Subclass of 4 Operator FM Synth  */
/*  by Perry R. Cook, 1995-96; recoded John ffitch 97/98 */
/*                                                       */
/*  Alg 3 is :          4--\                             */
/*                  3-->2-- + -->1-->Out                 */
/*                                                       */
/*  Controls: control1 = total mod index                 */
/*            control2 = crossfade of two modulators     */
/*                                                       */
/*********************************************************/

MYFLT FM4Alg3_tick(FM4OP *p, MYFLT c1, MYFLT c2)
{
    MYFLT       temp;
    MYFLT       lastOutput;

    temp = *p->modDepth * FL(0.2) *
      Wave_tick(&p->v_time, (int32_t)p->vibWave->flen,
                p->vibWave->ftable, p->v_rate, FL(0.0));
    p->w_rate[0] = p->baseFreq * (FL(1.0) + temp) * p->ratios[0];
    p->w_rate[1] = p->baseFreq * (FL(1.0) + temp) * p->ratios[1];
    p->w_rate[2] = p->baseFreq * (FL(1.0) + temp) * p->ratios[2];
    p->w_rate[3] = p->baseFreq * (FL(1.0) + temp) * p->ratios[3];

    temp = p->gains[2] * ADSR_tick(&p->adsr[2]) *
      Wave_tick(&p->w_time[2], (int32_t)p->waves[2]->flen, p->waves[2]->ftable,
                p->w_rate[2], p->w_phase[2]);
    p->w_phase[1] = p->waves[1]->flen * temp;
    p->w_phase[3] = p->waves[3]->flen * p->twozero.lastOutput;
    temp = (FL(1.0) - (c2 * FL(0.5))) * p->gains[3] * ADSR_tick(&p->adsr[3]) *
      Wave_tick(&p->w_time[3], (int32_t)p->waves[3]->flen, p->waves[3]->ftable,
                p->w_rate[3], p->w_phase[3]);
    TwoZero_tick(&p->twozero, temp);

    temp += c2 * FL(0.5) * p->gains[1] * ADSR_tick(&p->adsr[1]) *
      Wave_tick(&p->w_time[1], (int32_t)p->waves[1]->flen, p->waves[1]->ftable,
                p->w_rate[1], p->w_phase[1]);
    temp = temp * c1;

    p->w_phase[0] = p->waves[0]->flen * temp;
    temp = p->gains[0] * ADSR_tick(&p->adsr[0]) *
      Wave_tick(&p->w_time[0], (int32_t)p->waves[0]->flen, p->waves[0]->ftable,
                p->w_rate[0], p->w_phase[0]);

    lastOutput = temp * FL(0.5);
    return lastOutput;
}

int32_t heavymetset(CSOUND *csound, FM4OP *p)
{
    if (UNLIKELY(make_FM4Op(csound,p))) return NOTOK;
    if (UNLIKELY(FM4Op_loadWaves(csound,p))) return NOTOK;  /* Mixed -- 2 x sine;
                                                     1 x fwavblnk */
    FM4Op_setRatio(p, 0, FL(1.00)         );
    FM4Op_setRatio(p, 1, FL(4.00) * FL(0.999));
    FM4Op_setRatio(p, 2, FL(3.00) * FL(1.001));
    FM4Op_setRatio(p, 3, FL(0.50) * FL(1.002));
    ADSR_setAllTimes(csound, &p->adsr[0], FL(0.001), FL(0.001), FL(1.0), FL(0.01));
    ADSR_setAllTimes(csound, &p->adsr[1], FL(0.001), FL(0.010), FL(1.0), FL(0.50));
    ADSR_setAllTimes(csound, &p->adsr[2], FL(0.010), FL(0.005), FL(1.0), FL(0.20));
    ADSR_setAllTimes(csound, &p->adsr[3], FL(0.030), FL(0.010), FL(0.2), FL(0.20));
    /*      ADSR_setAll(&p->adsr[0], 0.050f, 0.0100f, FL(1.0), FL(0.001));  */
    /*      ADSR_setAll(&p->adsr[1], 0.050f, 0.0010f, FL(1.0), 0.0001f); */
    /*      ADSR_setAll(&p->adsr[2], FL(0.001), 0.0020f, FL(1.0), 0.0002f); */
    /*      ADSR_setAll(&p->adsr[3], 0.050f, 0.0010f, FL(0.2), 0.0002f); */
    p->twozero.gain = FL(2.0);
    /*     p->v_rate = 5.5 * p->vibWave->flen * CS_ONEDSR;  Vib rate */
    ADSR_keyOn(&p->adsr[0]);
    ADSR_keyOn(&p->adsr[1]);
    ADSR_keyOn(&p->adsr[2]);
    ADSR_keyOn(&p->adsr[3]);
    return OK;
}

int32_t heavymet(CSOUND *csound, FM4OP *p)
{
    MYFLT       *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       amp = *p->amp * AMP_RSCALE; /* Normalised */
    MYFLT       c1 = *p->control1;
    MYFLT       c2 = *p->control2;
    MYFLT       temp;

    p->baseFreq = *p->frequency;
    p->gains[0] = amp * FM4Op_gains[92];
    p->gains[1] = amp * FM4Op_gains[76];
    p->gains[2] = amp * FM4Op_gains[91];
    p->gains[3] = amp * FM4Op_gains[68];

    temp         = p->baseFreq * CS_ONEDSR;
    p->w_rate[0] = temp * p->ratios[0] * p->waves[0]->flen;
    p->w_rate[1] = temp * p->ratios[1] * p->waves[1]->flen;
    p->w_rate[2] = temp * p->ratios[2] * p->waves[2]->flen;
    p->w_rate[3] = temp * p->ratios[3] * p->waves[3]->flen;
    p->v_rate = *p->vibFreq * p->vibWave->flen * CS_ONEDSR;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT   lastOutput;
      lastOutput = FM4Alg3_tick(p, c1, c2);
      ar[n] = lastOutput*AMP_SCALE*FL(2.0);
    }
    return OK;
}

/**********************************************************/
/*  Algorithm 8 (TX81Z) Subclass of 4 Operator FM Synth   */
/*  by Perry R. Cook, 1995-96; recoded John ffitch 97-98  */
/*  This connection topology is simple Additive Synthesis */
/*                                                        */
/*                   1 --.                                */
/*                   2 -\|                                */
/*                       +-> Out                          */
/*                   3 -/|                                */
/*                   4 --                                 */
/*                                                        */
/*  Controls: control1 = op4 (fb) gain                    */
/*            control2 = op3 gain                         */
/*                                                        */
/**********************************************************/

MYFLT FM4Alg8_tick(FM4OP *p, MYFLT c1, MYFLT c2)
{
    MYFLT       temp;
    MYFLT       lastOutput;

    p->w_phase[3] = p->waves[3]->flen * p->twozero.lastOutput;

    temp = c1 * FL(2.0) * p->gains[3] * ADSR_tick(&p->adsr[3]) *
      Wave_tick(&p->w_time[3], (int32_t)p->waves[3]->flen, p->waves[3]->ftable,
                p->w_rate[3], p->w_phase[3]);
    TwoZero_tick(&p->twozero, temp);
    temp += c2 * FL(2.0) * p->gains[2] * ADSR_tick(&p->adsr[2]) *
      Wave_tick(&p->w_time[2], (int32_t)p->waves[2]->flen, p->waves[2]->ftable,
                p->w_rate[2], p->w_phase[2]);
    temp += p->gains[1] * ADSR_tick(&p->adsr[1]) *
      Wave_tick(&p->w_time[1], (int32_t)p->waves[1]->flen, p->waves[1]->ftable,
                p->w_rate[1], p->w_phase[1]);
    temp += p->gains[0] * ADSR_tick(&p->adsr[0]) *
      Wave_tick(&p->w_time[0], (int32_t)p->waves[0]->flen, p->waves[0]->ftable,
                p->w_rate[0], p->w_phase[0]);

    lastOutput = temp * FL(0.125);
    return lastOutput;
}

/**************************************************************/
/*  Hammond(OID) Organ Subclass of Algorithm 8 (TX81Z)        */
/*  Subclass of 4 Operator FM Synth by Perry R. Cook, 1995-96 */
/*  Recoded in C by John ffitch 1997-98                       */
/**************************************************************/

int32_t b3set(CSOUND *csound, FM4OP *p)
{
    MYFLT       amp = *p->amp * AMP_RSCALE; /* Normalised */
    MYFLT       temp = p->baseFreq * CS_ONEDSR;

    if (UNLIKELY(make_FM4Op(csound,p))) return NOTOK;
    if (UNLIKELY(FM4Op_loadWaves(csound,p))) return NOTOK;         /* sines */
    FM4Op_setRatio(p, 0, FL(0.999));
    FM4Op_setRatio(p, 1, FL(1.997));
    FM4Op_setRatio(p, 2, FL(3.006));
    FM4Op_setRatio(p, 3, FL(6.009));

    p->gains[0] = amp * FM4Op_gains[95];
    p->gains[1] = amp * FM4Op_gains[95];
    p->gains[2] = amp * FM4Op_gains[99];
    p->gains[3] = amp * FM4Op_gains[95];
    ADSR_setAllTimes(csound, &p->adsr[0], FL(0.005), FL(0.003), FL(1.0), FL(0.01));
    ADSR_setAllTimes(csound, &p->adsr[1], FL(0.005), FL(0.003), FL(1.0), FL(0.01));
    ADSR_setAllTimes(csound, &p->adsr[2], FL(0.005), FL(0.003), FL(1.0), FL(0.01));
    ADSR_setAllTimes(csound, &p->adsr[3], FL(0.005), FL(0.001), FL(0.4), FL(0.03));
    p->twozero.gain = FL(0.1);
    ADSR_keyOn(&p->adsr[0]);
    ADSR_keyOn(&p->adsr[1]);
    ADSR_keyOn(&p->adsr[2]);
    ADSR_keyOn(&p->adsr[3]);
    p->w_rate[0] = p->ratios[0] * temp * p->waves[0]->flen;
    p->w_rate[1] = p->ratios[1] * temp * p->waves[1]->flen;
    p->w_rate[2] = p->ratios[2] * temp * p->waves[2]->flen;
    p->w_rate[3] = p->ratios[3] * temp * p->waves[3]->flen;
    return OK;
}

int32_t hammondB3(CSOUND *csound, FM4OP *p)
{
    MYFLT       amp = *p->amp * AMP_RSCALE; /* Normalised */
    MYFLT       *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
     uint32_t n, nsmps = CS_KSMPS;
    MYFLT       c1 = *p->control1;
    MYFLT       c2 = *p->control2;
    MYFLT       temp;
    MYFLT       moddep  = *p->modDepth;

    p->baseFreq = *p->frequency;
    p->gains[0] = amp * FM4Op_gains[95];
    p->gains[1] = amp * FM4Op_gains[95];
    p->gains[2] = amp * FM4Op_gains[99];
    p->gains[3] = amp * FM4Op_gains[95];
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT   lastOutput;
      if (moddep > FL(0.0)) {
        p->v_rate = *p->vibFreq * p->vibWave->flen * CS_ONEDSR;
        temp = FL(1.0) + (moddep * FL(0.1) *
                          Wave_tick(&p->v_time, (int32_t)p->vibWave->flen,
                                    p->vibWave->ftable, p->v_rate, FL(0.0)));
        temp *= p->baseFreq * CS_ONEDSR;
        p->w_rate[0] = p->ratios[0] * temp * p->waves[0]->flen;
        p->w_rate[1] = p->ratios[1] * temp * p->waves[1]->flen;
        p->w_rate[2] = p->ratios[2] * temp * p->waves[2]->flen;
        p->w_rate[3] = p->ratios[3] * temp * p->waves[3]->flen;
      }
      // *** if modDepth is zero it looks as if w_rate should be initialised
      // *** but it make no difference ***
      lastOutput = FM4Alg8_tick(p, c1, c2);
      ar[n]= lastOutput*AMP_SCALE;
    }
    return OK;
}

/************************************************************/
/*  Algorithm 6 (TX81Z) Subclass of 4 Operator FM Synth     */
/*  by Perry R. Cook, 1995-96; recoded John ffitch 97-98    */
/*  This connection topology is three Carriers and a common */
/*  Modulator     /->1 -\                                   */
/*             4-|-->2 - +-> Out                            */
/*                \->3 -/                                   */
/*                                                          */
/*  Controls: control1 = vowel                              */
/*            control2 = spectral tilt                      */
/*                                                          */
/************************************************************/

MYFLT FM4Alg6_tick(CSOUND *csound, FM4OPV *q)
{
    MYFLT       temp,temp2;
    FM4OP       *p = (FM4OP*)q;

    temp = p->gains[3] * ADSR_tick(&p->adsr[3]) *
      Wave_tick(&p->w_time[3], (int32_t)p->waves[3]->flen, p->waves[3]->ftable,
                p->w_rate[3], p->w_phase[3]);
    /*  Calculate frequency mod  */
    temp2 = Wave_tick(&p->v_time, (int32_t)p->vibWave->flen, p->vibWave->ftable,
                      p->v_rate, FL(0.0)) * *p->modDepth * FL(0.1);

    temp2 = (FL(1.0) + temp2) * p->baseFreq * CS_ONEDSR;
    p->w_rate[0] = temp2 * p->ratios[0] * p->waves[0]->flen;
    p->w_rate[1] = temp2 * p->ratios[1] * p->waves[1]->flen;
    p->w_rate[2] = temp2 * p->ratios[2] * p->waves[2]->flen;
    p->w_rate[3] = temp2 * p->ratios[3] * p->waves[3]->flen;

    p->w_phase[0] = p->waves[0]->flen * temp * q->mods[0];
    p->w_phase[1] = p->waves[1]->flen * temp * q->mods[1];
    p->w_phase[2] = p->waves[2]->flen * temp * q->mods[2];
    p->w_phase[3] = p->waves[3]->flen * p->twozero.lastOutput;

    TwoZero_tick(&p->twozero, temp);

    temp =  p->gains[0] * q->tilt[0] * ADSR_tick(&p->adsr[0]) *
      Wave_tick(&p->w_time[0], (int32_t)p->waves[0]->flen, p->waves[0]->ftable,
                p->w_rate[0], p->w_phase[0]);
    temp += p->gains[1] * q->tilt[1] * ADSR_tick(&p->adsr[1]) *
      Wave_tick(&p->w_time[1], (int32_t)p->waves[1]->flen, p->waves[1]->ftable,
                p->w_rate[1], p->w_phase[1]);
    temp += p->gains[2] * q->tilt[2] * ADSR_tick(&p->adsr[2]) *
      Wave_tick(&p->w_time[2], (int32_t)p->waves[2]->flen, p->waves[2]->ftable,
                p->w_rate[2], p->w_phase[2]);

    return temp * FL(0.33);
}

MYFLT phonGains[32][2] =
  {{FL(1.0), FL(0.0)},    /* eee */
   {FL(1.0), FL(0.0)},    /* ihh */
   {FL(1.0), FL(0.0)},    /* ehh */
   {FL(1.0), FL(0.0)},    /* aaa */

   {FL(1.0), FL(0.0)},    /* ahh */
   {FL(1.0), FL(0.0)},    /* aww */
   {FL(1.0), FL(0.0)},    /* ohh */
   {FL(1.0), FL(0.0)},    /* uhh */

   {FL(1.0), FL(0.0)},    /* uuu */
   {FL(1.0), FL(0.0)},    /* ooo */
   {FL(1.0), FL(0.0)},    /* rrr */
   {FL(1.0), FL(0.0)},    /* lll */

   {FL(1.0), FL(0.0)},    /* mmm */
   {FL(1.0), FL(0.0)},    /* nnn */
   {FL(1.0), FL(0.0)},    /* nng */
   {FL(1.0), FL(0.0)},    /* ngg */

   {FL(0.0), FL(1.0)},    /* fff */
   {FL(0.0), FL(1.0)},    /* sss */
   {FL(0.0), FL(1.0)},    /* thh */
   {FL(0.0), FL(1.0)},    /* shh */

   {FL(0.0), FL(1.0)},    /* xxx */
   {FL(0.0), FL(0.1)},    /* hee */
   {FL(0.0), FL(0.1)},    /* hoo */
   {FL(0.0), FL(0.1)},    /* hah */

   {FL(1.0), FL(0.1)},    /* bbb */
   {FL(1.0), FL(0.1)},    /* ddd */
   {FL(1.0), FL(0.1)},    /* jjj */
   {FL(1.0), FL(0.1)},    /* ggg */

   {FL(1.0), FL(1.0)},    /* vvv */
   {FL(1.0), FL(1.0)},    /* zzz */
   {FL(1.0), FL(1.0)},    /* thz */
   {FL(1.0), FL(1.0)}     /* zhh */
  };

MYFLT phonParams[32][4][3] =
  {{  { FL(273.0), FL(0.996),   FL(0.0)},   /* eee (beet) */
      {FL(2086.0), FL(0.945), -FL(16.0)},
      {FL(2754.0), FL(0.979), -FL(12.0)},
      {FL(3270.0), FL(0.440), -FL(17.0)}},
   {  { FL(385.0), FL(0.987),  FL(10.0)},   /* ihh (bit) */
      {FL(2056.0), FL(0.930), -FL(20.0)},
      {FL(2587.0), FL(0.890), -FL(20.0)},
      {FL(3150.0), FL(0.400), -FL(20.0)}},
   {  { FL(515.0), FL(0.977),  FL(10.0)},   /* ehh (bet) */
      {FL(1805.0), FL(0.810), -FL(10.0)},
      {FL(2526.0), FL(0.875), -FL(10.0)},
      {FL(3103.0), FL(0.400), -FL(13.0)}},
   {  { FL(773.0), FL(0.950),  FL(10.0)},   /* aaa (bat) */
      {FL(1676.0), FL(0.830),  -FL(6.0)},
      {FL(2380.0), FL(0.880), -FL(20.0)},
      {FL(3027.0), FL(0.600), -FL(20.0)}},

   {  { FL(770.0), FL(0.950),   FL(0.0)},   /* ahh (father) */
      {FL(1153.0), FL(0.970),  -FL(9.0)},
      {FL(2450.0), FL(0.780), -FL(29.0)},
      {FL(3140.0), FL(0.800), -FL(39.0)}},
   {  { FL(637.0), FL(0.910),   FL(0.0)},   /* aww (bought) */
      { FL(895.0), FL(0.900),  -FL(3.0)},
      {FL(2556.0), FL(0.950), -FL(17.0)},
      {FL(3070.0), FL(0.910), -FL(20.0)}},
   {  { FL(637.0), FL(0.910),   FL(0.0)},   /* ohh (bone) */
      /*NOTE:: same as aww (bought) */
      { FL(895.0), FL(0.900),  -FL(3.0)},
      {FL(2556.0), FL(0.950), -FL(17.0)},
      {FL(3070.0), FL(0.910), -FL(20.0)}},
   {  { FL(561.0), FL(0.965),   FL(0.0)},   /* uhh (but) */
      {FL(1084.0), FL(0.930), -FL(10.0)},
      {FL(2541.0), FL(0.930), -FL(15.0)},
      {FL(3345.0), FL(0.900), -FL(20.0)}},

   {  { FL(515.0), FL(0.976),   FL(0.0)},   /* uuu (foot) */
      {FL(1031.0), FL(0.950),  -FL(3.0)},
      {FL(2572.0), FL(0.960), -FL(11.0)},
      {FL(3345.0), FL(0.960), -FL(20.0)}},
   {  { FL(349.0), FL(0.986), -FL(10.0)},   /* ooo (boot) */
      { FL(918.0), FL(0.940), -FL(20.0)},
      {FL(2350.0), FL(0.960), -FL(27.0)},
      {FL(2731.0), FL(0.950), -FL(33.0)}},
   {  { FL(394.0), FL(0.959), -FL(10.0)},   /* rrr (bird) */
      {FL(1297.0), FL(0.780), -FL(16.0)},
      {FL(1441.0), FL(0.980), -FL(16.0)},
      {FL(2754.0), FL(0.950), -FL(40.0)}},
   {  { FL(462.0), FL(0.990),  +FL(5.0)},   /* lll (lull) */
      {FL(1200.0), FL(0.640), -FL(10.0)},
      {FL(2500.0), FL(0.200), -FL(20.0)},
      {FL(3000.0), FL(0.100), -FL(30.0)}},

   {  { FL(265.0), FL(0.987), -FL(10.0)},   /* mmm (mom) */
      {FL(1176.0), FL(0.940), -FL(22.0)},
      {FL(2352.0), FL(0.970), -FL(20.0)},
      {FL(3277.0), FL(0.940), -FL(31.0)}},
   {  { FL(204.0), FL(0.980), -FL(10.0)},   /* nnn (nun) */
      {FL(1570.0), FL(0.940), -FL(15.0)},
      {FL(2481.0), FL(0.980), -FL(12.0)},
      {FL(3133.0), FL(0.800), -FL(30.0)}},
   {  { FL(204.0), FL(0.980), -FL(10.0)},   /* nng (sang) NOTE:: same as nnn */
      {FL(1570.0), FL(0.940), -FL(15.0)},
      {FL(2481.0), FL(0.980), -FL(12.0)},
      {FL(3133.0), FL(0.800), -FL(30.0)}},
   {  { FL(204.0), FL(0.980), -FL(10.0)},   /* ngg (bong) NOTE:: same as nnn */
      {FL(1570.0), FL(0.940), -FL(15.0)},
      {FL(2481.0), FL(0.980), -FL(12.0)},
      {FL(3133.0), FL(0.800), -FL(30.0)}},

   {  {FL(1000.0), FL(0.300),   FL(0.0)},   /* fff */
      {FL(2800.0), FL(0.860), -FL(10.0)},
      {FL(7425.0), FL(0.740),   FL(0.0)},
      {FL(8140.0), FL(0.860),   FL(0.0)}},
   {  {FL(0.0), FL(0.000),   FL(0.0)},      /* sss */
      {FL(2000.0), FL(0.700), -FL(15.0)},
      {FL(5257.0), FL(0.750),  -FL(3.0)},
      {FL(7171.0), FL(0.840),   FL(0.0)}},
   {  { FL(100.0), FL(0.900),   FL(0.0)},   /* thh */
      {FL(4000.0), FL(0.500), -FL(20.0)},
      {FL(5500.0), FL(0.500), -FL(15.0)},
      {FL(8000.0), FL(0.400), -FL(20.0)}},
   {  {FL(2693.0), FL(0.940),   FL(0.0)},   /* shh */
      {FL(4000.0), FL(0.720), -FL(10.0)},
      {FL(6123.0), FL(0.870), -FL(10.0)},
      {FL(7755.0), FL(0.750), -FL(18.0)}},

   {  {FL(1000.0), FL(0.300), -FL(10.0)},   /* xxx  NOTE:: Not Really Done Yet */
      {FL(2800.0), FL(0.860), -FL(10.0)},
      {FL(7425.0), FL(0.740),   FL(0.0)},
      {FL(8140.0), FL(0.860),   FL(0.0)}},
   {  { FL(273.0), FL(0.996), -FL(40.0)},   /* hee (beet)    (noisy eee) */
      {FL(2086.0), FL(0.945), -FL(16.0)},
      {FL(2754.0), FL(0.979), -FL(12.0)},
      {FL(3270.0), FL(0.440), -FL(17.0)}},
   {  { FL(349.0), FL(0.986), -FL(40.0)},   /* hoo (boot)    (noisy ooo) */
      { FL(918.0), FL(0.940), -FL(10.0)},
      {FL(2350.0), FL(0.960), -FL(17.0)},
      {FL(2731.0), FL(0.950), -FL(23.0)}},
   {  { FL(770.0), FL(0.950), -FL(40.0)},   /* hah (father)  (noisy ahh) */
      {FL(1153.0), FL(0.970),  -FL(3.0)},
      {FL(2450.0), FL(0.780), -FL(20.0)},
      {FL(3140.0), FL(0.800), -FL(32.0)}},

   {  {FL(2000.0), FL(0.700), -FL(20.0)},   /* bbb NOTE:: Not Really Done Yet */
      {FL(5257.0), FL(0.750), -FL(15.0)},
      {FL(7171.0), FL(0.840),  -FL(3.0)},
      {FL(9000.0), FL(0.900),   FL(0.0)}},
   {  { FL(100.0), FL(0.900),   FL(0.0)},   /* ddd NOTE:: Not Really Done Yet */
      {FL(4000.0), FL(0.500), -FL(20.0)},
      {FL(5500.0), FL(0.500), -FL(15.0)},
      {FL(8000.0), FL(0.400), -FL(20.0)}},
   {  {FL(2693.0), FL(0.940),   FL(0.0)},   /* jjj NOTE:: Not Really Done Yet */
      {FL(4000.0), FL(0.720), -FL(10.0)},
      {FL(6123.0), FL(0.870), -FL(10.0)},
      {FL(7755.0), FL(0.750), -FL(18.0)}},
   {  {FL(2693.0), FL(0.940),   FL(0.0)},   /* ggg NOTE:: Not Really Done Yet */
      {FL(4000.0), FL(0.720), -FL(10.0)},
      {FL(6123.0), FL(0.870), -FL(10.0)},
      {FL(7755.0), FL(0.750), -FL(18.0)}},

   {  {FL(2000.0), FL(0.700), -FL(20.0)},   /* vvv NOTE:: Not Really Done Yet */
      {FL(5257.0), FL(0.750), -FL(15.0)},
      {FL(7171.0), FL(0.840),  -FL(3.0)},
      {FL(9000.0), FL(0.900),   FL(0.0)}},
   {  { FL(100.0), FL(0.900),   FL(0.0)},   /* zzz NOTE:: Not Really Done Yet */
      {FL(4000.0), FL(0.500), -FL(20.0)},
      {FL(5500.0), FL(0.500), -FL(15.0)},
      {FL(8000.0), FL(0.400), -FL(20.0)}},
   {  {FL(2693.0), FL(0.940),   FL(0.0)},   /* thz NOTE:: Not Really Done Yet */
      {FL(4000.0), FL(0.720), -FL(10.0)},
      {FL(6123.0), FL(0.870), -FL(10.0)},
      {FL(7755.0), FL(0.750), -FL(18.0)}},
   {  {FL(2693.0), FL(0.940),   FL(0.0)},   /* zhh NOTE:: Not Really Done Yet */
      {FL(4000.0), FL(0.720), -FL(10.0)},
      {FL(6123.0), FL(0.870), -FL(10.0)},
      {FL(7755.0), FL(0.750), -FL(18.0)}}
  };

#define currentVowel (*q->control1)

void FMVoices_setFreq(FM4OPV *q, MYFLT frequency)
{
    MYFLT       temp,temp2 = FL(0.0);
    int32_t         tempi,tempi2 = 0;

    if (currentVowel < 32)      {
      tempi2 = (int32_t)currentVowel;
      temp2 = FL(0.9);
    }
    else if (currentVowel < 64) {
      tempi2 =(int32_t) currentVowel - 32;
      temp2 = FL(1.0);
    }
    else if (currentVowel < 96) {
      tempi2 = (int32_t)currentVowel - 64;
      temp2 = FL(1.1);
    }
    else if (currentVowel < 128)        {
      tempi2 = (int32_t)currentVowel - 96;
      temp2 = FL(1.2);
    }
    q->baseFreq = frequency;
    temp = (temp2 * phonParams[tempi2][0][0] / q->baseFreq) + FL(0.5);
    tempi = (int32_t) temp;
    FM4Op_setRatio((FM4OP*)q, 0, (MYFLT) tempi);
    temp = (temp2 * phonParams[tempi2][1][0] / q->baseFreq) + FL(0.5);
    tempi = (int32_t) temp;
    FM4Op_setRatio((FM4OP*)q, 1, (MYFLT) tempi);
    temp = (temp2 * phonParams[tempi2][2][0] / q->baseFreq) + FL(0.5);
    tempi = (int32_t) temp;
    FM4Op_setRatio((FM4OP*)q, 2, (MYFLT) tempi);
    q->gains[0] = FL(1.0);  /* pow(10.0f,phonParams[tempi2][0][2] * 0.05f); */
    q->gains[1] = FL(1.0);  /* pow(10.0f,phonParams[tempi2][1][2] * 0.05f); */
    q->gains[2] = FL(1.0);  /* pow(10.0f,phonParams[tempi2][2][2] * 0.05f); */
}

int32_t FMVoiceset(CSOUND *csound, FM4OPV *q)
{
    FM4OP       *p = (FM4OP *)q;
    MYFLT       amp = *q->amp * AMP_RSCALE;

    if (UNLIKELY(make_FM4Op(csound,p))) return NOTOK;
    if (UNLIKELY(FM4Op_loadWaves(csound,p))) return NOTOK;
    FM4Op_setRatio(p, 0, FL(2.00));
    FM4Op_setRatio(p, 1, FL(4.00));
    FM4Op_setRatio(p, 2, FL(12.0));
    FM4Op_setRatio(p, 3, FL(1.00));
    p->gains[3] = FM4Op_gains[80];
    ADSR_setAllTimes(csound, &p->adsr[0], FL(0.050), FL(0.050),
                     FM4Op_susLevels[15], FL(0.050));
    ADSR_setAllTimes(csound, &p->adsr[1], FL(0.050), FL(0.050),
                     FM4Op_susLevels[15], FL(0.050));
    ADSR_setAllTimes(csound, &p->adsr[2], FL(0.050), FL(0.050),
                     FM4Op_susLevels[15], FL(0.050));
    ADSR_setAllTimes(csound, &p->adsr[3], FL(0.001), FL(0.010),
                     FM4Op_susLevels[15], FL(0.500));
    p->twozero.gain = FL(0.0);
    /*     modDepth = 0.005; */
    q->tilt[0] = FL(1.0);
    q->tilt[1] = FL(0.5);
    q->tilt[2] = FL(0.2);
    q->mods[0] = FL(1.0);
    q->mods[1] = FL(1.1);
    q->mods[2] = FL(1.1);
    p->baseFreq = FL(110.0);
    FMVoices_setFreq(q, FL(110.0));
    q->tilt[0] = amp;
    q->tilt[1] = amp * amp;
    q->tilt[2] = amp * amp * amp;
    ADSR_keyOn(&p->adsr[0]);
    ADSR_keyOn(&p->adsr[1]);
    ADSR_keyOn(&p->adsr[2]);
    ADSR_keyOn(&p->adsr[3]);
    q->last_control = -FL(1.0);
    return OK;
}

int32_t FMVoice(CSOUND *csound, FM4OPV *q)
{
    FM4OP       *p = (FM4OP *)q;
    MYFLT       amp = *q->amp * AMP_RSCALE;
    MYFLT       *ar = q->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (p->baseFreq != *q->frequency || *q->control1 != q->last_control) {
      q->last_control = *q->control1;
      p->baseFreq = *q->frequency;
      FMVoices_setFreq(q, p->baseFreq);
    }
    q->tilt[0] = amp;
    q->tilt[1] = amp * amp;
    q->tilt[2] = amp * amp * amp;
    p->gains[3] = FM4Op_gains[(int32_t) (*p->control2 * FL(0.78125))];

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT   lastOutput;
      lastOutput = FM4Alg6_tick(csound,q);
      ar[n] = lastOutput*AMP_SCALE*FL(0.8);
    }

    return OK;
}

/* ********************************************************************** */

/*********************************************************/
/*  Algorithm 4 (TX81Z) Subclass of 4 Operator FM Synth  */
/*  by Perry R. Cook, 1995-96  Recoded John ffitch 97/98 */
/*                                                       */
/*  Alg 4 is :      4->3--\                              */
/*                     2-- + -->1-->Out                  */
/*                                                       */
/*  Controls: control1 = total mod index                 */
/*            control2 = crossfade of two                */
/*                          modulators                   */
/*                                                       */
/*********************************************************/

MYFLT FM4Alg4_tick(CSOUND *csound, FM4OP *p, MYFLT c1, MYFLT c2)
{
    MYFLT       temp;
    MYFLT       lastOutput;

    temp = Wave_tick(&p->v_time, (int32_t)p->vibWave->flen,
                     p->vibWave->ftable, p->v_rate, FL(0.0)) *
      *p->modDepth * FL(0.2);
    temp = p-> baseFreq * (FL(1.0) + temp)* CS_ONEDSR;
    p->w_rate[0] = p->ratios[0] * temp * p->waves[0]->flen;
    p->w_rate[1] = p->ratios[1] * temp * p->waves[1]->flen;
    p->w_rate[2] = p->ratios[2] * temp * p->waves[2]->flen;
    p->w_rate[3] = p->ratios[3] * temp * p->waves[3]->flen;

    p->w_phase[3] = p->waves[3]->flen * p->twozero.lastOutput;
    temp = p->gains[3] * ADSR_tick(&p->adsr[3]) *
      Wave_tick(&p->w_time[3], (int32_t)p->waves[3]->flen, p->waves[3]->ftable,
                p->w_rate[3], p->w_phase[3]);
    TwoZero_tick(&p->twozero, temp);
    p->w_phase[2] = p->waves[2]->flen * temp;
    temp = (FL(1.0) - (c2 * FL(0.5))) * p->gains[2] * ADSR_tick(&p->adsr[2]) *
      Wave_tick(&p->w_time[2], (int32_t)p->waves[2]->flen, p->waves[2]->ftable,
                p->w_rate[2], p->w_phase[2]);
    temp += c2 * FL(0.5) * p->gains[1] * ADSR_tick(&p->adsr[1]) *
      Wave_tick(&p->w_time[1], (int32_t)p->waves[1]->flen, p->waves[1]->ftable,
                p->w_rate[1], p->w_phase[1]);
    temp = temp * c1;
    p->w_phase[0] = p->waves[0]->flen * temp;
    temp = p->gains[0] * ADSR_tick(&p->adsr[0]) *
      Wave_tick(&p->w_time[0], (int32_t)p->waves[0]->flen, p->waves[0]->ftable,
                p->w_rate[0], p->w_phase[0]);

    lastOutput = temp * FL(0.5);
    return lastOutput;
}

int32_t percfluteset(CSOUND *csound, FM4OP *p)
{
    MYFLT       amp = *p->amp * AMP_RSCALE; /* Normalised */

    if (UNLIKELY(make_FM4Op(csound,p))) return NOTOK;
    if (UNLIKELY(FM4Op_loadWaves(csound,p)))
      return NOTOK;  /* 3 x sines; 1 x fwavblnk */

    FM4Op_setRatio(p, 0, FL(1.50)            );
    FM4Op_setRatio(p, 1, FL(3.00) * FL(0.995));
    FM4Op_setRatio(p, 2, FL(2.99) * FL(1.005));
    FM4Op_setRatio(p, 3, FL(6.00) * FL(0.997));

    p->gains[0] = amp * FM4Op_gains[99];
    p->gains[1] = amp * FM4Op_gains[71];
    p->gains[2] = amp * FM4Op_gains[93];
    p->gains[3] = amp * FM4Op_gains[85];
    ADSR_setAllTimes(csound, &p->adsr[0], FL(0.05), FL(0.05),
                     FM4Op_susLevels[14], FL(0.05));
    ADSR_setAllTimes(csound, &p->adsr[1], FL(0.02), FL(0.50),
                     FM4Op_susLevels[13], FL(0.5));
    ADSR_setAllTimes(csound, &p->adsr[2], FL(0.02), FL(0.30),
                     FM4Op_susLevels[11], FL(0.05));
    ADSR_setAllTimes(csound, &p->adsr[3], FL(0.02), FL(0.05),
                     FM4Op_susLevels[13], FL(0.01));
    p->twozero.gain = FL(0.0);
    /*     modDepth = FL(0.005); */
    ADSR_keyOn(&p->adsr[0]);
    ADSR_keyOn(&p->adsr[1]);
    ADSR_keyOn(&p->adsr[2]);
    ADSR_keyOn(&p->adsr[3]);
    return OK;
}

int32_t percflute(CSOUND *csound, FM4OP *p)
{
    MYFLT       *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       amp = *p->amp * AMP_RSCALE; /* Normalised */
    MYFLT       c1 = *p->control1;
    MYFLT       c2 = *p->control2;

    p->baseFreq = *p->frequency;
    p->gains[0] = amp * FM4Op_gains[99] * FL(0.5);
    p->gains[1] = amp * FM4Op_gains[71] * FL(0.5);
    p->gains[2] = amp * FM4Op_gains[93] * FL(0.5);
    p->gains[3] = amp * FM4Op_gains[85] * FL(0.5);
    p->v_rate = *p->vibFreq * p->vibWave->flen * CS_ONEDSR;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT   lastOutput = FM4Alg4_tick(csound, p, c1, c2);
      ar[n] = lastOutput*AMP_SCALE*FL(2.0);
    }
    return OK;
}

