/*
    savgol.c:

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
    Savitzky-Golay filter. See savgol.h for the opcode interface.

    Fitting a degree-o polynomial to the w samples of a window, in the
    least-squares sense, means solving A * p = y for the coefficient vector
    p, where y holds the samples and A is the Vandermonde design matrix

        A[i][j] = (i - centre)^j,   i in [0, w),  j in [0, o],

    with the abscissae centred so that evaluating the fit at the middle of
    the window is just reading p back. The least-squares solution is
    p = C * y with C the pseudo-inverse

        C = (A^T A)^-1 A^T,

    an (o + 1) by w matrix that depends only on w and o, never on the
    samples. So the fit costs one dot product per output: row d of C
    convolved with the window yields p[d], and the d-th derivative of the
    fit at the centre is d! * p[d] / delta^d. calculate_savgol_coeffs()
    builds C once at init time and get_coeffs() folds that scaling into the
    single row the perf pass needs.

    The normal equations are inverted by Gauss-Jordan elimination with
    partial pivoting. A^T A inherits the poor conditioning of the
    Vandermonde matrix, but it is only (o + 1) square and o stays small in
    practice, so the pivoting is enough and the cost is irrelevant next to
    an init-time allocation.

    At perf time both variants are a direct convolution, w multiply-adds per
    output sample, which stays well below the cost of the surrounding
    orchestra for any window a musician would choose. They differ only in
    how the history is kept: the a-rate variant keeps the w - 1 carried
    samples immediately ahead of the current block so the window is
    contiguous for every sample in it, while the k-rate variant, producing
    one sample per call, uses a ring of w samples instead. Either way the
    oldest sample comes first, matching the order of the coefficients.
*/


#include "Opcodes/savgol.h"
#include "arrays.h"
#include "coreDefs.h"
#include "csound.h"
#include "sysdep.h"
#include <csdl.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>


static void matrix_mult(const double *a, const double *b, double *c, uint32_t row_a, uint32_t col_a, uint32_t col_b) {
    for (uint32_t i = 0; i < row_a; i++) {
        for (uint32_t j = 0; j < col_b; j++) {
            double sum = 0.0;
            for (uint32_t k = 0; k < col_a; k++) {
                sum += a[i * col_a + k] * b[k * col_b + j];
            }
            c[i * col_b + j] = sum;
        }
    }
}

static int32_t matrix_inverse(CSOUND *csound, const double *matrix, double *inverse, uint32_t n) {
    double *work = (double *) csound->Calloc(csound, sizeof(double) * (size_t) n * n);

    for (uint32_t i = 0; i < n; ++i) {
        for (uint32_t j = 0; j < n; ++j) {
            work[i * n + j] = matrix[i * n + j];
            inverse[i * n + j] = (i == j) ? 1.0 : 0.0;
        }
    }

    for (uint32_t col = 0; col < n; ++col) {
        uint32_t pivot_row = col;
        double max_value = fabs(work[col * n + col]);

        for (uint32_t row = col + 1; row < n; ++row) {
            double value = fabs(work[row * n + col]);

            if (value > max_value) {
                max_value = value;
                pivot_row = row;
            }
        }

        if (max_value <= 1.0e-12) {
            csound->Free(csound, work);
            return NOTOK;
        }

        if (pivot_row != col) {
            for (uint32_t j = 0; j < n; ++j) {
                double tmp;

                tmp = work[col * n + j];
                work[col * n + j] = work[pivot_row * n + j];
                work[pivot_row * n + j] = tmp;

                tmp = inverse[col * n + j];
                inverse[col * n + j] = inverse[pivot_row * n + j];
                inverse[pivot_row * n + j] = tmp;
            }
        }

        double pivot = work[col * n + col];

        for (uint32_t j = 0; j < n; ++j) {
            work[col * n + j] /= pivot;
            inverse[col * n + j] /= pivot;
        }

        for (uint32_t row = 0; row < n; ++row) {
            if (row == col) continue;

            double factor = work[row * n + col];
            for (uint32_t j = 0; j < n; ++j) {
                work[row * n + j] -= factor * work[col * n + j];
                inverse[row * n + j] -= factor * inverse[col * n + j];
            }
        }
    }

    csound->Free(csound, work);
    return OK;
}

