/*
    srconvert.h

    Copyright (C) 2024 Victor Lazzarini

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

#ifndef SRCONVERT_H
#define SRCONVERT_H

#include "csound.h"

typedef struct _SR_CONVERTER
{
  float *bufferin, *bufferout;
  float   ratio;
  int32_t     size;
  int32_t     cnt;
  int32_t     mode;
  void   *data;
} SR_CONVERTER;


SR_CONVERTER *src_init(CSOUND *, int32_t, float, int32_t);
void src_deinit(CSOUND *, SR_CONVERTER *);
int32_t src_convert(CSOUND *, SR_CONVERTER *, MYFLT *, MYFLT *);

#endif
