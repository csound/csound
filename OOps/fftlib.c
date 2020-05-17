/*
  fftlib.c:

  Copyright 2005 John Green Istvan Varga Victor Lazzarini

  FFT library
  based on public domain code by John Green <green_jt@vsdec.npt.nuwc.navy.mil>
  original version is available at
  http://hyperarchive.lcs.mit.edu/
  /HyperArchive/Archive/dev/src/ffts-for-risc-2-c.hqx
  ported to Csound by Istvan Varga, 2005

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

#include <stdlib.h>
#include <math.h>
#include "csoundCore.h"
#include "csound.h"
#include "fftlib.h"
#include "pffft.h"



#define POW2(m) ((uint32) (1 << (m)))       /* integer power of 2 for m<32 */

/* fft's with M bigger than this bust primary cache */
#define MCACHE  (11 - (sizeof(MYFLT) / 8))

/* some math constants to 40 decimal places */
//#define MYPI      3.141592653589793238462643383279502884197   /* pi         */
//#define MYROOT2   1.414213562373095048801688724209698078569   /* sqrt(2)    */
#define MYCOSPID8 0.9238795325112867561281831893967882868224  /* cos(pi/8)  */
#define MYSINPID8 0.3826834323650897717284599840303988667613  /* sin(pi/8)  */


/*****************************************************
 * routines to initialize tables used by fft routines *
 *****************************************************/

static void fftCosInit(int32_t M, MYFLT *Utbl)
{
  /* Compute Utbl, the cosine table for ffts  */
  /* of size (pow(2,M)/4 +1)                  */
  /* INPUTS                                   */
  /*   M = log2 of fft size                   */
  /* OUTPUTS                                  */
  /*   *Utbl = cosine table                   */
  uint32_t fftN = POW2(M);
  uint32_t i1;

  Utbl[0] = FL(1.0);
  for (i1 = 1; i1 < fftN/4; i1++)
    Utbl[i1] = COS((FL(2.0) * PI_F * (MYFLT)i1) / (MYFLT)fftN);
  Utbl[fftN/4] = FL(0.0);
}

static void fftBRInit(int32_t M, int16 *BRLow)
{
  /* Compute BRLow, the bit reversed table for ffts */
  /* of size pow(2,M/2 -1)                          */
  /* INPUTS                                         */
  /*   M = log2 of fft size                         */
  /* OUTPUTS                                        */
  /*   *BRLow = bit reversed counter table          */
  int32_t Mroot_1 = M / 2 - 1;
  int32_t Nroot_1 = POW2(Mroot_1);
  int32_t i1;
  int32_t bitsum;
  int32_t bitmask;
  int32_t bit;

  for (i1 = 0; i1 < Nroot_1; i1++) {
    bitsum = 0;
    bitmask = 1;
    for (bit = 1; bit <= Mroot_1; bitmask <<= 1, bit++)
      if (i1 & bitmask)
        bitsum = bitsum + (Nroot_1 >> bit);
    BRLow[i1] = bitsum;
  }
}

/*****************
 * parts of ffts1 *
 *****************/

static void bitrevR2(MYFLT *ioptr, int32_t M, int16 *BRLow)
{
  /*** bit reverse and first radix 2 stage of forward or inverse fft ***/
  MYFLT f0r;
  MYFLT f0i;
  MYFLT f1r;
  MYFLT f1i;
  MYFLT f2r;
  MYFLT f2i;
  MYFLT f3r;
  MYFLT f3i;
  MYFLT f4r;
  MYFLT f4i;
  MYFLT f5r;
  MYFLT f5i;
  MYFLT f6r;
  MYFLT f6i;
  MYFLT f7r;
  MYFLT f7i;
  MYFLT t0r;
  MYFLT t0i;
  MYFLT t1r;
  MYFLT t1i;
  MYFLT *p0r;
  MYFLT *p1r;
  MYFLT *IOP;
  MYFLT *iolimit;
  int32_t Colstart;
  int32_t iCol;
  uint32_t posA;
  uint32_t posAi;
  uint32_t posB;
  uint32_t posBi;

  const uint32_t Nrems2 = POW2((M + 3) / 2);
  const uint32_t Nroot_1_ColInc = POW2(M) - Nrems2;
  const uint32_t Nroot_1 = POW2(M / 2 - 1) - 1;
  const uint32_t ColstartShift = (M + 1) / 2 + 1;

  posA = POW2(M);               /* 1/2 of POW2(M) complexes */
  posAi = posA + 1;
  posB = posA + 2;
  posBi = posB + 1;

  iolimit = ioptr + Nrems2;
  for (; ioptr < iolimit; ioptr += POW2(M / 2 + 1)) {
    for (Colstart = Nroot_1; Colstart >= 0; Colstart--) {
      iCol = Nroot_1;
      p0r = ioptr + Nroot_1_ColInc + BRLow[Colstart] * 4;
      IOP = ioptr + (Colstart << ColstartShift);
      p1r = IOP + BRLow[iCol] * 4;
      f0r = *(p0r);
      f0i = *(p0r + 1);
      f1r = *(p0r + posA);
      f1i = *(p0r + posAi);
      for (; iCol > Colstart;) {
        f2r = *(p0r + 2);
        f2i = *(p0r + (2 + 1));
        f3r = *(p0r + posB);
        f3i = *(p0r + posBi);
        f4r = *(p1r);
        f4i = *(p1r + 1);
        f5r = *(p1r + posA);
        f5i = *(p1r + posAi);
        f6r = *(p1r + 2);
        f6i = *(p1r + (2 + 1));
        f7r = *(p1r + posB);
        f7i = *(p1r + posBi);

        t0r = f0r + f1r;
        t0i = f0i + f1i;
        f1r = f0r - f1r;
        f1i = f0i - f1i;
        t1r = f2r + f3r;
        t1i = f2i + f3i;
        f3r = f2r - f3r;
        f3i = f2i - f3i;
        f0r = f4r + f5r;
        f0i = f4i + f5i;
        f5r = f4r - f5r;
        f5i = f4i - f5i;
        f2r = f6r + f7r;
        f2i = f6i + f7i;
        f7r = f6r - f7r;
        f7i = f6i - f7i;

        *(p1r) = t0r;
        *(p1r + 1) = t0i;
        *(p1r + 2) = f1r;
        *(p1r + (2 + 1)) = f1i;
        *(p1r + posA) = t1r;
        *(p1r + posAi) = t1i;
        *(p1r + posB) = f3r;
        *(p1r + posBi) = f3i;
        *(p0r) = f0r;
        *(p0r + 1) = f0i;
        *(p0r + 2) = f5r;
        *(p0r + (2 + 1)) = f5i;
        *(p0r + posA) = f2r;
        *(p0r + posAi) = f2i;
        *(p0r + posB) = f7r;
        *(p0r + posBi) = f7i;

        p0r -= Nrems2;
        f0r = *(p0r);
        f0i = *(p0r + 1);
        f1r = *(p0r + posA);
        f1i = *(p0r + posAi);
        iCol -= 1;
        p1r = IOP + BRLow[iCol] * 4;
      }
      f2r = *(p0r + 2);
      f2i = *(p0r + (2 + 1));
      f3r = *(p0r + posB);
      f3i = *(p0r + posBi);

      t0r = f0r + f1r;
      t0i = f0i + f1i;
      f1r = f0r - f1r;
      f1i = f0i - f1i;
      t1r = f2r + f3r;
      t1i = f2i + f3i;
      f3r = f2r - f3r;
      f3i = f2i - f3i;

      *(p0r) = t0r;
      *(p0r + 1) = t0i;
      *(p0r + 2) = f1r;
      *(p0r + (2 + 1)) = f1i;
      *(p0r + posA) = t1r;
      *(p0r + posAi) = t1i;
      *(p0r + posB) = f3r;
      *(p0r + posBi) = f3i;
    }
  }
}

static void fft2pt(MYFLT *ioptr)
{
  /***   RADIX 2 fft      ***/
  MYFLT f0r, f0i, f1r, f1i;
  MYFLT t0r, t0i;

  /* bit reversed load */
  f0r = ioptr[0];
  f0i = ioptr[1];
  f1r = ioptr[2];
  f1i = ioptr[3];

  /* Butterflys           */
  /*
    f0   -       -       t0
    f1   -  1 -  f1
  */

  t0r = f0r + f1r;
  t0i = f0i + f1i;
  f1r = f0r - f1r;
  f1i = f0i - f1i;

  /* store result */
  ioptr[0] = t0r;
  ioptr[1] = t0i;
  ioptr[2] = f1r;
  ioptr[3] = f1i;
}

static void fft4pt(MYFLT *ioptr)
{
  /***   RADIX 4 fft      ***/
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT t0r, t0i, t1r, t1i;

  /* bit reversed load */
  f0r = ioptr[0];
  f0i = ioptr[1];
  f1r = ioptr[4];
  f1i = ioptr[5];
  f2r = ioptr[2];
  f2i = ioptr[3];
  f3r = ioptr[6];
  f3i = ioptr[7];

  /* Butterflys           */
  /*
    f0   -       -       t0      -       -       f0
    f1   -  1 -  f1      -       -       f1
    f2   -       -       f2      -  1 -  f2
    f3   -  1 -  t1      - -i -  f3
  */

  t0r = f0r + f1r;
  t0i = f0i + f1i;
  f1r = f0r - f1r;
  f1i = f0i - f1i;

  t1r = f2r - f3r;
  t1i = f2i - f3i;
  f2r = f2r + f3r;
  f2i = f2i + f3i;

  f0r = t0r + f2r;
  f0i = t0i + f2i;
  f2r = t0r - f2r;
  f2i = t0i - f2i;

  f3r = f1r - t1i;
  f3i = f1i + t1r;
  f1r = f1r + t1i;
  f1i = f1i - t1r;

  /* store result */
  ioptr[0] = f0r;
  ioptr[1] = f0i;
  ioptr[2] = f1r;
  ioptr[3] = f1i;
  ioptr[4] = f2r;
  ioptr[5] = f2i;
  ioptr[6] = f3r;
  ioptr[7] = f3i;
}

static void fft8pt(MYFLT *ioptr)
{
  /***   RADIX 8 fft      ***/
  MYFLT w0r = (MYFLT)(1.0 / ROOT2);    /* cos(pi/4)   */
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT f4r, f4i, f5r, f5i, f6r, f6i, f7r, f7i;
  MYFLT t0r, t0i, t1r, t1i;
  const MYFLT Two = FL(2.0);

  /* bit reversed load */
  f0r = ioptr[0];
  f0i = ioptr[1];
  f1r = ioptr[8];
  f1i = ioptr[9];
  f2r = ioptr[4];
  f2i = ioptr[5];
  f3r = ioptr[12];
  f3i = ioptr[13];
  f4r = ioptr[2];
  f4i = ioptr[3];
  f5r = ioptr[10];
  f5i = ioptr[11];
  f6r = ioptr[6];
  f6i = ioptr[7];
  f7r = ioptr[14];
  f7i = ioptr[15];
  /* Butterflys           */
  /*
    f0   -       -       t0      -       -       f0      -       -       f0
    f1   -  1 -  f1      -       -       f1      -       -       f1
    f2   -       -       f2      -  1 -  f2      -       -       f2
    f3   -  1 -  t1      - -i -  f3      -       -       f3
    f4   -       -       t0      -       -       f4      -  1 -  t0
    f5   -  1 -  f5      -       -       f5      - w3 -  f4
    f6   -       -       f6      -  1 -  f6      - -i -  t1
    f7   -  1 -  t1      - -i -  f7      - iw3-  f6
  */

  t0r = f0r + f1r;
  t0i = f0i + f1i;
  f1r = f0r - f1r;
  f1i = f0i - f1i;

  t1r = f2r - f3r;
  t1i = f2i - f3i;
  f2r = f2r + f3r;
  f2i = f2i + f3i;

  f0r = t0r + f2r;
  f0i = t0i + f2i;
  f2r = t0r - f2r;
  f2i = t0i - f2i;

  f3r = f1r - t1i;
  f3i = f1i + t1r;
  f1r = f1r + t1i;
  f1i = f1i - t1r;

  t0r = f4r + f5r;
  t0i = f4i + f5i;
  f5r = f4r - f5r;
  f5i = f4i - f5i;

  t1r = f6r - f7r;
  t1i = f6i - f7i;
  f6r = f6r + f7r;
  f6i = f6i + f7i;

  f4r = t0r + f6r;
  f4i = t0i + f6i;
  f6r = t0r - f6r;
  f6i = t0i - f6i;

  f7r = f5r - t1i;
  f7i = f5i + t1r;
  f5r = f5r + t1i;
  f5i = f5i - t1r;

  t0r = f0r - f4r;
  t0i = f0i - f4i;
  f0r = f0r + f4r;
  f0i = f0i + f4i;

  t1r = f2r - f6i;
  t1i = f2i + f6r;
  f2r = f2r + f6i;
  f2i = f2i - f6r;

  f4r = f1r - f5r * w0r - f5i * w0r;
  f4i = f1i + f5r * w0r - f5i * w0r;
  f1r = f1r * Two - f4r;
  f1i = f1i * Two - f4i;

  f6r = f3r + f7r * w0r - f7i * w0r;
  f6i = f3i + f7r * w0r + f7i * w0r;
  f3r = f3r * Two - f6r;
  f3i = f3i * Two - f6i;

  /* store result */
  ioptr[0] = f0r;
  ioptr[1] = f0i;
  ioptr[2] = f1r;
  ioptr[3] = f1i;
  ioptr[4] = f2r;
  ioptr[5] = f2i;
  ioptr[6] = f3r;
  ioptr[7] = f3i;
  ioptr[8] = t0r;
  ioptr[9] = t0i;
  ioptr[10] = f4r;
  ioptr[11] = f4i;
  ioptr[12] = t1r;
  ioptr[13] = t1i;
  ioptr[14] = f6r;
  ioptr[15] = f6i;
}

static void bfR2(MYFLT *ioptr, int32_t M, int32_t NDiffU)
{
  /*** 2nd radix 2 stage ***/
  uint32_t pos;
  uint32_t posi;
  uint32_t pinc;
  uint32_t pnext;
  uint32_t NSameU;
  uint32_t SameUCnt;

  MYFLT *pstrt;
  MYFLT *p0r, *p1r, *p2r, *p3r;

  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT f4r, f4i, f5r, f5i, f6r, f6i, f7r, f7i;

  pinc = NDiffU * 2;            /* 2 floats per complex */
  pnext = pinc * 4;
  pos = 2;
  posi = pos + 1;
  NSameU = POW2(M) / 4 / NDiffU;        /* 4 Us at a time */
  pstrt = ioptr;
  p0r = pstrt;
  p1r = pstrt + pinc;
  p2r = p1r + pinc;
  p3r = p2r + pinc;

  /* Butterflys           */
  /*
    f0   -       -       f4
    f1   -  1 -  f5
    f2   -       -       f6
    f3   -  1 -  f7
  */
  /* Butterflys           */
  /*
    f0   -       -       f4
    f1   -  1 -  f5
    f2   -       -       f6
    f3   -  1 -  f7
  */

  for (SameUCnt = NSameU; SameUCnt > 0; SameUCnt--) {

    f0r = *p0r;
    f1r = *p1r;
    f0i = *(p0r + 1);
    f1i = *(p1r + 1);
    f2r = *p2r;
    f3r = *p3r;
    f2i = *(p2r + 1);
    f3i = *(p3r + 1);

    f4r = f0r + f1r;
    f4i = f0i + f1i;
    f5r = f0r - f1r;
    f5i = f0i - f1i;

    f6r = f2r + f3r;
    f6i = f2i + f3i;
    f7r = f2r - f3r;
    f7i = f2i - f3i;

    *p0r = f4r;
    *(p0r + 1) = f4i;
    *p1r = f5r;
    *(p1r + 1) = f5i;
    *p2r = f6r;
    *(p2r + 1) = f6i;
    *p3r = f7r;
    *(p3r + 1) = f7i;

    f0r = *(p0r + pos);
    f1i = *(p1r + posi);
    f0i = *(p0r + posi);
    f1r = *(p1r + pos);
    f2r = *(p2r + pos);
    f3i = *(p3r + posi);
    f2i = *(p2r + posi);
    f3r = *(p3r + pos);

    f4r = f0r + f1i;
    f4i = f0i - f1r;
    f5r = f0r - f1i;
    f5i = f0i + f1r;

    f6r = f2r + f3i;
    f6i = f2i - f3r;
    f7r = f2r - f3i;
    f7i = f2i + f3r;

    *(p0r + pos) = f4r;
    *(p0r + posi) = f4i;
    *(p1r + pos) = f5r;
    *(p1r + posi) = f5i;
    *(p2r + pos) = f6r;
    *(p2r + posi) = f6i;
    *(p3r + pos) = f7r;
    *(p3r + posi) = f7i;

    p0r += pnext;
    p1r += pnext;
    p2r += pnext;
    p3r += pnext;
  }
}

