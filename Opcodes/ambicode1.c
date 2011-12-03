/*
    ambicode.c:

    Copyright (C) 2005 Samuel Groner,
    Institute for Computer Music and Sound Technology, www.icst.net.

    Modified 2008 Richard Furse (richard at muse dot demon dot co dot
    uk). In general, 'in-phase' decodes were selected as these
    generally work alright and are more robust to use by
    beginners. The 5.0 decode is provided by Bruce Wiggins (b dot j
    dot wiggins at derby dot ac dot uk).

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

// #include "csdl.h"
#include "csoundCore.h"        
#include "interlocks.h"
#include <assert.h>
#include <math.h>

/* ------------------------------------------------------------------------- */

typedef struct {

  /* Required header: */
  OPDS   h;

  /* Output channels (4, 9 or 16 in use): */
  MYFLT *aouts[16];

  /* Input arguments: */
  MYFLT *ain, *kangle, *kelevation;

} AMBIC;

/* ------------------------------------------------------------------------- */

typedef struct {

  /* Required header: */
  OPDS   h;

  /* Output channels (up to eight supported here, depending on the
     isetup parameter). */
  MYFLT *aouts[8];

  /* Input arguments: */
  MYFLT *isetup, *ains[VARGMAX];

} AMBID;

/* ------------------------------------------------------------------------- */
#define ROOT27 (5.1961524227066318806)
#define ROOT135d16 (0.72618437741389066597) /* sqrt(135.0/256.0) */

static int
ibformenc(CSOUND * csound, AMBIC * p) {
  /* All we do in here is police our parameters. */
  switch (p->OUTOCOUNT) {
  case 4:
  case 9:
  case 16:
    return OK;
  default:
    return csound->InitError
      (csound, Str("The numbers of input and output arguments are not valid."));
  }
}

static int
abformenc(CSOUND * csound, AMBIC * p) {

  int sampleCount, sampleIndex, channelCount, channelIndex;
  double angle, elevation, x, y, z;
  MYFLT coefficients[16], coefficient, * output, * input;
  MYFLT x2, y2, z2;

  /* Find basic mode & angles: */
  sampleCount = csound->ksmps;
  channelCount = p->OUTOCOUNT;
  angle = (double)(*(p->kangle)) * (PI / 180.0);
  elevation = (double)(*(p->kelevation)) * (PI / 180.0);

  /* Find direction cosines: */
  x  = cos(elevation);
  y  = x;
  z  = sin(elevation);
  x *= cos(angle);
  y *= sin(angle);
  x2 = x * x;
  y2 = y * y;
  z2 = z * z;

  /* Find directional coefficients: */
  switch (channelCount) {
  case 16:
    /* Third order. */
    coefficients[ 9] = (MYFLT)((2.5 * z2 - 1.5) * z);
    coefficients[10] = (MYFLT)(ROOT135d16 * x * (5.0 * z2 - 1));
    coefficients[11] = (MYFLT)(ROOT135d16 * y * (5.0 * z2 - 1));
    coefficients[12] = (MYFLT)(0.5*ROOT27 * z * (x2 - y2));
    coefficients[13] = (MYFLT)(ROOT27 * x * y * z);
    coefficients[14] = (MYFLT)(x * (x2 - 3.0 * y2));
    coefficients[15] = (MYFLT)(y * (3.0 * x2 - y2));
    /* Deliberately no break;. */
  case 9:
    /* Second order. */
    coefficients[ 4] = (MYFLT)(1.5 * z2 - 0.5);
    coefficients[ 5] = (MYFLT)(2.0 * z * x);
    coefficients[ 6] = (MYFLT)(2.0 * y * z);
    coefficients[ 7] = (MYFLT)(x2 - y2);
    coefficients[ 8] = (MYFLT)(2.0 * x * y);
    /* Deliberately no break;. */
  case 4:
    /* Zero and first order. */
    coefficients[ 0] = SQRT(FL(0.5));
    coefficients[ 1] = (MYFLT)x;
    coefficients[ 2] = (MYFLT)y;
    coefficients[ 3] = (MYFLT)z;
    break;
  default:
    /* Should never be reached as this is policed at init time. */
    assert(0);
  }
  /* (There are some repeated multiplies in the code above, but I
     suggest these aren't removed until everyone is sure the
     unoptimised code is doing the right thing!) */

  /* Process channels: */
  for (channelIndex = 0; channelIndex < channelCount; channelIndex++) {
    coefficient = coefficients[channelIndex];
    input = p->ain;
    output = p->aouts[channelIndex];
    for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++)
      output[sampleIndex] = coefficient * input[sampleIndex];
  }

  return OK;

}

