/*
    clarinet.h:

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
/*  Clarinet model ala Smith              */
/*  after McIntyre, Schumacher, Woodhouse */
/*  by Perry Cook, 1995-96                */
/*  Recoded for Csound by John ffitch     */
/*  November 1997                         */
/*                                        */
/*  This is a waveguide model, and thus   */
/*  relates to various Stanford Univ.     */
/*  and possibly Yamaha and other patents.*/
/*                                        */
/******************************************/

#if !defined(__Clarinet_h)
#define __Clarinet_h

#include "physutil.h"

/**********************************************/
/*  One break poinr32_t linear reed table object  */
/*  by Perry R. Cook, 1995-96                 */
/*  Consult McIntyre, Schumacher, & Woodhouse */
/*        Smith, Hirschman, Cook, Scavone,    */
/*        more for information.               */
/**********************************************/

typedef struct ReedTabl {
    MYFLT       offSet;
    MYFLT       slope;
} ReedTabl;

/*******************************************/
/*  One Zero Filter Class,                 */
/*  by Perry R. Cook, 1995-96              */
/*  The parameter gain is an additional    */
/*  gain parameter applied to the filter   */
/*  on top of the normalization that takes */
/*  place automatically.  So the net max   */
/*  gain through the system equals the     */
/*  value of gain.  sgain is the combina-  */
/*  tion of gain and the normalization     */
/*  parameter, so if you set the poleCoeff */
/*  to alpha, sgain is always set to       */
/*  gain / (1.0 - fabs(alpha)).            */
/*******************************************/

typedef struct OneZero {
    MYFLT gain;                 /* Filter subclass */
    MYFLT inputs;
    MYFLT zeroCoeff;
    MYFLT sgain;
} OneZero;

void make_OneZero(OneZero*);
MYFLT OneZero_tick(OneZero*, MYFLT);
void OneZero_setGain(OneZero*, MYFLT);
void OneZero_setCoeff(OneZero*, MYFLT);
void OneZero_print(CSOUND*, OneZero*);

/* ********************************************************************** */
typedef struct CLARIN {
    OPDS    h;
    MYFLT       *ar;                  /* Output */
    MYFLT       *amp, *frequency;
    MYFLT       *reedStffns, *attack, *dettack, *noiseGain, *vibFreq;
    MYFLT       *vibAmt, *ifn, *lowestFreq;

    FUNC        *vibr;          /* Table for vibrato */
    MYFLT       v_rate;         /* Parameters for vibrato */
    MYFLT       v_time;
/*     MYFLT    v_phaseOffset; */
    DLineL      delayLine;
    ReedTabl    reedTable;
    OneZero     filter;
    Envelope    envelope;
    Noise       noise;
    int32       length;
    MYFLT       outputGain;
    int32       kloop;
} CLARIN;

/* int32_tclarinetset(CLARINET *p); */
/* int32_tclarinet(CLARINET *p) */

#endif

