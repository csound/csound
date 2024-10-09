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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "std_util.h"

/* Modified from BSD sources for strlcpy */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
/* modifed for speed -- JPff */
char *
strNcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit or until NULL */
    if (n != 0) {
      while (--n != 0) {
        if ((*d++ = *s++) == '\0')
          break;
      }
    }

    /* Not enough room in dst, add NUL */
    if (n == 0) {
      if (siz != 0)
        *d = '\0';                /* NUL-terminate dst */

      //while (*s++) ;
    }
    return dst;        /* count does not include NUL */
}

/* module interface */

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
    int32_t   err = 0;

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

PUBLIC int32_t csoundModuleInfo(void)
{
    return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t) sizeof(MYFLT));
}