static void deallocate_temp_buffer(CSOUND *csound, TEMP_BUFFER *buffer) {
    csound->Free(csound, buffer->data);
    csound->Free(csound, buffer->transposed);
    csound->Free(csound, buffer->normal);
    csound->Free(csound, buffer->inversed);
    csound->Free(csound, buffer->pinversed);
    memset(buffer, 0, sizeof(*buffer));
}

static void allocate_temp_buffer(CSOUND *csound, TEMP_BUFFER *buffer, uint32_t winsize, uint32_t ncoef) {
    size_t design = sizeof(double) * (size_t) winsize * ncoef;
    size_t square = sizeof(double) * (size_t) ncoef * ncoef;

    buffer->data       = (double *) csound->Calloc(csound, design);
    buffer->transposed = (double *) csound->Calloc(csound, design);
    buffer->normal     = (double *) csound->Calloc(csound, square);
    buffer->inversed   = (double *) csound->Calloc(csound, square);
    buffer->pinversed  = (double *) csound->Calloc(csound, design);
}

/* Fills sg->coeffs (ncoef * winsize doubles, allocated by the caller) with
   the pseudo-inverse of the Vandermonde design matrix. */
static int32_t calculate_savgol_coeffs(CSOUND *csound, SAVGOL_BUFFER *sg, uint32_t winsize, uint32_t ncoef) {
    TEMP_BUFFER temp = {0};
    allocate_temp_buffer(csound, &temp, winsize, ncoef);

    // Vandermonde matrix: A[i][j] = (i - center)^j
    double center = (double) (winsize - 1U) * 0.5;
    double value = 1.0;
    for (uint32_t i = 0; i < winsize; i++) {
        double x = (double) i - center;
        for (uint32_t j = 0; j < ncoef; j++) {
            value = j == 0 ? 1.0 : value * x;
            temp.data[i * ncoef + j] = value;
            temp.transposed[j * winsize + i] = value;
        }
    }

    // normal = A^T * A
    matrix_mult(temp.transposed, temp.data, temp.normal, ncoef, winsize, ncoef);

    if (matrix_inverse(csound, temp.normal, temp.inversed, ncoef) != OK) {
        deallocate_temp_buffer(csound, &temp);
        return NOTOK;
    }

    // pinversed = (A^T A)^-1 * A^T
    matrix_mult(temp.inversed, temp.transposed, temp.pinversed, ncoef, ncoef, winsize);
    memcpy(sg->coeffs, temp.pinversed, sizeof(double) * (size_t) ncoef * winsize);
    sg->nrows = ncoef;
    sg->ncols = winsize;

    deallocate_temp_buffer(csound, &temp);
    return OK;
}

static double factorial(uint32_t n) {
    double fac = 1.0;
    for (uint32_t i = 2; i <= n; i++) {
        fac *= (double) i;
    }

    return fac;
}

/* Row deriv of the coefficient matrix, scaled by deriv! / delta^deriv so that
   the convolution yields the deriv-th derivative of the fitted polynomial. */
static void get_coeffs(const SAVGOL_BUFFER *sg, double *coeffs_buffer, uint32_t deriv, double delta) {
    double scale = factorial(deriv) / pow(delta, (double) deriv);
    const double *row = sg->coeffs + (size_t) deriv * sg->ncols;
    for (uint32_t i = 0; i < sg->ncols; i++) {
        coeffs_buffer[i] = row[i] * scale;
    }
}

static int32_t validate_params(CSOUND *csound, MYFLT winsize_arg, MYFLT order_arg, MYFLT delta_arg, uint32_t *winsize, uint32_t *ncoef) {
    int32_t w = (int32_t) winsize_arg;
    int32_t o = (int32_t) order_arg;

    if (UNLIKELY(w < 3 || (w & 1) == 0)) {
        return csound->InitError(csound, "[savgol] winsize must be odd and at least 3");
    }

    if (UNLIKELY(o < 0 || o >= w)) {
        return csound->InitError(csound, "[savgol] order must be >= 0 and less than winsize");
    }

    if (UNLIKELY(delta_arg <= FL(0.0))) {
        return csound->InitError(csound, "[savgol] delta must be greater than 0");
    }

    *winsize = (uint32_t) w;
    *ncoef = (uint32_t) o + 1U;
    return OK;
}

