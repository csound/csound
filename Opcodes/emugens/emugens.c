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

#include <csdl.h>
// #include "/usr/local/include/csound/csdl.h"

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

#define INITERR(m) (csound->InitError(csound, "%s", m))
/* These  two do not work with translations */
/* #define MSG(m) (csound->Message(csound, Str(m))) */
/* #define MSGF(m, ...) (csound->Message(csound, Str(m), __VA_ARGS__)) */
#define PERFERROR(m) (csound->PerfError(csound, &(p->h), "%s", m))

/* from Opcodes/arrays.c, original name: tabensure */
#include "arrays.h"

#if 0
static inline void
arrayensure(CSOUND *csound, ARRAYDAT *p, size_t size) {
    if (p->data==NULL || p->dimensions == 0 ||
        (p->dimensions==1 && p->sizes[0] < (int)size)) {
        size_t ss;
        if (p->data == NULL) {
            CS_VARIABLE *var = p->arrayType->createVariable(csound, NULL);
            p->arrayMemberSize = var->memBlockSize;
        }
        ss = p->arrayMemberSize*size;
        if (p->data==NULL)
            p->data = (MYFLT*)csound->Calloc(csound, ss);
        else
            p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
        if (p->dimensions==0) {
          p->dimensions = 1;
          p->sizes = (int*)csound->Malloc(csound, sizeof(size_t));
        }
        p->sizes[0] = size;
    }
#endif



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
    int32_t numitems;
} LINLINARR1;

static int32_t linlinarr1_init(CSOUND *csound, LINLINARR1 *p) {
    size_t numitems = p->xs->sizes[0];
    tabensure(csound, p->ys, numitems);
    p->numitems = numitems;
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
    if(numitems > p->numitems) {
        tabensure(csound, p->ys, numitems);
        p->numitems = numitems;
    }
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
    tabensure(csound, p->out, numitems);
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
        return PERFERROR(Str("linlin: Division by zero"));
    }
    int32_t numitemsA = p->A->sizes[0];
    int32_t numitemsB = p->B->sizes[0];
    int32_t numitems = numitemsA < numitemsB ? numitemsA : numitemsB;
    if(numitems > p->numitems) {
        tabensure(csound, p->out, numitems);
        p->numitems = numitems;
    }

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
        return PERFERROR(Str("lincos: Division by zero"));
    }
    // PI is defined in csoundCore.h, use that instead of M_PI from math.h, which
    // is not defined in windows
    MYFLT dx = ((x-x0) / (x1-x0)) * PI + PI;           // dx range pi - 2pi
    *p->kout = y0 + ((y1 - y0) * (1 + cos(dx)) / 2.0);
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

