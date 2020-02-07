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

#ifndef __MP3DEC_H
#define __MP3DEC_H

#include <stdint.h>
#include "mpadec.h"

#define MP3DEC_RETCODE_OK                 0
#define MP3DEC_RETCODE_INVALID_HANDLE     1
#define MP3DEC_RETCODE_BAD_STATE          2
#define MP3DEC_RETCODE_INVALID_PARAMETERS 3
#define MP3DEC_RETCODE_NOT_MPEG_STREAM    4
#define MP3DEC_RETCODE_SEEK_FAILED        5
#define MP3DEC_RETCODE_UNKNOWN            6

#define MP3DEC_SEEK_BYTES   0
#define MP3DEC_SEEK_SAMPLES 1
#define MP3DEC_SEEK_SECONDS 2

typedef void *mp3dec_t;

#ifdef __cplusplus
extern "C" {
#endif

mp3dec_t mp3dec_init(void);
int mp3dec_init_file(mp3dec_t mp3dec, int fd, int64_t length, int nogap);
int mp3dec_uninit(mp3dec_t mp3dec);
int mp3dec_reset(mp3dec_t mp3dec);
int mp3dec_configure(mp3dec_t mp3dec, mpadec_config_t *cfg);
int mp3dec_get_info(mp3dec_t mp3dec, void *info, int info_type);
int mp3dec_decode(mp3dec_t mp3dec, uint8_t *buf, uint32_t bufsize, uint32_t *used);
int mp3dec_seek(mp3dec_t mp3dec, int64_t pos, int units);
char *mp3dec_error(int code);

#ifdef __cplusplus
}
#endif

#endif
