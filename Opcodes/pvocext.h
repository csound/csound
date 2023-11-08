/*
    pvocext.h:

    Copyright (C) 1998 Richard Karpen

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

/* Spectral Extraction and Amplitude Gating functions */
/* By Richard Karpen  June, 1998  */
/* Based on ideas from Tom Erbe's SoundHack  */

/* Predeclare Functions */
#pragma once


#include <stdint.h>

#include "csoundCore.h"
#include "sysdep.h"

void    SpectralExtract(float *, float *, int32_t, int32, int32_t, MYFLT);
MYFLT   PvocMaxAmp(float *, int32, int32);
void    PvAmpGate(MYFLT *, int32, FUNC *, MYFLT);

