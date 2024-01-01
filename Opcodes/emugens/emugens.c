/*

    emugens.c:

    Copyright (C) 2017  Eduardo Moguillansky

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

#include <ctype.h>               // for isspace, isalpha
#include <math.h>                // for ceil, floor, round
#include <stdint.h>              // for int32_t, uint32_t, int64_t
#include <stdio.h>               // for NULL, sprintf, size_t, fflush, printf
#include <string.h>              // for strlen, memset, memcpy, strstr, strcpy

#include "arrays.h"              // for tabinit
#include "csound.h"              // for CSOUND, Str
#include "csoundCore.h"          // for SUBR, ARRAYDAT, STRINGDAT, OK, CSOUND_
#include "csound_type_system.h"  // for CS_TYPE, CS_VAR_MEM
#include "emugens_common.h"      // for INITERR, PERFERR, INITERRF, ARRAY_EN...
#include "interlocks.h"          // for TR, TB, TW
#include "msg_attr.h"            // for CSOUNDMSG_ORCH
#include "sysdep.h"              // for MYFLT, UNLIKELY, FL, COS, MYFLT2LRND

#define SAMPLE_ACCURATE \
    uint32_t n, nsmps = CS_KSMPS;                                    \
    MYFLT *out = p->out;                                             \
    uint32_t offset = p->h.insdshead->ksmps_offset;                  \
    uint32_t early = p->h.insdshead->ksmps_no_end;                   \
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));   \
    if (UNLIKELY(early)) {                                           \
        nsmps -= early;                                              \
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));              \
    }                                                                \

// needed for each opcode using audio-rate inputs/outputs
#define AUDIO_OPCODE(csound, p) \
    IGN(csound); \
    uint32_t n, nsmps = CS_KSMPS;                                    \
    uint32_t offset = p->h.insdshead->ksmps_offset;                  \
    uint32_t early = p->h.insdshead->ksmps_no_end;                   \

// initialize an audio output variable, for sample-accurate offset/early end
// this should be called for each audio output
#define AUDIO_OUTPUT(out) \
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));   \
    if (UNLIKELY(early)) {                                           \
        nsmps -= early;                                              \
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));              \
    }                                                                \


#define UI32MAX 0x7FFFFFFF


/*

   linlin

   linear to linear conversion

   ky   linlin kx, kylow, kyhigh, kxlow=0, kxhigh=1

   ky = (kx - kxlow) / (kxhigh - kxlow) * (kyhigh - kylow) + kylow

   linlin(0.25, 1, 3, 0, 1)   -> 1.5
   linlin(0.25, 1, 3)         -> 1.5
   linlin(0.25, 1, 3, 0, 0.5) -> 2

   Variants:

   ky[] linlin kx[], kylow, kyhigh, kxlow=0, kxhigh=1
   ky[] linlin kx, kylow[], kyhigh[], kxlow=0, kxhigh=1

 */

typedef struct {
    OPDS h;
    MYFLT *kout, *kx, *ky0, *ky1, *kx0, *kx1;
} LINLIN1;

static int32_t
linlin1_perf(CSOUND *csound, LINLIN1 *p) {
    MYFLT x0 = *p->kx0;
    MYFLT y0 = *p->ky0;
    MYFLT x = *p->kx;
    MYFLT x1 = *p->kx1;
    if (UNLIKELY(x0 == x1)) {
        return csound->PerfError(csound, &(p->h),
                                 "%s", Str("linlin.k: Division by zero"));
    }
    *p->kout = (x - x0) / (x1 -x0) * (*(p->ky1) - y0) + y0;
    return OK;
}

typedef struct {
    OPDS h;
    // kY[] linlin kX[], ky0, ky1, kx0=0, kx1=1
    ARRAYDAT *ys, *xs;
    MYFLT *ky0, *ky1, *kx0, *kx1;
} LINLINARR1;


static int32_t linlinarr1_init(CSOUND *csound, LINLINARR1 *p) {
    int numitems = p->xs->sizes[0];
    tabinit(csound, p->ys, numitems);
    CHECKARR1D(p->xs);
    CHECKARR1D(p->ys);
    return OK;
}

static int32_t
linlinarr1_perf(CSOUND *csound, LINLINARR1 *p) {
    MYFLT x0 = *p->kx0;
    MYFLT y0 = *p->ky0;
    MYFLT x1 = *p->kx1;
    MYFLT y1 = *p->ky1;

    if (UNLIKELY(x0 == x1)) {
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("linlin.k: Division by zero"));
    }
    MYFLT fact = 1/(x1 - x0) * (y1 - y0);

    int32_t numitems = p->xs->sizes[0];
    ARRAY_ENSURESIZE_PERF(csound, p->ys, numitems);
    MYFLT *out = p->ys->data;
    MYFLT *in  = p->xs->data;
    int32_t i;
    for(i=0; i<numitems; i++) {
        out[i] = (in[i] - x0) * fact + y0;
    }
    return OK;
}

static int32_t
linlinarr1_i(CSOUND *csound, LINLINARR1 *p) {
    linlinarr1_init(csound, p);
    return linlinarr1_perf(csound, p);
}

typedef struct {
    OPDS h;
    // kOut[] linlin kx, kA[], kB[], kx0=0, kx1=1
    ARRAYDAT *out;
    MYFLT *kx;
    ARRAYDAT *A, *B;
    MYFLT *kx0, *kx1;
    int32_t numitems;
} BLENDARRAY;

static int32_t
blendarray_init(CSOUND *csound, BLENDARRAY *p) {
    int32_t numitemsA = p->A->sizes[0];
    int32_t numitemsB = p->B->sizes[0];
    int32_t numitems = numitemsA < numitemsB ? numitemsA : numitemsB;
    tabinit(csound, p->out, numitems);
    p->numitems = numitems;
    return OK;
}

static int32_t
blendarray_perf(CSOUND *csound, BLENDARRAY *p)
{
    MYFLT x0 = *p->kx0;
    MYFLT x1 = *p->kx1;
    MYFLT x = *p->kx;

    if (UNLIKELY(x0 == x1)) {
        return PERFERR(Str("linlin: Division by zero"));
    }
    int32_t numitemsA = p->A->sizes[0];
    int32_t numitemsB = p->B->sizes[0];
    int32_t numitems = numitemsA < numitemsB ? numitemsA : numitemsB;
    ARRAY_ENSURESIZE_PERF(csound, p->out, numitems);

    MYFLT *out = p->out->data;
    MYFLT *A = p->A->data;
    MYFLT *B = p->B->data;
    MYFLT y0, y1;
    MYFLT fact = (x - x0) / (x1 - x0);
    int32_t i;
    for(i=0; i<numitems; i++) {
        y0 = A[i];
        y1 = B[i];
        out[i] = (y1 - y0) * fact + y0;
    }
    return OK;
}

static int32_t
blendarray_i(CSOUND *csound, BLENDARRAY *p) {
    blendarray_init(csound, p);
    return blendarray_perf(csound, p);
}


/*

    linear to cosine interpolation
    ky lincos kx, ky0, ky1, kx0=0, kx1=1

    given x between x0 and x1, find y between y0 and y1 with cosine interpolation

    The order of ky0 and ky1 are given first because it is often used in contexts
    where kx is defined between 0 and 1

*/

static int32_t
lincos_perf(CSOUND *csound, LINLIN1 *p) {
    MYFLT x0 = *p->kx0;
    MYFLT y0 = *p->ky0;
    MYFLT x = *p->kx;
    MYFLT x1 = *p->kx1;
    MYFLT y1 = *p->ky1;
    if (UNLIKELY(x0 == x1)) {
        return PERFERR(Str("lincos: Division by zero"));
    }
    // PI is defined in csoundCore.h, use that instead of M_PI from math.h, which
    // is not defined in windows
    MYFLT dx = ((x-x0) / (x1-x0)) * PI + PI;           // dx range pi - 2pi
    *p->kout = y0 + ((y1 - y0) * (1 + COS(dx)) / 2.0);
    return OK;
}

/* ------------- xyscale --------------

   2d linear interpolation (normalized)

   Given values for four points at (0, 0), (0, 1), (1, 0), (1, 1),
   calculate the interpolated value at a given coord (x, y) inside this square

   inputs: kx, ky, v00, v10, v01, v11

   kx, ky: coord, between 0-1

   This is conceptually the same as:

   ky0 = scale(kx, v01, v00)
   ky1 = scale(kx, v11, v10)
   kout = scale(ky, ky1, ky0)

 */

typedef struct {
    OPDS h;
    MYFLT *kout, *kx, *ky, *v00, *v10, *v01, *v11;
    MYFLT d0, d1;
} XYSCALE;

static int32_t xyscalei_init(CSOUND *csound, XYSCALE *p) {
    IGN(csound);
    p->d0 = (*p->v01) - (*p->v00);
    p->d1 = (*p->v11) - (*p->v10);
    return OK;
}

static int32_t xyscalei(CSOUND *csound, XYSCALE *p) {
    IGN(csound);
    // x, y: between 0-1
    MYFLT x = *p->kx;
    MYFLT y0 = x * (p->d0) + (*p->v00);
    MYFLT y1 = x * (p->d1) + (*p->v10);
    *p->kout = (*p->ky) * (y1 - y0) + y0;
    return OK;
}

static int32_t xyscale(CSOUND *csound, XYSCALE *p) {
    IGN(csound);
    // x, y: between 0-1
    // x, y will interpolate between the values at the 4 corners
    MYFLT v00 = *p->v00;
    MYFLT v10 = *p->v10;
    MYFLT x = *p->kx;
    MYFLT y0 = x * (*p->v01 - v00) + v00;
    MYFLT y1 = x * (*p->v11 - v10) + v10;
    *p->kout = (*p->ky) * (y1 - y0) + y0;
    return OK;
}


/*  mtof -- ftom

   midi to frequency conversion

   mtof(midinote)

   kfreq = mtof(69)

 */

typedef struct {
  OPDS h;
  MYFLT *r, *k, *irnd;
  MYFLT freqA4;
  int rnd;
} PITCHCONV;

static inline MYFLT
mtof_func(MYFLT midi, MYFLT a4) {
    return POWER(FL(2.0), (midi - FL(69.0)) / FL(12.0)) * a4;
}

static int32_t mtof(CSOUND *csound, PITCHCONV *p) {
    IGN(csound);
    *p->r = mtof_func(*p->k, p->freqA4);
    return OK;
}

static int32_t
mtof_init(CSOUND *csound, PITCHCONV *p) {
    p->freqA4 = csound->GetA4(csound);
    mtof(csound, p);
    return OK;
}

static int32_t ftom(CSOUND *csound, PITCHCONV *p) {
    MYFLT ans;
    IGN(csound);
    ans = FL(12.0) * LOG2(*p->k / p->freqA4) + FL(69.0);
    if (UNLIKELY(p->rnd)) ans = (MYFLT)MYFLT2LRND(ans);
    *p->r = ans;
    return OK;
}

static int32_t
ftom_init(CSOUND *csound, PITCHCONV *p) {
    p->freqA4 = csound->GetA4(csound);
    p->rnd = (int)(*p->irnd);
    ftom(csound, p);
    return OK;
}

static int32_t pchtom(CSOUND *csound, PITCHCONV *p) {
    IGN(csound);
    MYFLT pch = *p->k;
    MYFLT oct = FLOOR(pch);
    MYFLT note = pch - oct;
    *p->r = (oct - FL(3.0)) * FL(12.0) + note * FL(100.0);
    return OK;
}


typedef struct {
    OPDS h;
    ARRAYDAT *outarr, *inarr;
    MYFLT *irnd;
    MYFLT freqA4;
    int rnd;
    int skip;
} PITCHCONV_ARR;


static int32_t
ftom_arr(CSOUND *csound, PITCHCONV_ARR *p) {
    MYFLT x, *indata, *outdata;
    int32_t i;
    if(p->skip) {
        p->skip = 0;
        return OK;
    }
    MYFLT a4 = p->freqA4;
    IGN(csound);
    indata = p->inarr->data;
    outdata = p->outarr->data;
    for(i=0; i < p->inarr->sizes[0]; i++) {
        x = indata[i];
        outdata[i] = FL(12.0) * LOG2(x / a4) + FL(69.0);
    }
    if(UNLIKELY(p->rnd)) {
        for(i=0; i < p->inarr->sizes[0]; i++) {
            outdata[i] = (MYFLT)MYFLT2LRND(outdata[i]);
        }
    }
    return OK;
}

static int32_t
ftom_arr_init(CSOUND *csound, PITCHCONV_ARR *p) {
    p->freqA4 = csound->GetA4(csound);
    p->rnd = (int)*p->irnd;
    tabinit(csound, p->outarr, p->inarr->sizes[0]);
    p->skip = 0;
    ftom_arr(csound, p);
    p->skip = 1;
    return OK;
}

