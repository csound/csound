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

/* $Id: mpadec.c,v 1.3 2009/03/01 15:27:05 jpff Exp $ */

#include <stdlib.h>
#include "csoundCore.h"
#include "mpadec_internal.h"

extern const uint16_t crc_table[256];
extern void *synth_table[2][2][4][4];

const uint32_t bitmask[17] = {
  0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F,
  0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

const int32_t frequency_table[9] = {
  44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000 };

const int16_t mpa_bitrate_table[2][3][16] = {
  { { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 560 },
    { 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 448 },
    { 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384 } },
  { { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 320 },
    { 0,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160, 192 },
    { 0,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160, 192 } }
};

extern void init_tables(mpadec_t mpadec, MYFLT scale, int32_t sblimit);
extern void decode_layer1(mpadec_t mpadec, uint8_t *buffer);
extern void decode_layer2(mpadec_t mpadec, uint8_t *buffer);
extern void decode_layer3(mpadec_t mpadec, uint8_t *buffer);

uint32_t mpa_getbits(mpadec_t mpadec, unsigned n)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;

    while ((mpa->bits_left <= 24) && mpa->bytes_left) {
      mpa->bit_buffer = (mpa->bit_buffer << 8) | *mpa->next_byte++;
      mpa->bits_left += (uint8_t)8;
      mpa->bytes_left--;
    }
    while (mpa->bits_left <= 24) {
      mpa->bit_buffer = (mpa->bit_buffer << 8);
      mpa->bits_left += (uint8_t)8;
    }
    mpa->bits_left -= (uint8_t)n;
    return (mpa->bit_buffer >> mpa->bits_left) & bitmask[n];
}

uint16_t update_crc(uint16_t init, uint8_t *buf, int length)
{
    register uint32_t crc = (uint32_t)init, tmp;
    register int l = length;
    register uint8_t *b = buf;

    for (; l >= 8; l -= 8)
      crc = (crc << 8) ^ crc_table[((crc >> 8) ^ (*b++)) & 0xFF];
    tmp = (uint32_t)(*b) << 8;
    while (l--) {
      tmp <<= 1;
      crc <<= 1;
      if ((crc ^ tmp) & 0x10000) crc ^= 0x8005;
    }
    return (uint16_t)crc;
}

static uint32_t detect_frame_size(mpadec_t mpadec)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    register uint8_t *buf = mpa->next_byte;
    uint32_t i = mpa->bytes_left, hdr = 0;

    if (i < (mpa->frame.frame_size + 4)) return 0;
    buf += mpa->frame.frame_size;
    i -= mpa->frame.frame_size;
    while (i >= 4) {
      register uint32_t tmp = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
      if (((tmp & 0xFFE00000) == 0xFFE00000) &&
          (tmp & (3 << 17)) &&
          ((tmp & (3 << 10)) != (3 << 10))) {
        if ((mpa->frame.layer == (uint8_t)(4 - ((tmp >> 17) & 3))) &&
            (mpa->frame.frequency_index ==
             (((tmp >> 10) & 3) + 3*(mpa->frame.LSF + mpa->frame.MPEG25))) &&
            (mpa->frame.channels == ((((tmp >> 6) & 3) == MPG_MD_MONO) ? 1 : 2))) {
          if (mpa->config.dblsync) {
            uint32_t fs = mpa->bytes_left - i - mpa->frame.padding + ((tmp>>9) & 1);
            if (i >= (fs + 4)) {
              buf += fs;
              tmp = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
              buf -= fs;
              if (((tmp & 0xFFE00000) == 0xFFE00000) &&
                  (tmp & (3 << 17))                  &&
                  ((tmp & (3 << 10)) != (3 << 10))) {
                if ((mpa->frame.layer == (uint8_t)(4 - ((tmp >> 17) & 3))) &&
                    (mpa->frame.frequency_index ==
                     (((tmp>>10)&3) + 3*(mpa->frame.LSF + mpa->frame.MPEG25))) &&
                    (mpa->frame.channels == ((((tmp>>6)&3) == MPG_MD_MONO)?1:2))) {
                  hdr = tmp;
                  break;
                }
              }
            } else return 0;
          } else {
            hdr = tmp;
            break;
          }
        }
      }
      buf++; i--;
    }
    return (hdr ? (mpa->bytes_left - i) : 0);
}

