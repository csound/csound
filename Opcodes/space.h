/*
    space.h:

    Copyright (C) 1998 Richard Karpen

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

/******************************************/
/* The applications in this file were     */
/* designed and coded by Richard Karpen   */
/* University of Washington, Seattle 1998 */
/******************************************/

#include "stdopcod.h"

typedef struct {
    OPDS    h;
    MYFLT   *r1, *r2, *r3, *r4, *asig, *ifn, *time, *reverbamount, *kx, *ky;
    MYFLT   ch1, ch2, ch3, ch4;
    FUNC    *ftp;
    AUXCH   auxch;
    MYFLT   *rrev1, *rrev2, *rrev3, *rrev4;
} SPACE;

typedef struct {
    OPDS    h;
    MYFLT   *r1, *r2, *r3, *r4;
    SPACE   *space;
} SPSEND;

typedef struct {
    OPDS    h;
    MYFLT   *r, *ifn, *time, *kx, *ky;
    FUNC    *ftp;
} SPDIST;

