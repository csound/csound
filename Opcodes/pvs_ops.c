/*
    pvs_ops.c:

    Copyright (c) 2006 Istvan Varga, John ffitch

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

#include "pvs_ops.h"

/*
PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
    (void) csound;
    return 0;
}
*/

int32_t pvsopc_ModuleInit(CSOUND *csound)
{
    int32_t     err = 0;

    err |= ifd_init_(csound);
    err |= partials_init_(csound);
    err |= psynth_init_(csound);
    err |= pvsbasic_init_(csound);
    err |= pvscent_init_(csound);
    err |= pvsdemix_init_(csound);
    err |= pvsband_init_(csound);
    err |= pvsbuffer_localops_init_(csound);
    err |= pvsgendy_localops_init_(csound);

    return (err ? CSOUND_ERROR : CSOUND_SUCCESS);
}

#ifdef BUILD_PLUGINS

PUBLIC int32_t csoundModuleCreate(CSOUND *csound) {  
        return 0;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound) {
  return pvsopc_ModuleInit(csound);
}

PUBLIC int32_t csoundModuleInfo(void)
{
    return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t
) sizeof(MYFLT));
}

#endif


