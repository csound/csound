/*
  hrtferx.h:

  Copyright (C) 1995, 2001 Eli Breder, David McIntyre, John ffitch

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

/****************** hrtferxk.h *******************/

#pragma once

#include "3Dug.h"

typedef struct {
  OPDS  h;
  MYFLT         *aLeft, *aRight;             /* outputs  */
  MYFLT         *aIn, *kAz, *kElev;          /* inputs   */
  STRINGDAT     *ifilno; /* and inputs */
  MEMFIL        *mfp;                        /* file pointer */
  int16         *fpbegin;
  int32_t           oldel_index, oldaz_index;
  int32         incount, outfront, outend, outcount;
  AUXCH         auxch;      /* will point to allocated memory */
  HRTF_DATUM    hrtf_data, oldhrtf_data;  /* matrix to store HRTF data */
  MYFLT         outl[BUF_LEN], outr[BUF_LEN];
  MYFLT         x[BUF_LEN], yl[BUF_LEN], yr[BUF_LEN];
  MYFLT         bl[FILT_LENm1], br[FILT_LENm1];
  void *setup, *isetup;
} HRTFER;
