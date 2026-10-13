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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

                                                        /* repluck.h */
#pragma once

typedef struct _DelayLine {
    cs_float   *data;
    int32_t length;
    cs_float   *pointer;
    cs_float   *end;
} DelayLine;

typedef struct  {
    OPDS    h;
    cs_float   *ar, *plk, *xamp, *icps, *pickup, *reflect;
    cs_float   *ain;
    AUXCH   upper;
    AUXCH   lower;
    AUXCH   up_data;
    AUXCH   down_data;
    cs_float   state;
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
        cs_float   *result, *ainput, *afr, *ifdbgain;
        cs_double   LPdelay, APdelay;
        cs_float   *Cdelay;
        AUXCH   aux;
        int32_t     wpointer, rpointer, size;
} STRES;
