/*
    mxfft.c:

    Copyright (C) 2002 Trevor Wishart, Keith Henderson

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

/* This program converted from the FORTRAN routines by Singleton in
 * Section 1.4 of  "Programs for Digital Signal Processing", IEEE Press, 1979.
 *  Conversion by Trevor Wishart and Keith Henderson, York Univ.
 */
/*
static char *rcsid = "$Id$";
 */
/*
 *      $Log$
 *      Revision 1.15  2008-12-04 14:55:04  jpff
 *      Branch prediction
 *
 *      Revision 1.14  2005/08/13 14:44:37  istvanv
 *      Minor code changes
 *
 *      Revision 1.13  2005/08/12 19:01:23  istvanv
 *      Renamed ENVIRON to CSOUND
 *
 *      Revision 1.12  2005/08/10 09:57:07  istvanv
 *      Use CSOUND* type for Csound instance pointers instead of void*
 *
 *      Revision 1.11  2005/07/15 10:13:28  istvanv
 *      Removed cs.h
 *
 *      Revision 1.10  2005/06/05 16:36:24  istvanv
 *      Minor code improvements
 *
 *      Revision 1.9  2005/06/05 13:07:08  istvanv
 *      Added mxfft.c functions to API
 *
 *      Revision 1.8  2005/05/28 13:00:02  istvanv
 *      Minor code changes (tabs etc.)
 *
 *      Revision 1.7  2005/04/17 17:56:25  jpff
 *      Warnings
 *
 *      Revision 1.6  2005/02/18 16:21:14  istvanv
 *      added csound pointer to csound->Malloc, auxalloc, and other functions
 *
 *      Revision 1.5  2005/01/27 19:22:50  istvanv
 *      Merged changes from 4.24.1, including new localization system,
 *      timers, and allow use of underscore character in opcode names
 *
 *      Revision 1.4  2004/09/27 05:52:31  jpff
 *      Minor coding
 *
 *      Revision 1.3  2004/06/07 11:33:09  jpff
 *      line endings
 *
 *      Revision 1.2  2004/05/31 15:53:06  jpff
 *      Removing warnings mainly
 *
 *      Revision 1.1.1.1  2003/06/19 11:11:53  jpff
 *      Initial upload of sources
 *
 *      Revision 1.2  2003/05/21 11:31:06  jpff
 *      Added Copyright notices
 *
 * Revision 3.4  1994/10/31  17:37:28  martin
 * Starting with rcs
 *
 */
#include "csoundCore.h"
#include <math.h>
#include <assert.h>

static void fft_(CSOUND *,MYFLT *, MYFLT *, int32_t, int32_t, int32_t, int32_t);
static void fftmx(MYFLT *, MYFLT *, int32_t, int32_t, int32_t, int32_t, int32_t,
                  int32_t*, MYFLT *, MYFLT *, MYFLT *, MYFLT *, int32_t *,
                  int32_t[]);
static void reals_(CSOUND *,MYFLT *, MYFLT *, int32_t, int32_t);

/*
 *-----------------------------------------------------------------------
 * subroutine:  fft
 * multivariate complex fourier transform, computed in place
 * using mixed-radix fast fourier transform algorithm.
 *-----------------------------------------------------------------------
 *
 *      this is the call from C:
 *              fft_(anal,banal,&one,&N2,&one,&mtwo);
 *      CHANGED TO:-
 *              fft_(csound,anal,banal,one,N2,one,mtwo);
 */

