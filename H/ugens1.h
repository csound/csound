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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/*                                                                      UGENS1.H        */

typedef struct {
        OPDS    h;
        MYFLT   *xr, *ia, *idur, *ib;
        MYFLT   val, incr;
} LINE;

typedef struct {
        OPDS    h;
        MYFLT   *xr, *ia, *idur, *ib;
        MYFLT   val, mlt;
} EXPON;

typedef struct {
        long   cnt;
        MYFLT  val, mlt;
} XSEG;

typedef struct {
        long   cnt;
        MYFLT  nxtpt;
} SEG;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *argums[VARGMAX];
        SEG     *cursegp;
        long    nsegs;
        long    segsrem, curcnt;
        MYFLT   curval, curinc, curainc;
        AUXCH   auxch;
        long    xtra;
} LINSEG;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *argums[VARGMAX];
        SEG     *cursegp;
        long    segsrem, curcnt;
        MYFLT   curval, curmlt, curamlt;
        long    nsegs;
        AUXCH   auxch;
        long    xtra;
} EXPSEG;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *argums[VARGMAX];
        XSEG    *cursegp;
        long    segsrem, curcnt;
        MYFLT   curval, curmlt, curamlt;
        long    nsegs;
        AUXCH   auxch;
} EXXPSEG;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *sig, *iris, *idur, *idec;
        MYFLT   lin1, inc1, val, lin2, inc2;
        long    cnt1, cnt2;
} LINEN;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *sig, *iris, *idec, *iatdec;
        MYFLT   lin1, inc1, val, val2, mlt2;
        long    cnt1;
} LINENR;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *xamp, *irise, *idur, *idec, *ifn, *iatss;
        MYFLT   *iatdec, *ixmod;
        long    phs, ki, cnt1;
        MYFLT   val, mlt1, mlt2, asym;
        FUNC    *ftp;
} ENVLPX;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *xamp, *irise, *idec, *ifn, *iatss, *iatdec;
        MYFLT   *ixmod, *irind;
        long    phs, ki, rlsing, rlscnt, rindep;
        MYFLT   val, mlt1, mlt2, asym, atdec;
        FUNC    *ftp;
} ENVLPR;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *argums[VARGMAX];
        XSEG    *cursegp;
        long    nsegs;
        AUXCH   auxch;
} EXPSEG2;                         /*gab-A1*/
