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

/* $Id: mp3dec_internal.h,v 1.2 2004/08/02 09:38:09 metal_man Exp $ */

#ifndef __MP3DEC_INTERNAL_H
#define __MP3DEC_INTERNAL_H

#include "mpadec_config.h"
#include "mp3dec.h"

#define MP3DEC_FLAG_INITIALIZED 1
#define MP3DEC_FLAG_SEEKABLE    2

struct mp3dec_t {
  uint32_t size;
  mpadec_t mpadec;
  int32_t fd;
  uint32_t flags;
  off_t stream_offset;
  int64_t stream_size;
  int64_t stream_position;
  mpadec_info_t mpainfo;
  mp3tag_info_t taginfo;
  uint32_t in_buffer_offset;
  uint32_t in_buffer_used;
  uint32_t out_buffer_offset;
  uint32_t out_buffer_used;
  uint8_t in_buffer[0x10000];
  uint8_t out_buffer[8*1152];
};

#endif

