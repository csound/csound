/*
    flute.h:

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
/*  WaveGuide Flute ala Karjalainen,      */
/*  Smith, Waryznyk, etc.                 */
/*  with polynomial Jet ala Cook          */
/*  by Perry Cook, 1995-96                */
/*  Recoded for Csound by John ffitch     */
/*  November 1997                         */
/*                                        */
/*  This is a waveguide model, and thus   */
/*  relates to various Stanford Univ.     */
/*  and possibly Yamaha and other patents.*/
/*                                        */
/******************************************/

#if !defined(__Flute_h)
#define __Flute_h

#include "physutil.h"

/**********************************************/
/* Jet Table Object by Perry R. Cook, 1995-96 */
/* Consult Fletcher and Rossing, Karjalainen, */
/*       Cook, more, for information.         */
/* This, as with many other of my "tables",   */
/* is not a table, but is computed by poly-   */
/* nomial calculation.                        */
/**********************************************/

typedef struct FLUTE {
    OPDS        h;
    MYFLT       *ar;                  /* Output */
    MYFLT       *amp, *frequency;
    MYFLT       *jetRatio, *attack, *dettack, *noiseGain, *vibFreq;
    MYFLT       *vibAmt, *ifn, *lowestFreq;
    MYFLT       *jetRefl;       /* Optional 0.5 */
    MYFLT       *endRefl;       /* Optional 0.5 */

    FUNC        *vibr;
    MYFLT       v_rate;         /* Parameters for vibrato */
    MYFLT       v_time;
/*     MYFLT    v_phaseOffset; */
    DLineL      jetDelay;
    DLineL      boreDelay;
    OnePole     filter;
    DCBlock     dcBlock;
    Noise       noise;
    ADSR        adsr;
    MYFLT       lastFreq;
    MYFLT       lastJet;
    MYFLT       maxPress;
    MYFLT       vibrGain;
    MYFLT       outputGain;
    MYFLT       kloop;
    MYFLT       lastamp;
    MYFLT       limit;
} FLUTE;

#endif
