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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
#define sinf(a) (MYFLT)sin((double)(a))
#define cosf(a) (MYFLT)cos((double)(a))
#define sqrtf(a) (MYFLT)sqrt((double)(a))
#define atan2f(a,b) (MYFLT)atan2((double)(a),(double)(b))
#define powf(a,b) (MYFLT)pow((double)(a),(double)(b))
#endif

/* TYPEDEFS */
typedef int64_t    len_t;    /* length type */

/* CLASS DEFINITIONS */

/* circularBuffer -- circular buffer class */
/* serves as base class for waveguides and lattice filters */
typedef struct {
  int32_t    inited;           /* Data initialization flag */
  len_t  size;             /* Size of the digital filter lattice */
  MYFLT* insertionPoint;   /* Position in queue to place new data */
  MYFLT* extractionPoint;  /* Position to read data from */
  MYFLT* data;             /* The lattice data */
  MYFLT* endPoint;         /* The end of the data */
  MYFLT* pointer;          /* pointer to current position in data */
} circularBuffer;

/* class filter -- recursive filter implementation class */
typedef struct {
  circularBuffer buffer; /* The filter's delay line */
  MYFLT* coeffs;         /* The filter's coefficients */
} filter;

/* class filter3-- JPff */
typedef struct {
  MYFLT         x1, x2;         /* Delay line */
  MYFLT         a0, a1;         /* The filter's coefficients */
} filter3;

/* filter member functions */
static void filter3Set(filter3*,MYFLT,MYFLT); /* set the coefficients */
static MYFLT filter3FIR(filter3*,MYFLT);      /* convolution filter routine */

/* waveguide rail implementation class */
typedef circularBuffer guideRail; /* It's just a circular buffer really */

/* guideRail member functions */
static MYFLT guideRailAccess(guideRail*,len_t);  /* delay line access routine */
static void guideRailUpdate(guideRail*,MYFLT);   /* delay line update routine */

/* waveguide -- abstract base class definition for waveguide classes */
typedef struct{
  int32_t
    excited;         /* excitation flag */
  guideRail upperRail; /* the right-going wave */
  guideRail lowerRail; /* the left-going wave */
  MYFLT c;             /* The tuning filter coefficient */
  MYFLT p;             /* The tuning fitler state */
  MYFLT w0;            /* The fundamental frequency (PI normalized) */
  MYFLT f0;            /* The fundamental frequency (Hertz) */
  MYFLT sr;
} waveguide;

static MYFLT filterAllpass(waveguide*,MYFLT);/* 1st-order allpass filtering*/

/* waveguide member functions */
static void waveguideWaveguide(CSOUND *, waveguide*, MYFLT, MYFLT*, MYFLT*, MYFLT);
static void waveguideSetTuning(CSOUND *,waveguide*, MYFLT); /* Set tuning filters */
#endif