static void bfR4(MYFLT *ioptr, int32_t M, int32_t NDiffU)
{
  /*** 1 radix 4 stage ***/
  uint32_t pos;
  uint32_t posi;
  uint32_t pinc;
  uint32_t pnext;
  uint32_t pnexti;
  uint32_t NSameU;
  uint32_t SameUCnt;

  MYFLT *pstrt;
  MYFLT *p0r, *p1r, *p2r, *p3r;

  MYFLT w1r = FL(1.0) / FL(ROOT2);    /* cos(pi/4)   */
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT f4r, f4i, f5r, f5i, f6r, f6i, f7r, f7i;
  MYFLT t1r, t1i;
  const MYFLT Two = FL(2.0);

  pinc = NDiffU * 2;            /* 2 floats per complex */
  pnext = pinc * 4;
  pnexti = pnext + 1;
  pos = 2;
  posi = pos + 1;
  NSameU = POW2(M) / 4 / NDiffU;        /* 4 pts per butterfly */
  pstrt = ioptr;
  p0r = pstrt;
  p1r = pstrt + pinc;
  p2r = p1r + pinc;
  p3r = p2r + pinc;

  /* Butterflys           */
  /*
    f0   -       -       f0      -       -       f4
    f1   -  1 -  f5      -       -       f5
    f2   -       -       f6      -  1 -  f6
    f3   -  1 -  f3      - -i -  f7
  */
  /* Butterflys           */
  /*
    f0   -       -       f4      -       -       f4
    f1   - -i -  t1      -       -       f5
    f2   -       -       f2      - w1 -  f6
    f3   - -i -  f7      - iw1-  f7
  */

  f0r = *p0r;
  f1r = *p1r;
  f2r = *p2r;
  f3r = *p3r;
  f0i = *(p0r + 1);
  f1i = *(p1r + 1);
  f2i = *(p2r + 1);
  f3i = *(p3r + 1);

  f5r = f0r - f1r;
  f5i = f0i - f1i;
  f0r = f0r + f1r;
  f0i = f0i + f1i;

  f6r = f2r + f3r;
  f6i = f2i + f3i;
  f3r = f2r - f3r;
  f3i = f2i - f3i;

  for (SameUCnt = NSameU - 1; SameUCnt > 0; SameUCnt--) {

    f7r = f5r - f3i;
    f7i = f5i + f3r;
    f5r = f5r + f3i;
    f5i = f5i - f3r;

    f4r = f0r + f6r;
    f4i = f0i + f6i;
    f6r = f0r - f6r;
    f6i = f0i - f6i;

    f2r = *(p2r + pos);
    f2i = *(p2r + posi);
    f1r = *(p1r + pos);
    f1i = *(p1r + posi);
    f3i = *(p3r + posi);
    f0r = *(p0r + pos);
    f3r = *(p3r + pos);
    f0i = *(p0r + posi);

    *p3r = f7r;
    *p0r = f4r;
    *(p3r + 1) = f7i;
    *(p0r + 1) = f4i;
    *p1r = f5r;
    *p2r = f6r;
    *(p1r + 1) = f5i;
    *(p2r + 1) = f6i;

    f7r = f2r - f3i;
    f7i = f2i + f3r;
    f2r = f2r + f3i;
    f2i = f2i - f3r;

    f4r = f0r + f1i;
    f4i = f0i - f1r;
    t1r = f0r - f1i;
    t1i = f0i + f1r;

    f5r = t1r - f7r * w1r + f7i * w1r;
    f5i = t1i - f7r * w1r - f7i * w1r;
    f7r = t1r * Two - f5r;
    f7i = t1i * Two - f5i;

    f6r = f4r - f2r * w1r - f2i * w1r;
    f6i = f4i + f2r * w1r - f2i * w1r;
    f4r = f4r * Two - f6r;
    f4i = f4i * Two - f6i;

    f3r = *(p3r + pnext);
    f0r = *(p0r + pnext);
    f3i = *(p3r + pnexti);
    f0i = *(p0r + pnexti);
    f2r = *(p2r + pnext);
    f2i = *(p2r + pnexti);
    f1r = *(p1r + pnext);
    f1i = *(p1r + pnexti);

    *(p2r + pos) = f6r;
    *(p1r + pos) = f5r;
    *(p2r + posi) = f6i;
    *(p1r + posi) = f5i;
    *(p3r + pos) = f7r;
    *(p0r + pos) = f4r;
    *(p3r + posi) = f7i;
    *(p0r + posi) = f4i;

    f6r = f2r + f3r;
    f6i = f2i + f3i;
    f3r = f2r - f3r;
    f3i = f2i - f3i;

    f5r = f0r - f1r;
    f5i = f0i - f1i;
    f0r = f0r + f1r;
    f0i = f0i + f1i;

    p3r += pnext;
    p0r += pnext;
    p1r += pnext;
    p2r += pnext;
  }
  f7r = f5r - f3i;
  f7i = f5i + f3r;
  f5r = f5r + f3i;
  f5i = f5i - f3r;

  f4r = f0r + f6r;
  f4i = f0i + f6i;
  f6r = f0r - f6r;
  f6i = f0i - f6i;

  f2r = *(p2r + pos);
  f2i = *(p2r + posi);
  f1r = *(p1r + pos);
  f1i = *(p1r + posi);
  f3i = *(p3r + posi);
  f0r = *(p0r + pos);
  f3r = *(p3r + pos);
  f0i = *(p0r + posi);

  *p3r = f7r;
  *p0r = f4r;
  *(p3r + 1) = f7i;
  *(p0r + 1) = f4i;
  *p1r = f5r;
  *p2r = f6r;
  *(p1r + 1) = f5i;
  *(p2r + 1) = f6i;

  f7r = f2r - f3i;
  f7i = f2i + f3r;
  f2r = f2r + f3i;
  f2i = f2i - f3r;

  f4r = f0r + f1i;
  f4i = f0i - f1r;
  t1r = f0r - f1i;
  t1i = f0i + f1r;

  f5r = t1r - f7r * w1r + f7i * w1r;
  f5i = t1i - f7r * w1r - f7i * w1r;
  f7r = t1r * Two - f5r;
  f7i = t1i * Two - f5i;

  f6r = f4r - f2r * w1r - f2i * w1r;
  f6i = f4i + f2r * w1r - f2i * w1r;
  f4r = f4r * Two - f6r;
  f4i = f4i * Two - f6i;

  *(p2r + pos) = f6r;
  *(p1r + pos) = f5r;
  *(p2r + posi) = f6i;
  *(p1r + posi) = f5i;
  *(p3r + pos) = f7r;
  *(p0r + pos) = f4r;
  *(p3r + posi) = f7i;
  *(p0r + posi) = f4i;
}

static void bfstages(MYFLT *ioptr, int32_t M, MYFLT *Utbl, int32_t Ustride,
                     int32_t NDiffU, int32_t StageCnt)
{
  /***   RADIX 8 Stages   ***/
  uint32_t pos;
  uint32_t posi;
  uint32_t pinc;
  uint32_t pnext;
  uint32_t NSameU;
  int32_t          Uinc;
  int32_t          Uinc2;
  int32_t          Uinc4;
  uint32_t DiffUCnt;
  uint32_t SameUCnt;
  uint32_t U2toU3;

  MYFLT *pstrt;
  MYFLT *p0r, *p1r, *p2r, *p3r;
  MYFLT *u0r, *u0i, *u1r, *u1i, *u2r, *u2i;

  MYFLT w0r, w0i, w1r, w1i, w2r, w2i, w3r, w3i;
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT f4r, f4i, f5r, f5i, f6r, f6i, f7r, f7i;
  MYFLT t0r, t0i, t1r, t1i;
  const MYFLT Two = FL(2.0);

  pinc = NDiffU * 2;            /* 2 floats per complex */
  pnext = pinc * 8;
  pos = pinc * 4;
  posi = pos + 1;
  NSameU = POW2(M) / 8 / NDiffU;        /* 8 pts per butterfly */
  Uinc = (int32_t) NSameU * Ustride;
  Uinc2 = Uinc * 2;
  Uinc4 = Uinc * 4;
  U2toU3 = (POW2(M) / 8) * Ustride;
  for (; StageCnt > 0; StageCnt--) {

    u0r = &Utbl[0];
    u0i = &Utbl[POW2(M - 2) * Ustride];
    u1r = u0r;
    u1i = u0i;
    u2r = u0r;
    u2i = u0i;

    w0r = *u0r;
    w0i = *u0i;
    w1r = *u1r;
    w1i = *u1i;
    w2r = *u2r;
    w2i = *u2i;
    w3r = *(u2r + U2toU3);
    w3i = *(u2i - U2toU3);

    pstrt = ioptr;

    p0r = pstrt;
    p1r = pstrt + pinc;
    p2r = p1r + pinc;
    p3r = p2r + pinc;

    /* Butterflys           */
    /*
      f0   -       -       t0      -       -       f0      -       -       f0
      f1   - w0-   f1      -       -       f1      -       -       f1
      f2   -       -       f2      - w1-   f2      -       -       f4
      f3   - w0-   t1      - iw1-  f3      -       -       f5

      f4   -       -       t0      -       -       f4      - w2-   t0
      f5   - w0-   f5      -       -       f5      - w3-   t1
      f6   -       -       f6      - w1-   f6      - iw2-  f6
      f7   - w0-   t1      - iw1-  f7      - iw3-  f7
    */

    for (DiffUCnt = NDiffU; DiffUCnt > 0; DiffUCnt--) {
      f0r = *p0r;
      f0i = *(p0r + 1);
      f1r = *p1r;
      f1i = *(p1r + 1);
      for (SameUCnt = NSameU - 1; SameUCnt > 0; SameUCnt--) {
        f2r = *p2r;
        f2i = *(p2r + 1);
        f3r = *p3r;
        f3i = *(p3r + 1);

        t0r = f0r + f1r * w0r + f1i * w0i;
        t0i = f0i - f1r * w0i + f1i * w0r;
        f1r = f0r * Two - t0r;
        f1i = f0i * Two - t0i;

        f4r = *(p0r + pos);
        f4i = *(p0r + posi);
        f5r = *(p1r + pos);
        f5i = *(p1r + posi);

        f6r = *(p2r + pos);
        f6i = *(p2r + posi);
        f7r = *(p3r + pos);
        f7i = *(p3r + posi);

        t1r = f2r - f3r * w0r - f3i * w0i;
        t1i = f2i + f3r * w0i - f3i * w0r;
        f2r = f2r * Two - t1r;
        f2i = f2i * Two - t1i;

        f0r = t0r + f2r * w1r + f2i * w1i;
        f0i = t0i - f2r * w1i + f2i * w1r;
        f2r = t0r * Two - f0r;
        f2i = t0i * Two - f0i;

        f3r = f1r + t1r * w1i - t1i * w1r;
        f3i = f1i + t1r * w1r + t1i * w1i;
        f1r = f1r * Two - f3r;
        f1i = f1i * Two - f3i;

        t0r = f4r + f5r * w0r + f5i * w0i;
        t0i = f4i - f5r * w0i + f5i * w0r;
        f5r = f4r * Two - t0r;
        f5i = f4i * Two - t0i;

        t1r = f6r - f7r * w0r - f7i * w0i;
        t1i = f6i + f7r * w0i - f7i * w0r;
        f6r = f6r * Two - t1r;
        f6i = f6i * Two - t1i;

        f4r = t0r + f6r * w1r + f6i * w1i;
        f4i = t0i - f6r * w1i + f6i * w1r;
        f6r = t0r * Two - f4r;
        f6i = t0i * Two - f4i;

        f7r = f5r + t1r * w1i - t1i * w1r;
        f7i = f5i + t1r * w1r + t1i * w1i;
        f5r = f5r * Two - f7r;
        f5i = f5i * Two - f7i;

        t0r = f0r - f4r * w2r - f4i * w2i;
        t0i = f0i + f4r * w2i - f4i * w2r;
        f0r = f0r * Two - t0r;
        f0i = f0i * Two - t0i;

        t1r = f1r - f5r * w3r - f5i * w3i;
        t1i = f1i + f5r * w3i - f5i * w3r;
        f1r = f1r * Two - t1r;
        f1i = f1i * Two - t1i;

        *(p0r + pos) = t0r;
        *(p1r + pos) = t1r;
        *(p0r + posi) = t0i;
        *(p1r + posi) = t1i;
        *p0r = f0r;
        *p1r = f1r;
        *(p0r + 1) = f0i;
        *(p1r + 1) = f1i;

        p0r += pnext;
        f0r = *p0r;
        f0i = *(p0r + 1);

        p1r += pnext;

        f1r = *p1r;
        f1i = *(p1r + 1);

        f4r = f2r - f6r * w2i + f6i * w2r;
        f4i = f2i - f6r * w2r - f6i * w2i;
        f6r = f2r * Two - f4r;
        f6i = f2i * Two - f4i;

        f5r = f3r - f7r * w3i + f7i * w3r;
        f5i = f3i - f7r * w3r - f7i * w3i;
        f7r = f3r * Two - f5r;
        f7i = f3i * Two - f5i;

        *p2r = f4r;
        *p3r = f5r;
        *(p2r + 1) = f4i;
        *(p3r + 1) = f5i;
        *(p2r + pos) = f6r;
        *(p3r + pos) = f7r;
        *(p2r + posi) = f6i;
        *(p3r + posi) = f7i;

        p2r += pnext;
        p3r += pnext;
      }

      f2r = *p2r;
      f2i = *(p2r + 1);
      f3r = *p3r;
      f3i = *(p3r + 1);

      t0r = f0r + f1r * w0r + f1i * w0i;
      t0i = f0i - f1r * w0i + f1i * w0r;
      f1r = f0r * Two - t0r;
      f1i = f0i * Two - t0i;

      f4r = *(p0r + pos);
      f4i = *(p0r + posi);
      f5r = *(p1r + pos);
      f5i = *(p1r + posi);

      f6r = *(p2r + pos);
      f6i = *(p2r + posi);
      f7r = *(p3r + pos);
      f7i = *(p3r + posi);

      t1r = f2r - f3r * w0r - f3i * w0i;
      t1i = f2i + f3r * w0i - f3i * w0r;
      f2r = f2r * Two - t1r;
      f2i = f2i * Two - t1i;

      f0r = t0r + f2r * w1r + f2i * w1i;
      f0i = t0i - f2r * w1i + f2i * w1r;
      f2r = t0r * Two - f0r;
      f2i = t0i * Two - f0i;

      f3r = f1r + t1r * w1i - t1i * w1r;
      f3i = f1i + t1r * w1r + t1i * w1i;
      f1r = f1r * Two - f3r;
      f1i = f1i * Two - f3i;

      if ((int32_t) DiffUCnt == NDiffU / 2)
        Uinc4 = -Uinc4;

      u0r += Uinc4;
      u0i -= Uinc4;
      u1r += Uinc2;
      u1i -= Uinc2;
      u2r += Uinc;
      u2i -= Uinc;

      pstrt += 2;

      t0r = f4r + f5r * w0r + f5i * w0i;
      t0i = f4i - f5r * w0i + f5i * w0r;
      f5r = f4r * Two - t0r;
      f5i = f4i * Two - t0i;

      t1r = f6r - f7r * w0r - f7i * w0i;
      t1i = f6i + f7r * w0i - f7i * w0r;
      f6r = f6r * Two - t1r;
      f6i = f6i * Two - t1i;

      f4r = t0r + f6r * w1r + f6i * w1i;
      f4i = t0i - f6r * w1i + f6i * w1r;
      f6r = t0r * Two - f4r;
      f6i = t0i * Two - f4i;

      f7r = f5r + t1r * w1i - t1i * w1r;
      f7i = f5i + t1r * w1r + t1i * w1i;
      f5r = f5r * Two - f7r;
      f5i = f5i * Two - f7i;

      w0r = *u0r;
      w0i = *u0i;
      w1r = *u1r;
      w1i = *u1i;

      if ((int32_t) DiffUCnt <= NDiffU / 2)
        w0r = -w0r;

      t0r = f0r - f4r * w2r - f4i * w2i;
      t0i = f0i + f4r * w2i - f4i * w2r;
      f0r = f0r * Two - t0r;
      f0i = f0i * Two - t0i;

      f4r = f2r - f6r * w2i + f6i * w2r;
      f4i = f2i - f6r * w2r - f6i * w2i;
      f6r = f2r * Two - f4r;
      f6i = f2i * Two - f4i;

      *(p0r + pos) = t0r;
      *p2r = f4r;
      *(p0r + posi) = t0i;
      *(p2r + 1) = f4i;
      w2r = *u2r;
      w2i = *u2i;
      *p0r = f0r;
      *(p2r + pos) = f6r;
      *(p0r + 1) = f0i;
      *(p2r + posi) = f6i;

      p0r = pstrt;
      p2r = pstrt + pinc + pinc;

      t1r = f1r - f5r * w3r - f5i * w3i;
      t1i = f1i + f5r * w3i - f5i * w3r;
      f1r = f1r * Two - t1r;
      f1i = f1i * Two - t1i;

      f5r = f3r - f7r * w3i + f7i * w3r;
      f5i = f3i - f7r * w3r - f7i * w3i;
      f7r = f3r * Two - f5r;
      f7i = f3i * Two - f5i;

      *(p1r + pos) = t1r;
      *p3r = f5r;
      *(p1r + posi) = t1i;
      *(p3r + 1) = f5i;
      w3r = *(u2r + U2toU3);
      w3i = *(u2i - U2toU3);
      *p1r = f1r;
      *(p3r + pos) = f7r;
      *(p1r + 1) = f1i;
      *(p3r + posi) = f7i;

      p1r = pstrt + pinc;
      p3r = p2r + pinc;
    }
    NSameU /= 8;
    Uinc /= 8;
    Uinc2 /= 8;
    Uinc4 = Uinc * 4;
    NDiffU *= 8;
    pinc *= 8;
    pnext *= 8;
    pos *= 8;
    posi = pos + 1;
  }
}

