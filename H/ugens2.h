/*
    ugens2.h:

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

/*                                                              UGENS2.H        */

typedef struct {
        OPDS    h;
        MYFLT   *sr, *xcps, *iphs;
        double  curphs;
} PHSOR;

typedef struct {
        OPDS    h;
        MYFLT   *sr,*aphs, *xcps, *kR, *iphs;
        double  curphs;
        double  b;
} EPHSOR;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *xndx, *xfn, *ixmode, *ixoff, *iwrap;
        MYFLT   offset;
        int32   pfn;
        int32   xbmul;
        int     wrap;
        FUNC    *ftp;
} TABLE;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *idel, *kamp, *idur, *ifn;
        int32   kinc, phs;
        int32   dcnt;
        FUNC    *ftp;
} OSCIL1;

typedef struct  {
        OPDS    h;
        MYFLT   *rslt, *kamp, *ifrq, *ifn, *itimes;
        MYFLT   index, inc, maxndx;
        int32   ntimes;
        FUNC    *ftp;
} OSCILN;

typedef struct {
        OPDS    h;
        MYFLT   *sr, *xamp, *xcps, *ifn, *iphs;
        int32   lphs;
        FUNC    *ftp;
        FUNC    FF;
} OSC;
