/*
    mandolin.c: code for physical model of mandolin

    Copyright (C) 1997 John ffitch, Perry Cook

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

/********************************************/
/*  Commuted Mandolin Subclass of enhanced  */
/*  dual plucked-string model               */
/*  by Perry Cook, 1995-96                  */
/*   Controls:    CONTROL1 = bodySize       */
/*                CONTROL2 = pluckPosition  */
/*                CONTROL3 = loopGain       */
/*                MOD_WHEEL= deTuning       */
/*                                          */
/*  Note: Commuted Synthesis, as with many  */
/*  other WaveGuide techniques, is covered  */
/*  by patents, granted, pending, and/or    */
/*  applied-for.  Many are assigned to the  */
/*  Board of Trustees, Stanford University. */
/*  For information, contact the Office of  */
/*  Technology Licensing, Stanford U.       */
/********************************************/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "mandolin.h"

static inline int32_t infoTick(MANDOL *p, MYFLT rate)
{
    int32 temp;
    MYFLT temp_time, alpha;
    int32_t allDone = 0;

    p->s_time += rate;    /*  Update current time          */

    if (p->s_time >= (MYFLT)p->soundfile->flen) { /*  Check for end of sound */
      p->s_time = (MYFLT)(p->soundfile->flen-1L); /*  stick at end      */
      allDone = 1;                 /* Information for one-shot use  */
    }
    else if (p->s_time < FL(0.0))  /*  Check for end of sound       */
      p->s_time = FL(0.0);         /*  stick at beg                 */

    temp_time = p->s_time;

    temp = (int32) temp_time;       /*  Integer part of time address */
    alpha = temp_time - (MYFLT) temp; /*  fractional part of time address */
    p->s_lastOutput = FL(0.05) *
      (p->soundfile->ftable[temp] + alpha *
       (p->soundfile->ftable[temp+1] - p->soundfile->ftable[temp]));
    return allDone;
}

/* Suggested values pluckAmp = 0.3; pluckPos = 0.4; detuning = 0.995; */
int32_t mandolinset(CSOUND *csound, MANDOL *p)
{
    FUNC *ftp;

    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL))
      p->soundfile = ftp;
    else {                                      /* Expect pluck wave */
      return csound->InitError(csound, "%s", Str("No table for Mandolin"));
    }
    if (*p->lowestFreq>=FL(0.0)) {      /* Skip initialisation if negative. */
      double frequency = *p->lowestFreq;
      double length;
      if (frequency == 0.0) {
        frequency = *p->frequency;
        if (frequency == 0.0) {
          csound->Warning(csound, "%s", Str("No base frequency for mandolin"));
          frequency = 50.0;
        }
      }
      /* Allow the documented detuning range down to 0.9. */
      length = CS_ESR / (frequency * 0.9) + 1.0;
      if (UNLIKELY(!(length >= 3.0 && length <= INT32_MAX)))
        return csound->InitError(csound, "%s",
                                 Str("Invalid minimum mandolin frequency"));
      p->length = (int32_t) length;
      p->lastFreq = FL(0.0);
      p->lastDetune = FL(0.0);
      p->lastPluck = FL(-1.0);
      make_DLineA(csound,&p->delayLine1, p->length);
      make_DLineA(csound,&p->delayLine2, p->length);
      make_DLineL(csound,&p->combDelay, p->length);
      make_OneZero(&p->filter1);
      make_OneZero(&p->filter2);
      p->lastLength = 0.0;  /* Force setup on the first control cycle. */
      p->s_time = FL(0.0);
      p->s_lastOutput = FL(0.0);
      p->dampTime = 0;
      p->waveDone = 0;
      {
        int32_t relestim = (int32_t)(CS_EKR * FL(0.1));
        /* 1/10th second decay extention */
        if (relestim > p->h.insdshead->xtratim)
          p->h.insdshead->xtratim = relestim;
      }
    }
    else if (UNLIKELY(p->length == 0))
      return csound->InitError(csound, "%s", Str("mandolin: not initialised"));
    return OK;
}