static void fftrecurs(MYFLT *ioptr, int32_t M, MYFLT *Utbl, int32_t Ustride,
                      int32_t NDiffU, int32_t StageCnt)
{
  /* recursive bfstages calls to maximize on chip cache efficiency */
  int32_t i1;

  if (M <= (int32_t) MCACHE)              /* fits on chip ? */
    bfstages(ioptr, M, Utbl, Ustride, NDiffU, StageCnt); /* RADIX 8 Stages */
  else {
    for (i1 = 0; i1 < 8; i1++) {
      fftrecurs(&ioptr[i1 * POW2(M - 3) * 2], M - 3, Utbl, 8 * Ustride,
                NDiffU, StageCnt - 1);  /*  RADIX 8 Stages      */
    }
    bfstages(ioptr, M, Utbl, Ustride, POW2(M - 3), 1);  /*  RADIX 8 Stage */
  }
}

static void ffts1(MYFLT *ioptr, int32_t M, MYFLT *Utbl, int16 *BRLow)
{
  /* Compute in-place complex fft on the rows of the input array  */
  /* INPUTS                                                       */
  /*   *ioptr = input data array                                  */
  /*   M = log2 of fft size (ex M=10 for 1024 point fft)          */
  /*   *Utbl = cosine table                                       */
  /*   *BRLow = bit reversed counter table                        */
  /* OUTPUTS                                                      */
  /*   *ioptr = output data array                                 */

  int32_t StageCnt;
  int32_t NDiffU;

  switch (M) {
  case 0:
    break;
  case 1:
    fft2pt(ioptr);            /* a 2 pt fft */
    break;
  case 2:
    fft4pt(ioptr);            /* a 4 pt fft */
    break;
  case 3:
    fft8pt(ioptr);            /* an 8 pt fft */
    break;
  default:
    bitrevR2(ioptr, M, BRLow);  /* bit reverse and first radix 2 stage */
    StageCnt = (M - 1) / 3;     /* number of radix 8 stages           */
    NDiffU = 2;                 /* one radix 2 stage already complete */
    if ((M - 1 - (StageCnt * 3)) == 1) {
      bfR2(ioptr, M, NDiffU); /* 1 radix 2 stage */
      NDiffU *= 2;
    }
    if ((M - 1 - (StageCnt * 3)) == 2) {
      bfR4(ioptr, M, NDiffU); /* 1 radix 4 stage */
      NDiffU *= 4;
    }
    if (M <= (int32_t) MCACHE)
      bfstages(ioptr, M, Utbl, 1, NDiffU, StageCnt);  /* RADIX 8 Stages */
    else
      fftrecurs(ioptr, M, Utbl, 1, NDiffU, StageCnt); /* RADIX 8 Stages */
  }
}

/******************
 * parts of iffts1 *
 ******************/

static void scbitrevR2(MYFLT *ioptr, int32_t M, int16 *BRLow, MYFLT scale)
{
  /*** scaled bit reverse and first radix 2 stage forward or inverse fft ***/
  MYFLT f0r;
  MYFLT f0i;
  MYFLT f1r;
  MYFLT f1i;
  MYFLT f2r;
  MYFLT f2i;
  MYFLT f3r;
  MYFLT f3i;
  MYFLT f4r;
  MYFLT f4i;
  MYFLT f5r;
  MYFLT f5i;
  MYFLT f6r;
  MYFLT f6i;
  MYFLT f7r;
  MYFLT f7i;
  MYFLT t0r;
  MYFLT t0i;
  MYFLT t1r;
  MYFLT t1i;
  MYFLT *p0r;
  MYFLT *p1r;
  MYFLT *IOP;
  MYFLT *iolimit;
  int32_t Colstart;
  int32_t iCol;
  uint32_t posA;
  uint32_t posAi;
  uint32_t posB;
  uint32_t posBi;

  const uint32_t Nrems2 = POW2((M + 3) / 2);
  const uint32_t Nroot_1_ColInc = POW2(M) - Nrems2;
  const uint32_t Nroot_1 = POW2(M / 2 - 1) - 1;
  const uint32_t ColstartShift = (M + 1) / 2 + 1;

  posA = POW2(M);               /* 1/2 of POW2(M) complexes */
  posAi = posA + 1;
  posB = posA + 2;
  posBi = posB + 1;

  iolimit = ioptr + Nrems2;
  for (; ioptr < iolimit; ioptr += POW2(M / 2 + 1)) {
    for (Colstart = Nroot_1; Colstart >= 0; Colstart--) {
      iCol = Nroot_1;
      p0r = ioptr + Nroot_1_ColInc + BRLow[Colstart] * 4;
      IOP = ioptr + (Colstart << ColstartShift);
      p1r = IOP + BRLow[iCol] * 4;
      f0r = *(p0r);
      f0i = *(p0r + 1);
      f1r = *(p0r + posA);
      f1i = *(p0r + posAi);
      for (; iCol > Colstart;) {
        f2r = *(p0r + 2);
        f2i = *(p0r + (2 + 1));
        f3r = *(p0r + posB);
        f3i = *(p0r + posBi);
        f4r = *(p1r);
        f4i = *(p1r + 1);
        f5r = *(p1r + posA);
        f5i = *(p1r + posAi);
        f6r = *(p1r + 2);
        f6i = *(p1r + (2 + 1));
        f7r = *(p1r + posB);
        f7i = *(p1r + posBi);

        t0r = f0r + f1r;
        t0i = f0i + f1i;
        f1r = f0r - f1r;
        f1i = f0i - f1i;
        t1r = f2r + f3r;
        t1i = f2i + f3i;
        f3r = f2r - f3r;
        f3i = f2i - f3i;
        f0r = f4r + f5r;
        f0i = f4i + f5i;
        f5r = f4r - f5r;
        f5i = f4i - f5i;
        f2r = f6r + f7r;
        f2i = f6i + f7i;
        f7r = f6r - f7r;
        f7i = f6i - f7i;

        *(p1r) = scale * t0r;
        *(p1r + 1) = scale * t0i;
        *(p1r + 2) = scale * f1r;
        *(p1r + (2 + 1)) = scale * f1i;
        *(p1r + posA) = scale * t1r;
        *(p1r + posAi) = scale * t1i;
        *(p1r + posB) = scale * f3r;
        *(p1r + posBi) = scale * f3i;
        *(p0r) = scale * f0r;
        *(p0r + 1) = scale * f0i;
        *(p0r + 2) = scale * f5r;
        *(p0r + (2 + 1)) = scale * f5i;
        *(p0r + posA) = scale * f2r;
        *(p0r + posAi) = scale * f2i;
        *(p0r + posB) = scale * f7r;
        *(p0r + posBi) = scale * f7i;

        p0r -= Nrems2;
        f0r = *(p0r);
        f0i = *(p0r + 1);
        f1r = *(p0r + posA);
        f1i = *(p0r + posAi);
        iCol -= 1;
        p1r = IOP + BRLow[iCol] * 4;
      }
      f2r = *(p0r + 2);
      f2i = *(p0r + (2 + 1));
      f3r = *(p0r + posB);
      f3i = *(p0r + posBi);

      t0r = f0r + f1r;
      t0i = f0i + f1i;
      f1r = f0r - f1r;
      f1i = f0i - f1i;
      t1r = f2r + f3r;
      t1i = f2i + f3i;
      f3r = f2r - f3r;
      f3i = f2i - f3i;

      *(p0r) = scale * t0r;
      *(p0r + 1) = scale * t0i;
      *(p0r + 2) = scale * f1r;
      *(p0r + (2 + 1)) = scale * f1i;
      *(p0r + posA) = scale * t1r;
      *(p0r + posAi) = scale * t1i;
      *(p0r + posB) = scale * f3r;
      *(p0r + posBi) = scale * f3i;
    }
  }
}

static void ifft2pt(MYFLT *ioptr, MYFLT scale)
{
  /***   RADIX 2 ifft     ***/
  MYFLT f0r, f0i, f1r, f1i;
  MYFLT t0r, t0i;

  /* bit reversed load */
  f0r = ioptr[0];
  f0i = ioptr[1];
  f1r = ioptr[2];
  f1i = ioptr[3];

  /* Butterflys           */
  /*
    f0   -       -       t0
    f1   -  1 -  f1
  */

  t0r = f0r + f1r;
  t0i = f0i + f1i;
  f1r = f0r - f1r;
  f1i = f0i - f1i;

  /* store result */
  ioptr[0] = scale * t0r;
  ioptr[1] = scale * t0i;
  ioptr[2] = scale * f1r;
  ioptr[3] = scale * f1i;
}

static void ifft4pt(MYFLT *ioptr, MYFLT scale)
{
  /***   RADIX 4 ifft     ***/
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT t0r, t0i, t1r, t1i;

  /* bit reversed load */
  f0r = ioptr[0];
  f0i = ioptr[1];
  f1r = ioptr[4];
  f1i = ioptr[5];
  f2r = ioptr[2];
  f2i = ioptr[3];
  f3r = ioptr[6];
  f3i = ioptr[7];

  /* Butterflys           */
  /*
    f0   -       -       t0      -       -       f0
    f1   -  1 -  f1      -       -       f1
    f2   -       -       f2      -  1 -  f2
    f3   -  1 -  t1      -  i -  f3
  */

  t0r = f0r + f1r;
  t0i = f0i + f1i;
  f1r = f0r - f1r;
  f1i = f0i - f1i;

  t1r = f2r - f3r;
  t1i = f2i - f3i;
  f2r = f2r + f3r;
  f2i = f2i + f3i;

  f0r = t0r + f2r;
  f0i = t0i + f2i;
  f2r = t0r - f2r;
  f2i = t0i - f2i;

  f3r = f1r + t1i;
  f3i = f1i - t1r;
  f1r = f1r - t1i;
  f1i = f1i + t1r;

  /* store result */
  ioptr[0] = scale * f0r;
  ioptr[1] = scale * f0i;
  ioptr[2] = scale * f1r;
  ioptr[3] = scale * f1i;
  ioptr[4] = scale * f2r;
  ioptr[5] = scale * f2i;
  ioptr[6] = scale * f3r;
  ioptr[7] = scale * f3i;
}

static void ifft8pt(MYFLT *ioptr, MYFLT scale)
{
  /***   RADIX 8 ifft     ***/
  MYFLT w0r = FL(1.0) / FL(ROOT2);    /* cos(pi/4)   */
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT f4r, f4i, f5r, f5i, f6r, f6i, f7r, f7i;
  MYFLT t0r, t0i, t1r, t1i;
  const MYFLT Two = FL(2.0);

  /* bit reversed load */
  f0r = ioptr[0];
  f0i = ioptr[1];
  f1r = ioptr[8];
  f1i = ioptr[9];
  f2r = ioptr[4];
  f2i = ioptr[5];
  f3r = ioptr[12];
  f3i = ioptr[13];
  f4r = ioptr[2];
  f4i = ioptr[3];
  f5r = ioptr[10];
  f5i = ioptr[11];
  f6r = ioptr[6];
  f6i = ioptr[7];
  f7r = ioptr[14];
  f7i = ioptr[15];

  /* Butterflys           */
  /*
    f0   -       -       t0      -       -       f0      -       -       f0
    f1   -  1 -  f1      -       -       f1      -       -       f1
    f2   -       -       f2      -  1 -  f2      -       -       f2
    f3   -  1 -  t1      -  i -  f3      -       -       f3
    f4   -       -       t0      -       -       f4      -  1 -  t0
    f5   -  1 -  f5      -       -       f5      - w3 -  f4
    f6   -       -       f6      -  1 -  f6      -  i -  t1
    f7   -  1 -  t1      -  i -  f7      - iw3-  f6
  */

  t0r = f0r + f1r;
  t0i = f0i + f1i;
  f1r = f0r - f1r;
  f1i = f0i - f1i;

  t1r = f2r - f3r;
  t1i = f2i - f3i;
  f2r = f2r + f3r;
  f2i = f2i + f3i;

  f0r = t0r + f2r;
  f0i = t0i + f2i;
  f2r = t0r - f2r;
  f2i = t0i - f2i;

  f3r = f1r + t1i;
  f3i = f1i - t1r;
  f1r = f1r - t1i;
  f1i = f1i + t1r;

  t0r = f4r + f5r;
  t0i = f4i + f5i;
  f5r = f4r - f5r;
  f5i = f4i - f5i;

  t1r = f6r - f7r;
  t1i = f6i - f7i;
  f6r = f6r + f7r;
  f6i = f6i + f7i;

  f4r = t0r + f6r;
  f4i = t0i + f6i;
  f6r = t0r - f6r;
  f6i = t0i - f6i;

  f7r = f5r + t1i;
  f7i = f5i - t1r;
  f5r = f5r - t1i;
  f5i = f5i + t1r;

  t0r = f0r - f4r;
  t0i = f0i - f4i;
  f0r = f0r + f4r;
  f0i = f0i + f4i;

  t1r = f2r + f6i;
  t1i = f2i - f6r;
  f2r = f2r - f6i;
  f2i = f2i + f6r;

  f4r = f1r - f5r * w0r + f5i * w0r;
  f4i = f1i - f5r * w0r - f5i * w0r;
  f1r = f1r * Two - f4r;
  f1i = f1i * Two - f4i;

  f6r = f3r + f7r * w0r + f7i * w0r;
  f6i = f3i - f7r * w0r + f7i * w0r;
  f3r = f3r * Two - f6r;
  f3i = f3i * Two - f6i;

  /* store result */
  ioptr[0] = scale * f0r;
  ioptr[1] = scale * f0i;
  ioptr[2] = scale * f1r;
  ioptr[3] = scale * f1i;
  ioptr[4] = scale * f2r;
  ioptr[5] = scale * f2i;
  ioptr[6] = scale * f3r;
  ioptr[7] = scale * f3i;
  ioptr[8] = scale * t0r;
  ioptr[9] = scale * t0i;
  ioptr[10] = scale * f4r;
  ioptr[11] = scale * f4i;
  ioptr[12] = scale * t1r;
  ioptr[13] = scale * t1i;
  ioptr[14] = scale * f6r;
  ioptr[15] = scale * f6i;
}

