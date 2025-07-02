/*
    nlfilt.h:

    Copyright (C) 1996 John ffitch, Richard Dobson

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

                        /* Structure for Dobson/Fitch nonlinear filter */
#pragma once

typedef struct  {
        OPDS    h;
        MYFLT   *ar, *in, *a, *b, *d, *C, *L;   /* The parameter */
        AUXCH   delay;          /* Buffer for old values */
        int32_t     point;          /* Pointer to old values */
} NLFILT;

