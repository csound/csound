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

/*
    To call an FFT, one must first assign the complex exponential factors:
    A call such as
        e = AssignBasis(NULL, size)
    will set up the complex *e to be the array of cos, sin pairs corresponding
    to total number of complex data = size.   This call allocates the
    (cos, sin) array memory for you.  If you already have such memory
    allocated, pass the allocated pointer instead of NULL.
 */

#ifndef MYFLT
#include "sysdep.h"
#endif

typedef struct {
    MYFLT re, im;
} complex;

typedef struct lnode {
    struct lnode *next;
    long          size;
    complex      *table;
} LNODE;

void putcomplexdata(complex *, long);
void ShowCpx(complex *, long, char *);
int PureReal(complex *, long);
int IsPowerOfTwo(long);
complex *FindTable(long);       /* search our list of existing LUT's */
complex *AssignBasis(complex *, long);
void reverseDig(complex *, long, int);
void reverseDigpacked(complex *, long);
void FFT2dimensional(complex *, long, long, complex *);
void FFT2torl(complex *, long, int, MYFLT, complex *);
void FFT2torlpacked(complex *, long, MYFLT, complex *);
void ConjScale(complex *, long, MYFLT);
void FFT2real(complex *, long, int, complex *);
void FFT2realpacked(complex *, long, complex *);
void Reals(complex *, long, int, int, complex *);
void Realspacked(complex *, long,int, complex *);
void FFT2(complex *, long, int, complex *);
void FFT2raw(complex *, long, int, int, complex *);
void FFT2rawpacked(complex *, long, int, complex *);
void FFTarb(complex *, complex *, long, complex *);
void DFT(complex *, complex *, long, complex *);
void cxmult(complex *,complex *,long);

#endif /* FFT_H */
