/*
    fft.c:

    Copyright (C) 1990, 1995 Dan Ellis, Greg Sullivan

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

/***********************************************************************\
*       fft.c                                                           *
*   Fast Fourier Transform C library -                                  *
*   Based on RECrandell's. With all declarations                        *
*   dpwe 22jan90                                                        *
*   08apr90 With experimental FFT2torl - converse of FFT2real           *
*                                                                       *
*   G. Sullivan                                                         *
*   Created 'packed' versions, which accept data that is packed, and    *
*   which do not produce the (redundant) conjugate half of the result.  *
*   Apr95                                                               *
\***********************************************************************/

/*  Routines include:
    FFT2raw : Radix-2 FFT, in-place, with yet-scrambled result.
    FFT2rawpacked : Radix-2 FFT, in-place, with yet-scrambled result.
                    Assumes packed data, so is faster than the above.
    FFT2    : Radix-2 FFT, in-place and in-order.
    FFTreal : Radix-2 FFT, with real data assumed,
              in-place and in-order.
    FFTrealpacked : Radix-2 FFT, with real data assumed, and accepts packed
                    data. (this routine is the fastest of the lot).
    FFT2torl: Converse of FFTreal
    FFT2torlpacked: Converse of FFTrealpacked
    FFTarb  : Arbitrary-radix FFT, in-order but not in-place.
    FFT2dimensional : Image FFT, for real data, easily modified for other
                      purposes.

    To call an FFT, one must first assign the complex exponential factors:
    A call such as
        e = AssignBasis(NULL, size)
    will set up the complex *e to be the array of cos, sin pairs corresponding
    to total number of complex data = size.   This call allocates the
    (cos, sin) array memory for you.  If you already have such memory
    allocated, pass the allocated pointer instead of NULL.
 */

#include <stdio.h>
#include "cs.h"   /* for mmalloc, mcalloc prototypes, DO NOT USE MALLOC */
#include <math.h>
#include "fft.h"
#include "prototyp.h"

static LNODE   *lroot = NULL;  /* root of look up table list */

void fftRESET(void)
{
    LNODE *x;
    while (lroot!=NULL) {
      x = lroot->next;
      mfree(lroot);
      lroot = x;
    }
}

void putcomplexdata(complex x[], long n)
{
    long j;
    for (j=0;j<n;j++) printf("%f %f\n",x[j].re, x[j].im);
}

void ShowCpx(complex x[], long siz, char *s)
{
    long i;

    printf("%s \n",s);
    for (i=0; i<siz; ++i) {
        printf(" %6.2f",x[i].re);
    }
    printf("\n");
    for (i=0; i<siz; ++i)
        printf(" %6.2f",x[i].im);
    printf("\n");
}

int PureReal(complex x[], long n) /* Query whether the data is pure real. */
{
    long m;
    for (m=0;m<n;m++) {
      if (x[m].im!=FL(0.0)) return(0);
    }
    return(1);
}

int IsPowerOfTwo(long n)        /* Query whether n is a pure power of 2 */
{
    while (n>1L) {
      if (n%2L) break;
      n >>= 1L;
    }
    return(n==1L);
}

complex *FindTable(long n)      /* search our list of existing LUT's */
                                /* set up globals expn and lutSize too */
{
    LNODE *plnode;
    complex *ex = NULL;

/*    lutSize = 0; expn = NULL; */ /* globals */
    plnode = lroot;
    for (plnode = lroot; plnode != NULL && plnode->size != n;
         plnode = plnode->next);
    if (plnode != NULL)
      ex = plnode->table;
    /*  { expn = plnode->table; lutSize = plnode->size; } */
    return(ex);
}