static int decode_header(mpadec_t mpadec, uint32_t header)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    unsigned int layer, bridx, fridx;

    layer = 4 - ((header >> 17) & 3);
    bridx = ((header >> 12) & 0x0F);
    fridx = ((header >> 10) & 3);
    if ((fridx == 3) || (layer == 4) || ((layer != 3) && !bridx)) return FALSE;
    if (header & (1 << 20)) {
      mpa->frame.LSF = (uint8_t)((header & (1 << 19)) ? FALSE : TRUE);
      mpa->frame.MPEG25 = FALSE;
    } else mpa->frame.LSF = mpa->frame.MPEG25 = TRUE;
    mpa->frame.layer = (uint8_t)layer;
    mpa->frame.bitrate_index = bridx;
    mpa->frame.bitrate = mpa_bitrate_table[mpa->frame.LSF][layer - 1][bridx];
    mpa->frame.frequency_index = (fridx += 3*(mpa->frame.LSF + mpa->frame.MPEG25));
    mpa->frame.frequency = frequency_table[fridx];
    mpa->frame.decoded_frequency = mpa->frame.frequency >> mpa->config.quality;
    mpa->frame.CRC = (uint8_t)(((header >> 16) & 1) ^ 1);
    mpa->frame.padding = ((header >> 9) & 1);
    mpa->frame.extension = (uint8_t)((header >> 8) & 1);
    mpa->frame.mode = (uint8_t)((header >> 6) & 3);
    mpa->frame.mode_ext = (uint8_t)((header >> 4) & 3);
    mpa->frame.copyright = (uint8_t)((header >> 3) & 1);
    mpa->frame.original = (uint8_t)((header >> 2) & 1);
    mpa->frame.emphasis = (uint8_t)(header & 3);
    mpa->frame.channels = (uint8_t)((mpa->frame.mode == MPG_MD_MONO) ? 1 : 2);
    switch (mpa->config.mode) {
    case MPADEC_CONFIG_CHANNEL1:
    case MPADEC_CONFIG_CHANNEL2:
    case MPADEC_CONFIG_MONO:     mpa->frame.decoded_channels = 1; break;
    case MPADEC_CONFIG_STEREO:   mpa->frame.decoded_channels = 2; break;
    default:                     mpa->frame.decoded_channels = mpa->frame.channels;
  }
    mpa->free_format = FALSE;
    switch (mpa->frame.layer) {
    case 1: mpa->frame.frame_samples = 384;
      mpa->frame.frame_size =
        (12000*mpa->frame.bitrate/mpa->frame.frequency + mpa->frame.padding) << 2;
      mpa->ssize = 0;
      break;
    case 2: mpa->frame.frame_samples = 1152;
      mpa->frame.frame_size =
        144000*mpa->frame.bitrate/mpa->frame.frequency + mpa->frame.padding;
      mpa->ssize = 0;
      break;
    case 3: mpa->frame.frame_samples = 1152 >> mpa->frame.LSF;
      if (mpa->frame.LSF) mpa->ssize = (mpa->frame.channels > 1) ? 17 : 9;
      else mpa->ssize = (mpa->frame.channels > 1) ? 32 : 17;
      if (mpa->frame.bitrate_index) {
        mpa->frame.frame_size =
          144000*mpa->frame.bitrate/(mpa->frame.frequency << mpa->frame.LSF) +
          mpa->frame.padding;
        mpa->prev_frame_size = 0;
      } else {
        register uint32_t div = 125*mpa->frame.frame_samples;
        if (!mpa->prev_frame_size) {
          mpa->frame.frame_size =
            (mpa->frame.CRC ? 6 : 4) + mpa->ssize + mpa->frame.padding;
          mpa->prev_frame_size = detect_frame_size(mpa);
          if (mpa->prev_frame_size) mpa->prev_frame_size -= mpa->frame.padding;
        }
        mpa->frame.frame_size = mpa->prev_frame_size + mpa->frame.padding;
        mpa->frame.bitrate =
          (mpa->frame.frame_size*mpa->frame.frequency + (div >> 1))/div;
        mpa->free_format = TRUE;
      }
      break;
    }
    mpa->frame.decoded_samples = mpa->frame.frame_samples >> mpa->config.quality;
    mpa->frame.downsample = FALSE;
    mpa->frame.downsample_sblimit = SBLIMIT;
    mpa->frame.decoded_size =
      mpa->frame.decoded_samples*mpa->frame.decoded_channels;
    switch (mpa->config.format) {
    case MPADEC_CONFIG_24BIT: mpa->frame.decoded_size *= 3; break;
    case MPADEC_CONFIG_32BIT:
    case MPADEC_CONFIG_FLOAT: mpa->frame.decoded_size <<= 2; break;
    default:                  mpa->frame.decoded_size <<= 1; break;
    }
    mpa->hsize = mpa->frame.CRC ? 6 : 4;
    if (mpa->frame.frame_size < (mpa->hsize + mpa->ssize))
      mpa->frame.frame_size = mpa->hsize + mpa->ssize;
    mpa->dsize = mpa->frame.frame_size - (mpa->hsize + mpa->ssize);

    return TRUE;
}

