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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
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
    cs_float       *ar;                  /* Output */
    cs_float       *amp;
    cs_float       *frequency;
    cs_float       *pluckPos;
    cs_float       *detuning;
    cs_float       *baseLoopGain;
    cs_float       *s_rate;
    cs_float       *ifn;
    cs_float       *lowestFreq;

    FUNC        *soundfile;
    cs_float       s_time;
    cs_float       s_lastOutput;
    DLineA      delayLine1;
    DLineA      delayLine2;
    DLineL      combDelay;
    OneZero     filter1;
    OneZero     filter2;
    int32       length;
    cs_float       lastFreq;
    cs_double      lastLength;
    cs_float       lastDetune;
    cs_float       lastPluck;
    int32       dampTime;
    int32_t         waveDone;
} MANDOL;

#endif
