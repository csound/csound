/*
    shaker.h:

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

/******************************************/
/*  Maracha (& other shakers) Simulation  */
/*  by Perry R. Cook, 1996                */
/*  by Perry Cook, 1995-96                */
/*                                        */
/*  See ICMC96 paper "PhISM: Percussive   */
/*  Synthesis." for more exciting details */
/******************************************/

#if !defined(__Shaker_h)
#define __Shaker_h

#include "physutil.h"

typedef struct Shaker {
    OPDS        h;
    MYFLT       *ar;                  /* Output */
    MYFLT       *amp, *kfreq;
    MYFLT       *beancount, *shake_damp;
    MYFLT       *times, *dettack;

    BiQuad      filter;
    ADSR        envelope;
    int32_t         num_beans;
    int32_t         wait_time;
    int32_t         shake_num;
    MYFLT       shake_speed;    /* A + amp*N -- hides two parameters */
    MYFLT       res_freq;
    MYFLT       coll_damp;
    MYFLT       shakeEnergy;
    MYFLT       noiseGain;
    MYFLT       gain_norm;
    int32_t
    kloop;
    MYFLT       freq;
} SHAKER;

#endif