static uint32_t sync_buffer(mpadec_t mpadec)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    register uint8_t *buf = mpa->next_byte;
    uint32_t retval = 0, i = mpa->bytes_left;
    if (mpa->state == MPADEC_STATE_START) {
      buf += 128; i -= 128;
      while (i >= 4) {
        register uint32_t tmp = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
        if (((tmp & 0xFFE00000) == 0xFFE00000) &&
            (tmp & (3<<17))                    &&
            ((tmp & (3<<10)) != (3<<10))) {
          if (mpa->config.dblsync) {
            if (decode_header(mpa, tmp)) {
              if ((i < (mpa->frame.frame_size + 4)) ||
                  (mpa->free_format && !mpa->prev_frame_size)) {
                break;
              }
              else {
                register uint32_t tmp2 =
                  (buf[mpa->frame.frame_size]<<24)     |
                  (buf[mpa->frame.frame_size + 1]<<16) |
                  (buf[mpa->frame.frame_size + 2]<<8)  |
                  buf[mpa->frame.frame_size + 3];
                if (((tmp2 & 0xFFE00000) == 0xFFE00000) &&
                    (tmp2 & (3<<17))                    &&
                    ((tmp2 & (3<<10)) != (3<<10))) {
                  if ((mpa->frame.layer == (uint8_t)(4 - ((tmp2 >> 17) & 3))) &&
                      (mpa->frame.frequency_index ==
                       (((tmp2>>10)&3) + 3*(mpa->frame.LSF + mpa->frame.MPEG25))) &&
                      (mpa->frame.channels == ((((tmp2>>6)&3)==MPG_MD_MONO)?1:2))) {
                    retval = tmp;
                    break;
                  }
                }
              }
            }
          } else {
            retval = tmp;
            break;
          }
        }
        buf++; i--;

      }
    } else {
      while (i >= 4) {
        register uint32_t tmp = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
        if (((tmp & 0xFFE00000) == 0xFFE00000) &&
            (tmp & (3<<17))                    &&
            ((tmp & (3<<10)) != (3<<10))) {
          if ((mpa->frame.layer == (uint8_t)(4 - ((tmp >> 17) & 3))) &&
              (mpa->frame.frequency_index ==
               (((tmp >> 10) & 3) + 3*(mpa->frame.LSF + mpa->frame.MPEG25))) &&
              (mpa->frame.channels == ((((tmp>>6)&3) == MPG_MD_MONO) ? 1 : 2))) {
            retval = tmp;
            break;
          }
        }
        buf++; i--;
      }
    }
    if (i < mpa->bytes_left) {
      i = mpa->bytes_left - i;
      mpa->next_byte = buf;;
      mpa->bytes_left -= i;
      if (i >= 512) {
        mpa->reservoir_size = 0;
        i = 512;
      }
      memcpy(mpa->reservoir + mpa->reservoir_size, mpa->next_byte - i, i);
      mpa->reservoir_size += i;
    }
    return retval;
}

static int first_frame(mpadec_t mpadec)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    int i, skip = FALSE;
    uint32_t framesize; MYFLT scale;

    if (mpa->frame.channels > 1)
      i = ((mpa->config.mode == MPADEC_CONFIG_STEREO) ||
           (mpa->config.mode == MPADEC_CONFIG_AUTO)) ? 3 : 2;
    else i = (mpa->config.mode == MPADEC_CONFIG_STEREO) ? 1 : 0;
    mpa->synth_func =
      synth_table[mpa->config.quality][mpa->config.endian][mpa->config.format][i];
    mpa->sample_size = mpa->frame.decoded_channels;
    switch (mpa->config.format) {
    case MPADEC_CONFIG_24BIT: mpa->sample_size *= 3; scale = 0x800000; break;
    case MPADEC_CONFIG_32BIT: mpa->sample_size <<= 2; scale = 0x80000000; break;
    case MPADEC_CONFIG_FLOAT: mpa->sample_size <<= 2; scale = 1; break;
    default:                  mpa->sample_size <<= 1; scale = 0x8000; break;
    }
    mpa->synth_size = (mpa->sample_size<<5) >> mpa->config.quality;
    if (mpa->config.replaygain != MPADEC_CONFIG_REPLAYGAIN_CUSTOM) {
      mpa->config.gain = 0.0;
      mpa->replay_gain = 1.0;
    }
    mpa->skip_samples = 0;
    mpa->padding_samples = 0;
    mpa->decoded_frames = 0;
    mpa->decoded_samples = 0;
    memset(&mpa->tag_info, 0, sizeof(mpa->tag_info));
    framesize = (mpa->frame.frame_size < mpa->bytes_left) ?
      mpa->frame.frame_size : mpa->bytes_left;
    if ((mpa->frame.layer == 3) && (framesize >= (mpa->ssize + 124))) {
      register uint8_t *buf = mpa->next_byte + 4 + mpa->ssize;
      if (((buf[0] == 'X') && (buf[1] == 'i') &&
           (buf[2] == 'n') && (buf[3] == 'g')) ||
          ((buf[0] == 'I') && (buf[1] == 'n') &&
           (buf[2] == 'f') && (buf[3] == 'o'))) {
        skip = TRUE;
        mpa->next_byte += framesize;
        mpa->bytes_left -= framesize;
        buf += 4;
        mpa->tag_info.flags = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
        buf += 4;
        if (mpa->tag_info.flags & 1) {
          mpa->tag_info.frames = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
          buf += 4;
        };
        if (mpa->tag_info.flags & 2) {
          mpa->tag_info.bytes = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
          buf += 4;
        };
        if (mpa->tag_info.flags & 4) {
          memcpy(mpa->tag_info.toc, buf, 100);
          buf += 100;
        };
        if (mpa->tag_info.flags & 8) buf += 4;
        mpa->tag_info.flags &= 7;
        if (framesize >= mpa->ssize + 160) {
          buf += 15;
          mpa->tag_info.replay_gain[0] = ((buf[0]<<8) | buf[1]) & 0x1FF;
          if (buf[0] & 2)
            mpa->tag_info.replay_gain[0] = -mpa->tag_info.replay_gain[0];
          buf += 2;
          mpa->tag_info.replay_gain[1] = ((buf[0]<<8) | buf[1]) & 0x1FF;
          if (buf[0] & 2)
            mpa->tag_info.replay_gain[1] = -mpa->tag_info.replay_gain[1];
          buf += 4;
          mpa->tag_info.enc_delay = (buf[0]<<4) | ((buf[1] >> 4) & 0x0F);
          mpa->tag_info.enc_padding = ((buf[1] & 0x0F)<<8) | buf[2];
          if (((mpa->tag_info.enc_delay < 0)     ||
               (mpa->tag_info.enc_delay > 3000)) ||
              ((mpa->tag_info.enc_padding < 0)   ||
               (mpa->tag_info.enc_padding > 3000))) {
            mpa->tag_info.replay_gain[0] = mpa->tag_info.replay_gain[1] = 0;
            mpa->tag_info.enc_delay = 0;
            mpa->tag_info.enc_padding = 0;
          } else {
            if (mpa->config.replaygain == MPADEC_CONFIG_REPLAYGAIN_RADIO) {
              mpa->config.gain = ((MYFLT)mpa->tag_info.replay_gain[0])/10.0;
            }
            else
              if (mpa->config.replaygain == MPADEC_CONFIG_REPLAYGAIN_AUDIOPHILE) {
                mpa->config.gain = ((MYFLT)mpa->tag_info.replay_gain[1])/10.0;
              }
          }
          mpa->skip_samples = mpa->tag_info.enc_delay;
          mpa->padding_samples = mpa->tag_info.enc_padding;
        }
      }
    }
    mpa->replay_gain = pow(10.0, mpa->config.gain/20.0);
    if (mpa->config.replaygain != MPADEC_CONFIG_REPLAYGAIN_NONE)
      scale *= mpa->replay_gain;
    init_tables(mpa, scale, SBLIMIT >> mpa->config.quality);
    if (mpa->frame.layer == 3) {
      mpa->skip_samples += 529;
      if ((mpa->tag_info.flags & 1) && (mpa->padding_samples > 529))
        mpa->padding_samples -= 529;
      else mpa->padding_samples = 0;
    } else {
      mpa->skip_samples += 241;
      mpa->padding_samples = 0;
    }
    mpa->padding_start = mpa->tag_info.frames*mpa->frame.frame_samples;
    if (mpa->padding_start > mpa->padding_samples)
      mpa->padding_start -= mpa->padding_samples;
    else mpa->padding_start = 0;
    mpa->state = MPADEC_STATE_DECODE;
    return skip;
}

