#ifndef PITCH_H
#define PITCH_H
/*
    pitch.h:

    Copyright (C) 1999 John ffitch, Istvan Varga, Peter Neubäcker,
                       rasmus ekman, Phil Burk

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

                        /*                                      PITCH.H */

#define MAXPTL 10
typedef struct {
        OPDS    h;
        cs_float   *ans;
        cs_float   *pnum;
} PFUN;

typedef struct {
        OPDS    h;
        cs_float   *ans;
        cs_float   *pnum;
        AUXCH   pfield;
} PFUNK;

typedef struct {
        OPDS    h;
        cs_float   *ins;
        cs_float   *onoff;
} MUTE;

typedef struct {
        OPDS    h;
        cs_float   *cnt;
        cs_float   *ins;
        cs_float   *opt;
        cs_float   *norel;
} INSTCNT;

typedef struct {
    OPDS        h;
    cs_float       *instrnum, *ipercent, *iopc;    /* IV - Oct 31 2002 */
} CPU_PERC;

typedef struct {
    OPDS        h;
    cs_float       *instrnum, *icount, *iturnoff_mode;
} CPU_MAXALLOC;

/*

*/


#endif /* PITCH_H */

