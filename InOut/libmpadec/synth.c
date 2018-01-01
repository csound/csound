/*
 *  mpadec - MPEG audio decoder
 *  Copyright (C) 2002-2004 Dmitriy Startsev (dstartsev@rambler.ru)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* $Id: synth.c,v 1.3 2004/08/03 05:22:22 metal_man Exp $ */

#include "mpadec_internal.h"

#define ROUND(x) (floor((x) + 0.5))
#define LROUND(x) ((int32_t)(ROUND(x)))
#define LLROUND(x) ((int64_t)(ROUND(x)))

static const MYFLT costab[32] = {
  0.50060299823519630134550410676638, 0.50547095989754365998444458560696,
  0.51544730992262454697495130564925, 0.53104259108978417447573393235719,
  0.55310389603444452782938083813705, 0.58293496820613387367383070125262,
  0.62250412303566481615725615676281, 0.67480834145500574602596871104104,
  0.74453627100229844977698119197295, 0.83934964541552703873926374662543,
  0.97256823786196069369768941405256, 1.16943993343288495515577028404220,
  1.48416461631416627724332693742810, 2.05778100995341155085655447971040,
  3.40760841846871878570119133345910, 10.1900081235480568112121092010360,
  0.50241928618815570551167011928012, 0.52249861493968888062857531905669,
  0.56694403481635770368053791515488, 0.64682178335999012954836011165200,
  0.78815462345125022473398248719742, 1.06067768599034747134045174723310,
  1.72244709823833392781591536415660, 5.10114861868916385810624549234540,
  0.50979557910415916894193980398784, 0.60134488693504528054372182390922,
  0.89997622313641570463850954094189, 2.56291544774150617879608629617770,
  0.54119610014619698439972320536639, 1.30656296487637652785664317342720,
  0.70710678118654752440084436210485, 0.0
};

