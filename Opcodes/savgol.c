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
    p = C * y with C the pseudo-inverse of A, an (o + 1) by w matrix that
    depends only on w and o, never on the samples. So the fit costs one dot
    product per output: row d of C convolved with the window yields p[d],
    and the d-th derivative of the fit at the centre is d! * p[d] / delta^d.
    calculate_savgol_coeffs() builds the derivative weights once at init
    time and get_coeffs() folds in 1 / delta^d for the single row the perf
    pass needs.

    C is never formed from the normal equations: A^T A squares the already
    exponential conditioning of the Vandermonde matrix, and in double
    precision it returns a wrong kernel from about w = 21, o = 20 on. The
    abscissae are scaled to [-1, 1] instead, and Arnoldi iteration (Gram-
    Schmidt run twice, as in Brubeck, Nakatsukasa and Trefethen,
    "Vandermonde with Arnoldi", SIAM Review 63, 2021) builds polynomials
    q_0 .. q_o orthonormal over the window. The fit is then the projection
    sum_k (q_k . y) q_k, and its derivatives at the centre follow from the
    recurrence that defines q_k, differentiated. Every accepted order comes
    out within a few ulps of the exact coefficients.

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


static double dot(const double *a, const double *b, uint32_t n) {
    double sum = 0.0;
    for (uint32_t i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }

    return sum;
}

/* Fills sg->coeffs (ncoef * winsize doubles, allocated by the caller): row d
   holds the weights giving the d-th derivative of the least-squares fit at
   the centre of the window, for a sample spacing of 1. NOTOK if the basis
   breaks down, which distinct abscissae rule out unless arithmetic fails. */
static int32_t calculate_savgol_coeffs(CSOUND *csound, SAVGOL_BUFFER *sg, uint32_t winsize, uint32_t ncoef) {
    double half = (double) (winsize - 1U) * 0.5;
    double *t = (double *) csound->Calloc(csound, sizeof(double) * (size_t) winsize);
    double *q = (double *) csound->Calloc(csound, sizeof(double) * (size_t) ncoef * winsize);
    double *h = (double *) csound->Calloc(csound, sizeof(double) * (size_t) ncoef * ncoef);
    double *deriv = (double *) csound->Calloc(csound, sizeof(double) * (size_t) ncoef * ncoef);
    int32_t res = OK;

    for (uint32_t i = 0; i < winsize; i++) {
        t[i] = ((double) i - half) / half;
        q[i] = 1.0 / sqrt((double) winsize);
    }

    /* Arnoldi: q_k = (t * q_{k-1} - sum_j h[j][k] q_j) / h[k][k], with the
       Gram-Schmidt pass run twice so that the columns stay orthonormal to
       working precision at any order. */
    for (uint32_t k = 1; k < ncoef && res == OK; k++) {
        double *qk = q + (size_t) k * winsize;
        const double *prev = qk - winsize;

        for (uint32_t i = 0; i < winsize; i++) {
            qk[i] = t[i] * prev[i];
        }

        for (int32_t pass = 0; pass < 2; pass++) {
            for (uint32_t j = 0; j < k; j++) {
                const double *qj = q + (size_t) j * winsize;
                double c = dot(qj, qk, winsize);
                h[j * ncoef + k] += c;
                for (uint32_t i = 0; i < winsize; i++) {
                    qk[i] -= c * qj[i];
                }
            }
        }

        double norm = sqrt(dot(qk, qk, winsize));
        if (UNLIKELY(!(norm > 0.0) || !isfinite(norm))) {
            res = NOTOK;
            break;
        }

        h[k * ncoef + k] = norm;
        for (uint32_t i = 0; i < winsize; i++) {
            qk[i] /= norm;
        }
    }

    if (res == OK) {
        /* deriv[d][k] is the d-th derivative of q_k at t = 0, from the same
           recurrence differentiated d times; q_k has degree k, so the higher
           derivatives stay zero. */
        deriv[0] = 1.0 / sqrt((double) winsize);
        for (uint32_t k = 1; k < ncoef; k++) {
            double hkk = h[k * ncoef + k];
            for (uint32_t d = 0; d <= k; d++) {
                double sum = d > 0 ? (double) d * deriv[(d - 1) * ncoef + k - 1] : 0.0;
                for (uint32_t j = 0; j < k; j++) {
                    sum -= h[j * ncoef + k] * deriv[d * ncoef + j];
                }
                deriv[d * ncoef + k] = sum / hkk;
            }
        }

        /* The fit is sum_k (q_k . y) q_k, so its d-th derivative at the
           centre weights sample i by sum_k deriv[d][k] q_k[i]; 1 / half^d
           converts from t back to samples. */
        double scale = 1.0;
        for (uint32_t d = 0; d < ncoef && res == OK; d++) {
            double *row = sg->coeffs + (size_t) d * winsize;
            for (uint32_t i = 0; i < winsize; i++) {
                double sum = 0.0;
                for (uint32_t k = d; k < ncoef; k++) {
                    sum += deriv[d * ncoef + k] * q[(size_t) k * winsize + i];
                }
                row[i] = sum * scale;
                if (UNLIKELY(!isfinite(row[i]))) {
                    res = NOTOK;
                }
            }
            scale /= half;
        }
    }

    csound->Free(csound, t);
    csound->Free(csound, q);
    csound->Free(csound, h);
    csound->Free(csound, deriv);

    sg->nrows = ncoef;
    sg->ncols = winsize;
    return res;
}