mpadec_t mpadec_init(void)
{
    register struct mpadec_t *mpa =
      (struct mpadec_t *)malloc(sizeof(struct mpadec_t));
    union {
      uint8_t  t8[2];
      uint16_t t16;
    } ch;
    if (!mpa) return NULL;
    memset(mpa, 0, sizeof(struct mpadec_t));
    mpa->size = sizeof(struct mpadec_t);
    ch.t16 = 1;
    // *((int16_t *)temp) = 1;
    mpa->config.quality = MPADEC_CONFIG_FULL_QUALITY;
    mpa->config.mode = MPADEC_CONFIG_AUTO;
    mpa->config.format = MPADEC_CONFIG_16BIT;
    mpa->config.endian =
      (uint8_t)(ch.t8[0] ? MPADEC_CONFIG_LITTLE_ENDIAN :
                           MPADEC_CONFIG_BIG_ENDIAN);
    mpa->config.replaygain = MPADEC_CONFIG_REPLAYGAIN_NONE;
    mpa->config.skip = TRUE;
    mpa->config.crc = TRUE;
    mpa->config.dblsync = TRUE;
    mpa->config.gain = 0.0;
    mpa->replay_gain = 1.0;
    init_tables(mpa, 0x8000, SBLIMIT);
    mpa->synth_bufoffs = 1;
    mpa->state = MPADEC_STATE_START;
    return mpa;
}

int mpadec_uninit(mpadec_t mpadec)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;

    if (mpa && (mpa->size == sizeof(struct mpadec_t))) {
      mpa->size = mpa->state = 0;
      free(mpa);
      return MPADEC_RETCODE_OK;
    } else return MPADEC_RETCODE_INVALID_HANDLE;
}

int mpadec_reset(mpadec_t mpadec)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;

    if (mpa && (mpa->size == sizeof(struct mpadec_t))) {
      memset(mpa->hybrid_in, 0, sizeof(mpa->hybrid_in));
      memset(mpa->hybrid_out, 0, sizeof(mpa->hybrid_out));
      memset(mpa->hybrid_buffers, 0, sizeof(mpa->hybrid_buffers));
      memset(mpa->synth_buffers, 0, sizeof(mpa->synth_buffers));
      mpa->hybrid_block[0] = mpa->hybrid_block[1] = 0;
      mpa->synth_bufoffs = 1;
      memset(&mpa->tag_info, 0, sizeof(mpa->tag_info));
      if (mpa->config.replaygain != MPADEC_CONFIG_REPLAYGAIN_CUSTOM)
        mpa->config.gain = 0.0;
      mpa->prev_frame_size = 0;
      mpa->free_format = FALSE;
      mpa->error = FALSE;
      mpa->reservoir_size = 0;
      mpa->replay_gain = 1.0;
      mpa->skip_samples = 0;
      mpa->padding_samples = 0;
      mpa->decoded_frames = 0;
      mpa->decoded_samples = 0;
      mpa->state = MPADEC_STATE_START;
      return MPADEC_RETCODE_OK;
    } else return MPADEC_RETCODE_INVALID_HANDLE;
}