/* Shared i-time work: validate, build the coefficient matrix and store the
   single scaled row the perf pass convolves with. Leaves the sample history
   to the caller, whose layout differs between the a- and k-rate variants. */
static int32_t savgol_setup(CSOUND *csound, SAVGOL *p) {
    uint32_t winsize, ncoef;
    int32_t res = validate_params(csound, *p->winsize, *p->order, *p->delta, &winsize, &ncoef);
    if (UNLIKELY(res != OK)) {
        return res;
    }

    int32_t deriv = (int32_t) *p->deriv;
    if (UNLIKELY(deriv < 0 || (uint32_t) deriv >= ncoef)) {
        return csound->InitError(csound, "[savgol] deriv must be >= 0 and <= order");
    }

    SAVGOL_BUFFER sg = { NULL, 0, 0 };
    sg.coeffs = (double *) csound->Calloc(csound, sizeof(double) * (size_t) ncoef * winsize);

    if (UNLIKELY(calculate_savgol_coeffs(csound, &sg, winsize, ncoef) != OK)) {
        csound->Free(csound, sg.coeffs);
        return csound->InitError(csound, "[savgol] could not compute savgol coefficients");
    }

    csound->AuxAlloc(csound, sizeof(double) * (size_t) winsize, &p->coeffs);
    get_coeffs(&sg, (double *) p->coeffs.auxp, (uint32_t) deriv, (double) *p->delta);
    csound->Free(csound, sg.coeffs);

    p->winsize_i = winsize;
    return OK;
}

int32_t savgol_audio_init(CSOUND *csound, SAVGOL *p) {
    int32_t res = savgol_setup(csound, p);
    if (UNLIKELY(res != OK)) {
        return res;
    }

    csound->AuxAlloc(csound, sizeof(MYFLT) * (size_t) ((p->winsize_i - 1U) + CS_KSMPS), &p->buffer_mem);
    p->buffer_ptr = (MYFLT *) p->buffer_mem.auxp;

    return OK;
}

int32_t savgol_audio_perf(CSOUND *csound, SAVGOL *p) {
    (void) csound;

    MYFLT *out = p->y;
    const MYFLT *in = p->signal;
    const double *coeffs = (const double *) p->coeffs.auxp;
    uint32_t winsize = p->winsize_i;
    uint32_t win_offset = winsize - 1U;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) {
        memset(out, 0, sizeof(MYFLT) * (size_t) offset);
    }

    if (UNLIKELY(early)) {
        nsmps -= early;
        memset(out + nsmps, 0, sizeof(MYFLT) * (size_t) early);
    }

    memcpy(p->buffer_ptr + win_offset, in, sizeof(MYFLT) * (size_t) nsmps);

    /* buffer_ptr[i .. i + winsize - 1] runs oldest to newest, matching the
       coefficient order; out[i] is the fit at the window centre, so the
       opcode has a latency of (winsize - 1) / 2 samples. */
    for (uint32_t i = offset; i < nsmps; i++) {
        double sum = 0.0;
        for (uint32_t j = 0; j < winsize; j++) {
            sum += coeffs[j] * (double) p->buffer_ptr[i + j];
        }
        out[i] = (MYFLT) sum;
    }

    memmove(p->buffer_ptr, p->buffer_ptr + nsmps, sizeof(MYFLT) * (size_t) win_offset);

    return OK;
}

int32_t savgol_control_init(CSOUND *csound, SAVGOL *p) {
    int32_t res = savgol_setup(csound, p);
    if (UNLIKELY(res != OK)) {
        return res;
    }

    csound->AuxAlloc(csound, sizeof(MYFLT) * (size_t) p->winsize_i, &p->buffer_mem);
    p->buffer_ptr = (MYFLT *) p->buffer_mem.auxp;
    p->write_pos = 0;

    return OK;
}

