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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"
#include "dam.h"
#include <math.h>

#define SQRT2 FL(1.41421356237309504880168872421)

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

static int daminit(CSOUND *csound, DAM *p)
{
   /* Initialise gain value */

    p->gain = FL(1.0);

   /* Compute the gain speed changes from parameter given by Csound */
   /* the computed values are stored in the opcode data structure p */
   /* for later use in the main processing                          */

    p->rspeed = (*p->rtime)*csound->onedsr*FL(1000.0);
    p->fspeed = (*p->ftime)*csound->onedsr*FL(1000.0);
    p->kthr = -FL(1.0);
    return OK;
}

/*
 * Run-time computation code
 */

static int dam(CSOUND *csound, DAM *p)
{
    int i;
    MYFLT *ain,*aout;
    MYFLT threshold;
    MYFLT gain;
    MYFLT comp1,comp2;
    MYFLT *powerPos;
    MYFLT *powerBuffer;
    MYFLT power;
    MYFLT tg;
    int nsmps = csound->ksmps;

    /* Initialize power value and buffer at first ksamp computed as
     * it depends on kthreshold
     */
    if (p->kthr < FL(0.0)) {
      MYFLT x = (p->kthr = *(p->kthreshold))/(MYFLT)POWER_BUFSIZE;
      p->power = p->kthr;
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
    powerPos    = p->powerPos;
    powerBuffer = p->powerBuffer;
    power       = p->power;

 /* Process ksmps samples */

    for (i=0;i<nsmps;i++) {

        /* Estimates the current power level */

      *powerPos = FABS(ain[i])/(MYFLT)(POWER_BUFSIZE*SQRT2);
      power    += (*powerPos++);
      if ((powerPos-powerBuffer)==POWER_BUFSIZE) {
        powerPos = p->powerBuffer;
      }
      power -= (*powerPos);

      /* Looks where the power is related to the threshold
         and compute target gain */

      if (power>threshold) {
        tg = ((power-threshold)*comp1+threshold)/power;
      }
      else {
        tg = threshold*(POWER((power/threshold),
                                     FL(1.0)/comp2))/power;
      }

      /* move gain toward target */

      if (gain<tg) {
        gain += p->rspeed;
      }
      else {
        gain -= p->fspeed;
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
{ "dam",     S(DAM),     5,     "a",    "akiiii",(SUBR)daminit, NULL, (SUBR)dam },
};

int dam_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

