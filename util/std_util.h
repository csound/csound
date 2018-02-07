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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef CSOUND_STD_UTIL_H
#define CSOUND_STD_UTIL_H

#include "csdl.h"
#include <inttypes.h>

extern int32_t atsa_init_(CSOUND *);
extern int32_t cvanal_init_(CSOUND *);
extern int32_t dnoise_init_(CSOUND *);
extern int32_t envext_init_(CSOUND *);
extern int32_t het_export_init_(CSOUND *);
extern int32_t het_import_init_(CSOUND *);
extern int32_t hetro_init_(CSOUND *);
extern int32_t lpanal_init_(CSOUND *);
extern int32_t lpc_export_init_(CSOUND *);
extern int32_t lpc_import_init_(CSOUND *);
extern int32_t mixer_init_(CSOUND *);
extern int32_t pvanal_init_(CSOUND *);
extern int32_t pvlook_init_(CSOUND *);
extern int32_t pv_export_init_(CSOUND *);
extern int32_t pv_import_init_(CSOUND *);
extern int32_t scale_init_(CSOUND *);
extern int32_t sndinfo_init_(CSOUND *);
extern int32_t srconv_init_(CSOUND *);
extern int32_t xtrct_init_(CSOUND *);

#endif  /* CSOUND_STD_UTIL_H */

