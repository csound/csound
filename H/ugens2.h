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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/*                                                              UGENS2.H        */

#pragma once

typedef struct {
        OPDS    h;
        cs_float   *sr, *xcps, *iphs;
        cs_double  curphs;
} PHSOR;

typedef struct {
        OPDS    h;
        cs_float   *sr,*aphs, *xcps, *kR, *iphs;
        cs_double  curphs;
        cs_double  b;
} EPHSOR;

typedef struct {
        OPDS    h;
        cs_float   *rslt, *xndx, *xfn, *ixmode, *ixoff, *iwrap;
        cs_float   offset;
        int32   pfn;
        int32   xbmul;
        int32_t     wrap;
        FUNC    *ftp;
} TABLE;

typedef struct {
        OPDS    h;
        cs_float   *rslt, *idel, *kamp, *idur, *ifn;
        int32   kinc, phs;
        cs_double   fphs, inc;
        int32   dcnt;
        FUNC    *ftp;
} OSCIL1;

typedef struct  {
        OPDS    h;
        cs_float   *rslt, *kamp, *ifrq, *ifn, *itimes;
        cs_double  phase, inc;
        int32_t ntimes, cycles;
        FUNC    *ftp;
} OSCILN;

typedef struct {
        OPDS    h;
        cs_float   *sr, *xamp, *xcps, *ifn, *iphs;
        int32   lphs;
        cs_double   phs;
        FUNC    *ftp;
        int32       tablen;
        cs_double      tablenUPsr;
        FUNC    FF;
        AUXCH   arraydata;
} OSC;

typedef struct  {
    OPDS        h;
    cs_float       *out, *amp, *freq, *kloop, *kend, *ift, *iphs;
    FUNC        *ftp;
    int32        tablen;
    cs_float       fsr;
    cs_double      phs, looplength;
} LPOSC;

