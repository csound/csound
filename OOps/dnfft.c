/*  
    dnfft.c:

    Copyright (C) 2000 Mark Dolson, John ffitch

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

#include <math.h>
#include <stdio.h>
#include "cs.h"

void ford1(int, MYFLT *);
void ford2(int, MYFLT *);
void fr4syn(int, int,MYFLT *, MYFLT *, MYFLT *, MYFLT *, MYFLT *,
            MYFLT *, MYFLT *, MYFLT *);

double  c22, s22;
MYFLT p7, p7two;

/* -----------------------------------------------------------------
                                fast

This is the FFT subroutine fast.f from the IEEE library recoded in C.
        It takes a pointer to a real vector b[i], for i = 0, 1, ...,
        N-1, and replaces it with its finite discrete fourier transform.

-------------------------------------------------------------------*/


void fr2tr(int off, MYFLT *b0, MYFLT *b1)
{
    int i;

    MYFLT tmp;

    for (i=0; i<off; i++) {
      tmp = b0[i] + b1[i];
      b1[i] = b0[i] - b1[i];
      b0[i] = tmp;
    }

    return;

}

void fr4tr(int off, int nn, MYFLT *b0, MYFLT *b1, MYFLT *b2,
           MYFLT *b3, MYFLT *b4, MYFLT *b5, MYFLT *b6, MYFLT *b7)
{
    int         i, off4, jj0, jlast, k0, kl, ji, jl, jr, j, k,
                jj1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12,
                j13, j14, jthet, th2,
                l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12,
                l13, l14, l15, l[15];

    MYFLT piovn, arg, c1, c2, c3, s1, s2, s3, r1, r5, pr, pi,
                t0, t1, t2, t3, t4, t5, t6, t7;

/* jthet is a reversed binary counter; jr steps two at a time to
        locate real parts of intermediate results, and ji locates
        the imaginary part corresponding to jr.
*/

    l[0] = nn / 4;
    for (i=1; i<15; i++) {
      if (l[i-1] < 2) l[i-1] = 2;
      else if (l[i-1] != 2) {
        l[i] = l[i-1] / 2;
        continue;
      }
      l[i] = 2;
    }

    l1 = l[14];
    l2 = l[13];
    l3 = l[12];
    l4 = l[11];
    l5 = l[10];
    l6 = l[9];
    l7 = l[8];
    l8 = l[7];
    l9 = l[6];
    l10 = l[5];
    l11 = l[4];
    l12 = l[3];
    l13 = l[2];
    l14 = l[1];
    l15 = l[0];

    piovn = (MYFLT)PI / nn;
    ji = 3;
    jl = 2;
    jr = 2;

    /* for (j1=2; jj1<=l1; jj1+=2) { */
    jj1 = 2;
 loop1:  /* for (j2=jj1; j2<=l2; j2+=l1) { */
    j2 = jj1;
 loop2:   /* for (j3=j2; j3<=l3; j3+=l2) { */
    j3 = j2;
 loop3:    /* for (j4=j3; j4<=l4; j4+=l3) { */
    j4 = j3;
 loop4:     /* for (j5=j4; j5<=l5; j5+=l4) { */
    j5 = j4;
 loop5:      /* for (j6=j5; j6<=l6; j6+=l5) { */
    j6 = j5;
 loop6:       /* for (j7=j6; j7<=l7; j7+=l6) { */
    j7 = j6;
 loop7:        /* for (j8=j7; j8<=l8; j8+=l7) { */
    j8 = j7;
 loop8:         /* for (j9=j8; j9<=l9; j9+=l8) { */
    j9 = j8;
 loop9:
    for (j10=j9; j10<=l10; j10+=l9) {
      for (j11=j10; j11<=l11; j11+=l10) {
        for (j12=j11; j12<=l12; j12+=l11) {
          for (j13=j12; j13<=l13; j13+=l12) {
            for (j14=j13; j14<=l14; j14+=l13) {
              for (jthet=j14; jthet<=l15; jthet+=l14) {
                th2 = jthet - 2;
                if (th2 <= 0) {
                  for (i=0; i<off; i++) {
                    t0 = b0[i] + b2[i];
                    t1 = b1[i] + b3[i];
                    b2[i] = b0[i] - b2[i];
                    b3[i] = b1[i] - b3[i];
                    b0[i] = t0 + t1;
                    b1[i] = t0 - t1;
                  }
                  if (nn > 4) {
                    k0 = 4 * off;
                    kl = k0 + off;
                    for (i=k0; i<kl; i++) {
                      pr = (MYFLT)(p7 * (b1[i] - b3[i]));
                      pi = (MYFLT)(p7 * (b1[i] + b3[i]));
                      b3[i] = b2[i] + pi;
                      b1[i] = pi - b2[i];
                      b2[i] = b0[i] - pr;
                      b0[i] = b0[i] + pr;
                    }
                  }
                }
                else {
                  arg = th2 * piovn;
                  c1 = (MYFLT)cos(arg);
                  s1 = (MYFLT)sin(arg);
                  c2 = c1*c1 - s1*s1;
                  s2 = c1*s1 + c1*s1;
                  c3 = c1*c2 - s1*s2;
                  s3 = c2*s1 + s2*c1;
                  off4 = 4 * off;
                  jj0 = jr * off4;
                  k0 = ji * off4;
                  jlast = jj0 + off;
                  for (j=jj0; j<jlast; j++) {
                    k = k0 + j - jj0;
                    r1 = b1[j]*c1 - b5[k]*s1;
                    r5 = b1[j]*s1 + b5[k]*c1;
                    t2 = b2[j]*c2 - b6[k]*s2;
                    t6 = b2[j]*s2 + b6[k]*c2;
                    t3 = b3[j]*c3 - b7[k]*s3;
                    t7 = b3[j]*s3 + b7[k]*c3;
                    t0 = b0[j]+t2;
                    t4 = b4[k]+t6;
                    t2 = b0[j]-t2;
                    t6 = b4[k]-t6;
                    t1 = r1+t3;
                    t5 = r5+t7;
                    t3 = r1-t3;
                    t7 = r5-t7;
                    b0[j] = t0+t1;
                    b7[k] = t4+t5;
                    b6[k] = t0-t1;
                    b1[j] = t5-t4;
                    b2[j] = t2-t7;
                    b5[k] = t6+t3;
                    b4[k] = t2+t7;
                    b3[j] = t3-t6;
                  }
                  jr += 2;
                  ji -= 2;
                  if (ji <= jl) {
                    ji = 2*jr - 1;
                    jl = jr;
                  }
                }
              }
            }
          }
        }
      }
    }
    j9 += l8;
    if (j9 <= l9) goto loop9;
    /* } */
    j8 += l7;
    if (j8 <= l8) goto loop8;
    /* } */
    j7 += l6;
    if (j7 <= l7) goto loop7;
    /* } */
    j6 += l5;
    if (j6 <= l6) goto loop6;
    /* } */
    j5 += l4;
    if (j5 <= l5) goto loop5;
    /* } */
    j4 += l3;
    if (j4 <= l4) goto loop4;
    /* } */
    j3 += l2;
    if (j3 <= l3) goto loop3;
    /* } */
    j2 += l1;
    if (j2 <= l2) goto loop2;
    /* } */
    jj1 += 2;
    if (jj1 <= l1) goto loop1;
    /* } */

    return;

}

