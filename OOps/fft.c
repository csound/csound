/*
    fft.c:

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
*       fft.c                                                           *
*   Fast Fourier Transform C library -                                  *
*   Based on RECrandell's. With all declarations                        *
*   dpwe 22jan90                                                        *
*   08apr90 With experimental FFT2torl - converse of FFT2real           *
*                                                                       *
*   G. Sullivan                                                         *
*   Created 'packed' versions, which accept data that is packed, and    *
*   which do not produce the (redundant) conjugate half of the result.  *
*   Apr95                                                               *
\***********************************************************************/

/*  Routines include:
    FFT2realpacked: Radix-2 FFT, with real data assumed, and accepts packed
                    data. (this routine is the fastest of the lot).
    FFT2torlpacked: Converse of FFTrealpacked
*/

#include <stdio.h>
#include "cs.h"   /* for mmalloc, mcalloc prototypes, DO NOT USE MALLOC */
#include <math.h>
#include "fft.h"
#include "prototyp.h"

static int deprecated_cnt[3] = { 1, 1, 1 };

static const char *deprecated_msg[5] = {
    "FFT2torlpacked() is deprecated. Use csoundInverseRealFFT() instead.\n",
    "FFT2realpacked() is deprecated. Use csoundRealFFT() instead.\n",
    "cxmult() is deprecated. Use csoundRealFFTMult() instead.\n"
};

#define PRINT_DEPRECATED_FFT(n)                         \
{                                                       \
    if (deprecated_cnt[n]) {                            \
      deprecated_cnt[n]--;                              \
      csoundMessage(&cenviron, Str("WARNING: "));       \
      csoundMessage(&cenviron, Str(deprecated_msg[n])); \
    }                                                   \
}

void fftRESET(void)
{
    deprecated_cnt[0] = deprecated_cnt[1] = deprecated_cnt[2] = 1;
}

int IsPowerOfTwo(long n)        /* Query whether n is a pure power of 2 */
{
    if (n < 1L || (n & (n - 1L)) != 0L)
      return 0;
    return 1;
}

void FFT2torlpacked(complex x_[], long n, MYFLT scale, complex ex[])
/* Performs FFT on data assumed complex conjugate to give purely
 * real result. Also assumes data is packed (skip == 1).
 * Expects data in a complex array size n/2 + 1.
 */
           /* scale (frigged for ugens7.c) */
           /* ex pass in lookup table */
{
    MYFLT *x;
    MYFLT scaleFac;
    int   i;
    ex = ex;
    x = (MYFLT*) x_;
    PRINT_DEPRECATED_FFT(0);
    x[1] = x[n];
    x[n] = x[n + 1] = FL(0.0);
    scaleFac = csoundGetInverseRealFFTScale(&cenviron, (int) n);
    scaleFac *= (scale * (MYFLT) n);
    if (scaleFac < FL(0.999999) || scaleFac > FL(1.000001)) {
      for (i = 0; i < n; i++)
        x[i] *= scaleFac;
    }
    csoundInverseRealFFT(&cenviron, x, (int) n);
}

void FFT2realpacked(complex x_[], long n, complex ex[])
/* Perform real FFT on packed input data. re,re,re....re(n)
 * leaving complex result in x. Assumes data is packed (skip == 1)
 * Array size must be n+2, to accomodate the nyquist rate frequency sample
 */
{
    MYFLT *x;
    ex = ex;
    x = (MYFLT*) x_;
    PRINT_DEPRECATED_FFT(1);
    csoundRealFFT(&cenviron, x, (int) n);
    x[n] = x[1];
    x[1] = x[n + 1] = FL(0.0);
}

/* Point for point complex multiply. Leaves result in array b. Does
   NOT conjugate */
void cxmult(complex *a, complex *b, long n)
{
    complex c;
    PRINT_DEPRECATED_FFT(2);
    for (; n > 0; --n) {
      c.re = (a->re * b->re) - (a->im * b->im);
      c.im = (a->im * b->re) + (a->re * b->im);
      b->re = c.re;
      b->im = c.im;
      a++;b++;
    }
}