static void ibfR2(MYFLT *ioptr, int32_t M, int32_t NDiffU)
{
  /*** 2nd radix 2 stage ***/
  uint32_t pos;
  uint32_t posi;
  uint32_t pinc;
  uint32_t pnext;
  uint32_t NSameU;
  uint32_t SameUCnt;

  MYFLT *pstrt;
  MYFLT *p0r, *p1r, *p2r, *p3r;

  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT f4r, f4i, f5r, f5i, f6r, f6i, f7r, f7i;

  pinc = NDiffU * 2;            /* 2 floats per complex */
  pnext = pinc * 4;
  pos = 2;
  posi = pos + 1;
  NSameU = POW2(M) / 4 / NDiffU;        /* 4 Us at a time */
  pstrt = ioptr;
  p0r = pstrt;
  p1r = pstrt + pinc;
  p2r = p1r + pinc;
  p3r = p2r + pinc;

  /* Butterflys           */
  /*
    f0   -       -       f4
    f1   -  1 -  f5
    f2   -       -       f6
    f3   -  1 -  f7
  */
  /* Butterflys           */
  /*
    f0   -       -       f4
    f1   -  1 -  f5
    f2   -       -       f6
    f3   -  1 -  f7
  */

  for (SameUCnt = NSameU; SameUCnt > 0; SameUCnt--) {

    f0r = *p0r;
    f1r = *p1r;
    f0i = *(p0r + 1);
    f1i = *(p1r + 1);
    f2r = *p2r;
    f3r = *p3r;
    f2i = *(p2r + 1);
    f3i = *(p3r + 1);

    f4r = f0r + f1r;
    f4i = f0i + f1i;
    f5r = f0r - f1r;
    f5i = f0i - f1i;

    f6r = f2r + f3r;
    f6i = f2i + f3i;
    f7r = f2r - f3r;
    f7i = f2i - f3i;

    *p0r = f4r;
    *(p0r + 1) = f4i;
    *p1r = f5r;
    *(p1r + 1) = f5i;
    *p2r = f6r;
    *(p2r + 1) = f6i;
    *p3r = f7r;
    *(p3r + 1) = f7i;

    f0r = *(p0r + pos);
    f1i = *(p1r + posi);
    f0i = *(p0r + posi);
    f1r = *(p1r + pos);
    f2r = *(p2r + pos);
    f3i = *(p3r + posi);
    f2i = *(p2r + posi);
    f3r = *(p3r + pos);

    f4r = f0r - f1i;
    f4i = f0i + f1r;
    f5r = f0r + f1i;
    f5i = f0i - f1r;

    f6r = f2r - f3i;
    f6i = f2i + f3r;
    f7r = f2r + f3i;
    f7i = f2i - f3r;

    *(p0r + pos) = f4r;
    *(p0r + posi) = f4i;
    *(p1r + pos) = f5r;
    *(p1r + posi) = f5i;
    *(p2r + pos) = f6r;
    *(p2r + posi) = f6i;
    *(p3r + pos) = f7r;
    *(p3r + posi) = f7i;

    p0r += pnext;
    p1r += pnext;
    p2r += pnext;
    p3r += pnext;
  }
}

static void ibfR4(MYFLT *ioptr, int32_t M, int32_t NDiffU)
{
  /*** 1 radix 4 stage ***/
  uint32_t pos;
  uint32_t posi;
  uint32_t pinc;
  uint32_t pnext;
  uint32_t pnexti;
  uint32_t NSameU;
  uint32_t SameUCnt;

  MYFLT *pstrt;
  MYFLT *p0r, *p1r, *p2r, *p3r;

  MYFLT w1r = FL(1.0) / FL(ROOT2);    /* cos(pi/4)   */
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT f4r, f4i, f5r, f5i, f6r, f6i, f7r, f7i;
  MYFLT t1r, t1i;
  const MYFLT Two = FL(2.0);

  pinc = NDiffU * 2;            /* 2 floats per complex */
  pnext = pinc * 4;
  pnexti = pnext + 1;
  pos = 2;
  posi = pos + 1;
  NSameU = POW2(M) / 4 / NDiffU;        /* 4 pts per butterfly */
  pstrt = ioptr;
  p0r = pstrt;
  p1r = pstrt + pinc;
  p2r = p1r + pinc;
  p3r = p2r + pinc;

  /* Butterflys           */
  /*
    f0   -       -       f0      -       -       f4
    f1   -  1 -  f5      -       -       f5
    f2   -       -       f6      -  1 -  f6
    f3   -  1 -  f3      - -i -  f7
  */
  /* Butterflys           */
  /*
    f0   -       -       f4      -       -       f4
    f1   - -i -  t1      -       -       f5
    f2   -       -       f2      - w1 -  f6
    f3   - -i -  f7      - iw1-  f7
  */

  f0r = *p0r;
  f1r = *p1r;
  f2r = *p2r;
  f3r = *p3r;
  f0i = *(p0r + 1);
  f1i = *(p1r + 1);
  f2i = *(p2r + 1);
  f3i = *(p3r + 1);

  f5r = f0r - f1r;
  f5i = f0i - f1i;
  f0r = f0r + f1r;
  f0i = f0i + f1i;

  f6r = f2r + f3r;
  f6i = f2i + f3i;
  f3r = f2r - f3r;
  f3i = f2i - f3i;

  for (SameUCnt = NSameU - 1; SameUCnt > 0; SameUCnt--) {

    f7r = f5r + f3i;
    f7i = f5i - f3r;
    f5r = f5r - f3i;
    f5i = f5i + f3r;

    f4r = f0r + f6r;
    f4i = f0i + f6i;
    f6r = f0r - f6r;
    f6i = f0i - f6i;

    f2r = *(p2r + pos);
    f2i = *(p2r + posi);
    f1r = *(p1r + pos);
    f1i = *(p1r + posi);
    f3i = *(p3r + posi);
    f0r = *(p0r + pos);
    f3r = *(p3r + pos);
    f0i = *(p0r + posi);

    *p3r = f7r;
    *p0r = f4r;
    *(p3r + 1) = f7i;
    *(p0r + 1) = f4i;
    *p1r = f5r;
    *p2r = f6r;
    *(p1r + 1) = f5i;
    *(p2r + 1) = f6i;

    f7r = f2r + f3i;
    f7i = f2i - f3r;
    f2r = f2r - f3i;
    f2i = f2i + f3r;

    f4r = f0r - f1i;
    f4i = f0i + f1r;
    t1r = f0r + f1i;
    t1i = f0i - f1r;

    f5r = t1r - f7r * w1r - f7i * w1r;
    f5i = t1i + f7r * w1r - f7i * w1r;
    f7r = t1r * Two - f5r;
    f7i = t1i * Two - f5i;

    f6r = f4r - f2r * w1r + f2i * w1r;
    f6i = f4i - f2r * w1r - f2i * w1r;
    f4r = f4r * Two - f6r;
    f4i = f4i * Two - f6i;

    f3r = *(p3r + pnext);
    f0r = *(p0r + pnext);
    f3i = *(p3r + pnexti);
    f0i = *(p0r + pnexti);
    f2r = *(p2r + pnext);
    f2i = *(p2r + pnexti);
    f1r = *(p1r + pnext);
    f1i = *(p1r + pnexti);

    *(p2r + pos) = f6r;
    *(p1r + pos) = f5r;
    *(p2r + posi) = f6i;
    *(p1r + posi) = f5i;
    *(p3r + pos) = f7r;
    *(p0r + pos) = f4r;
    *(p3r + posi) = f7i;
    *(p0r + posi) = f4i;

    f6r = f2r + f3r;
    f6i = f2i + f3i;
    f3r = f2r - f3r;
    f3i = f2i - f3i;

    f5r = f0r - f1r;
    f5i = f0i - f1i;
    f0r = f0r + f1r;
    f0i = f0i + f1i;

    p3r += pnext;
    p0r += pnext;
    p1r += pnext;
    p2r += pnext;
  }
  f7r = f5r + f3i;
  f7i = f5i - f3r;
  f5r = f5r - f3i;
  f5i = f5i + f3r;

  f4r = f0r + f6r;
  f4i = f0i + f6i;
  f6r = f0r - f6r;
  f6i = f0i - f6i;

  f2r = *(p2r + pos);
  f2i = *(p2r + posi);
  f1r = *(p1r + pos);
  f1i = *(p1r + posi);
  f3i = *(p3r + posi);
  f0r = *(p0r + pos);
  f3r = *(p3r + pos);
  f0i = *(p0r + posi);

  *p3r = f7r;
  *p0r = f4r;
  *(p3r + 1) = f7i;
  *(p0r + 1) = f4i;
  *p1r = f5r;
  *p2r = f6r;
  *(p1r + 1) = f5i;
  *(p2r + 1) = f6i;

  f7r = f2r + f3i;
  f7i = f2i - f3r;
  f2r = f2r - f3i;
  f2i = f2i + f3r;

  f4r = f0r - f1i;
  f4i = f0i + f1r;
  t1r = f0r + f1i;
  t1i = f0i - f1r;

  f5r = t1r - f7r * w1r - f7i * w1r;
  f5i = t1i + f7r * w1r - f7i * w1r;
  f7r = t1r * Two - f5r;
  f7i = t1i * Two - f5i;

  f6r = f4r - f2r * w1r + f2i * w1r;
  f6i = f4i - f2r * w1r - f2i * w1r;
  f4r = f4r * Two - f6r;
  f4i = f4i * Two - f6i;

  *(p2r + pos) = f6r;
  *(p1r + pos) = f5r;
  *(p2r + posi) = f6i;
  *(p1r + posi) = f5i;
  *(p3r + pos) = f7r;
  *(p0r + pos) = f4r;
  *(p3r + posi) = f7i;
  *(p0r + posi) = f4i;
}

static void ibfstages(MYFLT *ioptr, int32_t M, MYFLT *Utbl, int32_t Ustride,
                      int32_t NDiffU, int32_t StageCnt)
{
  /***   RADIX 8 Stages   ***/
  uint32_t pos;
  uint32_t posi;
  uint32_t pinc;
  uint32_t pnext;
  uint32_t NSameU;
  int32_t          Uinc;
  int32_t          Uinc2;
  int32_t          Uinc4;
  uint32_t DiffUCnt;
  uint32_t SameUCnt;
  uint32_t U2toU3;

  MYFLT *pstrt;
  MYFLT *p0r, *p1r, *p2r, *p3r;
  MYFLT *u0r, *u0i, *u1r, *u1i, *u2r, *u2i;

  MYFLT w0r, w0i, w1r, w1i, w2r, w2i, w3r, w3i;
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT f4r, f4i, f5r, f5i, f6r, f6i, f7r, f7i;
  MYFLT t0r, t0i, t1r, t1i;
  const MYFLT Two = FL(2.0);

  pinc = NDiffU * 2;            /* 2 floats per complex */
  pnext = pinc * 8;
  pos = pinc * 4;
  posi = pos + 1;
  NSameU = POW2(M) / 8 / NDiffU;        /* 8 pts per butterfly */
  Uinc = (int32_t) NSameU * Ustride;
  Uinc2 = Uinc * 2;
  Uinc4 = Uinc * 4;
  U2toU3 = (POW2(M) / 8) * Ustride;
  for (; StageCnt > 0; StageCnt--) {

    u0r = &Utbl[0];
    u0i = &Utbl[POW2(M - 2) * Ustride];
    u1r = u0r;
    u1i = u0i;
    u2r = u0r;
    u2i = u0i;

    w0r = *u0r;
    w0i = *u0i;
    w1r = *u1r;
    w1i = *u1i;
    w2r = *u2r;
    w2i = *u2i;
    w3r = *(u2r + U2toU3);
    w3i = *(u2i - U2toU3);

    pstrt = ioptr;

    p0r = pstrt;
    p1r = pstrt + pinc;
    p2r = p1r + pinc;
    p3r = p2r + pinc;

    /* Butterflys           */
    /*
      f0   -       -       t0      -       -       f0      -       -       f0
      f1   - w0-   f1      -       -       f1      -       -       f1
      f2   -       -       f2      - w1-   f2      -       -       f4
      f3   - w0-   t1      - iw1-  f3      -       -       f5

      f4   -       -       t0      -       -       f4      - w2-   t0
      f5   - w0-   f5      -       -       f5      - w3-   t1
      f6   -       -       f6      - w1-   f6      - iw2-  f6
      f7   - w0-   t1      - iw1-  f7      - iw3-  f7
    */

    for (DiffUCnt = NDiffU; DiffUCnt > 0; DiffUCnt--) {
      f0r = *p0r;
      f0i = *(p0r + 1);
      f1r = *p1r;
      f1i = *(p1r + 1);
      for (SameUCnt = NSameU - 1; SameUCnt > 0; SameUCnt--) {
        f2r = *p2r;
        f2i = *(p2r + 1);
        f3r = *p3r;
        f3i = *(p3r + 1);

        t0r = f0r + f1r * w0r - f1i * w0i;
        t0i = f0i + f1r * w0i + f1i * w0r;
        f1r = f0r * Two - t0r;
        f1i = f0i * Two - t0i;

        f4r = *(p0r + pos);
        f4i = *(p0r + posi);
        f5r = *(p1r + pos);
        f5i = *(p1r + posi);

        f6r = *(p2r + pos);
        f6i = *(p2r + posi);
        f7r = *(p3r + pos);
        f7i = *(p3r + posi);

        t1r = f2r - f3r * w0r + f3i * w0i;
        t1i = f2i - f3r * w0i - f3i * w0r;
        f2r = f2r * Two - t1r;
        f2i = f2i * Two - t1i;

        f0r = t0r + f2r * w1r - f2i * w1i;
        f0i = t0i + f2r * w1i + f2i * w1r;
        f2r = t0r * Two - f0r;
        f2i = t0i * Two - f0i;

        f3r = f1r + t1r * w1i + t1i * w1r;
        f3i = f1i - t1r * w1r + t1i * w1i;
        f1r = f1r * Two - f3r;
        f1i = f1i * Two - f3i;

        t0r = f4r + f5r * w0r - f5i * w0i;
        t0i = f4i + f5r * w0i + f5i * w0r;
        f5r = f4r * Two - t0r;
        f5i = f4i * Two - t0i;

        t1r = f6r - f7r * w0r + f7i * w0i;
        t1i = f6i - f7r * w0i - f7i * w0r;
        f6r = f6r * Two - t1r;
        f6i = f6i * Two - t1i;

        f4r = t0r + f6r * w1r - f6i * w1i;
        f4i = t0i + f6r * w1i + f6i * w1r;
        f6r = t0r * Two - f4r;
        f6i = t0i * Two - f4i;

        f7r = f5r + t1r * w1i + t1i * w1r;
        f7i = f5i - t1r * w1r + t1i * w1i;
        f5r = f5r * Two - f7r;
        f5i = f5i * Two - f7i;

        t0r = f0r - f4r * w2r + f4i * w2i;
        t0i = f0i - f4r * w2i - f4i * w2r;
        f0r = f0r * Two - t0r;
        f0i = f0i * Two - t0i;

        t1r = f1r - f5r * w3r + f5i * w3i;
        t1i = f1i - f5r * w3i - f5i * w3r;
        f1r = f1r * Two - t1r;
        f1i = f1i * Two - t1i;

        *(p0r + pos) = t0r;
        *(p0r + posi) = t0i;
        *p0r = f0r;
        *(p0r + 1) = f0i;

        p0r += pnext;
        f0r = *p0r;
        f0i = *(p0r + 1);

        *(p1r + pos) = t1r;
        *(p1r + posi) = t1i;
        *p1r = f1r;
        *(p1r + 1) = f1i;

        p1r += pnext;

        f1r = *p1r;
        f1i = *(p1r + 1);

        f4r = f2r - f6r * w2i - f6i * w2r;
        f4i = f2i + f6r * w2r - f6i * w2i;
        f6r = f2r * Two - f4r;
        f6i = f2i * Two - f4i;

        f5r = f3r - f7r * w3i - f7i * w3r;
        f5i = f3i + f7r * w3r - f7i * w3i;
        f7r = f3r * Two - f5r;
        f7i = f3i * Two - f5i;

        *p2r = f4r;
        *(p2r + 1) = f4i;
        *(p2r + pos) = f6r;
        *(p2r + posi) = f6i;

        p2r += pnext;

        *p3r = f5r;
        *(p3r + 1) = f5i;
        *(p3r + pos) = f7r;
        *(p3r + posi) = f7i;

        p3r += pnext;
      }

      f2r = *p2r;
      f2i = *(p2r + 1);
      f3r = *p3r;
      f3i = *(p3r + 1);

      t0r = f0r + f1r * w0r - f1i * w0i;
      t0i = f0i + f1r * w0i + f1i * w0r;
      f1r = f0r * Two - t0r;
      f1i = f0i * Two - t0i;

      f4r = *(p0r + pos);
      f4i = *(p0r + posi);
      f5r = *(p1r + pos);
      f5i = *(p1r + posi);

      f6r = *(p2r + pos);
      f6i = *(p2r + posi);
      f7r = *(p3r + pos);
      f7i = *(p3r + posi);

      t1r = f2r - f3r * w0r + f3i * w0i;
      t1i = f2i - f3r * w0i - f3i * w0r;
      f2r = f2r * Two - t1r;
      f2i = f2i * Two - t1i;

      f0r = t0r + f2r * w1r - f2i * w1i;
      f0i = t0i + f2r * w1i + f2i * w1r;
      f2r = t0r * Two - f0r;
      f2i = t0i * Two - f0i;

      f3r = f1r + t1r * w1i + t1i * w1r;
      f3i = f1i - t1r * w1r + t1i * w1i;
      f1r = f1r * Two - f3r;
      f1i = f1i * Two - f3i;

      if ((int32_t) DiffUCnt == NDiffU / 2)
        Uinc4 = -Uinc4;

      u0r += Uinc4;
      u0i -= Uinc4;
      u1r += Uinc2;
      u1i -= Uinc2;
      u2r += Uinc;
      u2i -= Uinc;

      pstrt += 2;

      t0r = f4r + f5r * w0r - f5i * w0i;
      t0i = f4i + f5r * w0i + f5i * w0r;
      f5r = f4r * Two - t0r;
      f5i = f4i * Two - t0i;

      t1r = f6r - f7r * w0r + f7i * w0i;
      t1i = f6i - f7r * w0i - f7i * w0r;
      f6r = f6r * Two - t1r;
      f6i = f6i * Two - t1i;

      f4r = t0r + f6r * w1r - f6i * w1i;
      f4i = t0i + f6r * w1i + f6i * w1r;
      f6r = t0r * Two - f4r;
      f6i = t0i * Two - f4i;

      f7r = f5r + t1r * w1i + t1i * w1r;
      f7i = f5i - t1r * w1r + t1i * w1i;
      f5r = f5r * Two - f7r;
      f5i = f5i * Two - f7i;

      w0r = *u0r;
      w0i = *u0i;
      w1r = *u1r;
      w1i = *u1i;

      if ((int32_t) DiffUCnt <= NDiffU / 2)
        w0r = -w0r;

      t0r = f0r - f4r * w2r + f4i * w2i;
      t0i = f0i - f4r * w2i - f4i * w2r;
      f0r = f0r * Two - t0r;
      f0i = f0i * Two - t0i;

      f4r = f2r - f6r * w2i - f6i * w2r;
      f4i = f2i + f6r * w2r - f6i * w2i;
      f6r = f2r * Two - f4r;
      f6i = f2i * Two - f4i;

      *(p0r + pos) = t0r;
      *p2r = f4r;
      *(p0r + posi) = t0i;
      *(p2r + 1) = f4i;
      w2r = *u2r;
      w2i = *u2i;
      *p0r = f0r;
      *(p2r + pos) = f6r;
      *(p0r + 1) = f0i;
      *(p2r + posi) = f6i;

      p0r = pstrt;
      p2r = pstrt + pinc + pinc;

      t1r = f1r - f5r * w3r + f5i * w3i;
      t1i = f1i - f5r * w3i - f5i * w3r;
      f1r = f1r * Two - t1r;
      f1i = f1i * Two - t1i;

      f5r = f3r - f7r * w3i - f7i * w3r;
      f5i = f3i + f7r * w3r - f7i * w3i;
      f7r = f3r * Two - f5r;
      f7i = f3i * Two - f5i;

      *(p1r + pos) = t1r;
      *p3r = f5r;
      *(p1r + posi) = t1i;
      *(p3r + 1) = f5i;
      w3r = *(u2r + U2toU3);
      w3i = *(u2i - U2toU3);
      *p1r = f1r;
      *(p3r + pos) = f7r;
      *(p1r + 1) = f1i;
      *(p3r + posi) = f7i;

      p1r = pstrt + pinc;
      p3r = p2r + pinc;
    }
    NSameU /= 8;
    Uinc /= 8;
    Uinc2 /= 8;
    Uinc4 = Uinc * 4;
    NDiffU *= 8;
    pinc *= 8;
    pnext *= 8;
    pos *= 8;
    posi = pos + 1;
  }
}

