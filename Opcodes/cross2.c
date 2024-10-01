/*
    cross2.c:

    Copyright (C) 1997 Paris Smaragdis, John ffitch

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

#include "stdopcod.h"
#include "ptrigtbl.h"
#include "fhtfun.h"
#include "interlocks.h"
#include <math.h>

#define CH_THRESH       1.19209e-7
#define CHOP(a) (a < CH_THRESH ? CH_THRESH : a)

static int32 plog2(int32 x)
{
    int32 mask, i;

    if (x == 0) return (-1);
    x--;

    for (mask = ~1 , i = 0; ; mask = mask+mask, i++) {
      if (x == 0) return (i);
      x = x & mask;
    }
}

static void getmag(MYFLT *x, int32 size)
{
    MYFLT       *i = x + 1, *j = x + size - 1, max = FL(0.0);
    int32        n = size/2 - 1;

    do {
      MYFLT ii = *i;
      MYFLT jj = *j;
      ii = HYPOT(ii,jj);
      if (ii > max)
        max = ii;
      *i = ii;
      i++;
      j--;
    } while (--n);

    if (LIKELY(max!=FL(0.0))) {
      int32_t NN = size/2 + 1;
      for (n=0; n<NN; n++) {
        x[n] /= max;
      }
    }
}

static void mult(MYFLT *x, MYFLT *y, int32 size, MYFLT w)
{
    MYFLT *j = x + size - 1;

    size = size/2 + 1;
    do {
      MYFLT z = w * *y++;
      *x++ *= z;
      *j-- *= z;
    } while (--size);
}

static void lineaprox(MYFLT *x, int32 size, int32 m)
{
    int32 i, c;
    MYFLT a, f;
    MYFLT rm = FL(1.0)/(MYFLT)m;

    f = x[0];
    for (i = 0 ; i < size ; i += m) {
      a = FL(0.0);
      for (c = 0 ; c < m ; c++) {
        if (a < FABS(x[i + c]))
          a = x[i + c];
      }
      x[i] = a;
    }
    a = (x[0] + f) * rm;
    for (c = 0 ; c < m ; c++)
      x[c] = a * c + f;
    for (i = m ; i < size ; i += m)     {
      a = ( x[i] - x[i - 1]) * rm;
      for ( c = 0 ; c < m ; c++)
        x[i + c] = a * c + x[i - 1];
    }
}

static void do_fht(MYFLT *real, int32 n)
{
    MYFLT       a, b;
    int32        i, j, k;

    pfht(real,n);
    for (i = 1, j = n-1, k = n/2 ; i < k ; i++, j--) {
      a = real[i];
      b = real[j];
      real[j] = (a-b)*FL(0.5);
      real[i] = (a+b)*FL(0.5);
    }
}

static void do_ifht(MYFLT *real, int32 n)
{
    MYFLT       a, b;
    int32        i, j, k;

    for (i = 1, j = n-1, k = n/2 ; i < k ; i++, j--) {
      a = real[i];
      b = real[j];
      real[j] = (a-b);
      real[i] = (a+b);
    }
    for (i = 0 ; i < n ; i++) real[i] /= n;
    pfht(real,n);
}

static void pfht(MYFLT *fz, int32 n)
{
    int32_t        i, k, k1, k2, k3, k4, kx;
    MYFLT       *fi, *fn, *gi;
    TRIG_VARS;

    k1 = 1;
    k2 = 0;
    do {
      MYFLT a;

      for (k = n >> 1 ; !( (k2 ^= k) & k) ; k >>= 1);
      if (k1 > k2) {
        a = fz[k1];
        fz[k1] = fz[k2];
        fz[k2] = a;
      }
      k1++;
    } while (k1 < n);

    k = 0;
    i = 1;
    do {
      k++;
    } while ( (i << k) < n);

    k  &= 1;
    if (k == 0) {
      fi = fz;
      fn = fz + n;
      do {
        MYFLT f0, f1, f2, f3;
        f1 = fi[0] - fi[1];
        f0 = fi[0] + fi[1];
        f3 = fi[2] - fi[3];
        f2 = fi[2] + fi[3];
        fi[2] = f0 - f2;
        fi[0] = f0 + f2;
        fi[3] = f1 - f3;
        fi[1] = f1 + f3;
        fi += 4;
      } while (fi < fn);
    }
    else {
      fi = fz;
      fn = fz + n;
      gi = fi + 1;
      do {
        MYFLT s1, c1, s2, c2, s3, c3, s4, c4, g0, f0, f1, g1, f2, g2, f3, g3;

        c1 = fi[0] - gi[0];
        s1 = fi[0] + gi[0];
        c2 = fi[2] - gi[2];
        s2 = fi[2] + gi[2];
        c3 = fi[4] - gi[4];
        s3 = fi[4] + gi[4];
        c4 = fi[6] - gi[6];
        s4 = fi[6] + gi[6];
        f1 = s1 - s2;
        f0 = s1 + s2;
        g1 = c1 - c2;
        g0 = c1 + c2;
        f3 = s3 - s4;
        f2 = s3 + s4;
        g3 = ROOT2 * c4;
        g2 = ROOT2 * c3;
        fi[4] = f0 - f2;
        fi[0] = f0 + f2;
        fi[6] = f1 - f3;
        fi[2] = f1 + f3;
        gi[4] = g0 - g2;
        gi[0] = g0 + g2;
        gi[6] = g1 - g3;
        gi[2] = g1 + g3;

        fi += 8;
        gi += 8;
      } while (fi < fn);
    }
    if (n < 16)
      return;
    do {
      MYFLT s1, c1;

      k += 2;
      k1 = 1 << k;
      k2 = k1 << 1;
      k4 = k2 << 1;
      k3 = k2 + k1;
      kx = k1 >> 1;
      fi = fz;
      gi = fi + kx;
      fn = fz + n;
      do {
        MYFLT g0, f0, f1, g1, f2, g2, f3, g3;

        f1 = fi[0] - fi[k1];
        f0 = fi[0] + fi[k1];
        f3 = fi[k2] - fi[k3];
        f2 = fi[k2] + fi[k3];

        fi[k2] = f0 - f2;
        fi[0] = f0 + f2;
        fi[k3] = f1 - f3;
        fi[k1] = f1 + f3;

        g1 = gi[0] - gi[k1];
        g0 = gi[0] + gi[k1];
        g3 = ROOT2 * gi[k3];
        g2 = ROOT2 * gi[k2];

        gi[k2] = g0 - g2;
        gi[0] = g0 + g2;
        gi[k3] = g1 - g3;
        gi[k1] = g1 + g3;

        gi += k4;
        fi += k4;
      } while (fi < fn);

      TRIG_INIT(k, c1, s1);

      i = 1;
      do {
        MYFLT c2, s2;
        TRIG_NEXT(k, c1, s1);

        c2 = c1 * c1 - s1 * s1;
        s2 = 2 * c1 * s1;
        fn = fz + n;
        fi = fz + i;
        gi = fz + k1 - i;

        do {
          MYFLT a, b, g0, f0, f1, g1, f2, g2, f3, g3;

          b = s2 * fi[k1] - c2 * gi[k1];
          a = c2 * fi[k1] + s2 * gi[k1];
          f1 = fi[0] - a;
          f0 = fi[0] + a;
          g1 = gi[0] - b;
          g0 = gi[0] + b;

          b = s2 * fi[k3] - c2 * gi[k3];
          a = c2 * fi[k3] + s2 * gi[k3];
          f3 = fi[k2] - a;
          f2 = fi[k2] + a;
          g3 = gi[k2] - b;
          g2 = gi[k2] + b;

          b = s1 * f2 - c1 * g3;
          a = c1 * f2 + s1 * g3;
          fi[k2] = f0 - a;
          fi[0] = f0 + a;
          gi[k3] = g1 - b;
          gi[k1] = g1 + b;

          b = c1 * g2 - s1 * f3;
          a = s1 * g2 + c1 * f3;
          gi[k2] = g0 - a;
          gi[0] = g0 + a;
          fi[k3] = f1 - b;
          fi[k1] = f1 + b;

          gi += k4;
          fi += k4;
        } while (fi < fn);

        i++;
      } while (i < kx);
    } while (k4 < n);
}

static int32_t Xsynthset(CSOUND *csound, CON *p)
{
    uint32_t    flen, bufsize;
    MYFLT       *b;
    FUNC        *ftp;
    MYFLT       ovlp = *p->ovlp;

    flen = (int32)*p->len;
    if (UNLIKELY(flen<1))
      return csound->InitError(csound, "%s", Str("cross2: length must be at least 1"));
    p->m = plog2(flen);
    flen = 1 << p->m;

    if (ovlp < FL(2.0)) ovlp = FL(2.0);
    else if (ovlp > (MYFLT)(flen+flen)) ovlp = (MYFLT)(flen+flen);
    ovlp = (MYFLT)(1 << (int32_t)plog2((int32)ovlp));

    bufsize = 10 * flen * sizeof(MYFLT);

    if (p->mem.auxp == NULL || bufsize > p->mem.size)
      csound->AuxAlloc(csound, bufsize, &p->mem);
    else
      memset(p->mem.auxp, 0, (size_t)bufsize); /* Replaces loop */

    b = (MYFLT*)p->mem.auxp;
    p->buffer_in1 = b;     b += 2 * flen;
    p->buffer_in2 = b;     b += 2 * flen;
    p->buffer_out = b;     b += 2 * flen;
    p->in1 = b;            b += 2 * flen;
    p->in2 = b;            //b += 2 * flen;

    if ((ftp = csound->FTFind(csound, p->iwin)) != NULL)
      p->win = ftp;
    else return NOTOK;

    p->count = 0;
    p->s_ovlp = ovlp;
    return OK;
}

