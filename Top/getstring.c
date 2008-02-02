/*
    getstring.c:

    Copyright (C) 1999 John ffitch
    Jan 27 2005: replaced with new implementation by Istvan Varga
    Dec 25 2007: added GNU gettext implementation as alternative -- John ffitch

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

#include "csoundCore.h"

#ifdef HAVE_DIRENT_H
#  include <sys/types.h>
#  include <dirent.h>
#  if 0 && defined(__MACH__)
typedef void* DIR;
DIR opendir(const char *);
struct dirent *readdir(DIR*);
int closedir(DIR*);
#  endif
#endif

#include "namedins.h"

#define CSSTRNGS_VERSION 0x2000
#include <locale.h>
#ifdef NOGETTEXT
void init_getstring(void)
{
}
PUBLIC char *csoundLocalizeString(const char *s)
{
    return (char*)s;
}
#else
void init_getstring(void)
{
    const char  *s;

    s = csoundGetEnv(NULL, "CS_LANG");
    if (s == NULL)              /* Default locale */
      setlocale (LC_MESSAGES, "");
    else 
      setlocale (LC_MESSAGES, s);    /* Set to particular value */
    textdomain("csound5");
    /* bind_textdomain_codeset("csound5", "UTF-8"); */
#ifdef never
    /* This is experimental; where should these be?? */
    bindtextdomain("csound5", "/home/jpff/Sourceforge/csound5/po");
#endif
}

PUBLIC char *csoundLocalizeString(const char *s)
{
    return gettext(s);
}
#endif

/* This stub is needed for backwards compatibility */
PUBLIC void csoundSetLanguage(cslanguage_t lang_code)
{
    return;
}
