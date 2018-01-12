/*
    dumpf.h:

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

                                                        /*  DUMPF.H  */
typedef struct {
        OPDS   h;
        MYFLT  *ksig, *ifilcod, *iformat, *iprd;
        int    format;
        int32   countdown, timcount;
        FILE   *f;
        FDCH   fdch;
} KDUMP;

typedef struct {
        OPDS   h;
        MYFLT  *ksig1, *ksig2, *ifilcod, *iformat, *iprd;
        int    format;
        int32   countdown, timcount;
        FILE   *f;
        FDCH   fdch;
} KDUMP2;

typedef struct {
        OPDS   h;
        MYFLT  *ksig1, *ksig2, *ksig3, *ifilcod, *iformat, *iprd;
        int    format;
        int32   countdown, timcount;
        FILE   *f;
        FDCH   fdch;
} KDUMP3;

typedef struct {
        OPDS   h;
        MYFLT  *ksig1, *ksig2, *ksig3, *ksig4, *ifilcod, *iformat, *iprd;
        int    format;
        int32   countdown, timcount;
        FILE   *f;
        FDCH   fdch;
} KDUMP4;

typedef struct {
        OPDS   h;
        MYFLT  *k1, *ifilcod, *iformat, *iprd;
        /* MYFLT  *interp; */
        int    format;
        int32   countdown, timcount;
        MYFLT  k[4];
        FILE   *f;
        FDCH   fdch;
} KREAD;

typedef struct {
        OPDS   h;
        MYFLT  *k1, *k2, *ifilcod, *iformat, *iprd;
        /* MYFLT  *interp; */
        int    format;
        int32   countdown, timcount;
        MYFLT  k[4];
        FILE   *f;
        FDCH   fdch;
} KREAD2;

typedef struct {
        OPDS   h;
        MYFLT  *k1, *k2, *k3, *ifilcod, *iformat, *iprd;
        /* MYFLT  *interp; */
        int    format;
        int32   countdown, timcount;
        MYFLT  k[4];
        FILE   *f;
        FDCH   fdch;
} KREAD3;

typedef struct {
        OPDS   h;
        MYFLT  *k1, *k2, *k3, *k4, *ifilcod, *iformat, *iprd;
        /* MYFLT  *interp; */
        int    format;
        int32  countdown, timcount;
        MYFLT  k[4];
        FILE   *f;
        FDCH   fdch;
} KREAD4;

typedef struct {
        OPDS   h;
        STRINGDAT  *str;
        MYFLT *ifilcod, *iprd;
        int32  countdown, timcount;
        char   *lasts;
        FILE   *f;
        FDCH   fdch;
} KREADS;

