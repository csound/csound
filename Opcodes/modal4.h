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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
    MYFLT       w_rate;
    MYFLT       w_time;
    MYFLT       w_phaseOffset;
    MYFLT       w_lastOutput;
    BiQuad      filters[4];
    OnePole     onepole;
    FUNC        *vibr;
    MYFLT       v_rate;         /* Parameters for vibrato */
    MYFLT       v_time;
    MYFLT       v_phaseOffset;
    MYFLT       v_lastOutput;
    MYFLT       vibrGain;
    MYFLT       masterGain;
    MYFLT       directGain;
    MYFLT       baseFreq;
    MYFLT       ratios[4];
    MYFLT       resons[4];
    MYFLT       sr;
} Modal4;

void Modal4_clear(Modal4 *);
void Modal4_setFreq(CSOUND*, Modal4 *m, MYFLT frequency);
void Modal4_setRatioAndReson(CSOUND*,Modal4 *m, int32_t
                             whichOne,
                             MYFLT ratio, MYFLT reson);
#define Modal4_setMasterGain(m,Gain)    (m->masterGain = aGain)
#define Modal4_setDirectGain(m,aGain)   (m->directGain = aGain)
#define Modal4_setFiltGain(m,whichOne,gain) \
                    (BiQuad_setGain(m->filters[whichOne], gain))
/*void Modal4_strike(CSOUND *, Modal4 *m, MYFLT amplitude);
void Modal4_damp(CSOUND *, Modal4 *m, MYFLT amplitude);
MYFLT Modal4_tick(Modal4 *);*/

#endif