static int32_t Xsynth(CSOUND *csound, CON *p)
{
     IGN(csound);
    MYFLT               *s, *f, *out, *buf1, *buf2, *outbuf, rfn;
    int32                size, div;
    int32                n, m;
    uint32_t             offset = p->h.insdshead->ksmps_offset;
    uint32_t             early  = p->h.insdshead->ksmps_no_end;
    uint32_t             nn, nsmps = CS_KSMPS;

    s = p->as;
    f = p->af;
    out = p->out;

    outbuf = p->buffer_out;
    buf1 = p->buffer_in1;
    buf2 = p->buffer_in2;

    size = (int32)*p->len;
    div = size / (int32)p->s_ovlp;
    rfn = (MYFLT)p->win->flen / (MYFLT)size; /* Moved here for efficiency */

    n = p->count;
    m = n % div;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (nn = offset; nn < nsmps; nn++) {
      buf1[n] = s[nn];
      buf2[n] = f[nn];

      out[nn] = outbuf[n];
      n++; m++;
      if (n == size) n = 0;     /* Moved to here from inside loop */
      if (m == div) {           /* wrap */
        int32           i, mask, index;
        MYFLT           window;
        MYFLT           *x, *y, *win;
        m = 0;
        mask = size - 1;
        win = p->win->ftable;
        x = p->in1;
        y = p->in2;

        for (i = 0 ; i < size ; i++) {

          window = win[(int32)(i * rfn)];
          index = (i + n) & mask;

          x[i] = buf1[index] * window;
          y[i] = buf2[index] * window;
        }

        memset(&x[size], 0, sizeof(MYFLT)*size);
        memset(&y[size], 0, sizeof(MYFLT)*size);
        /* for (; i < 2 * size ; i++) { */
        /*   x[i] = FL(0.0); */
        /*   y[i] = FL(0.0); */
        /* } */

        if (*p->bias != FL(0.0)) {
          int32_t nsize = (int32_t)(size+size);

          do_fht( x, nsize);
          do_fht( y, nsize);
          getmag( y, nsize);

          lineaprox( y, nsize, 16);
          mult( x, y, nsize, *p->bias);

          do_ifht( x, nsize);

        }

        for (i =  n + size - div ; i < n + size ; i++)
          outbuf[i&mask] = FL(0.0);

        window = FL(5.0) / p->s_ovlp;

        for (i = 0 ; i < size ; i++)
          outbuf[(i+n)&mask] += x[i] * window;

      }
      if (n == size) n = 0;     /* Moved to here from inside loop */
    }

    p->count = n;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "cross2",  S(CON), TR,  "a", "aaiiik",(SUBR)Xsynthset, (SUBR)Xsynth}
};

int32_t cross2_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t) (sizeof(localops) / sizeof(OENTRY)));
}

