/*
    cwindow.h:

    Copyright (C) 1990 Dan Ellis

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

#ifndef CWINDOW_H
#define CWINDOW_H

/*******************************************************\
*       cwindow.h                                       *
*       portable window graphs stolen from Csound       *
*       necessary header declarations                   *
*       08nov90 dpwe                                    *
\*******************************************************/

#include "csound.h"

#define CAPSIZE  60

struct windat_ {
    uintptr_t windid;           /* set by MakeGraph() */
    MYFLT   *fdata;             /* data passed to DrawGraph */
    int32   npts;               /* size of above array */
    char    caption[CAPSIZE];   /* caption string for graph */
    int16   waitflg;            /* set =1 to wait for ms after Draw */
    int16   polarity;           /* controls positioning of X axis */
    MYFLT   max, min;           /* workspace .. extrema this frame */
    MYFLT   absmax;             /* workspace .. largest of above */
    MYFLT   oabsmax;            /* Y axis scaling factor */
    int     danflag;            /* set to 1 for extra Yaxis mid span */
    int     absflag;            /* set to 1 to skip abs check */
};

enum {                  /* symbols for WINDAT.polarity field */
    NOPOL,
    NEGPOL,
    POSPOL,
    BIPOL
};

struct xyindat_ {       /* for 'joystick' input window */
    uintptr_t windid;   /* xwindow handle */
    int     m_x,m_y;    /* current crosshair pixel adr */
    MYFLT   x,y;        /* current proportions of fsd */
    int     down;
};

#endif  /*  CWINDOW_H */

