/*
    pluck.h:

    Copyright (C) 1994, 2000 Michael A. Casey, John ffitch

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

/* pluck.h -- plucked string class declarations */

/*
 * Code conversion from C++ to C (October 1994)
 * Author: Michael A. Casey MIT Media Labs
 * Language: C
 * Copyright (c) 1994 MIT Media Lab, All Rights Reserved
 * Some modifications by John ffitch, 2000, simplifying code
 */

#ifndef _pluck_h
#define _pluck_h

#include "wavegde.h"

/* pluck -- derived class to implement simple plucked string algorithm */
typedef struct {
  OPDS h;
  MYFLT *out;                   /* plucked string output */
  MYFLT *freq,*amp,*pickupPos,*pickPos,*Aw0,*AwPI,*afdbk; /* inputs */

  waveguide wg;                 /* general waveguide model structure   */
  filter3 bridge;               /* lowpass bridge filter               */
  len_t pickSamp;               /* where to pluck the string           */

  /* Auxillary memory allocation */
  AUXCH upperData;              /* upper rail data space */
  AUXCH lowerData;              /* lower rail data space */
  AUXCH bridgeCoeffs;           /* Bridge filter coefficients */
  AUXCH bridgeData;             /* Bridge filter lattice data */
} WGPLUCK;

#endif