static int32_t
mtof_arr(CSOUND *csound, PITCHCONV_ARR *p) {
    MYFLT x, *indata, *outdata;
    int32_t i;
    if(p->skip) {
        p->skip = 0;
        return OK;
    }
    MYFLT a4 = p->freqA4;
    IGN(csound);
    indata = p->inarr->data;
    outdata = p->outarr->data;
    int32_t numitems = p->inarr->sizes[0];
    ARRAY_ENSURESIZE_PERF(csound, p->outarr, numitems);
    for(i=0; i < numitems; i++) {
        x = indata[i];
        outdata[i] = POWER(FL(2.0), (x - FL(69.0)) / FL(12.0)) * a4;
    }
    return OK;
}

static int32_t
mtof_arr_init(CSOUND *csound, PITCHCONV_ARR *p) {
    p->freqA4 = csound->GetA4(csound);
    tabinit(csound, p->outarr, p->inarr->sizes[0]);
    p->skip = 0;
    mtof_arr(csound, p);
    p->skip = 1;
    return OK;
}

/*
   bpf  --> break point function with linear interpolation

   ky bpf kx, kx0, ky0, kx1, ky1, ...
   iy bpf ix, ix0, iy0, ix1, iy1, ...
   kys[] bpf kxs[], kx0, ky0, kx1, ky1, ...
   ky bpf kx, kxs[], kys[]
   ky bpf kx, kpairs[]  ??
   ay bpf ax, kx0, ky0, kx1, ky1, ...
   ay bpf ax, kxs[], kys[]




 */

// VL Clang complains this is unused, commenting out.
/*
static inline int32_t bpf_find_multidim(MYFLT x, MYFLT *xs, int32_t xslen, int32_t step, int32_t lastidx) {
    // xslen: size of xs (number of frames, not size of array)
    // step: normally 1, can be more for the case where xs and ys (and possibly zs, etc)
    // are all embedded in the same array: [x0, y0, z0, x1, y1, z1, ...]
    // returns: index or -1: lower bound, -2: upper bound
    if(lastidx < xslen - 1 && xs[lastidx*step] <= x && x < xs[(lastidx+1)*step]) {
        return lastidx;
    }
    // check next idx
    if(lastidx < xslen - 2 && xs[(lastidx+1)*step] <= x && x < xs[(lastidx+2)*step]) {
        return lastidx+1;
    }
    // Generic search. first check bounds
    if(x <= xs[0]) {
        return -1;
    }
    if(x >= xs[(xslen-1)*step]) {
        return -2;
    }
    // bin search
    int32_t imin = 0;
    int32_t imax = xslen;
    int32_t imid;
    while (imin < imax) {
        imid = (imax + imin) / 2;
        if (xs[imid*step] < x)
            imin = imid + 1;
        else
            imax = imid;
    }
    return imin;
}
*/

#define BPF_MAXPOINTS 256

typedef struct {
    OPDS h;
    MYFLT *r, *x, *data[BPF_MAXPOINTS];
    int32_t lastidx;
} BPFX;


static int32_t bpfx_init(CSOUND *csound, BPFX *p) {
    int32_t datalen = p->INOCOUNT - 1;
    p->lastidx = -1;
    if(datalen % 2) {
      return INITERR(Str("bpf: data length should be even (pairs of x, y)"));
    }
    if(datalen >= BPF_MAXPOINTS) {
      return INITERR(Str("bpf: too many pargs (max=256)"));
    }
    return OK;
}


static int32_t bpfx_k(CSOUND *csound, BPFX *p);


static int32 bpfx_i(CSOUND *csound, BPFX *p) {
    bpfx_init(csound, p);
    return bpfx_k(csound, p);
}


static inline int32_t bpfx_find(MYFLT **data, MYFLT x, int32_t datalen, int32_t lastidx) {
    // returns -1 if x is less than the lowest breakpoint
    if (x <= *data[0])
        return -1;
    // -2 if x is higher than the highest breakpoint
    if (x>=*data[datalen-2])
        return -2;
    if(lastidx >= 0 && lastidx < datalen-4 && *data[lastidx] <= x && x < *data[lastidx+2])
        return lastidx;
    // bin search
    int32_t numpairs = datalen / 2;
    int32_t pairmin = 0;
    int32_t pairmax = numpairs;
    int32_t pairmid;

    while (pairmin < pairmax) {
        pairmid = (pairmax + pairmin) / 2;
        if (*data[pairmid * 2] < x)
            pairmin = pairmid + 1;
        else
            pairmax = pairmid;
    }
    // now the right pair is in pairmin
    return (pairmin-1)*2;
}

static int32_t bpfx_k(CSOUND *csound, BPFX *p) {
    MYFLT x = *p->x;
    MYFLT **data = p->data;
    int32_t datalen = p->INOCOUNT - 1;
    MYFLT x0, x1, y0, y1;

    int32_t idx = bpfx_find(data, x, datalen, p->lastidx);

    if(idx == -1) {
        *p->r = *data[1];
        p->lastidx = -1;
        return OK;
    }

    if(idx == -2) {
        *p->r = *data[datalen-1];
        p->lastidx = -1;
        return OK;
    }

    x0 = *data[idx];
    x1 = *data[idx+2];
    if(UNLIKELY(x < x0 || x > x1)) {
        printf("Bug in bpfx_k. x: %f should be between %f and %f\n", x, x0, x1);
        return NOTOK;
    }

    y0 = *data[idx+1];
    y1 = *data[idx+3];
    *p->r = (x-x0)/(x1-x0)*(y1-y0)+y0;
    p->lastidx = idx;
    return OK;
}

static int32_t bpfxcos_k(CSOUND *csound, BPFX *p) {
    MYFLT x = *p->x;
    MYFLT **data = p->data;
    int32_t datalen = p->INOCOUNT - 1;
    MYFLT x0, x1, y0, y1, dx;

    int32_t idx = bpfx_find(data, x, datalen, p->lastidx);

    if(idx == -1) {
        *p->r = *data[1];
        p->lastidx = -1;
        return OK;
    }

    if(idx == -2) {
        *p->r = *data[datalen-1];
        p->lastidx = -1;
        return OK;
    }

    x0 = *data[idx];
    x1 = *data[idx+2];
    if(UNLIKELY(x0 > x || x >= x1))
        return NOTOK;
    y0 = *data[idx+1];
    y1 = *data[idx+3];
    dx = ((x-x0) / (x1-x0)) * PI + PI;
    *p->r = y0 + ((y1 - y0) * (1 + COS(dx)) / 2.0);
    p->lastidx = idx;
    return OK;
}

static int32 bpfxcos_i(CSOUND *csound, BPFX *p) {
    bpfx_init(csound, p);
    return bpfxcos_k(csound, p);
}

/*
 * ky bpf kx, kxs[], kys[]
 */

typedef struct {
    OPDS h;
    MYFLT *y, *x;
    ARRAYDAT *xs, *ys;
    int64_t lastidx;
} BPF_k_kKK;



static inline int64_t bpfarr_find(MYFLT x, MYFLT *xs, int64_t xslen, int64_t lastidx) {
    // -1: lower bound, -2: upper bound
    if(x <= xs[0]) {
        return -1;
    }
    if(x >= xs[xslen-1]) {
        return -2;
    }
    if(lastidx >= 0 && lastidx < xslen-2 && xs[lastidx] <= x && x < xs[lastidx+1]) {
        return lastidx;
    }

    int64_t imin = 0;
    int64_t imax = xslen;
    int64_t imid;

    while (imin < imax) {
        imid = (imax + imin) / 2;
        if (xs[imid] < x)
            imin = imid + 1;
        else
            imax = imid;
    }
    // now the right pair is in pairmin
    return imin - 1;
}


static int32_t bpf_k_kKK_init(CSOUND *csound, BPF_k_kKK *p) {
    IGN(csound);
    p->lastidx = -1;
    return OK;
}

static int32_t bpf_k_kKK_kr(CSOUND *csound, BPF_k_kKK *p) {
    IGN(csound);
    int64_t numxs = p->xs->sizes[0];
    int64_t numys = p->ys->sizes[0];
    int64_t N = numxs < numys ? numxs : numys;
    MYFLT *xs = p->xs->data;
    MYFLT *ys = p->ys->data;
    MYFLT x = *p->x;
    MYFLT x0, y0, x1, y1;

    int64_t idx = bpfarr_find(x, xs, N, p->lastidx);

    if(idx == -1) {
        *p->y = ys[0];
        p->lastidx = -1;
        return OK;
    }

    if(idx == -2) {
        *p->y = ys[N-1];
        p->lastidx = -1;
        return OK;
    }

    x0 = xs[idx];
    x1 = xs[idx+1];
    y0 = ys[idx];
    y1 = ys[idx+1];
    *p->y = (x-x0)/(x1-x0)*(y1-y0)+y0;
    p->lastidx = idx;
    return OK;

}



static int32_t bpf_k_kKK_ir(CSOUND *csound, BPF_k_kKK *p) {
    bpf_k_kKK_init(csound, p);
    return bpf_k_kKK_kr(csound, p);
}


static int32_t bpfcos_k_kKK_kr(CSOUND *csound, BPF_k_kKK *p) {
    IGN(csound);
    int32_t numxs = p->xs->sizes[0];
    int32_t numys = p->ys->sizes[0];
    int32_t N = numxs < numys ? numxs : numys;
    MYFLT *xs = p->xs->data;
    MYFLT *ys = p->ys->data;
    MYFLT x = *p->x;
    int32_t i = bpfarr_find(x, xs, N, p->lastidx);
    MYFLT x0, y0, x1, y1, dx;
    if(i == -1) {
        *p->y = ys[0];
        return OK;
    }
    if(i == -2) {
        *p->y = ys[N-1];
        return OK;
    }
    if(UNLIKELY(i == -3)) {
        return NOTOK;
    }
    x0 = xs[i];
    x1 = xs[i+1];
    y0 = ys[i];
    y1 = ys[i+1];
    dx = ((x-x0) / (x1-x0)) * PI + PI;
    *p->y = y0 + ((y1 - y0) * (1 + COS(dx)) / 2.0);
    return OK;
}

static int32_t bpfcos_k_kKK_ir(CSOUND *csound, BPF_k_kKK *p) {
    bpf_k_kKK_init(csound, p);
    return bpfcos_k_kKK_kr(csound, p);
}


// ay bpf ax, kxs[], kys[]

static int32_t bpf_a_aKK_kr(CSOUND *csound, BPF_k_kKK *p) {
    IGN(csound);
    int64_t numxs = p->xs->sizes[0];
    int64_t numys = p->ys->sizes[0];
    int64_t N = numxs < numys ? numxs : numys;
    int64_t i;
    MYFLT *xs = p->xs->data;
    MYFLT *ys = p->ys->data;
    MYFLT x0, y0, x1, y1, x;

    MYFLT *out = p->y;
    MYFLT *in = p->x;

    int64_t lastidx = p->lastidx;

    AUDIO_OPCODE(csound, p);
    AUDIO_OUTPUT(out);

    MYFLT firsty = ys[0];
    MYFLT lasty = ys[N-1];

    for(n=offset; n<nsmps; n++) {
        x = in[n];
        i = bpfarr_find(x, xs, N, lastidx);
        if(i == -1) {
            out[n] = firsty;
            lastidx = -1;
        } else if (i == -2) {
            out[n] = lasty;
            lastidx = -1;
        } else {
            x0 = xs[i];
            x1 = xs[i+1];
            if(UNLIKELY(x0 > x || x >= x1))
                return NOTOK;
            y0 = ys[i];
            y1 = ys[i+1];
            out[n] = (x-x0)/(x1-x0)*(y1-y0)+y0;
            lastidx = i;
        }
    }
    p->lastidx = lastidx;
    return OK;
}

static int32_t bpfcos_a_aKK_kr(CSOUND *csound, BPF_k_kKK *p) {
    IGN(csound);
    int64_t numxs = p->xs->sizes[0];
    int64_t numys = p->ys->sizes[0];
    int64_t N = numxs < numys ? numxs : numys;
    int64_t i;
    MYFLT *xs = p->xs->data;
    MYFLT *ys = p->ys->data;
    MYFLT x0, y0, x1, y1, x, dx;

    MYFLT *out = p->y;
    MYFLT *in = p->x;

    int64_t lastidx = p->lastidx;

    AUDIO_OPCODE(csound, p);
    AUDIO_OUTPUT(out);

    MYFLT firsty = ys[0];
    MYFLT lasty = ys[N-1];

    for(n=offset; n<nsmps; n++) {
        x = in[n];
        i = bpfarr_find(x, xs, N, lastidx);
        if(i == -1) {
            out[n] = firsty;
            lastidx = -1;
        } else if (i == -2) {
            out[n] = lasty;
            lastidx = -1;
        } else {
            x0 = xs[i];
            x1 = xs[i+1];
            if(UNLIKELY(x0 > x || x >= x1))
                return NOTOK;
            y0 = ys[i];
            y1 = ys[i+1];
            dx = ((x-x0) / (x1-x0)) * PI + PI;
            out[n] = y0 + ((y1 - y0) * (1 + COS(dx)) / 2.0);
            lastidx = i;
        }
    }
    p->lastidx = lastidx;
    return OK;
}
// ky, kz bpf kx, kxs[], kys[], kzs[]
typedef struct {
    OPDS h;
    MYFLT *y, *z, *x;
    ARRAYDAT *xs, *ys, *zs;
    int64_t lastidx;
} BPF_kk_kKKK;