static void ifftrecurs(MYFLT *ioptr, int32_t M, MYFLT *Utbl, int32_t Ustride,
                       int32_t NDiffU, int32_t StageCnt)
{
  /* recursive bfstages calls to maximize on chip cache efficiency */
  int32_t i1;

  if (M <= (int32_t) MCACHE)
    ibfstages(ioptr, M, Utbl, Ustride, NDiffU, StageCnt); /* RADIX 8 Stages */
  else {
    for (i1 = 0; i1 < 8; i1++) {
      ifftrecurs(&ioptr[i1 * POW2(M - 3) * 2], M - 3, Utbl, 8 * Ustride,
                 NDiffU, StageCnt - 1);           /*  RADIX 8 Stages       */
    }
    ibfstages(ioptr, M, Utbl, Ustride, POW2(M - 3), 1);   /* RADIX 8 Stage */
  }
}

static void iffts1(MYFLT *ioptr, int32_t M, MYFLT *Utbl, int16 *BRLow)
{
  /* Compute in-place inverse complex fft on the rows of the input array  */
  /* INPUTS                                                               */
  /*   *ioptr = input data array                                          */
  /*   M = log2 of fft size                                               */
  /*   *Utbl = cosine table                                               */
  /*   *BRLow = bit reversed counter table                                */
  /* OUTPUTS                                                              */
  /*   *ioptr = output data array                                         */

  int32_t StageCnt;
  int32_t NDiffU;
  const MYFLT scale = FL(1.0) / POW2(M);

  switch (M) {
  case 0:
    break;
  case 1:
    ifft2pt(ioptr, scale);    /* a 2 pt fft */
    break;
  case 2:
    ifft4pt(ioptr, scale);    /* a 4 pt fft */
    break;
  case 3:
    ifft8pt(ioptr, scale);    /* an 8 pt fft */
    break;
  default:
    /* bit reverse and first radix 2 stage */
    scbitrevR2(ioptr, M, BRLow, scale);
    StageCnt = (M - 1) / 3;   /* number of radix 8 stages */
    NDiffU = 2;               /* one radix 2 stage already complete */
    if ((M - 1 - (StageCnt * 3)) == 1) {
      ibfR2(ioptr, M, NDiffU);        /* 1 radix 2 stage */
      NDiffU *= 2;
    }
    if ((M - 1 - (StageCnt * 3)) == 2) {
      ibfR4(ioptr, M, NDiffU);        /* 1 radix 4 stage */
      NDiffU *= 4;
    }
    if (M <= (int32_t) MCACHE)
      ibfstages(ioptr, M, Utbl, 1, NDiffU, StageCnt);  /* RADIX 8 Stages */
    else
      ifftrecurs(ioptr, M, Utbl, 1, NDiffU, StageCnt); /* RADIX 8 Stages */
  }
}

/******************
 * parts of rffts1 *
 ******************/

static void rfft1pt(MYFLT *ioptr)
{
  /***   RADIX 2 rfft     ***/
  MYFLT f0r, f0i;
  MYFLT t0r, t0i;

  /* bit reversed load */
  f0r = ioptr[0];
  f0i = ioptr[1];

  /* finish rfft */
  t0r = f0r + f0i;
  t0i = f0r - f0i;

  /* store result */
  ioptr[0] = t0r;
  ioptr[1] = t0i;
}

static void rfft2pt(MYFLT *ioptr)
{
  /***   RADIX 4 rfft     ***/
  MYFLT f0r, f0i, f1r, f1i;
  MYFLT t0r, t0i;

  /* bit reversed load */
  f0r = ioptr[0];
  f0i = ioptr[1];
  f1r = ioptr[2];
  f1i = ioptr[3];

  /* Butterflys           */
  /*
    f0   -       -       t0
    f1   -  1 -  f1
  */

  t0r = f0r + f1r;
  t0i = f0i + f1i;
  f1r = f0r - f1r;
  f1i = f1i - f0i;
  /* finish rfft */
  f0r = t0r + t0i;
  f0i = t0r - t0i;

  /* store result */
  ioptr[0] = f0r;
  ioptr[1] = f0i;
  ioptr[2] = f1r;
  ioptr[3] = f1i;
}

static void rfft4pt(MYFLT *ioptr)
{
  /***   RADIX 8 rfft     ***/
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT t0r, t0i, t1r, t1i;
  MYFLT w0r = 1.0 / ROOT2;    /* cos(pi/4)   */
  const MYFLT Two = FL(2.0);
  const MYFLT scale = FL(0.5);

  /* bit reversed load */
  f0r = ioptr[0];
  f0i = ioptr[1];
  f1r = ioptr[4];
  f1i = ioptr[5];
  f2r = ioptr[2];
  f2i = ioptr[3];
  f3r = ioptr[6];
  f3i = ioptr[7];

  /* Butterflys           */
  /*
    f0   -       -       t0      -       -       f0
    f1   -  1 -  f1      -       -       f1
    f2   -       -       f2      -  1 -  f2
    f3   -  1 -  t1      - -i -  f3
  */

  t0r = f0r + f1r;
  t0i = f0i + f1i;
  f1r = f0r - f1r;
  f1i = f0i - f1i;

  t1r = f2r - f3r;
  t1i = f2i - f3i;
  f2r = f2r + f3r;
  f2i = f2i + f3i;

  f0r = t0r + f2r;
  f0i = t0i + f2i;
  f2r = t0r - f2r;
  f2i = f2i - t0i;              /* neg for rfft */

  f3r = f1r - t1i;
  f3i = f1i + t1r;
  f1r = f1r + t1i;
  f1i = f1i - t1r;

  /* finish rfft */
  t0r = f0r + f0i;              /* compute Re(x[0]) */
  t0i = f0r - f0i;              /* compute Re(x[N/2]) */

  t1r = f1r + f3r;
  t1i = f1i - f3i;
  f0r = f1i + f3i;
  f0i = f3r - f1r;

  f1r = t1r + w0r * f0r + w0r * f0i;
  f1i = t1i - w0r * f0r + w0r * f0i;
  f3r = Two * t1r - f1r;
  f3i = f1i - Two * t1i;

  /* store result */
  ioptr[4] = f2r;
  ioptr[5] = f2i;
  ioptr[0] = t0r;
  ioptr[1] = t0i;

  ioptr[2] = scale * f1r;
  ioptr[3] = scale * f1i;
  ioptr[6] = scale * f3r;
  ioptr[7] = scale * f3i;
}

static void rfft8pt(MYFLT *ioptr)
{
  /***   RADIX 16 rfft    ***/
  MYFLT w0r = 1.0 / ROOT2;    /* cos(pi/4)   */
  MYFLT w1r = MYCOSPID8;        /* cos(pi/8)     */
  MYFLT w1i = MYSINPID8;        /* sin(pi/8)     */
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT f4r, f4i, f5r, f5i, f6r, f6i, f7r, f7i;
  MYFLT t0r, t0i, t1r, t1i;
  const MYFLT Two = FL(2.0);
  const MYFLT scale = FL(0.5);

  /* bit reversed load */
  f0r = ioptr[0];
  f0i = ioptr[1];
  f1r = ioptr[8];
  f1i = ioptr[9];
  f2r = ioptr[4];
  f2i = ioptr[5];
  f3r = ioptr[12];
  f3i = ioptr[13];
  f4r = ioptr[2];
  f4i = ioptr[3];
  f5r = ioptr[10];
  f5i = ioptr[11];
  f6r = ioptr[6];
  f6i = ioptr[7];
  f7r = ioptr[14];
  f7i = ioptr[15];
  /* Butterflys           */
  /*
    f0   -       -       t0      -       -       f0      -       -       f0
    f1   -  1 -  f1      -       -       f1      -       -       f1
    f2   -       -       f2      -  1 -  f2      -       -       f2
    f3   -  1 -  t1      - -i -  f3      -       -       f3
    f4   -       -       t0      -       -       f4      -  1 -  t0
    f5   -  1 -  f5      -       -       f5      - w3 -  f4
    f6   -       -       f6      -  1 -  f6      - -i -  t1
    f7   -  1 -  t1      - -i -  f7      - iw3-  f6
  */

  t0r = f0r + f1r;
  t0i = f0i + f1i;
  f1r = f0r - f1r;
  f1i = f0i - f1i;

  t1r = f2r - f3r;
  t1i = f2i - f3i;
  f2r = f2r + f3r;
  f2i = f2i + f3i;

  f0r = t0r + f2r;
  f0i = t0i + f2i;
  f2r = t0r - f2r;
  f2i = t0i - f2i;

  f3r = f1r - t1i;
  f3i = f1i + t1r;
  f1r = f1r + t1i;
  f1i = f1i - t1r;

  t0r = f4r + f5r;
  t0i = f4i + f5i;
  f5r = f4r - f5r;
  f5i = f4i - f5i;

  t1r = f6r - f7r;
  t1i = f6i - f7i;
  f6r = f6r + f7r;
  f6i = f6i + f7i;

  f4r = t0r + f6r;
  f4i = t0i + f6i;
  f6r = t0r - f6r;
  f6i = t0i - f6i;

  f7r = f5r - t1i;
  f7i = f5i + t1r;
  f5r = f5r + t1i;
  f5i = f5i - t1r;

  t0r = f0r - f4r;
  t0i = f4i - f0i;              /* neg for rfft */
  f0r = f0r + f4r;
  f0i = f0i + f4i;

  t1r = f2r - f6i;
  t1i = f2i + f6r;
  f2r = f2r + f6i;
  f2i = f2i - f6r;

  f4r = f1r - f5r * w0r - f5i * w0r;
  f4i = f1i + f5r * w0r - f5i * w0r;
  f1r = f1r * Two - f4r;
  f1i = f1i * Two - f4i;

  f6r = f3r + f7r * w0r - f7i * w0r;
  f6i = f3i + f7r * w0r + f7i * w0r;
  f3r = f3r * Two - f6r;
  f3i = f3i * Two - f6i;

  /* finish rfft */
  f5r = f0r + f0i;              /* compute Re(x[0]) */
  f5i = f0r - f0i;              /* compute Re(x[N/2]) */

  f0r = f2r + t1r;
  f0i = f2i - t1i;
  f7r = f2i + t1i;
  f7i = t1r - f2r;

  f2r = f0r + w0r * f7r + w0r * f7i;
  f2i = f0i - w0r * f7r + w0r * f7i;
  t1r = Two * f0r - f2r;
  t1i = f2i - Two * f0i;

  f0r = f1r + f6r;
  f0i = f1i - f6i;
  f7r = f1i + f6i;
  f7i = f6r - f1r;

  f1r = f0r + w1r * f7r + w1i * f7i;
  f1i = f0i - w1i * f7r + w1r * f7i;
  f6r = Two * f0r - f1r;
  f6i = f1i - Two * f0i;

  f0r = f3r + f4r;
  f0i = f3i - f4i;
  f7r = f3i + f4i;
  f7i = f4r - f3r;

  f3r = f0r + w1i * f7r + w1r * f7i;
  f3i = f0i - w1r * f7r + w1i * f7i;
  f4r = Two * f0r - f3r;
  f4i = f3i - Two * f0i;

  /* store result */
  ioptr[8] = t0r;
  ioptr[9] = t0i;
  ioptr[0] = f5r;
  ioptr[1] = f5i;

  ioptr[4] = scale * f2r;
  ioptr[5] = scale * f2i;
  ioptr[12] = scale * t1r;
  ioptr[13] = scale * t1i;

  ioptr[2] = scale * f1r;
  ioptr[3] = scale * f1i;
  ioptr[6] = scale * f3r;
  ioptr[7] = scale * f3i;
  ioptr[10] = scale * f4r;
  ioptr[11] = scale * f4i;
  ioptr[14] = scale * f6r;
  ioptr[15] = scale * f6i;
}

