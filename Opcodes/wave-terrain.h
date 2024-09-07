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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#pragma once

#include "stdopcod.h"

typedef struct {

  OPDS h;

  MYFLT *aout;
  MYFLT *kamp;

  MYFLT *kpch;

  MYFLT *kcx, *kcy;
  MYFLT *krx, *kry;

  MYFLT *i_tabx, *i_taby;       /* Table numbers */
/* Internals */

  MYFLT *xarr, *yarr;           /* Actual tables */

  MYFLT sizx, sizy;
  double theta;

} WAVETER;

typedef struct {

  OPDS h;

  MYFLT *isrc;
  MYFLT *idst;
  MYFLT *ipos;
  MYFLT *imode;

  /* imode:
     0 : dest = src
     1 : dest = dest + src/imode
  */

} SCANHAMMER;

typedef struct {

  OPDS h;

  MYFLT *aout;
  MYFLT *kamp;
  MYFLT *kpch;
  MYFLT *i_point;
  MYFLT *i_mass;
  MYFLT *i_stiff;
  MYFLT *i_damp;
  MYFLT *i_vel;

/* End of arguments */

  AUXCH newloca;
  AUXCH newvela;
  MYFLT *newloc, *newvel;
  MYFLT size;

  MYFLT pos;
  FUNC *fpoint;
  FUNC *fmass;
  FUNC *fstiff;
  FUNC *fdamp;
  FUNC *fvel;

} SCANTABLE;
