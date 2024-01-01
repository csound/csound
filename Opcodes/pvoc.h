/*
    pvoc.h:

    Copyright (c) 2005 Istvan Varga

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

#ifndef CSOUND_PVOC_H
#define CSOUND_PVOC_H

#include <stddef.h>        // for NULL

#include "csound.h"        // for CSOUND
// #include "csdl.h"
#include "csoundCore.h"    // for CSOUND_
#include "pvinterp.h"      // for PVBUFREAD
#include "pvoc_forward.h"  // for PVOC_GLOBALS
#include "sysdep.h"        // for MYFLT
#include "vpvoc.h"         // for TABLESEG

struct PVOC_GLOBALS_ {
    CSOUND    *csound;
    MYFLT     *dsputil_sncTab;
    PVBUFREAD *pvbufreadaddr;
    TABLESEG  *tbladr;
};

extern PVOC_GLOBALS *PVOC_AllocGlobals(CSOUND *csound);

static inline PVOC_GLOBALS *PVOC_GetGlobals(CSOUND *csound)
{
    PVOC_GLOBALS  *p;

    p = (PVOC_GLOBALS*) csound->QueryGlobalVariable(csound, "pvocGlobals");
    if (p == NULL)
      return PVOC_AllocGlobals(csound);
    return p;
}

#endif  /* CSOUND_PVOC_H */