static void frstage(MYFLT *ioptr, int32_t M, MYFLT *Utbl)
{
  /*      Finish RFFT             */

  uint32_t pos;
  uint32_t posi;
  uint32_t diffUcnt;

  MYFLT *p0r, *p1r;
  MYFLT *u0r, *u0i;

  MYFLT w0r, w0i;
  MYFLT f0r, f0i, f1r, f1i, f4r, f4i, f5r, f5i;
  MYFLT t0r, t0i, t1r, t1i;
  const MYFLT Two = FL(2.0);

  pos = POW2(M - 1);
  posi = pos + 1;

  p0r = ioptr;
  p1r = ioptr + pos / 2;

  u0r = Utbl + POW2(M - 3);

  w0r = *u0r, f0r = *(p0r);
  f0i = *(p0r + 1);
  f4r = *(p0r + pos);
  f4i = *(p0r + posi);
  f1r = *(p1r);
  f1i = *(p1r + 1);
  f5r = *(p1r + pos);
  f5i = *(p1r + posi);

  t0r = Two * f0r + Two * f0i;  /* compute Re(x[0]) */
  t0i = Two * f0r - Two * f0i;  /* compute Re(x[N/2]) */
  t1r = f4r + f4r;
  t1i = -f4i - f4i;

  f0r = f1r + f5r;
  f0i = f1i - f5i;
  f4r = f1i + f5i;
  f4i = f5r - f1r;

  f1r = f0r + w0r * f4r + w0r * f4i;
  f1i = f0i - w0r * f4r + w0r * f4i;
  f5r = Two * f0r - f1r;
  f5i = f1i - Two * f0i;

  *(p0r) = t0r;
  *(p0r + 1) = t0i;
  *(p0r + pos) = t1r;
  *(p0r + posi) = t1i;
  *(p1r) = f1r;
  *(p1r + 1) = f1i;
  *(p1r + pos) = f5r;
  *(p1r + posi) = f5i;

  u0r = Utbl + 1;
  u0i = Utbl + (POW2(M - 2) - 1);

  w0r = *u0r, w0i = *u0i;

  p0r = (ioptr + 2);
  p1r = (ioptr + (POW2(M - 2) - 1) * 2);

  /* Butterflys */
  /*
    f0   -       t0      -       -       f0
    f5   -       t1      - w0    -       f5

    f1   -       t0      -       -       f1
    f4   -       t1      -iw0 -  f4
  */

  for (diffUcnt = POW2(M - 3) - 1; diffUcnt > 0; diffUcnt--) {

    f0r = *(p0r);
    f0i = *(p0r + 1);
    f5r = *(p1r + pos);
    f5i = *(p1r + posi);
    f1r = *(p1r);
    f1i = *(p1r + 1);
    f4r = *(p0r + pos);
    f4i = *(p0r + posi);

    t0r = f0r + f5r;
    t0i = f0i - f5i;
    t1r = f0i + f5i;
    t1i = f5r - f0r;

    f0r = t0r + w0r * t1r + w0i * t1i;
    f0i = t0i - w0i * t1r + w0r * t1i;
    f5r = Two * t0r - f0r;
    f5i = f0i - Two * t0i;

    t0r = f1r + f4r;
    t0i = f1i - f4i;
    t1r = f1i + f4i;
    t1i = f4r - f1r;

    f1r = t0r + w0i * t1r + w0r * t1i;
    f1i = t0i - w0r * t1r + w0i * t1i;
    f4r = Two * t0r - f1r;
    f4i = f1i - Two * t0i;

    *(p0r) = f0r;
    *(p0r + 1) = f0i;
    *(p1r + pos) = f5r;
    *(p1r + posi) = f5i;

    w0r = *++u0r;
    w0i = *--u0i;

    *(p1r) = f1r;
    *(p1r + 1) = f1i;
    *(p0r + pos) = f4r;
    *(p0r + posi) = f4i;

    p0r += 2;
    p1r -= 2;
  }
}

static void rffts1(MYFLT *ioptr, int32_t M, MYFLT *Utbl, int16 *BRLow)
{
  /* Compute in-place real fft on the rows of the input array           */
  /* The result is the complex spectra of the positive frequencies      */
  /* except the location for the first complex number contains the real */
  /* values for DC and Nyquest                                          */
  /* INPUTS                                                             */
  /*   *ioptr = real input data array                                   */
  /*   M = log2 of fft size                                             */
  /*   *Utbl = cosine table                                             */
  /*   *BRLow = bit reversed counter table                              */
  /* OUTPUTS                                                            */
  /*   *ioptr = output data array   in the following order              */
  /*     Re(x[0]), Re(x[N/2]), Re(x[1]), Im(x[1]), Re(x[2]), Im(x[2]),  */
  /*     ... Re(x[N/2-1]), Im(x[N/2-1]).                                */

  MYFLT scale;
  int32_t StageCnt;
  int32_t NDiffU;

  M = M - 1;
  switch (M) {
  case -1:
    break;
  case 0:
    rfft1pt(ioptr);           /* a 2 pt fft */
    break;
  case 1:
    rfft2pt(ioptr);           /* a 4 pt fft */
    break;
  case 2:
    rfft4pt(ioptr);           /* an 8 pt fft */
    break;
  case 3:
    rfft8pt(ioptr);           /* a 16 pt fft */
    break;
  default:
    scale = 0.5;
    /* bit reverse and first radix 2 stage */
    scbitrevR2(ioptr, M, BRLow, scale);
    StageCnt = (M - 1) / 3;   /* number of radix 8 stages           */
    NDiffU = 2;               /* one radix 2 stage already complete */
    if ((M - 1 - (StageCnt * 3)) == 1) {
      bfR2(ioptr, M, NDiffU); /* 1 radix 2 stage */
      NDiffU *= 2;
    }
    if ((M - 1 - (StageCnt * 3)) == 2) {
      bfR4(ioptr, M, NDiffU); /* 1 radix 4 stage */
      NDiffU *= 4;
    }
    if (M <= (int32_t) MCACHE)
      bfstages(ioptr, M, Utbl, 2, NDiffU, StageCnt);  /* RADIX 8 Stages */
    else
      fftrecurs(ioptr, M, Utbl, 2, NDiffU, StageCnt); /* RADIX 8 Stages */
    frstage(ioptr, M + 1, Utbl);
  }
}

/*******************
 * parts of riffts1 *
 *******************/

static void rifft1pt(MYFLT *ioptr, MYFLT scale)
{
  /***   RADIX 2 rifft    ***/
  MYFLT f0r, f0i;
  MYFLT t0r, t0i;

  /* bit reversed load */
  f0r = ioptr[0];
  f0i = ioptr[1];

  /* finish rfft */
  t0r = f0r + f0i;
  t0i = f0r - f0i;

  /* store result */
  ioptr[0] = scale * t0r;
  ioptr[1] = scale * t0i;
}

static void rifft2pt(MYFLT *ioptr, MYFLT scale)
{
  /***   RADIX 4 rifft    ***/
  MYFLT f0r, f0i, f1r, f1i;
  MYFLT t0r, t0i;
  const MYFLT Two = FL(2.0);

  /* bit reversed load */
  t0r = ioptr[0];
  t0i = ioptr[1];
  f1r = Two * ioptr[2];
  f1i = Two * ioptr[3];

  /* start rifft */
  f0r = t0r + t0i;
  f0i = t0r - t0i;
  /* Butterflys           */
  /*
    f0   -       -       t0
    f1   -  1 -  f1
  */

  t0r = f0r + f1r;
  t0i = f0i - f1i;
  f1r = f0r - f1r;
  f1i = f0i + f1i;

  /* store result */
  ioptr[0] = scale * t0r;
  ioptr[1] = scale * t0i;
  ioptr[2] = scale * f1r;
  ioptr[3] = scale * f1i;
}

static void rifft4pt(MYFLT *ioptr, MYFLT scale)
{
  /***   RADIX 8 rifft    ***/
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT t0r, t0i, t1r, t1i;
  MYFLT w0r = 1.0 / ROOT2;    /* cos(pi/4)   */
  const MYFLT Two = FL(2.0);

  /* bit reversed load */
  t0r = ioptr[0];
  t0i = ioptr[1];
  f2r = ioptr[2];
  f2i = ioptr[3];
  f1r = Two * ioptr[4];
  f1i = Two * ioptr[5];
  f3r = ioptr[6];
  f3i = ioptr[7];

  /* start rfft */
  f0r = t0r + t0i;              /* compute Re(x[0]) */
  f0i = t0r - t0i;              /* compute Re(x[N/2]) */

  t1r = f2r + f3r;
  t1i = f2i - f3i;
  t0r = f2r - f3r;
  t0i = f2i + f3i;

  f2r = t1r - w0r * t0r - w0r * t0i;
  f2i = t1i + w0r * t0r - w0r * t0i;
  f3r = Two * t1r - f2r;
  f3i = f2i - Two * t1i;

  /* Butterflys           */
  /*
    f0   -       -       t0      -       -       f0
    f1   -  1 -  f1      -       -       f1
    f2   -       -       f2      -  1 -  f2
    f3   -  1 -  t1      -  i -  f3
  */

  t0r = f0r + f1r;
  t0i = f0i - f1i;
  f1r = f0r - f1r;
  f1i = f0i + f1i;

  t1r = f2r - f3r;
  t1i = f2i - f3i;
  f2r = f2r + f3r;
  f2i = f2i + f3i;

  f0r = t0r + f2r;
  f0i = t0i + f2i;
  f2r = t0r - f2r;
  f2i = t0i - f2i;

  f3r = f1r + t1i;
  f3i = f1i - t1r;
  f1r = f1r - t1i;
  f1i = f1i + t1r;

  /* store result */
  ioptr[0] = scale * f0r;
  ioptr[1] = scale * f0i;
  ioptr[2] = scale * f1r;
  ioptr[3] = scale * f1i;
  ioptr[4] = scale * f2r;
  ioptr[5] = scale * f2i;
  ioptr[6] = scale * f3r;
  ioptr[7] = scale * f3i;
}

static void rifft8pt(MYFLT *ioptr, MYFLT scale)
{
  /***   RADIX 16 rifft   ***/
  MYFLT w0r = (MYFLT) (1.0 / ROOT2);    /* cos(pi/4)    */
  MYFLT w1r = MYCOSPID8;                  /* cos(pi/8)    */
  MYFLT w1i = MYSINPID8;                  /* sin(pi/8)    */
  MYFLT f0r, f0i, f1r, f1i, f2r, f2i, f3r, f3i;
  MYFLT f4r, f4i, f5r, f5i, f6r, f6i, f7r, f7i;
  MYFLT t0r, t0i, t1r, t1i;
  const MYFLT Two = FL(2.0);

  /* bit reversed load */
  t0r = ioptr[0];
  t0i = ioptr[1];
  f4r = ioptr[2];
  f4i = ioptr[3];
  f2r = ioptr[4];
  f2i = ioptr[5];
  f6r = ioptr[6];
  f6i = ioptr[7];
  f1r = Two * ioptr[8];
  f1i = Two * ioptr[9];
  f5r = ioptr[10];
  f5i = ioptr[11];
  f3r = ioptr[12];
  f3i = ioptr[13];
  f7r = ioptr[14];
  f7i = ioptr[15];

  /* start rfft */
  f0r = t0r + t0i;              /* compute Re(x[0]) */
  f0i = t0r - t0i;              /* compute Re(x[N/2]) */

  t0r = f2r + f3r;
  t0i = f2i - f3i;
  t1r = f2r - f3r;
  t1i = f2i + f3i;

  f2r = t0r - w0r * t1r - w0r * t1i;
  f2i = t0i + w0r * t1r - w0r * t1i;
  f3r = Two * t0r - f2r;
  f3i = f2i - Two * t0i;

  t0r = f4r + f7r;
  t0i = f4i - f7i;
  t1r = f4r - f7r;
  t1i = f4i + f7i;

  f4r = t0r - w1i * t1r - w1r * t1i;
  f4i = t0i + w1r * t1r - w1i * t1i;
  f7r = Two * t0r - f4r;
  f7i = f4i - Two * t0i;

  t0r = f6r + f5r;
  t0i = f6i - f5i;
  t1r = f6r - f5r;
  t1i = f6i + f5i;

  f6r = t0r - w1r * t1r - w1i * t1i;
  f6i = t0i + w1i * t1r - w1r * t1i;
  f5r = Two * t0r - f6r;
  f5i = f6i - Two * t0i;

  /* Butterflys           */
  /*
    f0   -       -       t0      -       -       f0      -       -       f0
    f1*  -  1 -  f1      -       -       f1      -       -       f1
    f2   -       -       f2      -  1 -  f2      -       -       f2
    f3   -  1 -  t1      -  i -  f3      -       -       f3
    f4   -       -       t0      -       -       f4      -  1 -  t0
    f5   -  1 -  f5      -       -       f5      - w3 -  f4
    f6   -       -       f6      -  1 -  f6      -  i -  t1
    f7   -  1 -  t1      -  i -  f7      - iw3-  f6
  */

  t0r = f0r + f1r;
  t0i = f0i - f1i;
  f1r = f0r - f1r;
  f1i = f0i + f1i;

  t1r = f2r - f3r;
  t1i = f2i - f3i;
  f2r = f2r + f3r;
  f2i = f2i + f3i;

  f0r = t0r + f2r;
  f0i = t0i + f2i;
  f2r = t0r - f2r;
  f2i = t0i - f2i;

  f3r = f1r + t1i;
  f3i = f1i - t1r;
  f1r = f1r - t1i;
  f1i = f1i + t1r;

  t0r = f4r + f5r;
  t0i = f4i + f5i;
  f5r = f4r - f5r;
  f5i = f4i - f5i;

  t1r = f6r - f7r;
  t1i = f6i - f7i;
  f6r = f6r + f7r;
  f6i = f6i + f7i;

  f4r = t0r + f6r;
  f4i = t0i + f6i;
  f6r = t0r - f6r;
  f6i = t0i - f6i;

  f7r = f5r + t1i;
  f7i = f5i - t1r;
  f5r = f5r - t1i;
  f5i = f5i + t1r;

  t0r = f0r - f4r;
  t0i = f0i - f4i;
  f0r = f0r + f4r;
  f0i = f0i + f4i;

  t1r = f2r + f6i;
  t1i = f2i - f6r;
  f2r = f2r - f6i;
  f2i = f2i + f6r;

  f4r = f1r - f5r * w0r + f5i * w0r;
  f4i = f1i - f5r * w0r - f5i * w0r;
  f1r = f1r * Two - f4r;
  f1i = f1i * Two - f4i;

  f6r = f3r + f7r * w0r + f7i * w0r;
  f6i = f3i - f7r * w0r + f7i * w0r;
  f3r = f3r * Two - f6r;
  f3i = f3i * Two - f6i;

  /* store result */
  ioptr[0] = scale * f0r;
  ioptr[1] = scale * f0i;
  ioptr[2] = scale * f1r;
  ioptr[3] = scale * f1i;
  ioptr[4] = scale * f2r;
  ioptr[5] = scale * f2i;
  ioptr[6] = scale * f3r;
  ioptr[7] = scale * f3i;
  ioptr[8] = scale * t0r;
  ioptr[9] = scale * t0i;
  ioptr[10] = scale * f4r;
  ioptr[11] = scale * f4i;
  ioptr[12] = scale * t1r;
  ioptr[13] = scale * t1i;
  ioptr[14] = scale * f6r;
  ioptr[15] = scale * f6i;
}

static void ifrstage(MYFLT *ioptr, int32_t M, MYFLT *Utbl)
{
  /*      Start RIFFT             */

  uint32_t pos;
  uint32_t posi;
  uint32_t diffUcnt;

  MYFLT *p0r, *p1r;
  MYFLT *u0r, *u0i;

  MYFLT w0r, w0i;
  MYFLT f0r, f0i, f1r, f1i, f4r, f4i, f5r, f5i;
  MYFLT t0r, t0i, t1r, t1i;
  const MYFLT Two = FL(2.0);

  pos = POW2(M - 1);
  posi = pos + 1;

  p0r = ioptr;
  p1r = ioptr + pos / 2;

  u0r = Utbl + POW2(M - 3);

  w0r = *u0r, f0r = *(p0r);
  f0i = *(p0r + 1);
  f4r = *(p0r + pos);
  f4i = *(p0r + posi);
  f1r = *(p1r);
  f1i = *(p1r + 1);
  f5r = *(p1r + pos);
  f5i = *(p1r + posi);

  t0r = f0r + f0i;
  t0i = f0r - f0i;
  t1r = f4r + f4r;
  t1i = -f4i - f4i;

  f0r = f1r + f5r;
  f0i = f1i - f5i;
  f4r = f1r - f5r;
  f4i = f1i + f5i;

  f1r = f0r - w0r * f4r - w0r * f4i;
  f1i = f0i + w0r * f4r - w0r * f4i;
  f5r = Two * f0r - f1r;
  f5i = f1i - Two * f0i;

  *(p0r) = t0r;
  *(p0r + 1) = t0i;
  *(p0r + pos) = t1r;
  *(p0r + posi) = t1i;
  *(p1r) = f1r;
  *(p1r + 1) = f1i;
  *(p1r + pos) = f5r;
  *(p1r + posi) = f5i;

  u0r = Utbl + 1;
  u0i = Utbl + (POW2(M - 2) - 1);

  w0r = *u0r, w0i = *u0i;

  p0r = (ioptr + 2);
  p1r = (ioptr + (POW2(M - 2) - 1) * 2);

  /* Butterflys */
  /*
    f0   -        t0             -       f0
    f1   -     t1     -w0-   f1

    f2   -        t0             -       f2
    f3   -     t1           -iw0-  f3
  */

  for (diffUcnt = POW2(M - 3) - 1; diffUcnt > 0; diffUcnt--) {

    f0r = *(p0r);
    f0i = *(p0r + 1);
    f5r = *(p1r + pos);
    f5i = *(p1r + posi);
    f1r = *(p1r);
    f1i = *(p1r + 1);
    f4r = *(p0r + pos);
    f4i = *(p0r + posi);

    t0r = f0r + f5r;
    t0i = f0i - f5i;
    t1r = f0r - f5r;
    t1i = f0i + f5i;

    f0r = t0r - w0i * t1r - w0r * t1i;
    f0i = t0i + w0r * t1r - w0i * t1i;
    f5r = Two * t0r - f0r;
    f5i = f0i - Two * t0i;

    t0r = f1r + f4r;
    t0i = f1i - f4i;
    t1r = f1r - f4r;
    t1i = f1i + f4i;

    f1r = t0r - w0r * t1r - w0i * t1i;
    f1i = t0i + w0i * t1r - w0r * t1i;
    f4r = Two * t0r - f1r;
    f4i = f1i - Two * t0i;

    *(p0r) = f0r;
    *(p0r + 1) = f0i;
    *(p1r + pos) = f5r;
    *(p1r + posi) = f5i;

    w0r = *++u0r;
    w0i = *--u0i;

    *(p1r) = f1r;
    *(p1r + 1) = f1i;
    *(p0r + pos) = f4r;
    *(p0r + posi) = f4i;

    p0r += 2;
    p1r -= 2;
  }
}

