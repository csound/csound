/*
    pvs_ops.c:

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

#include "pvs_ops.h"

/*
PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    (void) csound;
    return 0;
}
*/

int pvsopc_ModuleInit(CSOUND *csound)
{
    int     err = 0;

    err |= ifd_init_(csound);
    err |= partials_init_(csound);
    err |= psynth_init_(csound);
    err |= pvsbasic_init_(csound);
    err |= pvscent_init_(csound);
    err |= pvsdemix_init_(csound);
    err |= pvsband_init_(csound);

    return (err ? CSOUND_ERROR : CSOUND_SUCCESS);
}

/*
PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}
*/

