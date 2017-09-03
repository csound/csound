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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include <csdl.h>

#define SAMPLE_ACCURATE \
uint32_t offset = p->h.insdshead->ksmps_offset;                   \
uint32_t early  = p->h.insdshead->ksmps_no_end;                   \
if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));    \
if (UNLIKELY(early)) {                                            \
    nsmps -= early;                                               \
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));               \
}                                                                 \

#define INITERR(m) (csound->InitError(csound, Str(m)))

/* from Opcodes/arrays.c, original name: tabensure */
static inline void arrayensure(CSOUND *csound, ARRAYDAT *p, int size)
{
    if (p->data==NULL || p->dimensions == 0 || (p->dimensions==1 && p->sizes[0] < size)) {
        size_t ss;
        if (p->data == NULL) {
            CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
            p->arrayMemberSize = var->memBlockSize;
        }
        ss = p->arrayMemberSize*size;
        if (p->data==NULL) {
            p->data = (MYFLT*)csound->Calloc(csound, ss);
        }
        else {
            p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
        }
        p->dimensions = 1;
        p->sizes = (int*)csound->Malloc(csound, sizeof(int));
        p->sizes[0] = size;
    }
}


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

static int linlin1_perf(CSOUND* csound, LINLIN1* p)
{
    MYFLT x0 = *p->kx0;
    MYFLT y0 = *p->ky0;
    MYFLT x = *p->kx;
    MYFLT x1 = *p->kx1;
    if (UNLIKELY(x0 == x1))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("linlin.k: Division by zero"));
    *p->kout = (x - x0) / (x1 -x0) * (*(p->ky1) - y0) + y0;
    return OK;
}

typedef struct {
    OPDS h;
    // kY[] linlin kX[], ky0, ky1, kx0=0, kx1=1
    ARRAYDAT *ys, *xs;
    MYFLT *ky0, *ky1, *kx0, *kx1;
    int numitems;
} LINLINARR1;

static int linlinarr1_init(CSOUND* csound, LINLINARR1* p) {
    int numitems = p->xs->sizes[0];
    arrayensure(csound, p->ys, numitems);
    p->numitems = numitems;
    return OK;
}

static int linlinarr1_perf(CSOUND* csound, LINLINARR1* p)
{
    MYFLT x0 = *p->kx0;
    MYFLT y0 = *p->ky0;
    MYFLT x1 = *p->kx1;
    MYFLT y1 = *p->ky1;

    if (UNLIKELY(x0 == x1))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("linlin: Division by zero"));
    MYFLT fact = 1/(x1 - x0) * (y1 - y0);

    int numitems = p->xs->sizes[0];
    if(numitems > p->numitems) {
        arrayensure(csound, p->ys, numitems);
        p->numitems = numitems;
    }
    MYFLT *out = p->ys->data;
    MYFLT *in  = p->xs->data;
    int i;
    for(i=0; i<numitems; i++) {
        out[i] = (in[i] - x0) * fact + y0;
    }
    return OK;
}

static int linlinarr1_i(CSOUND* csound, LINLINARR1* p) {
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
    int numitems;
} BLENDARRAY;

static int blendarray_init(CSOUND* csound, BLENDARRAY* p) {
    int numitemsA = p->A->sizes[0];
    int numitemsB = p->B->sizes[0];
    int numitems = numitemsA < numitemsB ? numitemsA : numitemsB;
    arrayensure(csound, p->out, numitems);
    p->numitems = numitems;
    return OK;
}

static int blendarray_perf(CSOUND* csound, BLENDARRAY* p)
{
    MYFLT x0 = *p->kx0;
    MYFLT x1 = *p->kx1;
    MYFLT x = *p->kx;

    if (UNLIKELY(x0 == x1))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("linlin: Division by zero"));
    int numitemsA = p->A->sizes[0];
    int numitemsB = p->B->sizes[0];
    int numitems = numitemsA < numitemsB ? numitemsA : numitemsB;
    if(numitems > p->numitems) {
        arrayensure(csound, p->out, numitems);
        p->numitems = numitems;
    }

    MYFLT *out = p->out->data;
    MYFLT *A = p->A->data;
    MYFLT *B = p->B->data;
    MYFLT y0, y1;
    MYFLT fact = (x - x0) / (x1 - x0);
    int i;
    for(i=0; i<numitems; i++) {
        y0 = A[i];
        y1 = B[i];
        out[i] = (y1 - y0) * fact + y0;
    }
    return OK;
}

