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

/* $Id$ */

#ifndef __MPADEC_H
#define __MPADEC_H

#undef FALSE
#undef TRUE
#define FALSE 0
#define TRUE  1

#define MPADEC_VERSION 0x0900

#define MPADEC_RETCODE_OK                 0
#define MPADEC_RETCODE_INVALID_HANDLE     1
#define MPADEC_RETCODE_NOT_ENOUGH_MEMORY  2
#define MPADEC_RETCODE_BAD_STATE          3
#define MPADEC_RETCODE_INVALID_PARAMETERS 4
#define MPADEC_RETCODE_NEED_MORE_DATA     5
#define MPADEC_RETCODE_BUFFER_TOO_SMALL   6
#define MPADEC_RETCODE_NO_SYNC            7
#define MPADEC_RETCODE_UNKNOWN            8

#define MPADEC_CONFIG_FULL_QUALITY 0
#define MPADEC_CONFIG_HALF_QUALITY 1

#define MPADEC_CONFIG_AUTO     0
#define MPADEC_CONFIG_MONO     1
#define MPADEC_CONFIG_STEREO   2
#define MPADEC_CONFIG_CHANNEL1 3
#define MPADEC_CONFIG_CHANNEL2 4

#define MPADEC_CONFIG_16BIT 0
#define MPADEC_CONFIG_24BIT 1
#define MPADEC_CONFIG_32BIT 2
#define MPADEC_CONFIG_FLOAT 3

#define MPADEC_CONFIG_LITTLE_ENDIAN 0
#define MPADEC_CONFIG_BIG_ENDIAN    1

#define MPADEC_CONFIG_REPLAYGAIN_NONE       0
#define MPADEC_CONFIG_REPLAYGAIN_RADIO      1
#define MPADEC_CONFIG_REPLAYGAIN_AUDIOPHILE 2
#define MPADEC_CONFIG_REPLAYGAIN_CUSTOM     3

#define MPADEC_INFO_STREAM 0
#define MPADEC_INFO_TAG    1
#define MPADEC_INFO_CONFIG 2

typedef struct {
  uint8_t quality;
  uint8_t mode;
  uint8_t format;
  uint8_t endian;
  uint8_t replaygain;
  uint8_t skip;
  uint8_t crc;
  uint8_t dblsync;
  float gain;
} mpadec_config_t;

typedef struct {
  int32_t layer;
  int32_t channels;
  int32_t frequency;
  int32_t bitrate;
  uint8_t mode;
  uint8_t copyright;
  uint8_t original;
  uint8_t emphasis;
  int32_t frames;
  int32_t frame_size;
  int32_t frame_samples;
  int32_t decoded_channels;
  int32_t decoded_frequency;
  int32_t decoded_sample_size;
  int32_t decoded_frame_samples;
  int32_t duration;
} mpadec_info_t;

typedef struct {
  uint32_t flags;
  uint32_t frames;
  uint32_t bytes;
  uint8_t toc[100];
  int32_t replay_gain[2];
  int32_t enc_delay;
  int32_t enc_padding;
} mp3tag_info_t;

typedef void *mpadec_t;
typedef void *mpadec2_t;

#ifdef __cplusplus
extern "C" {
#endif

mpadec_t mpadec_init(void);
int mpadec_uninit(mpadec_t mpadec);
int mpadec_reset(mpadec_t mpadec);
int mpadec_configure(mpadec_t mpadec, mpadec_config_t *cfg);
int mpadec_get_info(mpadec_t mpadec, void *info, int info_type);
int mpadec_decode(mpadec_t mpadec, uint8_t *srcbuf, uint32_t srcsize,
                  uint8_t *dstbuf, uint32_t dstsize, uint32_t *srcused,
                  uint32_t *dstused);
char *mpadec_error(int code);

mpadec2_t mpadec2_init(void);
int mpadec2_uninit(mpadec2_t mpadec);
int mpadec2_reset(mpadec2_t mpadec);
int mpadec2_configure(mpadec2_t mpadec, mpadec_config_t *cfg);
int mpadec2_get_info(mpadec2_t mpadec, void *info, int info_type);
int mpadec2_decode(mpadec2_t mpadec, uint8_t *srcbuf, uint32_t srcsize,
                   uint8_t *dstbuf, uint32_t dstsize, uint32_t *dstused);
#define mpadec2_error(x) mpadec_error(x)

#ifdef __cplusplus
}
#endif

#endif
