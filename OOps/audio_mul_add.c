/* audio_mul_add.c:

   Copyright (C) 1991 Barry Vercoe, John ffitch, Gabriel Maldonado
   (c) 2024 V Lazzarini

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

#include "csoundCore.h"
#include "aops.h"
#include <float.h>

/* Private multiply-then-add kernel used by the orchestra optimizer. */
int32_t audio_mul_add(CSOUND *csound, AUDIO_MUL_ADD *p)
{
    IGN(csound);
    uint32_t count = CS_KSMPS, offset = 0;
    const cs_float gain = *p->gain;
    /* Match mulak/addaa, including their ksmps=1 path. */
    if (count != 1) {
      offset = p->h.insdshead->ksmps_offset;
      uint32_t early = p->h.insdshead->ksmps_no_end;
      if (offset) memset(p->out, 0, offset * sizeof(cs_float));
      if (early) {
        count -= early;
        memset(p->out + count, 0, early * sizeof(cs_float));
      }
    }
    for (uint32_t n = offset; n < count; n++) {
      /* CMake disables contraction for this source. Other build systems, or
         targets with excess evaluation precision, need a rounded volatile
         store. An ordinary C temporary alone does not prevent an FMA. */
#if defined(CSOUND_FUSION_STRICT_FP) && defined(FLT_EVAL_METHOD) && FLT_EVAL_METHOD == 0
      cs_float product = p->input[n] * gain;
#else
      volatile cs_float product = p->input[n] * gain;
#endif
      p->out[n] = product + p->other[n];
    }
    return OK;
}
