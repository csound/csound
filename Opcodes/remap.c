/*
    remap.c:

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
    remap - map a value onto an arbitrary curve defined by a breakpoint table.

    Given a table of breakpoints (xdata, ydata), remap returns the y matching
    the input x, interpolating between the two breakpoints that bracket it.
    The x table must be strictly increasing; both tables must hold at least
    two points and have the same length.

    SYNTAX

      y:k   = remap(x:k,   xdata:k[], ydata:k[], mode:i, bounds:i [, fill:i])
      y:k[] = remap(x:k[], xdata:k[], ydata:k[], mode:i, bounds:i [, fill:i])
      y:a   = remap(x:a,   xdata:k[], ydata:k[], mode:i, bounds:i [, fill:i])

    The two tables may be i- or k-rate arrays. With i-rate tables the
    breakpoints cannot change during the note, so they are validated once at
    init; with k-rate tables they may be rewritten at any control period and
    are re-checked every time.

    imode - how to interpolate between the two bracketing breakpoints

      0  linear
      1  nearest      the y of the closer breakpoint
      2  previous     the y of the left breakpoint
      3  next         the y of the right breakpoint
      4  cubic        shape-preserving cubic (PCHIP, Fritsch-Carlson):
                      smooth, but without the overshoot a natural cubic
                      spline introduces, so a monotonic table stays monotonic

    ibounds - what to do when x falls outside the table

      0  error        raise a performance error and stop the instrument
      1  clamp        hold the first or the last y
      2  fill         output ifill
      3  extrapolate  carry on along the first or the last segment

    ifill - the value emitted when ibounds is 2. Optional, defaults to 0.

    IMPLEMENTATION NOTES

    The interval search is branchless, so its cost depends only on the table
    size and not on how the input moves through the table: an unsorted input
    array costs the same as a ramp.

    Cubic mode relies on PCHIP being a local scheme - the slope at a
    breakpoint depends only on the adjacent intervals - so the coefficients of
    a single segment are derived on demand rather than tabulating the whole
    curve. This keeps the opcode free of any auxiliary allocation even when
    the breakpoints change at k-rate. The array and audio versions rebuild the
    coefficients only when consecutive inputs cross into a different segment.
*/


#include "remap.h"
#include "Opcodes/stdopcod.h"
#include "arrays.h"
#include "coreDefs.h"
#include "csdl.h"
#include "csound.h"
#include "sysdep.h"
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>


static int32_t check_vector(ARRAYDAT *vec) {
    if (vec == NULL || vec->sizes[0] < 2) {
        return NOTOK;
    }
    return OK;
}

static int32_t check_mode(int32_t mode) {
    int32_t is_valid_mode = mode >= REMAP_LINEAR && mode <= REMAP_CUBIC;
    if (!is_valid_mode) return NOTOK;
    return OK;
}

static int32_t check_bounds(int32_t bounds) {
    int32_t is_valid_bounds = bounds >= REMAP_ERROR && bounds <= REMAP_EXTRAPOLATE;
    if (!is_valid_bounds) return NOTOK;
    return OK;
}

/*
 * Largest i in [0, data_size - 2] with xdata[i] <= x.
 *
 * Branchless: the trip count depends only on data_size, so the loop is
 * perfectly predicted and the single data-dependent step compiles to a
 * conditional move. That makes the cost insensitive to the access pattern,
 * unlike a plain left/right binary search whose comparison branch mispredicts
 * on unsorted input.
 * Requires xdata[0] <= x <= xdata[data_size - 1].
 */
static int32_t find_interval(double x, const MYFLT *xdata, int32_t data_size) {
    int32_t base = 0;
    int32_t len = data_size - 1;

    while (len > 1) {
        const int32_t half = len >> 1;
        base += (x >= xdata[base + half]) ? half : 0;
        len -= half;
    }

    return base;
}