/* Row deriv of the coefficient matrix, scaled by 1 / delta^deriv so that the
   convolution yields the deriv-th derivative per unit of delta. */
static void get_coeffs(const SAVGOL_BUFFER *sg, double *coeffs_buffer, uint32_t deriv, double delta) {
    double scale = 1.0 / pow(delta, (double) deriv);
    const double *row = sg->coeffs + (size_t) deriv * sg->ncols;
    for (uint32_t i = 0; i < sg->ncols; i++) {
        coeffs_buffer[i] = row[i] * scale;
    }
}

static int32_t validate_params(CSOUND *csound, cs_float winsize_arg, cs_float order_arg, cs_float delta_arg, uint32_t *winsize, uint32_t *ncoef) {
    int32_t w = (int32_t) winsize_arg;
    int32_t o = (int32_t) order_arg;

    if (UNLIKELY(w < 3 || (w & 1) == 0)) {
        return csound->InitError(csound, "[savgol] winsize must be odd and at least 3\n");
    }

    if (UNLIKELY(o < 0 || o >= w)) {
        return csound->InitError(csound, "[savgol] order must be >= 0 and less than winsize\n");
    }

    if (UNLIKELY(delta_arg <= FL(0.0))) {
        return csound->InitError(csound, "[savgol] delta must be greater than 0\n");
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
        return csound->InitError(csound, "[savgol] deriv must be >= 0 and <= order\n");
    }

    SAVGOL_BUFFER sg = { NULL, 0, 0 };
    sg.coeffs = (double *) csound->Calloc(csound, sizeof(double) * (size_t) ncoef * winsize);

    if (UNLIKELY(calculate_savgol_coeffs(csound, &sg, winsize, ncoef) != OK)) {
        csound->Free(csound, sg.coeffs);
        return csound->InitError(csound, "[savgol] could not compute savgol coefficients\n");
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

    csound->AuxAlloc(csound, sizeof(cs_float) * (size_t) ((p->winsize_i - 1U) + CS_KSMPS), &p->buffer_mem);
    p->buffer_ptr = (cs_float *) p->buffer_mem.auxp;

    return OK;
}

int32_t savgol_audio_perf(CSOUND *csound, SAVGOL *p) {
    (void) csound;

    cs_float *out = p->y;
    const double *coeffs = (const double *) p->coeffs.auxp;
    uint32_t winsize = p->winsize_i;
    uint32_t win_offset = winsize - 1U;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;
    const cs_float *in = p->signal;

    if (UNLIKELY(offset)) {
        memset(out, 0, sizeof(cs_float) * (size_t) offset);
    }

    if (UNLIKELY(early)) {
        nsmps -= early;
        memset(out + nsmps, 0, sizeof(cs_float) * (size_t) early);
    }

    if (UNLIKELY(offset)) {
        memset(p->buffer_ptr + win_offset, 0, sizeof(cs_float) * (size_t) offset);
    }
    if (LIKELY(nsmps > offset)) {
        memcpy(p->buffer_ptr + win_offset + offset, in + offset,
               sizeof(cs_float) * (size_t) (nsmps - offset));
    }

    /* buffer_ptr[i .. i + winsize - 1] runs oldest to newest, matching the
       coefficient order; out[i] is the fit at the window centre, so the
       opcode has a latency of (winsize - 1) / 2 samples. */
    for (uint32_t i = offset; i < nsmps; i++) {
        double sum = 0.0;
        for (uint32_t j = 0; j < winsize; j++) {
            sum += coeffs[j] * (double) p->buffer_ptr[i + j];
        }
        out[i] = (cs_float) sum;
    }

    memmove(p->buffer_ptr, p->buffer_ptr + nsmps, sizeof(cs_float) * (size_t) win_offset);

    return OK;
}

int32_t savgol_control_init(CSOUND *csound, SAVGOL *p) {
    int32_t res = savgol_setup(csound, p);
    if (UNLIKELY(res != OK)) {
        return res;
    }

    csound->AuxAlloc(csound, sizeof(cs_float) * (size_t) p->winsize_i, &p->buffer_mem);
    p->buffer_ptr = (cs_float *) p->buffer_mem.auxp;
    p->write_pos = 0;

    return OK;
}

int32_t savgol_control_perf(CSOUND *csound, SAVGOL *p) {
    (void) csound;

    const double *coeffs = (const double *) p->coeffs.auxp;
    cs_float *buffer = p->buffer_ptr;
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

    *p->y = (cs_float) sum;
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
        return csound->InitError(csound, "[savgol] could not compute savgol coefficients\n");
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

    cs_float *out = (cs_float *) p->mat->data;
    double *row = (double *) csound->Calloc(csound, sizeof(double) * (size_t) sg.ncols);

    for (uint32_t i = 0; i < sg.nrows; i++) {
        get_coeffs(&sg, row, i, (double) *p->delta);
        for (uint32_t j = 0; j < sg.ncols; j++) {
            out[i * sg.ncols + j] = (cs_float) row[j];
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
