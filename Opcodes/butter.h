/*
    butter.h:

    Copyright (C) 1994 Paris Smaragdis, John ffitch

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/*              Butterworth filters coded by Paris Smaragdis 1994       */
/*              Berklee College of Music Csound development team        */
/*              Copyright (c) May 1994.  All rights reserved            */

typedef struct  {
        OPDS    h;
        MYFLT   *sr, *ain, *kfc, *istor;
        MYFLT   lkf;
        MYFLT   a[8];
} BFIL;

typedef struct  {
        OPDS    h;
        MYFLT   *sr, *ain, *kfo, *kbw, *istor;
        MYFLT   lkf, lkb;
        MYFLT   a[8];
} BBFIL;
