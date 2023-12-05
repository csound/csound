/*
    ugmoss.h:

    Copyright (C) 2001 Willian 'Pete' Moss

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

                                                         /* ugmoss.h */

#pragma once

typedef struct {
  OPDS                  h;
  MYFLT                 *ar, *ain, *isize, *ifn;
  MYFLT                 *curp;
  FUNC                  *ftp;
  AUXCH                 sigbuf;
  uint32_t          len;
} DCONV;

typedef struct {
  OPDS                  h;
  MYFLT                 *ar, *asig, *krvt, *xlpt, *imaxlpt, *istor, *insmps;
  MYFLT                 g, rvt, lpt, *pntr, maxlpt;
  AUXCH                 auxch;
  int16                 lpta;
} VCOMB;

typedef struct {
  OPDS                  h;
  MYFLT                 *kftndx, *iftfn, *iresfn;
  FUNC                  *ftfn, *resfn;
  MYFLT                 ftndx;
  uint32_t              len;
} FTMORF;

/* end ugmoss.h */
