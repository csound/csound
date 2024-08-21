/*
    dsputil.h:

    Copyright (C) 1991 Dan Ellis, Barry Vercoe, John ffitch,

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

/****************************************************************/
/*  dsputil.h                                                   */
/* DSP utility functions for Csound - dispfft and pvoc          */
/* Header file - just declarations                              */
/* 20apr90 dpwe                                                 */
/****************************************************************/

#pragma once

#define     SPTS    (16)    /* SINC TABLE: How many points in each lobe   */
#define     SPDS    (6)     /*   (was 8)   How many sinc lobes to go out  */
#define     SBW     0.9     /* To compensate for short sinc, reduce bandw */

/* Predeclare static supporting functions */

void    Polar2Real_PVOC(CSOUND *, MYFLT *, CSOUND_FFT_SETUP *);
void    RewrapPhase(MYFLT *, int32, MYFLT *);
void    FrqToPhase(MYFLT *, int32, MYFLT, MYFLT, MYFLT);
void    FetchIn(float *, MYFLT *, int32, MYFLT);
void    ApplyHalfWin(MYFLT *, MYFLT *, int32);
void    addToCircBuf(MYFLT *, MYFLT *, int32, int32, int32);
void    writeClrFromCircBuf(MYFLT *, MYFLT *, int32, int32, int32);
void    UDSample(PVOC_GLOBALS *, MYFLT *, MYFLT, MYFLT *, int32, int32, MYFLT);
void    MakeSinc(PVOC_GLOBALS *);
void    PreWarpSpec(MYFLT *, int32, MYFLT, MYFLT *);