static void dct64(MYFLT *outptr0, MYFLT *outptr1, MYFLT *samples)
{
  MYFLT tmp1[32], tmp2[32];

  {
    MYFLT *in = samples;

    tmp1[0] = in[0] + in[31];
    tmp1[1] = in[1] + in[30];
    tmp1[31] = (in[0] - in[31])*costab[0];
    tmp1[30] = (in[1] - in[30])*costab[1];

    tmp1[2] = in[2] + in[29];
    tmp1[3] = in[3] + in[28];
    tmp1[29] = (in[2] - in[29])*costab[2];
    tmp1[28] = (in[3] - in[28])*costab[3];

    tmp1[4] = in[4] + in[27];
    tmp1[5] = in[5] + in[26];
    tmp1[27] = (in[4] - in[27])*costab[4];
    tmp1[26] = (in[5] - in[26])*costab[5];

    tmp1[6] = in[6] + in[25];
    tmp1[7] = in[7] + in[24];
    tmp1[25] = (in[6] - in[25])*costab[6];
    tmp1[24] = (in[7] - in[24])*costab[7];

    tmp1[8] = in[8] + in[23];
    tmp1[9] = in[9] + in[22];
    tmp1[23] = (in[8] - in[23])*costab[8];
    tmp1[22] = (in[9] - in[22])*costab[9];

    tmp1[10] = in[10] + in[21];
    tmp1[11] = in[11] + in[20];
    tmp1[21] = (in[10] - in[21])*costab[10];
    tmp1[20] = (in[11] - in[20])*costab[11];

    tmp1[12] = in[12] + in[19];
    tmp1[13] = in[13] + in[18];
    tmp1[19] = (in[12] - in[19])*costab[12];
    tmp1[18] = (in[13] - in[18])*costab[13];

    tmp1[14] = in[14] + in[17];
    tmp1[15] = in[15] + in[16];
    tmp1[17] = (in[14] - in[17])*costab[14];
    tmp1[16] = (in[15] - in[16])*costab[15];
  }
  {
    tmp2[0] = tmp1[0] + tmp1[15];
    tmp2[1] = tmp1[1] + tmp1[14];
    tmp2[15] = (tmp1[0] - tmp1[15])*costab[16 + 0];
    tmp2[14] = (tmp1[1] - tmp1[14])*costab[16 + 1];

    tmp2[2] = tmp1[2] + tmp1[13];
    tmp2[3] = tmp1[3] + tmp1[12];
    tmp2[13] = (tmp1[2] - tmp1[13])*costab[16 + 2];
    tmp2[12] = (tmp1[3] - tmp1[12])*costab[16 + 3];

    tmp2[4] = tmp1[4] + tmp1[11];
    tmp2[5] = tmp1[5] + tmp1[10];
    tmp2[11] = (tmp1[4] - tmp1[11])*costab[16 + 4];
    tmp2[10] = (tmp1[5] - tmp1[10])*costab[16 + 5];

    tmp2[6] = tmp1[6] + tmp1[9];
    tmp2[7] = tmp1[7] + tmp1[8];
    tmp2[9] = (tmp1[6] - tmp1[9])*costab[16 + 6];
    tmp2[8] = (tmp1[7] - tmp1[8])*costab[16 + 7];

    tmp2[16] = tmp1[16] + tmp1[31];
    tmp2[17] = tmp1[17] + tmp1[30];
    tmp2[31] = (tmp1[31] - tmp1[16])*costab[16 + 0];
    tmp2[30] = (tmp1[30] - tmp1[17])*costab[16 + 1];

    tmp2[18] = tmp1[18] + tmp1[29];
    tmp2[19] = tmp1[19] + tmp1[28];
    tmp2[29] = (tmp1[29] - tmp1[18])*costab[16 + 2];
    tmp2[28] = (tmp1[28] - tmp1[19])*costab[16 + 3];

    tmp2[20] = tmp1[20] + tmp1[27];
    tmp2[21] = tmp1[21] + tmp1[26];
    tmp2[27] = (tmp1[27] - tmp1[20])*costab[16 + 4];
    tmp2[26] = (tmp1[26] - tmp1[21])*costab[16 + 5];

    tmp2[22] = tmp1[22] + tmp1[25];
    tmp2[23] = tmp1[23] + tmp1[24];
    tmp2[25] = (tmp1[25] - tmp1[22])*costab[16 + 6];
    tmp2[24] = (tmp1[24] - tmp1[23])*costab[16 + 7];
  }
  {
    tmp1[0] = tmp2[0] + tmp2[7];
    tmp1[7] = (tmp2[0] - tmp2[7])*costab[16 + 8 + 0];
    tmp1[1] = tmp2[1] + tmp2[6];
    tmp1[6] = (tmp2[1] - tmp2[6])*costab[16 + 8 + 1];
    tmp1[2] = tmp2[2] + tmp2[5];
    tmp1[5] = (tmp2[2] - tmp2[5])*costab[16 + 8 + 2];
    tmp1[3] = tmp2[3] + tmp2[4];
    tmp1[4] = (tmp2[3] - tmp2[4])*costab[16 + 8 + 3];

    tmp1[8] = tmp2[8] + tmp2[15];
    tmp1[15] = (tmp2[15] - tmp2[8])*costab[16 + 8 + 0];
    tmp1[9] = tmp2[9] + tmp2[14];
    tmp1[14] = (tmp2[14] - tmp2[9])*costab[16 + 8 + 1];
    tmp1[10] = tmp2[10] + tmp2[13];
    tmp1[13] = (tmp2[13] - tmp2[10])*costab[16 + 8 + 2];
    tmp1[11] = tmp2[11] + tmp2[12];
    tmp1[12] = (tmp2[12] - tmp2[11])*costab[16 + 8 + 3];

    tmp1[16] = tmp2[16] + tmp2[23];
    tmp1[23] = (tmp2[16] - tmp2[23])*costab[16 + 8 + 0];
    tmp1[17] = tmp2[17] + tmp2[22];
    tmp1[22] = (tmp2[17] - tmp2[22])*costab[16 + 8 + 1];
    tmp1[18] = tmp2[18] + tmp2[21];
    tmp1[21] = (tmp2[18] - tmp2[21])*costab[16 + 8 + 2];
    tmp1[19] = tmp2[19] + tmp2[20];
    tmp1[20] = (tmp2[19] - tmp2[20])*costab[16 + 8 + 3];

    tmp1[24] = tmp2[24] + tmp2[31];
    tmp1[31] = (tmp2[31] - tmp2[24])*costab[16 + 8 + 0];
    tmp1[25] = tmp2[25] + tmp2[30];
    tmp1[30] = (tmp2[30] - tmp2[25])*costab[16 + 8 + 1];
    tmp1[26] = tmp2[26] + tmp2[29];
    tmp1[29] = (tmp2[29] - tmp2[26])*costab[16 + 8 + 2];
    tmp1[27] = tmp2[27] + tmp2[28];
    tmp1[28] = (tmp2[28] - tmp2[27])*costab[16 + 8 + 3];
  }
  {
    tmp2[0] = tmp1[0] + tmp1[3];
    tmp2[3] = (tmp1[0] - tmp1[3])*costab[16 + 8 + 4 + 0];
    tmp2[1] = tmp1[1] + tmp1[2];
    tmp2[2] = (tmp1[1] - tmp1[2])*costab[16 + 8 + 4 + 1];

    tmp2[4] = tmp1[4] + tmp1[7];
    tmp2[7] = (tmp1[7] - tmp1[4])*costab[16 + 8 + 4 + 0];
    tmp2[5] = tmp1[5] + tmp1[6];
    tmp2[6] = (tmp1[6] - tmp1[5])*costab[16 + 8 + 4 + 1];

    tmp2[8] = tmp1[8] + tmp1[11];
    tmp2[11] = (tmp1[8] - tmp1[11])*costab[16 + 8 + 4 + 0];
    tmp2[9] = tmp1[9] + tmp1[10];
    tmp2[10] = (tmp1[9] - tmp1[10])*costab[16 + 8 + 4 + 1];

    tmp2[12] = tmp1[12] + tmp1[15];
    tmp2[15] = (tmp1[15] - tmp1[12])*costab[16 + 8 + 4 + 0];
    tmp2[13] = tmp1[13] + tmp1[14];
    tmp2[14] = (tmp1[14] - tmp1[13])*costab[16 + 8 + 4 + 1];

    tmp2[16] = tmp1[16] + tmp1[19];
    tmp2[19] = (tmp1[16] - tmp1[19])*costab[16 + 8 + 4 + 0];
    tmp2[17] = tmp1[17] + tmp1[18];
    tmp2[18] = (tmp1[17] - tmp1[18])*costab[16 + 8 + 4 + 1];

    tmp2[20] = tmp1[20] + tmp1[23];
    tmp2[23] = (tmp1[23] - tmp1[20])*costab[16 + 8 + 4 + 0];
    tmp2[21] = tmp1[21] + tmp1[22];
    tmp2[22] = (tmp1[22] - tmp1[21])*costab[16 + 8 + 4 + 1];

    tmp2[24] = tmp1[24] + tmp1[27];
    tmp2[27] = (tmp1[24] - tmp1[27])*costab[16 + 8 + 4 + 0];
    tmp2[25] = tmp1[25] + tmp1[26];
    tmp2[26] = (tmp1[25] - tmp1[26])*costab[16 + 8 + 4 + 1];

    tmp2[28] = tmp1[28] + tmp1[31];
    tmp2[31] = (tmp1[31] - tmp1[28])*costab[16 + 8 + 4 + 0];
    tmp2[29] = tmp1[29] + tmp1[30];
    tmp2[30] = (tmp1[30] - tmp1[29])*costab[16 + 8 + 4 + 1];
  }
  {
    tmp1[0] = tmp2[0] + tmp2[1];
    tmp1[1] = (tmp2[0] - tmp2[1])*costab[16 + 8 + 4 + 2];
    tmp1[2] = tmp2[2] + tmp2[3];
    tmp1[3] = (tmp2[3] - tmp2[2])*costab[16 + 8 + 4 + 2];
    tmp1[2] += tmp1[3];

    tmp1[4] = tmp2[4] + tmp2[5];
    tmp1[5] = (tmp2[4] - tmp2[5])*costab[16 + 8 + 4 + 2];
    tmp1[6] = tmp2[6] + tmp2[7];
    tmp1[7] = (tmp2[7] - tmp2[6])*costab[16 + 8 + 4 + 2];
    tmp1[6] += tmp1[7];
    tmp1[4] += tmp1[6];
    tmp1[6] += tmp1[5];
    tmp1[5] += tmp1[7];

    tmp1[8] = tmp2[8] + tmp2[9];
    tmp1[9] = (tmp2[8] - tmp2[9])*costab[16 + 8 + 4 + 2];
    tmp1[10] = tmp2[10] + tmp2[11];
    tmp1[11] = (tmp2[11] - tmp2[10])*costab[16 + 8 + 4 + 2];
    tmp1[10] += tmp1[11];

    tmp1[12] = tmp2[12] + tmp2[13];
    tmp1[13] = (tmp2[12] - tmp2[13])*costab[16 + 8 + 4 + 2];
    tmp1[14] = tmp2[14] + tmp2[15];
    tmp1[15] = (tmp2[15] - tmp2[14])*costab[16 + 8 + 4 + 2];
    tmp1[14] += tmp1[15];
    tmp1[12] += tmp1[14];
    tmp1[14] += tmp1[13];
    tmp1[13] += tmp1[15];

    tmp1[16] = tmp2[16] + tmp2[17];
    tmp1[17] = (tmp2[16] - tmp2[17])*costab[16 + 8 + 4 + 2];
    tmp1[18] = tmp2[18] + tmp2[19];
    tmp1[19] = (tmp2[19] - tmp2[18])*costab[16 + 8 + 4 + 2];
    tmp1[18] += tmp1[19];

    tmp1[20] = tmp2[20] + tmp2[21];
    tmp1[21] = (tmp2[20] - tmp2[21])*costab[16 + 8 + 4 + 2];
    tmp1[22] = tmp2[22] + tmp2[23];
    tmp1[23] = (tmp2[23] - tmp2[22])*costab[16 + 8 + 4 + 2];
    tmp1[22] += tmp1[23];
    tmp1[20] += tmp1[22];
    tmp1[22] += tmp1[21];
    tmp1[21] += tmp1[23];

    tmp1[24] = tmp2[24] + tmp2[25];
    tmp1[25] = (tmp2[24] - tmp2[25])*costab[16 + 8 + 4 + 2];
    tmp1[26] = tmp2[26] + tmp2[27];
    tmp1[27] = (tmp2[27] - tmp2[26])*costab[16 + 8 + 4 + 2];
    tmp1[26] += tmp1[27];

    tmp1[28] = tmp2[28] + tmp2[29];
    tmp1[29] = (tmp2[28] - tmp2[29])*costab[16 + 8 + 4 + 2];
    tmp1[30] = tmp2[30] + tmp2[31];
    tmp1[31] = (tmp2[31] - tmp2[30])*costab[16 + 8 + 4 + 2];
    tmp1[30] += tmp1[31];
    tmp1[28] += tmp1[30];
    tmp1[30] += tmp1[29];
    tmp1[29] += tmp1[31];
  }
  {
    MYFLT tmp, *out0 = outptr0, *out1 = outptr1;

    out0[16*16] = tmp1[0];
    out0[12*16] = tmp1[4];
    out0[8*16] = tmp1[2];
    out0[4*16] = tmp1[6];
    out0[0*16] = tmp1[1];
    out1[0*16] = tmp1[1];
    out1[4*16] = tmp1[5];
    out1[8*16] = tmp1[3];
    out1[12*16] = tmp1[7];

    out0[14*16] = tmp1[8] + tmp1[12];
    out0[10*16] = tmp1[12] + tmp1[10];
    out0[6*16] = tmp1[10] + tmp1[14];
    out0[2*16] = tmp1[14] + tmp1[9];
    out1[2*16] = tmp1[9] + tmp1[13];
    out1[6*16] = tmp1[13] + tmp1[11];
    out1[10*16] = tmp1[11] + tmp1[15];
    out1[14*16] = tmp1[15];

    tmp = tmp1[24] + tmp1[28];
    out0[15*16] = tmp + tmp1[16];
    out0[13*16] = tmp + tmp1[20];
    tmp = tmp1[28] + tmp1[26];
    out0[11*16] = tmp + tmp1[20];
    out0[9*16] = tmp + tmp1[18];
    tmp = tmp1[26] + tmp1[30];
    out0[7*16] = tmp + tmp1[18];
    out0[5*16] = tmp + tmp1[22];
    tmp = tmp1[30] + tmp1[25];
    out0[3*16] = tmp + tmp1[22];
    out0[1*16] = tmp + tmp1[17];
    tmp = tmp1[25] + tmp1[29];
    out1[1*16] = tmp + tmp1[17];
    out1[3*16] = tmp + tmp1[21];
    tmp = tmp1[29] + tmp1[27];
    out1[5*16] = tmp + tmp1[21];
    out1[7*16] = tmp + tmp1[19];
    tmp = tmp1[27] + tmp1[31];
    out1[9*16] = tmp + tmp1[19];
    out1[11*16] = tmp + tmp1[23];
    out1[13*16] = tmp1[23] + tmp1[31];
    out1[15*16] = tmp1[31];
  }
}