static void fft_(CSOUND *csound, MYFLT *a, MYFLT *b,
                                  int32_t nseg, int32_t n, int32_t nspn, int32_t isn)
  /*    *a,       pointer to array 'anal'  */
  /*    *b;       pointer to array 'banal' */
{
    int32_t nfac[32];           /*  These are one bigger than needed   */
                                /*  because wish to use Fortran array  */
                                /* index which runs 1 to n, not 0 to n */

    int32_t     m = 0,
                k,
                kt,
                jj,
                j,
                nf,
                ntot,
                maxf, maxp=-1;

    /* work space pointers */
    void        *buf;
    MYFLT       *at, *ck, *bt, *sk;
    int32_t         *np;

    /* reduce the pointers to input arrays - by doing this, FFT uses FORTRAN
       indexing but retains compatibility with C arrays */
    a--;        b--;

    /*
     * determine the factors of n
     */
    k = nf = abs(n);
    if (nf==1)
      return;

    nspn = abs(nf*nspn);
    ntot = abs(nspn*nseg);

    for (m=0; !(k%16); nfac[++m]=4,k/=16);
    assert(m<16);
    for (j=3,jj=9; jj<=k; j+=2,jj=j*j)
      for (; !(k%jj); nfac[++m]=j,k/=jj);

    if (k<=4) {
      kt = m;
      nfac[m+1] = k;
      if (k != 1)
        m++;
    }
    else {
      if (k%4==0) {
        nfac[++m]=2;
        k/=4;
      }

      kt = m;
      maxp = (kt+kt+2 > k-1 ? kt+kt+2 : k-1);
      for (j=2; j<=k; j=1+((j+1)/2)*2)
        if (k%j==0) {
          nfac[++m]=j;
          k/=j;
        }
    }
    if (m <= kt+1)
      maxp = m + kt + 1;
    if (UNLIKELY(m+kt > 15)) {
      csound->Warning(csound, Str("\nerror - fft parameter n has "
                                  "more than 15 factors : %d"), n);
      return;
    }
    if (kt!=0) {
      j = kt;
      while (j)
        nfac[++m]=nfac[j--];
    }
    maxf = nfac[m-kt];
    if (kt > 0 && maxf <nfac[kt])
      maxf = nfac[kt];

    /* allocate workspace - assume no errors! */
    buf = csound->Calloc(csound, sizeof(MYFLT) * 4 * maxf + sizeof(int32_t) * maxp);
    at = (MYFLT*) buf;
    ck = (MYFLT*) at + (int32_t) maxf;
    bt = (MYFLT*) ck + (int32_t) maxf;
    sk = (MYFLT*) bt + (int32_t) maxf;
    np = (int32_t*) ((void*) ((MYFLT*) sk + (int32_t) maxf));

    /* decrement pointers to allow FORTRAN type usage in fftmx */
    at--; bt--; ck--; sk--; np--;

    /* call fft driver */
    fftmx(a, b, ntot, nf, nspn, isn, m, &kt, at, ck, bt, sk, np, nfac);

    /* release working storage before returning - assume no problems */
    csound->Free(csound,buf);
}

/*
 *-----------------------------------------------------------------------
 * subroutine:  fftmx
 * called by subroutine 'fft' to compute mixed-radix fourier transform
 *-----------------------------------------------------------------------
 */

