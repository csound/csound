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

// #include <math.h>
#include <csdl.h>

#define INITERR(m) (csound->InitError(csound, Str(m)))

/*

linlin

linear to linear conversion

ky = linlin(kx, kxlow, kxhigh, kylow, kyhigh)

ky = (kx - kxlow) / (kxhigh - kxlow) * (kyhigh - kylow) + kylow

linlin(0.25, 0, 1, 1, 3) ; --> 1.5

 */

typedef struct {
    OPDS h;
    MYFLT *kout, *kx, *kx0, *kx1, *ky0, *ky1;
} LINLINK;

static int linlink(CSOUND* csound, LINLINK* p)
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

#define INTERP_L(X, X0, X1, Y0, Y1) ((X) < (X0) ? (Y0) : \
                                     (((X)-(X0))/((X1)-(X0)) * ((Y1)-(Y0)) + (Y0)))

inline MYFLT interpol_l(MYFLT x, MYFLT x0, MYFLT x1, MYFLT y0, MYFLT y1)
{
    return x < x0 ? y0 : ((x - x0) / (x1 - x0) * (y1 - y0) + y0);
}

#define INTERP_R(X, X0, X1, Y0, Y1) ((X) > (X1) ? (Y1) : \
                                     (((X) - (X0)) / ((X1) - (X0)) * ((Y1) - (Y0)) + (Y0)))

inline MYFLT interpol_r(MYFLT x, MYFLT x0, MYFLT x1, MYFLT y0, MYFLT y1)
{
    return x > x1 ? y0 : ((x - x0) / (x1 - x0) * (y1 - y0) + y0);
}

#define INTERP_M(X, X0, X1, Y0, Y1) (((X)-(X0))/((X1)-(X0)) * ((Y1)-(Y0)) + (Y0))

inline MYFLT interpol_m(MYFLT x, MYFLT x0, MYFLT x1, MYFLT y0, MYFLT y1)
{
    return (x - x0) / (x1 - x0) * (y1 - y0) + y0;
}

typedef struct {
    OPDS h;
    MYFLT *r, *x, *x0, *y0, *x1, *y1, *x2, *y2;
} BPF3;

static int bpf3(CSOUND* csound, BPF3* p) {
    MYFLT r, x = *p->x;
    if (x < *p->x1) 
        r = INTERP_L(x, *p->x0, *p->y0, *p->x1, *p->y1);
    else 
        r = INTERP_R(x, *p->x1, *p->x2, *p->y1, *p->y2);
    *p->r = r;
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *r, *x, *x0, *y0, *x1, *y1, *x2, *y2, *x3, *y3;
} BPF4;

static int bpf4(CSOUND* csound, BPF4* p) {
    MYFLT r, x = *p->x;
    if (x < (*p->x1)) 
        r = INTERP_L(x, *p->x0, *p->y0, *p->x1, *p->y1);
    else if (x < (*p->x2))
        r = INTERP_M(x, *p->x1, *p->y1, *p->x2, *p->y2);
    else 
        r = INTERP_R(x, *p->x2, *p->y2, *p->x3, *p->y3);
    *p->r = r;
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *r, *x, *x0, *y0, *x1, *y1, *x2, *y2, *x3, *y3, *x4, *y4;
} BPF5;

static int bpf5(CSOUND* csound, BPF5* p) {
    MYFLT x = *p->x;
    MYFLT r;
    if (x < (*p->x2)) {
        if (x < (*p->x1))
            r = INTERP_L(x, *p->x0, *p->x1, *p->y0, *p->y1);
        else
            r = INTERP_M(x, *p->x1, *p->x2, *p->y1, *p->y2);
    }
    else if (x < (*p->x3))
        r = INTERP_M(x, *p->x2, *p->x3, *p->y2, *p->y3);
    else 
        r = INTERP_R(x, *p->x3, *p->x4, *p->y3, *p->y4);
    *p->r = r;
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *r, *x, *x0, *y0, *x1, *y1, *x2, *y2, *x3, *y3, *x4, *y4, *x5, *y5;
} BPF6;

static int bpf6(CSOUND* csound, BPF6* p) {
    MYFLT x = *p->x;
    MYFLT r;
    if (x < (*p->x2)) {
        if (x < (*p->x1)) 
            r = INTERP_L(x, *p->x0, *p->x1, *p->y0, *p->y1);
        else 
            r = INTERP_M(x, *p->x1, *p->x2, *p->y1, *p->y2);
    }
    else if (x < (*p->x3))
        r = INTERP_M(x, *p->x2, *p->x3, *p->y2, *p->y3);
    else if (x < (*p->x4)) 
        r = INTERP_M(x, *p->x3, *p->x4, *p->y3, *p->y4);
    else 
        r = INTERP_R(x, *p->x4, *p->x5, *p->y4, *p->y5);
    *p->r = r;
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *r, *x, *x0, *y0, *x1, *y1, *x2, *y2, *x3, *y3, *x4, *y4, *x5, *y5, *x6, *y6;
} BPF7;