complex *AssignBasis(complex ex[], long n)
{
    long j;
    MYFLT a = FL(0.0), inc = FL(2.0)*PI_F/(MYFLT)n;
    LNODE    *plnode;
    complex  *expn;

    if ((expn=FindTable(n))!=0)
      return(expn);
    if (ex != NULL) expn = ex;
    else {
      expn = (complex *) mmalloc((long)n*sizeof(complex));
      if (expn==NULL) return(NULL);
    }
    for (j=0;j<n;j++) {
      a = j*inc;
      expn[j].re = (MYFLT)cos((double)a);
      expn[j].im = -(MYFLT)sin((double)a);
    }
    plnode = lroot;
    lroot = (LNODE *) mmalloc((long)sizeof(LNODE));
    lroot->next = plnode;
    lroot->size = n;
    lroot->table = expn;
    /*    lutSize = n; */              /* set the global */
    return(expn);
}

void reverseDig(complex x[], long n, int skip)
{
    long i,j,k, jj, ii;
    complex tmp;
    for (i=0,j=0;i<n-1;i++) {
      if (i<j) {
        jj = j*skip; ii = i*skip;
        tmp = x[jj];
        x[jj]=x[ii];
        x[ii]=tmp;
      }
      k = n/2;
      while (k<=j) {
        j -= k;
        k>>=1;
      }
      j += k;
    }
}

void reverseDigpacked(complex x[], long n)
/* data is assumed packed (skip == 1) */
{
    long i,j,k;
    complex tmp;
    for (i=0,j=0; i<n-1; i++) {
      if (i<j) {
        tmp = x[j];
        x[j]=x[i];
        x[i]=tmp;
      }
      k = n/2;
      while (k<=j) {
        j -= k;
        k>>=1;
      }
      j += k;
    }
}


void FFT2dimensional(complex *x, long w, long h, complex *ex)
/* Perform 2D FFT on image of width w, height h (both powers of 2).
   IMAGE IS ASSUMED PURE-REAL.  If not, the FFT2real call should be just FFT2.
 */
{
    long j;
    for (j=0;j<h;j++) FFT2real(x+j*w, w, 1, ex);
    for (j=0;j<w;j++) FFT2(x+j, h, (int) w, ex);
}

void FFT2torl(          /* Performs FFT on data assumed complex conjugate */
    complex x[],        /* to give purely real result */
    long n,
    int  skip,
    MYFLT   scale,              /* (frigged for ugens7.c) */
    complex ex[] )              /* pass in lookup table */
{
    long half = n>>1, quarter = half>>1, m, mm;

    if (!quarter) return;
    Reals(x,n,skip,-1,ex);
    ConjScale(x, half+1, FL(2.0) * scale);
    FFT2raw(x, half, 2, skip, ex);
    reverseDig(x, half, skip);
    for (mm= half-1;mm>=0;mm--) {
      m = mm*skip;
      x[m<<1].re = x[m].re;
      x[(m<<1)+skip].re = -x[m].im;  /* need to conjugate result for true ifft */
      x[m<<1].im = x[(m<<1)+skip].im = FL(0.0);
    }
}

void FFT2torlpacked(complex x[], long n, MYFLT scale, complex ex[])
/* Performs FFT on data assumed complex conjugate to give purely
 * real result. Also assumes data is packed (skip == 1).
 * Expects data in a complex array size n/2 + 1.
 */
           /* scale (frigged for ugens7.c) */
           /* ex pass in lookup table */
{
    long half = n>>1, quarter = half>>1, m;
    if (!quarter) return;
    Realspacked(x,n,-1,ex);
    ConjScale(x, half+1, FL(2.0) * scale);
    FFT2rawpacked(x, half, 2, ex);
    reverseDigpacked(x, half);
    for (m= half-1; m>=0; m--)
      x[m].im *= -1;      /* need to conjugate result for true ifft */
}



void ConjScale(complex x[], long len, MYFLT scale)
        /* Conjugate and scale complex data (e.g. prior to IFFT by FFT) */
{
    MYFLT miscale = -scale;

    while (len--) {
      x->re *= scale;
      x->im *= miscale;
      x++;
    }
}

