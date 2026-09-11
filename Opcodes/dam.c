/*
    dam.c:

    Copyright (C) 1997 Marc Resibois

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

#include "stdopcod.h"
#include "dam.h"
#include <math.h>


/*
 *   Dynamic Amplitude Modifier.
 *
 *       (C) Marc Resibois 1997
 *
 *   I place this source code in the public domain. Just
 *   let me know if you do something you like with it ;-)
 *
 *   For bugs, question, please write to Marc.Resibois@ping.be
 */

/*
 *      Initialisation code
 */

static int32_t daminit(CSOUND *csound, DAM *p)
{
   /* Initialise gain value */

    p->gain = FL(1.0);

   /* Compute the gain speed changes from parameter given by Csound */
   /* the computed values are stored in the opcode data structure p */
   /* for later use in the main processing                          */

    p->rspeed = *p->rtime > FL(0.0) ? CS_ONEDSR / *p->rtime
                                  : (MYFLT)INFINITY;
    p->fspeed = *p->ftime > FL(0.0) ? CS_ONEDSR / *p->ftime
                                  : (MYFLT)INFINITY;
    p->kthr = -FL(1.0);
    return OK;
}

/*
 * Run-time computation code
 */

static int32_t dam(CSOUND *csound, DAM *p)
{
     IGN(csound);
    MYFLT *ain,*aout;
    MYFLT threshold;
    MYFLT gain;
    MYFLT comp1,comp2;
    MYFLT exponent;
    MYFLT *powerPos;
    MYFLT *powerBuffer;
    double power;
    MYFLT tg;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    /* Initialize power value and buffer at first ksamp computed as
     * it depends on kthreshold
     */
    if (p->kthr < FL(0.0)) {
      MYFLT x = (p->kthr = *(p->kthreshold))/(MYFLT)POWER_BUFSIZE;
      p->power = (double)x * POWER_BUFSIZE;
      /* Initialise table as threshhold changed */
      for (i=0;i<POWER_BUFSIZE;i++) {
        p->powerBuffer[i] = x;
      }
      p->powerPos = p->powerBuffer;
    }

    ain         = p->ain;
    aout        = p->aout;
    threshold   = *(p->kthreshold);
    gain        = p->gain;
    comp1       = *(p->icomp1);
    comp2       = *(p->icomp2);
    exponent    = comp2 != FL(0.0) ? FL(1.0)/comp2 - FL(1.0)
                                  : (MYFLT)INFINITY;
    powerPos    = p->powerPos;
    powerBuffer = p->powerBuffer;
    power       = p->power;

 /* Process ksmps samples */
    if (UNLIKELY(offset)) memset(aout, '\0', offset*sizeof(MYFLT));
     if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&aout[nsmps], '\0', early*sizeof(MYFLT));
    }
   for (i=offset;i<nsmps;i++) {

        /* Estimates the current power level */

      power -= *powerPos;
      *powerPos = FABS(ain[i])/(MYFLT)(POWER_BUFSIZE*ROOT2);
      power    += (*powerPos++);
      if ((powerPos-powerBuffer)==POWER_BUFSIZE) {
        powerPos = p->powerBuffer;
      }
      if (power < FL(0.0)) power = FL(0.0);

      /* Looks where the power is related to the threshold
         and compute target gain */

      if (power>threshold) {
        tg = comp1 + (FL(1.0)-comp1)*(threshold/power);
      }
      else if (power > FL(0.0)) {
        tg = POWER(power/threshold, exponent);
      }
      else {
        /* Compression tends to zero gain at silence; unity stays unity.
           Expansion has no finite limit, so retain its current gain. */
        tg = comp2 < FL(1.0) ? FL(0.0)
             : comp2 == FL(1.0) ? FL(1.0) : gain;
      }

      /* move gain toward target */

      if (gain<tg) {
        gain += p->rspeed;
        if (gain>tg) gain = tg;
      }
      else if (gain>tg) {
        gain -= p->fspeed;
        if (gain<tg) gain = tg;
      }

      /* compute output */

      aout[i] = ain[i]*gain;
    }

    /* Store the last gain value for next call */

    p->gain     = gain;
    p->power    = power;
    p->powerPos = powerPos;

    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "dam",     S(DAM),  0,     "a",    "akiiii",(SUBR)daminit, (SUBR)dam },
};

int32_t dam_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}

