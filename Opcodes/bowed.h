/*
    bowed.h:

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

/******************************************/
/*  Bowed String model ala Smith          */
/* after McIntyre, Schumacher, Woodhouse  */
/*  by Perry Cook, 1995-96                */
/*  Recoded for Csound by John ffitch     */
/*  November 1997                         */
/*                                        */
/*  This is a waveguide model, and thus   */
/*  relates to various Stanford Univ.     */
/*  and possibly Yamaha and other patents.*/
/*                                        */
/******************************************/

#if !defined(__Bowed_h)
#define __Bowed_h

#include "physutil.h"

/***********************************************/
/*  Simple Bow Table Object, after Smith       */
/*    by Perry R. Cook, 1995-96                */
/***********************************************/

typedef struct BowTabl {
    MYFLT       offSet;
    MYFLT       slope;
    MYFLT       lastOutput;
} BowTabl;

MYFLT BowTabl_lookup(CSOUND *,BowTabl*, MYFLT sample);

typedef struct BOWED {
    OPDS        h;
    MYFLT       *ar;                  /* Output */
    MYFLT       *amp, *frequency;
    MYFLT       *bowPress, *betaRatio, *vibFreq;
    MYFLT       *vibAmt, *ifn, *lowestFreq;

    FUNC        *vibr;
    MYFLT       v_rate;         /* Parameters for vibrato */
    MYFLT       v_time;
    MYFLT       v_phaseOffset;
    MYFLT       v_lastOutput;
    DLineL      neckDelay;
    DLineL      bridgeDelay;
    BowTabl     bowTabl;
    OnePole     reflFilt;
    BiQuad      bodyFilt;
    ADSR        adsr;
    MYFLT       maxVelocity;
    MYFLT       baseDelay;
    MYFLT       vibrGain;
    MYFLT       lastpress;
    MYFLT       lastfreq;
    MYFLT       lastbeta;
    MYFLT       lastamp;
    MYFLT       limit;
    int         kloop;
} BOWED;

#endif

