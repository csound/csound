/*
    remap.h:

    Copyright (C) 2026 Pasquale Mainolfi.

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

/*
    remap - shared declarations. See remap.c for what the opcode does.

    The interpolation mode and the out-of-bounds policy reach the opcode from
    the orchestra as plain integers, so the two enums below fix that encoding:
    their numeric values are part of the opcode interface and must not be
    reordered or renumbered.

    LERP_INTERVAL carries the outcome of one table lookup: the bracketing
    breakpoints, the index of the segment they delimit, and how the bounds
    policy resolved the request. PCHIP_SEGMENT holds the cubic Hermite
    coefficients of one segment, kept apart from the evaluation so that the
    array and audio versions can reuse them across consecutive inputs landing
    in the same segment.

    REMAP_VALUE backs the scalar and the audio overloads, REMAP_VEC the array
    ones.
*/


#ifndef __REMAP_H
#define __REMAP_H

#include "csdl.h"
#include "csound.h"
#include "sysdep.h"
#include <stdint.h>


typedef enum {
    REMAP_LINEAR   = 0,
    REMAP_NEAREST  = 1,
    REMAP_PREVIOUS = 2,
    REMAP_NEXT     = 3,
    REMAP_CUBIC    = 4
} INTERP_MODE;

typedef enum {
    REMAP_ERROR       = 0,
    REMAP_CLAMP       = 1,
    REMAP_FILL        = 2,
    REMAP_EXTRAPOLATE = 3
} INTERP_BOUNDS;

typedef enum {
    REMAP_VALID = 0,
    REMAP_NOT_VALID,
    REMAP_CLAMP_LEFT,
    REMAP_CLAMP_RIGHT,
    REMAP_FILL_VALUE,
    REMAP_EXTRAPOLATE_LEFT,
    REMAP_EXTRAPOLATE_RIGHT
} INTERVAL_BOUNDS_MODE;

typedef struct{
   double x0;
   double x1;
   double y0;
   double y1;
   int32_t index;
   INTERVAL_BOUNDS_MODE bmode;
} LERP_INTERVAL;

/* cubic Hermite coefficients of a single PCHIP segment, relative to x0 */
typedef struct {
    double x0;
    double a;
    double b;
    double c;
    double d;
} PCHIP_SEGMENT;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *y;
    // inputs
    MYFLT *x;
    ARRAYDAT *xdata;
    ARRAYDAT *ydata;
    MYFLT *mode;
    MYFLT *bounds;
    MYFLT *fill;
    // private
    double fill_value;
    int32_t imode;
    int32_t ibounds;
    /* set when both data arrays are i-rate: the breakpoints cannot change
       during performance, so they are validated once at init */
    int32_t static_data;
} REMAP_VALUE;

typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *y;
    // inputs
    ARRAYDAT *x;
    ARRAYDAT *xdata;
    ARRAYDAT *ydata;
    MYFLT *mode;
    MYFLT *bounds;
    MYFLT *fill;
    // private
    double fill_value;
    int32_t imode;
    int32_t ibounds;
    int32_t static_data;
} REMAP_VEC;

// REMAP

int32_t remap_value(CSOUND *csound, REMAP_VALUE *p);
int32_t remap_vec(CSOUND *csound, REMAP_VEC *p);
int32_t remap_audio(CSOUND *csound, REMAP_VALUE *p);

#endif
