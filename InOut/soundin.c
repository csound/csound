/*  
    soundin.c:

    Copyright (C) 1991, 2000 Barry Vercoe, John ffitch, Richard Dobson

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

#include "cs.h"                 /*                      SOUNDIN.C       */
#include "soundio.h"
#include "oload.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/*
 * $Id$
 */

/*RWD 3:2000*/
/* quick hack to save typing etc... */
#undef INLONGFAC
#define INLONGFAC  (double)long_to_dbfs
/* #define INLONGFAC (1.0 / 65536.0)  /\* convert 32bit long to quasi 16 bit range *\/ */
/* #define INMYFLTFAC (FL(32767.0)) */


/* RWD 5:2001 version with 24bit support. defs in soundio.h  */

static  MYFLT   fzero = FL(0.0);
static  long    datpos= 0L;       /* Used in resetting only */

extern  HEADATA *readheader(int, char *, SOUNDIN*);
#ifdef ULAW
extern  short   ulaw_decode[];
#endif
extern  int     bytrevhost(void), openin(char*);
extern  OPARMS  O;
extern  void sndwrterr(int, int);

void soundinRESET(void)
{
    datpos = 0;
}


/* RWD 5:2001 */
void bytrev3 (char *buf,int nbytes)
{
    char temp, *p = buf;
    /* only need to swap outer bytes */
    int samps = nbytes/3;

    do {
      temp = p[2];
      p[2] = p[0];
      p[0] = temp;
      p += 3;
    } while (--samps);
}


