/*
    brass.h:

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
/*  Simple Brass Instrument Model ala     */
/*  Cook (TBone, HosePlayer)              */
/*  by Perry R. Cook, 1995-96             */
/*                                        */
/*  This is a waveguide model, and thus   */
/*  relates to various Stanford Univ.     */
/*  and possibly Yamaha and other patents.*/
/*                                        */
/*   Controls:    CONTROL1 = lipTension   */
/*                CONTROL2 = slideLength  */
/*                CONTROL3 = vibFreq      */
/*                MOD_WHEEL= vibAmt       */
/******************************************/

#if !defined(__brass_h)
#define __brass_h

#include "physutil.h"

#include <math.h>

/*******************************************/
/*                                         */
/*  AllPass Interpolating Delay Line       */
/*  Object by Perry R. Cook 1995-96        */
/*  This one uses a delay line of maximum  */
/*  length specified on creation, and      */
/*  interpolates fractional length using   */
/*  an all-pass filter.  This version is   */
/*  more efficient for computing static    */
/*  length delay lines (alpha and coeff    */
/*  are computed only when the length      */
/*  is set, there probably is a more       */
/*  efficient computational form if alpha  */
/*  is changed often (each sample)).       */
/*                                         */
/*******************************************/

typedef struct DLineA {
    AUXCH       inputs;
    cs_float       lastOutput;
    int32        inPoint;
    int32        outPoint;
    int32        length;
    cs_float       alpha;
    cs_float       coeff;
    cs_float       lastIn;
} DLineA;

void make_DLineA(CSOUND *,DLineA *, int32 max_length);
/* void DLineA_clear(DLineA *); */
int32_t DLineA_setDelay(CSOUND *,DLineA *, cs_float length);
cs_float DLineA_tick(DLineA *, cs_float sample);

/***********************************************/
/*  Lip Filter Object by Perry R. Cook, 1995-96*/
/*  The lip of the brass player has dynamics   */
/*  which are controlled by the mass, spring   */
/*  constant, and damping of the lip.  This    */
/*  filter simulates that behavior and the     */
/*  transmission/reflection properties as      */
/*  well.  See Cook TBone and HosePlayer       */
/*  instruments and articles.                  */
/***********************************************/

typedef BiQuad LipFilt;

void make_LipFilt(LipFilt*);
void LipFilt_clear(LipFilt*);
//void LipFilt_setFreq(CSOUND*,LipFilt*, cs_float frequency);
cs_float LipFilt_tick(LipFilt*, cs_float mouthSample,cs_float boreSample);
cs_float LipFilt_lastOut(LipFilt*);

/* ---------------------------------------------------------------------- */
typedef struct BRASS {
    OPDS        h;
    cs_float       *ar;                  /* Output */
    cs_float       *amp, *frequency;
    cs_float       *liptension, *dettack;
    cs_float       *vibFreq, *vibAmt, *ifn, *lowestFreq;

    FUNC        *vibr;          /* Table for vibrato */
    cs_float       v_rate;         /* Parameters for vibrato */
    cs_float       v_time;
/*     cs_float    v_phaseOffset; */
    DLineA      delayLine;
    LipFilt     lipFilter;
    DCBlock     dcBlock;
    ADSR        adsr;
    int32       length;
    cs_float       slideTarget;
    cs_float       maxPressure;
    cs_float       lastamp;
    cs_float       lipTarget;
    cs_float       frq;            /* Remember previous value */
    cs_float       lipT;           /* and lip tension */
    cs_float       limit;
    cs_double      kloop;
} BRASS;

#endif