static int bpf7(CSOUND* csound, BPF7* p)
{
    MYFLT x = *p->x;
    MYFLT r;
    if (x < (*p->x3)) {
        if (x < (*p->x1)) 
            r = INTERP_L(x, *p->x0, *p->x1, *p->y0, *p->y1);
        else if (x < *p->x2 )
            r = INTERP_M(x, *p->x1, *p->x2, *p->y1, *p->y2);
        else
            r = INTERP_M(x, *p->x2, *p->x3, *p->y2, *p->y3);
    }
    else if (x < (*p->x4))
        r = INTERP_M(x, *p->x3, *p->x4, *p->y3, *p->y4);
    else if (x < (*p->x5)) 
        r = INTERP_M(x, *p->x4, *p->x5, *p->y4, *p->y5);
    else 
        r = INTERP_R(x, *p->x5, *p->x6, *p->y5, *p->y6);
    *p->r = r;
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *r, *x, *x0, *y0, *x1, *y1, *x2, *y2, *x3, *y3, \
                  *x4, *y4, *x5, *y5, *x6, *y6, *x7, *y7;
} BPF8;

static int bpf8(CSOUND* csound, BPF8* p)
{
    MYFLT x = *p->x;
    MYFLT r;
    if (x < *p->x4) {
        if (x < (*p->x2)) {
            if (x < (*p->x1)) 
                r = INTERP_L(x, *p->x0, *p->x1, *p->y0, *p->y1);
            else 
                r = INTERP_M(x, *p->x1, *p->x2, *p->y1, *p->y2);
        } else if (x < *p->x3)
            r = INTERP_M(x, *p->x2, *p->x3, *p->y2, *p->y3);
        else
            r = INTERP_M(x, *p->x3, *p->x4, *p->y3, *p->y4);
    } else if (x < *p->x6) {
        if (x < *p->x5) 
            r = INTERP_M(x, *p->x4, *p->x5, *p->y4, *p->y5);
        else
            r = INTERP_M(x, *p->x5, *p->x6, *p->y5, *p->y6);
    } else 
        r = INTERP_R(x, *p->x6, *p->x7, *p->y6, *p->y7);
    *p->r = r;
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

*/

typedef struct {
    OPDS h;
    MYFLT *out, *a0;
    STRINGDAT* op;
    MYFLT* a1;
    int mode;
} Cmp;

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

static int cmp_aa(CSOUND* csound, Cmp* p) {
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT* out = p->out;
    MYFLT* a0 = p->a0;
    MYFLT* a1 = p->a1;
    switch (p->mode) {
    case 0:
        for (n = 0; n < nsmps; n++) {
            out[n] = a0[n] > a1[n];
        }
        break;
    case 1:
        for (n = 0; n < nsmps; n++) {
            out[n] = a0[n] >= a1[n];
        }
        break;
    case 2:
        for (n = 0; n < nsmps; n++) {
            out[n] = a0[n] < a1[n];
        }
        break;
    case 3:
        for (n = 0; n < nsmps; n++) {
            out[n] = a0[n] <= a1[n];
        }
        break;
    case 4:
        for (n = 0; n < nsmps; n++) {
            out[n] = a0[n] == a1[n];
        }
        break;
    case 5:
        for (n = 0; n < nsmps; n++) {
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
    switch (p->mode) {
    case 0:
        for (n = 0; n < nsmps; n++) {
            out[n] = a0[n] > a1;
        }
        break;
    case 1:
        for (n = 0; n < nsmps; n++) {
            out[n] = a0[n] >= a1;
        }
        break;
    case 2:
        for (n = 0; n < nsmps; n++) {
            out[n] = a0[n] < a1;
        }
        break;
    case 3:
        for (n = 0; n < nsmps; n++) {
            out[n] = a0[n] <= a1;
        }
        break;
    case 4:
        for (n = 0; n < nsmps; n++) {
            out[n] = a0[n] == a1;
        }
        break;
    case 5:
        for (n = 0; n < nsmps; n++) {
            out[n] = a0[n] != a1;
        }
        break;
    }
    return OK;
}

/*

typedef struct {
    OPDS h;
    MYFLT *y, *x;
} PURE1;



static int nextpow2(CSOUND* csound, PURE1* p) {
    // taken from http://bits.stephan-brumme.com/roundUpToNextPowerOfTwo.html

    uint32_t x = *p->x;
    x--;
    x |= x >> 1;  // handle  2 bit numbers
    x |= x >> 2;  // handle  4 bit numbers
    x |= x >> 4;  // handle  8 bit numbers
    x |= x >> 8;  // handle 16 bit numbers
    x |= x >> 16; // handle 32 bit numbers
    x++;
    *p->y = (MYFLT)x;
    return OK;
}

*/

///////////////////////////////////////////
/*
General purpose routine to copy data from a 2D
table consisting of parallel streams of data.

A such 2D table would consist of multiple rows
where each row collects multiple streams sampled
at a regular time period. For example, a table
consisting of 3 streams, A, B, C, where each stream
has itself three channels of information

t0 -> [A0 A1 A2 B0 B1 B2 C0 C1 C2]
t1 -> [A0 A1 A2 B0 B1 B2 C0 C1 C2]
...

Each stream can consist of multiple channels, representing
different dimensions of the stream. For example, a stream can 
be a partial with 2 channels: frequency and amplitude. Multiple
partials are sampled at a regular time period and the data
is collected in a table. This routine can then copy one
row of this table (for example, all frequencies) and put the
results in a secondary 1D-table. If a fractional row is given,
linear interpolation is performed between the corresponding
values of two rows.

krow        : The (fractional) row to read from
ifnsource   : The source table to read from (a 2D table)
ifndest     : The table to write to. If should be at least (istreamend - istreambegin)
inumstreams : The number of streams in the table
ioffset     : The place where the data begins. This foresees the case where 
              the table has a header with values indicating some metadata
              of the table, like dimensions of the data, sampling period, etc.
inumchans   : Number of channels per stream
ichan       : Channel to read from each stream
istreambeg, 
istreamend  : The streams to read (stream=streambeg; stream<streamend; stream++)

Example 1
=========

Read first channel of all streams, each stream has 3 channels, there
are 4 streams in total (x indicates the columns read)

start=0, end=0, step=3

    x--x--x--x--

    inumstreams = 4    This needs to be set because a table is 1D
    inumchans = 3      3 channels per stream
    ich = 0            Which channel to read
    istreambeg = 0     Read all streams (from 0 to end)
    istreamend = 0

Example 2
=========

Read part of table sequentially

    ----xxxx--

    inumstreams = 10
    inumchans = 1
    ich = 0
    istreambeg = 4
    istreamend = istreambeg + 4
*/


typedef struct {
    OPDS h;
    MYFLT *krow, *ifnsrc, *ifndest, *inumstreams, *ioffset, *inumchans, *ichan, \
          *istreambeg, *istreamend;
    MYFLT* tabsource;
    MYFLT* tabdest;
    int tabsourcelen;
    int tabdestlen;
    int streamend;
} TABROW0;

// krow, ifnsrc, ifndest, inumstreams, ioffset=0, inumchans=1, ichan=0, istreambeg=0, istreamend=0
// k     i          i        i            0          1            0        0               0
// k     i          i        i            o          p            o        o               o
// kiiiopooo
// idx = ioffset + row*irowsize + stream*inumchans + ichan

static int tabrowcopy_init0(CSOUND* csound, TABROW0* p)
{
    FUNC* ftp;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifnsrc)) == NULL)) {
        return csound->InitError(csound, Str("tabrowcopy: incorrect table number"));
    }
    p->tabsource = ftp->ftable;
    p->tabsourcelen = ftp->flen;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifndest)) == NULL)) {
        return csound->InitError(csound, Str("tabrowcopy: incorrect table number"));
    }
    p->tabdest = ftp->ftable;
    p->tabdestlen = ftp->flen;

    int streamend = *p->istreamend;
    streamend = streamend > 0 ? streamend : *p->inumstreams;
    if (streamend <= *p->istreambeg) {
        return csound->InitError(csound, Str("tabrowcopy: streamend should be higher than streambegin."));
    }
    p->streamend = streamend;
    
    int streams_to_copy = streamend - *p->istreambeg;
    if (streams_to_copy > p->tabdestlen) {
        return csound->PerfError(csound, p->h.insdshead, Str("tabrowcopy: destination table too small"));
    }

    return OK;
}


