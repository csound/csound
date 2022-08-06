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

/* $Id: mpadec_internal.h,v 1.1.1.1 2004/07/27 02:57:55 metal_man Exp $ */

#ifndef __MPADEC_INTERNAL_H
#define __MPADEC_INTERNAL_H

#include "mpadec_config.h"
#include "mpadec.h"

#define MPADEC_STATE_UNDEFINED 0
#define MPADEC_STATE_START     1
#define MPADEC_STATE_DECODE    2

#define MPG_MD_STEREO       0
#define MPG_MD_JOINT_STEREO 1
#define MPG_MD_DUAL_CHANNEL 2
#define MPG_MD_MONO         3

#define SBLIMIT     32
#define SSLIMIT     18
#define SCALE_BLOCK 12

typedef struct {
  FLOAT decwin[512 + 32];
  FLOAT muls[27][64];
  FLOAT gainpow2[256 + 122];
  FLOAT ispow[8207];
  FLOAT win[2][4][36];
  FLOAT *istabs[3][2][2];
  FLOAT tan1_1[16];
  FLOAT tan2_1[16];
  FLOAT tan1_2[16];
  FLOAT tan2_2[16];
  FLOAT pow1_1[2][16];
  FLOAT pow2_1[2][16];
  FLOAT pow1_2[2][16];
  FLOAT pow2_2[2][16];
  int32_t long_limit[9][23];
  int32_t short_limit[9][14];
  int32_t n_slen2[512];
  int32_t i_slen2[256];
  int32_t mapbuf0[9][152];
  int32_t mapbuf1[9][156];
  int32_t mapbuf2[9][44];
  int32_t *map[9][3];
  int32_t *mapend[9][3];
  uint8_t *mp2tables[10];
  uint8_t grp3tab[32*3];
  uint8_t grp5tab[128*3];
  uint8_t grp9tab[1024*3];
} mpadec_tables_t;

typedef struct {
  uint16_t bits;
  int16_t d;
} alloc_table_t;

typedef struct {
  uint32_t linbits;
  int16_t *table;
} newhuff_t;

typedef struct {
  int16_t long_idx[23];
  int16_t long_diff[22];
  int16_t short_idx[14];
  int16_t short_diff[13];
} bandinfo_t;

typedef struct {
  uint8_t layer;
  uint8_t mode;
  uint8_t channels;
  uint8_t decoded_channels;
  uint8_t LSF;
  uint8_t MPEG25;
  uint8_t CRC;
  uint8_t extension;
  uint8_t mode_ext;
  uint8_t copyright;
  uint8_t original;
  uint8_t emphasis;
  uint8_t jsbound;
  uint8_t sblimit;
  uint8_t downsample;
  uint8_t downsample_sblimit;
  uint16_t crc;
  uint16_t rsvd;
  uint32_t padding;
  uint32_t bitrate_index;
  uint32_t frequency_index;
  uint32_t bitrate;
  uint32_t frequency;
  uint32_t frame_size;
  uint32_t frame_samples;
  uint32_t decoded_frequency;
  uint32_t decoded_samples;
  uint32_t decoded_size;
  alloc_table_t *alloc_table;
} frameinfo_t;

typedef struct {
  int32_t scfsi;
  uint32_t part2_3_length;
  uint32_t big_values;
  uint32_t scalefac_compress;
  uint8_t block_type;
  uint8_t mixed_block_flag;
  uint8_t preflag;
  uint8_t scalefac_scale;
  uint32_t table_select[3];
  uint32_t subblock_gain[3];
  uint32_t maxband[3];
  uint32_t maxbandl;
  uint32_t maxb;
  uint32_t region1start;
  uint32_t region2start;
  uint32_t count1table_select;
  FLOAT *full_gain[3];
  FLOAT *pow2gain;
} grinfo_t;

typedef struct {
  uint32_t main_data_begin;
  uint32_t private_bits;
  struct {
    grinfo_t gr[2];
  } ch[2];
} sideinfo_t;

struct mpadec_t {
  uint32_t size;
  uint32_t state;
  uint8_t *next_byte;
  uint32_t bytes_left;
  uint32_t bit_buffer;
  uint8_t bits_left;
  uint8_t error;
  uint8_t free_format;
  uint8_t pad1;
  uint32_t sample_size;
  uint32_t prev_frame_size;
  uint32_t header;
  uint32_t hsize;
  uint32_t ssize;
  uint32_t dsize;
  uint16_t crc;
  uint16_t pad2;
  uint32_t skip_samples;
  uint32_t padding_samples;
  uint32_t padding_start;
  uint32_t decoded_frames;
  uint32_t decoded_samples;
  mp3tag_info_t tag_info;
  uint32_t synth_size;
  FLOAT replay_gain;
  void (*synth_func)(void *mpadec, FLOAT block[SBLIMIT],
                     int channel, uint8_t *buffer);
  uint32_t reservoir_size;
  uint8_t reservoir[2048];
  frameinfo_t frame;
  sideinfo_t sideinfo;
  mpadec_config_t config;
  mpadec_tables_t tables;
  uint32_t synth_bufoffs;
  uint8_t hybrid_block[4];
  FLOAT hybrid_in[2][SBLIMIT][SSLIMIT];
  FLOAT hybrid_out[2][SSLIMIT][SBLIMIT];
  FLOAT hybrid_buffers[2][2][SBLIMIT*SSLIMIT];
  FLOAT synth_buffers[2][2][0x110];
};

struct mpabuffer_t {
  uint32_t size;
  uint32_t offset;
  uint32_t used;
  uint8_t *buffer;
  struct mpabuffer_t *next;
};

struct mpadec2_t {
  uint32_t size;
  mpadec_t mpadec;
  struct mpabuffer_t *buffers;
  uint32_t in_buffer_offset;
  uint32_t in_buffer_used;
  uint32_t out_buffer_offset;
  uint32_t out_buffer_used;
  uint8_t in_buffer[0x10000];
  uint8_t out_buffer[8*1152];
};

#define GETBITS(n) ((mpa->bits_left >= (uint8_t)(n)) \
                    ? ((mpa->bit_buffer >> (mpa->bits_left -= \
                       (uint8_t)(n))) & bitmask[n]) : mpa_getbits(mpa, n))

#endif
