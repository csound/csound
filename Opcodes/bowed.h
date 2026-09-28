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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
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
    cs_float       offSet;
    cs_float       slope;
    cs_float       lastOutput;
} BowTabl;

cs_float BowTabl_lookup(CSOUND *,BowTabl*, cs_float sample);

typedef struct BOWED {
    OPDS        h;
    cs_float       *ar;                  /* Output */
    cs_float       *amp, *frequency;
    cs_float       *bowPress, *betaRatio, *vibFreq;
    cs_float       *vibAmt, *ifn, *lowestFreq;

    FUNC        *vibr;
    cs_float       v_rate;         /* Parameters for vibrato */
    cs_float       v_time;
    cs_float       v_phaseOffset;
    cs_float       v_lastOutput;
    DLineL      neckDelay;
    DLineL      bridgeDelay;
    BowTabl     bowTabl;
    OnePole     reflFilt;
    BiQuad      bodyFilt;
    ADSR        adsr;
    cs_float       maxVelocity;
    cs_float       baseDelay;
    cs_float       vibrGain;
    cs_float       lastpress;
    cs_float       lastfreq;
    cs_float       lastbeta;
    cs_float       lastamp;
    cs_float       limit;
} BOWED;

#endif

