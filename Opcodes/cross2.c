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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"
#include "ptrigtbl.h"
#include "fhtfun.h"
#include <math.h>

#define CH_THRESH       1.19209e-7
#define CHOP(a) (a < CH_THRESH ? CH_THRESH : a)

static long plog2(long x)
{
    long mask, i;

    if (x == 0) return (-1);
    x--;

    for (mask = ~1 , i = 0; ; mask = mask+mask, i++) {
      if (x == 0) return (i);
      x = x & mask;
    }
}

/* void makepolar(MYFLT *x, long size) */
/* { */
/*     MYFLT    *i = x + 1, *j = x + size - 1; */
/*     double   t; */
/*     size = size/2 - 1; */

/*     do { */
/*       double jj = (double) *j; */
/*       t =  (double) *i; */
/*       *i++ = (MYFLT)sqrt(t * t + jj * jj); */
/*       *j-- = (MYFLT)atan2(jj, t); */
/*     } while (--size); */
/* } */

/* void makerect(MYFLT *x, long size) */
/* { */
/*     MYFLT    *i = x + 1, *j = x + size - 1; */
/*     double   t; */
/*     size = size/2 - 1; */

/*     do { */
/*       double jj = (double)*j; */
/*       t = (double) *i; */
/*       *i++ = t * (MYFLT)cos(jj); */
/*       *j-- = t * (MYFLT)sin(jj); */
/*     } while (--size); */
/* } */

static void getmag(MYFLT *x, long size)
{
    MYFLT       *i = x + 1, *j = x + size - 1, max = FL(0.0);
    long        n = size/2 - 1;

    do {
      MYFLT ii = *i;
      MYFLT jj = *j;
      ii = (MYFLT)sqrt((double)(ii * ii + jj * jj));
      if (ii > max)
        max = ii;
      *i = ii;
      i++;
      j--;
    } while (--n);

    n = size/2 + 1;
    do {
      *x++ /= max;
    } while ( --n);
}

/* void scalemag(MYFLT *x, long size) */
/* { */
/*     MYFLT    *i = x, f = 0.; */
/*     long     n = size; */

/*     do       f += *i++; */
/*     while ( --n); */

/*     f = size/f; */
/*     n = size; */
/*     do       *x++ *= f; */
/*     while ( --n); */
/* } */

/* void makelog(MYFLT *x, long size) */
/* { */
/*     size = size/2 + 1; */

/*     do       { */
/*       MYFLT y = *x; */
/*       *x++ = 20.0 * (MYFLT) log10((double) CHOP(y)); */
/*     } while ( --size); */
/* } */

/* void unlog(MYFLT *x, long size) */
/* { */
/*     size = size/2 + 1; */
/*     do       *x++ =  (MYFLT) pow(10.0, (double) *x / 20.0); */
/*     while ( --size); */
/* } */

/* void scalemod(MYFLT *y, long size) */
/* { */
/*     long i = size/2 + 1; */
/*     MYFLT max = 0, *z = y-1; */

/*     do       if (*++z > max) max = *z; */
/*     while ( --i); */
/*     i = size/2 + 1; */
/*     do       *y++ /= max; */
/*     while ( --i); */
/* } */

static void mult(MYFLT *x, MYFLT *y, long size, MYFLT w)
{
    MYFLT *j = x + size - 1;

    size = size/2 + 1;
    do {
      MYFLT z = w * *y++;
      *x++ *= z;
      *j-- *= z;
    } while (--size);
}

