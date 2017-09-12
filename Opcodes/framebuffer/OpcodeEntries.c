/*

 OpcodeEntries.c
 Framebuffer

 Created by Edward Costello on 10/06/2015.
 Copyright (c) 2015 Edward Costello. All rights reserved.

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

#include "Framebuffer.h"
#include "OLABuffer.h"

static OENTRY localops[] = {

    {
        .opname = "framebuffer",
        .dsblksiz = sizeof(Framebuffer),
        .thread = 3,
        .outypes = "*",
        .intypes = "*",
        .iopadr = (SUBR)Framebuffer_initialise,
        .kopadr = (SUBR)Framebuffer_process,
        .aopadr = NULL
    },
    {
        .opname = "olabuffer",
        .dsblksiz = sizeof(OLABuffer),
        .thread = 3,
        .outypes = "a",
        .intypes = "k[]i",
        .iopadr = (SUBR)OLABuffer_initialise,
        .kopadr = (SUBR)OLABuffer_process,
        .aopadr = NULL
    }
};


LINKAGE
