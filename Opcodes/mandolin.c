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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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

static inline int32_t infoTick(MANDOL *p)
{
    int32 temp;
    MYFLT temp_time, alpha;
    int32_t allDone = 0;

    p->s_time += *p->s_rate;    /*  Update current time          */

    if (p->s_time >= (MYFLT)p->soundfile->flen) { /*  Check for end of sound */
      p->s_time = (MYFLT)(p->soundfile->flen-1L); /*  stick at end      */
      allDone = 1;                 /* Information for one-shot use  */
    }
    else if (p->s_time < FL(0.0))  /*  Check for end of sound       */
      p->s_time = FL(0.0);         /*  stick at beg                 */

    temp_time = p->s_time;

    temp = (int32) temp_time;       /*  Integer part of time address */
    alpha = temp_time - (MYFLT) temp; /*  fractional part of time address */
    p->s_lastOutput =              /* Do linear interpolation       */
      FL(0.05)*((MYFLT*)(p->soundfile->ftable))[temp];
    p->s_lastOutput = p->s_lastOutput + /*  same as alpha*data[temp+1]      */
        (alpha * FL(0.05)*(((MYFLT*)(p->soundfile->ftable))[temp+1] -
                p->s_lastOutput)); /*  + (1-alpha)data[temp]           */
    /* p->s_lastOutput *= FL(0.51);*/ /* Scaling hack; see below, and changed */
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
    if (*p->lowestFreq>=FL(0.0)) {      /* Skip initialisation */
      if (*p->lowestFreq!=FL(0.0)) {
        p->length = (int32) (CS_ESR / (*p->lowestFreq * FL(0.9)) + FL(1.0));
      }
      else if (LIKELY(*p->frequency!=FL(0.0))) {
        p->length = (int32) (CS_ESR / *p->frequency + FL(1.0));
      }
      else {
        csound->Warning(csound, "%s", Str("No base frequency for mandolin"));
        p->length = (int32) (CS_ESR / FL(50.0) + FL(1.0));
      }
      p->lastFreq = FL(50.0);
/*     p->baseLoopGain = 0.995; */
/*     p->loopGain = 0.999; */
      make_DLineA(csound,&p->delayLine1, p->length);
      make_DLineA(csound,&p->delayLine2, p->length);
      make_DLineL(csound,&p->combDelay, p->length);
      make_OneZero(&p->filter1);
      make_OneZero(&p->filter2);
      //      p->lastLength = p->length * FL(0.5);
/*    soundfile->normalize(0.05);    Empirical hack here transferred to use  */
      p->lastLength = ( CS_ESR / p->lastFreq);        /* length - delays */
/*    DLineA_setDelay(&p->delayLine1, (p->lastLength / *p->detuning) - 0.5f); */
/*    DLineA_setDelay(&p->delayLine2, (p->lastLength * *p->detuning) - 0.5f); */

                           /* this function gets interesting here, */
      p->s_time = FL(0.0); /* because pluck may be longer than     */
                           /* string length, so we just reset the  */
                           /* soundfile and add in the pluck in    */
                           /* the tick method.                     */
                           /* Set Pick Position                    */
      DLineL_setDelay(&p->combDelay, FL(0.5) * *p->pluckPos * p->lastLength);
                           /*   which puts zeroes at pos*length    */
      p->dampTime = (int32) p->lastLength; /* See tick method below */
      p->waveDone = 0;
      {
        int32_t relestim = (int32_t)(CS_EKR * FL(0.1));
        /* 1/10th second decay extention */
        if (relestim > p->h.insdshead->xtratim)
          p->h.insdshead->xtratim = relestim;
      }
      p->kloop = (int32_t)(p->h.insdshead->offtim * CS_EKR);  /* ??? */
    }
    return OK;
}

int32_t mandolin(CSOUND *csound, MANDOL *p)
{
    MYFLT *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT amp = (*p->amp)*AMP_RSCALE; /* Normalise */
    MYFLT lastOutput;
    MYFLT loopGain;

    loopGain = *p->baseLoopGain + (p->lastFreq * FL(0.000005));
    if (loopGain>FL(1.0)) loopGain = FL(0.99999);

    if (p->kloop>0 && p->h.insdshead->relesing) p->kloop=1;

    if (p->lastFreq != *p->frequency) {
      p->lastFreq = *p->frequency;
      p->lastLength = ( CS_ESR / p->lastFreq);        /* length - delays */
      DLineA_setDelay(csound, &p->delayLine1,
                              (p->lastLength / *p->detuning) - FL(0.5));
      DLineA_setDelay(csound, &p->delayLine2,
                              (p->lastLength * *p->detuning) - FL(0.5));
    }

    if ((--p->kloop) == 0) {
        loopGain = (FL(1.0) - amp) * FL(0.5);
    }

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT temp = FL(0.0);
      if (!p->waveDone) {
        p->waveDone = infoTick(p);       /* as long as it goes . . .   */
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
      ar[n] = lastOutput*AMP_SCALE;
    }
    return OK;
}