int mpadec_configure(mpadec_t mpadec, mpadec_config_t *cfg)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    int32_t i, sblimit; MYFLT scale;

    if (mpa && (mpa->size == sizeof(struct mpadec_t))) {
      if (!cfg) return MPADEC_RETCODE_INVALID_PARAMETERS;
      if ((cfg->quality > MPADEC_CONFIG_HALF_QUALITY) ||
          (cfg->mode > MPADEC_CONFIG_CHANNEL2) ||
          (cfg->format > MPADEC_CONFIG_FLOAT) ||
          (cfg->endian > MPADEC_CONFIG_BIG_ENDIAN) ||
          (cfg->replaygain > MPADEC_CONFIG_REPLAYGAIN_CUSTOM))
        return MPADEC_RETCODE_INVALID_PARAMETERS;
      mpa->config.quality = cfg->quality;
      mpa->config.mode = cfg->mode;
      mpa->config.format = cfg->format;
      mpa->config.endian = cfg->endian;
      mpa->config.replaygain = cfg->replaygain;
      mpa->config.skip = (uint8_t)(cfg->skip ? TRUE : FALSE);
      mpa->config.crc = (uint8_t)(cfg->crc ? TRUE : FALSE);
      mpa->config.dblsync = (uint8_t)(cfg->dblsync ? TRUE : FALSE);
      if (mpa->config.replaygain == MPADEC_CONFIG_REPLAYGAIN_CUSTOM) {
        mpa->config.gain = cfg->gain;
      } else {
        mpa->config.gain = 0.0;
        if (mpa->tag_info.flags) {
          if (mpa->config.replaygain == MPADEC_CONFIG_REPLAYGAIN_RADIO) {
            mpa->config.gain = ((MYFLT)mpa->tag_info.replay_gain[0])/10.0;
          }
          else
            if (mpa->config.replaygain == MPADEC_CONFIG_REPLAYGAIN_AUDIOPHILE) {
              mpa->config.gain = ((MYFLT)mpa->tag_info.replay_gain[1])/10.0;
            }
        }
      }
      mpa->replay_gain = pow(10.0, mpa->config.gain/20.0);
      switch (mpa->config.format) {
      case MPADEC_CONFIG_24BIT: scale = 0x800000; break;
      case MPADEC_CONFIG_32BIT: scale = 0x80000000; break;
      case MPADEC_CONFIG_FLOAT: scale = 1; break;
      default:                  scale = 0x8000; break;
      }
      sblimit = SBLIMIT >> mpa->config.quality;
      if (mpa->config.replaygain != MPADEC_CONFIG_REPLAYGAIN_NONE)
        scale *= mpa->replay_gain;
      init_tables(mpa, scale, sblimit);
      if ((mpa->state > MPADEC_STATE_START) && mpa->header) {
        decode_header(mpa, mpa->header);
        if (mpa->frame.channels < 2)
          i = (mpa->config.mode == MPADEC_CONFIG_STEREO) ? 1 : 0;
        else i = ((mpa->config.mode == MPADEC_CONFIG_STEREO) ||
                  (mpa->config.mode == MPADEC_CONFIG_AUTO)) ? 3 : 2;
        mpa->synth_func =
          synth_table[mpa->config.quality][mpa->config.endian]
          [mpa->config.format][i];
        mpa->sample_size = mpa->frame.decoded_channels;
        switch (mpa->config.format) {
        case MPADEC_CONFIG_24BIT: mpa->sample_size *= 3; break;
        case MPADEC_CONFIG_32BIT:
        case MPADEC_CONFIG_FLOAT: mpa->sample_size <<= 2; break;
        default:                  mpa->sample_size <<= 1; break;
        }
        mpa->synth_size = (mpa->sample_size<<5) >> mpa->config.quality;
      } else mpa->state = MPADEC_STATE_START;
      return MPADEC_RETCODE_OK;
    } else return MPADEC_RETCODE_INVALID_HANDLE;
}

