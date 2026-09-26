/*
    fm4op.h:

    Copyright (C) 1998 Perry Cook, John ffitch

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

/*******************************************/
/*  Master Class for 4 Operator FM Synth   */
/*  by Perry R. Cook, 1995-96              */
/*  This instrument contains an 4 waves,   */
/*  4 envelopes, and various state vars.   */
/*                                         */
/*  The basic Chowning/Stanford FM patent  */
/*  expired April 1995, but there exist    */
/*  follow-on patents, mostly assigned to  */
/*  Yamaha.  If you are of the type who    */
/*  should worry about this (making money) */
/*  worry away.                            */
/*                                         */
/*******************************************/

#if !defined(__FM4OP_h)
#define __FM4OP_h

#include "physutil.h"

/*******************************************/
/*  Two Zero Filter Class,                 */
/*  by Perry R. Cook, 1995-96              */
/*  See books on filters to understand     */
/*  more about how this works.  Nothing    */
/*  out of the ordinary in this version.   */
/*******************************************/

typedef struct TwoZero {
    cs_float gain;
    cs_float lastOutput;
    cs_float inputs[2];
    cs_float zeroCoeffs[2];
} TwoZero;

/* ********************************************************************** */

typedef struct FM4OP {
    OPDS        h;
    cs_float       *ar;                  /* Output */
    cs_float       *amp, *frequency;
    cs_float       *control1, *control2, *modDepth; /* Control1 doubles as vowel */
    cs_float       *vibFreq;
    cs_float       *ifn0, *ifn1, *ifn2, *ifn3, *vifn;
    cs_float       *opt;
    ADSR        adsr[4];
    FUNC        *waves[4];
    cs_float       w_rate[4];         /* Parameters for vibrato */
    cs_float       w_time[4];
    cs_float       w_phase[4];
    FUNC        *vibWave;
    cs_float       v_rate;         /* Parameters for vibrato */
    cs_float       v_time;
/*     cs_float    v_phaseOffset; */
    TwoZero     twozero;
    cs_float       baseFreq;
    cs_float       ratios[4];
    cs_float       gains[4];
} FM4OP;

typedef struct FM4OPV {
    FM4OP       fm;
    cs_float       tilt[3];
    cs_float       mods[3];
    cs_float       last_control;
} FM4OPV;

#endif