void fast(MYFLT *b, int N)
{

  /* The DC term is returned in location b[0] with b[1] set to 0.
     Thereafter, the i'th harmonic is returned as a complex
     number stored as b[2*i] + j b[2*i+1].  The N/2 harmonic
     is returned in b[N] with b[N+1] set to 0.  Hence, b must
     be dimensioned to size N+2.  The subroutine is called as
     fast(b,N) where N=2**M and b is the real array described
     above.
  */

    MYFLT pi8, tmp;

    int   M;

    int   i, Nt, off;

    pi8 = PI_F / FL(8.0);
    p7 = FL(1.0) / (MYFLT)sqrt(2.0);
    p7two = FL(2.0) * p7;
    c22 = cos(pi8);
    s22 = sin(pi8);

  /* determine M -- max is 13 (i.e, N = 8192) because 14 requires HUGE array */

    for (i=1, Nt=2; i<14; i++,Nt *= 2) if (Nt==N) break;
    M = i;
    if (M==14) {
      err_printf(Str(X_42,"fast: N is not an allowable power of two\n"));
      exit(1);
    }

    /* do a radix 2 iteration first if one is required */

    Nt = 1;
    if (M%2 != 0) {
      Nt = 2;
      off = N / Nt;
      fr2tr(off,b,b+off);
    }

    /* perform radix 4 iterations */

    if (M != 1) for (i=1; i<=M/2; i++) {
      Nt *= 4;
      off = N / Nt;
      fr4tr(off,Nt,b,b+off,b+2*off,b+3*off,b,b+off,b+2*off,b+3*off);
    }

    /* perform in-place reordering */

    ford1(M,b);
    ford2(M,b);

    tmp = b[1];
    b[1] = FL(0.0);
    b[N] = tmp;
    b[N+1] = FL(0.0);

    for (i=3; i<N; i+=2) b[i] *= -FL(1.0);

    return;
}

