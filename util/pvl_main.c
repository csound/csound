/*
    pvl_main.c:

    Copyright (C) 2006 Istvan Varga

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

#include "csound.h"
#include <stdarg.h>

static void messageCallback_(CSOUND *csound, int32_t attr,
                             const char *fmt, va_list args)
{
    (void) csound;
    switch (attr & CSOUNDMSG_TYPE_MASK) {
    case CSOUNDMSG_ORCH:
      vfprintf(stdout, fmt, args);
      break;
    default:
      vfprintf(stderr, fmt, args);
      break;
    }
}

int32_t main(int32_t argc, char **argv)
{
    CSOUND  *csound;
    int32_t     n = -1;

    if ((csound = csoundCreate(NULL,NULL)) != NULL) {
      csoundSetMessageCallback(csound, messageCallback_);
      n = csoundRunUtility(csound, "pvlook", argc, argv);
      csoundDestroy(csound);
    }
    return (n == CSOUND_EXITJMP_SUCCESS ? 0 : n);
}

