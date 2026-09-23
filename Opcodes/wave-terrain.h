/*
    wave-terrain.h:

    Copyright (C) 2002 Matt Gilliard, John ffitch

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

#pragma once

#include "stdopcod.h"

typedef struct {

  OPDS h;

  cs_float *aout;
  cs_float *kamp;

  cs_float *kpch;

  cs_float *kcx, *kcy;
  cs_float *krx, *kry;

  cs_float *i_tabx, *i_taby;       /* Table numbers */
/* Internals */

  cs_float *xarr, *yarr;           /* Actual tables */

  cs_float sizx, sizy;
  cs_double theta;

} WAVETER;

typedef struct {

  OPDS h;

  cs_float *isrc;
  cs_float *idst;
  cs_float *ipos;
  cs_float *imode;

  /* imode:
     0 : dest = src
     1 : dest = dest + src/imode
  */

} SCANHAMMER;

typedef struct {

  OPDS h;

  cs_float *aout;
  cs_float *kamp;
  cs_float *kpch;
  cs_float *i_point;
  cs_float *i_mass;
  cs_float *i_stiff;
  cs_float *i_damp;
  cs_float *i_vel;

/* End of arguments */

  AUXCH newloca;
  AUXCH newvela;
  cs_float *newloc, *newvel;
  uint32_t size;

  cs_double pos;
  FUNC *fpoint;
  FUNC *fmass;
  FUNC *fstiff;
  FUNC *fdamp;
  FUNC *fvel;

} SCANTABLE;