static void find_lerp_interval(LERP_INTERVAL *interval, double x, const MYFLT *xdata, const MYFLT *ydata, int32_t data_size, INTERP_BOUNDS bounds) {

    if (x < xdata[0] || x > xdata[data_size - 1]) {
        switch (bounds) {
            case REMAP_ERROR:
                interval->bmode = REMAP_NOT_VALID;
                return;
            case REMAP_CLAMP:
                if (x < xdata[0]) {
                    interval->bmode = REMAP_CLAMP_LEFT;
                } else {
                    interval->bmode = REMAP_CLAMP_RIGHT;
                }
                return;
            case REMAP_FILL:
                interval->bmode = REMAP_FILL_VALUE;
                return;
            case REMAP_EXTRAPOLATE:
                if (x < xdata[0]) {
                    interval->x0 = xdata[0];
                    interval->x1 = xdata[1];
                    interval->y0 = ydata[0];
                    interval->y1 = ydata[1];
                    interval->index = 0;
                    interval->bmode = REMAP_EXTRAPOLATE_LEFT;
                } else {
                    interval->x0 = xdata[data_size - 2];
                    interval->x1 = xdata[data_size - 1];
                    interval->y0 = ydata[data_size - 2];
                    interval->y1 = ydata[data_size - 1];
                    interval->index = data_size - 2;
                    interval->bmode = REMAP_EXTRAPOLATE_RIGHT;
                }
                return;
        }
    }

    const int32_t left = find_interval(x, xdata, data_size);

    interval->x0 = xdata[left];
    interval->x1 = xdata[left + 1];
    interval->y0 = ydata[left];
    interval->y1 = ydata[left + 1];
    interval->index = left;
    interval->bmode = REMAP_VALID;
    return;
}


/*
 * LINEAR INTERPOLATION
 * y = (y0 (x1 - x) + y1 (x - x0)) / (x1 - x0)
 */
static double lerp(double x, double x0, double x1, double y0, double y1) {
    return (y0 * (x1 - x) + y1 * (x - x0)) / (x1 - x0);
}

/*
 * NEAREST INTERPOLATION
 *     | y0 if (x - x0) <= (x1 - x)
 * y = |
 *     | y1 if (x - x0) > (x1 - x)
 */
static double nearest(double x, double x0, double x1, double y0, double y1) {
    return ((x - x0) <= (x1 - x)) ? y0 : y1;
}

/*
 * CUBIC PCHIP
 */
static double pchip_endpoint(double h0, double h1, double d0, double d1) {
    double m = ((2.0 * h0 + h1) * d0 - h0 * d1) / (h0 + h1);
    if ((m > 0.0) != (d0 > 0.0)) {
        return 0.0;
    }

    if (((d0 > 0.0) != (d1 > 0.0)) && fabs(m) > 3.0 * fabs(d0)) {
        return 3.0 * d0;
    }

    return m;
}

/*
 * PCHIP slope at breakpoint i.
 *
 * The Fritsch-Carlson scheme is local: m[i] only depends on the one or two
 * intervals adjacent to i. Evaluating it on demand therefore costs O(1) and
 * removes the need to tabulate (and re-tabulate) the whole curve whenever the
 * breakpoints change at k-rate.
 */
static double pchip_slope(const MYFLT *x, const MYFLT *y, int32_t n, int32_t i) {
    double h0, h1, d0, d1;

    if (n == 2) {
        return ((double) y[1] - (double) y[0]) / ((double) x[1] - (double) x[0]);
    }

    if (i == 0) {
        h0 = (double) x[1] - (double) x[0];
        h1 = (double) x[2] - (double) x[1];
        d0 = ((double) y[1] - (double) y[0]) / h0;
        d1 = ((double) y[2] - (double) y[1]) / h1;
        return pchip_endpoint(h0, h1, d0, d1);
    }

    if (i == n - 1) {
        h0 = (double) x[n - 1] - (double) x[n - 2];
        h1 = (double) x[n - 2] - (double) x[n - 3];
        d0 = ((double) y[n - 1] - (double) y[n - 2]) / h0;
        d1 = ((double) y[n - 2] - (double) y[n - 3]) / h1;
        return pchip_endpoint(h0, h1, d0, d1);
    }

    h0 = (double) x[i] - (double) x[i - 1];
    h1 = (double) x[i + 1] - (double) x[i];
    d0 = ((double) y[i] - (double) y[i - 1]) / h0;
    d1 = ((double) y[i + 1] - (double) y[i]) / h1;
    if (d0 == 0.0 || d1 == 0.0 || ((d0 > 0.0) != (d1 > 0.0))) {
        return 0.0;
    }

    const double w1 = 2.0 * h1 + h0;
    const double w2 = h1 + 2.0 * h0;
    return (w1 + w2) / (w1 / d0 + w2 / d1);
}