static int tabrowcopyk0(CSOUND* csound, TABROW0* p)
{
    int i;
    MYFLT row = *p->krow;
    int row0 = (int)row;
    MYFLT delta = row0 - row;
    MYFLT x, x0, x1;
    int numch = *p->inumchans;
    int header = *p->ioffset;
    int ch = *p->ichan;
    int numstreams = *p->inumstreams;
    MYFLT* tabsource = p->tabsource;
    MYFLT* tabdest = p->tabdest;
    int rowlen = numstreams * numch;
    int idx0 = header + rowlen * row0 + ch;
    int i0 = idx0;
    int streamend = p->streamend;
    // check bounds
    i = idx0 + rowlen;
    if (UNLIKELY(i > p->tabsourcelen || i < 0)) {
        return csound->PerfError(csound, p->h.insdshead, Str("tabrowcopyk: tab off end %i"), i);
    }

    if (LIKELY(delta != 0)) {
        for (i = *p->istreambeg; i < streamend; i++) {
            x0 = tabsource[i0];
            x1 = tabsource[i0 + rowlen];
            x = x0 + (x1 - x0) * delta;
            tabdest[i] = x;
            i0 += numch;
        }
    } else {
        for (i = *p->istreambeg; i < streamend; i++) {
            x = tabsource[i0];
            tabdest[i] = x;
            i0 += numch;
        }
    }
    return OK;
}


