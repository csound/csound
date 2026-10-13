/*
    disprep.h:

    Copyright (C) 1991 Barry Vercoe

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

                        /*                                      DISPREP.H       */
#pragma once

#include "pstream.h"

typedef struct {
        OPDS    h;
        cs_float   *iargs[VARGMAX];
} PRINTV;

typedef struct {
        OPDS    h;
        cs_float   *signal, *iprd, *inprds, *iwtflg;
        int32    npts, nprds, bufpts, totpts, pntcnt;
        WINDAT  dwindow;
        cs_float   *nxtp, *begp, *endp;
        AUXCH   auxch;
} DSPLAY;

typedef struct {
        OPDS    h;
        PVSDAT  *fin;
        cs_float   *points, *flag;
        int32_t     size;
        WINDAT  dwindow;
        AUXCH   fdata;
        uint32  lastframe;
} FSIGDISP;

#define WINDMAX 16384
#define WINDMIN 16

typedef struct {
        OPDS    h;
        cs_float   *signal, *iprd, *inpts, *ihann, *idbout, *iwtflg, *imin, *imax;
        cs_float   *sampbuf, *bufp, *endp, overN;
        int32   windsize, overlap, ncoefs;
        int32_t     hanning, dbout;
        int32_t     npts, start;
        WINDAT  dwindow;
        AUXCH   auxch;
        AUXCH  smpbuf;
} DSPFFT;

typedef struct {
        OPDS    h;
        cs_float   *kout,*kin,*iprd,*imindur,*imemdur,*ihp,*ithresh,*ihtim,*ixfdbak;
        cs_float   *istartempo,*ifn,*idisprd,*itweek;
        int32_t     countdown, timcount, npts, minlam, maxlam;
        cs_float   *hbeg, *hcur, *hend;
        cs_float   *xbeg, *xcur, *xend;
        cs_float   *stmemp, *linexp, *ftable, *xscale, *lmults;
        int16   *lambdas;
        cs_float   *stmemnow, ncross, coef0, coef1, yt1, thresh;
        cs_float   fwdcoef, fwdmask, xfdbak, avglam, tempscal, tempo, tweek;
        int32_t     dcntdown, dtimcnt;
        WINDAT  dwindow;
        AUXCH   auxch;
} TEMPEST;