static int32_t bpf_kk_kKKK_init(CSOUND *csound, BPF_kk_kKKK *p) {
    IGN(csound);
    p->lastidx = -1;
    return OK;
}

static int32_t bpf_kk_kKKK_kr(CSOUND *csound, BPF_kk_kKKK *p) {
    IGN(csound);
    int32_t numxs = p->xs->sizes[0];
    int32_t numys = p->ys->sizes[0];
    int32_t numzs = p->zs->sizes[0];
    int32_t N = numxs < numys ? numxs : numys;
    N = N < numzs ? N : numzs;

    MYFLT *xs = p->xs->data;
    MYFLT *ys = p->ys->data;
    MYFLT *zs = p->zs->data;
    MYFLT x = *p->x;
    int32_t i = bpfarr_find(x, xs, N, p->lastidx);

    if(i == -1) {
        *p->y = ys[0];
        *p->z = zs[0];
        return OK;
    }
    if(i == -2) {
        *p->y = ys[N-1];
        *p->z = zs[N-1];
        return OK;
    }

    MYFLT x0 = xs[i];
    MYFLT x1 = xs[i+1];
    MYFLT y0 = ys[i];
    MYFLT z0 = zs[i];

    MYFLT dx = (x-x0)/(x1-x0);
    *p->y = dx*(ys[i+1]-y0)+y0;
    *p->z = dx*(zs[i+1]-z0)+z0;
    p->lastidx = i;
    return OK;
}

static int32_t bpf_kk_kKKK_ir(CSOUND *csound, BPF_kk_kKKK *p) {
    bpf_kk_kKKK_init(csound, p);
    return bpf_kk_kKKK_kr(csound, p);
}


// kys[] bpf kxs[], kx0, ky0, kx1, ky1, ...
typedef struct {
    OPDS h;
    ARRAYDAT *out, *in;
    MYFLT *data[BPF_MAXPOINTS];
    int32_t lastidx;
} BPF_K_Km;


static int32_t bpf_K_Km_init(CSOUND *csound, BPF_K_Km *p) {
    tabinit(csound, p->out, p->in->sizes[0]);
    p->lastidx = -1;
    int32_t datalen = p->INOCOUNT - 1;
    if(datalen % 2)
        return INITERR(Str("bpf: data length should be even (pairs of x, y)"));
    if(datalen < 4)
        return INITERRF(Str("At least two pairs are needed, got %d"), datalen%2);
    if(datalen >= BPF_MAXPOINTS)
        return INITERR(Str("bpf: too many pargs (max=256)"));
    int32_t N = p->in->sizes[0];
    tabinit(csound, p->out, N);
    return OK;
}

static int32_t bpf_K_Km_kr(CSOUND *csound, BPF_K_Km *p) {
    int32_t N = p->in->sizes[0];
    ARRAY_ENSURESIZE_PERF(csound, p->out, N);

    MYFLT **data = p->data;
    MYFLT *out = p->out->data;
    MYFLT *in  = p->in->data;

    int32_t datalen = p->INOCOUNT - 1;
    int32_t idx, i;
    MYFLT x, x0, x1, y0, y1, firsty, lasty;
    firsty = *data[1];
    lasty = *data[datalen-1];
    int32_t lastidx = p->lastidx;

    for(idx=0; idx<N; idx++) {
        x = in[idx];
        i = bpfx_find(data, x, datalen, lastidx);

        if(i == -1) {
            out[idx] = firsty;
            lastidx = -1;
        } else if (i == -2) {
            out[idx] = lasty;
            lastidx = -1;
        } else {
            x0 = *data[i];
            x1 = *data[i+2];
            if(UNLIKELY(x0 > x || x >= x1))
                return NOTOK;
            y0 = *data[i+1];
            y1 = *data[i+3];
            out[idx] = (x-x0)/(x1-x0)*(y1-y0)+y0;
            lastidx = i;
        }
    }
    p->lastidx = lastidx;
    return OK;
}


typedef struct {
    OPDS h;
    MYFLT *out, *in;
    MYFLT *data[BPF_MAXPOINTS];
    int64_t lastidx;
} BPF_a_am;


static int32_t bpf_a_am_init(CSOUND *csound, BPF_a_am *p) {
    uint32_t datalen = p->INOCOUNT - 1;
    if(datalen % 2)
        return INITERRF(Str("bpf: data length should be even (pairs of x, y), got %d"), datalen);
    if(datalen < 4)
        return INITERRF(Str("At least two pairs are needed, got %d"), datalen%2);
    if(datalen >= BPF_MAXPOINTS)
        return INITERRF(Str("bpf: too many pargs (max=%d)"), BPF_MAXPOINTS);
    p->lastidx = -1;
    return OK;
}


static int32_t bpf_a_am_kr(CSOUND *csound, BPF_a_am *p) {
    MYFLT *out = p->out;
    MYFLT *in = p->in;
    uint32_t datalen = p->INOCOUNT - 1;
    int64_t lastidx = p->lastidx;

    AUDIO_OPCODE(csound, p);
    AUDIO_OUTPUT(out);

    MYFLT **data = p->data;

    int32_t i;

    MYFLT x, x0, x1, y0, y1, firsty, lasty;
    firsty = *data[1];
    lasty = *data[datalen-1];

    for(n=offset; n<nsmps; n++) {
        x = in[n];
        i = bpfx_find(data, x, datalen, lastidx);
        if(i == -1) {
            out[n] = firsty;
            lastidx = -1;
        } else if (i == -2) {
            out[n] = lasty;
            lastidx = -1;
        } else {
            x0 = *data[i];
            x1 = *data[i+2];
            if(UNLIKELY(x0 > x || x >= x1))
                return NOTOK;
            y0 = *data[i+1];
            y1 = *data[i+3];
            out[n] = (x-x0)/(x1-x0)*(y1-y0)+y0;
            lastidx = i;
        }
    }
    p->lastidx = lastidx;
    return OK;
}

// ay bpfcos ax, x0, y0, x1, y1, ...
static int32_t bpfcos_a_am_kr(CSOUND *csound, BPF_a_am *p) {
    MYFLT *out = p->out;
    MYFLT *in = p->in;
    uint32_t datalen = p->INOCOUNT - 1;
    int64_t lastidx = p->lastidx;

    AUDIO_OPCODE(csound, p);
    AUDIO_OUTPUT(out);

    MYFLT **data = p->data;

    int32_t i;

    MYFLT x, x0, x1, y0, y1, firsty, lasty, dx;
    firsty = *data[1];
    lasty = *data[datalen-1];

    for(n=offset; n<nsmps; n++) {
        x = in[n];
        i = bpfx_find(data, x, datalen, lastidx);
        if(i == -1) {
            out[n] = firsty;
            lastidx = -1;
        } else if (i == -2) {
            out[n] = lasty;
            lastidx = -1;
        } else {
            x0 = *data[i];
            x1 = *data[i+2];
            if(UNLIKELY(x0 > x || x >= x1))
                return NOTOK;
            y0 = *data[i+1];
            y1 = *data[i+3];
            dx = ((x-x0) / (x1-x0)) * PI + PI;
            out[n] = y0 + ((y1 - y0) * (1 + COS(dx)) / 2.0);
            lastidx = i;
        }
    }
    p->lastidx = lastidx;
    return OK;
}

// itab ftgen 0, 0, -2, 0, 100, 200
// aidx bpftab ax, ixtable
// ay table aidx, itab
// ay vecinterp kys[], aidx


// ky vecinterp kidx, kys[], imode=0, iparam=0
// ay vecinterp aidx, kys[], imode=0, iparam=0



// kys[] bpfcos kxs[], kx0, ky0, kx1, ky1, ...

static int32_t bpfcos_K_Km_kr(CSOUND *csound, BPF_K_Km *p) {
    int32_t N = p->in->sizes[0];
    ARRAY_ENSURESIZE_PERF(csound, p->out, N);

    MYFLT **data = p->data;
    MYFLT *out = p->out->data;
    MYFLT *in  = p->in->data;

    int32_t datalen = p->INOCOUNT - 1;
    int32_t idx, i;
    MYFLT x, x0, x1, y0, y1, firsty, lasty, dx;
    firsty = *data[1];
    lasty = *data[datalen-1];
    int32_t lastidx = p->lastidx;

    for(idx=0; idx<N; idx++) {
        x = in[idx];
        i = bpfx_find(data, x, datalen, lastidx);

        if(i == -1) {
            out[idx] = firsty;
            lastidx = -1;
        } else if (i == -2) {
            out[idx] = lasty;
            lastidx = -1;
        } else {
            x0 = *data[i];
            x1 = *data[i+2];
            if(UNLIKELY(x0 > x || x >= x1))
                return NOTOK;
            y0 = *data[i+1];
            y1 = *data[i+3];
            dx = ((x-x0) / (x1-x0)) * PI + PI;
            out[idx] = y0 + ((y1 - y0) * (1 + COS(dx)) / 2.0);
            lastidx = i;
        }
    }
    p->lastidx = lastidx;
    return OK;
}

/*  ntom  - mton

        midi to notename conversion

    imidi = ntom("A4-31")
    kmidi = ntom(Snotename)

    Snotename = mton(69.5)
    Snotename = mton(kmidi)

 */

typedef struct {
    OPDS h;
    MYFLT *r;
    STRINGDAT *notename;
} NTOM;

static int32_t _pcs[] = {9, 11, 0, 2, 4, 5, 7};

#define FAIL -999

static MYFLT ntomfunc(CSOUND *csound, char *note) {
    char *n = note;
    uint32_t notelen = strlen(note);
    int32_t octave = n[0] - '0';
    int32_t pcidx = n[1] - 'A';
    if (pcidx < 0 || pcidx >= 7) {
        csound->Message(csound,
                        Str("expecting a char between A and G, but got %c\n"),
                        n[1]);
        return FAIL;
    }
    int32_t pc = _pcs[pcidx];
    int32_t cents = 0;
    int32_t cursor;
    if (n[2] == '#') {
        pc += 1;
        cursor = 3;
    } else if (n[2] == 'b') {
        pc -= 1;
        cursor = 3;
    } else {
        cursor = 2;
    }
    int32_t rest = notelen - cursor;
    if (rest > 0) {
        int32_t sign = n[cursor] == '+' ? 1 : -1;
        if (rest == 1) {
            cents = 50;
        } else if (rest == 2) {
            cents = n[cursor + 1] - '0';
        } else if (rest == 3) {
            cents = 10 * (n[cursor + 1] - '0') + (n[cursor + 2] - '0');
        } else {
            csound->Message(csound,Str("format not understood, note: "
                                       "%s, notelen: %d\n"), n, notelen);
            return FAIL;
        }
        cents *= sign;
    }
    return ((octave + 1) * 12 + pc) + cents / FL(100.0);
}


static int32_t
ntom(CSOUND *csound, NTOM *p) {
    /*
       formats accepted: 8D+ (equals to +50 cents), 4C#, 8A-31 7Bb+30
       - no lowercase
       - octave is necessary and comes always first
       - no negative octaves, no octaves higher than 9
    */
    MYFLT midi = ntomfunc(csound, p->notename->data);
    if(midi == FAIL)
        return NOTOK;
    *p->r = midi;
    return OK;
}


/*

   mton -- midi to note

   Snote mton 69.5   -> 4A+50

 */

typedef struct {
    OPDS h;
    STRINGDAT *Sdst;
    MYFLT *kmidi;
} MTON;

//                                C  C# D D#  E  F  F# G G# A Bb B
static const int32_t _pc2idx[] = {2, 2, 3, 3, 4, 5, 5, 6, 6, 0, 1, 1};
static const int32_t _pc2alt[] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 2, 0};
static const char _alts[] = " #b";

