/*  
    version.h:

    Copyright (C) 1995 John ffitch

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
#if defined(HAVE_CONFIG_H)
#include "config.h"
#else
/* Define to the full name of this package. */
#define PACKAGE_NAME "Csound"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "Csound 5.0beta"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "csound"

/* Define to the version of this package. */
#define PACKAGE_VERSION "5.0beta"
#endif

# define APIVERSION 1  /* should be increased anytime a new version
			  contains changes that an older host will
                          not be able to handle -- most likely this
                          will be an change to an API function or
                          the GLOBALS struct */
# define APISUBVER 0   /* for minor changes that will still allow
                          compatiblity with older hosts */
