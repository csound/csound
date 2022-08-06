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

/* $Id: layer2.c,v 1.2 2009/03/01 15:27:05 jpff Exp $ */

#include "mpadec_internal.h"

extern const uint32_t bitmask[17];
extern alloc_table_t *alloc_tables[5];

extern unsigned mpa_getbits(mpadec_t mpadec, int n);
extern uint16_t update_crc(uint16_t init, uint8_t *buf, int length);

static void II_decode_bitalloc(mpadec_t mpadec, uint8_t *bit_alloc,
                               uint8_t *scalefac)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    alloc_table_t *alloc = mpa->frame.alloc_table;
    uint8_t *scfsi, *ba = bit_alloc, *scf = scalefac;
    unsigned i, step, sblimit2, sblimit = mpa->frame.sblimit;
    uint32_t crclen = mpa->bytes_left;
    uint8_t scfsi_buf[2*SBLIMIT];

    if (mpa->frame.channels > 1) {
      unsigned jsbound = mpa->frame.jsbound;
      sblimit2 = sblimit << 1;
      for (i = jsbound; i; i--, alloc += ((int64_t)(1) << step)) {
        step = alloc->bits;
        *ba++ = (uint8_t)GETBITS(step);
        *ba++ = (uint8_t)GETBITS(step);
      }
      for (i = sblimit - jsbound; i; i--, alloc += ((int64_t)(1) << step)) {
        step = alloc->bits;
        ba[0] = (uint8_t)GETBITS(step);
        ba[1] = ba[0];
        ba += 2;
      }
      ba = bit_alloc;
      scfsi = scfsi_buf;
      for (i = sblimit2; i; i--) if (*ba++) *scfsi++ = (uint8_t)GETBITS(2);
    } else {
      sblimit2 = sblimit;
      for (i = sblimit; i; i--, alloc += ((int64_t)(1) << step)) {
        step = alloc->bits;
        *ba++ = (uint8_t)GETBITS(step);
      }
      ba = bit_alloc;
      scfsi = scfsi_buf;
      for (i = sblimit; i; i--) if (*ba++) *scfsi++ = (uint8_t)GETBITS(2);
    }
    if (mpa->config.crc && mpa->frame.CRC) {
      crclen -= mpa->bytes_left;
      mpa->crc = update_crc(mpa->crc, mpa->next_byte - crclen,
                            ((crclen << 3) - mpa->bits_left));
      if (mpa->crc != mpa->frame.crc) mpa->error = TRUE;
    }
    ba = bit_alloc;
    scfsi = scfsi_buf;
    for (i = sblimit2; i; i--) {
      if (*ba++) {
        switch (*scfsi++) {
        case 0:
          scf[0] = (uint8_t)GETBITS(6);
          scf[1] = (uint8_t)GETBITS(6);
          scf[2] = (uint8_t)GETBITS(6);
          break;
        case 1:
          scf[0] = (uint8_t)GETBITS(6);
          scf[1] = scf[0];
          scf[2] = (uint8_t)GETBITS(6);
          break;
        case 2:
          scf[0] = (uint8_t)GETBITS(6);
          scf[1] = scf[2] = scf[0];
          break;
        default:
          scf[0] = (uint8_t)GETBITS(6);
          scf[1] = (uint8_t)GETBITS(6);
          scf[2] = scf[1];
          break;
        }
        scf += 3;
      }
    }
}

