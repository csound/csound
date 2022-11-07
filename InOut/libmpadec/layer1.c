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

/* $Id: layer1.c,v 1.1.1.1 2004/07/27 02:57:18 metal_man Exp $ */

#include "mpadec_internal.h"

extern const uint32_t bitmask[17];
extern alloc_table_t *alloc_tables[5];

extern unsigned mpa_getbits(mpadec_t mpadec, int n);
extern uint16_t update_crc(uint16_t init, uint8_t *buf, int length);

static void I_decode_bitalloc(mpadec_t mpadec, uint8_t *bit_alloc,
                              uint8_t *scalefac)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    uint8_t *ba = bit_alloc, *scf = scalefac;
    uint32_t crclen = mpa->bytes_left;

    if (mpa->frame.channels > 1) {
      unsigned i, jsbound = mpa->frame.jsbound;
      for (i = jsbound; i; i--) {
        *ba++ = (uint8_t)GETBITS(4);
        *ba++ = (uint8_t)GETBITS(4);
      }
      for (i = (SBLIMIT - jsbound); i; i--) *ba++ = (uint8_t)GETBITS(4);
      if (mpa->config.crc && mpa->frame.CRC) {
        crclen -= mpa->bytes_left;
        mpa->crc = update_crc(mpa->crc, mpa->next_byte - crclen,
                              ((crclen << 3) - mpa->bits_left));
        if (mpa->crc != mpa->frame.crc) mpa->error = TRUE;
      }
      ba = bit_alloc;
      for (i = jsbound; i; i--) {
        if (*ba++) *scf++ = (uint8_t)GETBITS(6);
        if (*ba++) *scf++ = (uint8_t)GETBITS(6);
      }
      for (i = (SBLIMIT - jsbound); i; i--) {
        if (*ba++) {
          *scf++ = (uint8_t)GETBITS(6);
          *scf++ = (uint8_t)GETBITS(6);
        }
      }
    } else {
      register unsigned i;
      for (i = SBLIMIT; i; i--) *ba++ = (uint8_t)GETBITS(4);
      if (mpa->config.crc && mpa->frame.CRC) {
        crclen -= mpa->bytes_left;
        mpa->crc = update_crc(mpa->crc, mpa->next_byte - crclen,
                              ((crclen << 3) - mpa->bits_left));
        if (mpa->crc != mpa->frame.crc) mpa->error = TRUE;
      }
      ba = bit_alloc;
      for (i = SBLIMIT; i; i--) if (*ba++) *scf++ = (uint8_t)GETBITS(6);
    }
}

static void I_decode_samples(mpadec_t mpadec, uint8_t *bit_alloc,
                             uint8_t *scalefac, MYFLT fraction[2][SBLIMIT])
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    uint8_t *ba = bit_alloc, *scf = scalefac;
    unsigned i, n;
    int cnst = -1;

    if (mpa->frame.channels > 1) {
      unsigned jsbound = mpa->frame.jsbound;
      MYFLT *f0 = fraction[0], *f1 = fraction[1];
      for (i = jsbound; i; i--) {
        if ((n = *ba++) != 0)
          *f0++ = (((cnst)<<n)+GETBITS(n + 1) + 1)*mpa->tables.muls[n + 1][*scf++];
        else *f0++ = 0.0;
        if ((n = *ba++) != 0)
          *f1++ = (((cnst)<<n) + GETBITS(n + 1) + 1)*mpa->tables.muls[n + 1][*scf++];
        else *f1++ = 0.0;
      }
      for (i = (SBLIMIT - jsbound); i; i--) {
        if ((n = *ba++) != 0) {
          register MYFLT tmp = (((cnst) << n) + GETBITS(n + 1) + 1);
          *f0++ = tmp*mpa->tables.muls[n + 1][*scf++];
          *f1++ = tmp*mpa->tables.muls[n + 1][*scf++];
        } else *f0++ = *f1++ = 0.0;
      }
      for (i = (SBLIMIT - mpa->frame.downsample_sblimit); i; i--)
        *--f0 = *--f1 = 0.0;
    } else {
      MYFLT *f0 = fraction[0];
      for (i = SBLIMIT; i; i--) {
        if ((n = *ba++) != 0)
          *f0++ = (((cnst)<<n) + GETBITS(n + 1) + 1)*mpa->tables.muls[n + 1][*scf++];
        else *f0++ = 0.0;
      }
      for (i = (SBLIMIT - mpa->frame.downsample_sblimit); i; i--) *--f0 = 0.0;
    }
}

void decode_layer1(mpadec_t mpadec, uint8_t *buffer)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    int i, j, single;
    MYFLT fraction[2][SBLIMIT];
    uint8_t bit_alloc[2*SBLIMIT];
    uint8_t scalefac[2*SBLIMIT];

    mpa->error = FALSE;
    mpa->bits_left = 0;
    mpa->frame.jsbound =
      (uint8_t)((mpa->frame.mode == MPG_MD_JOINT_STEREO) ?
                ((mpa->frame.mode_ext + 1) << 2) : SBLIMIT);
    if (mpa->frame.channels > 1) switch (mpa->config.mode) {
      case MPADEC_CONFIG_MONO:     single = 0; break;
      case MPADEC_CONFIG_CHANNEL1: single = 1; break;
      case MPADEC_CONFIG_CHANNEL2: single = 2; break;
      default:                     single = -1; break;
      } else single = 1;
    I_decode_bitalloc(mpa, bit_alloc, scalefac);
    for (i = 0; i < SCALE_BLOCK; i++) {
      I_decode_samples(mpa, bit_alloc, scalefac, fraction);
      if (!single)
        for (j = 0; j < SBLIMIT; j++)
          fraction[0][j] = 0.5*(fraction[0][j] + fraction[1][j]);
      if (single < 0) {
        mpa->synth_func(mpa, fraction[0], 0, buffer);
        mpa->synth_func(mpa, fraction[1], 1, buffer);
      } else if (!single) {
        mpa->synth_func(mpa, fraction[0], 0, buffer);
      } else {
        mpa->synth_func(mpa, fraction[single - 1], 0, buffer);
      }
      buffer += mpa->synth_size;
    }
    {
      register unsigned n = mpa->bits_left >> 3;
      mpa->next_byte -= n;
      mpa->bytes_left += n;
      mpa->reservoir_size = 0;
    }
}
