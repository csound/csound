/*
    midiops2.h:

    Copyright (C) 1997 Gabriel Maldonado

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

/****************************************/
/** midicXX   UGs by Gabriel Maldonado **/
/****************************************/
#ifndef MIDIOPS2_H
#define MIDIOPS2_H

typedef struct {
    OPDS   h;
    MYFLT  *r, *ictlno, *imin, *imax, *ifn;
    int16 flag;
    FUNC *ftp;
    int32   ctlno;
} MIDICTL2;

typedef struct {
    OPDS   h;
    MYFLT  *r, *ictlno1, *ictlno2, *imin, *imax, *ifn;
    int16 flag;
    FUNC *ftp;
    int32   ctlno1, ctlno2;
} MIDICTL3;

typedef struct {
    OPDS   h;
    MYFLT  *r, *ictlno1, *ictlno2, *ictlno3, *imin, *imax, *ifn;
    int16 flag;
    FUNC *ftp;
    int32   ctlno1, ctlno2, ctlno3;
} MIDICTL4;

/*----------------------------------------*/
/* GLOBAL MIDI IN CONTROLS activable by score-oriented instruments*/

typedef struct {
    OPDS   h;
    MYFLT  *r, *ichan, *ictlno, *imin, *imax, *ifn;
    int16 flag;
    FUNC *ftp;
    int32   ctlno;
} CTRL7;

typedef struct {
    OPDS   h;
    MYFLT  *r, *ichan, *ictlno1, *ictlno2, *imin, *imax, *ifn;
    int16 flag;
    FUNC *ftp;
    int32   ctlno1, ctlno2;
} CTRL14;

typedef struct {
    OPDS   h;
    MYFLT  *r, *ichan, *ictlno1, *ictlno2, *ictlno3, *imin, *imax, *ifn;
    int16 flag;
    FUNC *ftp;
    int32  ctlno1, ctlno2, ctlno3;
} CTRL21;

typedef struct {
    OPDS   h;
    MYFLT  *ichan, *ictlno, *ivalue;
} INITC7;

typedef struct {
    OPDS   h;
    MYFLT  *ichan, *ictlno1, *ictlno2, *ivalue;
} INITC14;

typedef struct {
    OPDS   h;
    MYFLT  *ichan, *ictlno1, *ictlno2, *ictlno3, *ivalue;
} INITC21;

#endif