static int32_t
mton(CSOUND *csound, MTON *p) {
    char *dst;
    MYFLT m = *p->kmidi;
    int32_t maxsize = 7; // 4C#+99\0
    if (p->Sdst->data == NULL) {
        p->Sdst->data = csound->Calloc(csound, maxsize);
        p->Sdst->size = maxsize;
    }
    dst = (char*) p->Sdst->data;
    int32_t octave = (int32_t) (m / 12 - 1);
    int32_t pc = (int32_t)m % 12;
    int32_t cents = round((m - floor(m)) * 100.0);
    int32_t sign, cursor;

    if (cents == 0) {
        sign = 0;
    } else if (cents <= 50) {
        sign = 1;
    } else {
        cents = 100 - cents;
        sign = -1;
        pc += 1;
        if (pc == 12) {
            pc = 0;
            octave += 1;
        }
    }
    if(octave >= 0) {
        dst[0] = '0' + octave;
        cursor = 1;
    } else {
        dst[0] = '-';
        dst[1] = '0' - octave;
        cursor = 2;
    }
    dst[cursor] = 'A' + _pc2idx[pc];
    cursor += 1;
    int32_t alt = _pc2alt[pc];
    if(alt > 0) {
        dst[cursor++] = _alts[alt];
    }
    if(sign == 1) {
        dst[cursor++] = '+';
        if (cents < 10) {
            dst[cursor++] = '0' + cents;
        } else if(cents != 50) {
            dst[cursor++] = '0' + (int32_t)(cents / 10);
            dst[cursor++] = '0' + (cents % 10);
        }
    } else if(sign == -1) {
        dst[cursor++] = '-';
        if(cents < 10) {
            dst[cursor++] = '0' + cents;
        } else if(cents != 50) {
            dst[cursor++] = '0' + (int32_t)(cents / 10);
            dst[cursor++] = '0' + (cents % 10);
        }
    }
    dst[cursor] = '\0';
    return OK;
}

/*

   ntof -- notename to frequency

   kfreq ntof Snotename

   kfreq = ntof("4A")  -> 440 (depending on the value of a4 variable)

 */

static int32_t
ntof(CSOUND *csound, NTOM *p) {
    MYFLT midi = ntomfunc(csound, p->notename->data);
    if(midi == FAIL)
        return NOTOK;
    MYFLT a4 = csound->GetA4(csound);
    *p->r = mtof_func(midi, a4);
    return OK;
}


/*

   cmp

   aout cmp a1, ">", a2
   aout cmp a1, "<=", k2
   aout cmp a1, "==", 0

   kOut[] cmp kA[], "<", k1
   kOut[] cmp kA[], "<", kB[]
   kOut[] cmp k1, "<", kA[], "<=", k2

 */

typedef struct {
    OPDS h;
    MYFLT *out, *a0;
    STRINGDAT *op;
    MYFLT *a1;
    int32_t mode;
} Cmp;

typedef struct {
    OPDS h;
    ARRAYDAT *out;
    ARRAYDAT *in;
    STRINGDAT *op;
    MYFLT *k1;
    int32_t mode;
} Cmp_array1;

typedef struct {
    OPDS h;
    ARRAYDAT *out;
    ARRAYDAT *in1;
    STRINGDAT *op;
    ARRAYDAT *in2;
    int32_t mode;
} Cmp_array2;

typedef struct {
    OPDS h;
    ARRAYDAT *out;
    MYFLT *a;
    STRINGDAT *op1;
    ARRAYDAT *in;
    STRINGDAT *op2;
    MYFLT *b;
    int32_t mode;
} Cmp2_array1;

static int32_t op2mode(char *op, int32_t opsize) {
    int32_t mode;
    if (op[0] == '>') {
        mode = (opsize == 1) ? 0 : 1;
    } else if (op[0] == '<') {
        mode = (opsize == 1) ? 2 : 3;
    } else if (op[0] == '=') {
        mode = 4;
    } else if (op[0] == '!' && op[1] == '=') {
        mode = 5;
    } else {
        return -1;
    }
    return mode;
}

static int32_t
cmp_init(CSOUND *csound, Cmp *p) {
    int32_t mode = op2mode(p->op->data, p->op->size-1);
    if(mode == -1) {
        return INITERR(Str("cmp: unknown operator. "
                           "Expecting <, <=, >, >=, ==, !="));
    }
    p->mode = mode;
    return OK;
}

static int32_t
cmparray1_init(CSOUND *csound, Cmp_array1 *p) {
    int32_t N = p->in->sizes[0];
    tabinit(csound, p->out, N);
    int32_t mode = op2mode(p->op->data, p->op->size-1);
    if(mode == -1) {
        return INITERR(Str("cmp: unknown operator. "
                           "Expecting <, <=, >, >=, ==, !="));
    }
    p->mode = mode;
    return OK;
}

static int32_t
cmparray2_init(CSOUND *csound, Cmp_array2 *p) {
    int32_t N1 = p->in1->sizes[0];
    int32_t N2 = p->in2->sizes[0];
    int32_t N = N1 < N2 ? N1 : N2;

    // make sure that we can put the result in `out`,
    // grow the array if necessary
    tabinit(csound, p->out, N);
    int32_t mode = op2mode(p->op->data, p->op->size-1);
    if(mode == -1) {
        return INITERR(Str("cmp: unknown operator. "
                           "Expecting <, <=, >, >=, ==, !="));
    }
    p->mode = mode;
    return OK;
}

static int32_t
cmp2array1_init(CSOUND *csound, Cmp2_array1 *p) {
    int32_t N = p->in->sizes[0];
    tabinit(csound, p->out, N);

    char *op1 = (char*)p->op1->data;
    int32_t op1size = p->op1->size - 1;
    char *op2 = (char*)p->op2->data;
    int32_t op2size = p->op2->size - 1;
    int32_t mode;

    if (op1[0] == '<') {
        mode = (op1size == 1) ? 0 : 1;
        if(op2[0] == '<')
            mode += 2 * ((op2size == 1) ? 0 : 1);
        else
            return INITERR(Str("cmp (ternary comparator): operator 2 expected <"));
    }
    else {
        return INITERR(Str("cmp (ternary comparator): operator 1 expected <"));
    }
    p->mode = mode;
    return OK;
}

static int32_t
cmp_aa(CSOUND *csound, Cmp* p) {
    IGN(csound);

    SAMPLE_ACCURATE

    MYFLT *a0 = p->a0;
    MYFLT *a1 = p->a1;

    switch(p->mode) {
    case 0:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] > a1[n];
        }
        break;
    case 1:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] >= a1[n];
        }
        break;
    case 2:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] < a1[n];
        }
        break;
    case 3:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] <= a1[n];
        }
        break;
    case 4:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] == a1[n];
        }
        break;
    case 5:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] != a1[n];
        }
        break;
    }
    return OK;
}

static int32_t
cmp_ak(CSOUND *csound, Cmp *p) {
    IGN(csound);

    SAMPLE_ACCURATE

    MYFLT *a0 = p->a0;
    MYFLT a1 = *(p->a1);

    switch(p->mode) {
    case 0:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] > a1;
        }
        break;
    case 1:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] >= a1;
        }
        break;
    case 2:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] < a1;
        }
        break;
    case 3:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] <= a1;
        }
        break;
    case 4:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] == a1;
        }
        break;
    case 5:
        for(n=offset; n<nsmps; n++) {
            out[n] = a0[n] != a1;
        }
        break;
    }
    return OK;
}

static int32_t
cmparray1_k(CSOUND *csound, Cmp_array1 *p) {
    int32_t L = p->in->sizes[0];
    ARRAY_ENSURESIZE_PERF(csound, p->out, L);

    MYFLT *out = p->out->data;
    MYFLT *in  = p->in->data;
    MYFLT k1 = *p->k1;
    int32_t i;

    switch(p->mode) {
    case 0:
        for(i=0; i<L; i++) {
            out[i] = in[i] > k1;
        }
        break;
    case 1:
        for(i=0; i<L; i++) {
            out[i] = in[i] >= k1;
        }
        break;
    case 2:
        for(i=0; i<L; i++) {
            out[i] = in[i] < k1;
        }
        break;
    case 3:
        for(i=0; i<L; i++) {
            out[i] = in[i] <= k1;
        }
        break;
    case 4:
        for(i=0; i<L; i++) {
            out[i] = in[i] == k1;
        }
        break;
    case 5:
        for(i=0; i<L; i++) {
            out[i] = in[i] != k1;
        }
        break;
    }
    return OK;
}

static int32_t
cmparray1_i(CSOUND *csound, Cmp_array1 *p) {
    cmparray1_init(csound, p);
    return cmparray1_k(csound, p);
}


static int32_t
cmp2array1_k(CSOUND *csound, Cmp2_array1 *p) {
    int32_t L = p->in->sizes[0];
    ARRAY_ENSURESIZE_PERF(csound, p->out, L);

    MYFLT *out = p->out->data;
    MYFLT *in  = p->in->data;
    MYFLT a = *p->a;
    MYFLT b = *p->b;
    int32_t i;
    MYFLT x;

    switch(p->mode) {
    case 0:   // 00 -> <   <
        for(i=0; i<L; i++) {
            x = in[i];
            out[i] = (a < x) && (x < b);
        }
        break;
    case 1:   // 01 -> <=   <
        for(i=0; i<L; i++) {
            x = in[i];
            out[i] = (a <= x) && (x < b);
        }
        break;
    case 2:   // 10 -> <   <=
        for(i=0; i<L; i++) {
            x = in[i];
            out[i] = (a < x) && (x <= b);
        }
        break;
    case 3:   // 11 -> <=   <=
        for(i=0; i<L; i++) {
            x = in[i];
            out[i] = (a <= x) && (x <= b);
        }
        break;
    }
    return OK;
}

static int32_t
cmp2array1_i(CSOUND *csound, Cmp2_array1 *p) {
    cmp2array1_init(csound, p);
    return cmp2array1_k(csound, p);
}

static int32_t
cmparray2_k(CSOUND *csound, Cmp_array2 *p) {
    int32_t L = p->in1->sizes[0];
    ARRAY_ENSURESIZE_PERF(csound, p->out, L);

    MYFLT *out = p->out->data;
    MYFLT *in1  = p->in1->data;
    MYFLT *in2  = p->in2->data;
    int32_t i;
    switch(p->mode) {
    case 0:
        for(i=0; i<L; i++) {
            out[i] = in1[i] > in2[i];
        }
        break;
    case 1:
        for(i=0; i<L; i++) {
            out[i] = in1[i] >= in2[i];
        }
        break;
    case 2:
        for(i=0; i<L; i++) {
            out[i] = in1[i] < in2[i];
        }
        break;
    case 3:
        for(i=0; i<L; i++) {
            out[i] = in1[i] <= in2[i];
        }
        break;
    case 4:
        for(i=0; i<L; i++) {
            out[i] = in1[i] == in2[i];
        }
        break;
    case 5:
        for(i=0; i<L; i++) {
            out[i] = in1[i] != in2[i];
        }
        break;
    }
    return OK;
}

static int32_t
cmparray2_i(CSOUND *csound, Cmp_array2 *p) {
    cmparray2_init(csound, p);
    return cmparray2_k(csound, p);
}


/*
  ftslice

  ftslice sourceTable:i, destTable:i, startIndex:k=0, endIndex:k=0, step:k=1

  copy slice from source table to dest table (end is not inclusive)

  sourceTable: the source table number
  destTable  : the destination table number
  startIndex : the index to start copying
  endIndex   : the index to end copying. NOT INCLUSIVE
  step       : how many indices to jump (1=contiguous)

  See also: tab2array
*/

typedef struct {
    OPDS h;
    MYFLT *fnsrc, *fndst, *kstart, *kend, *kstep;
    FUNC *ftpsrc;
    FUNC *ftpdst;
} TABSLICE;

static int32_t
tabslice_init(CSOUND *csound, TABSLICE *p) {
    FUNC *ftpsrc, *ftpdst;
    ftpsrc = csound->FTnp2Finde(csound, p->fnsrc);
    if(UNLIKELY(ftpsrc == NULL))
        return INITERRF("Source table not found: %d", (int)(*p->fnsrc));
    p->ftpsrc = ftpsrc;
    ftpdst = csound->FTnp2Finde(csound, p->fndst);
    if(UNLIKELY(ftpdst == NULL))
        return INITERRF("Destination table not found: %d", (int)(*p->fndst));
    p->ftpdst = ftpdst;
    return OK;
}

static int32_t
tabslice_k(CSOUND *csound, TABSLICE *p) {
    IGN(csound);
    FUNC *ftpsrc = p->ftpsrc;
    FUNC *ftpdst = p->ftpdst;
    int32_t start = (int32_t)*p->kstart;
    int32_t end = (int32_t)*p->kend;
    int32_t step = (int32_t)*p->kstep;
    if(end < 1)
        end = ftpsrc->flen;
    int32_t numitems = (int32_t) (ceil((end - start) / (float)step));
    if (numitems > (int32_t)ftpdst->flen)
        numitems = (int32_t)ftpdst->flen;
    MYFLT *src = ftpsrc->ftable;
    MYFLT *dst = ftpdst->ftable;

    int32_t i, j=start;
    for(i=0; i<numitems; i++) {
        dst[i] = src[j];
        j += step;
    }
    return OK;
}