int mpadec_get_info(mpadec_t mpadec, void *info, int info_type)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;

    if (!mpa || (mpa->size != sizeof(struct mpadec_t)))
      return MPADEC_RETCODE_INVALID_HANDLE;
    if (!info) return MPADEC_RETCODE_INVALID_PARAMETERS;
    if (info_type == MPADEC_INFO_CONFIG) {
      register mpadec_config_t *cfg = (mpadec_config_t *)info;
      cfg->quality = mpa->config.quality;
      cfg->mode = mpa->config.mode;
      cfg->format = mpa->config.format;
      cfg->endian = mpa->config.endian;
      cfg->replaygain = mpa->config.replaygain;
      cfg->skip = mpa->config.skip;
      cfg->crc = mpa->config.crc;
      cfg->dblsync = mpa->config.dblsync;
      cfg->gain = mpa->config.gain;
    } else if (info_type == MPADEC_INFO_TAG) {
      register mp3tag_info_t *tag = (mp3tag_info_t *)info;
      if (mpa->state < MPADEC_STATE_DECODE) {
        memset(tag, 0, sizeof(mp3tag_info_t));
        return MPADEC_RETCODE_BAD_STATE;
      } else memcpy(tag, &mpa->tag_info, sizeof(mpa->tag_info));
    } else if (info_type == MPADEC_INFO_STREAM) {
      register mpadec_info_t *inf = (mpadec_info_t *)info;
      if (mpa->state < MPADEC_STATE_DECODE) {
        memset(inf, 0, sizeof(mpadec_info_t));
        return MPADEC_RETCODE_BAD_STATE;
      } else {
        inf->layer = mpa->frame.layer;
        inf->channels = mpa->frame.channels;
        inf->frequency = mpa->frame.frequency;
        inf->bitrate = mpa->frame.bitrate;
        inf->mode = mpa->frame.mode;
        inf->copyright = mpa->frame.copyright;
        inf->original = mpa->frame.original;
        inf->emphasis = mpa->frame.emphasis;
        inf->decoded_channels = mpa->frame.decoded_channels;
        inf->decoded_frequency = mpa->frame.decoded_frequency;
        inf->decoded_sample_size = mpa->sample_size;
        inf->frame_size = mpa->frame.frame_size;
        inf->frame_samples = mpa->frame.frame_samples;
        inf->decoded_frame_samples = mpa->frame.decoded_samples;
        if (mpa->tag_info.flags & 1) {
          inf->frames = mpa->tag_info.frames;
          inf->duration = (mpa->tag_info.frames*mpa->frame.frame_samples +
                           (mpa->frame.frequency >> 1))/mpa->frame.frequency;
        } else {
          inf->frames = 0;
          inf->duration = 0;
        }
      }
    } else return MPADEC_RETCODE_INVALID_PARAMETERS;
    return MPADEC_RETCODE_OK;
}

int mpadec_decode(mpadec_t mpadec, uint8_t *srcbuf, uint32_t srcsize,
                  uint8_t *dstbuf, uint32_t dstsize, uint32_t *srcused,
                  uint32_t *dstused)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    int retcode = MPADEC_RETCODE_OK;
    uint32_t decoded_size = 0;

    if (srcused) *srcused = 0;
    if (dstused) *dstused = 0;
    if (!dstbuf) dstsize = 0;
    if (!mpa || (mpa->size != sizeof(struct mpadec_t)))
      return MPADEC_RETCODE_INVALID_HANDLE;
    if (mpa->state < MPADEC_STATE_START) return MPADEC_RETCODE_BAD_STATE;
    if (!srcbuf || ((mpa->state > MPADEC_STATE_START) && !dstbuf ))
      return MPADEC_RETCODE_INVALID_PARAMETERS;
    mpa->next_byte = srcbuf;
    mpa->bytes_left = srcsize;
    while (mpa->bytes_left >= 4) {
      mpa->error = FALSE;
      mpa->bits_left = 0;
      mpa->header = sync_buffer(mpa);
      if (!mpa->header) {
        if (!decoded_size) retcode = MPADEC_RETCODE_NO_SYNC;
        break;
      }
      decode_header(mpa, mpa->header);
      if ((mpa->bytes_left < mpa->frame.frame_size) ||
          (mpa->free_format && !mpa->prev_frame_size)) {
        retcode =  MPADEC_RETCODE_NEED_MORE_DATA;
        break;
      }
      if (mpa->state == MPADEC_STATE_START) {
        if (first_frame(mpa)) continue;
      } else if ((mpa->frame.layer == 3) &&
                 (mpa->frame.frame_size >= (mpa->ssize + 12))) {
        register uint8_t *buf = mpa->next_byte + 4 + mpa->ssize;
        if (((buf[0] == 'X') && (buf[1] == 'i') &&
             (buf[2] == 'n') && (buf[3] == 'g')) ||
            ((buf[0] == 'I') && (buf[1] == 'n') &&
             (buf[2] == 'f') && (buf[3] == 'o'))) {
          mpa->next_byte += mpa->frame.frame_size;
          mpa->bytes_left -= mpa->frame.frame_size;
          continue;
        }
      }
      if (dstsize < mpa->frame.decoded_size) {
        retcode = MPADEC_RETCODE_BUFFER_TOO_SMALL;
        break;
      }
      if (mpa->config.crc && mpa->frame.CRC) {
        mpa->frame.crc = (uint16_t)((mpa->next_byte[4]<<8) | mpa->next_byte[5]);
      mpa->crc = update_crc(0xFFFF, mpa->next_byte + 2, 16);
    }
    mpa->next_byte += mpa->hsize;
    mpa->bytes_left -= mpa->hsize;
    /* ??Check dstbuf non NULL?? */
    if (dstbuf == NULL) return MPADEC_RETCODE_UNKNOWN;
    switch (mpa->frame.layer) {
      case 1: decode_layer1(mpa, dstbuf); break;
      case 2: decode_layer2(mpa, dstbuf); break;
      case 3: decode_layer3(mpa, dstbuf); break;
    }
    if (mpa->config.crc && mpa->error) memset(dstbuf, 0, mpa->frame.decoded_size);
    dstbuf += mpa->frame.decoded_size;
    dstsize -= mpa->frame.decoded_size;
    decoded_size += mpa->frame.decoded_size;
    mpa->decoded_samples += mpa->frame.frame_samples;
    mpa->decoded_frames++;
    if (mpa->config.skip) {
      if (mpa->skip_samples) {
        if (mpa->skip_samples >= mpa->frame.frame_samples) {
          dstbuf -= mpa->frame.decoded_size;
          dstsize += mpa->frame.decoded_size;
          decoded_size -= mpa->frame.decoded_size;
          mpa->skip_samples -= mpa->frame.frame_samples;
        } else {
          uint32_t tmp = mpa->frame.frame_samples - mpa->skip_samples;
          tmp = mpa->sample_size*(tmp >> mpa->config.quality);
          memmove(dstbuf - mpa->frame.decoded_size, dstbuf - tmp, tmp);
          dstbuf -= mpa->frame.decoded_size - tmp;
          dstsize += mpa->frame.decoded_size - tmp;
          decoded_size -= mpa->frame.decoded_size - tmp;
          mpa->skip_samples = 0;
        }
      } else if ((mpa->padding_samples) &&
                 (mpa->decoded_samples > mpa->padding_start)) {
        uint32_t tmp = mpa->decoded_samples - mpa->padding_start;
        if (tmp > mpa->padding_samples) tmp = mpa->padding_samples;
        mpa->padding_start += tmp;
        mpa->padding_samples -= tmp;
        tmp = mpa->sample_size*(tmp >> mpa->config.quality);
        dstbuf -= tmp;
        dstsize += tmp;
        decoded_size -= tmp;
      }
    }
  }
  if (srcused) *srcused = srcsize - mpa->bytes_left;
  if (dstused) *dstused = decoded_size;
  if ((retcode == MPADEC_RETCODE_OK) && mpa->bytes_left)
    retcode = MPADEC_RETCODE_NEED_MORE_DATA;
  if (!dstbuf && (retcode == MPADEC_RETCODE_BUFFER_TOO_SMALL))
    retcode = MPADEC_RETCODE_OK;
  return retcode;
}

