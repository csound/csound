/*  
    revsets.h:

    Copyright (C) 1994 Paris Smaragdis

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

/*      reverb2 for Csound coded by Paris Smaragdis             */
/*      Berklee College of Music Csound development team        */
/*      Copyright (c) December 1994.  All rights reserved       */


MYFLT gc_time[Combs] = {
        FL(1237.0),
        FL(1381.0),
        FL(1607.0),
        FL(1777.0),
        FL(1949.0),
        FL(2063.0)
};

MYFLT gc_gain[Combs] = {
        FL(0.822),
        FL(0.802),
        FL(0.773),
        FL(0.753),
        FL(0.753),
        FL(0.753)
};

MYFLT ga_time[Alpas] = {
        FL(307.0),
        FL(97.0),
        FL(71.0),
        FL(53.0),
        FL(37.0)
};

MYFLT ga_gain[Alpas] = {
        FL(0.7),
        FL(0.7),
        FL(0.7),
        FL(0.7),
        FL(0.7),
};
