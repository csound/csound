/*
    pvs_ops.h:

    Copyright (c) 2003 Istvan Varga, John ffitch

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

#ifndef CSOUND_PVS_OPS_H
#define CSOUND_PVS_OPS_H

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "interlocks.h"


extern int32_t ifd_init_(CSOUND *);
extern int32_t partials_init_(CSOUND *);
extern int32_t psynth_init_(CSOUND *);
extern int32_t pvsbasic_init_(CSOUND *);
extern int32_t pvscent_init_(CSOUND *);
extern int32_t pvsdemix_init_(CSOUND *);
extern int32_t pvsband_init_(CSOUND *);
extern int32_t pvsbuffer_localops_init_(CSOUND *);
extern int32_t pvsgendy_localops_init_(CSOUND *);

#endif  /* CSOUND_PVS_OPS_H */