void FFT2real(complex x[], long n, int skip, complex ex[])
        /* Perform real FFT, data arrange as {re, 0, re, 0, re, 0...};
 * leaving full complex result in x.
 */
                                /* ex[] is lookup table */
{
    long half = n>>1, quarter = half>>1, m, mm;

    if (!quarter) return;
    for (mm=0;mm<half;mm++) {
      m = mm*skip;
      x[m].re = x[m<<1].re;
      x[m].im = x[(m<<1)+skip].re;
    }
    FFT2raw(x, half, 2, skip, ex);
    reverseDig(x, half, skip);
    Reals(x,n,skip,1,ex);
}


void FFT2realpacked(complex x[], long n, complex ex[])
/* Perform real FFT on packed input data. re,re,re....re(n)
 * leaving complex result in x. Assumes data is packed (skip == 1)
 * Array size must be n+2, to accomodate the nyquist rate frequency sample
 */
{
    long half = n>>1, quarter = half>>1;

    if (!quarter) return;
    FFT2rawpacked(x, half, 2, ex);
    reverseDigpacked(x, half);
    Realspacked(x,n,1,ex);
}



void Reals(                     /* sign is 1 for FFT2real, -1 for torl */
    complex x[],
    long n,
    int  skip,
    int sign,
    complex ex[])               /* lookup table passed in */
{
    int half = (int)(n>>1);
    int quarter = half>>1;
    int m, mm;
    MYFLT tmp;
    complex a,b,s,t;

    half *= skip; n *= skip;
    if (sign ==1)    x[half]= x[0];      /* only for Yc to Xr */
    for (mm=0; mm<=quarter; mm++) {
      m = mm*skip;
      s.re = (FL(1.0)+ex[mm].im)*FL(0.5);
      s.im = (-(MYFLT)sign)*ex[mm].re*FL(0.5);
      t.re = FL(1.0)-s.re;
      t.im = -s.im;
      a = x[m];
      b = x[half-m];
      b.im = -b.im;

      tmp = a.re;
      a.re = (a.re*s.re - a.im*s.im);
      a.im = (tmp*s.im + a.im*s.re);

      tmp = b.re;
      b.re = (b.re*t.re - b.im*t.im);
      b.im = (tmp*t.im + b.im*t.re);

      b.re += a.re;
      b.im += a.im;

      a = x[m];
      a.im = -a.im;
      x[m] = b;
      if (m) {
        b.im = -b.im;
        x[n-m] = b;
      }
      b = x[half-m];

      tmp = a.re;
      a.re = (a.re*t.re + a.im*t.im);
      a.im = (-tmp*t.im + a.im*t.re);

      tmp = b.re;
      b.re = (b.re*s.re + b.im*s.im);
      b.im = (-tmp*s.im + b.im*s.re);

      b.re += a.re;
      b.im += a.im;

      x[half-m] = b;
      if (m) {
        b.im = -b.im;
        x[half+m] = b;
      }
    }
}

void Realspacked(               /* sign is 1 for FFT2real, -1 for torl */
                                /* assumes packed data (skip == 1) */
    complex x[],
    long n,
    int  sign,
    complex ex[])       /* lookup table passed in */
{
    int half = (int)(n>>1);
    int quarter = half>>1;
    int m;
    MYFLT tmp;
    complex a,b,s,t;

    if (sign ==1)
      x[half]= x[0];  /* only for Yc to Xr */
    for (m=0; m<=quarter; m++) {
      s.re = (FL(1.0)+ex[m].im)*FL(0.5);
      s.im = (-(MYFLT)sign)*ex[m].re*FL(0.5);
      t.re = FL(1.0) - s.re;
      t.im = -s.im;
      a = x[m];
      b = x[half-m];
      b.im = -b.im;

      tmp = a.re;
      a.re = (a.re*s.re - a.im*s.im);
      a.im = (tmp*s.im + a.im*s.re);

      tmp = b.re;
      b.re = (b.re*t.re - b.im*t.im);
      b.im = (tmp*t.im + b.im*t.re);

      b.re += a.re;
      b.im += a.im;

      a = x[m];
      a.im = -a.im;
      x[m] = b;
      if (m)
        b.im = -b.im;
      b = x[half-m];

      tmp = a.re;
      a.re = (a.re*t.re + a.im*t.im);
      a.im = (-tmp*t.im + a.im*t.re);

      tmp = b.re;
      b.re = (b.re*s.re + b.im*s.im);
      b.im = (-tmp*s.im + b.im*s.re);

      b.re += a.re;
      b.im += a.im;

      x[half-m] = b;
      if (m)
        b.im = -b.im;
    }
}