static void lineaprox(MYFLT *x, long size, long m)
{
    long i, c;
    MYFLT a, f;
    MYFLT rm = FL(1.0)/(MYFLT)m;

    f = x[0];
    for (i = 0 ; i < size ; i += m) {
      a = FL(0.0);
      for (c = 0 ; c < m ; c++) {
        if (a < (MYFLT)fabs((double)(x[i + c])))
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

static void do_fht(MYFLT *real, long n)
{
    MYFLT       a, b;
    long        i, j, k;

    pfht(real,n);
    for (i = 1, j = n-1, k = n/2 ; i < k ; i++, j--) {
      a = real[i];
      b = real[j];
      real[j] = (a-b)*FL(0.5);
      real[i] = (a+b)*FL(0.5);
    }
}

static void do_ifht(MYFLT *real, long n)
{
    MYFLT       a, b;
    long        i, j, k;

    for (i = 1, j = n-1, k = n/2 ; i < k ; i++, j--) {
      a = real[i];
      b = real[j];
      real[j] = (a-b);
      real[i] = (a+b);
    }
    for (i = 0 ; i < n ; i++) real[i] /= n;
    pfht(real,n);
}

static void pfht(MYFLT *fz, long n)
{
    long        i, k, k1, k2, k3, k4, kx;
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
        g3 = SQRT2 * c4;
        g2 = SQRT2 * c3;
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
      k1 = 1L << k;
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
        g3 = SQRT2 * gi[k3];
        g2 = SQRT2 * gi[k2];

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

static int Xsynthset(CSOUND *csound, CON *p)
{
    long flen, bufsize;
    MYFLT       *b;
    FUNC        *ftp;
    MYFLT       ovlp = *p->ovlp;

    flen = (long)*p->len;
    p->m = plog2(flen);
    flen = 1L << p->m;

    if (ovlp < FL(2.0)) ovlp = FL(2.0);
    else if (ovlp > (MYFLT)(flen+flen)) ovlp = (MYFLT)(flen+flen);
    ovlp = (MYFLT)(1 << (int)plog2((long)ovlp));

    bufsize = 10 * flen * sizeof(MYFLT);

    if (p->mem.auxp == NULL || bufsize > p->mem.size)
      csound->AuxAlloc(csound, bufsize, &p->mem);

    b = (MYFLT*)p->mem.auxp;
    memset(p->mem.auxp, 0, (size_t)bufsize); /* Replaces loop */

    p->buffer_in1 = b;     b += 2 * flen;
    p->buffer_in2 = b;     b += 2 * flen;
    p->buffer_out = b;     b += 2 * flen;
    p->in1 = b;            b += 2 * flen;
    p->in2 = b;            b += 2 * flen;

    if ((ftp = csound->FTFind(csound, p->iwin)) != NULL)
      p->win = ftp;
    else return NOTOK;

    p->count = 0;
    p->s_ovlp = ovlp;
    return OK;
}

static int Xsynth(CSOUND *csound, CON *p)
{
    MYFLT               *s, *f, *out, *buf1, *buf2, *outbuf, rfn;
    long                n, size, samps, div;
    long                m;

    s = p->as;
    f = p->af;
    out = p->out;

    outbuf = p->buffer_out;
    buf1 = p->buffer_in1;
    buf2 = p->buffer_in2;

    size = (long)*p->len;
    div = size / (long)p->s_ovlp;
    rfn = (MYFLT)p->win->flen / (MYFLT)size; /* Moved here for efficiency */

    n = p->count;
    m = n % div;
    for (samps = 0; samps < csound->ksmps; samps++) {
      *(buf1 + n) = *s++;
      *(buf2 + n) = *f++;

      *out++ = *(outbuf + n);
      n++; m++; if (m==div) m = 0;

      if (m == 0)       {
        long            i, mask, index;
        MYFLT           window;
        MYFLT           *x, *y, *win;

        mask = size - 1;
        win = p->win->ftable;
        x = p->in1;
        y = p->in2;

        for (i = 0 ; i < size ; i++)    {

          window = *(win + (long)( i * rfn));
          index = (i + n) & mask;

          x[i] = *(buf1 + index) * window;
          y[i] = *(buf2 + index) * window;
        }

        for (; i < 2 * size ; i++)      {
          x[i] = FL(0.0);
          y[i] = FL(0.0);
        }

        if (*p->bias != FL(0.0))        {
          int nsize = (int)(size+size);

          do_fht( x, nsize);
          do_fht( y, nsize);
          getmag( y, nsize);

          lineaprox( y, nsize, 16);
          mult( x, y, nsize, *p->bias);

          do_ifht( x, nsize);

        }

        for (i =  n + size - div ; i < n + size ; i++)
          *(outbuf + (i&mask)) = FL(0.0);

        window = FL(5.0) / p->s_ovlp;

        for (i = 0 ; i < size ; i++)
          *(outbuf + ((i+n)&mask)) += x[i] * window;

      }
      if (n == size) n = 0;     /* Moved to here from inside loop */
    }

    p->count = n;
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
{ "cross2",  S(CON), 5, "a", "aaiiik",(SUBR)Xsynthset, NULL, (SUBR)Xsynth}
};

LINKAGE

