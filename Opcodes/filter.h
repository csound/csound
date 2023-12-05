/*
    filter.h:

    Copyright (C) 1997 Michael A. Casey, John ffitch

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

/* filter.h */

/* Author: Michael A. Casey
 * Language: C
 * Copyright (C) 1997 Michael A. Casey, MIT Media Lab, All Rights Reserved
 *
 */

#pragma once

#ifndef __filter_h
#define __filter_h

#define MAXZEROS 50 /* Allow up to 50th-order digital filters */
#define MAXPOLES 50

/* Csound structure for FILTER opcode */

typedef struct {
  OPDS h;

  MYFLT *out;       /* output signal */
  MYFLT *in;        /* input signal */
  MYFLT *nb, *na;   /* filter-order input arguments */
  MYFLT *coeffs[MAXPOLES+MAXZEROS+1]; /* filter-coefficient input arguments */
  MYFLT *d1,*d2;    /* These allow ZFILTER to access FILTER routines */

  int32_t numa;         /* i-var p-time storage registers */
  int32_t numb;

  double dcoeffs[MAXPOLES+MAXZEROS+1]; /* filter-coefficient double arguments */
  AUXCH delay;     /* delay-line state memory base pointer */
  double* currPos;  /* delay-line current position pointer */ /* >>Was float<< */
  int32_t   ndelay;    /* length of delay line (i.e. filter order) */
} FILTER;

typedef struct {
  OPDS h;

  MYFLT *out;       /* output signal */
  MYFLT *in;        /* input signal */
  MYFLT *kmagf, *kphsf; /* magnitude and phase pole nudging factors */
  MYFLT *nb, *na;   /* filter-order input arguments */
  MYFLT *coeffs[MAXPOLES+MAXZEROS+1]; /* filter-coefficient input arguments */

  int32_t numa;         /* i-var p-time storage registers */
  int32_t numb;

  double dcoeffs[MAXPOLES+MAXZEROS+1]; /* filter-coefficient double arguments */
  AUXCH delay;     /* delay-line state memory base pointer */
  double* currPos;  /* delay-line current position pointer */ /* >>Was float<< */
  int32_t
  ndelay;    /* length of delay line (i.e. filter order) */
  AUXCH roots;     /* pole roots memory for zfilter */
} ZFILTER;

#endif