static int blendarray_i(CSOUND* csound, BLENDARRAY* p) {
    blendarray_init(csound, p);
    return blendarray_perf(csound, p);
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

static int xyscalei_init(CSOUND* csound, XYSCALE* p)
{
    p->d0 = (*p->v01) - (*p->v00);
    p->d1 = (*p->v11) - (*p->v10);
    return OK;
}

static int xyscalei(CSOUND* csound, XYSCALE* p)
{
    // x, y: between 0-1
    MYFLT x = *p->kx;
    MYFLT y0 = x * (p->d0) + (*p->v00);
    MYFLT y1 = x * (p->d1) + (*p->v10);
    *p->kout = (*p->ky) * (y1 - y0) + y0;
    return OK;
}

static int xyscale(CSOUND* csound, XYSCALE* p)
{
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

kfreq = mtof(69)

*/

typedef struct {
    OPDS h;
    MYFLT *r, *k;
    MYFLT freqA4;
} PITCHCONV;

static int mtof(CSOUND* csound, PITCHCONV* p)
{
    *p->r = POWER(FL(2.0), (*p->k - FL(69.0)) / FL(12.0)) * p->freqA4;
    return OK;
}

static int mtof_init(CSOUND* csound, PITCHCONV* p)
{
    p->freqA4 = csound->GetA4(csound);
    mtof(csound, p);
    return OK;
}

static int ftom(CSOUND* csound, PITCHCONV* p)
{
    *p->r = FL(12.0) * LOG2(*p->k / p->freqA4) + FL(69.0);
    return OK;
}

static int ftom_init(CSOUND* csound, PITCHCONV* p)
{
    p->freqA4 = csound->GetA4(csound);
    ftom(csound, p);
    return OK;
}

static int pchtom(CSOUND* csound, PITCHCONV* p)
{
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

*/

typedef struct {
    OPDS h;
    MYFLT *r, *x, *data[256];
} BPFX;

static int bpfx(CSOUND *csound, BPFX *p) {
    // TODO: implement as a binary search
    MYFLT x = *p->x;
    MYFLT **data = p->data;
    int datalen = p->INOCOUNT -  1;
    if(datalen % 2) {
        printf("datalen: %d\n", datalen);
        return INITERR("bpf: number of data points should be even (pairs of x, y)");
    }
    int i;
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
    for(i=2;i<datalen;i+=2) {
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

/*  ntom  - mton

        midi to notename conversion

    imidi = ntom("A4-31")
    kmidi = ntom(Snotename)

    Snotename = mton(69.5)
    Snotename = mton(kmidi)

*/

typedef struct {
    OPDS h;
    MYFLT* r;
    STRINGDAT* notename;
} NTOM;

int _pcs[] = { 9, 11, 0, 2, 4, 5, 7 };

static int ntom(CSOUND* csound, NTOM* p)
{
    /*
      formats accepted: 8D+ (equals to +50 cents), 4C#, 8A-31 7Bb+30
      - no lowercase
      - octave is necessary and comes always first
      - no negative octaves, no octaves higher than 9
    */
    char* n = (char*)p->notename->data;
    int octave = n[0] - '0';
    int pcidx = n[1] - 'A';
    if (pcidx < 0 || pcidx >= 7) {
        csound->Message(csound,
            Str("expecting a char between A and G, but got %c\n"),
            n[1]);
        return NOTOK;
    }
    int pc = _pcs[pcidx];
    int cents = 0;
    int cursor;
    if (n[2] == '#') {
        pc += 1;
        cursor = 3;
    }
    else if (n[2] == 'b') {
        pc -= 1;
        cursor = 3;
    }
    else {
        cursor = 2;
    }
    int rest = p->notename->size - 1 - cursor;
    if (rest > 0) {
        int sign = n[cursor] == '+' ? 1 : -1;
        if (rest == 1) {
            cents = 50;
        }
        else if (rest == 2) {
            cents = n[cursor + 1] - '0';
        }
        else if (rest == 3) {
            cents = 10 * (n[cursor + 1] - '0') + (n[cursor + 2] - '0');
        }
        else {
            csound->Message(csound, Str("format not understood\n"));
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
    STRINGDAT* Sdst;
    MYFLT* kmidi;
} MTON;

//                C  C# D D#  E  F  F# G  G# A  Bb B
int _pc2idx[] = { 2, 2, 3, 3, 4, 5, 5, 6, 6, 0, 1, 1 };
int _pc2alt[] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 2, 0 };
char _alts[] = " #b";

static int mton(CSOUND* csound, MTON* p)
{
    char* dst;
    MYFLT m = *p->kmidi;
    int maxsize = 7; // 4C#+99\0
    if (p->Sdst->data == NULL) {
        p->Sdst->data = csound->Calloc(csound, maxsize);
        p->Sdst->size = maxsize;
    }
    dst = (char*)p->Sdst->data;
    int octave = m / 12 - 1;
    int pc = (int)m % 12;
    int cents = round((m - floor(m)) * 100.0);
    int sign, cursor, i;

    if (cents == 0) {
        sign = 0;
    }
    else if (cents <= 50) {
        sign = 1;
    }
    else {
        cents = 100 - cents;
        sign = -1;
        pc += 1;
        if (pc == 12) {
            pc = 0;
            octave += 1;
        }
    }
    if (octave >= 0) {
        dst[0] = '0' + octave;
        cursor = 1;
    }
    else {
        dst[0] = '-';
        dst[1] = '0' - octave;
        cursor = 2;
    }
    dst[cursor] = 'A' + _pc2idx[pc];
    cursor += 1;
    int alt = _pc2alt[pc];
    if (alt > 0) {
        dst[cursor++] = _alts[alt];
    }
    if (sign == 1) {
        dst[cursor++] = '+';
        if (cents < 10) {
            dst[cursor++] = '0' + cents;
        }
        else if (cents != 50) {
            dst[cursor++] = '0' + (int)(cents / 10);
            dst[cursor++] = '0' + (cents % 10);
        }
    }
    else if (sign == -1) {
        dst[cursor++] = '-';
        if (cents < 10) {
            dst[cursor++] = '0' + cents;
        }
        else if (cents != 50) {
            dst[cursor++] = '0' + (int)(cents / 10);
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
    STRINGDAT* op;
    MYFLT* a1;
    int mode;
} Cmp;

typedef struct {
    OPDS h;
    ARRAYDAT *out;
    ARRAYDAT *in;
    STRINGDAT* op;
    MYFLT *k1;
    int mode;
} Cmp_array1;

typedef struct {
    OPDS h;
    ARRAYDAT *out;
    ARRAYDAT *in1;
    STRINGDAT* op;
    ARRAYDAT *in2;
    int mode;
} Cmp_array2;


typedef struct {
    OPDS h;
    ARRAYDAT *out;
    MYFLT *a;
    STRINGDAT* op1;
    ARRAYDAT *in;
    STRINGDAT* op2;
    MYFLT *b;
    int mode;
} Cmp2_array1;

static int cmp_init(CSOUND* csound, Cmp* p) {
    char* op = (char*)p->op->data;
    int opsize = p->op->size - 1;

    if (op[0] == '>')
        p->mode = (opsize == 1) ? 0 : 1;
    else if (op[0] == '<') 
        p->mode = (opsize == 1) ? 2 : 3;
    else if (op[0] == '=')
        p->mode = 4;
    else if (op[0] == '!' && op[1] == '=')
        p->mode = 5;
    else {
        return INITERR("cmp: operator not understood. Expecting <, <=, >, >=, ==, !=");
    }
    return OK;
}



static int cmparray1_init(CSOUND* csound, Cmp_array1* p) {
    int N = p->in->sizes[0];
    arrayensure(csound, p->out, N);

    char* op = (char*)p->op->data;
    int opsize = p->op->size - 1;

    if (op[0] == '>')
        p->mode = (opsize == 1) ? 0 : 1;
    else if (op[0] == '<')
        p->mode = (opsize == 1) ? 2 : 3;
    else if (op[0] == '=')
        p->mode = 4;
    else if (op[0] == '!' && op[1] == '=')
        p->mode = 5;
    else {
        return INITERR("cmp: operator not understood. Expecting <, <=, >, >=, ==, !=");
    }
    return OK;
}

static int cmparray2_init(CSOUND* csound, Cmp_array2* p) {
    int N1 = p->in1->sizes[0];
    int N2 = p->in2->sizes[0];
    int N = N1 < N2 ? N1 : N2;

    // make sure that we can put the result in `out`,
    // grow the array if necessary
    arrayensure(csound, p->out, N);

    char* op = (char*)p->op->data;
    int opsize = p->op->size - 1;

    if (op[0] == '>')
        p->mode = (opsize == 1) ? 0 : 1;
    else if (op[0] == '<') 
        p->mode = (opsize == 1) ? 2 : 3;
    else if (op[0] == '=')
        p->mode = 4;
    else if (op[0] == '!' && op[1] == '=')
        p->mode = 5;
    else {
        return INITERR("cmp: operator not understood. Expecting <, <=, >, >=, ==, !=");
    }
    return OK;
}

static int cmp2array1_init(CSOUND* csound, Cmp2_array1* p) {
    int N = p->in->sizes[0];
    arrayensure(csound, p->out, N);

    char* op1 = (char*)p->op1->data;
    int op1size = p->op1->size - 1;
    char* op2 = (char*)p->op2->data;
    int op2size = p->op2->size - 1;
    int mode;

    if (op1[0] == '<') {
        mode = (op1size == 1) ? 0 : 1;
        if(op2[0] == '<') {
            mode += 2 * ((op2size == 1) ? 0 : 1);
        }
        else {
            return INITERR("cmp (ternary comparator): operator 2 expected <");
        }
    }
    else {
        return INITERR("cmp (ternary comparator): operator 1 expected <");
    }
    p->mode = mode;
    return OK;
}


static int cmp_aa(CSOUND* csound, Cmp* p) {
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT* out = p->out;
    MYFLT* a0 = p->a0;
    MYFLT* a1 = p->a1;

    SAMPLE_ACCURATE

    switch (p->mode) {
    case 0:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] > a1[n];
        }
        break;
    case 1:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] >= a1[n];
        }
        break;
    case 2:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] < a1[n];
        }
        break;
    case 3:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] <= a1[n];
        }
        break;
    case 4:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] == a1[n];
        }
        break;
    case 5:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] != a1[n];
        }
        break;
    }
    return OK;
}