char *mpadec_error(int code)
{
  static char *mpa_errors[] = { "No error",
                                "Invalid handle",
                                "Not enough memory",
                                "Bad decoder state",
                                "Invalid parameters",
                                "Need more data",
                                "Output buffer too small",
                                "Unable to synchronize",
                                "Unknown error" };
  if (code > MPADEC_RETCODE_UNKNOWN) code = MPADEC_RETCODE_UNKNOWN;
  return mpa_errors[code];
}

mpadec2_t mpadec2_init(void)
{
  register struct mpadec2_t *mpa =
    (struct mpadec2_t *)malloc(sizeof(struct mpadec2_t));

  if (!mpa) return NULL;
  mpa->size = sizeof(struct mpadec2_t);
  mpa->buffers = NULL;
  mpa->in_buffer_offset = mpa->in_buffer_used = 0;
  mpa->out_buffer_offset = mpa->out_buffer_used = 0;
  mpa->mpadec = mpadec_init();
  if (!mpa->mpadec) {
    free(mpa);
    return NULL;
  }
  return mpa;
}

int mpadec2_uninit(mpadec2_t mpadec)
{
  register struct mpadec2_t *mpa = (struct mpadec2_t *)mpadec;

  if (mpa && (mpa->size == sizeof(struct mpadec2_t))) {
    struct mpabuffer_t *buf = mpa->buffers, *tmp;
    while (buf) {
      tmp = buf;
      buf = buf->next;
      free(tmp);
    }
    mpadec_uninit(mpa->mpadec);
    free(mpa);
    return MPADEC_RETCODE_OK;
  } else return MPADEC_RETCODE_INVALID_HANDLE;
}

int mpadec2_reset(mpadec2_t mpadec)
{
  register struct mpadec2_t *mpa = (struct mpadec2_t *)mpadec;

  if (mpa && (mpa->size == sizeof(struct mpadec2_t))) {
    struct mpabuffer_t *buf = mpa->buffers, *tmp;
    while (buf) {
      tmp = buf;
      buf = buf->next;
      free(tmp);
    }
    mpa->buffers = NULL;
    mpa->in_buffer_offset = mpa->in_buffer_used = 0;
    mpa->out_buffer_offset = mpa->out_buffer_used = 0;
    mpadec_reset(mpa->mpadec);
    return MPADEC_RETCODE_OK;
  } else return MPADEC_RETCODE_INVALID_HANDLE;
}