static void fftmx(MYFLT *a, MYFLT *b,
                  int32_t ntot, int32_t n, int32_t nspan, int32_t isn, int32_t m,
                  int32_t *kt, MYFLT *at, MYFLT *ck, MYFLT *bt, MYFLT *sk,
                  int32_t *np, int32_t nfac[])
{
    int32_t i,inc,
        j,jc,jf, jj,
        k, k1, k2, k3=0, k4,
        kk,klim,ks,kspan, kspnn,
        lim,
        maxf,mm,
        nn,nt;
    double  aa, aj, ajm, ajp, ak, akm, akp,
      bb, bj, bjm, bjp, bk, bkm, bkp,
      c1, c2=0, c3=0, c72, cd,
      dr,
      rad,
      sd, s1, s2=0, s3=0, s72, s120;

    double      xx;     /****** ADDED APRIL 1991 *********/
    inc=abs(isn);
    nt = inc*ntot;
    ks = inc*nspan;
/******************* REPLACED MARCH 29: ***********************
                                        rad = atan(1.0);
**************************************************************/
    rad = 0.785398163397448278900;
/******************* REPLACED MARCH 29: ***********************
                                        s72 = rad/0.625;
                                        c72 = cos(s72);
                                        s72 = sin(s72);
**************************************************************/
    c72 = 0.309016994374947451270;
    s72 = 0.951056516295153531190;
/******************* REPLACED MARCH 29: ***********************
                                        s120 = sqrt(0.75);
**************************************************************/
    s120 = 0.866025403784438707600;

/* scale by 1/n for isn > 0 ( reverse transform ) */

    if (isn < 0) {
      s72 = -s72;
      s120 = -s120;
      rad = -rad;}
    else {
      ak = 1.0/(double)n;
      for (j=1; j<=nt;j += inc) {
        a[j] *= (MYFLT)ak;
        b[j] *= (MYFLT)ak;
      }
    }
    kspan = ks;
    nn = nt - inc;
    jc = ks/n;

/* sin, cos values are re-initialised each lim steps  */

    lim = 32;
    klim = lim * jc;
    i = 0;
    jf = 0;
    maxf = m - (*kt);
    maxf = nfac[maxf];
    if ((*kt) > 0 && maxf < nfac[*kt])
      maxf = nfac[*kt];

/*
 * compute Fourier transform
 */

 lbl40:
    dr = (8.0 * (double)jc)/((double)kspan);
/*************************** APRIL 1991 POW & POW2 not WORKING.. REPLACE *******
                    cd = 2.0 * (pow2 ( sin(0.5 * dr * rad)) );
*******************************************************************************/
    xx =  sin(0.5 * dr * rad);
    cd = 2.0 * xx * xx;
    sd = sin(dr * rad);
    kk = 1;
    if (nfac[++i]!=2) goto lbl110;
/*
 * transform for factor of 2 (including rotation factor)
 */
    kspan /= 2;
    k1 = kspan + 2;
    do {
      do {
        k2 = kk + kspan;
        ak = a[k2];
        bk = b[k2];
        a[k2] = (a[kk]) - (MYFLT)ak;
        b[k2] = (b[kk]) - (MYFLT)bk;
        a[kk] = (a[kk]) + (MYFLT)ak;
        b[kk] = (b[kk]) + (MYFLT)bk;
        kk = k2 + kspan;
      } while (kk <= nn);
      kk -= nn;
    } while (kk <= jc);
    if (kk > kspan) goto lbl350;
 lbl60:
    c1 = 1.0 - cd;
    s1 = sd;
    mm = (k1/2 < klim ? k1/2 :klim);
    goto lbl80;
 lbl70:
    ak = c1 - ((cd*c1)+(sd*s1));
    s1 = ((sd*c1)-(cd*s1)) + s1;
    c1 = ak;
 lbl80:
    do {
      do {
        k2 = kk + kspan;
        ak = a[kk] - a[k2];
        bk = b[kk] - b[k2];
        a[kk] = a[kk] + a[k2];
        b[kk] = b[kk] + b[k2];
        a[k2] = (MYFLT)((c1 * ak) - (s1 * bk));
        b[k2] = (MYFLT)((s1 * ak) + (c1 * bk));
        kk = k2 + kspan;
      } while (kk < nt);
      k2 = kk - nt;
      c1 = -c1;
      kk = k1 - k2;
    } while (kk > k2);
    kk += jc;
    if (kk <= mm) goto lbl70;
    if (kk < k2)  goto lbl90;
    k1 += (inc + inc);
    kk = ((k1-kspan)/2) + jc;
    if (kk <= (jc+jc)) goto lbl60;
    goto lbl40;
 lbl90:
    s1 = ((double)((kk-1)/jc)) * dr * rad;
    c1 = cos(s1);
    s1 = sin(s1);
    mm = (k1/2 < mm+klim ? k1/2 : mm+klim);
    goto lbl80;
/*
 * transform for factor of 3 (optional code)
 */

 lbl100:
    k1 = kk + kspan;
    k2 = k1 + kspan;
    ak = a[kk];
    bk = b[kk];
    aj = a[k1] + a[k2];
    bj = b[k1] + b[k2];
    a[kk] = (MYFLT)(ak + aj);
    b[kk] = (MYFLT)(bk + bj);
    ak += (-0.5 * aj);
    bk += (-0.5 * bj);
    aj = (a[k1] - a[k2]) * s120;
    bj = (b[k1] - b[k2]) * s120;
    a[k1] = (MYFLT)(ak - bj);
    b[k1] = (MYFLT)(bk + aj);
    a[k2] = (MYFLT)(ak + bj);
    b[k2] = (MYFLT)(bk - aj);
    kk = k2 + kspan;
    if (kk < nn)     goto lbl100;
    kk -= nn;
    if (kk <= kspan) goto lbl100;
    goto lbl290;

/*
 * transform for factor of 4
 */

 lbl110:
    if (nfac[i] != 4) goto lbl230;
    //kspnn = kspan;
    kspan = kspan/4;
 lbl120:
    c1 = 1.0;
    s1 = 0;
    mm = (kspan < klim ? kspan : klim);
    goto lbl150;
 lbl130:
    c2 = c1 - ((cd*c1)+(sd*s1));
    s1 = ((sd*c1)-(cd*s1)) + s1;
/*
 * the following three statements compensate for truncation
 * error.  if rounded arithmetic is used, substitute
 * c1=c2
 *
 * c1 = (0.5/(pow2(c2)+pow2(s1))) + 0.5;
 * s1 = c1*s1;
 * c1 = c1*c2;
 */
    c1 = c2;
 lbl140:
    c2 = (c1 * c1) - (s1 * s1);
    s2 = c1 * s1 * 2.0;
    c3 = (c2 * c1) - (s2 * s1);
    s3 = (c2 * s1) + (s2 * c1);
 lbl150:
    k1 = kk + kspan;
    k2 = k1 + kspan;
    k3 = k2 + kspan;
    akp = a[kk] + a[k2];
    akm = a[kk] - a[k2];
    ajp = a[k1] + a[k3];
    ajm = a[k1] - a[k3];
    a[kk] = (MYFLT)(akp + ajp);
    ajp = akp - ajp;
    bkp = b[kk] + b[k2];
    bkm = b[kk] - b[k2];
    bjp = b[k1] + b[k3];
    bjm = b[k1] - b[k3];
    b[kk] = (MYFLT)(bkp + bjp);
    bjp = bkp - bjp;
    if (isn < 0) goto lbl180;
    akp = akm - bjm;
    akm = akm + bjm;
    bkp = bkm + ajm;
    bkm = bkm - ajm;
    if (s1 == 0.0) goto lbl190;
 lbl160:
    a[k1] = (MYFLT)((akp*c1) - (bkp*s1));
    b[k1] = (MYFLT)((akp*s1) + (bkp*c1));
    a[k2] = (MYFLT)((ajp*c2) - (bjp*s2));
    b[k2] = (MYFLT)((ajp*s2) + (bjp*c2));
    a[k3] = (MYFLT)((akm*c3) - (bkm*s3));
    b[k3] = (MYFLT)((akm*s3) + (bkm*c3));
    kk = k3 + kspan;
    if (kk <= nt)   goto lbl150;
 lbl170:
    kk -= (nt - jc);
    if (kk <= mm)   goto lbl130;
    if (kk < kspan) goto lbl200;
    kk -= (kspan - inc);
    if (kk <= jc)   goto lbl120;
    if (kspan==jc)  goto lbl350;
    goto lbl40;
 lbl180:
    akp = akm + bjm;
    akm = akm - bjm;
    bkp = bkm - ajm;
    bkm = bkm + ajm;
    if (s1 != 0.0)  goto lbl160;
 lbl190:
    a[k1] = (MYFLT)akp;
    b[k1] = (MYFLT)bkp;
    a[k2] = (MYFLT)ajp;
    b[k2] = (MYFLT)bjp;
    a[k3] = (MYFLT)akm;
    b[k3] = (MYFLT)bkm;
    kk = k3 + kspan;
    if (kk <= nt) goto lbl150;
    goto lbl170;
 lbl200:
    s1 = ((double)((kk-1)/jc)) * dr * rad;
    c1 = cos(s1);
    s1 = sin(s1);
    mm = (kspan < mm+klim ? kspan : mm+klim);
    goto lbl140;

/*
 * transform for factor of 5 (optional code)
 */

 lbl210:
    c2 = (c72*c72) - (s72*s72);
    s2 = 2.0 * c72 * s72;
 lbl220:
    k1 = kk + kspan;
    k2 = k1 + kspan;
    k3 = k2 + kspan;
    k4 = k3 + kspan;
    akp = a[k1] + a[k4];
    akm = a[k1] - a[k4];
    bkp = b[k1] + b[k4];
    bkm = b[k1] - b[k4];
    ajp = a[k2] + a[k3];
    ajm = a[k2] - a[k3];
    bjp = b[k2] + b[k3];
    bjm = b[k2] - b[k3];
    aa = a[kk];
    bb = b[kk];
    a[kk] = (MYFLT)(aa + akp + ajp);
    b[kk] = (MYFLT)(bb + bkp + bjp);
    ak = (akp*c72) + (ajp*c2) + aa;
    bk = (bkp*c72) + (bjp*c2) + bb;
    aj = (akm*s72) + (ajm*s2);
    bj = (bkm*s72) + (bjm*s2);
    a[k1] = (MYFLT)(ak - bj);
    a[k4] = (MYFLT)(ak + bj);
    b[k1] = (MYFLT)(bk + aj);
    b[k4] = (MYFLT)(bk - aj);
    ak = (akp*c2) + (ajp*c72) + aa;
    bk = (bkp*c2) + (bjp*c72) + bb;
    aj = (akm*s2) - (ajm*s72);
    bj = (bkm*s2) - (bjm*s72);
    a[k2] = (MYFLT)(ak - bj);
    a[k3] = (MYFLT)(ak + bj);
    b[k2] = (MYFLT)(bk + aj);
    b[k3] = (MYFLT)(bk - aj);
    kk = k4 + kspan;
    if (kk < nn)     goto lbl220;
    kk -= nn;
    if (kk <= kspan) goto lbl220;
    goto lbl290;

/*
 * transform for odd factors
 */

 lbl230:
    k = nfac[i];
    kspnn = kspan;
    kspan /= k;
    if (k==3)   goto lbl100;
    if (k==5)   goto lbl210;
    if (k==jf)  goto lbl250;
    jf = k;
    s1 = rad/(((double)(k))/8.0);
    c1 = cos(s1);
    s1 = sin(s1);
    ck[jf] = FL(1.0);
    sk[jf] = FL(0.0);
    for (j=1; j<k ; j++) {
      ck[j] = (MYFLT)((ck[k])*c1 + (sk[k])*s1);
      sk[j] = (MYFLT)((ck[k])*s1 - (sk[k])*c1);
      k--;
      ck[k] = ck[j];
      sk[k] = -(sk[j]);
    }
 lbl250:
    k1 = kk;
    k2 = kk + kspnn;
    aa = a[kk];
    bb = b[kk];
    ak = aa;
    bk = bb;
    j = 1;
    k1 += kspan;
    do {
      k2 -= kspan;
      j++;
      at[j] = a[k1] + a[k2];
      ak = at[j] + ak;
      bt[j] = b[k1] + b[k2];
      bk = bt[j] + bk;
      j++;
      at[j] = a[k1] - a[k2];
      bt[j] = b[k1] - b[k2];
      k1 += kspan;
    } while (k1 < k2);
    a[kk] = (MYFLT)ak;
    b[kk] = (MYFLT)bk;
    k1 = kk;
    k2 = kk + kspnn;
    j = 1;
 lbl270:
    k1 += kspan;
    k2 -= kspan;
    jj = j;
    ak = aa;
    bk = bb;
    aj = 0.0;
    bj = 0.0;
    k = 1;
    do {
      k++;
      ak = (at[k] * ck[jj]) + ak;
      bk = (bt[k] * ck[jj]) + bk;
      k++;
      aj = (at[k] * sk[jj]) + aj;
      bj = (bt[k] * sk[jj]) + bj;
      jj += j;
      if (jj > jf)
        jj -= jf;
    } while (k < jf);
    k = jf - j;
    a[k1] = (MYFLT)(ak - bj);
    b[k1] = (MYFLT)(bk + aj);
    a[k2] = (MYFLT)(ak + bj);
    b[k2] = (MYFLT)(bk - aj);
    j++;
    if (j < k)     goto lbl270;
    kk += kspnn;
    if (kk <= nn)  goto lbl250;
    kk -= nn;
    if (kk<=kspan) goto lbl250;

/*
 * multiply by rotation factor (except for factors of 2 and 4)
 */

 lbl290:
    if (i==m) goto lbl350;
    kk = jc + 1;
 lbl300:
    c2 = 1.0 - cd;
    s1 = sd;
    mm = (kspan < klim ? kspan : klim);
    goto lbl320;
 lbl310:
    c2 = c1 - ((cd*c1) + (sd*s1));
    s1 = s1 + ((sd*c1) - (cd*s1));
 lbl320:
    c1 = c2;
    s2 = s1;
    kk += kspan;
 lbl330:
    ak = a[kk];
    a[kk] = (MYFLT)((c2*ak) - (s2 * b[kk]));
    b[kk] = (MYFLT)((s2*ak) + (c2 * b[kk]));
    kk += kspnn;
    if (kk <= nt) goto lbl330;
    ak = s1*s2;
    s2 = (s1*c2) + (c1*s2);
    c2 = (c1*c2) - ak;
    kk -= (nt - kspan);
    if (kk <= kspnn) goto lbl330;
    kk -= (kspnn - jc);
    if (kk <= mm)   goto lbl310;
    if (kk < kspan) goto lbl340;
    kk -= (kspan - jc - inc);
    if (kk <= (jc+jc)) goto lbl300;
    goto lbl40;
 lbl340:
    s1 = ((double)((kk-1)/jc)) * dr * rad;
    c2 = cos(s1);
    s1 = sin(s1);
    mm = (kspan < mm+klim ?  kspan :mm+klim);
    goto lbl320;

/*
 * permute the results to normal order---done in two stages
 * permutation for square factors of n
 */

 lbl350:
    np[1] = ks;
    if (!(*kt)) goto lbl440;
    k = *kt + *kt + 1;
    if (m < k)
      k--;
    np[k+1] = jc;
    for (j=1; j < k; j++,k--) {
      np[j+1] = np[j] / nfac[j];
      np[k] = np[k+1] * nfac[j];
    }
    k3 = np[k+1];
    kspan = np[2];
    kk = jc + 1;
    k2 = kspan + 1;
    j = 1;
    if (n != ntot) goto lbl400;
/*
 * permutation for single-variate transform (optional code)
 */
 lbl370:
    do {
      ak = a[kk];
      a[kk] = a[k2];
      a[k2] = (MYFLT)ak;
      bk = b[kk];
      b[kk] = b[k2];
      b[k2] = (MYFLT)bk;
      kk += inc;
      k2 += kspan;
    } while (k2 < ks);
lbl380:
    do {
      k2 -= np[j++];
      k2 += np[j+1];
    } while (k2 > np[j]);
    j = 1;
 lbl390:
    if (kk < k2) {
      goto lbl370;
    }
    kk += inc;
    k2 += kspan;
    if (k2 < ks) goto lbl390;
    if (kk < ks) goto lbl380;
    jc = k3;
    goto lbl440;
/*
 * permutation for multivariate transform
 */
 lbl400:
    do {
      do {
        k = kk + jc;
        do {
          ak = a[kk];
          a[kk] = a[k2];
          a[k2] = (MYFLT)ak;
          bk = b[kk];
          b[kk] = b[k2];
          b[k2] = (MYFLT)bk;
          kk += inc;
          k2 += inc;
        } while (kk < k);
        kk += (ks - jc);
        k2 += (ks - jc);
      } while (kk < nt);
      k2 -= (nt - kspan);
      kk -= (nt - jc);
    } while (k2 < ks);
 lbl420:
    do {
      k2 -= np[j++];
      k2 += np[j+1];
    } while (k2 > np[j]);
    j = 1;
 lbl430:
    if (kk < k2)         goto lbl400;
    kk += jc;
    k2 += kspan;
    if (k2 < ks)      goto lbl430;
    if (kk < ks)      goto lbl420;
    jc = k3;
 lbl440:
    if ((2*(*kt))+1 >= m)
      return;

    kspnn = *(np + *(kt) + 1);
    j = m - *kt;
    nfac[j+1] = 1;
 lbl450:
    nfac[j] = nfac[j] * nfac[j+1];
    j--;
    if (j != *kt) goto lbl450;
    *kt = *(kt) + 1;
    nn = nfac[*kt] - 1;
    jj = 0;
    j = 0;
    goto lbl480;
 lbl460:
    jj -= k2;
    k2 = kk;
    kk = nfac[++k];
 lbl470:
    jj += kk;
    if (jj >= k2) goto lbl460;
    np[j] = jj;
 lbl480:
    k2 = nfac[*kt];
    k = *kt + 1;
    kk = nfac[k];
    j++;
    if (j <= nn) goto lbl470;
/* Determine permutation cycles of length greater than 1 */
    j = 0;
    goto lbl500;
 lbl490:
    k = kk;
    kk = np[k];
    np[k] = -kk;
    if (kk != j) goto lbl490;
    k3 = kk;
 lbl500:
    kk = np[++j];
    if (kk < 0)  goto lbl500;
    if (kk != j) goto lbl490;
    np[j] = -j;
    if (j != nn) goto lbl500;
    maxf *= inc;
    /* Perform reordering following permutation cycles */
    goto lbl570;
 lbl510:
    j--;
    if (np[j] < 0) goto lbl510;
    jj = jc;
 lbl520:
    kspan = jj;
    if (jj > maxf)
      kspan = maxf;
    jj -= kspan;
    k = np[j];
    kk = (jc*k) + i + jj;
    k1 = kk + kspan;
    k2 = 0;
 lbl530:
    k2++;
    at[k2] = a[k1];
    bt[k2] = b[k1];
    k1 -= inc;
    if (k1 != kk) goto lbl530;
 lbl540:
    k1 = kk + kspan;
    k2 = k1 - (jc * (k + np[k]));
    k = -(np[k]);
 lbl550:
    a[k1] = a[k2];
    b[k1] = b[k2];
    k1 -= inc;
    k2 -= inc;
    if (k1 != kk) goto lbl550;
    kk = k2;
    if (k != j)   goto lbl540;
    k1 = kk + kspan;
    k2 = 0;
 lbl560:
    k2++;
    a[k1] = at[k2];
    b[k1] = bt[k2];
    k1 -= inc;
    if (k1 != kk) goto lbl560;
    if (jj)       goto lbl520;
    if (j  != 1)  goto lbl510;
lbl570:
    j = k3 + 1;
    nt -= kspnn;
    i = nt - inc + 1;
    if (nt >= 0)  goto lbl510;
    return;
}