int32_t savgol_control_perf(CSOUND *csound, SAVGOL *p) {
    (void) csound;

    const double *coeffs = (const double *) p->coeffs.auxp;
    MYFLT *buffer = p->buffer_ptr;
    uint32_t winsize = p->winsize_i;
    uint32_t oldest = p->write_pos;

    buffer[oldest] = *p->signal;
    if (++oldest >= winsize) oldest = 0;
    p->write_pos = oldest;

    /* The slot due to be overwritten next holds the oldest sample, so walking
       from it wraps around into the newest one, matching the coefficient
       order. The ring starts zeroed, which makes the first winsize - 1 outputs
       the same zero-padded transient the a-rate variant produces. Two
       contiguous runs keep the wrap out of the inner loop. */
    uint32_t head = winsize - oldest;
    double sum = 0.0;

    for (uint32_t i = 0; i < head; i++) {
        sum += coeffs[i] * (double) buffer[oldest + i];
    }

    for (uint32_t i = 0; i < oldest; i++) {
        sum += coeffs[head + i] * (double) buffer[i];
    }

    *p->y = (MYFLT) sum;
    return OK;
}

int32_t savgol_matrix(CSOUND *csound, SAVGOL_MATRIX *p) {
    uint32_t winsize, ncoef;
    int32_t res = validate_params(csound, *p->winsize, *p->order, *p->delta, &winsize, &ncoef);
    if (UNLIKELY(res != OK)) {
        return res;
    }

    SAVGOL_BUFFER sg = { NULL, 0, 0 };
    sg.coeffs = (double *) csound->Calloc(csound, sizeof(double) * (size_t) ncoef * winsize);

    if (UNLIKELY(calculate_savgol_coeffs(csound, &sg, winsize, ncoef) != OK)) {
        csound->Free(csound, sg.coeffs);
        return csound->InitError(csound, "[savgol] could not compute savgol coefficients");
    }

    /* tabinit only maintains sizes[] for 1-D arrays, so the 2-D shape is set
       here; sizes must already be valid when tabinit sees dimensions > 1. */
    if (p->mat->dimensions != 2) {
        p->mat->sizes = (int32_t *) csound->ReAlloc(csound, p->mat->sizes, sizeof(int32_t) * 2);
        p->mat->dimensions = 2;
    }
    p->mat->sizes[0] = (int32_t) sg.nrows;
    p->mat->sizes[1] = (int32_t) sg.ncols;
    tabinit(csound, p->mat, (int32_t) (ncoef * winsize), p->h.insdshead);

    MYFLT *out = (MYFLT *) p->mat->data;
    double *row = (double *) csound->Calloc(csound, sizeof(double) * (size_t) sg.ncols);

    for (uint32_t i = 0; i < sg.nrows; i++) {
        get_coeffs(&sg, row, i, (double) *p->delta);
        for (uint32_t j = 0; j < sg.ncols; j++) {
            out[i * sg.ncols + j] = (MYFLT) row[j];
        }
    }

    csound->Free(csound, row);
    csound->Free(csound, sg.coeffs);
    return OK;
}


#define S(x) sizeof(x)

static OENTRY localops[] = {
    { "savgol.a",    S(SAVGOL),        0, "a",     "aiiop", (SUBR) savgol_audio_init,   (SUBR) savgol_audio_perf,   NULL, NULL, 0 },
    { "savgol.k",    S(SAVGOL),        0, "k",     "kiiop", (SUBR) savgol_control_init, (SUBR) savgol_control_perf, NULL, NULL, 0 },
    { "savgolmat.i", S(SAVGOL_MATRIX), 0, "i[][]", "iip",   (SUBR) savgol_matrix,       NULL,                       NULL, NULL, 0 }
};

int32_t savgol_init_(CSOUND *csound) {
    return csound->AppendOpcodes(csound, &(localops[0]), (int32_t) (S(localops) / S(OENTRY)));
}
