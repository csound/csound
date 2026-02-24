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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifndef SRCONVERT_H
#define SRCONVERT_H

#include "csoundCore.h"
#include "csound_type_system.h"
#include "csound_standard_types.h"

typedef struct {
  float *bufferin, *bufferout; // buffers
  void   *data;  // converter state
  int32_t cnt;   // rw count
} CVTDAT;
  
typedef struct _SR_CONVERTER {
  float   ratio;  // src ratio
  int32_t size;   // vector size
  int32_t mode;   // conversion mode
  CVTDAT *dat; // converter data
  int32_t ncvt;   // no of converters
  CS_VARIABLE *var; // argument var
  CSOUND *csound;   
  INSDS *ip;  // calling insds
} SR_CONVERTER;


/** converter initialisation
    one per argument
    mode - conversion mode (0 - 4)
    ratio - conversion ratio (oversampling or 1/oversampling factor)
    size - vector size
    var - actual variable given to conversion
    ip - calling insds
*/
SR_CONVERTER *src_init(CSOUND *csound, int32_t mode,
                       float ratio, int32_t size, CS_VARIABLE *var,
                       INSDS *ip);

/** conversion de-initialisation
 */  
void src_deinit(CSOUND *csound, SR_CONVERTER *p);

/** sampling rate conversion
    in - signal to be converted
    out - signal to be converted

    src is only performed on time-domain signals (a,k and arrays thereof)
    other variable types are simply copied
 */
int32_t src_convert(CSOUND *csoubnd, SR_CONVERTER *p, MYFLT *in, MYFLT *out);

#endif
