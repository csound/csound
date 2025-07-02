/*
    clfilt.h:

    Copyright (C) 2002 Erik Spjut

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

                                                        /* clfilt.h */

#pragma once

#define CL_LIM 40  /* The limit on the number of biquadratic sections */

                                /* Structure for biquadratic filter */
typedef struct {
    OPDS    h;
    MYFLT   *out, *in, *freq, *lohi, *npol, *kind, *pbr, *sbr, *reinit;
    MYFLT   xnm1[CL_LIM], xnm2[CL_LIM], ynm1[CL_LIM], ynm2[CL_LIM],
      alpha[CL_LIM], beta[CL_LIM], odelta2[CL_LIM],
      b0[CL_LIM], b1[CL_LIM], b2[CL_LIM],
      a0[CL_LIM], a1[CL_LIM], a2[CL_LIM], prvfreq;
      int32_t          ilohi, nsec, ikind;
} CLFILT;

