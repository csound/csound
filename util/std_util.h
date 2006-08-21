/*
    std_util.h:

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

#ifndef CSOUND_STD_UTIL_H
#define CSOUND_STD_UTIL_H

#include "csdl.h"

extern int atsa_init_(CSOUND *);
extern int cvanal_init_(CSOUND *);
extern int dnoise_init_(CSOUND *);
extern int envext_init_(CSOUND *);
extern int het_export_init_(CSOUND *);
extern int het_import_init_(CSOUND *);
extern int hetro_init_(CSOUND *);
extern int lpanal_init_(CSOUND *);
extern int lpc_export_init_(CSOUND *);
extern int lpc_import_init_(CSOUND *);
extern int mixer_init_(CSOUND *);
extern int pvanal_init_(CSOUND *);
extern int pvlook_init_(CSOUND *);
extern int pv_export_init_(CSOUND *);
extern int pv_import_init_(CSOUND *);
extern int scale_init_(CSOUND *);
extern int sndinfo_init_(CSOUND *);
extern int srconv_init_(CSOUND *);
extern int xtrct_init_(CSOUND *);

#endif  /* CSOUND_STD_UTIL_H */

