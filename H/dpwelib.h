/*  
    dpwelib.h:

    Copyright (C) 1991 Dan Ellis

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


/*******************************************************\
*  dpwelib.h                                            *
*  little header to try and setup STDC for my lib       *
*  dpwe 28may91                                         *
\*******************************************************/

/* WHAT'S HERE:  a bunch of stuff typically needed for my code
                 - but you might not need it all
 Includes (& what they are needed for
   * <stdio.h>  (printf, FILE, stderr, fopen, (size_t))
   * <stdlib.h> or equivalent (malloc, atof, labs, NULL, size_t)
   * <fcntl.h>  (O_RDONLY etc)
   * <string.h> (strcmp, strrchr)
 Defines
   * READMODE, WRITEMODE for fopen of binary files
   * MYFLTARG for type of floats in prototypes
 Macros
   * MIN, MAX (arguments evaluated twice)
 */

#ifndef _DPWELIB_H_
#define _DPWELIB_H_

#include <stdio.h>

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef SYMANTEC
#include <stdlib.h>             /* for malloc prototype */
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#define hypot(a,b)      sqrt(pow(a,2.0)+pow(b,2.0))
#define READMODE "rb"
#define WRITEMODE "wb+"

#else  /* Unix, not mac (SYMANTEC) */
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#define READMODE "rb"
#define WRITEMODE "wb+"

#ifdef NeXT
#include <stdlib.h>

#else  /* ultrix, not NeXT */
# if defined(LATTICE) || defined(WIN32) || defined(SGI) || defined(__FreeBSD__)
#  include <stdlib.h>
# endif
#ifdef HAVE_MALLOC_H
#        include <malloc.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#endif /* NeXT or ultrix */

#endif /* mac or Unix */

#endif /* _DPWELIB_H_ */