static int cmp_ak(CSOUND* csound, Cmp* p) {
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT* out = p->out;
    MYFLT* a0 = p->a0;
    MYFLT a1 = *(p->a1);

    SAMPLE_ACCURATE

    switch (p->mode) {
    case 0:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] > a1;
        }
        break;
    case 1:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] >= a1;
        }
        break;
    case 2:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] < a1;
        }
        break;
    case 3:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] <= a1;
        }
        break;
    case 4:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] == a1;
        }
        break;
    case 5:
        for (n=offset; n<nsmps; n++) {
            out[n] = a0[n] != a1;
        }
        break;
    }
    return OK;
}

static int cmparray1_k(CSOUND* csound, Cmp_array1* p) {
    MYFLT *out = p->out->data;
    MYFLT *in  = p->in->data;
    MYFLT k1 = *p->k1;
    int L = p->out->sizes[0];
    int N = p->in->sizes[0];
    int i;

    switch (p->mode) {
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

static int cmp2array1_k(CSOUND* csound, Cmp2_array1* p) {
    MYFLT *out = p->out->data;
    MYFLT *in  = p->in->data;
    MYFLT a = *p->a;
    MYFLT b = *p->b;
    int L = p->out->sizes[0];
    int N = p->in->sizes[0];
    int i;
    MYFLT x;

    switch (p->mode) {
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


static int cmparray2_k(CSOUND* csound, Cmp_array2* p) {
    MYFLT *out = p->out->data;
    MYFLT *in1  = p->in1->data;
    MYFLT *in2  = p->in2->data;
    int L = p->out->sizes[0];
    int i;

    switch (p->mode) {
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

static int cmparray2_i(CSOUND* csound, Cmp_array2* p) {
    cmparray2_init(csound, p);
    return cmparray2_k(csound, p);
}


/*
 * tabslice
 *
 */

typedef struct {
    OPDS h;
    MYFLT *fnsrc, *fndst, *kstart, *kend, *kstep;
    FUNC *ftpsrc;
    FUNC *ftpdst;
} TABSLICE;

static int tabslice_init(CSOUND *csound, TABSLICE *p) {
    FUNC *ftpsrc, *ftpdst;
    ftpsrc = csound->FTFind(csound, p->fnsrc);
    if (UNLIKELY(ftpsrc == NULL)) {
        return NOTOK;
    }
    p->ftpsrc = ftpsrc;
    ftpdst = csound->FTFind(csound, p->fndst);
    if (UNLIKELY(ftpdst == NULL)) {
        return NOTOK;
    }
    p->ftpdst = ftpdst;

    return OK;
}

static int tabslice_k(CSOUND *csound, TABSLICE *p) {
    FUNC *ftpsrc = p->ftpsrc;
    FUNC *ftpdst = p->ftpdst;
    int start = (int)*p->kstart;
    int end = (int)*p->kend;
    int step = (int)*p->kstep;
    if (end < 1) {
        end = ftpsrc->flen;
    }
    int numitems = (int)ceil((end - start) / (float)step);
    if(numitems > ftpdst->flen) {
        numitems = ftpdst->flen;
    }
    MYFLT *src = ftpsrc->ftable;
    MYFLT *dst = ftpdst->ftable;

    int i, j=start;
    for(i=0; i<numitems; i++) {
        dst[i] = src[j];
        j += step;
    }
}

/*
 * reshapearray
 *
 */

typedef struct {
    OPDS h;
    ARRAYDAT *in;
    MYFLT *numrows, *numcols;
} ARRAYRESHAPE;

static int arrayreshape(CSOUND *csound, ARRAYRESHAPE *p) {
    ARRAYDAT *a = p->in;
    int dims = a->dimensions;
    int i;
    int numitems = 1;
    int numcols = (int)(*p->numcols);
    int numrows = (int)(*p->numrows);
    for(i=0; i<dims;i++) {
        numitems *= a->sizes[i];
    }
    int numitems2 = numcols*numrows;
    if (numitems != numitems2) {
        return NOTOK;
    }
    if (dims != 2) {
        csound->Free(csound, a->sizes);
        a->sizes = (int*)csound->Malloc(csound, sizeof(int)*2);
    }
    a->dimensions = 2;
    a->sizes[0] = numrows;
    a->sizes[1] = numcols;
}

/*
 * printarray
 *
 */

typedef struct {
    OPDS h;
    ARRAYDAT *in;
    MYFLT *trig;
    int lasttrig;
} ARRAYPRINT;

static int arrayprint_init(CSOUND *csound, ARRAYPRINT *p) {
    p->lasttrig = 0;
}

static int arrayprint_perf(CSOUND *csound, ARRAYPRINT *p) {
    int trig = (int)*p->trig;
    int lasttrig = p->lasttrig;
    int i, j;
    MYFLT *in;
    const int rowlength = 8;
    if(trig && (trig != lasttrig)) {
        in = p->in->data;
        switch(p->in->dimensions) {
        case 1:
            for(i=0;i<p->in->sizes[0];i++) {
                printf("%f.4f ", in[i]);
                if(i % rowlength == 0)
                    printf("\n");
            }
            break;
        case 2:
            for(i=0;i<p->in->sizes[0];i++) {
                for(j=0;j<p->in->sizes[1];j++) {
                    printf("%.4f ", *in);
                    in++;
                }
                printf("\n");
            }
            break;
        }
    }
    p->lasttrig = trig;
    return OK;
}

static int arrayprint_i(CSOUND *csound, ARRAYPRINT *p) {
    *p->trig = 1;
    arrayprint_init(csound, p);
    return arrayprint_perf(csound, p);
}

/*
 * bit | and & for array
 *
 */

typedef struct {
    OPDS h;
    ARRAYDAT *out;
    ARRAYDAT *in1, *in2;
    int numitems;
} BINOP_AAA;

static int array_binop_init(CSOUND *csound, BINOP_AAA *p) {
    int numitems = 1;
    int i;

    for(i=0; i<p->in1->dimensions;i++) {
        numitems *= p->in1->sizes[i];
    }
    arrayensure(csound, p->out, numitems);
    p->numitems = numitems;
}

static int array_or(CSOUND *csound, BINOP_AAA *p) {
    int numitems = p->numitems;
    int i;
    MYFLT *out = p->out->data;
    MYFLT *in1 = p->in1->data;
    MYFLT *in2 = p->in2->data;

    // TODO: ensure size AND shape
    for(i=0; i<numitems;i++) {
        *(out++) = (MYFLT)((int)*(in1++) | (int)*(in2++));
    }
}

static int array_and(CSOUND *csound, BINOP_AAA *p) {
    int numitems = p->numitems;
    int i;
    MYFLT *out = p->out->data;
    MYFLT *in1 = p->in1->data;
    MYFLT *in2 = p->in2->data;

    // TODO: ensure size AND shape
    for(i=0; i<numitems;i++) {
        *(out++) = (MYFLT)((int)*(in1++) & (int)*(in2++));
    }
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
    { "linlin", S(LINLINARR1), 0, 3, "k[]", "k[]kkOP", (SUBR)linlinarr1_init, (SUBR)linlinarr1_perf},
    { "linlin", S(LINLINARR1), 0, 1, "i[]", "i[]iiop", (SUBR)linlinarr1_i},
    { "linlin", S(BLENDARRAY), 0, 3, "k[]", "kk[]k[]OP", (SUBR)blendarray_init, (SUBR)blendarray_perf},
    { "linlin", S(BLENDARRAY), 0, 1, "i[]", "ii[]i[]op", (SUBR)blendarray_i},

    { "xyscale", S(XYSCALE), 0, 2, "k", "kkkkkk", NULL, (SUBR)xyscale },
    { "xyscale", S(XYSCALE), 0, 3, "k", "kkiiii", (SUBR)xyscalei_init, (SUBR)xyscalei },

    { "mtof", S(PITCHCONV), 0, 3, "k", "k", (SUBR)mtof_init, (SUBR)mtof },
    { "mtof", S(PITCHCONV), 0, 1, "i", "i", (SUBR)mtof_init },

    { "ftom", S(PITCHCONV), 0, 3, "k", "k", (SUBR)ftom_init, (SUBR)ftom },
    { "ftom", S(PITCHCONV), 0, 1, "i", "i", (SUBR)ftom_init },

    { "pchtom", S(PITCHCONV), 0, 1, "i", "i", (SUBR)pchtom },
    { "pchtom", S(PITCHCONV), 0, 2, "k", "k", NULL, (SUBR)pchtom },

    { "bpf", S(BPFX), 0, 2, "k", "kM", NULL, (SUBR)bpfx },
    { "bpf", S(BPFX), 0, 1, "i", "im", (SUBR)bpfx },

    { "ntom", S(NTOM), 0, 3, "k", "S", (SUBR)ntom, (SUBR)ntom },
    { "ntom", S(NTOM), 0, 1, "i", "S", (SUBR)ntom },

    { "mton", S(MTON), 0, 3, "S", "k", (SUBR)mton, (SUBR)mton },
    { "mton", S(MTON), 0, 1, "S", "i", (SUBR)mton },

    { "cmp", S(Cmp), 0, 5, "a", "aSa", (SUBR)cmp_init, NULL, (SUBR)cmp_aa ,},
    { "cmp", S(Cmp), 0, 5, "a", "aSk", (SUBR)cmp_init, NULL, (SUBR)cmp_ak },
    { "cmp", S(Cmp_array1), 0, 3, "k[]", "k[]Sk", (SUBR)cmparray1_init, (SUBR)cmparray1_k },
    { "cmp", S(Cmp_array2), 0, 3, "k[]", "k[]Sk[]", (SUBR)cmparray2_init, (SUBR)cmparray2_k },
    { "cmp", S(Cmp_array2), 0, 1, "i[]", "i[]Si[]", (SUBR)cmparray2_i },
    { "cmp", S(Cmp2_array1), 0, 3, "k[]", "kSk[]Sk", (SUBR)cmp2array1_init, (SUBR)cmp2array1_k },

    { "##or",  S(BINOP_AAA), 0, 3, "k[]", "k[]k[]", (SUBR)array_binop_init, (SUBR)array_or},
    { "##and", S(BINOP_AAA), 0, 3, "k[]", "k[]k[]", (SUBR)array_binop_init, (SUBR)array_and},

    { "reshapearray", S(ARRAYRESHAPE), 0, 1, "", "k[]ii", (SUBR)arrayreshape},
    { "reshapearray", S(ARRAYRESHAPE), 0, 1, "", "i[]ii", (SUBR)arrayreshape},
    { "reshapearray", S(ARRAYRESHAPE), 0, 2, "", ".[]ii", NULL, (SUBR)arrayreshape},

    { "ftslice", S(TABSLICE),  0, 3, "", "iiOOP", (SUBR)tabslice_init, (SUBR)tabslice_k},

    { "printarray", S(ARRAYPRINT), 0, 3, "", "k[]P", (SUBR)arrayprint_init, (SUBR)arrayprint_perf},
    { "printarray", S(ARRAYPRINT), 0, 1, "", "i[]", (SUBR)arrayprint_i},

};

LINKAGE