static int32_t mtof(CSOUND *csound, PITCHCONV *p) {
    IGN(csound);
    *p->r = POWER(FL(2.0), (*p->k - FL(69.0)) / FL(12.0)) * p->freqA4;
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
    p->rnd = (int)*p->irnd;
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


/*
   bpf  --> break point function with linear interpolation

   Useful for smaller cases where:

 * defining a table is overkill
 * higher accuracy in the x coord
 * values are changing at k-rate

   ky  bpf  kx, kx0, ky0, kx1, ky1, ...
   kY[] bpf kX[], kx0, ky0, kx1, ky1, ...

 */

#define BPF_MAXPOINTS 256

typedef struct {
    OPDS h;
    MYFLT *r, *x, *data[BPF_MAXPOINTS];
} BPFX;

static int32_t bpfx(CSOUND *csound, BPFX *p) {
    MYFLT x = *p->x;
    MYFLT **data = p->data;
    int32_t datalen = p->INOCOUNT - 1;
    if(datalen % 2) {
        return INITERR(Str("bpf: data length should be even (pairs of x, y)"));
    }
    if(datalen >= BPF_MAXPOINTS) {
        return INITERR(Str("bpf: too many pargs (max=256)"));
    }
    int32_t i;
    MYFLT x0, x1, y0, y1;
    x0 = *data[0];
    y0 = *data[1];

    if (x <= x0) {
        *p->r = y0;
        return OK;
    }
    if (x>=*data[datalen-2]) {
        *p->r = *data[datalen-1];
        return OK;
    }
    for(i=2; i<datalen; i+=2) {
        x1 = *data[i];
        y1 = *data[i+1];
        if( x <= x1 ) {
            *p->r = (x-x0)/(x1-x0)*(y1-y0)+y0;
            return OK;
        }
        x0 = x1;
        y0 = y1;
    }
    return NOTOK;
}

static int32_t bpfxcos(CSOUND *csound, BPFX *p) {
    MYFLT x = *p->x;
    MYFLT **data = p->data;
    int32_t datalen = p->INOCOUNT - 1;
    if(datalen % 2)
        return INITERR(Str("bpf: data length should be even (pairs of x, y)"));
    if(datalen >= BPF_MAXPOINTS)
        return INITERR(Str("bpf: too many pargs (max=256)"));
    int32_t i;
    MYFLT x0, x1, y0, y1, dx;
    x0 = *data[0];
    y0 = *data[1];

    if (x <= x0) {
        *p->r = y0;
        return OK;
    }
    if (x>=*data[datalen-2]) {
        *p->r = *data[datalen-1];
        return OK;
    }
    for(i=2; i<datalen; i+=2) {
        x1 = *data[i];
        y1 = *data[i+1];
        if( x <= x1 ) {
            dx = ((x-x0) / (x1-x0)) * PI + PI;
            *p->r = y0 + ((y1 - y0) * (1 + cos(dx)) / 2.0);
            return OK;
        }
        x0 = x1;
        y0 = y1;
    }
    return NOTOK;
}

typedef struct {
    OPDS h;
    ARRAYDAT *out, *in;
    MYFLT *data[BPF_MAXPOINTS];
} BPFARR;

static int32_t bpfarr(CSOUND *csound, BPFARR *p) {
    int32_t N = p->in->sizes[0];
    tabensure(csound, p->out, N);
    MYFLT **data = p->data;
    MYFLT *out = p->out->data;
    MYFLT *in  = p->in->data;

    int32_t datalen = p->INOCOUNT - 1;
    if(datalen % 2)
        return INITERR(Str("bpf: data length should be even (pairs of x, y)"));
    if(datalen >= BPF_MAXPOINTS)
        return INITERR(Str("bpf: too many pargs (max=256)"));
    int32_t idx, i;
    MYFLT x, x0, x1, y0, y1, firstx, firsty, lastx, lasty;
    firstx = *data[0];
    firsty = *data[1];
    lastx = *data[datalen-2];
    lasty = *data[datalen-1];

    for(idx=0; idx<N; idx++) {
        x = in[idx];
        x0 = firstx;
        y0 = firsty;

        if (x <= x0)
            out[idx] = y0;
        else if (x>=lastx)
            out[idx] = lasty;
        else {
            for(i=2; i<datalen; i+=2) {
                x1 = *data[i];
                y1 = *data[i+1];
                if( x <= x1 ) {
                    out[idx] = (x-x0)/(x1-x0)*(y1-y0)+y0;
                    break;
                }
                x0 = x1;
                y0 = y1;
            }
        }
    }
    return OK;
}


static int32_t bpfarrcos(CSOUND *csound, BPFARR *p) {
    int32_t N = p->in->sizes[0];
    tabensure(csound, p->out, N);
    MYFLT **data = p->data;
    MYFLT *out = p->out->data;
    MYFLT *in  = p->in->data;

    int32_t datalen = p->INOCOUNT - 1;
    if(datalen % 2)
        return INITERR(Str("bpf: data length should be even (pairs of x, y)"));
    if(datalen >= BPF_MAXPOINTS)
        return INITERR(Str("bpf: too many pargs (max=256)"));

    int32_t idx, i;
    MYFLT x, x0, x1, y0, y1, firstx, firsty, lastx, lasty, dx;
    firstx = *data[0];
    firsty = *data[1];
    lastx = *data[datalen-2];
    lasty = *data[datalen-1];

    for(idx=0; idx<N; idx++) {
        x = in[idx];
        x0 = firstx;
        y0 = firsty;

        if (x <= x0)
            out[idx] = y0;
        else if (x>=lastx)
            out[idx] = lasty;
        else {
            for(i=2; i<datalen; i+=2) {
                x1 = *data[i];
                y1 = *data[i+1];
                if( x <= x1 ) {
                    dx = ((x-x0) / (x1-x0)) * PI + PI;
                    out[idx] = y0 + ((y1 - y0) * (1 + cos(dx)) / 2.0);
                    break;
                }
                x0 = x1;
                y0 = y1;
            }
        }
    }
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

int32_t _pcs[] = {9, 11, 0, 2, 4, 5, 7};

static int32_t
ntom(CSOUND *csound, NTOM *p) {
    /*
       formats accepted: 8D+ (equals to +50 cents), 4C#, 8A-31 7Bb+30
       - no lowercase
       - octave is necessary and comes always first
       - no negative octaves, no octaves higher than 9
     */
    char *n = (char *) p->notename->data;
    int32_t octave = n[0] - '0';
    int32_t pcidx = n[1] - 'A';
    if (pcidx < 0 || pcidx >= 7) {
        csound->Message(csound,
                        Str("expecting a char between A and G, but got %c\n"),
                        n[1]);
        return NOTOK;
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
    int32_t rest = p->notename->size - 1 - cursor;
    if (rest > 0) {
        int32_t sign = n[cursor] == '+' ? 1 : -1;
        if (rest == 1) {
            cents = 50;
        } else if (rest == 2) {
            cents = n[cursor + 1] - '0';
        } else if (rest == 3) {
            cents = 10 * (n[cursor + 1] - '0') + (n[cursor + 2] - '0');
        } else {
            csound->Message(csound,"%s", Str("format not understood\n"));
            return NOTOK;
        }
        cents *= sign;
    }
    *p->r = ((octave + 1) * 12 + pc) + cents / FL(100.0);
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

//                   C  C# D D#  E  F  F# G G# A Bb B
int32_t _pc2idx[] = {2, 2, 3, 3, 4, 5, 5, 6, 6, 0, 1, 1};
int32_t _pc2alt[] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 2, 0};
char _alts[] = " #b";

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
    int32_t octave = m / 12 - 1;
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
        return INITERR(Str("cmp: unknown operator. Expecting <, <=, >, >=, ==, !="));
    }
    p->mode = mode;
    return OK;
}

static int32_t
cmparray1_init(CSOUND *csound, Cmp_array1 *p) {
    int32_t N = p->in->sizes[0];
    tabensure(csound, p->out, N);
    int32_t mode = op2mode(p->op->data, p->op->size-1);
    if(mode == -1) {
        return INITERR(Str("cmp: unknown operator. Expecting <, <=, >, >=, ==, !="));
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
    tabensure(csound, p->out, N);
    int32_t mode = op2mode(p->op->data, p->op->size-1);
    if(mode == -1) {
        return INITERR(Str("cmp: unknown operator. Expecting <, <=, >, >=, ==, !="));
    }
    p->mode = mode;
    return OK;
}

static int32_t
cmp2array1_init(CSOUND *csound, Cmp2_array1 *p) {
    int32_t N = p->in->sizes[0];
    tabensure(csound, p->out, N);

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
    MYFLT *out = p->out->data;
    MYFLT *in  = p->in->data;
    MYFLT k1 = *p->k1;
    int32_t L = p->out->sizes[0];
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
    MYFLT *out = p->out->data;
    MYFLT *in  = p->in->data;
    MYFLT a = *p->a;
    MYFLT b = *p->b;
    int32_t L = p->out->sizes[0];
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
    MYFLT *out = p->out->data;
    MYFLT *in1  = p->in1->data;
    MYFLT *in2  = p->in2->data;
    int32_t L = p->out->sizes[0];
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
    ftpsrc = csound->FTnp2Find(csound, p->fnsrc);
    if(UNLIKELY(ftpsrc == NULL))
        return NOTOK;
    p->ftpsrc = ftpsrc;
    ftpdst = csound->FTnp2Find(csound, p->fndst);
    if(UNLIKELY(ftpdst == NULL))
        return NOTOK;
    p->ftpdst = ftpdst;
    return OK;
}

static int32_t
tabslice_k(CSOUND *csound, TABSLICE *p) {
    FUNC *ftpsrc = p->ftpsrc;
    FUNC *ftpdst = p->ftpdst;
    int32_t start = (int32_t)*p->kstart;
    int32_t end = (int32_t)*p->kend;
    int32_t step = (int32_t)*p->kstep;
    if(end < 1)
        end = ftpsrc->flen;
    uint32_t numitems = (uint32_t)ceil((end - start) / (float)step);
    if(numitems > ftpdst->flen)
        numitems = ftpdst->flen;
    MYFLT *src = ftpsrc->ftable;
    MYFLT *dst = ftpdst->ftable;

    uint32_t i, j=start;
    for(i=0; i<numitems; i++) {
        dst[i] = src[j];
        j += step;
    }
    return OK;
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
    ftp = csound->FTnp2Find(csound, p->ifn);
    if (UNLIKELY(ftp == NULL))
        return NOTOK;
    p->ftp = ftp;
    int start = (int)*p->kstart;
    int end   = (int)*p->kend;
    int step  = (int)*p->kstep;
    if (end < 1)
        end = ftp->flen;
    int numitems = (int)ceil((end - start) / (float)step);
    if(numitems < 0) {
        return PERFERROR(Str("tab2array: can't copy a negative number of items"));
    }
    tabensure(csound, p->out, numitems);
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
    int numitems = (int)ceil((end - start) / (float)step);
    if(numitems < 0)
        return PERFERROR(Str("tab2array: can't copy a negative number of items"));
    if(numitems > p->numitems) {
        tabensure(csound, p->out, numitems);
        p->numitems = numitems;
    }
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
    int32_t numcols = (int32_t)(*p->numcols);
    int32_t numrows = (int32_t)(*p->numrows);
    for(i=0; i<dims; i++) {
        numitems *= a->sizes[i];
    }
    int32_t numitems2 = numcols*numrows;
    if(numitems != numitems2)
        return NOTOK;
    if(dims == 2 && numrows==0) {
        // 2 -> 1
        a->dimensions = 1;
        a->sizes[0] = numrows;
        a->sizes[1] = 0;
        return OK;
    }
    if(dims == 2) {
        // 2 -> 2
        a->sizes[0] = numrows;
        a->sizes[1] = numcols;
        return OK;
    }
    if(numcols>0) {
        // 1 -> 2
        csound->Free(csound, a->sizes);
        a->sizes = (int32_t*)csound->Malloc(csound, sizeof(int32_t)*2);
        a->dimensions = 2;
        a->sizes[0] = numrows;
        a->sizes[1] = numcols;
        return OK;
    }
    return PERFERROR(Str("reshapearray: can't reshape"));
}


/*
  printarray

  printarray karray[], [ktrig], [Sfmt], [Slabel]
  printarray iarray[], [Sfmt], [Slabel]

  Prints all the elements of the array (1- and 2- dymensions).

  ktrig=1   in the k-version, controls when to print
            if ktrig is -1, prints each cycle. Otherwise, prints whenever
            ktrig changes from 0 to 1
  Sfmt      Sets the format (passed to printf) for each element of the array (default="%.4f")
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
    const char *label;
} ARRAYPRINTK;

typedef struct {
    OPDS h;
    ARRAYDAT *in;
    STRINGDAT *Sfmt;
    STRINGDAT *Slabel;
    int32_t lasttrig;
} ARRAYPRINT;

static const uint32_t print_linelength = 80;
static const char default_printfmt[] = "%.4f";


static int32_t
arrayprint_init(CSOUND *csound, ARRAYPRINTK *p) {
    p->lasttrig = 0;
    if(p->Sfmt == NULL) {
        p->printfmt = default_printfmt;
    } else {
        p->printfmt = p->Sfmt->size > 1 ? p->Sfmt->data : default_printfmt;
    }
    p->label = p->Slabel != NULL ? p->Slabel->data : NULL;
    return OK;
}


static inline void arrprint(const char *fmt, int dims, MYFLT *data, int dim0, int dim1, const char *label) {
    MYFLT *in = data;
    int32_t i, j;
    const uint32_t linelength = print_linelength;

    uint32_t charswritten = 0;
    if(label != NULL) {
        printf("%s\n", label);
    }
    switch(dims) {
    case 1:
        printf("  ");
        for(i=0; i<dim0; i++) {
            charswritten += printf(fmt, in[i]) + 1;
            if(charswritten < linelength) {
                printf(" ");
            } else {
                printf("\n  ");
                charswritten = 0;
            }
        }
        break;
    case 2:
        for(i=0; i<dim0; i++) {
            printf(" %3d: ", i);
            for(j=0; j<dim1; j++) {
                charswritten += printf(fmt, *in) + 1;
                if(charswritten < linelength)
                    printf(" ");
                else {
                    printf("\n");
                    charswritten = 0;
                }
                in++;
            }
            printf("\n");
            charswritten = 0;
        }
        break;
    }
    printf("\n");
}


static inline void arrprinti(ARRAYPRINT *p, const char* fmt) {
    int dims = p->in->dimensions;
    int dim0 = p->in->sizes[0];
    int dim1 = dims > 1 ? p->in->sizes[1] : 0;
    const char *label = p->Slabel != NULL ? p->Slabel->data : NULL;
    arrprint(fmt, dims, p->in->data, dim0, dim1, label);
}

static int32_t
arrayprint_perf(CSOUND *csound, ARRAYPRINTK *p) {
    int32_t trig = (int32_t)*p->trig;
    int dims, dim0, dim1;
    if(trig < 0 || (trig>0 && p->lasttrig<=0)) {
        dims = p->in->dimensions;
        dim0 = p->in->sizes[0];
        dim1 = dims > 1 ? p->in->sizes[1] : 0;
        arrprint(p->printfmt, dims, p->in->data, dim0, dim1, p->label);
    }
    p->lasttrig = trig;
    return OK;
}

static int32_t
arrayprint_i(CSOUND *csound, ARRAYPRINT *p) {
    arrprinti(p, default_printfmt);
    return OK;
}

static int32_t
arrayprintf_i(CSOUND *csound, ARRAYPRINT *p) {
    const char *fmt = p->Sfmt->size > 1 ? p->Sfmt->data : default_printfmt;
    arrprinti(p, fmt);
    return OK;
}


/*

ftprint  - print the contents of a table (useful for debugging)

ftprint ifn, ktrig=1, kstart=0, kend=0, kstep=1, inumcols=0

ifn: the table to print
ktrig: table will be printed whenever this changes from non-positive to positive
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


static int32_t
ftprint_init(CSOUND *csound, FTPRINT *p) {
    p->lasttrig = 0;
    p->numcols = (int32_t)*p->inumcols;
    if(p->numcols == 0)
        p->numcols = 10;
    p->ftp = csound->FTnp2Find(csound, p->ifn);
    return OK;
}

static int32_t
ftprint_perf(CSOUND *csound, FTPRINT *p) {
    FUNC *ftp = p->ftp;
    int32_t start = (int32_t)*p->kstart;
    uint32_t end = (uint32_t)*p->kend;
    int32_t step = (int32_t)*p->kstep;
    uint32_t ftplen = ftp->flen;
    if(end < 1 || end > ftplen)
        end = ftplen;
    MYFLT *ftable = ftp->ftable;
    uint32_t i;
    int32_t numcols = (int32_t)p->numcols;
    int32_t elemsprinted = 0;
    printf("ftable %d:\n%3d: ", (int32_t)*p->ifn, start);
    for(i=start; i<end; i+=step) {
      printf(default_printfmt, ftable[i]);// + 1;
        elemsprinted++;
        if(elemsprinted < numcols) {
            printf(" ");
        } else {
            printf("\n%3d: ", i+1);
            elemsprinted = 0;
        }
    }
    printf("\n");
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
    tabensure(csound, p->out, numitems);
    p->numitems = numitems;
    return OK;
}

static int32_t
array_or(CSOUND *csound, BINOP_AAA *p) {
    int32_t numitems = p->numitems;
    int32_t i;
    MYFLT *out = p->out->data;
    MYFLT *in1 = p->in1->data;
    MYFLT *in2 = p->in2->data;

    // TODO: ensure size AND shape
    for(i=0; i<numitems; i++) {
        *(out++) = (MYFLT)((int32_t)*(in1++) | (int32_t)*(in2++));
    }
    return OK;
}

static int32_t
array_and(CSOUND *csound, BINOP_AAA *p) {
    int32_t numitems = p->numitems;
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


/*

   Input types:
 * a, k, s, i, w, f,
 * o (optional i-rate, default to 0), O optional krate=0
 * p (opt, default to 1), P optional krate=1
 * q (opt, 10),
 * v(opt, 0.5),
 * j(opt, ?1), J optional krate=-1
 * h(opt, 127),
 * y (multiple inputs, a-type),
 * z (multiple inputs, k-type),
 * Z (multiple inputs, alternating k- and a-types),
 * m (multiple inputs, i-type),
 * M (multiple inputs, any type)
 * n (multiple inputs, odd number of inputs, i-type).
 * . anytype
 * ? optional
 */

#define S(x) sizeof(x)

static OENTRY localops[] = {
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

    { "ftom", S(PITCHCONV), 0, 3,  "k", "ko", (SUBR)ftom_init, (SUBR)ftom},
    { "ftom", S(PITCHCONV), 0, 1,  "i", "io", (SUBR)ftom_init},

    { "pchtom", S(PITCHCONV), 0, 1, "i", "i", (SUBR)pchtom },
    { "pchtom", S(PITCHCONV), 0, 2, "k", "k", NULL, (SUBR)pchtom },

    { "bpf", S(BPFX), 0, 2, "k", "kM", NULL, (SUBR)bpfx },
    { "bpf", S(BPFX), 0, 1, "i", "im", (SUBR)bpfx },
    { "bpf", S(BPFARR), 0, 2, "k[]", "k[]M", NULL, (SUBR)bpfarr },
    { "bpfcos", S(BPFX), 0, 2, "k", "kM", NULL, (SUBR)bpfxcos },
    { "bpfcos", S(BPFX), 0, 1, "i", "im", (SUBR)bpfxcos },
    { "bpfcos", S(BPFARR), 0, 2, "k[]", "k[]M", NULL, (SUBR)bpfarrcos },

    { "ntom", S(NTOM), 0, 3, "k", "S", (SUBR)ntom, (SUBR)ntom },
    { "ntom", S(NTOM), 0, 1, "i", "S", (SUBR)ntom },

    { "mton", S(MTON), 0, 3, "S", "k", (SUBR)mton, (SUBR)mton },
    { "mton", S(MTON), 0, 1, "S", "i", (SUBR)mton },

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
    // { "reshapearray", S(ARRAYRESHAPE), 0, 1, "", "i[]io", (SUBR)arrayreshape},
    // { "reshapearray", S(ARRAYRESHAPE), 0, 2, "", ".[]io", NULL, (SUBR)arrayreshape},
    { "ftslice", S(TABSLICE),  0, 3, "", "iiOOP",
      (SUBR)tabslice_init, (SUBR)tabslice_k},
    { "tab2array", S(TAB2ARRAY), 0, 3, "k[]", "iOOP",
      (SUBR)tab2array_init, (SUBR)tab2array_k},
    { "tab2array", S(TAB2ARRAY), 0, 1, "i[]", "ioop", (SUBR)tab2array_i},

    { "printarray", S(ARRAYPRINTK), 0, 3, "", "k[]P", (SUBR)arrayprint_init, (SUBR)arrayprint_perf},
    { "printarray", S(ARRAYPRINTK), 0, 3, "", "k[]kS", (SUBR)arrayprint_init, (SUBR)arrayprint_perf},
    { "printarray", S(ARRAYPRINTK), 0, 3, "", "k[]kSS", (SUBR)arrayprint_init, (SUBR)arrayprint_perf},

    { "printarray", S(ARRAYPRINT), 0, 1, "", "i[]", (SUBR)arrayprint_i},
    { "printarray", S(ARRAYPRINT), 0, 1, "", "i[]S", (SUBR)arrayprintf_i},
    { "printarray", S(ARRAYPRINT), 0, 1, "", "i[]SS", (SUBR)arrayprintf_i},

    { "ftprint", S(FTPRINT), 0, 3, "", "iPOOPo", (SUBR)ftprint_init, (SUBR)ftprint_perf },

};

LINKAGE
