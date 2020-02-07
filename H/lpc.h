/*
    lpc.h:

    Copyright (C) 1992 Barry Vercoe

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

/*                                                                      LPC.H   */

#define LP_MAGIC    999
#define LP_MAGIC2   2399           /* pole file type */
#define LPBUFSIZ    4096           /* in lpanal */
#define MAXWINDIN   1000           /* was for 10ms hops at 50 KC */
#define MAXPOLES    5000
#define NDATA       4   /* number of data values stored with frame */

typedef struct {
        uint32_t headersize, lpmagic, npoles, nvals;
        MYFLT   framrate, srate, duration;
        char    text[4];
} LPHEADER;