static void synth_full(mpadec_t mpadec, MYFLT *bandptr, int channel, MYFLT *buffer)
{
  struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
  unsigned bo;
  MYFLT *b0, (*buf)[0x110];

  if (!channel) {
    mpa->synth_bufoffs--;
    mpa->synth_bufoffs &= 0x0F;
    buf = mpa->synth_buffers[0];
  } else buf = mpa->synth_buffers[1];
  if (mpa->synth_bufoffs & 1) {
    b0 = buf[0];
    bo = mpa->synth_bufoffs;
    dct64(buf[1] + ((mpa->synth_bufoffs + 1) & 0x0F),
          buf[0] + mpa->synth_bufoffs, bandptr);
  } else {
    b0 = buf[1];
    bo = mpa->synth_bufoffs + 1;
    dct64(buf[0] + mpa->synth_bufoffs, buf[1] + (mpa->synth_bufoffs + 1), bandptr);
  }
  {
    int i;
    MYFLT *out = buffer;
    MYFLT *win = mpa->tables.decwin + (16 - bo);

    for (i = 16; i; i--, win += 32, b0 += 16) {
      MYFLT sum = win[0]*b0[0];
      sum -= win[1]*b0[1];
      sum += win[2]*b0[2];
      sum -= win[3]*b0[3];
      sum += win[4]*b0[4];
      sum -= win[5]*b0[5];
      sum += win[6]*b0[6];
      sum -= win[7]*b0[7];
      sum += win[8]*b0[8];
      sum -= win[9]*b0[9];
      sum += win[10]*b0[10];
      sum -= win[11]*b0[11];
      sum += win[12]*b0[12];
      sum -= win[13]*b0[13];
      sum += win[14]*b0[14];
      sum -= win[15]*b0[15];
      *out++ = sum;
    }
    {
      MYFLT sum = win[0]*b0[0];
      sum += win[2]*b0[2];
      sum += win[4]*b0[4];
      sum += win[6]*b0[6];
      sum += win[8]*b0[8];
      sum += win[10]*b0[10];
      sum += win[12]*b0[12];
      sum += win[14]*b0[14];
      *out++ = sum;
      win -= 32; b0 -= 16;
    }
    win += (bo << 1);
    for (i = 15; i; i--, win -= 32, b0 -= 16)
    {
      MYFLT sum = -win[-1]*b0[0];
      sum -= win[-2]*b0[1];
      sum -= win[-3]*b0[2];
      sum -= win[-4]*b0[3];
      sum -= win[-5]*b0[4];
      sum -= win[-6]*b0[5];
      sum -= win[-7]*b0[6];
      sum -= win[-8]*b0[7];
      sum -= win[-9]*b0[8];
      sum -= win[-10]*b0[9];
      sum -= win[-11]*b0[10];
      sum -= win[-12]*b0[11];
      sum -= win[-13]*b0[12];
      sum -= win[-14]*b0[13];
      sum -= win[-15]*b0[14];
      sum -= win[-0]*b0[15];
      *out++ = sum;
    }
  }
}