static void II_decode_samples(mpadec_t mpadec, uint8_t *bit_alloc,
                              uint8_t *scalefac, MYFLT fraction[2][4][SBLIMIT],
                              int x1)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    alloc_table_t *alloc = mpa->frame.alloc_table, *alloc2;
    uint8_t *ba = bit_alloc, *scf = scalefac;
    unsigned i, j, k, step, sblimit = mpa->frame.sblimit;
    unsigned jsbound = mpa->frame.jsbound;

    for (i = 0; i < jsbound; i++, alloc += ((int64_t)1) << step) {
      step = alloc->bits;
      for (j = 0; j < (unsigned)mpa->frame.channels; j++) {
        unsigned b = *ba++; int d;
        if (b) {
          alloc2 = alloc + b;
          k = alloc2->bits;
          d = alloc2->d;
          if (d < 0) {
            register MYFLT cm = mpa->tables.muls[k][scf[x1]];
            fraction[j][0][i] = ((MYFLT)((int)GETBITS(k) + d))*cm;
            fraction[j][1][i] = ((MYFLT)((int)GETBITS(k) + d))*cm;
            fraction[j][2][i] = ((MYFLT)((int)GETBITS(k) + d))*cm;
          } else {
            unsigned idx = GETBITS(k), m = scf[x1];
            uint8_t *tab = (mpa->tables.mp2tables[d] + 3*idx);
            fraction[j][0][i] = mpa->tables.muls[*tab++][m];
            fraction[j][1][i] = mpa->tables.muls[*tab++][m];
            fraction[j][2][i] = mpa->tables.muls[*tab][m];
          }
          scf += 3;
        } else fraction[j][0][i] = fraction[j][1][i] = fraction[j][2][i] = 0.0;
      }
    }
    for (i = jsbound; i < sblimit; i++, alloc += ((int64_t)(1) << step)) {
      unsigned b = ba[1]; int d;
      step = alloc->bits;
      ba += 2;
      if (b) {
        alloc2 = alloc + b;
        k = alloc2->bits;
        d = alloc2->d;
        if (d < 0) {
          register MYFLT cm = mpa->tables.muls[k][scf[x1 + 3]];
          fraction[1][0][i] = fraction[0][0][i] = ((MYFLT)((int)GETBITS(k) + d))*cm;
          fraction[1][1][i] = fraction[0][1][i] = ((MYFLT)((int)GETBITS(k) + d))*cm;
          fraction[1][2][i] = fraction[0][2][i] = ((MYFLT)((int)GETBITS(k) + d))*cm;
          cm = mpa->tables.muls[k][scf[x1]];
          fraction[0][0][i] *= cm;
          fraction[0][1][i] *= cm;
          fraction[0][2][i] *= cm;
        } else {
          unsigned idx = GETBITS(k), m1 = scf[x1], m2 = scf[x1 + 3];
          uint8_t *tab = (mpa->tables.mp2tables[d] + 3*idx);
          fraction[0][0][i] = mpa->tables.muls[*tab][m1];
          fraction[1][0][i] = mpa->tables.muls[*tab++][m2];
          fraction[0][1][i] = mpa->tables.muls[*tab][m1];
          fraction[1][1][i] = mpa->tables.muls[*tab++][m2];
          fraction[0][2][i] = mpa->tables.muls[*tab][m1];
          fraction[1][2][i] = mpa->tables.muls[*tab][m2];
        }
        scf += 6;
      } else fraction[0][0][i] = fraction[0][1][i] = fraction[0][2][i] =
               fraction[1][0][i] = fraction[1][1][i] = fraction[1][2][i] = 0.0;
    }
    if (sblimit > (unsigned)mpa->frame.downsample_sblimit)
      sblimit = mpa->frame.downsample_sblimit;
    for (i = sblimit; i < SBLIMIT; i++)
      for (j = 0; j < (unsigned)mpa->frame.channels; j++)
        fraction[j][0][i] = fraction[j][1][i] = fraction[j][2][i] = 0.0;
}

void decode_layer2(mpadec_t mpadec, uint8_t *buffer)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    int i, j, table, single;
    MYFLT fraction[2][4][SBLIMIT];
    uint8_t bit_alloc[2*SBLIMIT];
    uint8_t scalefac[3*2*SBLIMIT];
    static uint8_t sblimits[5] = { 27 , 30 , 8, 12 , 30 };
    static uint8_t translate[3][2][16] =
      { { { 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, 0 },
          { 0, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 } },
        { { 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
          { 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { { 0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, 0 },
          { 0, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 } } };

    mpa->error = FALSE;
    mpa->bits_left = 0;
    if (mpa->frame.LSF) table = 4;
    else
      table =
        translate[mpa->frame.frequency_index]
                 [2 - mpa->frame.channels][mpa->frame.bitrate_index];
    mpa->frame.alloc_table = alloc_tables[table];
    mpa->frame.sblimit = sblimits[table];
    mpa->frame.jsbound =
      (uint8_t)((mpa->frame.mode == MPG_MD_JOINT_STEREO) ?
                ((mpa->frame.mode_ext + 1) << 2) : mpa->frame.sblimit);
    if (mpa->frame.channels > 1)
      switch (mpa->config.mode) {
      case MPADEC_CONFIG_MONO:     single = 0; break;
      case MPADEC_CONFIG_CHANNEL1: single = 1; break;
      case MPADEC_CONFIG_CHANNEL2: single = 2; break;
      default:                     single = -1; break;
      } else single = 1;
    II_decode_bitalloc(mpa, bit_alloc, scalefac);
    for (i = 0; i < SCALE_BLOCK; i++) {
      II_decode_samples(mpa, bit_alloc, scalefac, fraction, i >> 2);
      if (!single) for (j = 0; j < 3; j++) {
          register int k;
          for (k = 0; k < SBLIMIT; k++)
            fraction[0][j][k] = 0.5*(fraction[0][j][k] + fraction[1][j][k]);
        }
      if (single < 0) {
        for (j = 0; j < 3; j++, buffer += mpa->synth_size) {
          mpa->synth_func(mpa, fraction[0][j], 0, buffer);
          mpa->synth_func(mpa, fraction[1][j], 1, buffer);
        }
      } else if (!single) {
        for (j = 0; j < 3; j++, buffer += mpa->synth_size) {
          mpa->synth_func(mpa, fraction[0][j], 0, buffer);
        }
      } else {
        for (j = 0; j < 3; j++, buffer += mpa->synth_size) {
          mpa->synth_func(mpa, fraction[single - 1][j], 0, buffer);
        }
      }
    }
    {
      register unsigned n = mpa->bits_left >> 3;
      mpa->next_byte -= n;
      mpa->bytes_left += n;
      mpa->reservoir_size = 0;
    }
}
