/*
    repluck.h:

    Copyright (C) 1996, 1998 John ffitch, Victor Lazzarini

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

                                                        /* repluck.h */
#pragma once

typedef struct _DelayLine {
    MYFLT   *data;
    int32_t length;
    MYFLT   *pointer;
    MYFLT   *end;
} DelayLine;

typedef struct  {
    OPDS    h;
    MYFLT   *ar, *plk, *xamp, *icps, *pickup, *reflect;
    MYFLT   *ain;
    AUXCH   upper;
    AUXCH   lower;
    AUXCH   up_data;
    AUXCH   down_data;
    MYFLT   state;
    int32_t     scale;
    int32_t     rail_len;
} WGPLUCK2;

/****************************************************/
/* streson.h : string resonator header file         */
/*                                                  */
/*           Victor Lazzarini, 1998                 */
/****************************************************/
typedef struct{
        OPDS h;
        MYFLT   *result, *ainput, *afr, *ifdbgain;
        double   LPdelay, APdelay;
        MYFLT   *Cdelay;
        AUXCH   aux;
        int32_t     wpointer, rpointer, size;
} STRES;
