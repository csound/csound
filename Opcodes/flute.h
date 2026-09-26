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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
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
    cs_float       *ar;                  /* Output */
    cs_float       *amp, *frequency;
    cs_float       *jetRatio, *attack, *dettack, *noiseGain, *vibFreq;
    cs_float       *vibAmt, *ifn, *lowestFreq;
    cs_float       *jetRefl;       /* Optional 0.5 */
    cs_float       *endRefl;       /* Optional 0.5 */

    FUNC        *vibr;
    cs_float       v_rate;         /* Parameters for vibrato */
    cs_float       v_time;
/*     cs_float    v_phaseOffset; */
    DLineL      jetDelay;
    DLineL      boreDelay;
    OnePole     filter;
    DCBlock     dcBlock;
    Noise       noise;
    ADSR        adsr;
    cs_float       lastFreq;
    cs_float       lastJet;
    cs_float       maxPress;
    cs_float       vibrGain;
    cs_float       outputGain;
    cs_double      kloop;
    cs_float       lastamp;
    cs_float       limit;
} FLUTE;

#endif
