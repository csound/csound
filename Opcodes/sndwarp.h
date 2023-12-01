/*
    sndwarp.h:

    Copyright (C) 1997 Richard Karpen

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

typedef struct {
  int32_t    cnt, wsize, flag; /* , section; */
        MYFLT  ampincr, ampphs, offset;
} WARPSECTION;

typedef struct {
    OPDS    h;
    MYFLT   *r1, *r2, *xamp, *xtimewarp, *xresample, *isampfun, *ibegin,
            *iwsize, *irandw, *ioverlap, *ifn, *itimemode;
    FUNC    *ftpWind, *ftpSamp;
    int32   maxFr, prFlg, flen, sampflen, nsections;
    int32_t     chans, *frPtr, begin;
    WARPSECTION *exp;
    AUXCH   auxch;
    int16   ampcode, timewarpcode, resamplecode;
} SNDWARP;

typedef struct {
    OPDS    h;
    MYFLT   *r1, *r2, *r3, *r4, *xamp, *xtimewarp, *xresample, *isampfun,
            *ibegin, *iwsize, *irandw, *ioverlap, *ifn, *itimemode;
    FUNC    *ftpWind,  *ftpSamp;
    int32   maxFr, prFlg, flen, sampflen, nsections;
    int32_t     chans, *frPtr, begin;
    WARPSECTION *exp;
    AUXCH   auxch;
    int16   ampcode, timewarpcode, resamplecode;
} SNDWARPST;

