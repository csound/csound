/*
    filopen.c:

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

#include "cs.h"                                        /*  FILOPEN.C    */
#include <ctype.h>

#ifdef mills_macintosh
#include <unix.h>
#endif

void sssfinit(void)
{
    csoundInitEnv(&cenviron);
}

char *unquote(char *name)       /* remove any quotes from a filename   */
                                /* also for THINKC rm ./ & cvt / to :  */
{
    static char newname[MAXNAME];
    char c, *old = name, *nnew = newname;
    do {
      if ((c = *old++) != '"') {
#if defined(mac_classic) || defined(SYMANTEC)
        if (nnew == newname && (c == '.' || c == '/'))
          continue;
        if (c == '/')  c = DIRSEP;
#endif
        *nnew++ = c;
      }
    } while (c);
    return (newname);
}

#if defined MSVC
#define RD_OPTS  O_RDONLY | O_BINARY
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY,_S_IWRITE
#elif defined(mac_classic) || defined(SYMANTEC) || defined(WIN32)
#define RD_OPTS  O_RDONLY | O_BINARY
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, 0644
#elif defined DOSGCC
#define RD_OPTS  O_RDONLY | O_BINARY, 0
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, 0644
#else
#ifndef O_BINARY
# define O_BINARY (0)
#endif
#define RD_OPTS  O_RDONLY | O_BINARY, 0
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, 0644
#endif

int
openin(char *filnam)/* open a file for reading. If not fullpath, will search: */
                   /*  current directory, then SSDIR (if defined), then SFDIR */
{                  /*  returns normal fd, also sets a global return filename  */
                   /*  called by sndgetset (for soundin, gen01), and sfopenin */
    char *pathnam;
    int  infd;

    pathnam = csoundFindInputFile(&cenviron, filnam, "SFDIR;SSDIR");
    infd = -1;
    if (retfilnam != NULL)
      mfree(&cenviron, retfilnam);
    retfilnam = pathnam;
    if (pathnam != NULL)
      infd = open(pathnam, RD_OPTS);
    if (infd < 0)
      csoundDie(&cenviron, Str("cannot open %s.  "
                               "Not in cur dir, SSDIR or SFDIR as defined"),
                           filnam);
    return infd;
}

int openout(              /* open a file for writing.  If not fullpath, then  */
            char *filnam, /*   dirtyp 1 will put it in the current directory  */
            int  dirtyp)  /*   dirtyp 2 will put it in SFDIR                  */
{                         /*   dirtyp 3 will put it in SFDIR else in cur dir  */
                          /* returns normal fd, & sets global return filename */
                          /* called by anals,dumpf (typ 1), sfopenout (typ 3) */
    char *pathnam = NULL;
    int  outfd = -1;

    if (dirtyp == 2) {
      pathnam = csoundGetEnv(&cenviron, "SFDIR");
      if (pathnam == NULL || pathnam[0] == '\0')
        csoundDie(&cenviron, Str("cannot open %s, SFDIR undefined"), filnam);
    }
    else if (dirtyp < 1 || dirtyp > 3)
      csoundDie(&cenviron, Str("openout: illegal dirtyp"));
    if (dirtyp == 2 || dirtyp == 3)
      pathnam = csoundFindOutputFile(&cenviron, filnam, "SFDIR");
    else
      pathnam = csoundFindOutputFile(&cenviron, filnam, NULL);
    if (retfilnam != NULL)
      mfree(&cenviron, retfilnam);
    retfilnam = pathnam;
    if (pathnam != NULL)
      outfd = open(pathnam, WR_OPTS);
    if (outfd < 0)
      csoundDie(&cenviron, Str("cannot open %s."), filnam);
    return outfd;
}

/* fopenin() - patches fopen calls, searching file in current dir, INCDIR,
   SSDIR or SFDIR, in that order. Modelled on openin() above. (re May 2000) */

FILE *fopenin(char *filnam)
{
    char *pathnam;
    FILE *infil;

    pathnam = csoundFindInputFile(&cenviron, filnam, "SFDIR;SSDIR;INCDIR");
    infil = NULL;
    if (retfilnam != NULL)
      mfree(&cenviron, retfilnam);
    retfilnam = pathnam;
    if (pathnam != NULL)
      infil = fopen(pathnam, "r");
    if (infil == NULL)
      csoundDie(&cenviron, Str("cannot open %s.  Not in cur dir, "
                               "INCDIR SSDIR or SFDIR as defined"), filnam);
    return infil;
}

