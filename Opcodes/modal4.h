/*
    modal4.h:

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

/*******************************************/
/*  4 Resonance Modal Synthesis Instrument */
/*  by Perry R. Cook, 1995-96              */
/*  This instrument contains an excitation */
/*  wavetable, an envelope, and four reso- */
/*  nances (Non-Sweeping BiQuad Filters).  */
/*******************************************/

#if !defined(__Modal4_h)
#define __Modal4_h

#include "physutil.h"

typedef struct Modal4 {
    OPDS h;
    Envelope envelope;
    FUNC        *wave;
/*     int32_t              w_looping; */
    int32_t         w_myData;
    int32_t         w_allDone;
    cs_float       w_rate;
    cs_float       w_time;
    cs_float       w_phaseOffset;
    cs_float       w_lastOutput;
    BiQuad      filters[4];
    OnePole     onepole;
    FUNC        *vibr;
    cs_double      v_rate;         /* Parameters for vibrato */
    cs_double      v_time;
    cs_float       v_phaseOffset;
    cs_float       v_lastOutput;
    cs_float       vibrGain;
    cs_float       masterGain;
    cs_float       directGain;
    cs_float       baseFreq;
    cs_float       ratios[4];
    cs_float       resons[4];
    cs_float       sr;
} Modal4;

void Modal4_clear(Modal4 *);
void Modal4_setFreq(CSOUND*, Modal4 *m, cs_float frequency);
void Modal4_setRatioAndReson(CSOUND*,Modal4 *m, int32_t
                             whichOne,
                             cs_float ratio, cs_float reson);
#define Modal4_setMasterGain(m,Gain)    (m->masterGain = aGain)
#define Modal4_setDirectGain(m,aGain)   (m->directGain = aGain)
#define Modal4_setFiltGain(m,whichOne,gain) \
                    (BiQuad_setGain(m->filters[whichOne], gain))
/*void Modal4_strike(CSOUND *, Modal4 *m, cs_float amplitude);
void Modal4_damp(CSOUND *, Modal4 *m, cs_float amplitude);
cs_float Modal4_tick(Modal4 *);*/

#endif

