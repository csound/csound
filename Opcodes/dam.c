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

int daminit(DAM *p)
{
    int i;

   /* Initialise gain value */

    p->gain = FL(1.0);

   /* Compute the gain speed changes from parameter given by Csound */
   /* the computed values are stored in the opcode data structure p */
   /* for later use in the main processing                          */

    p->rspeed = (*p->rtime)*onedsr*FL(1000.0);
    p->fspeed = (*p->ftime)*onedsr*FL(1000.0);

   /* Initialize power value and buffer */

    p->power = *(p->kthreshold);
    for (i=0;i<POWER_BUFSIZE;i++) {
      p->powerBuffer[i] = p->power/(MYFLT)POWER_BUFSIZE;
    }

    p->powerPos = p->powerBuffer;
    return OK;
}

/*
 * Run-time computation code
 */

int dam(DAM *p)
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

    for (i=0;i<ksmps_;i++) {

        /* Estimates the current power level */

      *powerPos = (MYFLT)(fabs(ain[i]))/(MYFLT)(POWER_BUFSIZE*sqrt(2.0));
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
        tg = threshold*(MYFLT)(pow((double)(power/threshold),
                                 1.0/(double)comp2))/power;
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


#define S       sizeof

static OENTRY localops[] = {
{ "dam",     S(DAM),     5,     "a",    "akiiii",(SUBR)daminit, NULL, (SUBR)dam },
};

LINKAGE