/* -----------------------------------------------------------------
                                fsst

This is the FFT subroutine fsst.f from the IEEE library recoded in C.
        It takes a pointer to a vector b[i], for i = 0, 1, ..., N-1,
        and replaces it with its inverse DFT assuming real output.

-------------------------------------------------------------------*/

void fsst(MYFLT *b, int N)
{

  /* This subroutine synthesizes the real vector b[k] for k=0, 1,
     ..., N-1 from the fourier coefficients stored in the b
     array of size N+2.  The DC term is in location b[0] with
     b[1] equal to 0.  The i'th harmonic is a complex number
     stored as b[2*i] + j b[2*i+1].  The N/2 harmonic is in
     b[N] with b[N+1] equal to 0. The subroutine is called as
     fsst(b,N) where N=2**M and b is the real array described
     above.
  */
    MYFLT pi8, invN;

    int   M;

    int   i, Nt, off;

    pi8 = PI_F / FL(8.0);
    p7 = FL(1.0) / (MYFLT)sqrt(2.0);
    p7two = p7 + p7;
    c22 = cos(pi8);
    s22 = sin(pi8);

    /* determine M -- max is 13 (i.e, N = 8192) because 14 requires HUGE array */

    for (i=1, Nt=2; i<14; i++,Nt *= 2) if (Nt==N) break;
    M = i;
    if (M==14) {
      err_printf(Str(X_42,"fast: N is not an allowable power of two\n"));
      exit(1);
    }

    b[1] = b[N];

    for (i=3; i<N; i+=2) b[i] *= -1.;

    /* scale the input by N */

    invN = FL(1.0) / N;
    for (i=0; i<N; i++) b[i] *= invN;

    /* scramble the inputs */

    ford2(M,b);
    ford1(M,b);

    /* perform radix 4 iterations */

    Nt = 4*N;
    if (M != 1) for (i=1; i<=M/2; i++) {
      Nt /= 4;
      off = N / Nt;
      fr4syn(off,Nt,b,b+off,b+2*off,b+3*off,b,b+off,b+2*off,b+3*off);
    }

    /* do a radix 2 iteration if one is required */

    if (M%2 != 0) {
      off = N / 2;
      fr2tr(off,b,b+off);
    }

    return;
}

/*---------------------------------------
        ford1

in-place reordering subroutine
#--------------------------------------*/

void ford1(int m, MYFLT *b)
{
    int k, kl, n, j;

    MYFLT t;

    k = 4;
    kl = 2;
    n = (int)pow(2.0,(double)m);
    for (j = 4; j <= n; j+=2) {
      if (k>j) {
        t = b[j-1];
        b[j-1] = b[k-1];
        b[k-1] = t;
      }
      k = k-2;
      if (k<=kl) {
        k = 2*j;
        kl = j;
      }
    }

    return;

}


