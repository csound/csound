/*
    wavegde.h:

    Copyright (C) 1994 Michael A. Casey, John ffitch

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

/* waveguide.h -- primitive data types and declarations for waveguides */

/*
 * Code conversion from C++ to C (October 1994)
 * Author: Michael A. Casey MIT Media Labs
 * Language: C
 * Copyright (c) 1994 MIT Media Lab, All Rights Reserved
 */

#ifndef _waveguide_h
#define _waveguide_h

#include <math.h>
#ifndef sinf
#define sinf(a) (cs_float)sin((cs_double)(a))
#define cosf(a) (cs_float)cos((cs_double)(a))
#define sqrtf(a) (cs_float)sqrt((cs_double)(a))
#define atan2f(a,b) (cs_float)atan2((cs_double)(a),(cs_double)(b))
#define powf(a,b) (cs_float)pow((cs_double)(a),(cs_double)(b))
#endif

/* TYPEDEFS */
typedef int64_t    len_t;    /* length type */

/* CLASS DEFINITIONS */

/* circularBuffer -- circular buffer class */
/* serves as base class for waveguides and lattice filters */
typedef struct {
  int32_t    inited;           /* Data initialization flag */
  len_t  size;             /* Size of the digital filter lattice */
  cs_float* insertionPoint;   /* Position in queue to place new data */
  cs_float* extractionPoint;  /* Position to read data from */
  cs_float* data;             /* The lattice data */
  cs_float* endPoint;         /* The end of the data */
  cs_float* pointer;          /* pointer to current position in data */
} circularBuffer;

/* class filter -- recursive filter implementation class */
typedef struct {
  circularBuffer buffer; /* The filter's delay line */
  cs_float* coeffs;         /* The filter's coefficients */
} filter;

/* class filter3-- JPff */
typedef struct {
  cs_float         x1, x2;         /* Delay line */
  cs_float         a0, a1;         /* The filter's coefficients */
} filter3;

/* filter member functions */
static void filter3Set(filter3*,cs_float,cs_float); /* set the coefficients */
static cs_float filter3FIR(filter3*,cs_float);      /* convolution filter routine */

/* waveguide rail implementation class */
typedef circularBuffer guideRail; /* It's just a circular buffer really */

/* guideRail member functions */
static inline cs_float guideRailAccess(guideRail*,len_t);  /* delay line access routine */
static void guideRailUpdate(guideRail*,cs_float);   /* delay line update routine */

/* waveguide -- abstract base class definition for waveguide classes */
typedef struct{
  int32_t
    excited;         /* excitation flag */
  guideRail upperRail; /* the right-going wave */
  guideRail lowerRail; /* the left-going wave */
  cs_float c;             /* The tuning filter coefficient */
  cs_float p;             /* The tuning fitler state */
  cs_float w0;            /* The fundamental frequency (PI normalized) */
  cs_float f0;            /* The fundamental frequency (Hertz) */
  cs_float sr;
} waveguide;

static cs_float filterAllpass(waveguide*,cs_float);/* 1st-order allpass filtering*/

/* waveguide member functions */
static void waveguideWaveguide(CSOUND *, waveguide*, cs_float, cs_float*, cs_float*, cs_float);
static void waveguideSetTuning(CSOUND *,waveguide*, cs_float); /* Set tuning filters */
#endif