/*
 *-----------------------------------------------------------------------
 * subroutine:
 * reals
 * used with 'fft' to compute fourier transform or inverse for real data
 *-----------------------------------------------------------------------
 *      this is the call from C:
 *
 *              reals_(anal,banal,N2,mtwo);
 *      which has been changed from CARL call
 *              reals_(csound,anal,banal,&N2,&mtwo);
 */

static void reals_(CSOUND *csound, MYFLT *a, MYFLT *b, int32_t n, int32_t isn)

  /*    *a,       a refers to an array of floats 'anal'   */
  /*    *b;       b refers to an array of floats 'banal'  */
  /* See IEEE book for a long comment here on usage */

{
    IGN(csound);
    int32_t inc,
      j,
      k,
      lim,
      mm,ml,
      nf,nk,nh;

    double      aa,ab,
      ba,bb,
      cd,cn,
      dr,
      em,
      rad,re,
      sd,sn;
    double      xx;     /******* ADDED APRIL 1991 ******/
    /* adjust  input array pointers (called from C) */
    a--;        b--;
    inc = abs(isn);
    nf = abs(n);
    nk = (nf*inc) + 2;
    nh = nk/2;
/*****************************
        rad  = atan((double)1.0);
******************************/
    rad = 0.785398163397448278900;
    dr = -4.0/(double)(nf);
/********************************** POW2 REMOVED APRIL 1991 *****************
                                cd = 2.0 * (pow2(sin((double)0.5 * dr * rad)));
*****************************************************************************/
    xx = sin((double)0.5 * dr * rad);
    cd = 2.0 * xx * xx;
    sd = sin(dr * rad);
/*
 * sin,cos values are re-initialised each lim steps
 */
    lim = 32;
    mm = lim;
    ml = 0;
    sn = 0.0;
    if (isn<0) {
      cn = 1.0;
      a[nk-1] = a[1];
      b[nk-1] = b[1]; }
    else {
      cn = -1.0;
      sd = -sd;
    }
    for (j=1;j<=nh;j+=inc)      {
      k = nk - j;
      aa = a[j] + a[k];
      ab = a[j] - a[k];
      ba = b[j] + b[k];
      bb = b[j] - b[k];
      re = (cn*ba) + (sn*ab);
      em = (sn*ba) - (cn*ab);
      b[k] = (MYFLT)((em-bb)*0.5);
      b[j] = (MYFLT)((em+bb)*0.5);
      a[k] = (MYFLT)((aa-re)*0.5);
      a[j] = (MYFLT)((aa+re)*0.5);
      ml++;
      if (ml!=mm) {
        aa = cn - ((cd*cn)+(sd*sn));
        sn = ((sd*cn) - (cd*sn)) + sn;
        cn = aa;}
      else {
        mm +=lim;
        sn = ((MYFLT)ml) * dr * rad;
        cn = cos(sn);
        if (isn>0)
          cn = -cn;
        sn = sin(sn);
      }
    }
    return;
}

