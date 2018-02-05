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
  MYFLT lastOutput;
  int32 inPoint;
  int32 outPoint;
  int32 length;
} DLINEN;

typedef struct BowedBar {
    OPDS        h;
    MYFLT       *ar;                  /* Output */
    MYFLT       *amp, *frequency, *position, *bowPress, *GAIN;
    MYFLT       *integration_const, *trackVel, *bowposition, *lowestFreq;

    BowTabl     bowTabl;
    ADSR        adsr;
    BiQuad      bandpass[NR_MODES];

    MYFLT       maxVelocity;
    MYFLT       modes[4];
    DLINEN      delay[4];
/*      MYFLT   Zs[4][2]; */
/*      MYFLT   coeffs[4][2]; */
/*      MYFLT   filtOut[4]; */
/*      MYFLT   filtIn[4]; */
/*      MYFLT   filtGain[4]; */
    MYFLT       freq;
    int32_t         nr_modes;       /* Usually 4 */
    int32_t         length;
    MYFLT       gains[4];
    MYFLT       velinput;
    MYFLT       bowvel, bowTarg, lastBowPos;
    MYFLT       lastpos;
/*      int32_t             pluck; */
    MYFLT       lastpress;
    int32_t         kloop;
} BOWEDBAR;

#endif