static void synth_half(mpadec_t mpadec, MYFLT *bandptr, int channel, MYFLT *buffer)
{
  struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
  unsigned bo;
  MYFLT *b0, (*buf)[0x110];

  if (!channel) {
    mpa->synth_bufoffs--;
    mpa->synth_bufoffs &= 0x0F;
    buf = mpa->synth_buffers[0];
  } else buf = mpa->synth_buffers[1];
  if (mpa->synth_bufoffs & 1) {
    b0 = buf[0];
    bo = mpa->synth_bufoffs;
    dct64(buf[1] + ((mpa->synth_bufoffs + 1) & 0x0F),
          buf[0] + mpa->synth_bufoffs, bandptr);
  } else {
    b0 = buf[1];
    bo = mpa->synth_bufoffs + 1;
    dct64(buf[0] + mpa->synth_bufoffs,
          buf[1] + (mpa->synth_bufoffs + 1), bandptr);
  }
  {
    int i;
    MYFLT *out = buffer;
    MYFLT *win = mpa->tables.decwin + (16 - bo);

    for (i = 8; i; i--, win += 64, b0 += 32) {
      MYFLT sum = win[0]*b0[0];
      sum -= win[1]*b0[1];
      sum += win[2]*b0[2];
      sum -= win[3]*b0[3];
      sum += win[4]*b0[4];
      sum -= win[5]*b0[5];
      sum += win[6]*b0[6];
      sum -= win[7]*b0[7];
      sum += win[8]*b0[8];
      sum -= win[9]*b0[9];
      sum += win[10]*b0[10];
      sum -= win[11]*b0[11];
      sum += win[12]*b0[12];
      sum -= win[13]*b0[13];
      sum += win[14]*b0[14];
      sum -= win[15]*b0[15];
      *out++ = sum;
    }
    {
      MYFLT sum = win[0]*b0[0];
      sum += win[2]*b0[2];
      sum += win[4]*b0[4];
      sum += win[6]*b0[6];
      sum += win[8]*b0[8];
      sum += win[10]*b0[10];
      sum += win[12]*b0[12];
      sum += win[14]*b0[14];
      *out++ = sum;
      win -= 64; b0 -= 32;
    }
    win += (bo << 1);
    for (i = 7; i; i--, win -= 64, b0 -= 32)
    {
      MYFLT sum = -win[-1]*b0[0];
      sum -= win[-2]*b0[1];
      sum -= win[-3]*b0[2];
      sum -= win[-4]*b0[3];
      sum -= win[-5]*b0[4];
      sum -= win[-6]*b0[5];
      sum -= win[-7]*b0[6];
      sum -= win[-8]*b0[7];
      sum -= win[-9]*b0[8];
      sum -= win[-10]*b0[9];
      sum -= win[-11]*b0[10];
      sum -= win[-12]*b0[11];
      sum -= win[-13]*b0[12];
      sum -= win[-14]*b0[13];
      sum -= win[-15]*b0[14];
      sum -= win[-0]*b0[15];
      *out++ = sum;
    }
  }
}

