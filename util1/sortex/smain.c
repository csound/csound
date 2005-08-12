/*
    smain.c

    Copyright (C) 1991 Barry Vercoe, John ffitch

    This file is part of Csound.

    Csound is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "csound.h"                                    /*   SMAIN.C  */

PUBLIC void scsort(CSOUND *, FILE *, FILE *);

static void msg_callback(CSOUND *csound,
                         int attr, const char *fmt, va_list args)
{
    if (attr & CSOUNDMSG_TYPE_MASK) {
      vfprintf(stderr, fmt, args);
    }
}

int main(void)                           /* stdio stub for standalone scsort */
{
    CSOUND *csound;

    csound = csoundCreate(NULL);
    csoundSetMessageCallback(csound, msg_callback);
    csoundPreCompile(csound);
    scsort(csound, stdin, stdout);
    return 0;
}

