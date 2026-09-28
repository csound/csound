/*
  ugens4.h:

  Copyright (C) 1991 Barry Vercoe, John ffitch

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

/*                                                      UGENS4.H        */

#pragma once

typedef struct {
  OPDS    h;
  cs_float   *ar, *xamp, *xcps, *knh, *ifn, *iphs;
  int16   ampcod, cpscod;
  int32    lphs;
  FUNC    *ftp;
  int32_t     reported;
  int32_t  floatph;
  cs_double fphs;
} BUZZ;

typedef struct {
  OPDS    h;
  cs_float   *ar, *xamp, *xcps, *kn, *kk, *kr, *ifn, *iphs;
  int16   ampcod, cpscod;
  int32_t prvn;
  cs_float   prvr, twor, rsqp1, rtn, rtnp1, rsumr;
  int32    lphs;
  FUNC    *ftp;
  int32_t     reported;
  cs_float   last;
  int32_t  floatph;
  cs_double fphs;
} GBUZZ;

typedef struct {
  OPDS    h;
  cs_float   *ar, *kamp, *kcps, *icps, *ifn, *imeth, *ipar1, *ipar2;
  cs_float   sicps, param1, param2;
  int16   thresh1, thresh2, method;
  int32    phs256, npts, maxpts;
  AUXCH   auxch;
} PLUCK;

typedef struct {
  OPDS    h;
  cs_float   *ar, *xamp, *iseed, *sel, *base;
  int32_t     rand;
  int16   ampcod;
  int16   new;
} RAND;

typedef struct {
  OPDS    h;
  cs_float   *ar, *xamp, *xcps, *iseed, *sel, *base;
  int16   ampcod, cpscod, new;
  int32_t     rand;
  long    phs;
  cs_float   num1;
} RANDH;

typedef struct {
  OPDS    h;
  cs_float   *ar, *xamp, *xcps, *iseed, *sel, *base;
  int16   ampcod, cpscod, new;
  int32_t     rand;
  long    phs;
  cs_float   num1, num2, dfdmax;
} RANDI;

typedef struct {
  OPDS    h;
  cs_float   *ar, *xamp, *xcps, *iseed, *sel, *base;
  int16   ampcod, cpscod, new;
  int32_t     rand;
  int64_t phs;
  cs_float   period, num1, num2;
  cs_float   num3, num4;
} RANDC;