void FFT2(
/* Perform FFT for n = a power of 2.
   The relevant data are the complex numbers x[0], x[skip], x[2*skip], ...
 */
    complex x[],
    long n,
    int  skip,
    complex ex[])
{
    FFT2raw(x, n, 1, skip,ex);
    reverseDig(x, n, skip);
}

void FFT2raw(
    complex x[],                /* Data is x */
    long n,                     /* data size is n */
    int  dilate,                /* dilate means: library global expn is the
                                 * (cos, -j sin) array, EXCEPT for effective
                                 * data size n/dilate */
    int  skip,                  /* skip is the offset of each successive data
                                 * term, as in X_761,"FFT2" above */
    complex ex[])               /* lookup table */
{
    long j, m=1, p, q, i, k, n2=n, n1 ;
    MYFLT c, s, rtmp, itmp;

    while (m<n) {
      n1 = n2;
      n2 >>= 1;
      for (j=0, q=0; j<n2; j++) {
        c = ex[q].re;
        s = ex[q].im;
        q += m*dilate;
        for (k=j;k<n;k+=n1) {
          p = (k + n2)*skip;
          i = k*skip;
          rtmp = x[i].re - x[p].re;
          x[i].re += x[p].re;
          itmp = x[i].im - x[p].im;
          x[i].im += x[p].im;
          x[p].re = (c*rtmp - s*itmp);
          x[p].im = (c*itmp + s*rtmp);
        }
      }
      m <<= 1;
    }
}


void FFT2rawpacked(complex x[], long n, int dilate, complex ex[])
/* Data is x,
   data size is n,
   dilate means: library global expn is the (cos, -j sin) array, EXCEPT for
        effective data size n/dilate,
   Data is assumed packed (skip == 1)
 */
{
    long j, m=1, p, q, k, n2=n, n1 ;
    MYFLT c, s, rtmp, ktmp;

    while (m<n) {
      n1 = n2;
      n2 >>= 1;
      for (j=0, q=0; j<n2; j++) {
        c = ex[q].re;
        s = ex[q].im;
        q += m*dilate;
        for (k=j;k<n;k+=n1) {
          p = (k + n2);
          rtmp = x[k].re - x[p].re;
          x[k].re += x[p].re;
          ktmp = x[k].im - x[p].im;
          x[k].im += x[p].im;
          x[p].re = (c*rtmp - s*ktmp);
          x[p].im = (c*ktmp + s*rtmp);
        }
      }
      m <<= 1;
    }
}