/* Full quality */

/* 16 bit, little-endian */

static void synth_full16lmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out++) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767; else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((int8_t *)out)[1] = (int8_t)(tmp >> 8);
  }
}

#define synth_full16lsm synth_full16lmm

static void synth_full16lms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767; else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[0] = ((uint8_t *)out)[2] = (uint8_t)tmp;
    ((int8_t *)out)[1] = ((int8_t *)out)[3] = (int8_t)(tmp >> 8);
  }
}

static void synth_full16lss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767; else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((int8_t *)out)[1] = (int8_t)(tmp >> 8);
  }
}

/* 16 bit, big-endian */

static void synth_full16bmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out++) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767;
    else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[1] = (uint8_t)tmp;
    ((int8_t *)out)[0] = (int8_t)(tmp >> 8);
  }
}

#define synth_full16bsm synth_full16bmm

static void synth_full16bms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767; else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[1] = ((uint8_t *)out)[3] = (uint8_t)tmp;
    ((int8_t *)out)[0] = ((int8_t *)out)[2] = (int8_t)(tmp >> 8);
  }
}

static void synth_full16bss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767; else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[1] = (uint8_t)tmp;
    ((int8_t *)out)[0] = (int8_t)(tmp >> 8);
  }
}

/* 24 bit, little-endian */