/*
 * Cubic Hermite coefficients of segment i, with the PCHIP slopes resolved on
 * the fly. Kept separate from the evaluation so that the vector opcode can
 * reuse them across every input value falling in the same segment.
 */
static void pchip_segment(PCHIP_SEGMENT *s, const MYFLT *xdata, const MYFLT *ydata, int32_t n, int32_t i) {
    const double x0 = (double) xdata[i];
    const double y0 = (double) ydata[i];
    const double h = (double) xdata[i + 1] - x0;

    s->x0 = x0;
    s->a = y0;

    if (h <= 0.0) {               /* non-increasing x: no usable segment */
        s->b = s->c = s->d = 0.0;
        return;
    }

    const double delta = ((double) ydata[i + 1] - y0) / h;
    const double m0 = pchip_slope(xdata, ydata, n, i);
    const double m1 = pchip_slope(xdata, ydata, n, i + 1);

    s->b = m0;
    s->c = (3.0 * delta - 2.0 * m0 - m1) / h;
    s->d = (m0 + m1 - 2.0 * delta) / (h * h);
}

/*
 * Outside [x[i], x[i+1]] this extrapolates along the same cubic.
 */
static double pchip_segment_eval(const PCHIP_SEGMENT *s, double x) {
    const double t = x - s->x0;
    return ((s->d * t + s->c) * t + s->b) * t + s->a;
}

static double pchip_eval(double x, const MYFLT *xdata, const MYFLT *ydata, int32_t n, int32_t i) {
    PCHIP_SEGMENT s;
    pchip_segment(&s, xdata, ydata, n, i);
    return pchip_segment_eval(&s, x);
}

/*
 * True when the resolved overload takes i-rate arrays for both data inputs:
 * the breakpoints are then fixed for the whole note.
 */
static int32_t data_is_static(OPDS *h) {
    const char *t = h->optext->t.oentry->intypes;
    int32_t arg = 0;
    int32_t x_static = 0;
    int32_t y_static = 0;

    while (*t != '\0') {
        char base = *t++;
        int32_t is_array = 0;
        if (*t == '[') {
            t += 2;             /* skip the "[]" suffix */
            is_array = 1;
        }
        if (arg == 1) x_static = is_array && base == 'i';
        if (arg == 2) y_static = is_array && base == 'i';
        arg++;
    }

    return x_static && y_static;
}

/*
 * The interval search assumes strictly increasing breakpoints, for every
 * interpolation mode. Only checked for i-rate data, where it costs nothing
 * at performance time.
 */
static int32_t check_increasing(ARRAYDAT *vec) {
    const MYFLT *v = vec->data;
    for (int32_t i = 0; i < vec->sizes[0] - 1; ++i) {
        if (v[i + 1] <= v[i]) {
            return NOTOK;
        }
    }
    return OK;
}


int32_t remap_value_init(CSOUND *csound, REMAP_VALUE *p) {
    p->imode = (int32_t) *p->mode;
    p->ibounds = (int32_t) *p->bounds;
    p->fill_value = (double) *p->fill;

    if (check_mode(p->imode) == NOTOK) {
        return csound->InitError(csound, "[remap] Invalid interpolation mode");
    }

    if (check_bounds(p->ibounds) == NOTOK) {
        return csound->InitError(csound, "[remap] Invalid interpolation bounds");
    }

    if (p->xdata->sizes == NULL || p->ydata->sizes == NULL)
        return csound->InitError(csound, "[remap] array not initialised");

    p->static_data = data_is_static(&p->h);

    if (p->static_data) {
        if (check_vector(p->xdata) != OK) {
            return csound->InitError(csound, "[remap] Invalid x data array");
        }

        if (check_vector(p->ydata) != OK) {
            return csound->InitError(csound, "[remap] Invalid y data array");
        }

        if (p->xdata->sizes[0] != p->ydata->sizes[0]) {
            return csound->InitError(csound, "[remap] x and y must have same length");
        }

        if (check_increasing(p->xdata) != OK) {
            return csound->InitError(csound, "[remap] x data must be strictly increasing");
        }
    }

    return OK;
}

