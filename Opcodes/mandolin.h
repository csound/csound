/*
    mandolin.h: mandolin model

    Copyright (C) 1997 John ffitch, Perry Cook

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

/**********************************************************************/
/*  Commuted Mandolin Subclass of enhanced dual plucked-string model  */
/*  by Perry Cook, 1995-96                                            */
/*   Controls:  bodySize    pluckPosition       loopGain    deTuning  */
/*                                                                    */
/*  Note: Commuted Synthesis, as with many other WaveGuide techniques,*/
/*  is covered by patents, granted, pending, and/or applied-for.  All */
/*  are assigned to the Board of Trustees, Stanford University.       */
/*  For information, contact the Office of Technology Licensing,      */
/*  Stanford U.                                                       */
/**********************************************************************/

#if !defined(__Mandolin_h)
#define __Mandolin_h
#include "clarinet.h"
#include "brass.h"

typedef struct Mandolin {
    OPDS        h;
    MYFLT       *ar;                  /* Output */
    MYFLT       *amp;
    MYFLT       *frequency;
    MYFLT       *pluckPos;
    MYFLT       *detuning;
    MYFLT       *baseLoopGain;
    MYFLT       *s_rate;
    MYFLT       *ifn;
    MYFLT       *lowestFreq;

    FUNC        *soundfile;
    MYFLT       s_time;
    MYFLT       s_lastOutput;
    DLineA      delayLine1;
    DLineA      delayLine2;
    DLineL      combDelay;
    OneZero     filter1;
    OneZero     filter2;
    int32       length;
    MYFLT       lastFreq;
    MYFLT       lastLength;
    int32       dampTime;
    int32_t         waveDone;
    int32_t
    kloop;
} MANDOL;

#endif
