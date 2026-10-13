/*
    flanger.h:

    Copyright (C) 1998 Gabriel Maldonado, John ffitch

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

#pragma once

typedef struct {
        OPDS    h;
        cs_float   *ar, *asig, *xdel, *kfeedback, *maxd, *iskip;
        cs_float   yt1; /* filter instance variables */
        AUXCH   aux;  /* delay instance variables */
        uint32  left;
        uint32  maxdelay;
        cs_float   maxDelaySeconds;
} FLANGER;

typedef struct {
        OPDS    h;
        cs_float *ar, *asig, *xdel, *filt_khp, *kfeedback;
        cs_float c1, c2, yt1, prvhp; /* filter instance variables */
        AUXCH   aux;  /* delay instance variables */
        uint32  maxd;
        int32   left;
        int16   xdelcod;
} WGUIDE1;

typedef struct {
        OPDS    h;
        cs_float *ar, *asig, *xdel1, *xdel2, *filt_khp1;
        cs_float *filt_khp2, *kfeedback1, *kfeedback2;
        cs_float c1_1, c2_1, yt1_1, prvhp1; /* filter1 instance variables */
        cs_float c1_2, c2_2, yt1_2, prvhp2; /* filter1 instance variables */
        AUXCH   aux1;  /* delay1 instance variables */
        int32   left1;
        AUXCH   aux2;  /* delay1 instance variables */
        int32   left2;
        uint32  maxd;
        cs_float   old_out;
        int16   xdel1cod, xdel2cod;
} WGUIDE2;