static void synth_full24lmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out += 3) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[2] = (int8_t)(tmp >> 16);
  }
}

#define synth_full24lsm synth_full24lmm

static void synth_full24lms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out += 6) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[0] = ((uint8_t *)out)[3] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = ((uint8_t *)out)[4] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[2] = ((int8_t *)out)[5] = (int8_t)(tmp >> 16);
  }
}

static void synth_full24lss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  if (channel) out += 3;
  for (i = 0; i < SBLIMIT; i++, out += 6) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[2] = (int8_t)(tmp >> 16);
  }
}

/* 24 bit, big-endian */

static void synth_full24bmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out += 3) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[2] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[0] = (int8_t)(tmp >> 16);
  }
}

#define synth_full24bsm synth_full24bmm

static void synth_full24bms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out += 6) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[2] = ((uint8_t *)out)[5] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = ((uint8_t *)out)[4] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[0] = ((int8_t *)out)[3] = (int8_t)(tmp >> 16);
  }
}

static void synth_full24bss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  if (channel) out += 3;
  for (i = 0; i < SBLIMIT; i++, out += 6) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[2] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[0] = (int8_t)(tmp >> 16);
  }
}

/* 32 bit , little-endian */

static void synth_full32lmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out++) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[2] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[3] = (int8_t)(tmp >> 24);
  }
}

#define synth_full32lsm synth_full32lmm

static void synth_full32lms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[0] = ((uint8_t *)out)[4] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = ((uint8_t *)out)[5] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[2] = ((uint8_t *)out)[6] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[3] = ((int8_t *)out)[7] = (int8_t)(tmp >> 24);
  }
}

static void synth_full32lss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[2] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[3] = (int8_t)(tmp >> 24);
  }
}

/* 32 bit, big-endian */

static void synth_full32bmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out++) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[3] = (uint8_t)tmp;
    ((uint8_t *)out)[2] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[0] = (int8_t)(tmp >> 24);
  }
}

#define synth_full32bsm synth_full32bmm

static void synth_full32bms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[3] = ((uint8_t *)out)[7] = (uint8_t)tmp;
    ((uint8_t *)out)[2] = ((uint8_t *)out)[6] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[1] = ((uint8_t *)out)[5] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[0] = ((int8_t *)out)[4] = (int8_t)(tmp >> 24);
  }
}

static void synth_full32bss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[3] = (uint8_t)tmp;
    ((uint8_t *)out)[2] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[0] = (int8_t)(tmp >> 24);
  }
}

/* 32 bit floating-point, little-endian */

static void synth_full32flmm(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out++) {
    union tmp__ {
      int32_t i;
      float   f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[0] = (uint8_t)tmp.i;
    ((uint8_t *)out)[1] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[2] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[3] = (int8_t)(tmp.i >> 24);
  }
}

#define synth_full32flsm synth_full32flmm

static void synth_full32flms(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    union tmp__ {
      int32_t i;
      float   f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[0] = ((uint8_t *)out)[4] = (uint8_t)tmp.i;
    ((uint8_t *)out)[1] = ((uint8_t *)out)[5] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[2] = ((uint8_t *)out)[6] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[3] = ((int8_t *)out)[7] = (int8_t)(tmp.i >> 24);
  }
}

static void synth_full32flss(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    union tmp__ {
      int32_t i;
      float   f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[0] = (uint8_t)tmp.i;
    ((uint8_t *)out)[1] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[2] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[3] = (int8_t)(tmp.i >> 24);
  }
}

/* 32 bit floating-point, big-endian */

static void synth_full32fbmm(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out++) {
    union tmp__ {
      int32_t i;
      float   f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[3] = (uint8_t)tmp.i;
    ((uint8_t *)out)[2] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[1] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[0] = (int8_t)(tmp.i >> 24);
  }
}

#define synth_full32fbsm synth_full32fbmm

static void synth_full32fbms(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    union tmp__ {
      int32_t i;
      float   f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[3] = ((uint8_t *)out)[7] = (uint8_t)tmp.i;
    ((uint8_t *)out)[2] = ((uint8_t *)out)[6] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[1] = ((uint8_t *)out)[5] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[0] = ((int8_t *)out)[4] = (int8_t)(tmp.i >> 24);
  }
}