static void riffts1(MYFLT *ioptr, int32_t M, MYFLT *Utbl, int16 *BRLow)
{
  /* Compute in-place real ifft on the rows of the input array    */
  /* data order as from rffts1                                    */
  /* INPUTS                                                       */
  /*   *ioptr = input data array in the following order           */
  /*   M = log2 of fft size                                       */
  /*   Re(x[0]), Re(x[N/2]), Re(x[1]), Im(x[1]),                  */
  /*   Re(x[2]), Im(x[2]), ... Re(x[N/2-1]), Im(x[N/2-1]).        */
  /*   *Utbl = cosine table                                       */
  /*   *BRLow = bit reversed counter table                        */
  /* OUTPUTS                                                      */
  /*   *ioptr = real output data array                            */

  MYFLT scale;
  int32_t StageCnt;
  int32_t NDiffU;

  scale = (MYFLT)(1.0 / (double)((int32_t)POW2(M)));
  M = M - 1;
  switch (M) {
  case -1:
    break;
  case 0:
    rifft1pt(ioptr, scale);   /* a 2 pt fft */
    break;
  case 1:
    rifft2pt(ioptr, scale);   /* a 4 pt fft */
    break;
  case 2:
    rifft4pt(ioptr, scale);   /* an 8 pt fft */
    break;
  case 3:
    rifft8pt(ioptr, scale);   /* a 16 pt fft */
    break;
  default:
    ifrstage(ioptr, M + 1, Utbl);
    /* bit reverse and first radix 2 stage */
    scbitrevR2(ioptr, M, BRLow, scale);
    StageCnt = (M - 1) / 3;   /* number of radix 8 stages           */
    NDiffU = 2;               /* one radix 2 stage already complete */
    if ((M - 1 - (StageCnt * 3)) == 1) {
      ibfR2(ioptr, M, NDiffU);        /* 1 radix 2 stage */
      NDiffU *= 2;
    }
    if ((M - 1 - (StageCnt * 3)) == 2) {
      ibfR4(ioptr, M, NDiffU);        /* 1 radix 4 stage */
      NDiffU *= 4;
    }
    if (M <= (int32_t) MCACHE)
      ibfstages(ioptr, M, Utbl, 2, NDiffU, StageCnt); /*  RADIX 8 Stages */
    else
      ifftrecurs(ioptr, M, Utbl, 2, NDiffU, StageCnt); /* RADIX 8 Stages */
  }
}


static void fftInit(CSOUND *csound, int32_t M)
{
  /* malloc and init cosine and bit reversed tables for a given size  */
  /* fft, ifft, rfft, rifft                                           */
  /* INPUTS                                                           */
  /*   M = log2 of fft size (ex M=10 for 1024 point fft)              */
  /* OUTPUTS                                                          */
  /*   private cosine and bit reversed tables                         */

  MYFLT **UtblArray;
  int16 **BRLowArray;
  int32_t   i;

  if (!csound->FFT_max_size) {
    if (csound->FFT_table_1 == NULL)
      csound->FFT_table_1 = csound->Malloc(csound, sizeof(MYFLT*) * 32);
    if (csound->FFT_table_2 == NULL)
      csound->FFT_table_2 = csound->Malloc(csound, sizeof(int16*) * 32);
    for (i = 0; i < 32; i++) {
      ((MYFLT**) csound->FFT_table_1)[i] = (MYFLT*) NULL;
      ((int16**) csound->FFT_table_2)[i] = (int16*) NULL;
    }
  }
  UtblArray = (MYFLT**) csound->FFT_table_1;
  BRLowArray = (int16**) csound->FFT_table_2;

  /*** I did NOT test cases with M>27 ***/
  /* init cos table */
  UtblArray[M] = (MYFLT*) csound->Malloc(csound,
                                         (POW2(M) / 4 + 1) * sizeof(MYFLT));
  fftCosInit(M, UtblArray[M]);
  if (M > 1) {
    /* init bit reversed table for cmplx FFT */
    if (BRLowArray[M / 2] == NULL) {
      BRLowArray[M / 2] =
        (int16*) csound->Malloc(csound, POW2(M / 2 - 1) * sizeof(int16));
      fftBRInit(M, BRLowArray[M / 2]);
    }
  }
  if (M > 2) {
    /* init bit reversed table for real FFT */
    if (BRLowArray[(M - 1) / 2] == 0) {
      BRLowArray[(M - 1) / 2] =
        (int16*) csound->Malloc(csound,
                                POW2((M - 1) / 2 - 1) * sizeof(int16));
      fftBRInit(M - 1, BRLowArray[(M - 1) / 2]);
    }
  }

  csound->FFT_max_size |= (1 << M);
}


static inline int32_t ConvertFFTSize(CSOUND *csound, int32_t N)
{
  if (N <= 0)
    return (-N);
  switch (N) {
  case 0x00000001:  return 0;
  case 0x00000002:  return 1;
  case 0x00000004:  return 2;
  case 0x00000008:  return 3;
  case 0x00000010:  return 4;
  case 0x00000020:  return 5;
  case 0x00000040:  return 6;
  case 0x00000080:  return 7;
  case 0x00000100:  return 8;
  case 0x00000200:  return 9;
  case 0x00000400:  return 10;
  case 0x00000800:  return 11;
  case 0x00001000:  return 12;
  case 0x00002000:  return 13;
  case 0x00004000:  return 14;
  case 0x00008000:  return 15;
  case 0x00010000:  return 16;
  case 0x00020000:  return 17;
  case 0x00040000:  return 18;
  case 0x00080000:  return 19;
  case 0x00100000:  return 20;
  case 0x00200000:  return 21;
  case 0x00400000:  return 22;
  case 0x00800000:  return 23;
  case 0x01000000:  return 24;
  case 0x02000000:  return 25;
  case 0x04000000:  return 26;
  case 0x08000000:  return 27;
  case 0x10000000:  return 28;
  }
  csound->Warning(csound, Str(" *** fftlib.c: internal error: "
                              "invalid FFT size: %d"), N);
  return 0;
}

static inline void getTablePointers(CSOUND *p, MYFLT **ct, int16 **bt,
                                    int32_t cn, int32_t bn)
{
  if (!(p->FFT_max_size & (1 << cn)))
    fftInit(p, cn);
  *ct = ((MYFLT**) p->FFT_table_1)[cn];
  *bt = ((int16**) p->FFT_table_2)[bn];
}


/**
 * Returns the amplitude scale that should be applied to the result of
 * an inverse complex FFT with a length of 'FFTsize' samples.
 */
MYFLT csoundGetInverseComplexFFTScale(CSOUND *csound, int32_t FFTsize)
{
  IGN(FFTsize);
  IGN(csound);
  return FL(1.0);
}

/**
 * Returns the amplitude scale that should be applied to the result of
 * an inverse real FFT with a length of 'FFTsize' samples.
 */
MYFLT csoundGetInverseRealFFTScale(CSOUND *csound, int32_t FFTsize)
{
  IGN(FFTsize);
  IGN(csound);
  return FL(1.0);
}

/**
 * Compute in-place complex FFT
 * FFTsize: FFT length in samples
 * buf:     array of FFTsize*2 MYFLT values,
 *          in interleaved real/imaginary format
 */
void csoundComplexFFT(CSOUND *csound, MYFLT *buf, int32_t FFTsize)
{
  MYFLT *Utbl;
  int16 *BRLow;
  int32_t   M;

  M = ConvertFFTSize(csound, FFTsize);
  getTablePointers(csound, &Utbl, &BRLow, M, M / 2);
  ffts1(buf, M, Utbl, BRLow);
}

/**
 * Compute in-place inverse complex FFT
 * FFTsize: FFT length in samples
 * buf:     array of FFTsize*2 MYFLT values,
 *          in interleaved real/imaginary format
 * Output should be scaled by the return value of
 * csoundGetInverseComplexFFTScale(csound, FFTsize).
 */

void csoundInverseComplexFFT(CSOUND *csound, MYFLT *buf, int32_t FFTsize)
{
  MYFLT *Utbl;
  int16 *BRLow;
  int32_t   M;

  M = ConvertFFTSize(csound, FFTsize);
  getTablePointers(csound, &Utbl, &BRLow, M, M / 2);
  iffts1(buf, M, Utbl, BRLow);
}

/**
 * Compute in-place real FFT
 * FFTsize: FFT length in samples
 * buf:     array of FFTsize MYFLT values; output is in interleaved
 *          real/imaginary format, except for buf[1] which is the real
 *          part for the Nyquist frequency
 */

void csoundRealFFT(CSOUND *csound, MYFLT *buf, int32_t FFTsize)
{

    MYFLT *Utbl;
    int16 *BRLow;
    int32_t   M;
    M = ConvertFFTSize(csound, FFTsize);
    getTablePointers(csound, &Utbl, &BRLow, M, (M - 1) / 2);
    rffts1(buf, M, Utbl, BRLow);
}

/**
 * Compute in-place inverse real FFT
 * FFTsize: FFT length in samples
 * buf:     array of FFTsize MYFLT values; input is expected to be in
 *          interleaved real/imaginary format, except for buf[1] which
 *          is the real part for the Nyquist frequency
 * Output should be scaled by the return value of
 * csoundGetInverseRealFFTScale(csound, FFTsize).
 */

void csoundInverseRealFFT(CSOUND *csound, MYFLT *buf, int32_t FFTsize)
{
    MYFLT *Utbl;
    int16 *BRLow;
    int32_t   M;
    M = ConvertFFTSize(csound, FFTsize);
    getTablePointers(csound, &Utbl, &BRLow, M, (M - 1) / 2);
    riffts1(buf, M, Utbl, BRLow);
}

/**
 * Multiply two arrays (buf1 and buf2) of complex data in the format
 * returned by csoundRealFFT(), and leave the result in outbuf, which
 * may be the same as either buf1 or buf2.
 * An amplitude scale of 'scaleFac' is also applied.
 * The arrays should contain 'FFTsize' MYFLT values.
 */

void csoundRealFFTMult(CSOUND *csound, MYFLT *outbuf,
                       MYFLT *buf1, MYFLT *buf2, int32_t FFTsize, MYFLT scaleFac)
{
  MYFLT re, im;
  int32_t   i;
  IGN(csound);

  if (scaleFac != FL(1.0)) {
    outbuf[0] = buf1[0] * buf2[0] * scaleFac;
    if (FFTsize < 2)
      return;
    outbuf[1] = buf1[1] * buf2[1] * scaleFac;
    for (i = 2; i < FFTsize; ) {
      re = ((buf1[i] * buf2[i]) - (buf1[i + 1] * buf2[i + 1])) * scaleFac;
      im = ((buf1[i] * buf2[i + 1]) + (buf2[i] * buf1[i + 1])) * scaleFac;
      outbuf[i++] = re;
      outbuf[i++] = im;
    }
  }
  else {
    outbuf[0] = buf1[0] * buf2[0];
    if (FFTsize < 2)
      return;
    outbuf[1] = buf1[1] * buf2[1];
    for (i = 2; i < FFTsize; ) {
      re = (buf1[i] * buf2[i]) - (buf1[i + 1] * buf2[i + 1]);
      im = (buf1[i] * buf2[i + 1]) + (buf2[i] * buf1[i + 1]);
      outbuf[i++] = re;
      outbuf[i++] = im;
    }
  }
}



/*
  New FFT interface
  VL, 2016
*/
static
void pffft_execute(CSOUND_FFT_SETUP *setup,
                   MYFLT *sig) {
  int32_t i, N = setup->N;
  float s, *buf;
  buf = (float *) setup->buffer;
  for(i=0;i<N;i++)
    buf[i] = sig[i];
  pffft_transform_ordered((PFFFT_Setup *)
                          setup->setup,
                          buf,buf,NULL,setup->d);
  s = (setup->d == PFFFT_BACKWARD ?
       (MYFLT) setup->N : FL(1.0));
  for(i=0;i<N;i++)
    sig[i] = buf[i]/s;
}

#if defined(__MACH__)
/* vDSP FFT implementation */
#include <Accelerate/Accelerate.h>
static
void vDSP_execute(CSOUND_FFT_SETUP *setup,
                   MYFLT *sig){
#ifdef USE_DOUBLE
  DSPDoubleSplitComplex tmp;
#else
  DSPSplitComplex tmp;
#endif
  int32_t i,j;
  MYFLT s;
  int32_t N = setup->N;
  tmp.realp = &setup->buffer[0];
  tmp.imagp = &setup->buffer[N>>1];
  for(i=j=0;i<N;i+=2,j++){
    tmp.realp[j] = sig[i];
    tmp.imagp[j] = sig[i+1];
    }
#ifdef USE_DOUBLE
  vDSP_fft_zripD((FFTSetupD) setup->setup,
                 &tmp, 1,
                 setup->M,setup->d);
#else
  vDSP_fft_zrip((FFTSetup) setup->setup,
                 &tmp, 1,
                 setup->M,setup->d);
#endif
 s = (setup->d == -1 ? (MYFLT)(N) : FL(2.0));
 for(i=j=0;i<N;i+=2,j++){
    sig[i] = tmp.realp[j]/s;
    sig[i+1] = tmp.imagp[j]/s;
    }
}
#endif

#define ALIGN_BYTES 64
static
void *align_alloc(CSOUND *csound, size_t nb_bytes){
  void *p, *p0 = csound->Malloc(csound, nb_bytes + ALIGN_BYTES);
  if(!p0) return (void *) 0;
  p = (void *) (((size_t) p0 + ALIGN_BYTES)
                & (~((size_t) (ALIGN_BYTES-1))));
  *((void **) p - 1) = p0;
  return p;
}

