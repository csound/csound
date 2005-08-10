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

#ifndef CSOUND_VERSION_H
#define CSOUND_VERSION_H

/* Define to the full name of this package. */
#define CS_PACKAGE_NAME     "Csound"

/* Define to the full name and version of this package. */
#define CS_PACKAGE_STRING   "Csound 5.00.0"

/* Define to the one symbol short name of this package. */
#define CS_PACKAGE_TARNAME  "csound"

/* Define to the version of this package. */
#define CS_PACKAGE_VERSION  "5.00.0"
#define CS_VERSION          (5)
#define CS_SUBVER           (0)
#define CS_PATCHLEVEL       (0)

#define CS_APIVERSION       1   /* should be increased anytime a new version
                                   contains changes that an older host will
                                   not be able to handle -- most likely this
                                   will be a change to an API function or
                                   the ENVIRON struct */
#define CS_APISUBVER        0   /* for minor changes that will still allow
                                   compatiblity with older hosts */

#endif /* CSOUND_VERSION_H */