static void synth_full32fbss(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT];

  synth_full(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT; i++, out += 2) {
    union tmp__ {
      int32_t i;
      float   f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[3] = (uint8_t)tmp.i;
    ((uint8_t *)out)[2] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[1] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[0] = (int8_t)(tmp.i >> 24);
  }
}

/* Half quality */

/* 16 bit, little-endian */

static void synth_half16lmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out++) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767; else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((int8_t *)out)[1] = (int8_t)(tmp >> 8);
  }
}

#define synth_half16lsm synth_half16lmm

static void synth_half16lms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767; else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[0] = ((uint8_t *)out)[2] = (uint8_t)tmp;
    ((int8_t *)out)[1] = ((int8_t *)out)[3] = (int8_t)(tmp >> 8);
  }
}

static void synth_half16lss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767; else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((int8_t *)out)[1] = (int8_t)(tmp >> 8);
  }
}

/* 16 bit, big-endian */

static void synth_half16bmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out++) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767; else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[1] = (uint8_t)tmp;
    ((int8_t *)out)[0] = (int8_t)(tmp >> 8);
  }
}

#define synth_half16bsm synth_half16bmm

static void synth_half16bms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767; else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[1] = ((uint8_t *)out)[3] = (uint8_t)tmp;
    ((int8_t *)out)[0] = ((int8_t *)out)[2] = (int8_t)(tmp >> 8);
  }
}

static void synth_half16bss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int16_t *out = (int16_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 32767) tmp = 32767; else if (tmp < -32768) tmp = -32768;
    ((uint8_t *)out)[1] = (uint8_t)tmp;
    ((int8_t *)out)[0] = (int8_t)(tmp >> 8);
  }
}

/* 24 bit, little-endian */

static void synth_half24lmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out += 3) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[2] = (int8_t)(tmp >> 16);
  }
}

#define synth_half24lsm synth_half24lmm

static void synth_half24lms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out += 6) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[0] = ((uint8_t *)out)[3] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = ((uint8_t *)out)[4] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[2] = ((int8_t *)out)[5] = (int8_t)(tmp >> 16);
  }
}

static void synth_half24lss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  if (channel) out += 3;
  for (i = 0; i < SBLIMIT/2; i++, out += 6) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[2] = (int8_t)(tmp >> 16);
  }
}

/* 24 bit, big-endian */

static void synth_half24bmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out += 3) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[2] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[0] = (int8_t)(tmp >> 16);
  }
}

#define synth_half24bsm synth_half24bmm

static void synth_half24bms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out += 6) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[2] = ((uint8_t *)out)[5] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = ((uint8_t *)out)[4] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[0] = ((int8_t *)out)[3] = (int8_t)(tmp >> 16);
  }
}

static void synth_half24bss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  uint8_t *out = (uint8_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  if (channel) out += 3;
  for (i = 0; i < SBLIMIT/2; i++, out += 6) {
    int32_t tmp = LROUND(buf[i]);
    if (tmp > 0x7FFFFF) tmp = 0x7FFFFF;
    else if (tmp < -0x800000) tmp = -0x800000;
    ((uint8_t *)out)[2] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((int8_t *)out)[0] = (int8_t)(tmp >> 16);
  }
}

/* 32 bit, little-endian */

static void synth_half32lmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out++) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[2] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[3] = (int8_t)(tmp >> 24);
  }
}

#define synth_half32lsm synth_half32lmm

static void synth_half32lms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[0] = ((uint8_t *)out)[4] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = ((uint8_t *)out)[5] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[2] = ((uint8_t *)out)[6] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[3] = ((int8_t *)out)[7] = (int8_t)(tmp >> 24);
  }
}

static void synth_half32lss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[0] = (uint8_t)tmp;
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[2] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[3] = (int8_t)(tmp >> 24);
  }
}

/* 32 bit, big-endian */

static void synth_half32bmm(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out++) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[3] = (uint8_t)tmp;
    ((uint8_t *)out)[2] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[0] = (int8_t)(tmp >> 24);
  }
}

#define synth_half32bsm synth_half32bmm

static void synth_half32bms(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[3] = ((uint8_t *)out)[7] = (uint8_t)tmp;
    ((uint8_t *)out)[2] = ((uint8_t *)out)[6] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[1] = ((uint8_t *)out)[5] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[0] = ((int8_t *)out)[4] = (int8_t)(tmp >> 24);
  }
}

