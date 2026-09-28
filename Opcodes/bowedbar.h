/*
    bowedbar.h:

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/*********************************************/
/*  Bowed Bar model                          */
/*  by Georg Essl, 1999                      */
/*  For details refer to:                    */
/*    G.Essl, P.R.Cook: "Banded Waveguides:  */
/*    Towards Physical Modelling of Bar      */
/*    Percussion Instruments", ICMC'99       */
/*********************************************/

#if !defined(__BowedBar_h)
#define __BowedBar_h
#define NR_MODES (4)

#include "physutil.h"
#include "bowed.h"

/*******************************************/
/*  Non-Interpolating Delay Line           */
/*  Object by Perry R. Cook 1995-96.       */
/*  Revised by Gary Scavone, 1999.         */
/*                                         */
/*  This one uses either a delay line of   */
/*  maximum length specified on creation   */
/*  or a default length of 2048 samples.   */
/*  A non-interpolating delay line is      */
/*  typically used in non-time varying     */
/*  (reverb) applications.                 */
/*******************************************/

typedef struct DLineN {
  AUXCH inputs;
  cs_float lastOutput;
  int32 inPoint;
  int32 outPoint;
  int32 length;
} DLINEN;

typedef struct BowedBar {
    OPDS        h;
    cs_float       *ar;                  /* Output */
    cs_float       *amp, *frequency, *position, *bowPress, *GAIN;
    cs_float       *integration_const, *trackVel, *bowposition, *lowestFreq;

    BowTabl     bowTabl;
    ADSR        adsr;
    BiQuad      bandpass[NR_MODES];

    cs_float       maxVelocity;
    cs_float       modes[4];
    DLINEN      delay[4];
/*      cs_float   Zs[4][2]; */
/*      cs_float   coeffs[4][2]; */
/*      cs_float   filtOut[4]; */
/*      cs_float   filtIn[4]; */
/*      cs_float   filtGain[4]; */
    cs_float       freq;
    int32_t         nr_modes;       /* Usually 4 */
    int32_t         length;
    cs_float       gains[4];
    cs_float       velinput;
    cs_float       bowvel, bowTarg, lastBowPos;
    cs_float       lastpos;
/*      int32_t             pluck; */
    cs_float       lastpress;
    int32_t         kloop;
} BOWEDBAR;

#endif
