/*
    follow.h:

    Copyright (C) 1994, 1999 Paris Smaragdis, John ffitch

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

/*              Envelope follower by Paris Smaragdis                    */
/*              Berklee College of Music Csound development team        */
/*              Copyright (c) August 1994.  All rights reserved         */

#pragma once

typedef struct  {
        OPDS            h;
        MYFLT           *out, *in, *len;
        MYFLT           max, wgh;
        int32           length;
        int32           count;
} FOL;

/* For implementation of Jot envelope follower -- JPff Feb 2000 */
typedef struct {
        OPDS    h;
        MYFLT   *out, *in, *attack, *release;
        MYFLT   lastatt, lastrel, envelope, ga, gr;
} ENV;