/*-------------------------------
        ford2

in-place reordering subroutine
-------------------------------*/
void ford2(int m, MYFLT *b)
{
    MYFLT       t;

    int n, ij, ji, k,
      jj1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12,
      j13, j14,
      l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12,
      l13, l14, l15, l[15];

    n = (int)pow(2.0,(double)m);
    l[0] = n;
    for (k = 2; k <= m; k++) l[k-1] = l[k-2]/2;
    for (k = m; k <= 14; k++) l[k] = 2;
    ij = 2;

    l1 = l[14];
    l2 = l[13];
    l3 = l[12];
    l4 = l[11];
    l5 = l[10];
    l6 = l[9];
    l7 = l[8];
    l8 = l[7];
    l9 = l[6];
    l10 = l[5];
    l11 = l[4];
    l12 = l[3];
    l13 = l[2];
    l14 = l[1];
    l15 = l[0];


    /* for (jj1=2; jj1<=l1; jj1+=2) { */
    jj1 = 2;
 loop1:  /* for (j2=jj1; j2<=l2; j2+=l1) { */
    j2 = jj1;
 loop2:   /* for (j3=j2; j3<=l3; j3+=l2) { */
    j3 = j2;
 loop3:    /* for (j4=j3; j4<=l4; j4+=l3) { */
    j4 = j3;
 loop4:     /* for (j5=j4; j5<=l5; j5+=l4) { */
    j5 = j4;
 loop5:      /* for (j6=j5; j6<=l6; j6+=l5) { */
    j6 = j5;
 loop6:       /* for (j7=j6; j7<=l7; j7+=l6) { */
    j7 = j6;
 loop7:        /* for (j8=j7; j8<=l8; j8+=l7) { */
    j8 = j7;
 loop8:
    for (j9=j8; j9<=l9; j9+=l8) {
      for (j10=j9; j10<=l10; j10+=l9) {
        for (j11=j10; j11<=l11; j11+=l10) {
          for (j12=j11; j12<=l12; j12+=l11) {
            for (j13=j12; j13<=l13; j13+=l12) {
              for (j14=j13; j14<=l14; j14+=l13) {
                for (ji=j14; ji<=l15; ji+=l14) {
                  if (ij<ji) {
                    t = b[ij-2];
                    b[ij-2] = b[ji-2];
                    b[ji-2] = t;
                    t = b[ij-1];
                    b[ij-1] = b[ji-1];
                    b[ji-1] = t;
                  }
                  ij = ij+2;
                }
              }
            }
          }
        }
      }
    }
    j8 += l7;
    if (j8 <= l8) goto loop8;
    /* } */
    j7 += l6;
    if (j7 <= l7) goto loop7;
    /* } */
    j6 += l5;
    if (j6 <= l6) goto loop6;
    /* } */
    j5 += l4;
    if (j5 <= l5) goto loop5;
    /* } */
    j4 += l3;
    if (j4 <= l4) goto loop4;
    /* } */
    j3 += l2;
    if (j3 <= l3) goto loop3;
    /* } */
    j2 += l1;
    if (j2 <= l2) goto loop2;
    /* } */
    jj1 += 2;
    if (jj1 <= l1) goto loop1;
    /* } */

    return;

}

/*-------------------------------
        fr4syn

radix 4 synthesis
-------------------------------*/