static int32_t
tabslice_allk(CSOUND *csound, TABSLICE *p) {
    p->ftpsrc = csound->FTnp2Finde(csound, p->fnsrc);
    if(UNLIKELY(p->ftpsrc == NULL))
        return PERFERRF("Source table not found: %d", (int)*p->fnsrc);
    p->ftpdst = csound->FTnp2Finde(csound, p->fndst);
    if(UNLIKELY(p->ftpdst == NULL))
        return PERFERRF("Destination table not found: %d", (int)*p->fnsrc);
    return tabslice_k(csound, p);
}

static int32_t
tabslice_i(CSOUND *csound, TABSLICE *p) {
    int error = tabslice_init(csound, p);
    if(error)
        return NOTOK;
    return tabslice_k(csound, p);
    return OK;
}

/*
  ftset

  ftset table:k, value:k, startIndex:k=0, endIndex:k=0, step:k=1

  Set elements of table to value

  table      : the table to modify
  value      : the value to set the table elements to
  startIndex : the index to start copying
  endIndex   : the index to end copying. NOT INCLUSIVE
  step       : how many indices to jump (1=contiguous)

  ftset ift, 0      ; clears the table
  ftset ift, 1, 10  ; set elements between index 10 and end of the table to 1

*/

typedef struct {
    OPDS h;
    MYFLT *tabnum, *value, *kstart, *kend, *kstep;
    FUNC *tab;
    int lastTabnum;
} FTSET;


static int32_t
ftset_init(CSOUND *csound, FTSET *p) {
    IGN(csound);
    p->lastTabnum = -1;
    return OK;
}

static int32_t
ftset_common(CSOUND *csound, FTSET *p) {
    IGN(csound);
    FUNC *tab = p->tab;
    MYFLT *data = tab->ftable;
    int tablen = tab->flen;
    int32_t start = (int32_t)*p->kstart;
    int32_t end = (int32_t)*p->kend;
    int32_t step = (int32_t)*p->kstep;
    MYFLT value = *p->value;

    if(end <= 0)
        end += tab->flen;
    else if(end > tablen)
        end = tablen;

    if(step == 1 && value == 0) {
        // special case: clear the table, use memset
        memset(data + start, '\0', sizeof(MYFLT) * (end - start));
        return OK;
    }

    for(int i=start; i<end; i+=step) {
        data[i] = value;
    }
    return OK;
}

static int32_t
ftset_k(CSOUND *csound, FTSET *p) {
    int tabnum = (int)(*p->tabnum);
    FUNC *tab;
    if(UNLIKELY(tabnum != p->lastTabnum)) {
        tab = csound->FTnp2Finde(csound, p->tabnum);
        if(UNLIKELY(tab == NULL))
            return PERFERRF(Str("Table %d not found"), tabnum);
        p->tab = tab;
        p->lastTabnum = tabnum;
    } else if(UNLIKELY(p->tab == NULL))
        return PERFERR(Str("Table not set"));

    return ftset_common(csound, p);
}

static int32_t
ftset_i(CSOUND *csound, FTSET *p) {
    FUNC *tab = csound->FTnp2Finde(csound, p->tabnum);
    if(UNLIKELY(tab == NULL))
        return INITERRF(Str("Table %d not found"), (int)(*p->tabnum));
    p->tab = tab;
    return ftset_common(csound, p);
}

/*

  tab2array

  kout[] tab2array ifn, kstart=0, kend=0, kstep=1
  iout[] tab2array ifn, istart=0, iend=0, istep=1

  copy slice from table into an array

  kstart: start index
  kend: end index (not inclusive). 0=end of table/array
  kstep: increment

  To copy a slice of an array, see slicearray

 */


typedef struct {
    OPDS h;
    ARRAYDAT *out;
    MYFLT *ifn, *kstart, *kend, *kstep;
    FUNC * ftp;
    int numitems;
} TAB2ARRAY;

static int
tab2array_init(CSOUND *csound, TAB2ARRAY *p) {
    FUNC *ftp;
    ftp = csound->FTnp2Finde(csound, p->ifn);
    if (UNLIKELY(ftp == NULL))
        return NOTOK;
    p->ftp = ftp;
    int start = (int)*p->kstart;
    int end   = (int)*p->kend;
    int step  = (int)*p->kstep;
    if (end < 1)
        end = ftp->flen;
    int numitems = (int) (ceil((end - start) / (float)step));
    if(numitems < 0) {
        return PERFERR(Str("tab2array: cannot copy a negative number of items"));
    }
    tabinit(csound, p->out, numitems);
    p->numitems = numitems;
    return OK;
}

static int
tab2array_k(CSOUND *csound, TAB2ARRAY *p) {
    FUNC *ftp = p->ftp;
    int start = (int)*p->kstart;
    int end   = (int)*p->kend;
    int step  = (int)*p->kstep;
    if (end < 1)
        end = ftp->flen;
    int numitems = (int) (ceil((end - start) / (double)step));
    if(numitems < 0)
        return PERFERR(Str("tab2array: cannot copy a negative number of items"));

    ARRAY_ENSURESIZE_PERF(csound, p->out, numitems);
    p->numitems = numitems;

    MYFLT *out   = p->out->data;
    MYFLT *table = ftp->ftable;

    int i, j=0;
    for(i=start; i<end; i+=step) {
        out[j++] = table[i];
    }
    return OK;
}

static int
tab2array_i(CSOUND *csound, TAB2ARRAY *p) {
    if(tab2array_init(csound, p) == OK)
        return tab2array_k(csound, p);
    return NOTOK;
}


/*

  reshapearray

  Reshape an array, maintaining the capacity of the array
  (it does NOT resize the array).

  You can reshape a 2D array to another array of equal capacity
  of reshape a 1D array to a 2D array, or a 2D array to a 1D
  array

  reshapearray array[], inumrows, inumcols=0

  works with i and k arrays, at i-time and k-time

  1:  if the sizes of the array and the size of the reshaped array do not match
it needs an error message. Currently it is rather silent.

2: in calculating the sizes if numcols is zero (the default value)
multiplying by zero always leads to an error.

3:  in numrows is 0 you end up with a 0 x 0 array.

(3, 5)

0   1  2  3  4
10 11 12 13 14
20 21 22 23 24

*/

typedef struct {
    OPDS h;
    ARRAYDAT *in;
    MYFLT *numrows, *numcols;
} ARRAYRESHAPE;

static int32_t
arrayreshape(CSOUND *csound, ARRAYRESHAPE *p) {
    ARRAYDAT *a = p->in;
    int32_t dims = a->dimensions;
    int32_t i;
    int32_t numitems = 1;
    int32_t numrows = (int32_t)(*p->numrows);
    int32_t numcols = (int32_t)(*p->numcols);

    if(numrows < 0 || numcols < 0) {
        return INITERR(Str("reshapearray: neither numcols nor numrows can be negative"));
    }

    if(dims > 2) {
        return INITERR(Str("Arrays of more than 2 dimensions are not supported yet"));
    }

    for(i=0; i<dims; i++) {
        numitems *= a->sizes[i];
    }
    int32_t numitems2 = numrows * (numcols > 0 ? numcols : 1);
    if(numitems != numitems2)
      return INITERRF(Str("reshapearray: The number of items do not match."
                          "The array has %d elements, but the new shape"
                          "results in %d total elements"),
                      numitems, numitems2);

    if(dims == 2) {
        if(numcols==0) {
            // 2 dims to 1 dim
            a->dimensions = 1;
        }
        a->sizes[0] = numrows;
        a->sizes[1] = numcols;
        return OK;
    }

    if(numcols==0) {
        // 1 dim to 1 dim, nothing to do
        return OK;
    }

    if(numcols>0) {
        // 1 dim. to 2 dimensions
        a->sizes = csound->ReAlloc(csound, a->sizes, sizeof(int32_t)*2);
        a->dimensions = 2;
        a->sizes[0] = numrows;
        a->sizes[1] = numcols;
        return OK;
    }
    return PERFERR(Str("reshapearray: cannot reshape"));
}


/*
  printarray

  printarray karray[], [ktrig], [Sfmt], [Slabel]
  printarray iarray[], [Sfmt], [Slabel]
  printarray Sarray[], [ktrig], [Sfmt], [Slabel]

  Prints all the elements of the array (1- and 2- dymensions).

  ktrig=1   in the k-version, controls when to print
            if ktrig is -1, prints each cycle. Otherwise, prints whenever
            ktrig changes from 0 to 1
  Sfmt      Sets the format (passed to printf) for each element of the
            array (default="%.4f")
  Slabel    Optional string to print before the whole array

  Works with 1- and 2-dimensional arrays, at i- and k-time

*/

typedef struct {
    OPDS h;
    ARRAYDAT *in;
    MYFLT *trig;
    STRINGDAT *Sfmt;
    STRINGDAT *Slabel;

    int32_t lasttrig;
    const char *printfmt;
    char fmtdata[128];
    const char *label;
} ARRAYPRINTK;

typedef struct {
    OPDS h;
    ARRAYDAT *in;
    STRINGDAT *Sfmt;
    STRINGDAT *Slabel;
    int32_t lasttrig;

    const char *printfmt;
    char fmtdata[128];
    const char *label;
} ARRAYPRINT;

#define ARRPRINT_SEP (csound->MessageS(csound, CSOUNDMSG_ORCH, "\n"))
#define ARRPRINT_MAXLINE 1024
#define ARRPRINT_IDXLIMIT 100


static const uint32_t print_linelength = 80;
static const char default_printfmt[] = "%.4f";
static const char default_printfmt_str[] = "\"%s\"";

/** replace all occurrences of needle within src by replacement
 * and put the result in target, which should have enough memory to
 * hold the result
*/

void str_replace(char *dest, const char *src, const char *needle,
                 const char *replacement)
{
    char buffer[512] = { 0 };
    char *insert_point = &buffer[0];
    const char *tmp = src;
    size_t needle_len = strlen(needle);
    size_t repl_len = strlen(replacement);

    while (1) {
        const char *p = strstr(tmp, needle);

        // walked past last occurrence of needle; copy remaining part
        if (p == NULL) {
            strcpy(insert_point, tmp);
            break;
        }

        // copy part before needle
        memcpy(insert_point, tmp, p - tmp);
        insert_point += p - tmp;

        // copy replacement string
        memcpy(insert_point, replacement, repl_len);
        insert_point += repl_len;

        // adjust pointers, move on
        tmp = p + needle_len;
    }

    // write altered string back to target
    strcpy(dest, buffer);
}


static int32_t
arrayprint_init(CSOUND *csound, ARRAYPRINTK *p) {
    if(p->in->arrayType->varTypeName[0] == 'S' && p->in->dimensions > 1)
        return INITERR(Str("cannot print multidimensional string arrays"));
    if(p->in->dimensions > 2)
        return INITERRF(Str("only 1-D and 2-D arrays supported, got %d dimensions"),
                        p->in->dimensions);
    p->lasttrig = 0;
    char arraytype = p->in->arrayType->varTypeName[0];
    const char *default_fmt =
      arraytype == 'S' ? default_printfmt_str : default_printfmt;
    p->printfmt =
      (p->Sfmt == NULL || strlen(p->Sfmt->data) < 2) ? default_fmt : p->Sfmt->data;

    if(strstr(p->printfmt, "%d") != NULL) {
        str_replace(p->fmtdata, p->printfmt, "%d", "%.0f"); fflush(stdout);
        p->printfmt = p->fmtdata;
    }

    p->label = p->Slabel != NULL ? p->Slabel->data : NULL;
    return OK;
}

static int32_t
arrayprint_init_notrig(CSOUND *csound, ARRAYPRINT *p) {
    if(p->in->arrayType->varTypeName[0] == 'S' && p->in->dimensions > 1)
        return INITERR(Str("cannot print multidimensional string arrays"));
    if(p->in->dimensions > 2)
        return INITERRF(Str("only 1-D and 2-D arrays supported, got %d dimensions"),
                        p->in->dimensions);
    char arraytype = p->in->arrayType->varTypeName[0];
    const char *default_fmt =
      arraytype == 'S' ? default_printfmt_str : default_printfmt;
    p->printfmt =
      (p->Sfmt == NULL || strlen(p->Sfmt->data) < 2) ? default_fmt : p->Sfmt->data;

    if(strstr(p->printfmt, "%d") != NULL) {
        str_replace(p->fmtdata, p->printfmt, "%d", "%.0f"); fflush(stdout);
        p->printfmt = p->fmtdata;
    }

    p->label = p->Slabel != NULL ? p->Slabel->data : NULL;
    return OK;
}