/* Control-rate delay updates stay within the allocated string length. */
static MYFLT mandolin_delay(double delay, int32_t length)
{
    if (delay < 0.5) return FL(0.5);
    if (delay > length - 1.0) return (MYFLT)(length - 1);
    return (MYFLT) delay;
}

int32_t mandolin(CSOUND *csound, MANDOL *p)
{
    MYFLT *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT fullscale = AMP_SCALE;
    MYFLT amp = *p->amp * (FL(1.0) / fullscale);
    MYFLT lastOutput;
    MYFLT loopGain;
    MYFLT frequency = *p->frequency;
    MYFLT detune = *p->detuning;
    MYFLT rate = *p->s_rate;
    int32_t frequencyChanged = p->lastLength == 0.0 || frequency != p->lastFreq;

    if (frequencyChanged) {
      double period = CS_ESR / (double) frequency;
      if (UNLIKELY(!(period > 0.0 && period <= INT32_MAX)))
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("Invalid mandolin frequency"));
      if (p->lastLength == 0.0)
        p->dampTime = (int32_t) period;
      p->lastLength = period;
      p->lastFreq = frequency;
    }
    if (frequencyChanged || detune != p->lastDetune) {
      if (UNLIKELY(!(detune > FL(0.0))))
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("Invalid mandolin detuning"));
      DLineA_setDelay(csound, &p->delayLine1,
                     mandolin_delay(p->lastLength / detune - 0.5, p->length));
      DLineA_setDelay(csound, &p->delayLine2,
                     mandolin_delay(p->lastLength * detune - 0.5, p->length));
      p->lastDetune = detune;
    }
    if (frequencyChanged || *p->pluckPos != p->lastPluck) {
      MYFLT pluck = *p->pluckPos;
      if (UNLIKELY(!(pluck >= FL(0.0) && pluck <= FL(1.0))))
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("Invalid mandolin pluck position"));
      DLineL_setDelay(&p->combDelay, (MYFLT)
                     fmin(0.5 * pluck * p->lastLength, p->length - 1.0));
      p->lastPluck = pluck;
    }

    loopGain = *p->baseLoopGain + frequency * FL(0.000005);
    if (loopGain>FL(1.0)) loopGain = FL(0.99999);
    if (p->h.insdshead->relesing)
      loopGain = (FL(1.0) - amp) * FL(0.5);

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT temp = FL(0.0);
      if (!p->waveDone) {
        p->waveDone = infoTick(p, rate);       /* as long as it goes . . .   */
        temp = p->s_lastOutput * amp;    /* scaled pluck excitation    */
        temp = temp - DLineL_tick(&p->combDelay, temp);/* with comb filtering */
      }
      if (p->dampTime>=0) {              /* Damping hack to help avoid */
        p->dampTime -= 1;                /*   overflow on replucking   */
        lastOutput =
          DLineA_tick(&p->delayLine1, /* Calculate 1st delay */
                      OneZero_tick(&p->filter1, /*  filterered reflection */
                                   temp + /*  plus pluck excitation    */
                                   (p->delayLine1.lastOutput * FL(0.7))));
        lastOutput +=
          DLineA_tick(&p->delayLine2, /* and 2nd delay just like the 1st */
                      OneZero_tick(&p->filter2,
                                   temp
                                   + (p->delayLine2.lastOutput * FL(0.7))));
                              /* that's the whole thing!!        */
      }
      else {                  /*  No damping hack after 1 period */
        lastOutput =
          DLineA_tick(&p->delayLine1, /* Calculate 1st delay */
                      OneZero_tick(&p->filter1,   /*  filtered reflection  */
                                   temp +    /*  plus pluck excitation     */
                                   (p->delayLine1.lastOutput * loopGain)));
        lastOutput +=
          DLineA_tick(&p->delayLine2,  /* and 2nd delay      */
                      OneZero_tick(&p->filter2, /*  just like the 1st */
                                   temp +
                                   (p->delayLine2.lastOutput * loopGain)));
      }
      lastOutput *= FL(3.7);
      ar[n] = lastOutput*fullscale;
    }
    return OK;
}

