#ifndef FFT_H
#define FFT_H
/*
    fft.h:

    Copyright (C) 1990, 1995 Dan Ellis, Greg Sullivan

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

/***********************************************************************\
*       fft.h                                                           *
*   Fast Fourier Transform C library - header file                      *
*   Based on RECrandell's                                               *
*   dpwe 22jan90                                                        *
*   08apr90 With experimental FFT2torl - converse of FFT2real           *
\***********************************************************************/

#ifndef MYFLT
#include "sysdep.h"
#endif

typedef struct {
    MYFLT re, im;
} complex;

int IsPowerOfTwo(long);
void FFT2torlpacked(complex *, long, MYFLT, complex *);
void FFT2realpacked(complex *, long, complex *);
void cxmult(complex *,complex *,long);

#endif /* FFT_H */

