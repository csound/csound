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
  MYFLT   *xr, *ia, *idur, *ib;
  MYDBL   val, incr, kincr;
} LINE;

typedef struct {
  OPDS    h;
  MYFLT   *xr, *ia, *idur, *ib;
  MYDBL   val, mlt, kmlt;
} EXPON;

typedef struct {
  int32  cnt, acnt;
  MYFLT  val, mlt, amlt;
} XSEG;

typedef struct {
  int32   cnt, acnt;
  MYDBL  nxtpt;
} SEG;

typedef struct {
  OPDS    h;
  MYFLT   *rslt, *argums[VARGMAX];
  SEG     *cursegp;
  int32   nsegs;
  int32   segsrem, curcnt;
  MYDBL  curval, curinc, curainc;
  AUXCH   auxch;
  int32   xtra;
} LINSEG;

typedef struct {
  OPDS    h;
  MYFLT   *rslt, *argums[VARGMAX];
  SEG     *cursegp;
  int32   nsegs;
  int32   segsrem, curcnt;
  MYDBL  y1, y2, x, inc, val;
  AUXCH   auxch;
  int32   xtra;
} COSSEG;

typedef struct {
  OPDS    h;
  MYFLT   *rslt, *argums[VARGMAX];
  SEG     *cursegp;
  int32   segsrem, curcnt;
  MYDBL  curval, curmlt, curamlt;
  int32   nsegs;
  AUXCH   auxch;
  int32   xtra;
} EXPSEG;

typedef struct {
  OPDS    h;
  MYFLT   *rslt, *argums[VARGMAX];
  XSEG    *cursegp;
  int32   segsrem, curcnt;
  MYDBL  curval, curmlt, curamlt;
  int32   nsegs;
  AUXCH   auxch;
} EXXPSEG;

typedef struct {
  OPDS    h;
  MYFLT   *rslt, *sig, *iris, *idur, *idec;
  MYDBL  lin1, inc1, lin2, inc2;
  int64_t  cnt1, cnt2;
} LINEN;

typedef struct {
  OPDS    h;
  MYFLT   *rslt, *sig, *iris, *idec, *iatdec;
  MYDBL  lin1, inc1, val, val2, mlt2;
  int64_t  cnt1;
} LINENR;

typedef struct {
  OPDS    h;
  MYFLT   *rslt, *xamp, *irise, *idur, *idec, *ifn, *iatss;
  MYFLT   *iatdec, *ixmod;
  int32   phs, ki, cnt1;
  MYDBL  val, mlt1, mlt2, asym, phsf, kif;
  FUNC    *ftp;
  int32  floatph;
} ENVLPX;

typedef struct {
  OPDS    h;
  MYFLT   *rslt, *xamp, *irise, *idec, *ifn, *iatss, *iatdec;
  MYFLT   *ixmod, *irind;
  int32   phs, ki, rlsing, rlscnt, rindep;
  MYDBL  val, mlt1, mlt2, asym, atdec, kif, phsf;
  FUNC    *ftp;
  int32  floatph;
} ENVLPR;

typedef struct {
  OPDS    h;
  MYFLT   *rslt, *argums[VARGMAX];
  XSEG    *cursegp;
  int32   nsegs;
  AUXCH   auxch;
} EXPSEG2;                         /*gab-A1*/