int mpadec2_configure(mpadec2_t mpadec, mpadec_config_t *cfg)
{
  register struct mpadec2_t *mpa = (struct mpadec2_t *)mpadec;

  if (!mpa || (mpa->size != sizeof(struct mpadec2_t)))
    return MPADEC_RETCODE_INVALID_HANDLE;
  mpa->out_buffer_offset = mpa->out_buffer_used = 0;
  return (mpadec_configure(mpa->mpadec, cfg));
}

int mpadec2_get_info(mpadec2_t mpadec, void *info, int info_type)
{
  register struct mpadec2_t *mpa = (struct mpadec2_t *)mpadec;

  if (!mpa || (mpa->size != sizeof(struct mpadec2_t)))
    return MPADEC_RETCODE_INVALID_HANDLE;
  return (mpadec_get_info(mpa->mpadec, info, info_type));
}

int mpadec2_decode(mpadec2_t mpadec, uint8_t *srcbuf, uint32_t srcsize,
                   uint8_t *dstbuf, uint32_t dstsize, uint32_t *dstused)
{
  register struct mpadec2_t *mpa = (struct mpadec2_t *)mpadec;
  uint32_t n, src_used, dst_used; int r;

  if (dstused) *dstused = 0;
  if (!mpa || (mpa->size != sizeof(struct mpadec2_t)))
    return MPADEC_RETCODE_INVALID_HANDLE;
  if (((struct mpadec_t *)mpa->mpadec)->state < MPADEC_STATE_START)
    return MPADEC_RETCODE_BAD_STATE;
  if (srcbuf && srcsize) {
    struct mpabuffer_t *last = mpa->buffers, *buf;
    if (last) {
      while (last->next) last = last->next;
      if ((last->offset + last->used) < last->size) {
        n = last->size - (last->offset + last->used);
        if (n > srcsize) n = srcsize;
        memcpy(last->buffer + last->offset + last->used, srcbuf, n);
        last->used += n;
        srcbuf += n;
        srcsize -= n;
      }
    }
    if (srcsize) {
      n = (srcsize > 4096) ? srcsize : 4096;
      buf = (struct mpabuffer_t *)malloc(n + sizeof(struct mpabuffer_t));
      if (buf) {
        buf->size = n;
        buf->offset = buf->used = 0;
        buf->buffer = (uint8_t *)buf + sizeof(struct mpabuffer_t);
        buf->next = NULL;
        memcpy(buf->buffer, srcbuf, srcsize);
        buf->used = srcsize;
        if (last) last->next = buf; else mpa->buffers = buf;
      } else return MPADEC_RETCODE_NOT_ENOUGH_MEMORY;
    }
  }
  if (!dstbuf || !dstsize) return MPADEC_RETCODE_OK;
  while (dstsize) {
    struct mpabuffer_t *buf = mpa->buffers;
    if (mpa->out_buffer_used) {
      n = (dstsize < mpa->out_buffer_used) ? dstsize : mpa->out_buffer_used;
      memcpy(dstbuf, mpa->out_buffer + mpa->out_buffer_offset, n);
      mpa->out_buffer_offset += n;
      mpa->out_buffer_used -= n;
      dstbuf += n;
      dstsize -= n;
      if (dstused) *dstused += n;
    }
    if (!dstsize) break;
    if (mpa->in_buffer_used && mpa->in_buffer_offset)
      memmove(mpa->in_buffer,
              mpa->in_buffer + mpa->in_buffer_offset, mpa->in_buffer_used);
    mpa->in_buffer_offset = 0;
    while (buf && (mpa->in_buffer_used < sizeof(mpa->in_buffer))) {
      if (buf->used) {
        n = sizeof(mpa->in_buffer) - mpa->in_buffer_used;
        if (n > buf->used) n = buf->used;
        memcpy(mpa->in_buffer + mpa->in_buffer_offset + mpa->in_buffer_used,
               buf->buffer + buf->offset, n);
        buf->offset += n;
        buf->used -= n;
        mpa->in_buffer_used += n;
      }
      if (!buf->used) {
        struct mpabuffer_t *tmp = buf;
        buf = buf->next;
        free(tmp);
      }
    }
    mpa->buffers = buf;
    r = mpadec_decode(mpa->mpadec, mpa->in_buffer + mpa->in_buffer_offset,
                      mpa->in_buffer_used, dstbuf, dstsize, &src_used, &dst_used);
    mpa->in_buffer_offset += src_used;
    mpa->in_buffer_used -= src_used;
    dstbuf += dst_used;
    dstsize -= dst_used;
    if (dstused) *dstused += dst_used;
    if (r == MPADEC_RETCODE_BUFFER_TOO_SMALL) {
      mpa->out_buffer_offset = mpa->out_buffer_used = 0;
      mpadec_decode(mpa->mpadec, mpa->in_buffer + mpa->in_buffer_offset,
                    mpa->in_buffer_used, mpa->out_buffer, sizeof(mpa->out_buffer),
                    &src_used, &mpa->out_buffer_used);
      mpa->in_buffer_offset += src_used;
      mpa->in_buffer_used -= src_used;
      if (!mpa->out_buffer_used) break;
    } else if (!mpa->buffers) break;
  }
  return MPADEC_RETCODE_OK;
}
