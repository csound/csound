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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/****************************************************************/
/*  dsputil.h                                                   */
/* DSP utility functions for Csound - dispfft and pvoc          */
/* Header file - just declarations                              */
/* 20apr90 dpwe                                                 */
/****************************************************************/

#define     SPTS    (16)    /* SINC TABLE: How many points in each lobe */
#define     SPDS    (6)     /*   (was 8)   How many sinc lobes to go out */
#define     SBW     0.9     /* To compensate for short sinc, reduce bandw */

#ifndef PI
#define PI      (3.14159265358979323846)
#endif
#ifndef TWOPI
#define TWOPI   (6.28318530717958647692)
#endif
#ifndef PI_F
#define PI_F    (FL(3.14159265358979323846))
#endif
#ifndef TWOPI_F
#define TWOPI_F (FL(6.28318530717958647692))
#endif

/* Predeclare static supporting functions */

void CopySamps(MYFLT *, MYFLT *, long);
MYFLT *MakeBuf(long);
void FillHalfWin(MYFLT *, long, MYFLT, int);
MYFLT *MakeHalfWin(long, MYFLT, int);
void UnpackReals(MYFLT *, long);
void PackReals(MYFLT *, long);
void Rect2Polar(MYFLT *, long);
void Polar2Rect(MYFLT *, long);
void Lin2DB(MYFLT *, long);
void DB2Lin(MYFLT *, long);
MYFLT maskPhs(MYFLT);
void UnwrapPhase(MYFLT *, long, MYFLT *);
void RewrapPhase(MYFLT *, long, MYFLT *);
void PhaseToFrq(MYFLT *, long, MYFLT, MYFLT);
void FrqToPhase(MYFLT *, long, MYFLT, MYFLT, MYFLT);
void FetchIn(MYFLT *, MYFLT *, long, MYFLT);
void FillFFTnConj(MYFLT *, long, MYFLT, int);
void ApplyHalfWin(MYFLT *, MYFLT *, long);
void addToCircBuf(MYFLT *, MYFLT *, long, long, long);
void writeClrFromCircBuf(MYFLT *, MYFLT *, long, long, long);
void FixAndAdd(MYFLT *, short *, long);
long NumFrames(long, long, long);
long NumSampls(long, long, long);
void MYFLTAndCopy(short *, MYFLT *, long);
void WriteOut(MYFLT *, MYFLT **, long);
void UDSample(MYFLT *, MYFLT, MYFLT *, long, long, MYFLT);
void MakeSinc(void);
void DestroySinc(void);   /* relase the lookup table */
MYFLT SincIntp(MYFLT); /* return interpolated sinc - not used */
void PreWarpSpec(MYFLT *, long, MYFLT);

