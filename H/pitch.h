#ifndef PITCH_H
#define PITCH_H
/*
    pitch.h:

    Copyright (C) 1999 John ffitch, Istvan Varga, Peter Neub�cker,
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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

                        /*                                      PITCH.H */
//#include "spectra.h"
//#include "uggab.h"
#define MAXPTL 10
typedef struct {
        OPDS    h;
        MYFLT   *ans;
        MYFLT   *pnum;
} PFUN;

typedef struct {
        OPDS    h;
        MYFLT   *ans;
        MYFLT   *pnum;
        AUXCH   pfield;
} PFUNK;

typedef struct {
        OPDS    h;
        MYFLT   *ins;
        MYFLT   *onoff;
} MUTE;

typedef struct {
        OPDS    h;
        MYFLT   *cnt;
        MYFLT   *ins;
        MYFLT   *opt;
        MYFLT   *norel;
} INSTCNT;

typedef struct {
    OPDS        h;
    MYFLT       *instrnum, *ipercent, *iopc;    /* IV - Oct 31 2002 */
} CPU_PERC;

/*

*/


#endif /* PITCH_H */