void FFTarb(complex data[], complex result[], long n, complex ex[])
/* Compute FFT for arbitrary radix, with limitation  n <= 1024.*/
/* ex is lookup table */
{
    long    p, p0, i, j, a, b, c, v, k, m;
    MYFLT   x,y;
    long    sum, car, q, arg;
    long    aa[10], pr[10], cc[10], jj[1024];
    complex t[1024];

    /* Next, get the prime factors of n */
    m = n;
    v = 0;
    j = 2;
    while (m!=1) {
      while (m%j==0) {
        m /= j;
        ++v;
        pr[v] = j;
      }
      j += 2;
      if (j==4) j=3;
    }
    /* pr[] is now the array of prime factors of n, with v relevant elements */

    /* Next, re-order the array in reverse-complement binary */
    cc[0] = 1;
    for (i=1; i<v; i++) cc[i] = cc[i - 1] * pr[i];
    for (m=1; m<10; m++) aa[m] = 0;
    jj[0] = 0;
    /* jj array will be the input order when xr, xi are read */
    for (i=1; i<n; i++) {
      j = v;
      car = 1;
      while (car) {
        aa[j] += car;
        car = aa[j]/pr[j];
        aa[j] %=  pr[j];
        --j;
      }
      sum = 0;
      for (q=0; q<v; q++) sum += aa[q + 1] * cc[q];
      jj[sum] = i;
    }

    /* Next, read in the data */
    for (i=0;i<n;i++) {
      result[jj[i]].re = data[i].re;
      result[jj[i]].im = data[i].im;
    }
    c = v;
    a = 1;
    b = 1;
    while (c) {
      a *= pr[c];
      for (k=0;k<n;k++) {
        arg = a * (k/a) + k%b;
        p0 = (k * n) / a;
        p = 0;
        x = FL(0.0);
        y = FL(0.0);
        for (q=0;q<pr[c];q++) {
          x += (result[arg].re * ex[p].re - result[arg].im * ex[p].im);
          y += (result[arg].re * ex[p].im + result[arg].im * ex[p].re);
          p = (p + p0) % n;
          arg += b;
        }
        t[k].re = x;
        t[k].im = y;
      }
      for (k=0;k<n;k++) result[k] = t[k];
      --c;
      b = a;
    }
}

void DFT(complex data[], complex result[], long n, complex ex[])
/* Perform direct Discrete Fourier Transform. */
{
    long j,k,m;
    MYFLT s, c;

    for (j=0;j<n;j++) {
      result[j].re = FL(0.0); result[j].im = FL(0.0);
      for (k=0;k<n;k++) {
        m = (j*k)%n;
        c = ex[m].re; s = ex[m].im;
        result[j].re += (data[k].re * c - data[k].im * s);
        result[j].im += (data[k].re * s + data[k].im * c);
      }
    }
}

/* Point for point complex multiply. Leaves result in array b. Does
   NOT conjugate */
void cxmult(complex *a, complex *b, long n)
{
    complex c;
    for (; n > 0; --n) {
      c.re = (a->re * b->re) - (a->im * b->im);
      c.im = (a->im * b->re) + (a->re * b->im);
      b->re = c.re;
      b->im = c.im;
      a++;b++;
    }
}

#ifdef MAIN

static TestArith(void)
/* Lets just see what happens if you do these things ..*/
{
    MYFLT   a,b,c,d;

    a = -10000;
    b = (1/2);
    c = (a*b);
    printf(Str("Result .. and what it should have been\n"));
    printf("%f %f\n",c,a/2);
}

#define tDLEN 16

static TestReals(void)
/* test out our 'reals' function */
{
    complex data[tDLEN],datb[tDLEN];
    complex *e;
    int i,len;

    len = tDLEN /2;
/*     printf("Hello dan.\n"); */
    srand((int)time(NULL));
    e = AssignBasis(NULL,tDLEN);
    for (i = 0; i<tDLEN; i++) {
      datb[i].re = data[i].re = -7+(15&rand()); /* (i-1)? 0 : 1; */ /* i+1; */
      datb[i].im = data[i].im = 0; /* (tDLEN - i); */
    }
    ShowCpx(data, tDLEN, Str("Start data"));
    FFT2real(data, tDLEN, 1, e);
    ShowCpx(data, tDLEN, Str("Transform"));
    data[tDLEN/2].re = 0; data[tDLEN/2].im = 0;
    for (i=tDLEN/2+1; i<tDLEN; ++i) {
      data[i].re = FL(0.0); data[i].im = FL(0.0);
    }
    /* conjugate 2nd half of data should make no difference to FFT2torl */
    FFT2torl(data, tDLEN, 1, 1.0/(MYFLT)tDLEN,e); /* */
    ShowCpx(data, tDLEN, Str("Final result"));
}


int main(void)
{
    TestReals();
}

#endif