/**
 * Compute in-place real FFT, allowing non power of two FFT sizes.
 *
 * buf:     array of FFTsize + 2 MYFLT values; output is in interleaved
 *          real/imaginary format (note: the real part of the Nyquist
 *          frequency is stored in buf[FFTsize], and not in buf[1]).
 * FFTsize: FFT length in samples; not required to be an integer power of two,
 *          but should be even and not have too many factors.
 */
void csoundRealFFTnp2(CSOUND *csound, MYFLT *buf, int32_t FFTsize)
{
    if (!(FFTsize & (FFTsize - 1))) {
      /* if FFT size is power of two: */
      csound->RealFFT(csound, buf, FFTsize);
      buf[FFTsize] = buf[1];
    }
    else {
      if (UNLIKELY(FFTsize < 2 || (FFTsize & 1))) {
        csound->Warning(csound,
                        Str("csoundRealFFTnp2(): invalid FFT size, %d"), FFTsize);
        return;
      }
      buf[FFTsize] = buf[FFTsize + 1] = FL(0.0);
      fft_(csound, buf, &(buf[1]), 1, (FFTsize >> 1), 1, -2);
      reals_(csound, buf, &(buf[1]), (FFTsize >> 1), -2);
    }
    buf[1] = buf[FFTsize + 1] = FL(0.0);
}