int32_t remap_value_perf(CSOUND *csound, REMAP_VALUE *p) {
    if (!p->static_data) {
        if (check_vector(p->xdata) != OK) {
            return csound->PerfError(csound, &(p->h), "[remap] Invalid x data array");
        }

        if (check_vector(p->ydata) != OK) {
            return csound->PerfError(csound, &(p->h), "[remap] Invalid y data array");
        }

        if (p->xdata->sizes[0] != p->ydata->sizes[0]) {
            return csound->PerfError(csound, &(p->h), "[remap] x and y must have same length");
        }
    }

    const MYFLT *xdata = p->xdata->data;
    const MYFLT *ydata = p->ydata->data;
    const double x = (double) *p->x;
    int32_t size = p->ydata->sizes[0];

    LERP_INTERVAL l_interval = {0};
    find_lerp_interval(&l_interval, x, xdata, ydata, size, (INTERP_BOUNDS) p->ibounds);
    switch (l_interval.bmode) {
        case REMAP_NOT_VALID:
            return csound->PerfError(csound, &(p->h), "[remap] x value out of bounds");
        case REMAP_CLAMP_LEFT:
            *p->y = ydata[0];
            break;
        case REMAP_CLAMP_RIGHT:
            *p->y = ydata[size - 1];
            break;
        case REMAP_FILL_VALUE:
            *p->y = (MYFLT) p->fill_value;
            break;
        case REMAP_VALID:
        case REMAP_EXTRAPOLATE_LEFT:
        case REMAP_EXTRAPOLATE_RIGHT:
            switch (p->imode) {
                case REMAP_LINEAR:
                    *p->y = (MYFLT) lerp(x, l_interval.x0, l_interval.x1, l_interval.y0, l_interval.y1);
                    break;
                case REMAP_NEAREST:
                    *p->y = (MYFLT) nearest(x, l_interval.x0, l_interval.x1, l_interval.y0, l_interval.y1);
                    break;
                case REMAP_PREVIOUS:
                    *p->y = (MYFLT) l_interval.y0;
                    break;
                case REMAP_NEXT:
                    *p->y = (MYFLT) l_interval.y1;
                    break;
                case REMAP_CUBIC:
                    *p->y = (MYFLT) pchip_eval(x, xdata, ydata, size, l_interval.index);
                    break;
            }
            break;
    }

    return OK;

}

int32_t remap_vec_init(CSOUND *csound, REMAP_VEC *p) {
    p->imode = (int32_t) *p->mode;
    p->ibounds = (int32_t) *p->bounds;
    p->fill_value = (double) *p->fill;

    if (check_mode(p->imode) == NOTOK) {
        return csound->InitError(csound, "[remap] Invalid interpolation mode");
    }

    if (check_bounds(p->ibounds) == NOTOK) {
        return csound->InitError(csound, "[remap] Invalid interpolation bounds");
    }

    if (p->xdata->sizes == NULL || p->ydata->sizes == NULL || p->x->sizes == NULL) {
        return csound->InitError(csound, "[remap] array not initialised");
    }

    p->static_data = data_is_static(&p->h);

    if (p->static_data) {
        if (check_vector(p->xdata) != OK) {
            return csound->InitError(csound, "[remap] Invalid x data array");
        }

        if (check_vector(p->ydata) != OK) {
            return csound->InitError(csound, "[remap] Invalid y data array");
        }

        if (p->xdata->sizes[0] != p->ydata->sizes[0]) {
            return csound->InitError(csound, "[remap] x and y must have same length");
        }

        if (check_increasing(p->xdata) != OK) {
            return csound->InitError(csound, "[remap] x data must be strictly increasing");
        }
    }

    tabinit(csound, p->y, p->x->sizes[0], p->h.insdshead);
    return OK;
}

