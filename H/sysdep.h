/*  
    sysdep.h:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

#ifdef __STDC__                                              /* SYSDEP.H */
#  include <stdlib.h>
#  include <stdio.h>
#endif

#include "config.h"

                                /* Experiment with doubles or floats */
#ifndef __FL_DEF
#ifndef USE_DOUBLE
# define MYFLT float
# define FL(x) x##f
#else
# define MYFLT double
# define FL(x) x
#endif
#define __FL_DEF
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(macintosh)
# define mac_classic /* All Mac Compiles Before OSX, including Carbon */
# define mills_macintosh /* DEFINE THIS to COMPILE the Mills"Perf"Version */
# include <stdlib.h>
# include <stat.h>
# define  SFDIGDES
# define  WINDOWS
# define  RTAUDIO
/*# define  SFIRCAM*/
# define  u_char  unsigned char
# define  u_short unsigned short
# define  u_int   unsigned int
# define  u_long  unsigned long
# define  O_NDELAY (0)
# ifdef mills_macintosh
#  define exit(x) die("");
# endif
# define DIRSEP ':'
#elif defined(SYMANTEC)   
# include <stdlib.h>
# include <unix.h>       /* for open() etc protos on mac */
# define  SFDIGDES
# define  WINDOWS       /* with winmac.c */
# define  u_char  unsigned char
# define  u_short unsigned short
# define  u_int   unsigned int
# define  u_long  unsigned long
# define DIRSEP ':'
extern  off_t lseek(int, off_t, int);
#else
#  define DIRSEP '/'
#  ifdef LATTICE
#  define  u_char  unsigned char
#  define  u_short unsigned short
#  define  u_int   unsigned int
#  define  u_long  unsigned long
#  include <sys/types.h>
#  else
#     ifdef __WATCOMC__
#      define  u_char  unsigned char
#      define  u_short unsigned short
#      define  u_int   unsigned int
#      define  u_long  unsigned long
#      define  O_NDELAY (0)
#      include <io.h>
#     else
#     ifdef WIN32
#      define  u_char  unsigned char
#      define  u_short unsigned short
#      define  u_int   unsigned int
#      define  u_long  unsigned long
#      define  O_NDELAY (0)
#      include <io.h>
#     else
#      ifdef DOSGCC
#       define O_NDELAY (0)
#      endif
#      include <sys/types.h>
#     endif
/*  RWD for WIN32 on VC++ */
#         ifndef _MSC_VER
#     include <sys/file.h>
#         endif
#    endif
#   endif
# include <sys/stat.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#elif HAVE_STRINGS_H
# include <strings.h>
#endif

#ifdef __BEOS__
# if !defined(O_NDELAY) && !defined(CSSVINTERFACE_H)
#  error "Please use Makefile.be to build the BeOS version of Csound."
# endif
#endif