/**
 * Compute in-place inverse real FFT, allowing non power of two FFT sizes.
 * The output does not need to be scaled.
 *
 * buf:     array of FFTsize + 2 MYFLT values, in interleaved real/imaginary
 *          format (note: the real part of the Nyquist frequency is stored
 *          in buf[FFTsize], and not in buf[1]).
 * FFTsize: FFT length in samples; not required to be an integer power of two,
 *          but should be even and not have too many factors.
 */
void csoundInverseRealFFTnp2(CSOUND *csound, MYFLT *buf, int32_t FFTsize)
{
  if (UNLIKELY(FFTsize < 2 || (FFTsize & 1))){
      csound->Warning(csound, Str("csoundInverseRealFFTnp2(): invalid FFT size"));
      return;
  }
    buf[1] = buf[FFTsize + 1] = FL(0.0);
    reals_(csound, buf, &(buf[1]), (FFTsize >> 1), 2);
    fft_(csound, buf, &(buf[1]), 1, (FFTsize >> 1), 1, 2);
    buf[FFTsize] = buf[FFTsize + 1] = FL(0.0);
}

void csoundInverseComplexFFTnp2(CSOUND *csound, MYFLT *buf, int32_t FFTsize)
{
  if (UNLIKELY(FFTsize < 2 || (FFTsize & 1))){
      csound->Warning(csound, Str("csoundInverseRealFFTnp2(): invalid FFT size"));
      return;
  }
    fft_(csound, buf, buf, 1, FFTsize, 1, 2);
}

void csoundComplexFFTnp2(CSOUND *csound, MYFLT *buf, int32_t FFTsize)
{
      if (UNLIKELY(FFTsize < 2 || (FFTsize & 1))) {
        csound->Warning(csound, Str("csoundRealFFTnp2(): invalid FFT size"));
        return;
      }
      fft_(csound, buf, buf, 1, FFTsize, 1, -2);
}