int32_t remap_vec_perf(CSOUND *csound, REMAP_VEC *p) {
    if (p->x == NULL || p->x->sizes == NULL || p->x->sizes[0] < 1) {
        return csound->PerfError(csound, &(p->h), "[remap] Invalid x array");
    }

    if (!p->static_data) {
        if (check_vector(p->xdata) != OK) {
            return csound->PerfError(csound, &(p->h), "[remap] Invalid x data array");
        }

        if (check_vector(p->ydata) != OK) {
            return csound->PerfError(csound, &(p->h), "[remap] Invalid y data array");
        }

        if (p->xdata->sizes[0] != p->ydata->sizes[0]) {
            return csound->PerfError(csound, &(p->h), "[remap] x and y must have same length");
        }
    }

    const MYFLT *xdata = p->xdata->data;
    const MYFLT *ydata = p->ydata->data;
    const MYFLT *x = p->x->data;
    int32_t size = p->ydata->sizes[0];
    int32_t out_size = p->x->sizes[0];

    if (p->y->sizes[0] != out_size) {       /* the input array may be resized */
        tabinit(csound, p->y, out_size, p->h.insdshead);
    }
    MYFLT *y = p->y->data;

    /* The PCHIP coefficients are rebuilt only when the segment changes, so at
       most min(out_size, size - 1) times: never more work than tabulating the
       whole curve, and much less whenever consecutive inputs are close. */
    PCHIP_SEGMENT segment;
    int32_t cached = -1;

    for (int32_t k = 0; k < out_size; ++k) {
        const double xv = (double) x[k];
        LERP_INTERVAL l_interval = {0};

        find_lerp_interval(&l_interval, xv, xdata, ydata, size, (INTERP_BOUNDS) p->ibounds);
        switch (l_interval.bmode) {
            case REMAP_NOT_VALID:
                return csound->PerfError(csound, &(p->h), "[remap] x value out of bounds");
            case REMAP_CLAMP_LEFT:
                y[k] = ydata[0];
                break;
            case REMAP_CLAMP_RIGHT:
                y[k] = ydata[size - 1];
                break;
            case REMAP_FILL_VALUE:
                y[k] = (MYFLT) p->fill_value;
                break;
            case REMAP_VALID:
            case REMAP_EXTRAPOLATE_LEFT:
            case REMAP_EXTRAPOLATE_RIGHT:
                switch (p->imode) {
                    case REMAP_LINEAR:
                        y[k] = (MYFLT) lerp(xv, l_interval.x0, l_interval.x1, l_interval.y0, l_interval.y1);
                        break;
                    case REMAP_NEAREST:
                        y[k] = (MYFLT) nearest(xv, l_interval.x0, l_interval.x1, l_interval.y0, l_interval.y1);
                        break;
                    case REMAP_PREVIOUS:
                        y[k] = (MYFLT) l_interval.y0;
                        break;
                    case REMAP_NEXT:
                        y[k] = (MYFLT) l_interval.y1;
                        break;
                    case REMAP_CUBIC:
                        if (l_interval.index != cached) {
                            pchip_segment(&segment, xdata, ydata, size, l_interval.index);
                            cached = l_interval.index;
                        }
                        y[k] = (MYFLT) pchip_segment_eval(&segment, xv);
                        break;
                }
                break;
        }
    }

    return OK;

}

