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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
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

typedef struct {

    /* Required header: */
    OPDS   h;

    /* Output channels (4, 9 or 16 in use): */
    ARRAYDAT    *tabout;

    /* Input arguments: */
    MYFLT *ain, *kangle, *kelevation;

} AMBICA;

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

typedef struct {

    /* Required header: */
    OPDS   h;

    /* Output channels (up to eight supported here, depending on the
     isetup parameter). */
    ARRAYDAT  *tabout;

    /* Input arguments: */
    MYFLT *isetup;
    ARRAYDAT *tabin;
    uint32_t dim;
} AMBIDA;

/* ------------------------------------------------------------------------- */
#define ROOT27 (5.1961524227066318806)
#define ROOT135d16 (0.72618437741389066597) /* sqrt(135.0/256.0) */

static int32_t ibformenc(CSOUND * csound, AMBIC * p)
{
    /* All we do in here is police our parameters. */
    switch (p->OUTOCOUNT) {
    case 4:
    case 9:
    case 16:
      return OK;
    default:
      return csound->InitError
        (csound, "%s", Str("The numbers of input and output arguments are not valid."));
  }
}

static int32_t ibformenc_a(CSOUND * csound, AMBICA * p)
{
    if (UNLIKELY(p->tabout->data==NULL || p->tabout->dimensions!=1))
      return csound->InitError(csound,
                               "%s", Str("array not initialised in ambibformenc1"));

    /* All we do in here is police our parameters. */
    switch (p->tabout->sizes[0]) {
    case 4:
    case 9:
    case 16:
      return OK;
    default:
      return csound->InitError
        (csound, "%s", Str("The numbers of input and output arguments are not valid."));
  }
}

static int32_t
abformenc(CSOUND * csound, AMBIC * p) {

    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t sampleCount, sampleIndex, channelCount, channelIndex;
    double angle, elevation, x, y, z;
    MYFLT coefficients[16], coefficient, * output, * input;
    MYFLT x2, y2, z2;

    /* Find basic mode & angles: */
    sampleCount = CS_KSMPS;
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
      /* FALLTHRU */
    case 9:
      /* Second order. */
      coefficients[ 4] = (MYFLT)(1.5 * z2 - 0.5);
      coefficients[ 5] = (MYFLT)(2.0 * z * x);
      coefficients[ 6] = (MYFLT)(2.0 * y * z);
      coefficients[ 7] = (MYFLT)(x2 - y2);
      coefficients[ 8] = (MYFLT)(2.0 * x * y);
      /* Deliberately no break;. */
      /* FALLTHRU */
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
    if (UNLIKELY(early)) sampleCount -= early;
    for (channelIndex = 0; channelIndex < channelCount; channelIndex++) {
      coefficient = coefficients[channelIndex];
      input = p->ain;
      output = p->aouts[channelIndex];
      if (UNLIKELY(offset)) memset(output, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) memset(&output[sampleCount], '\0', early*sizeof(MYFLT));
      for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++)
        output[sampleIndex] = coefficient * input[sampleIndex];
    }

    return OK;

}

static int32_t
abformenc_a(CSOUND * csound, AMBICA * p) {
   IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t sampleCount, sampleIndex, channelCount, channelIndex, ksmps;
    double angle, elevation, x, y, z;
    MYFLT coefficients[16], coefficient, * output, * input;
    MYFLT x2, y2, z2;

    /* Find basic mode & angles: */
    ksmps = sampleCount = CS_KSMPS;
    channelCount = p->tabout->sizes[0];
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
      /* FALLTHRU */
    case 9:
      /* Second order. */
      coefficients[ 4] = (MYFLT)(1.5 * z2 - 0.5);
      coefficients[ 5] = (MYFLT)(2.0 * z * x);
      coefficients[ 6] = (MYFLT)(2.0 * y * z);
      coefficients[ 7] = (MYFLT)(x2 - y2);
      coefficients[ 8] = (MYFLT)(2.0 * x * y);
      /* Deliberately no break;. */
      /* FALLTHRU */
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
    if (UNLIKELY(early)) sampleCount -= early;
    for (channelIndex = 0; channelIndex < channelCount; channelIndex++) {
      coefficient = coefficients[channelIndex];
      input = p->ain;
      output = &p->tabout->data[ksmps*channelIndex];
      if (UNLIKELY(offset)) memset(output, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) memset(&output[sampleCount], '\0', early*sizeof(MYFLT));
      for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++)
        output[sampleIndex] = coefficient * input[sampleIndex];
    }

    return OK;

}