/* ------------------------------------------------------------------------- */

static int
ibformdec(CSOUND * csound, AMBID * p) {
  /* All we do in here is police our parameters. */
    if (UNLIKELY(p->INOCOUNT != 1 + 4 && p->INOCOUNT != 1 + 9 && p->INOCOUNT != 1 + 16)) {
    return csound->InitError(csound,
                             Str("The number of input arguments is not valid."));
  }
    else if (UNLIKELY(*(p->isetup) < 1 || *(p->isetup) > 5)) {
    return csound->InitError(csound,
                             Str("The isetup value should be between 1 and 5."));
  }
  else {
    /* Then we check the output arguments. */
    if (*(p->isetup) == 1 && p->OUTOCOUNT == 2) {
      /* Stereo. */
      return OK;
    }
    else if (*(p->isetup) == 2 && p->OUTOCOUNT == 4) {
      /* Quad. */
      return OK;
    }
    else if (*(p->isetup) == 3 && p->OUTOCOUNT == 5) {
      /* Surround 5.0. */
      return OK;
    }
    else if (*(p->isetup) == 4 && p->OUTOCOUNT == 8) {
      /* Octagon. */
      return OK;
    }
    else if (*(p->isetup) == 5 && p->OUTOCOUNT == 8) {
      /* Cube. */
      return OK;
    }
    else {
      return csound->InitError(csound,
                                Str("The output channel count does not"
                                    " match the isetup value."));
    }
  }
  return OK;                    /* Never used */
}