int32_t remap_audio_perf(CSOUND *csound, REMAP_VALUE *p) {
    if (!p->static_data) {
        if (check_vector(p->xdata) != OK) {
            return csound->PerfError(csound, &(p->h), "[remap] Invalid x data array");
        }

        if (check_vector(p->ydata) != OK) {
            return csound->PerfError(csound, &(p->h), "[remap] Invalid y data array");
        }

        if (p->xdata->sizes[0] != p->ydata->sizes[0]) {
            return csound->PerfError(csound, &(p->h), "[remap] x and y must have same length");
        }
    }

    const MYFLT *xdata = p->xdata->data;
    const MYFLT *ydata = p->ydata->data;
    int32_t size = p->ydata->sizes[0];

    PCHIP_SEGMENT segment;
    int32_t cached = -1;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t nsamples = CS_KSMPS;

    MYFLT *y = p->y;

    if (UNLIKELY(offset)) {
        memset(y, 0, sizeof(MYFLT) * offset);
    }

    if (UNLIKELY(early)) {
        nsamples -= early;
        memset(y + nsamples, 0, sizeof(MYFLT) * early);
    }

    for (uint32_t k = offset; k < nsamples; ++k) {
        const double xv = (double) p->x[k];
        LERP_INTERVAL l_interval = {0};

        find_lerp_interval(&l_interval, xv, xdata, ydata, size, (INTERP_BOUNDS) p->ibounds);
        switch (l_interval.bmode) {
            case REMAP_NOT_VALID:
                return csound->PerfError(csound, &(p->h), "[remap] x value out of bounds");
            case REMAP_CLAMP_LEFT:
                y[k] = ydata[0];
                break;
            case REMAP_CLAMP_RIGHT:
                y[k] = ydata[size - 1];
                break;
            case REMAP_FILL_VALUE:
                y[k] = (MYFLT) p->fill_value;
                break;
            case REMAP_VALID:
            case REMAP_EXTRAPOLATE_LEFT:
            case REMAP_EXTRAPOLATE_RIGHT:
                switch (p->imode) {
                    case REMAP_LINEAR:
                        y[k] = (MYFLT) lerp(xv, l_interval.x0, l_interval.x1, l_interval.y0, l_interval.y1);
                        break;
                    case REMAP_NEAREST:
                        y[k] = (MYFLT) nearest(xv, l_interval.x0, l_interval.x1, l_interval.y0, l_interval.y1);
                        break;
                    case REMAP_PREVIOUS:
                        y[k] = (MYFLT) l_interval.y0;
                        break;
                    case REMAP_NEXT:
                        y[k] = (MYFLT) l_interval.y1;
                        break;
                    case REMAP_CUBIC:
                        if (l_interval.index != cached) {
                            pchip_segment(&segment, xdata, ydata, size, l_interval.index);
                            cached = l_interval.index;
                        }
                        y[k] = (MYFLT) pchip_segment_eval(&segment, xv);
                        break;
                }
                break;
        }
    }

    return OK;

}

#define S(x) sizeof(x)

static OENTRY remap[] = {
    { "remap.kk",  S(REMAP_VALUE), 0, "k",   "kk[]k[]iio",   (SUBR) remap_value_init, (SUBR) remap_value_perf, NULL },
    { "remap.ik",  S(REMAP_VALUE), 0, "k",   "ki[]k[]iio",   (SUBR) remap_value_init, (SUBR) remap_value_perf, NULL },
    { "remap.ki",  S(REMAP_VALUE), 0, "k",   "ki[]i[]iio",   (SUBR) remap_value_init, (SUBR) remap_value_perf, NULL },
    { "remap.kkk", S(REMAP_VEC),   0, "k[]", "k[]k[]k[]iio", (SUBR) remap_vec_init,   (SUBR) remap_vec_perf,   NULL },
    { "remap.kik", S(REMAP_VEC),   0, "k[]", "k[]i[]k[]iio", (SUBR) remap_vec_init,   (SUBR) remap_vec_perf,   NULL },
    { "remap.kii", S(REMAP_VEC),   0, "k[]", "k[]i[]i[]iio", (SUBR) remap_vec_init,   (SUBR) remap_vec_perf,   NULL },
    { "remap.akk", S(REMAP_VALUE), 0, "a",   "ak[]k[]iio",   (SUBR) remap_value_init, (SUBR) remap_audio_perf,   NULL },
    { "remap.aik", S(REMAP_VALUE), 0, "a",   "ai[]k[]iio",   (SUBR) remap_value_init, (SUBR) remap_audio_perf,   NULL },
    { "remap.aii", S(REMAP_VALUE), 0, "a",   "ai[]i[]iio",   (SUBR) remap_value_init, (SUBR) remap_audio_perf,   NULL },
};

int32_t remap_init_(CSOUND *csound) {
    return csound->AppendOpcodes(csound, &(remap[0]), (int32_t) (sizeof(remap) / sizeof(OENTRY)));
}