void fr4syn(int off, int nn,
            MYFLT *b0, MYFLT *b1, MYFLT *b2, MYFLT *b3, MYFLT *b4,
            MYFLT *b5, MYFLT *b6, MYFLT *b7)
{

    MYFLT       piovn, arg, c1, c2, c3, s1, s2, s3,
                t0, t1, t2, t3, t4, t5, t6, t7;

    int         i, off4, jj0, jlast, k0, kl, ji, jl, jr, j, k,
                jj1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12,
                j13, j14, jthet, th2,
                l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12,
                l13, l14, l15,
                l[15];

/* jthet is a reversed binary counter; jr steps two at a time to
        locate real parts of intermediate results, and ji locates
        the imaginary part corresponding to jr.
*/

    l[0] = nn/4;
    for (i=1; i<15; i++) {
      if (l[i-1]<2) l[i-1] = 2;
      else if (l[i-1]!=2) {
        l[i] = l[i-1]/2;
        continue;
      }
      l[i] = 2;
    }

    l1    = l[14];
    l2    = l[13];
    l3    = l[12];
    l4    = l[11];
    l5    = l[10];
    l6    = l[9];
    l7    = l[8];
    l8    = l[7];
    l9    = l[6];
    l10   = l[5];
    l11   = l[4];
    l12   = l[3];
    l13   = l[2];
    l14   = l[1];
    l15   = l[0];

    piovn = PI_F / nn;
    ji    = 3;
    jl    = 2;
    jr    = 2;

    /* for (jj1=2; jj1<=l1; jj1+=2) { */
    jj1 = 2;
 loop1:  /* for (j2=jj1; j2<=l2; j2+=l1) { */
    j2 = jj1;
 loop2:   /* for (j3=j2; j3<=l3; j3+=l2) { */
    j3 = j2;
 loop3:    /* for (j4=j3; j4<=l4; j4+=l3) { */
    j4 = j3;
 loop4:     /* for (j5=j4; j5<=l5; j5+=l4) { */
    j5 = j4;
 loop5:      /* for (j6=j5; j6<=l6; j6+=l5) { */
    j6 = j5;
 loop6:       /* for (j7=j6; j7<=l7; j7+=l6) { */
    j7 = j6;
 loop7:       /*  for (j8=j7; j8<=l8; j8+=l7) { */
    j8 = j7;
 loop8:         /* for (j9=j8; j9<=l9; j9+=l8) { */
    j9 = j8;
 loop9:
    for (j10=j9; j10<=l10; j10+=l9) {
      for (j11=j10; j11<=l11; j11+=l10) {
        for (j12=j11; j12<=l12; j12+=l11) {
          for (j13=j12; j13<=l13; j13+=l12) {
            for (j14=j13; j14<=l14; j14+=l13) {
              for (jthet=j14; jthet<=l15; jthet+=l14) {
                th2 = jthet - 2;
                if (th2 > 0) {
                  arg = th2 * piovn;
                  c1 = (MYFLT)cos(arg);
                  s1 = -(MYFLT)sin(arg);
                  c2 = c1*c1 - s1*s1;
                  s2 = c1*s1 + c1*s1;
                  c3 = c1*c2 - s1*s2;
                  s3 = c2*s1 + s2*c1;
                  off4 = 4 * off;
                  jj0 = jr * off4;
                  k0 = ji * off4;
                  jlast = jj0 + off;
                  for (j=jj0; j<jlast; j++) {
                    k = k0+j-jj0;
                    t0 = b0[j] + b6[k];
                    t1 = b7[k] - b1[j];
                    t2 = b0[j] - b6[k];
                    t3 = b7[k] + b1[j];
                    t4 = b2[j] + b4[k];
                    t5 = b5[k] - b3[j];
                    t6 = b5[k] + b3[j];
                    t7 = b4[k] - b2[j];
                    b0[j] = t0 + t4;
                    b4[k] = t1 + t5;
                    b1[j] = (t2+t6)*c1 - (t3+t7)*s1;
                    b5[k] = (t2+t6)*s1 + (t3+t7)*c1;
                    b2[j] = (t0-t4)*c2 - (t1-t5)*s2;
                    b6[k] = (t0-t4)*s2 + (t1-t5)*c2;
                    b3[j] = (t2-t6)*c3 - (t3-t7)*s3;
                    b7[k] = (t2-t6)*s3 + (t3-t7)*c3;
                  }
                  jr = jr + 2;
                  ji = ji - 2;
                  if (ji <= jl) {
                    ji = 2*jr - 1;
                    jl = jr;
                  }
                }
                else {
                  for (i=0; i<off; i++) {
                    t0 = b0[i] + b1[i];
                    t1 = b0[i] - b1[i];
                    t2 = b2[i] + b2[i];
                    t3 = b3[i] + b3[i];
                    b0[i] = t0 + t2;
                    b2[i] = t0 - t2;
                    b1[i] = t1 + t3;
                    b3[i] = t1 - t3;
                  }
                  if (nn>4) {
                    k0 = 4*off;
                    kl = k0 + off;
                    for (i=k0; i<kl; i++) {
                      t2 = b0[i] - b2[i];
                      t3 = b1[i] + b3[i];
                      b0[i] = (b0[i] + b2[i]) * FL(2.0);
                      b2[i] = (b3[i] - b1[i]) * FL(2.0);
                      b1[i] = (t2 + t3) * p7two;
                      b3[i] = (t3 - t2) * p7two;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    j9 += l8;
    if (j9 <= l9) goto loop9;
    /* } */
    j8 += l7;
    if (j8 <= l8) goto loop8;
    /* } */
    j7 += l6;
    if (j7 <= l7) goto loop7;
    /* } */
    j6 += l5;
    if (j6 <= l6) goto loop6;
    /* } */
    j5 += l4;
    if (j5 <= l5) goto loop5;
    /* } */
    j4 += l3;
    if (j4 <= l4) goto loop4;
    /* } */
    j3 += l2;
    if (j3 <= l3) goto loop3;
    /* } */
    j2 += l1;
    if (j2 <= l2) goto loop2;
    /* } */
    jj1 += 2;
    if (jj1 <= l1) goto loop1;
    /* } */

    return;

}