static void synth_half32bss(mpadec_t mpadec, MYFLT *bandptr,
                            int channel, uint8_t *buffer)
{
  int i;
  int32_t *out = (int32_t *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    int64_t tmp = LLROUND(buf[i]);
    if (tmp > 0x7FFFFFFF) tmp = 0x7FFFFFFF;
    else if (tmp < (-0x7FFFFFFF - 1)) tmp = (-0x7FFFFFFF - 1);
    ((uint8_t *)out)[3] = (uint8_t)tmp;
    ((uint8_t *)out)[2] = (uint8_t)(tmp >> 8);
    ((uint8_t *)out)[1] = (uint8_t)(tmp >> 16);
    ((int8_t *)out)[0] = (int8_t)(tmp >> 24);
  }
}

/* 32 bit floating-point, little-endian */

static void synth_half32flmm(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out++) {
    union tmp__ {
      int32_t i;
      float f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[0] = (uint8_t)tmp.i;
    ((uint8_t *)out)[1] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[2] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[3] = (int8_t)(tmp.i >> 24);
  }
}

#define synth_half32flsm synth_half32flmm

static void synth_half32flms(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    union tmp__ {
      int32_t i;
      float   f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[0] = ((uint8_t *)out)[4] = (uint8_t)tmp.i;
    ((uint8_t *)out)[1] = ((uint8_t *)out)[5] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[2] = ((uint8_t *)out)[6] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[3] = ((int8_t *)out)[7] = (int8_t)(tmp.i >> 24);
  }
}

static void synth_half32flss(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    union tmp__ {
      int32_t i;
      float   f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[0] = (uint8_t)tmp.i;
    ((uint8_t *)out)[1] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[2] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[3] = (int8_t)(tmp.i >> 24);
  }
}

/* 32 bit floating-point, big-endian */

static void synth_half32fbmm(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out++) {
    union tmp__ {
      int32_t i;
      float   f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[3] = (uint8_t)tmp.i;
    ((uint8_t *)out)[2] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[1] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[0] = (int8_t)(tmp.i >> 24);
  }
}

#define synth_half32fbsm synth_half32fbmm

static void synth_half32fbms(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    union tmp__ {
      int32_t i;
      float   f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[3] = ((uint8_t *)out)[7] = (uint8_t)tmp.i;
    ((uint8_t *)out)[2] = ((uint8_t *)out)[6] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[1] = ((uint8_t *)out)[5] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[0] = ((int8_t *)out)[4] = (int8_t)(tmp.i >> 24);
  }
}

static void synth_half32fbss(mpadec_t mpadec, MYFLT *bandptr,
                             int channel, uint8_t *buffer)
{
  int i;
  float *out = (float *)buffer;
  MYFLT buf[SBLIMIT/2];

  synth_half(mpadec, bandptr, channel, buf);
  if (channel) out++;
  for (i = 0; i < SBLIMIT/2; i++, out += 2) {
    union tmp__ {
      int32 i;
      float f;
    } tmp;
    tmp.f = (float)buf[i];
    ((uint8_t *)out)[3] = (uint8_t)tmp.i;
    ((uint8_t *)out)[2] = (uint8_t)(tmp.i >> 8);
    ((uint8_t *)out)[1] = (uint8_t)(tmp.i >> 16);
    ((int8_t *)out)[0] = (int8_t)(tmp.i >> 24);
  }
}

void *synth_table[2][2][4][4] = {
  { { { synth_full16lmm,  synth_full16lms,  synth_full16lsm,  synth_full16lss  },
      { synth_full24lmm,  synth_full24lms,  synth_full24lsm,  synth_full24lss  },
      { synth_full32lmm,  synth_full32lms,  synth_full32lsm,  synth_full32lss  },
      { synth_full32flmm, synth_full32flms, synth_full32flsm, synth_full32flss } },
    { { synth_full16bmm,  synth_full16bms,  synth_full16bsm,  synth_full16bss  },
      { synth_full24bmm,  synth_full24bms,  synth_full24bsm,  synth_full24bss  },
      { synth_full32bmm,  synth_full32bms,  synth_full32bsm,  synth_full32bss  },
      { synth_full32fbmm, synth_full32fbms, synth_full32fbsm, synth_full32fbss } } },
  { { { synth_half16lmm,  synth_half16lms,  synth_half16lsm,  synth_half16lss  },
      { synth_half24lmm,  synth_half24lms,  synth_half24lsm,  synth_half24lss  },
      { synth_half32lmm,  synth_half32lms,  synth_half32lsm,  synth_half32lss  },
      { synth_half32flmm, synth_half32flms, synth_half32flsm, synth_half32flss } },
    { { synth_half16bmm,  synth_half16bms,  synth_half16bsm,  synth_half16bss  },
      { synth_half24bmm,  synth_half24bms,  synth_half24bsm,  synth_half24bss  },
      { synth_half32bmm,  synth_half32bms,  synth_half32bsm,  synth_half32bss  },
      { synth_half32fbmm, synth_half32fbms, synth_half32fbsm, synth_half32fbss } } }
};
