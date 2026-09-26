/*
  ugens1.h:

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

/*                                                         UGENS1.H        */

#pragma once

typedef struct {
  OPDS    h;
  cs_float   *xr, *ia, *idur, *ib;
  cs_double   val, incr, kincr;
} LINE;

typedef struct {
  OPDS    h;
  cs_float   *xr, *ia, *idur, *ib;
  cs_double   val, mlt, kmlt;
} EXPON;

typedef struct {
  int32  cnt, acnt;
  cs_float  val, mlt, amlt;
} XSEG;

typedef struct {
  int32   cnt, acnt;
  cs_double  nxtpt;
} SEG;

typedef struct {
  OPDS    h;
  cs_float   *rslt, *argums[VARGMAX];
  SEG     *cursegp;
  int32   nsegs;
  int32   segsrem, curcnt;
  cs_double  curval, curinc, curainc;
  AUXCH   auxch;
  int32   xtra;
} LINSEG;

typedef struct {
  OPDS    h;
  cs_float   *rslt, *argums[6];
  int32_t counts[5], stage, xtra;
  int32_t midi, exponential, hold, initialized;
  uint32_t scale;
  int64_t remaining;
  cs_double sustain, value, target, increment, multiplier;
} ADSR;

typedef struct {
  OPDS    h;
  cs_float   *rslt, *argums[VARGMAX];
  SEG     *cursegp;
  int32   nsegs;
  int32   segsrem, curcnt;
  cs_double  y1, y2, x, inc, val;
  AUXCH   auxch;
  int32   xtra;
} COSSEG;

typedef struct {
  OPDS    h;
  cs_float   *rslt, *argums[VARGMAX];
  SEG     *cursegp;
  int32   segsrem, curcnt;
  cs_double  curval, curmlt, curamlt;
  int32   nsegs;
  AUXCH   auxch;
  int32   xtra;
} EXPSEG;

typedef struct {
  OPDS    h;
  cs_float   *rslt, *argums[VARGMAX];
  XSEG    *cursegp;
  int32   segsrem, curcnt;
  cs_double  curval, curmlt, curamlt;
  int32   nsegs;
  AUXCH   auxch;
} EXXPSEG;

typedef struct {
  OPDS    h;
  cs_float   *rslt, *sig, *iris, *idur, *idec;
  cs_double  lin1, inc1, lin2, inc2;
  int64_t  cnt1, cnt2;
} LINEN;

typedef struct {
  OPDS    h;
  cs_float   *rslt, *sig, *iris, *idec, *iatdec;
  cs_double  lin1, inc1, val, val2, mlt2;
  int64_t  cnt1;
} LINENR;

typedef struct {
  OPDS    h;
  cs_float   *rslt, *xamp, *irise, *idur, *idec, *ifn, *iatss;
  cs_float   *iatdec, *ixmod;
  int32   phs, ki, cnt1;
  cs_double  val, mlt1, mlt2, asym, phsf, kif;
  FUNC    *ftp;
  int32  floatph;
} ENVLPX;

typedef struct {
  OPDS    h;
  cs_float   *rslt, *xamp, *irise, *idec, *ifn, *iatss, *iatdec;
  cs_float   *ixmod, *irind;
  int32   phs, ki, rlsing, rlscnt, rindep;
  cs_double  val, mlt1, mlt2, asym, atdec, kif, phsf;
  FUNC    *ftp;
  int32  floatph;
} ENVLPR;

typedef struct {
  OPDS    h;
  cs_float   *rslt, *argums[VARGMAX];
  XSEG    *cursegp;
  int32   nsegs;
  AUXCH   auxch;
} EXPSEG2;                         /*gab-A1*/
