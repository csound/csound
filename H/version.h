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

# define VERSION (4)
# define SUBVER  (24)
#ifdef BETA
# define VERSIONSTRING  " v4.24beta"
# define PVERSION "\p                                               Csound 4.24betsa"
#else
# define VERSIONSTRING  " v4.24"
# define PVERSION "\p                                               Csound 4.24"
#endif

# define APIVERSION 1  /* should be increased anytime a new version
			  contains changes that an older host will
                          not be able to handle -- most likely this
                          will be an change to an API function or
                          the GLOBALS struct */
# define APISUBVER 0   /* for minor changes that will still allow
                          compatiblity with older hosts */
