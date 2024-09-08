/*
    smain.c

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include "csound.h"                                    /*   SMAIN.C  */

#if defined(LINUX) || defined(SGI) || defined(sol) || \
    defined(__MACH__) || defined(__EMX__)
#include <signal.h>
#endif

static void msg_callback(CSOUND *csound,
                         int attr, const char *fmt, va_list args)
{
  (void) csound;
    if (attr & CSOUNDMSG_TYPE_MASK) {
      vfprintf(stderr, fmt, args);
    }
}

int main(void)                          /* stdio stub for standalone scsort */
{
    CSOUND *csound;
    int    err;

    csound = csoundCreate(NULL,NULL);
#if defined(LINUX) || defined(SGI) || defined(sol) || \
    defined(__MACH__) || defined(__EMX__)
    signal(SIGPIPE, SIG_DFL);
#endif
    csoundSetMessageCallback(csound, msg_callback);
    err = csoundScoreSort(csound, stdin, stdout);
    csoundDestroy(csound);

    return err;
}

