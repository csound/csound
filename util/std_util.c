/*
    std_util.c:

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

#include "std_util.h"

/* module interface */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    int   err = 0;

    err |= atsa_init_(csound);
    err |= envext_init_(csound);
    err |= het_export_init_(csound);
    err |= het_import_init_(csound);
    err |= lpc_export_init_(csound);
    err |= lpc_import_init_(csound);
    err |= pv_export_init_(csound);
    err |= pv_import_init_(csound);
    err |= xtrct_init_(csound);
    err |= cvanal_init_(csound);
    err |= dnoise_init_(csound);
    err |= hetro_init_(csound);
    err |= lpanal_init_(csound);
    err |= mixer_init_(csound);
    err |= pvanal_init_(csound);
    err |= pvlook_init_(csound);
    err |= scale_init_(csound);
    err |= sndinfo_init_(csound);
    err |= srconv_init_(csound);
    return err;
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}