// print a string arry
static int32_t arrprint_str(CSOUND *csound, ARRAYDAT *arr,
                            const char *fmt, const char *label) {
    int32_t i;
    uint32_t charswritten = 0;
    STRINGDAT *strs = (STRINGDAT *)(arr->data);
    char currline[ARRPRINT_MAXLINE];
    const uint32_t linelength = print_linelength;
    if(label != NULL)
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", label);
    // fmt = "%s";  // TODO, set default fmt according to type of array
    for(i = 0; i < arr->sizes[0]; ++i) {
        if(charswritten > 0) {
            currline[charswritten++] = ',';
            currline[charswritten++] = ' ';
        }
        charswritten += sprintf(currline + charswritten, fmt, strs[i].data);
        if(charswritten >= linelength) {
            currline[charswritten+1] = '\0';
            csound->MessageS(csound, CSOUNDMSG_ORCH, " %s\n", (char*)currline);
            charswritten = 0;
        }
    }

    if(charswritten > 0) {
        currline[charswritten+1] = '\0';
        csound->MessageS(csound, CSOUNDMSG_ORCH, " %s\n", (char*)currline);
    }
    return OK;
}

// print a numeric array
static int32_t arrprint(CSOUND *csound, ARRAYDAT *arr,
                         const char *fmt, const char *label) {
    MYFLT *in = arr->data;
    int32_t dims = arr->dimensions;
    int32_t i, j, startidx;
    const uint32_t linelength = print_linelength;
    char currline[ARRPRINT_MAXLINE];
    uint32_t charswritten = 0;
    int32_t showidx = 0;
    if(label != NULL) {
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", label);
    }
    switch(dims) {
    case 1:
        startidx = 0;
        if (arr->sizes[0] > ARRPRINT_IDXLIMIT) {
            showidx = 1;
        }
        for(i=0; i<arr->sizes[0]; i++) {
            charswritten += sprintf(currline+charswritten, fmt, in[i]);
            if(charswritten < linelength) {
                currline[charswritten++] = ' ';
            } else {
                currline[charswritten+1] = '\0';
                if (showidx) {
                    csound->MessageS(csound, CSOUNDMSG_ORCH,
                                     " %3d: %s\n", startidx, (char*)currline);
                } else {
                    csound->MessageS(csound, CSOUNDMSG_ORCH,
                                     " %s\n", (char*)currline);
                }
                charswritten = 0;
                startidx = i+1;
            }
        }
        if (charswritten > 0) {
            currline[charswritten] = '\0';
            if (showidx) {
                csound->MessageS(csound, CSOUNDMSG_ORCH,
                                 " %3d: %s\n", startidx, (char*)currline);
            } else {
                csound->MessageS(csound, CSOUNDMSG_ORCH,
                                 " %s\n", (char*)currline);
            }
        }
        break;
    case 2:
        for(i=0; i<arr->sizes[0]; i++) {
            charswritten += sprintf(currline+charswritten, " %3d: ", i);
            for(j=0; j<arr->sizes[1]; j++) {
                charswritten += sprintf(currline+charswritten, fmt, *in);
                if(charswritten < linelength) {
                    currline[charswritten++] = ' ';
                }
                else {
                    currline[charswritten+1] = '\0';
                    csound->MessageS(csound, CSOUNDMSG_ORCH,
                                     "%s\n", (char*)currline);
                    charswritten = 0;
                }
                in++;
            }
            if (charswritten > 0) {
                currline[charswritten] = '\0';
                csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)currline);
                charswritten = 0;
            }
        }
        break;
    }
    return OK;
}


static inline int32_t
arrprint_(CSOUND *csound, ARRAYDAT *arr, const char* fmt, const char* label) {
    char *typename = arr->arrayType->varTypeName;
    switch(typename[0]) {
    case 'i':
    case 'k':
        return arrprint(csound, arr, fmt, label);
    case 'S':
        return arrprint_str(csound, arr, fmt, label);
    }
    return INITERRF(Str("type not supported for printing: %s"), typename);
}

static int32_t
arrayprint_perf(CSOUND *csound, ARRAYPRINTK *p) {
    int32_t trig = (int32_t)*p->trig;
    int32_t ret = OK;
    if(trig < 0 || (trig>0 && p->lasttrig<=0)) {
        ret = arrprint_(csound, p->in, p->printfmt, p->label);
    }
    p->lasttrig = trig;
    return ret;
}

static int32_t
arrayprint_perf_notrig(CSOUND *csound, ARRAYPRINT *p) {
    return arrprint_(csound, p->in, p->printfmt, p->label);
}

static int32_t
arrayprint_i(CSOUND *csound, ARRAYPRINT *p) {
    const char *label = p->Slabel != NULL ? p->Slabel->data : NULL;
    return arrprint(csound, p->in, default_printfmt, label);
}

static int32_t
arrayprintf_i(CSOUND *csound, ARRAYPRINT *p) {
    char tmpfmt[256];
    const char *fmt;
    if(strlen(p->Sfmt->data) == 0) {

        fmt = default_printfmt;
    } else {
        if(strstr(p->Sfmt->data, "%d") == NULL) {
            fmt = p->Sfmt->data;
        } else {
            str_replace(tmpfmt, p->Sfmt->data, "%d", "%.0f");
            fmt = tmpfmt;
        }
    }
    const char *label = p->Slabel != NULL ? p->Slabel->data : NULL;
    return arrprint(csound, p->in, fmt, label);
}


/*

ftprint  - print the contents of a table (useful for debugging)

ftprint ifn, ktrig=1, kstart=0, kend=0, kstep=1, inumcols=0

ifn: the table to print
ktrig: table will be printed whenever this changes from non-positive
       to positive and is different to last trigger
kstart: start index
kend: end index (non inclusive)
kstep: number of elements to skip
inumcols: number of elements to print per line

See also: printarray

*/

typedef struct {
    OPDS h;
    MYFLT *ifn, *ktrig, *kstart, *kend, *kstep, *inumcols;
    int32_t lasttrig;
    int32_t numcols;
    FUNC *ftp;
} FTPRINT;

static int32_t ftprint_perf(CSOUND *csound, FTPRINT *p);

static int32_t
ftprint_init(CSOUND *csound, FTPRINT *p) {
    p->lasttrig = 0;
    p->numcols = (int32_t)*p->inumcols;
    if(p->numcols == 0)
        p->numcols = 10;
    p->ftp = csound->FTnp2Finde(csound, p->ifn);
    int32_t trig = (int32_t)*p->ktrig;

    if (trig > 0) {
        ftprint_perf(csound, p);
    }
    return OK;
}

/** allow negative indices to count from the end
 */
static int handle_negative_idx(uint32_t *out, int32_t idx, uint32_t length) {
    if(idx >= 0) {
        *out = (uint32_t) idx;
        return OK;
    }
    int64_t res = (int64_t)length + idx + 1;
    if(res < 0) {
        return NOTOK;
    }
    *out = (uint32_t) res;
    return NOTOK;
}

static int32_t
ftprint_perf(CSOUND *csound, FTPRINT *p) {
    int32_t trig = (int32_t)*p->ktrig;
    if(trig == 0) {
      p->lasttrig = 0;
      return OK;
    }
    if(trig > 0 && p->lasttrig > 0)
        return OK;
    p->lasttrig = trig;
    FUNC *ftp = p->ftp;
    const MYFLT *ftable = ftp->ftable;
    const uint32_t ftplen = ftp->flen;
    const uint32_t numcols = (uint32_t)p->numcols;
    const uint32_t step = (uint32_t)*p->kstep;
    uint32_t end, start;
    int error = handle_negative_idx(&start, (int32_t)*p->kstart, ftplen);
    if(error)
        return PERFERRF(Str("Could not handle start index: %d"),
                        (int32_t)*p->kstart);
    int32_t _end = (int32_t)*p->kend;
    if(_end == 0)
        end = ftplen;
    else {
        error = handle_negative_idx(&end, _end, ftplen);
        if(error)
            return PERFERRF(Str("Could not handle end index: %d"), _end);
    }
    const char *fmt = default_printfmt;
    char currline[ARRPRINT_MAXLINE];
    uint32_t i,
             elemsprinted = 0,
             charswritten = 0,
             startidx = start;
    csound->MessageS(csound, CSOUNDMSG_ORCH,
                     "ftable %d:\n", (int32_t)*p->ifn);
    for(i=start; i < end; i+=step) {
        charswritten += sprintf(currline+charswritten, fmt, ftable[i]);
        elemsprinted++;
        if(elemsprinted < numcols) {
            currline[charswritten++] = ' ';
        } else {
            currline[charswritten++] = '\0';
            csound->MessageS(csound, CSOUNDMSG_ORCH,
                             " %3d: %s\n", startidx, currline);
            startidx = i+step;
            elemsprinted = 0;
            charswritten = 0;
        }
    }
    if(charswritten > 0) {
        currline[charswritten] = '\0';
        csound->MessageS(csound, CSOUNDMSG_ORCH,
                         " %3d: %s\n", startidx, currline);
    }
    return OK;
}


/*
  bit | and & for array

*/

typedef struct {
    OPDS h;
    ARRAYDAT *out;
    ARRAYDAT *in1, *in2;
    int32_t numitems;
} BINOP_AAA;

static int32_t
array_binop_init(CSOUND *csound, BINOP_AAA *p) {
    int32_t numitems = 1;
    int32_t i;

    for(i=0; i<p->in1->dimensions; i++) {
        numitems *= p->in1->sizes[i];
    }
    tabinit(csound, p->out, numitems);
    p->numitems = numitems;
    return OK;
}

static int32_t
array_or(CSOUND *csound, BINOP_AAA *p) {
    int32_t numitems = p->numitems;
    ARRAY_ENSURESIZE_PERF(csound, p->out, numitems);
    int32_t i;
    MYFLT *out = p->out->data;
    MYFLT *in1 = p->in1->data;
    MYFLT *in2 = p->in2->data;

    for(i=0; i<numitems; i++) {
        *(out++) = (MYFLT)((int32_t)*(in1++) | (int32_t)*(in2++));
    }
    return OK;
}

static int32_t
array_and(CSOUND *csound, BINOP_AAA *p) {
    int32_t numitems = p->numitems;
    ARRAY_ENSURESIZE_PERF(csound, p->out, numitems);

    int32_t i;
    MYFLT *out = p->out->data;
    MYFLT *in1 = p->in1->data;
    MYFLT *in2 = p->in2->data;

    // TODO: ensure size AND shape
    for(i=0; i<numitems; i++) {
        *(out++) = (MYFLT)((int32_t)*(in1++) & (int32_t)*(in2++));
    }
    return OK;
}


typedef struct {
    OPDS h;
    MYFLT *iout, *ifn;
    // FUNC *ftp;
} FTEXISTS;

static int32_t
ftexists_init(CSOUND *csound, FTEXISTS *p) {
    int ifn = (int)*p->ifn;
    if(ifn == 0) {
        csound->DebugMsg(csound, Str("ftexists: table number is 0"));
        *p->iout = 0.;
    }
    FUNC *ftp = csound->FTnp2Find(csound, p->ifn);
    *p->iout = (ftp != NULL) ? 1.0 : 0.0;
    return OK;
}

/*

 lastcycle: 1 if this is the last performance cycle for this event

 kislast lastcycle

 if lastcycle() then
 ; do something
 endif

*/

typedef struct {
    OPDS h;
    MYFLT *out;
    int extracycles;
    int numcycles;
    // 0 - tied note, has extra time;
    // 1 - fixed p3, no extra time
    // 2 - fixed p3, extra time
    int mode;
    int fired;
} LASTCYCLE;

static int32_t
lastcycle_init(CSOUND *csound, LASTCYCLE *p) {
    MYFLT p3 = p->h.insdshead->p3.value;
    p->numcycles = p3 < 0 ? 0 :
        (int)(p->h.insdshead->offtim * csound->GetKr(csound) + 0.5);
    p->extracycles = p->h.insdshead->xtratim;
    if(p->extracycles == 0) {
        p->h.insdshead->xtratim = 1;
        p->extracycles = 1;
        // MSG(Str("lastcycle: adding an extra cycle to the duration of the event\n"));
    }
    p->numcycles += p->extracycles;
    if(p3 < 0) {
        p->mode = 0;
    }
    else if (p->extracycles > 0) {
        p->mode = 2;
    } else {
      csound->Warning(csound, "%s",
                      Str("lastcycle: no extra time defined, turnoff2 will"
                          " not be detected\n"));
        p->mode = 1;
    }
    *p->out = 0;
    p->fired = 0;
    return OK;
}

static int32_t
lastcycle(CSOUND *csound, LASTCYCLE *p) {
    IGN(csound);
    if(p->fired == 1) {
        // this prevents double firing in the case were a lower instr turns
        // us off
        *p->out = 0;
        return OK;
    }
    switch(p->mode) {
    case 1:
        p->numcycles--;
        if(p->numcycles == 0) {
            *p->out = 1;
            p->fired = 1;
        }
        break;
    case 2:
        p->numcycles--;
        if(p->h.insdshead->relesing) {
            p->extracycles--;
        }
        if(p->numcycles == 0 || p->extracycles == 0) {
            *p->out = 1;
            p->fired = 1;
        }
        break;
    case 0:
        if (p->h.insdshead->relesing) {
            p->extracycles -= 1;
            if(p->extracycles == 0) {
                *p->out = 1;
                p->fired = 1;
            }
        }
        break;
    }
    return OK;
}


