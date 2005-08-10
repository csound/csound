/*
    utilmain.h:

    Copyright (C) 2005 Istvan Varga

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

#ifndef CSOUND_UTILMAIN_H
#define CSOUND_UTILMAIN_H

/* main function for utility frontends */

/* FIXME: temporary hack to make this file compile */
#include "csdl.h"

static int csoundUtilMain(const char *name, int argc, char **argv)
{
    ENVIRON *csound;
    int     n;

    if ((csound = (ENVIRON*) csoundCreate(NULL)) == NULL)
      return -1;
    if ((n = csoundPreCompile(csound)) == 0) {
      csound->scorename = csound->orchname = (char*) name;
      n = csound->Utility(csound, name, argc, argv);
    }
    csoundDestroy(csound);

    return (n == CSOUND_EXITJMP_SUCCESS ? 0 : n);
}

#define UTIL_MAIN(x)                            \
                                                \
int main(int argc, char **argv)                 \
{                                               \
    return (csoundUtilMain(x, argc, argv));     \
}

#endif      /* CSOUND_UTILMAIN_H */