/* ------------------------------------------------------------------------- */

static int32_t
ibformdec(CSOUND * csound, AMBID * p) {
    /* All we do in here is police our parameters. */
    if (UNLIKELY(p->INOCOUNT != 1 + 4 &&
                 p->INOCOUNT != 1 + 9 &&
                 p->INOCOUNT != 1 + 16)) {
      return csound->InitError(csound,
                               "%s", Str("The number of input arguments is not valid."));
    }
    else if (UNLIKELY(*(p->isetup) < 1 || *(p->isetup) > 5)) {
      return csound->InitError(csound,
                               "%s", Str("The isetup value should be between 1 and 5."));
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
                                 "%s", Str("The output channel count does not"
                                     " match the isetup value."));
      }
    }
    return OK;                    /* Never used */
}

static int32_t
abformdec(CSOUND * csound, AMBID * p) {

    /* All assert() calls in here should already have been validated in
       ibformdec(). I've also abused the notation a fair bit, often
       holding scaled values in w, x, y etc. */

    /* (There are some repeated multiplies in the code below, but I
       suggest these aren't removed until everyone is sure the
       unoptimised code is doing the right thing!) */

    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t sampleCount = CS_KSMPS, sampleIndex;
    MYFLT p0, q, u, v, w, x, y, z;

    assert(p->INOCOUNT >= 5);

    switch ((int32_t)*(p->isetup)) {
    case 1: /* Stereo */
      assert(p->OUTOCOUNT == 2);
      /* Use a 90degree stereo decode, equivalent to a M+S microphone
         array at the origin. Works better than front-facing
         arrangements for most purposes, as a composer using this opcode
         probably wants to hear the back stage. */
      if (UNLIKELY(offset)) {
        memset(p->aouts[0], '\0', offset*sizeof(MYFLT));
        memset(p->aouts[1], '\0', offset*sizeof(MYFLT));
      }
      if (UNLIKELY(early)) {
        sampleCount -= early;
        memset(&p->aouts[0][sampleCount], '\0', early*sizeof(MYFLT));
        memset(&p->aouts[1][sampleCount], '\0', early*sizeof(MYFLT));
      }
      for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
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
      if (UNLIKELY(offset)) {
        memset(p->aouts[0], '\0', offset*sizeof(MYFLT));
        memset(p->aouts[1], '\0', offset*sizeof(MYFLT));
        memset(p->aouts[2], '\0', offset*sizeof(MYFLT));
        memset(p->aouts[3], '\0', offset*sizeof(MYFLT));
      }
      if (UNLIKELY(early)) {
        sampleCount -= early;
        memset(&p->aouts[0][sampleCount], '\0', early*sizeof(MYFLT));
        memset(&p->aouts[1][sampleCount], '\0', early*sizeof(MYFLT));
        memset(&p->aouts[2][sampleCount], '\0', early*sizeof(MYFLT));
        memset(&p->aouts[3][sampleCount], '\0', early*sizeof(MYFLT));
      }
      /* Use a first-order 'in-phase' decode. */
      for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
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
      if (UNLIKELY(offset)) {
        memset(p->aouts[0], '\0', offset*sizeof(MYFLT));
        memset(p->aouts[1], '\0', offset*sizeof(MYFLT));
        memset(p->aouts[2], '\0', offset*sizeof(MYFLT));
        memset(p->aouts[3], '\0', offset*sizeof(MYFLT));
      }
     if (UNLIKELY(early)) {
      sampleCount -= early;
      memset(&p->aouts[0][sampleCount], '\0', early*sizeof(MYFLT));
      memset(&p->aouts[1][sampleCount], '\0', early*sizeof(MYFLT));
      memset(&p->aouts[2][sampleCount], '\0', early*sizeof(MYFLT));
      memset(&p->aouts[3][sampleCount], '\0', early*sizeof(MYFLT));
    }
     /* This is a second order decoder provided by Bruce Wiggins. It is
         optimised for high frequency use within a dual-band decoder,
         however it has good a low-frequency response. It is not quite
         'in-phase' but it is not far off. */
      if (p->INOCOUNT == 1 + 4) {
        /* Matrix truncated to first order (not ideal). */
        for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
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
        if (UNLIKELY(offset)) {
          memset(p->aouts[0], '\0', offset*sizeof(MYFLT));
          memset(p->aouts[1], '\0', offset*sizeof(MYFLT));
          memset(p->aouts[2], '\0', offset*sizeof(MYFLT));
          memset(p->aouts[3], '\0', offset*sizeof(MYFLT));
        }
        if (UNLIKELY(early)) {
          sampleCount -= early;
          memset(&p->aouts[0][sampleCount], '\0', early*sizeof(MYFLT));
          memset(&p->aouts[1][sampleCount], '\0', early*sizeof(MYFLT));
          memset(&p->aouts[2][sampleCount], '\0', early*sizeof(MYFLT));
          memset(&p->aouts[3][sampleCount], '\0', early*sizeof(MYFLT));
        }
        for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
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
        if (UNLIKELY(offset))
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(p->aouts[sampleIndex], '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          sampleCount -= early;
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(&p->aouts[sampleIndex][sampleCount], '\0', early*sizeof(MYFLT));
        }
        for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
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
        if (UNLIKELY(offset))
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(p->aouts[sampleIndex], '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          sampleCount -= early;
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(&p->aouts[sampleIndex][sampleCount], '\0', early*sizeof(MYFLT));
        }
        for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
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
        if (UNLIKELY(offset))
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(p->aouts[sampleIndex], '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          sampleCount -= early;
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(&p->aouts[sampleIndex][sampleCount], '\0', early*sizeof(MYFLT));
        }
        /* Third order 'in-phase' / 'controlled opposites' decode: */
        for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
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
      if (UNLIKELY(offset))
        for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
          memset(p->aouts[sampleIndex], '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
          sampleCount -= early;
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(&p->aouts[sampleIndex][sampleCount], '\0', early*sizeof(MYFLT));
        }
      for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
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

static int32_t
ibformdec_a(CSOUND * csound, AMBIDA * p) {
    int32_t dim;
    if (p->tabout->data==NULL || p->tabout->dimensions!=1)
      return csound->InitError(csound,
                               "%s", Str("bformdec1 output array not initialised"));
    dim = p->tabin->sizes[0];
    /* All we do in here is police our parameters. */
    if (UNLIKELY(dim != 4 &&
                 dim != 9 &&
                 dim != 16)) {
      return csound->InitError(csound,
                               "%s", Str("The number of input arguments is not valid."));
    }
    else if (UNLIKELY(*(p->isetup) < 1 || *(p->isetup) > 5)) {
      return csound->InitError(csound,
                               "%s", Str("The isetup value should be between 1 and 5."));
    }
    else {
      p->dim = dim = p->tabout->sizes[0];
      /* Then we check the output arguments. */
      if (*(p->isetup) == 1 && dim == 2) {
        /* Stereo. */
        return OK;
      }
      else if (*(p->isetup) == 2 && dim == 4) {
        /* Quad. */
        return OK;
      }
      else if (*(p->isetup) == 3 && dim == 5) {
        /* Surround 5.0. */
        return OK;
      }
      else if (*(p->isetup) == 4 && dim == 8) {
        /* Octagon. */
        return OK;
      }
      else if (*(p->isetup) == 5 && dim == 8) {
        /* Cube. */
        return OK;
      }
      else {
        return csound->InitError(csound,
                                 "%s", Str("The output channel count does not"
                                     " match the isetup value."));
      }
    }
    return OK;                    /* Never used */
}

static int32_t
abformdec_a(CSOUND * csound, AMBIDA * p) {

    /* All assert() calls in here should already have been validated in
       ibformdec(). I've also abused the notation a fair bit, often
       holding scaled values in w, x, y etc. */

    /* (There are some repeated multiplies in the code below, but I
       suggest these aren't removed until everyone is sure the
       unoptimised code is doing the right thing!) */
    IGN(csound);

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t sampleCount = CS_KSMPS, sampleIndex;
    uint32_t ksmps = sampleCount;
    MYFLT p0, q, u, v, w, x, y, z;
    uint32_t dim = p->dim;
    MYFLT *tabin = p->tabin->data, *tabout = p->tabout->data;

    switch ((int32_t
             )*(p->isetup)) {
    case 1: /* Stereo */
      /* Use a 90degree stereo decode, equivalent to a M+S microphone
         array at the origin. Works better than front-facing
         arrangements for most purposes, as a composer using this opcode
         probably wants to hear the back stage. */
      if (UNLIKELY(offset)) {
        memset(&tabout[0], '\0', offset*sizeof(MYFLT));
        memset(&tabout[ksmps], '\0', offset*sizeof(MYFLT));
      }
      if (UNLIKELY(early)) {
        sampleCount -= early;
        memset(&tabout[sampleCount], '\0', early*sizeof(MYFLT));
        memset(&tabout[ksmps+sampleCount], '\0', early*sizeof(MYFLT));
      }
      for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
        w = tabin[sampleIndex] * SQRT(FL(0.5));
        y = tabin[2*ksmps+sampleIndex] * FL(0.5);
        /* Left: */
        tabout[sampleIndex] = w + y;
        /* Right: */
        tabout[ksmps+sampleIndex] = w - y;
      }
      break;
    case 2: /* Quad */
      if (UNLIKELY(offset)) {
        memset(&tabout[0], '\0', offset*sizeof(MYFLT));
        memset(&tabout[ksmps], '\0', offset*sizeof(MYFLT));
        memset(&tabout[2*ksmps], '\0', offset*sizeof(MYFLT));
        memset(&tabout[3*ksmps], '\0', offset*sizeof(MYFLT));
      }
      if (UNLIKELY(early)) {
        sampleCount -= early;
        memset(&tabout[sampleCount], '\0', early*sizeof(MYFLT));
        memset(&tabout[ksmps+sampleCount], '\0', early*sizeof(MYFLT));
        memset(&tabout[2*ksmps+sampleCount], '\0', early*sizeof(MYFLT));
        memset(&tabout[3*ksmps+sampleCount], '\0', early*sizeof(MYFLT));
      }
      /* Use a first-order 'in-phase' decode. */
      for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
        w = tabin[sampleIndex] * FL(0.35355);
        x = tabin[ksmps+sampleIndex] * FL(0.17677);
        y = tabin[2*ksmps+sampleIndex] * FL(0.17677);
        /* Front left: */
        tabout[sampleIndex] = w + x + y;
        /* Back left: */
        tabout[ksmps+sampleIndex] = w - x + y;
        /* Back right: */
        tabout[2*ksmps+sampleIndex] = w - x - y;
        /* Front right: */
        tabout[3*ksmps+sampleIndex] = w + x - y;
      }
      break;
    case 3: /* 5.0 */
      if (UNLIKELY(offset)) {
        memset(&tabout[0], '\0', offset*sizeof(MYFLT));
        memset(&tabout[ksmps], '\0', offset*sizeof(MYFLT));
        memset(&tabout[2*ksmps], '\0', offset*sizeof(MYFLT));
        memset(&tabout[3*ksmps], '\0', offset*sizeof(MYFLT));
      }
     if (UNLIKELY(early)) {
      sampleCount -= early;
      memset(&tabout[sampleCount], '\0', early*sizeof(MYFLT));
      memset(&tabout[ksmps+sampleCount], '\0', early*sizeof(MYFLT));
      memset(&tabout[2*ksmps+sampleCount], '\0', early*sizeof(MYFLT));
      memset(&tabout[3*ksmps+sampleCount], '\0', early*sizeof(MYFLT));
    }
     /* This is a second order decoder provided by Bruce Wiggins. It is
         optimised for high frequency use within a dual-band decoder,
         however it has good a low-frequency response. It is not quite
         'in-phase' but it is not far off. */
      if (dim == 4) {
        /* Matrix truncated to first order (not ideal). */
        for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
          w = tabin[sampleIndex];
          x = tabin[ksmps+sampleIndex];
          y = tabin[2*ksmps+sampleIndex];
          /* Left: */
          tabout[sampleIndex]
            = w * FL(0.405) + x * FL(0.32) + y * FL(0.31);
          /* Right: */
          tabout[ksmps+sampleIndex]
            = w * FL(0.405) + x * FL(0.32) - y * FL(0.31);
          /* Centre: */
          tabout[2*ksmps+sampleIndex]
            = w * FL(0.085) + x * FL(0.04);
          /* Surround Left: */
          tabout[3*ksmps+sampleIndex]
            = w * FL(0.635) - x * FL(0.335) + y * FL(0.28);
          /* Surround Right: */
          tabout[4*ksmps+sampleIndex]
            = w * FL(0.635) - x * FL(0.335) - y * FL(0.28);
        }
      }
      else {
        /* This is the full matrix. */
        if (UNLIKELY(offset)) {
          memset(&tabout[0], '\0', offset*sizeof(MYFLT));
          memset(&tabout[ksmps], '\0', offset*sizeof(MYFLT));
          memset(&tabout[2*ksmps], '\0', offset*sizeof(MYFLT));
          memset(&tabout[3*ksmps], '\0', offset*sizeof(MYFLT));
        }
        if (UNLIKELY(early)) {
          sampleCount -= early;
          memset(&tabout[0+sampleCount], '\0', early*sizeof(MYFLT));
          memset(&tabout[ksmps+sampleCount], '\0', early*sizeof(MYFLT));
          memset(&tabout[2*ksmps+sampleCount], '\0', early*sizeof(MYFLT));
          memset(&tabout[3*ksmps+sampleCount], '\0', early*sizeof(MYFLT));
        }
        for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
          w = tabin[sampleIndex];
          x = tabin[ksmps+sampleIndex];
          y = tabin[2*ksmps+sampleIndex];
          u = tabin[7*ksmps+sampleIndex];
          v = tabin[8*ksmps+sampleIndex];
          /* Left: */
          tabout[sampleIndex]
            = (w * FL(0.405) + x * FL(0.32) + y * FL(0.31)
               + u * FL(0.085) + v * FL(0.125));
          /* Right: */
          tabout[ksmps+sampleIndex]
            = (w * FL(0.405) + x * FL(0.32) - y * FL(0.31)
               + u * FL(0.085) - v * FL(0.125));
          /* Centre: */
          tabout[2*ksmps+sampleIndex]
            = (w * FL(0.085) + x * FL(0.04)
               + u * FL(0.045));
          /* Surround Left: */
          tabout[3*ksmps+sampleIndex]
            = (w * FL(0.635) - x * FL(0.335) + y * FL(0.28)
               - u * FL(0.08) + v * FL(0.08));
          /* Surround Right: */
          tabout[4*ksmps+sampleIndex]
            = (w * FL(0.635) - x * FL(0.335) - y * FL(0.28)
               - u * FL(0.08) - v * FL(0.08));
        }
      }
      break;
    case 4: /* Octagon: */
      if (p->tabin->sizes[0] == 4) {
        /* First order 'in-phase' decode: */
        if (UNLIKELY(offset))
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(&tabout[ksmps*sampleIndex], '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          sampleCount -= early;
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(&tabout[ksmps*sampleIndex+sampleCount], '\0',
                   early*sizeof(MYFLT));
        }
        for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
          w = tabin[sampleIndex] * FL(0.17677);
          x = tabin[ksmps+sampleIndex];
          y = tabin[2*ksmps+sampleIndex];
          /* About 11 o'clock: */
          tabout[sampleIndex] = w + x * FL(0.11548) + y * FL(0.04783);
          /* About 10 o'clock: */
          tabout[ksmps+sampleIndex] = w + x * FL(0.04783) + y * FL(0.11546);
          /* About 8 o'clock: */
          tabout[2*ksmps+sampleIndex] = w - x * FL(0.04783) + y * FL(0.11546);
          /* About 7 o'clock: */
          tabout[3*ksmps+sampleIndex] = w - x * FL(0.11548) + y * FL(0.04783);
          /* About 5 o'clock: */
          tabout[4*ksmps+sampleIndex] = w - x * FL(0.11548) - y * FL(0.04783);
          /* About 4 o'clock: */
          tabout[5*ksmps+sampleIndex] = w - x * FL(0.04783) - y * FL(0.11546);
          /* About 2 o'clock: */
          tabout[6*ksmps+sampleIndex] = w + x * FL(0.04783) - y * FL(0.11546);
          /* About 1 o'clock: */
          tabout[7*ksmps+sampleIndex] = w + x * FL(0.11548) - y * FL(0.04783);
        }
      }
      else if (p->tabin->sizes[0] == 9) {
        /* Second order 'in-phase' / 'controlled opposites' decode: */
        if (UNLIKELY(offset))
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(&tabout[ksmps*sampleIndex], '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          sampleCount -= early;
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(&tabout[ksmps*sampleIndex+sampleCount], '\0',
                   early*sizeof(MYFLT));
        }
        for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
          w = tabin[sampleIndex] * FL(0.17677);
          x = tabin[ksmps+sampleIndex];
          y = tabin[2*ksmps+sampleIndex];
          u = tabin[7*ksmps+sampleIndex] * FL(0.03417);
          v = tabin[8*ksmps+sampleIndex] * FL(0.03417);
          /* About 11 o'clock: */
          tabout[sampleIndex]
            = w + x * FL(0.15906) + y * FL(0.06588) + u + v;
          /* About 10 o'clock: */
          tabout[ksmps+sampleIndex]
            = w + x * FL(0.06588) + y * FL(0.15906) - u + v;
          /* About 8 o'clock: */
          tabout[2*ksmps+sampleIndex]
            = w - x * FL(0.06588) + y * FL(0.15906) - u - v;
          /* About 7 o'clock: */
          tabout[3*ksmps+sampleIndex]
            = w - x * FL(0.15906) + y * FL(0.06588) + u - v;
          /* About 5 o'clock: */
          tabout[4*ksmps+sampleIndex]
            = w - x * FL(0.15906) - y * FL(0.06588) + u + v;
          /* About 4 o'clock: */
          tabout[5*ksmps+sampleIndex]
            = w - x * FL(0.06588) - y * FL(0.15906) - u + v;
          /* About 2 o'clock: */
          tabout[6*ksmps+sampleIndex]
            = w + x * FL(0.06588) - y * FL(0.15906) - u - v;
          /* About 1 o'clock: */
          tabout[7*ksmps+sampleIndex]
            = w + x * FL(0.15906) - y * FL(0.06588) + u - v;
        }
      }
      else {
        assert(p->tabin->sizes[0]==16);
        if (UNLIKELY(offset))
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(&tabout[ksmps*sampleIndex], '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          sampleCount -= early;
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(&tabout[ksmps*sampleIndex+sampleCount], '\0',
                   early*sizeof(MYFLT));
        }
        /* Third order 'in-phase' / 'controlled opposites' decode: */
        for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
          w  = tabin[ksmps* 0+sampleIndex] * FL(0.176777);
          x  = tabin[ksmps* 1+sampleIndex];
          y  = tabin[ksmps* 2+sampleIndex];
          u  = tabin[ksmps* 7+sampleIndex] * FL(0.053033);
          v  = tabin[ksmps* 8+sampleIndex] * FL(0.053033);
          p0 = tabin[ksmps*14+sampleIndex];
          q  = tabin[ksmps*15+sampleIndex];
          /* About 11 o'clock: */
          tabout[0+sampleIndex]
            = (w
               + x * FL(0.173227) + y * FL(0.071753)
               + u + v
               + p0 * FL(0.004784) + q * FL(0.011548));
          /* About 10 o'clock: */
          tabout[ksmps+sampleIndex]
            = (w
               + x * FL(0.071753) + y * FL(0.173227)
               - u + v
               - p0 * FL(0.011548) - q * FL(0.004784));
          /* About 8 o'clock: */
          tabout[2*ksmps+sampleIndex]
            = (w
               - x * FL(0.071753) + y * FL(0.173227)
               - u - v
               + p0 * FL(0.004784) - q * FL(0.011548));
          /* About 7 o'clock: */
          tabout[3*ksmps+sampleIndex]
            = (w
               - x * FL(0.173227) + y * FL(0.071753)
               + u - v
               - p0 * FL(0.011548) + q * FL(0.004784));
          /* About 5 o'clock: */
          tabout[ksmps*4+sampleIndex]
            = (w
               - x * FL(0.173227) - y * FL(0.071753)
               + u + v
               - p0 * FL(0.004784) - q * FL(0.011548));
          /* About 4 o'clock: */
          tabout[ksmps*5+sampleIndex]
            = (w
               - x * FL(0.071753) - y * FL(0.173227)
               - u + v
               + p0 * FL(0.011548) + q * FL(0.004784));
          /* About 2 o'clock: */
          tabout[ksmps*6+sampleIndex]
            = (w
               + x * FL(0.071753) - y * FL(0.173227)
               - u - v
               - p0 * FL(0.004784) + q * FL(0.011548));
          /* About 1 o'clock: */
          tabout[ksmps*7+sampleIndex]
            = (w
               + x * FL(0.173227) - y * FL(0.071753)
               + u - v
               + p0 * FL(0.011548) - q * FL(0.004784));
        }
      }
      break;
    case 5: /* Cube: */
      /* First order 'in-phase' decode: */
      if (UNLIKELY(offset))
        for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
          memset(&tabout[ksmps*sampleIndex], '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
          sampleCount -= early;
          for (sampleIndex = 0; sampleIndex<8; sampleIndex++)
            memset(&tabout[ksmps*sampleIndex+sampleCount],
                   '\0', early*sizeof(MYFLT));
        }
      for (sampleIndex = offset; sampleIndex < sampleCount; sampleIndex++) {
        w = tabin[sampleIndex] * FL(0.17677);
        x = tabin[ksmps+sampleIndex] * FL(0.07216);
        y = tabin[2*ksmps+sampleIndex] * FL(0.07216);
        z = tabin[3*ksmps+sampleIndex] * FL(0.07216);
        /* Front left bottom: */
        p->tabout->data[sampleIndex] = w + x + y - z;
        /* Front left top: */
        p->tabout->data[ksmps+sampleIndex] = w + x + y + z;
        /* Back left bottom: */
        p->tabout->data[2*ksmps+sampleIndex] = w - x + y - z;
        /* Back left top: */
        p->tabout->data[3*ksmps+sampleIndex] = w - x + y + z;
        /* Back right bottom: */
        p->tabout->data[ksmps*4+sampleIndex] = w - x - y - z;
        /* Back right top: */
        p->tabout->data[ksmps*5+sampleIndex] = w - x - y + z;
        /* Front right bottom: */
        p->tabout->data[ksmps*6+sampleIndex] = w + x - y - z;
        /* Front right top: */
        p->tabout->data[ksmps*7+sampleIndex] = w + x - y + z;
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
  { "bformenc1.a", S(AMBIC), 0,  "mmmmmmmmmmmmmmmm", "akk",
                (SUBR)ibformenc,  (SUBR)abformenc },         
  { "bformenc1.A", S(AMBIC), 0,  "a[]", "akk",
                (SUBR)ibformenc_a,  (SUBR)abformenc_a },
  { "bformdec1.a", S(AMBID), 0,  "mmmmmmmm", "iy",
    (SUBR)ibformdec,  (SUBR)abformdec },
  { "bformdec1.A", S(AMBIDA), 0,  "a[]", "ia[]",
    (SUBR)ibformdec_a,  (SUBR)abformdec_a },
};

LINKAGE_BUILTIN(ambicode1_localops)