#define S(x) sizeof(x)

/*

Input types: 
  * a, k, s, i, w, f, 
  * o (optional i-rate, default to 0), 
  * p (opt, default to 1), 
  * q (opt, 10),  
  * v(opt, 0.5), 
  * j(opt, ?1), 
  * h(opt, 127), 
  * y (multiple inputs, a-type),
  * z (multiple inputs, k-type), 
  * Z (multiple inputs, alternating k- and a-types), 
  * m (multiple inputs, i-type), 
  * M (multiple inputs, any type) 
  * n (multiple inputs, odd number of inputs, i-type).
*/

static OENTRY localops[] = {
    { "linlin", S(LINLINK), 0, 2, "k", "kkkkk", NULL, (SUBR)linlink },
    { "xyscale", S(XYSCALE), 0, 2, "k", "kkkkkk", NULL, (SUBR)xyscale },
    { "xyscale", S(XYSCALE), 0, 3, "k", "kkiiii",
        (SUBR)xyscalei_init, (SUBR)xyscalei },
    { "mtof", S(PITCHCONV), 0, 3, "k", "k", (SUBR)mtof_init, (SUBR)mtof },
    { "mtof", S(PITCHCONV), 0, 1, "i", "i", (SUBR)mtof_init },
    { "ftom", S(PITCHCONV), 0, 3, "k", "k", (SUBR)ftom_init, (SUBR)ftom },
    { "ftom", S(PITCHCONV), 0, 1, "i", "i", (SUBR)ftom_init },
    { "pchtom", S(PITCHCONV), 0, 1, "i", "i", (SUBR)pchtom },
    { "pchtom", S(PITCHCONV), 0, 2, "k", "k", NULL, (SUBR)pchtom },
    { "bpf", S(BPF3), 0, 3, "k", "kkkkkkk", (SUBR)bpf3, (SUBR)bpf3 },
    { "bpf", S(BPF4), 0, 3, "k", "kkkkkkkkk", (SUBR)bpf4, (SUBR)bpf4 },
    { "bpf", S(BPF5), 0, 3, "k", "kkkkkkkkkkk", (SUBR)bpf5, (SUBR)bpf5 },
    { "bpf", S(BPF6), 0, 3, "k", "kkkkkkkkkkkkk", (SUBR)bpf6, (SUBR)bpf6 },
    { "bpf", S(BPF7), 0, 3, "k", "kkkkkkkkkkkkkkk", (SUBR)bpf7, (SUBR)bpf7 },
    { "bpf", S(BPF8), 0, 3, "k", "kkkkkkkkkkkkkkkkk", (SUBR)bpf8, (SUBR)bpf8 },
    { "ntom", S(NTOM), 0, 3, "k", "S", (SUBR)ntom, (SUBR)ntom },
    { "ntom", S(NTOM), 0, 1, "i", "S", (SUBR)ntom },
    { "mton", S(MTON), 0, 3, "S", "k", (SUBR)mton, (SUBR)mton },
    { "mton", S(MTON), 0, 1, "S", "i", (SUBR)mton },
    { "cmp", S(Cmp), 0, 5, "a", "aSa", (SUBR)cmp_init, NULL, (SUBR)cmp_aa },
    { "cmp", S(Cmp), 0, 5, "a", "aSk", (SUBR)cmp_init, NULL, (SUBR)cmp_ak },
    // { "nextpow2", S(PURE1), 0, 1, "i", "i", (SUBR)nextpow2 },
    // { "nextpow2", S(PURE1), 0, 2, "k", "k", NULL, (SUBR)nextpow2 }
    // { "tabrowcopy", S(TABROW), 0, 3, "", "kiiiopooo", (SUBR)tabrowcopy_init, (SUBR)tabrowcopyk }
};

LINKAGE
