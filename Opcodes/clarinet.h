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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
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
    cs_float       offSet;
    cs_float       slope;
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
    cs_float gain;                 /* Filter subclass */
    cs_float inputs;
    cs_float zeroCoeff;
    cs_float sgain;
} OneZero;

void make_OneZero(OneZero*);
cs_float OneZero_tick(OneZero*, cs_float);
void OneZero_setGain(OneZero*, cs_float);
void OneZero_setCoeff(OneZero*, cs_float);
void OneZero_print(CSOUND*, OneZero*);

/* ********************************************************************** */
typedef struct CLARIN {
    OPDS    h;
    cs_float       *ar;                  /* Output */
    cs_float       *amp, *frequency;
    cs_float       *reedStffns, *attack, *dettack, *noiseGain, *vibFreq;
    cs_float       *vibAmt, *ifn, *lowestFreq;

    FUNC        *vibr;          /* Table for vibrato */
    cs_float       v_rate;         /* Parameters for vibrato */
    cs_float       v_time;
/*     cs_float    v_phaseOffset; */
    DLineL      delayLine;
    ReedTabl    reedTable;
    OneZero     filter;
    Envelope    envelope;
    Noise       noise;
    int32       length;
    cs_float       outputGain;
    cs_double      kloop;
    int32_t     attackPending;
} CLARIN;

/* int32_tclarinetset(CLARINET *p); */
/* int32_tclarinet(CLARINET *p) */

#endif