/**
 * strstrip
 *
 * remove whitespace from left, right or both sides
 *
 * Sout strstrip Sin
 * Sout strstrip Sin, "l"
 * Sout strstrip Sin, "r"
 *
 */

// make sure that the out string has enough allocated space
// This can be run only at init time
static int32_t _string_ensure(CSOUND *csound, STRINGDAT *s, int size) {
    if (s->size >= (size_t)size)
        return OK;
    csound->ReAlloc(csound, s->data, size);
    s->size = size;
    return OK;
}

typedef struct {
    OPDS h;
    STRINGDAT *out;
    STRINGDAT *in;
    STRINGDAT *which;
} STR1_1;

static int32_t
stripl(CSOUND *csound, STR1_1 *p) {
    char *str = p->in->data;
    size_t idx0;
    for(idx0=0; idx0 < p->in->size; idx0++) {
        if(!isspace(str[idx0]))
            break;
    }
    // now idx points to start of content
    if(str[idx0] == 0) {
        // empty string
        _string_ensure(csound, p->out, 1);
        p->out->data[0] = 0;
        return OK;
    }
    str += idx0;
    size_t insize = strlen(str);
    _string_ensure(csound, p->out, insize);
    memcpy(p->out->data, str, insize);
    return OK;
}

static int32_t
stripr(CSOUND *csound, STR1_1 *p) {
    // Trim trailing space
    char *str = p->in->data;
    int size = strlen(str) - 1;
    const char *end = str + size;
    while(size && isspace(*end)) {
        end--;
        size--;
    }
    size += 1;
    if(size > 0) {
        _string_ensure(csound, p->out, size);
        memcpy(p->out->data, str, size);
    } else {
        _string_ensure(csound, p->out, 1);
        p->out->data[0] = 0;
    }
     return OK;

}

static int32_t
stripside(CSOUND *csound, STR1_1 *p) {
    if(p->which->size < 2)
        return INITERR("which should not be empty");
    char which = p->which->data[0];
    if(which == 'l')
        return stripl(csound, p);
    else if (which == 'r')
        return stripr(csound, p);
    return INITERRF("which should be one of 'l' or 'r', got %s", p->which->data);
}

// returns length
int _str_find_edges(const char *str, int *startidx) {
    // left
    int idx0 = 0;
    while(isspace((unsigned char)*str)) {
        str++;
        idx0++;
    }

    if(*str == 0) {
        // Only whitespace
        return 0;
    }

    // right
    int size = strlen(str) - 1;
    const char *end = str + size;
    while(size && isspace(*end)) {
        end--;
        size--;
    }
    *startidx = idx0;
    return size+1;
}


static int32_t
strstrip(CSOUND *csound, STR1_1 *p) {
    int startidx;
    int size = _str_find_edges(p->in->data, &startidx);
    if(size > 0) {
        _string_ensure(csound, p->out, size);
        memcpy(p->out->data, p->in->data + startidx, size);
    } else {
        _string_ensure(csound, p->out, 1);
        p->out->data[0] = 0;
    }
    return OK;
}


/*
 * println
 *
 * The same as printf but without a trigger
 *
 * println Sfmt, xvar1, [xvar2, ...]
 *
 * println "foo bar %s: %k,", Skey, kvalue
 *
 */


typedef struct {
    OPDS    h;
    STRINGDAT   *sfmt;
    MYFLT   *args[64];
    int allocatedBuf;
    int newline;
    int fmtlen;
    STRINGDAT buf;
    STRINGDAT strseg;
    int initDone;
} PRINTLN;


int32_t println_reset(CSOUND *csound, PRINTLN *p) {
    if(p->buf.data != NULL && p->allocatedBuf) {
        csound->Free(csound, p->buf.data);
        p->buf.data = NULL;
        p->buf.size = 0;
        p->allocatedBuf = 0;
    }
    if(p->strseg.data != NULL) {
        csound->Free(csound, p->strseg.data);
        p->strseg.data = NULL;
        p->strseg.size = 0;
    }
    return OK;
}

int32_t printsk_init(CSOUND *csound, PRINTLN *p) {
    size_t bufsize = 2048;
    size_t fmtlen = strlen(p->sfmt->data);
    int32_t numVals = (int32_t)p->INOCOUNT - 1;
    size_t maxSegmentSize = fmtlen + numVals*7 + 1;

    // Try to reuse memory from previous instances
    if(p->buf.size < bufsize || p->strseg.size < maxSegmentSize) {
        if(p->buf.data == NULL)
            p->buf.data = csound->Calloc(csound, bufsize);
        else
            p->buf.data = csound->ReAlloc(csound, p->buf.data, bufsize);
        p->buf.size = bufsize;
        if(p->strseg.data == NULL)
            p->strseg.data = csound->Malloc(csound, maxSegmentSize);
        else
            p->strseg.data = csound->ReAlloc(csound,
                                             p->strseg.data, maxSegmentSize);
        p->strseg.size = maxSegmentSize;
        p->allocatedBuf = 1;
        csound->RegisterResetCallback(csound, p,
                                      (int32_t(*)(CSOUND*, void*))(println_reset));
    } else {
        p->allocatedBuf = 0;
    }
    p->newline = 0;
    p->fmtlen = fmtlen;
    p->initDone = 1;
    return OK;
}

int32_t println_init(CSOUND *csound, PRINTLN *p) {
    int ret = printsk_init(csound, p);
    if(ret != OK)
        return INITERR(Str("Error while inititalizing println"));
    p->newline = 1;
    return OK;
}


// #define IS_AUDIO_ARG(x) (csound->GetTypeForArg(x) == &CS_VAR_TYPE_A)
#define IS_AUDIO_ARG(x) (!strcmp("a", csound->GetTypeForArg(x)->varTypeName))

// #define IS_STRING_ARG(x) (csound->GetTypeForArg(x) == &CS_VAR_TYPE_S)
#define IS_STRING_ARG(x) (!strcmp("S", csound->GetTypeForArg(x)->varTypeName))


// This is taken from OOps/str_ops.c, with minor modifications to adapt it
// to plugin API
// Memory is actually never allocated here
static int32_t
sprintf_opcode_(CSOUND *csound,
                PRINTLN *p,       /* opcode data structure pointer       */
                STRINGDAT *str,   /* pointer to space for output string  */
                const char *fmt,  /* format string                       */
                int fmtlen,       /* length of format string             */
                MYFLT **kvals,    /* array of argument pointers          */
                int32_t numVals,      /* number of arguments             */
                int32_t strCode)      /* bit mask for string arguments   */
{
    if(p->initDone == 0)
        return PERFERRF(Str("Opcode %s not initialised"), p->h.optext->t.opcod);
    int32_t     len = 0;
    char *outstring = str->data;
    MYFLT *parm = NULL;
    int32_t i = 0, j = 0, n;
    const char *segwaiting = NULL;
    int32_t maxChars;
    int32_t strsegsize = p->strseg.size;
    char *strseg = p->strseg.data;

    const char *fmtend = fmt+(fmtlen-0);

    for (i = 0; i < numVals; i++) {
        if(UNLIKELY( IS_AUDIO_ARG(kvals[i])) )
            return PERFERR(Str("a-rate argument not allowed"));
    }

    if (UNLIKELY((int32_t) ((OPDS*) p)->optext->t.inArgCount > 31)){
        return PERFERR(Str("too many arguments"));
    }
    if (numVals==0) {
        strcpy(str->data, fmt);
        return OK;
    }

    i = 0;

    while (1) {
        if (UNLIKELY(i >= strsegsize)) {
            csound->Warning(csound, "%s", "println: Allocating memory");
            strsegsize *= 2;
            p->strseg.data = strseg = csound->ReAlloc(csound, strseg, strsegsize);
            p->strseg.size = strsegsize;
        }
        if (*fmt != '%' && fmt != fmtend && *fmt != '\0') {
            strseg[i++] = *fmt++;
            continue;
        }
        if (fmt[0] == '%' && fmt[1] == '%') {
            strseg[i++] = *fmt++;   /* Odd code: %% is usually % and as we
                                   know the value of *fmt the loads are
                                   unnecessary */
            strseg[i++] = *fmt++;
            continue;
        }

        /* if already a segment waiting, then lets print it */
        if (segwaiting != NULL) {
            maxChars = str->size - len;
            strseg[i] = '\0';
            if (UNLIKELY(numVals <= 0)) {
                return PERFERR(Str("insufficient arguments for format"));
            }
            numVals--;
            strCode >>= 1;
            parm = kvals[j++];

            switch (*segwaiting) {
            case 'd':
            case 'i':
            case 'o':
            case 'x':
            case 'X':
            case 'u':
            case 'c':
                n = sprintf(outstring, strseg, (int32_t) MYFLT2LRND(*parm));
                break;
            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
                n = sprintf(outstring, strseg, (double)*parm);
                break;
            case 's':
                if(!IS_STRING_ARG(parm)) {
                    return PERFERRF(Str("String argument expected, but type is %s"),
                                    csound->GetTypeForArg(parm)->varTypeName);
                }
                if (((STRINGDAT*)parm)->data == str->data) {
                    return PERFERR(Str("output argument may not be "
                                       "the same as any of the input args"));
                }
                if ((((STRINGDAT*)parm)->size+strlen(strseg)) >= (uint32_t)maxChars) {
                    int32_t offs = outstring - str->data;
                    int newsize = str->size  +
                      ((STRINGDAT*)parm)->size + strlen(strseg);
                    csound->Warning(csound, "%s",
                                    Str("println/printsk: Allocating extra "
                                        "memory for output string"));
                    str->data = csound->ReAlloc(csound, str->data, newsize);
                    if(str->data == NULL){
                        return PERFERR(Str("memory allocation failure"));
                    }
                    str->size += ((STRINGDAT*)parm)->size + strlen(strseg);
                    maxChars += ((STRINGDAT*)parm)->size + strlen(strseg);
                    outstring = str->data + offs;
                }
                n = snprintf(outstring, maxChars, strseg, ((STRINGDAT*)parm)->data);
                break;
            default:
                return PERFERR(Str("invalid format string"));
            }
            if (n < 0 || n >= maxChars) {
                /* safely detected excess string length */
                int32_t offs = outstring - str->data;
                csound->Warning(csound, "%s",
                                Str("Allocating extra memory for output string"));
                str->data = csound->ReAlloc(csound, str->data, maxChars*2);
                if (str->data == NULL)
                    return PERFERR(Str("memory allocation failure"));
                outstring = str->data + offs;
                str->size = maxChars*2;
            }
            outstring += n;
            len += n;
            i = 0;
        }
        if (*fmt == '\0' || fmt == fmtend)
            break;

        /* copy the '%' */
        strseg[i++] = *fmt++;
        /* find the format code */
        segwaiting = fmt;

        while (!isalpha(*segwaiting) && segwaiting != fmtend && *segwaiting != '\0')
            segwaiting++;
    }
    if (UNLIKELY(numVals > 0)) {
        return PERFERR(Str("too many arguments for format"));
    }
    return OK;
}

int32_t println_perf(CSOUND *csound, PRINTLN *p) {
    int32_t err = sprintf_opcode_(csound, p, &p->buf,
                                  (char*)p->sfmt->data, p->fmtlen,
                                  &(p->args[0]), (int32_t)p->INOCOUNT - 1, 0);
    if(err!=OK)
        return NOTOK;
    csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", p->buf.data);
    return OK;
}

int32_t printsk_perf(CSOUND *csound, PRINTLN *p) {
    int32_t err = sprintf_opcode_(csound, p, &p->buf,
                                  (char*)p->sfmt->data, p->fmtlen,
                                  &(p->args[0]), (int32_t)p->INOCOUNT - 1, 0);
    if(err!=OK)
        return NOTOK;
    csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", p->buf.data);
    return OK;
}


/*

   Input types:

 - a, k, s, i, w, f,
 - o (optional i-rate, default to 0), O optional krate=0
 - p (opt, default to 1), P optional krate=1
 - q (opt, 10),
 - v(opt, 0.5),
 - j(opt, ?1), J optional krate=-1
 - h(opt, 127),
 - y (multiple inputs, a-type),
 - z (multiple inputs, k-type),
 - Z (multiple inputs, alternating k- and a-types),
 - m (multiple inputs, i-type),
 - M (multiple inputs, any type)
 - n (multiple inputs, odd number of inputs, i-type).
 - . anytype
 - ? optional
 - * any type, any number of inputs
 */

#define S(x) sizeof(x)