static int
abformdec(CSOUND * csound, AMBID * p) {

  /* All assert() calls in here should already have been validated in
     ibformdec(). I've also abused the notation a fair bit, often
     holding scaled values in w, x, y etc. */

  /* (There are some repeated multiplies in the code below, but I
     suggest these aren't removed until everyone is sure the
     unoptimised code is doing the right thing!) */

  int sampleCount, sampleIndex;
  MYFLT p0, q, u, v, w, x, y, z;

  sampleCount = csound->ksmps;
  assert(p->INOCOUNT >= 5);

  switch ((int)*(p->isetup)) {
  case 1: /* Stereo */
    assert(p->OUTOCOUNT == 2);
    /* Use a 90degree stereo decode, equivalent to a M+S microphone
       array at the origin. Works better than front-facing
       arrangements for most purposes, as a composer using this opcode
       probably wants to hear the back stage. */
    for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
      w = p->ains[0][sampleIndex] * SQRT(FL(0.5));
      y = p->ains[2][sampleIndex] * FL(0.5);
      /* Left: */
      p->aouts[0][sampleIndex] = w + y;
      /* Right: */
      p->aouts[1][sampleIndex] = w - y;
    }
    break;
  case 2: /* Quad */
    assert(p->OUTOCOUNT == 4);
    /* Use a first-order 'in-phase' decode. */
    for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
      w = p->ains[0][sampleIndex] * FL(0.35355);
      x = p->ains[1][sampleIndex] * FL(0.17677);
      y = p->ains[2][sampleIndex] * FL(0.17677);
      /* Front left: */
      p->aouts[0][sampleIndex] = w + x + y;
      /* Back left: */
      p->aouts[1][sampleIndex] = w - x + y;
      /* Back right: */
      p->aouts[2][sampleIndex] = w - x - y;
      /* Front right: */
      p->aouts[3][sampleIndex] = w + x - y;
    }
    break;
  case 3: /* 5.0 */
    assert(p->OUTOCOUNT == 5);
    /* This is a second order decoder provided by Bruce Wiggins. It is
       optimised for high frequency use within a dual-band decoder,
       however it has good a low-frequency response. It isn't quite
       'in-phase' but it is not far off. */
    if (p->INOCOUNT == 1 + 4) {
      /* Matrix truncated to first order (not ideal). */
      for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
        w = p->ains[0][sampleIndex];
        x = p->ains[1][sampleIndex];
        y = p->ains[2][sampleIndex];
        /* Left: */
        p->aouts[0][sampleIndex]
          = w * FL(0.405) + x * FL(0.32) + y * FL(0.31);
        /* Right: */
        p->aouts[1][sampleIndex]
          = w * FL(0.405) + x * FL(0.32) - y * FL(0.31);
        /* Centre: */
        p->aouts[2][sampleIndex]
          = w * FL(0.085) + x * FL(0.04);
        /* Surround Left: */
        p->aouts[3][sampleIndex]
          = w * FL(0.635) - x * FL(0.335) + y * FL(0.28);
        /* Surround Right: */
        p->aouts[4][sampleIndex]
          = w * FL(0.635) - x * FL(0.335) - y * FL(0.28);
      }
    }
    else {
      /* This is the full matrix. */
      for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
        w = p->ains[0][sampleIndex];
        x = p->ains[1][sampleIndex];
        y = p->ains[2][sampleIndex];
        u = p->ains[7][sampleIndex];
        v = p->ains[8][sampleIndex];
        /* Left: */
        p->aouts[0][sampleIndex]
          = (w * FL(0.405) + x * FL(0.32) + y * FL(0.31)
             + u * FL(0.085) + v * FL(0.125));
        /* Right: */
        p->aouts[1][sampleIndex]
          = (w * FL(0.405) + x * FL(0.32) - y * FL(0.31)
             + u * FL(0.085) - v * FL(0.125));
        /* Centre: */
        p->aouts[2][sampleIndex]
          = (w * FL(0.085) + x * FL(0.04)
             + u * FL(0.045));
        /* Surround Left: */
        p->aouts[3][sampleIndex]
          = (w * FL(0.635) - x * FL(0.335) + y * FL(0.28)
             - u * FL(0.08) + v * FL(0.08));
        /* Surround Right: */
        p->aouts[4][sampleIndex]
          = (w * FL(0.635) - x * FL(0.335) - y * FL(0.28)
             - u * FL(0.08) - v * FL(0.08));
      }
    }
    break;
  case 4: /* Octagon: */
    assert(p->OUTOCOUNT == 8);
    if (p->INOCOUNT == 1 + 4) {
      /* First order 'in-phase' decode: */
      for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
        w = p->ains[0][sampleIndex] * FL(0.17677);
        x = p->ains[1][sampleIndex];
        y = p->ains[2][sampleIndex];
        /* About 11 o'clock: */
        p->aouts[0][sampleIndex] = w + x * FL(0.11548) + y * FL(0.04783);
        /* About 10 o'clock: */
        p->aouts[1][sampleIndex] = w + x * FL(0.04783) + y * FL(0.11546);
        /* About 8 o'clock: */
        p->aouts[2][sampleIndex] = w - x * FL(0.04783) + y * FL(0.11546);
        /* About 7 o'clock: */
        p->aouts[3][sampleIndex] = w - x * FL(0.11548) + y * FL(0.04783);
        /* About 5 o'clock: */
        p->aouts[4][sampleIndex] = w - x * FL(0.11548) - y * FL(0.04783);
        /* About 4 o'clock: */
        p->aouts[5][sampleIndex] = w - x * FL(0.04783) - y * FL(0.11546);
        /* About 2 o'clock: */
        p->aouts[6][sampleIndex] = w + x * FL(0.04783) - y * FL(0.11546);
        /* About 1 o'clock: */
        p->aouts[7][sampleIndex] = w + x * FL(0.11548) - y * FL(0.04783);
      }
    }
    else if (p->INOCOUNT == 1 + 9) {
      /* Second order 'in-phase' / 'controlled opposites' decode: */
      for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
        w = p->ains[0][sampleIndex] * FL(0.17677);
        x = p->ains[1][sampleIndex];
        y = p->ains[2][sampleIndex];
        u = p->ains[7][sampleIndex] * FL(0.03417);
        v = p->ains[8][sampleIndex] * FL(0.03417);
        /* About 11 o'clock: */
        p->aouts[0][sampleIndex]
          = w + x * FL(0.15906) + y * FL(0.06588) + u + v;
        /* About 10 o'clock: */
        p->aouts[1][sampleIndex]
          = w + x * FL(0.06588) + y * FL(0.15906) - u + v;
        /* About 8 o'clock: */
        p->aouts[2][sampleIndex]
          = w - x * FL(0.06588) + y * FL(0.15906) - u - v;
        /* About 7 o'clock: */
        p->aouts[3][sampleIndex]
          = w - x * FL(0.15906) + y * FL(0.06588) + u - v;
        /* About 5 o'clock: */
        p->aouts[4][sampleIndex]
          = w - x * FL(0.15906) - y * FL(0.06588) + u + v;
        /* About 4 o'clock: */
        p->aouts[5][sampleIndex]
          = w - x * FL(0.06588) - y * FL(0.15906) - u + v;
        /* About 2 o'clock: */
        p->aouts[6][sampleIndex]
          = w + x * FL(0.06588) - y * FL(0.15906) - u - v;
        /* About 1 o'clock: */
        p->aouts[7][sampleIndex]
          = w + x * FL(0.15906) - y * FL(0.06588) + u - v;
      }
    }
    else {
      assert(p->INOCOUNT == 1 + 16);
      /* Third order 'in-phase' / 'controlled opposites' decode: */
      for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
        w  = p->ains[ 0][sampleIndex] * FL(0.176777);
        x  = p->ains[ 1][sampleIndex];
        y  = p->ains[ 2][sampleIndex];
        u  = p->ains[ 7][sampleIndex] * FL(0.053033);
        v  = p->ains[ 8][sampleIndex] * FL(0.053033);
        p0 = p->ains[14][sampleIndex];
        q  = p->ains[15][sampleIndex];
        /* About 11 o'clock: */
        p->aouts[0][sampleIndex]
          = (w
             + x * FL(0.173227) + y * FL(0.071753)
             + u + v
             + p0 * FL(0.004784) + q * FL(0.011548));
        /* About 10 o'clock: */
        p->aouts[1][sampleIndex]
          = (w
             + x * FL(0.071753) + y * FL(0.173227)
             - u + v
             - p0 * FL(0.011548) - q * FL(0.004784));
        /* About 8 o'clock: */
        p->aouts[2][sampleIndex]
          = (w
             - x * FL(0.071753) + y * FL(0.173227)
             - u - v
             + p0 * FL(0.004784) - q * FL(0.011548));
        /* About 7 o'clock: */
        p->aouts[3][sampleIndex]
          = (w
             - x * FL(0.173227) + y * FL(0.071753)
             + u - v
             - p0 * FL(0.011548) + q * FL(0.004784));
        /* About 5 o'clock: */
        p->aouts[4][sampleIndex]
          = (w
             - x * FL(0.173227) - y * FL(0.071753)
             + u + v
             - p0 * FL(0.004784) - q * FL(0.011548));
        /* About 4 o'clock: */
        p->aouts[5][sampleIndex]
          = (w
             - x * FL(0.071753) - y * FL(0.173227)
             - u + v
             + p0 * FL(0.011548) + q * FL(0.004784));
        /* About 2 o'clock: */
        p->aouts[6][sampleIndex]
          = (w
             + x * FL(0.071753) - y * FL(0.173227)
             - u - v
             - p0 * FL(0.004784) + q * FL(0.011548));
        /* About 1 o'clock: */
        p->aouts[7][sampleIndex]
          = (w
             + x * FL(0.173227) - y * FL(0.071753)
             + u - v
             + p0 * FL(0.011548) - q * FL(0.004784));
      }
    }
    break;
  case 5: /* Cube: */
    assert(p->OUTOCOUNT == 8);
    /* First order 'in-phase' decode: */
    for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
      w = p->ains[0][sampleIndex] * FL(0.17677);
      x = p->ains[1][sampleIndex] * FL(0.07216);
      y = p->ains[2][sampleIndex] * FL(0.07216);
      z = p->ains[3][sampleIndex] * FL(0.07216);
      /* Front left bottom: */
      p->aouts[0][sampleIndex] = w + x + y - z;
      /* Front left top: */
      p->aouts[1][sampleIndex] = w + x + y + z;
      /* Back left bottom: */
      p->aouts[2][sampleIndex] = w - x + y - z;
      /* Back left top: */
      p->aouts[3][sampleIndex] = w - x + y + z;
      /* Back right bottom: */
      p->aouts[4][sampleIndex] = w - x - y - z;
      /* Back right top: */
      p->aouts[5][sampleIndex] = w - x - y + z;
      /* Front right bottom: */
      p->aouts[6][sampleIndex] = w + x - y - z;
      /* Front right top: */
      p->aouts[7][sampleIndex] = w + x - y + z;
    }
    break;
  default:
    assert(0);
  }

  return OK;

}

/* ------------------------------------------------------------------------- */

#define S(x) sizeof(x)

static OENTRY ambicode1_localops[] = {
  { "bformenc1", S(AMBIC), 5, "mmmmmmmmmmmmmmmm", "akk",
                (SUBR)ibformenc, NULL, (SUBR)abformenc },
  { "bformdec1", S(AMBID), 5, "mmmmmmmm", "iy",
    (SUBR)ibformdec, NULL, (SUBR)abformdec },
};

LINKAGE1(ambicode1_localops)