int32_t setupDispose(CSOUND *csound, void *pp){
  IGN(csound);
  CSOUND_FFT_SETUP *setup =(CSOUND_FFT_SETUP *) pp;
  switch(setup->lib){
#if defined(__MACH__)
  case VDSP_LIB:
#ifdef USE_DOUBLE
     vDSP_destroy_fftsetupD((FFTSetupD)
#else
     vDSP_destroy_fftsetup((FFTSetup)
#endif
                           setup->setup);
    break;
#endif
  case PFFT_LIB:
    pffft_destroy_setup((PFFFT_Setup *)setup->setup);
    break;
  }
  return OK;
}

int32_t isPowTwo(int32_t N) {
  return (N != 0) ? !(N & (N - 1)) : 0;
}

void *csoundRealFFT2Setup(CSOUND *csound,
                         int32_t FFTsize,
                         int32_t d){
  CSOUND_FFT_SETUP *setup;
  int32_t lib = csound->oparms->fft_lib;
  if(lib == PFFT_LIB && FFTsize <= 16){
    csound->Warning(csound,
      "FFTsize %d \n"
      "Cannot use PFFT with sizes <= 16\n"
      "--defaulting to FFTLIB",
        FFTsize);
    lib = 0;
  }
  setup = (CSOUND_FFT_SETUP *)
    csound->Calloc(csound, sizeof(CSOUND_FFT_SETUP));
  setup->N = FFTsize;
  setup->p2 = isPowTwo(FFTsize);
  switch(lib){
#if defined(__MACH__)
  case VDSP_LIB:
    setup->M = ConvertFFTSize(csound, FFTsize);
    setup->setup = (void *)
#ifdef USE_DOUBLE
      vDSP_create_fftsetupD(setup->M,kFFTRadix2);
#else
      vDSP_create_fftsetup(setup->M,kFFTRadix2);
#endif
      setup->d = (d ==  FFT_FWD ?
                kFFTDirection_Forward :
                kFFTDirection_Inverse);
    setup->lib = lib;
    break;
#endif
  case PFFT_LIB:
    setup->setup = (void *)
      pffft_new_setup(FFTsize,PFFFT_REAL);
    setup->d = (d ==  FFT_FWD ?
                PFFFT_FORWARD :
                PFFFT_BACKWARD);
    setup->lib = lib;
    break;
  default:
    setup->lib = 0;
    setup->d = d;
    return (void *) setup;
  }
  setup->buffer = (MYFLT *) align_alloc(csound, sizeof(MYFLT)*FFTsize);
  csound->RegisterResetCallback(csound, (void*) setup,
                                (int32_t (*)(CSOUND *, void *))
                                setupDispose);
  return (void *) setup;
}

void csoundRealFFT2(CSOUND *csound,
                     void *p, MYFLT *sig){
  CSOUND_FFT_SETUP *setup =
        (CSOUND_FFT_SETUP *) p;
  switch(setup->lib) {
#if defined(__MACH__)
  case VDSP_LIB:
    vDSP_execute(setup,sig);
    break;
#endif
  case PFFT_LIB:
    pffft_execute(setup,sig);
    break;
  default:
    (setup->d == FFT_FWD ?
      csoundRealFFT(csound,
                     sig,setup->N) :
      csoundInverseRealFFT(csound,
                     sig,setup->N));
    setup->lib = 0;
  }
}


void *csoundDCTSetup(CSOUND *csound,
                     int32_t FFTsize, int32_t d){
 CSOUND_FFT_SETUP *setup;
 setup = (CSOUND_FFT_SETUP *)
   csoundRealFFT2Setup(csound,
                       FFTsize*4,d);
 if(setup->lib == 0){
  setup->buffer = (MYFLT *)
    csound->Calloc(csound, sizeof(MYFLT)*setup->N);
 }
 return setup;
}


void pffft_DCT_execute(CSOUND *csound,
                     void *p, MYFLT *sig){
  IGN(csound);
  CSOUND_FFT_SETUP *setup =
        (CSOUND_FFT_SETUP *) p;
  int32_t i,j, N= setup->N;
  float *buffer = (float *)setup->buffer;
  if(setup->d == FFT_FWD){
  for(i=j = 0; i < N/2; i+=2, j++){
    buffer[i] = FL(0.0);
    buffer[i+1] = sig[j];
  }
  for(i=N/2,j=N/4-1; i < N; i+=2, j--){
    buffer[i] = FL(0.0);
    buffer[i+1] = sig[j];
  }
  pffft_transform_ordered((PFFFT_Setup *)
                          setup->setup,
                          buffer,buffer,
                          NULL,PFFFT_FORWARD);
  for(i=j=0; i < N/2; i+=2, j++){
    sig[j] = buffer[i];
  }
  } else {
  buffer[0] = sig[0];
  buffer[1] = -sig[0];
  for(i=2,j=1; i < N/2; i+=2, j++){
    buffer[i] = sig[j];
    buffer[i+1] = FL(0.0);
  }
  buffer[N/2] = buffer[N/2+1] = FL(0.0);
  for(i=N/2+2,j=N/4-1; i < N; i+=2, j--){
    buffer[i] = -sig[j];
    buffer[i+1] = FL(0.0);
  }
  pffft_transform_ordered((PFFFT_Setup *)
                          setup->setup,
                          buffer,buffer,
                          NULL,PFFFT_BACKWARD);
  for(i=j=0; i < N/2; i+=2, j++){
    sig[j] = buffer[i+1]/N;
  }
  }
}

#if defined(__MACH__)
void vDSP_DCT_execute(CSOUND *csound,
                     void *p, MYFLT *sig){
  IGN(csound);
  CSOUND_FFT_SETUP *setup =
        (CSOUND_FFT_SETUP *) p;
  int32_t i,j, N= setup->N;
#ifdef USE_DOUBLE
  DSPDoubleSplitComplex tmp;
#else
  DSPSplitComplex tmp;
#endif
  tmp.realp = &setup->buffer[0];
  tmp.imagp = &setup->buffer[N>>1];
  if(setup->d ==  kFFTDirection_Forward){
  for(j=0;j<N/4;j++){
    tmp.realp[j] = FL(0.0);
    tmp.imagp[j] = sig[j];
    }
  for(i=N/4,j=N/4-1;i<N/2;i++,j--){
    tmp.realp[i] = FL(0.0);
    tmp.imagp[i] = sig[j];
  }
#ifdef USE_DOUBLE
  vDSP_fft_zripD((FFTSetupD) setup->setup,
#else
  vDSP_fft_zrip((FFTSetup) setup->setup,
#endif
                 &tmp, 1,
                 setup->M,setup->d);

  for(j=0; j < N/4; j++){
    sig[j] = tmp.realp[j]/FL(2.0);
  }
  } else {
  tmp.realp[0] = sig[0];
  tmp.imagp[0] = -sig[0];
  for(j=1; j < N/4; j++){
    tmp.realp[j] = sig[j];
    tmp.imagp[j] = FL(0.0);
  }
  tmp.realp[N/4] = tmp.imagp[N/4] = FL(0.0);
  for(i=N/4+1,j=N/4-1; i < N/2; i++, j--){
    tmp.realp[i] = -sig[j];
    tmp.imagp[i] = FL(0.0);
  }
#ifdef USE_DOUBLE
  vDSP_fft_zripD((FFTSetupD) setup->setup,
#else
  vDSP_fft_zrip((FFTSetup) setup->setup,
#endif
                 &tmp, 1,
                 setup->M,setup->d);
  for(j=0; j < N/4; j++){
    sig[j] = tmp.imagp[j]/N;
  }
  }
}
#endif

void DCT_execute(CSOUND *csound,
                     void *p, MYFLT *sig){
  CSOUND_FFT_SETUP *setup =
        (CSOUND_FFT_SETUP *) p;
  int32_t i,j, N= setup->N;
  MYFLT *buffer = setup->buffer;
  if(setup->d == FFT_FWD){
  for(i=j = 0; i < N/2; i+=2, j++){
    buffer[i] = FL(0.0);
    buffer[i+1] = sig[j];
  }
  for(i=N/2,j=N/4-1; i < N; i+=2, j--){
    buffer[i] = FL(0.0);
    buffer[i+1] = sig[j];
  }
  csoundRealFFT(csound,buffer,N);
  for(i=j=0; i < N/2; i+=2, j++){
    sig[j] = buffer[i];
  }
  } else {
  buffer[0] = sig[0];
  buffer[1] = -sig[0];
  for(i=2,j=1; i < N/2; i+=2, j++){
    buffer[i] = sig[j];
    buffer[i+1] = FL(0.0);
  }
  buffer[N/2] = buffer[N/2+1] = FL(0.0);
  for(i=N/2+2,j=N/4-1; i < N; i+=2, j--){
    buffer[i] = -sig[j];
    buffer[i+1] = FL(0.0);
  }
  csoundInverseRealFFT(csound,buffer,N);
  for(i=j=0; i < N/2; i+=2, j++){
    sig[j] = buffer[i+1];
  }
  }
}

void csoundDCT(CSOUND *csound,
               void *p, MYFLT *sig){
CSOUND_FFT_SETUP *setup =
        (CSOUND_FFT_SETUP *) p;
  switch(setup->lib) {
#if defined(__MACH__)
  case VDSP_LIB:
    vDSP_DCT_execute(csound,setup,sig);
    break;
#endif
  case PFFT_LIB:
    pffft_DCT_execute(csound,setup,sig);
    break;
  default:
    DCT_execute(csound,setup,sig);
    setup->lib = 0;
  }
}

/* =======--====================*/
#if 0
#ifdef HAVE_VECLIB
/* vDSP FFT interface */

#define ALIGN64 64
static
void *vDSP_alloc(CSOUND *csound, size_t nb_bytes){
  void *p, *p0 = csound->Malloc(csound, nb_bytes + ALIGN64);
  if(!p0) return (void *) 0;
  p = (void *) (((size_t) p0 + ALIGN64)
                & (~((size_t) (ALIGN64-1))));
  *((void **) p - 1) = p0;
  return p;
}

static
void vDSP_free(CSOUND *csound, void *p) {
  if(p)csound->Free(csound, *((void **) p - 1));
}

/* reset vdsp */
int32_t vDSP_reset(CSOUND *csound, void *pp){
  IGN(pp);
#ifdef USE_DOUBLE
  vDSP_destroy_fftsetupD(csound->vdsp_setup);
#else
  vDSP_destroy_fftsetup(csound->vdsp_setup);
#endif
  csound->vdsp_setup = NULL;
  vDSP_free(csound, csound->vdsp_buffer);
  return OK;
}

/* create setup if setup does not exist */
static
void vDSP_setup(CSOUND *csound, int32_t FFTsize){
  int32_t M = ConvertFFTSize(csound, FFTsize);
  if(csound->FFT_max_size < FFTsize){
#ifdef USE_DOUBLE
    if(csound->vdsp_setup != NULL)
      vDSP_destroy_fftsetupD(csound->vdsp_setup);
    csound->vdsp_setup =
      vDSP_create_fftsetupD(M,kFFTRadix2);
#else
    if(csound->vdsp_setup != NULL)
      vDSP_destroy_fftsetup(csound->vdsp_setup);
    csound->vdsp_setup =
      vDSP_create_fftsetup(M,kFFTRadix2);
#endif
    if(csound->vdsp_buffer != NULL)
      vDSP_free(csound, csound->vdsp_buffer);
    csound->vdsp_buffer = vDSP_alloc(csound,
                                     FFTsize*(sizeof(MYFLT)));
    if(csound->FFT_max_size == 0)
      csound->RegisterResetCallback(csound, (void*) NULL,
                                    (int32_t (*)(CSOUND *, void *))
                                    vDSP_reset);
    csound->FFT_max_size = FFTsize;
  }
}

static
void vDSP_RealFFT(CSOUND *csound,int32_t FFTsize,MYFLT *sig,FFTDirection d){
#ifdef USE_DOUBLE
  DSPDoubleSplitComplex tmp;
#else
  DSPSplitComplex tmp;
#endif
  int32_t i,j;
  MYFLT s;
  vDSP_setup(csound, FFTsize);
  tmp.realp = &csound->vdsp_buffer[0];
  tmp.imagp = &csound->vdsp_buffer[FFTsize>>1];
  for(i=j=0;i<FFTsize;i+=2,j++){
    tmp.realp[j] = sig[i];
    tmp.imagp[j] = sig[i+1];
    }
#ifdef USE_DOUBLE
  vDSP_fft_zripD(csound->vdsp_setup, &tmp, 1,
                 ConvertFFTSize(csound, FFTsize),
                 d);
#else
  vDSP_fft_zrip(csound->vdsp_setup, &tmp, 1,
                ConvertFFTSize(csound, FFTsize),
                d);
#endif
 s = (d == -1 ? (MYFLT)(FFTsize) : FL(2.0));
 for(i=j=0;i<FFTsize;i+=2,j++){
    sig[i] = tmp.realp[j]/s;
    sig[i+1] = tmp.imagp[j]/s;
    }

}

/* these functions are available from OSX 10.9 */
int32_t vDSP_reset_New(CSOUND *csound, void *pp){
  IGN(pp);
#ifdef USE_DOUBLE
  vDSP_DFT_DestroySetupD(csound->vdsp_setup_fwd);
  vDSP_DFT_DestroySetupD(csound->vdsp_setup_inv);
#else
  vDSP_DFT_DestroySetup(csound->vdsp_setup_fwd);
  vDSP_DFT_DestroySetup(csound->vdsp_setup_inv);
#endif
  vDSP_free(csound, csound->vdsp_buffer);
  return OK;
}

/* create setup if setup does not exist */
static
#ifdef USE_DOUBLE
vDSP_DFT_SetupD
#else
vDSP_DFT_Setup
#endif
vDSP_setup_New(CSOUND *csound, int32_t FFTsize, int32_t d){
  if(csound->vdsp_setup_fwd == NULL ||
     csound->vdsp_setup_inv == NULL){
#ifdef USE_DOUBLE
    if(d == 1)
      csound->vdsp_setup_fwd =
        vDSP_DFT_zrop_CreateSetupD(csound->vdsp_setup_fwd,
                                   FFTsize, d);
    else
      csound->vdsp_setup_inv =
        vDSP_DFT_zrop_CreateSetupD(csound->vdsp_setup_inv,
                                   FFTsize, d);
#else
    if(d == 1)
      csound->vdsp_setup_fwd =
        vDSP_DFT_zrop_CreateSetup(csound->vdsp_setup_fwd,
                                  FFTsize, d);
    else
      csound->vdsp_setup_inv =
        vDSP_DFT_zrop_CreateSetup(csound->vdsp_setup_inv,
                                  FFTsize, d);
#endif
  }
  if(csound->FFT_max_size < FFTsize) {
    vDSP_free(csound, csound->vdsp_buffer);
    csound->vdsp_buffer = vDSP_alloc(csound,
                                     FFTsize*(sizeof(MYFLT)));
  }

  if(csound->FFT_max_size == 0)
    csound->RegisterResetCallback(csound, (void*) NULL,
                                  (int32_t (*)(CSOUND *, void *))
                                  vDSP_reset_New);
  csound->FFT_max_size = FFTsize;
  return d == 1 ? csound->vdsp_setup_fwd :
    csound->vdsp_setup_inv;
}

void vDSP_RealFFT_New(CSOUND *csound,int32_t FFTsize,MYFLT *sig,
                  FFTDirection d){
  int32_t i,j;
  MYFLT s;
#ifdef USE_DOUBLE
  vDSP_DFT_SetupD setup;
#else
  vDSP_DFT_Setup setup;
#endif
  setup = vDSP_setup_New(csound, FFTsize, d);
  MYFLT *inr = &csound->vdsp_buffer[0];
  MYFLT *ini = &csound->vdsp_buffer[FFTsize>>1];

  for(i=j=0;i<FFTsize;i+=2,j++){
    inr[j] = sig[i];
    ini[j] = sig[i+1];
  }
#ifdef USE_DOUBLE
  vDSP_DFT_ExecuteD(setup,inr,ini,inr,ini);
#else
  vDSP_DFT_Execute(setup,inr,ini,inr,ini);
#endif
    s = (d == -1 ? (MYFLT)(FFTsize) : FL(2.0));
  for(i=j=0;i<FFTsize;i+=2,j++){
    sig[i] = inr[j]/s;
    sig[i+1] = ini[j]/s;
  }
}

#else /* HAVE_VECLIB */
int32_t pffft_reset(CSOUND *csound, void *pp){
  IGN(pp);
  int32_t i;
  for(i=0; i < 32; i++)
    if(csound->setup[i]) pffft_destroy_setup(csound->setup[i]);
  pffft_aligned_free(csound->vdsp_buffer);
  return OK;
}

/* create setup if setup does not exist */
static
void pffft_setup(CSOUND *csound, int32_t FFTsize, int32_t M){
  if(csound->FFT_max_size != FFTsize){
    if(csound->FFT_max_size == 0)
      csound->RegisterResetCallback(csound, (void*) NULL,
                                    (int32_t (*)(CSOUND *, void *)) pffft_reset);
    if(csound->setup[M] == NULL)
       csound->setup[M] = pffft_new_setup(FFTsize,PFFFT_REAL);
    if(csound->FFT_max_size < FFTsize) {
      pffft_aligned_free(csound->vdsp_buffer);
      csound->vdsp_buffer = (MYFLT *) pffft_aligned_malloc(FFTsize*(sizeof(float)));
      memset(csound->vdsp_buffer,0, FFTsize*(sizeof(float)));
      csound->FFT_max_size = FFTsize;
    }
  }
}

static
void pffft_RealFFT(CSOUND *csound,
                   int32_t FFTsize,MYFLT *sig,
                   int32_t d){
  int32_t i;
  float s, *buf;
  int32_t M = ConvertFFTSize(csound, FFTsize);
  pffft_setup(csound, FFTsize, M);
  buf = (float *)csound->vdsp_buffer;
  for(i=0;i<FFTsize;i++)
    buf[i] = sig[i];
  pffft_transform_ordered(csound->setup[M],
                          buf,buf,NULL,d);
  s = (d == PFFFT_BACKWARD ? (MYFLT)FFTsize : FL(1.0));
  for(i=0;i<FFTsize;i++)
    sig[i] = buf[i]/s;
}
#endif
#endif