static OENTRY emugens_localops[] = {
    { "linlin", S(LINLIN1), 0, 2, "k", "kkkkk", NULL, (SUBR)linlin1_perf },
    { "linlin", S(LINLIN1), 0, 2, "k", "kkkop", NULL, (SUBR)linlin1_perf },
    { "linlin", S(LINLIN1), 0, 1, "i", "iiiop", (SUBR)linlin1_perf},
    { "linlin", S(LINLINARR1), 0, 3, "k[]", "k[]kkOP", (SUBR)linlinarr1_init,
      (SUBR)linlinarr1_perf},
    { "linlin", S(LINLINARR1), 0, 1, "i[]", "i[]iiop", (SUBR)linlinarr1_i},
    { "linlin", S(BLENDARRAY), 0, 3, "k[]", "kk[]k[]OP",
      (SUBR)blendarray_init, (SUBR)blendarray_perf},
    { "linlin", S(BLENDARRAY), 0, 1, "i[]", "ii[]i[]op", (SUBR)blendarray_i},
    { "lincos", S(LINLIN1), 0, 2, "k", "kkkOP", NULL, (SUBR)lincos_perf },
    { "lincos", S(LINLIN1), 0, 1, "i", "iiiop", (SUBR)lincos_perf },

    { "xyscale", S(XYSCALE), 0, 2, "k", "kkkkkk", NULL, (SUBR)xyscale },
    { "xyscale", S(XYSCALE), 0, 3, "k", "kkiiii", (SUBR)xyscalei_init,
        (SUBR)xyscalei },

    { "mtof", S(PITCHCONV), 0, 3, "k", "k", (SUBR)mtof_init, (SUBR)mtof },
    { "mtof", S(PITCHCONV), 0, 1, "i", "i", (SUBR)mtof_init },
    { "mtof", S(PITCHCONV_ARR), 0, 3, "k[]", "k[]",
      (SUBR)mtof_arr_init, (SUBR)mtof_arr },
    { "mtof", S(PITCHCONV_ARR), 0, 1, "i[]", "i[]", (SUBR)mtof_arr_init },

    { "ftom", S(PITCHCONV), 0, 3,  "k", "ko", (SUBR)ftom_init, (SUBR)ftom},
    { "ftom", S(PITCHCONV), 0, 1,  "i", "io", (SUBR)ftom_init},
    { "ftom", S(PITCHCONV_ARR), 0, 3,  "k[]", "k[]o",
      (SUBR)ftom_arr_init, (SUBR)ftom_arr},
    { "ftom", S(PITCHCONV_ARR), 0, 1,  "i[]", "i[]o",
      (SUBR)ftom_arr_init, (SUBR)ftom_arr},


    { "pchtom", S(PITCHCONV), 0, 1, "i", "i", (SUBR)pchtom },
    { "pchtom", S(PITCHCONV), 0, 2, "k", "k", NULL, (SUBR)pchtom },

    { "bpf.k_kM", S(BPFX), 0, 3, "k", "kM", (SUBR)bpfx_init, (SUBR)bpfx_k },
    { "bpfcos.k_kM", S(BPFX), 0, 3, "k", "kM", (SUBR)bpfx_init, (SUBR)bpfxcos_k },

    { "bpf.i_im", S(BPFX), 0, 1, "i", "im", (SUBR)bpfx_i },
    { "bpfcos.i_im", S(BPFX), 0, 1, "i", "im", (SUBR)bpfxcos_i },

    { "bpf.K_KM", S(BPF_K_Km), 0, 3, "k[]", "k[]M", (SUBR)bpf_K_Km_init, (SUBR)bpf_K_Km_kr },
    { "bpfcos.K_KM", S(BPF_K_Km), 0, 3, "k[]", "k[]M", (SUBR)bpf_K_Km_init, (SUBR)bpfcos_K_Km_kr },

    { "bpf.a_aM", S(BPF_a_am), 0, 3, "a", "aM", (SUBR)bpf_a_am_init, (SUBR)bpf_a_am_kr },
    { "bpfcos.a_aM", S(BPF_a_am), 0, 3, "a", "aM", (SUBR)bpf_a_am_init, (SUBR)bpfcos_a_am_kr },

    { "bpf.k_kKK", S(BPF_k_kKK), 0, 3, "k", "kk[]k[]", (SUBR)bpf_k_kKK_init, (SUBR)bpf_k_kKK_kr },
    { "bpfcos.k_kKK", S(BPF_k_kKK), 0, 3, "k", "kk[]k[]", (SUBR)bpf_k_kKK_init, (SUBR)bpfcos_k_kKK_kr },

    { "bpf.k_kII", S(BPF_k_kKK), 0, 3, "k", "ki[]i[]", (SUBR)bpf_k_kKK_init, (SUBR)bpf_k_kKK_kr },
    { "bpfcos.k_kII", S(BPF_k_kKK), 0, 3, "k", "ki[]i[]", (SUBR)bpf_k_kKK_init, (SUBR)bpfcos_k_kKK_kr },

    { "bpf.a_aKK", S(BPF_k_kKK), 0, 3, "a", "ak[]k[]", (SUBR)bpf_k_kKK_init, (SUBR)bpf_a_aKK_kr },
    { "bpfcos.a_aKK", S(BPF_k_kKK), 0, 3, "a", "ak[]k[]", (SUBR)bpf_k_kKK_init, (SUBR)bpfcos_a_aKK_kr },

    { "bpf.a_aII", S(BPF_k_kKK), 0, 3, "a", "ai[]i[]", (SUBR)bpf_k_kKK_init, (SUBR)bpf_a_aKK_kr },
    { "bpfcos.a_aII", S(BPF_k_kKK), 0, 3, "a", "ai[]i[]", (SUBR)bpf_k_kKK_init, (SUBR)bpfcos_a_aKK_kr },

    { "bpf.i_iII", S(BPF_k_kKK), 0, 1, "i", "ii[]i[]", (SUBR)bpf_k_kKK_ir },
    { "bpfcos.i_iII", S(BPF_k_kKK), 0, 1, "i", "ii[]i[]", (SUBR)bpfcos_k_kKK_ir },

    { "bpf.kk_kKKK", S(BPF_kk_kKKK), 0, 3, "kk", "kk[]k[]k[]", (SUBR)bpf_kk_kKKK_init, (SUBR)bpf_kk_kKKK_kr },
    // TODO

    { "bpf.kk_kIII", S(BPF_kk_kKKK), 0, 3, "kk", "ki[]i[]i[]", (SUBR)bpf_kk_kKKK_init, (SUBR)bpf_kk_kKKK_kr },
    // TODO

    { "bpf.ii_iIII", S(BPF_kk_kKKK), 0, 1, "ii", "ii[]i[]i[]", (SUBR)bpf_kk_kKKK_ir },
    // TODO


    { "ntom.i", S(NTOM), 0, 1, "i", "S", (SUBR)ntom },
    { "ntom.k", S(NTOM), 0, 3, "k", "S", (SUBR)ntom, (SUBR)ntom },

    { "mton.i", S(MTON), 0, 1, "S", "i", (SUBR)mton },
    { "mton.k", S(MTON), 0, 3, "S", "k", (SUBR)mton, (SUBR)mton },

    { "ntof.i", S(NTOM), 0, 1, "i", "S", (SUBR)ntof },
    { "ntof.k", S(NTOM), 0, 3, "k", "S", (SUBR)ntof, (SUBR)ntof },

    { "cmp", S(Cmp), 0, 3, "a", "aSa", (SUBR)cmp_init, (SUBR)cmp_aa,},
    { "cmp", S(Cmp), 0, 3, "a", "aSk", (SUBR)cmp_init, (SUBR)cmp_ak },
    { "cmp", S(Cmp_array1), 0, 3, "k[]", "k[]Sk",
      (SUBR)cmparray1_init, (SUBR)cmparray1_k },
    { "cmp", S(Cmp_array1), 0, 1, "i[]", "i[]Si", (SUBR)cmparray1_i },

    { "cmp", S(Cmp_array2), 0, 3, "k[]", "k[]Sk[]",
      (SUBR)cmparray2_init, (SUBR)cmparray2_k },
    { "cmp", S(Cmp_array2), 0, 1, "i[]", "i[]Si[]",
      (SUBR)cmparray2_i },
    { "cmp", S(Cmp2_array1), 0, 3, "k[]", "kSk[]Sk",
      (SUBR)cmp2array1_init, (SUBR)cmp2array1_k },
    { "cmp", S(Cmp2_array1), 0, 1, "i[]", "iSi[]Si", (SUBR)cmp2array1_i},


    { "##or",  S(BINOP_AAA), 0, 3, "k[]", "k[]k[]",
      (SUBR)array_binop_init, (SUBR)array_or},
    { "##and", S(BINOP_AAA), 0, 3, "k[]", "k[]k[]",
      (SUBR)array_binop_init, (SUBR)array_and},
    { "reshapearray", S(ARRAYRESHAPE), 0, 1, "", ".[]io", (SUBR)arrayreshape},

    { "ftslicei", S(TABSLICE), TB, 1, "", "iioop", (SUBR)tabslice_i },

    { "ftslice.perf", S(TABSLICE),  TB, 3, "", "iiOOP",
      (SUBR)tabslice_init, (SUBR)tabslice_k},

    { "ftslice.onlyperf", S(TABSLICE),  TB, 2, "", "kkOOP",
      NULL, (SUBR)tabslice_allk},

    { "ftset.i", S(FTSET), TW, 1, "", "iioop", (SUBR)ftset_i },
    { "ftset.k", S(FTSET), TW, 3, "", "kkOOP", (SUBR)ftset_init, (SUBR)ftset_k },

    { "tab2array", S(TAB2ARRAY), TR, 3, "k[]", "iOOP",
      (SUBR)tab2array_init, (SUBR)tab2array_k},
    { "tab2array", S(TAB2ARRAY), TR, 1, "i[]", "ioop", (SUBR)tab2array_i},

    { "printarray", S(ARRAYPRINTK), 0, 3, "", "k[]J",
      (SUBR)arrayprint_init, (SUBR)arrayprint_perf},
    { "printarray", S(ARRAYPRINTK), 0, 3, "", "k[]kS",
      (SUBR)arrayprint_init, (SUBR)arrayprint_perf},
    { "printarray.k_notrig", S(ARRAYPRINT), 0, 3, "", "k[]S",
      (SUBR)arrayprint_init_notrig, (SUBR)arrayprint_perf_notrig},

    { "printarray", S(ARRAYPRINTK), 0, 3, "", "k[]kSS",
      (SUBR)arrayprint_init, (SUBR)arrayprint_perf},

    { "printarray.k_notrig", S(ARRAYPRINT), 0, 3, "", "k[]SS",
      (SUBR)arrayprint_init_notrig, (SUBR)arrayprint_perf_notrig},


    { "printarray.i", S(ARRAYPRINT), 0, 1, "", "i[]", (SUBR)arrayprint_i},
    { "printarray.fmt_i", S(ARRAYPRINT), 0, 1, "", "i[]S", (SUBR)arrayprintf_i},
    { "printarray.fmt_label_i", S(ARRAYPRINT), 0, 1, "", "i[]SS",
      (SUBR)arrayprintf_i},

    { "printarray", S(ARRAYPRINTK), 0, 3, "", "S[]J",
      (SUBR)arrayprint_init, (SUBR)arrayprint_perf},
    { "printarray", S(ARRAYPRINTK), 0, 3, "", "S[]kS",
      (SUBR)arrayprint_init, (SUBR)arrayprint_perf},

    { "printarray", S(ARRAYPRINT), 0, 3, "", "S[]S",
      (SUBR)arrayprint_init_notrig, (SUBR)arrayprint_perf_notrig},

    { "printarray", S(ARRAYPRINT), 0, 3, "", "S[]SS",
      (SUBR)arrayprint_init_notrig, (SUBR)arrayprint_perf_notrig},

    { "ftprint", S(FTPRINT), TR, 3, "", "iPOOPo",
      (SUBR)ftprint_init, (SUBR)ftprint_perf },

    { "ftexists", S(FTEXISTS), TR, 1, "i", "i",
      (SUBR)ftexists_init},
    { "ftexists", S(FTEXISTS), TR, 3, "k", "k",
      (SUBR)ftexists_init, (SUBR)ftexists_init},
    { "lastcycle", S(LASTCYCLE), 0, 3, "k", "",
      (SUBR)lastcycle_init, (SUBR)lastcycle},
    { "strstrip.i_side", S(STR1_1), 0, 1, "S", "SS", (SUBR)stripside},
    { "strstrip.i", S(STR1_1), 0, 1, "S", "S", (SUBR)strstrip},
    { "println", S(PRINTLN), 0, 3, "", "SN",
      (SUBR)println_init, (SUBR)println_perf},
    { "printsk", S(PRINTLN), 0, 3, "", "SN",
      (SUBR)printsk_init, (SUBR)printsk_perf}
};

LINKAGE_BUILTIN(emugens_localops)
