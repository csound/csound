/*
    sort.h:

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

#include <stdio.h>                                 /*      SORT.H */
#ifndef MYFLT
#include "sysdep.h"
#endif
#define SP ' '
#define LF '\n'

typedef struct srtblk {
        struct srtblk *nxtblk;
        struct srtblk *prvblk;
        int16   insno;
        int16   pcnt;
        MYFLT   p1val;
        MYFLT   p2val;
        MYFLT   p3val;
        MYFLT   newp2;
        MYFLT   newp3;
        int16   lineno;
        char    preced;
        char    text[9];
} SRTBLK;

