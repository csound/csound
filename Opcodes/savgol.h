/*
    savgol.h:

    Copyright (C) 2026 Pasquale Mainolfi

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
    Savitzky-Golay smoothing and differentiation filter.

        ares savgol    asig, iwinsize, iorder [, ideriv] [, idelta]
        kres savgol    ksig, iwinsize, iorder [, ideriv] [, idelta]
        imat[][] savgolmat iwinsize, iorder [, idelta]

    A polynomial of degree iorder is fitted, in the least-squares sense, to
    the iwinsize samples of a sliding window, and the output is that
    polynomial evaluated at the centre of the window. Because the fit is
    linear in the samples the whole operation collapses to a fixed FIR
    kernel, computed once at init time.

    Compared with a plain lowpass of similar bandwidth the fit preserves the
    height and width of peaks, which is what makes the filter useful for
    envelopes, control signals and any material where the shape of a
    transient matters more than the stopband depth.

    ideriv selects which derivative of the fitted polynomial to output:
    0 (the default) smooths, 1 gives the slope, 2 the curvature, and so on
    up to iorder. idelta is the spacing between samples and defaults to 1,
    so derivatives come out per sample; pass 1/sr at a-rate or 1/kr at
    k-rate to get them per second.

    iwinsize must be odd and at least 3, iorder must be less than iwinsize,
    ideriv must not exceed iorder, and idelta must be positive. Since the
    result belongs to the centre of the window, the output lags the input by
    (iwinsize - 1) / 2 samples at a-rate, or the same number of k-cycles at
    k-rate. The history starts at zero, so the first (iwinsize - 1) outputs
    are the filter's response to a signal that was silent beforehand.

    savgolmat returns the whole kernel bank instead of filtering: an
    (iorder + 1) by iwinsize array whose row d holds the coefficients for
    the d-th derivative, already scaled by d! / idelta^d. Row 0 is the
    smoothing kernel; for iwinsize 5 and iorder 2 it is the familiar
    [-3, 12, 17, 12, -3] / 35.
*/


#ifndef __SAVGOL_H
#define __SAVGOL_H


#include "csdl.h"
#include "csound.h"
#include "sysdep.h"
#include <stdint.h>


/* Savitzky-Golay coefficient matrix C = (A^T A)^-1 A^T, row-major.
   Row d holds the least-squares coefficients of the d-th derivative,
   before the d! / delta^d scaling applied by get_coeffs(). */
typedef struct {
    double *coeffs;
    uint32_t nrows; // order + 1
    uint32_t ncols; // winsize
} SAVGOL_BUFFER;

typedef struct {
    double *data;
    double *transposed;
    double *normal;
    double *inversed;
    double *pinversed;
} TEMP_BUFFER;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *y;
    // inputs;
    MYFLT *signal;
    MYFLT *winsize; // must be odd
    MYFLT *order;   // order + 1 coefficient rows
    MYFLT *deriv;
    MYFLT *delta;
    // private
    AUXCH coeffs;     // winsize scaled doubles for the requested derivative
    AUXCH buffer_mem; // a-rate: (winsize - 1) history samples + ksmps
                      // k-rate: winsize samples used as a ring buffer
    MYFLT *buffer_ptr;
    uint32_t winsize_i;
    uint32_t write_pos; // k-rate only: next ring slot to overwrite
} SAVGOL;

typedef struct {
    OPDS h;
    // outputs
    ARRAYDAT *mat;
    // inputs;
    MYFLT *winsize;
    MYFLT *order;
    MYFLT *delta;
} SAVGOL_MATRIX;


int32_t savgol_audio_init(CSOUND *csound, SAVGOL *p);
int32_t savgol_control_init(CSOUND *csound, SAVGOL *p);
int32_t savgol_audio_perf(CSOUND *csound, SAVGOL *p);
int32_t savgol_control_perf(CSOUND *csound, SAVGOL *p);
int32_t savgol_matrix(CSOUND *csound, SAVGOL_MATRIX *p); // i-rate

#endif
