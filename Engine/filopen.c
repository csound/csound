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

/* char    *sadirpath;      actual filename returned  */
static  char    *incdirpath;

void sssfinit(void)
{
    csoundInitEnv(&cenviron);
    ssdirpath = csoundGetEnv(&cenviron, "SSDIR");
    sfdirpath = csoundGetEnv(&cenviron, "SFDIR");
    sadirpath = csoundGetEnv(&cenviron, "SADIR");
    incdirpath = csoundGetEnv(&cenviron, "INCDIR");
}

int isfullpath(char *name)
{
    if
#if defined(mac_classic) || defined(SYMANTEC)
       (strchr(name, DIRSEP) != NULL)      /* if name already a pathname */
#else
       (
# if defined(DOSGCC) || defined(__WATCOMC__) || defined(WIN32)
     (isalpha(*name) && *(name+1)==':') ||
# endif
     *name == DIRSEP || *name == '.')
#endif
    return (1);                     /*   return yes  */
    else return (0);
}

char *catpath(char *path, char *name) /*  build a fullpath filename     */
{
    static char fullname[MAXNAME];
    sprintf(fullname,"%s%c%s",path,DIRSEP,name);
    return(fullname);
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
    char *pathnam = filnam;
    int  infd;

    if (isfullpath(filnam))
      infd = open(filnam, RD_OPTS);
    else {
      if ((infd = open(filnam, RD_OPTS)) >= 0)
        goto done;
      if (ssdirpath != NULL) {
        pathnam = catpath(ssdirpath, filnam);
        if ((infd = open(pathnam, RD_OPTS)) >= 0)
          goto done;
      }
      if (sfdirpath != NULL) {
        pathnam = catpath(sfdirpath, filnam);
        if ((infd = open(pathnam, RD_OPTS)) >= 0)
          goto done;
      }
      dies(Str("cannot open %s.  Not in cur dir, SSDIR or SFDIR as defined"),
           filnam);
    }
 done:
    retfilnam = pathnam;
    return(infd);
}

int openout(              /* open a file for writing.  If not fullpath, then  */
            char *filnam, /*   dirtyp 1 will put it in the current directory  */
            int  dirtyp)  /*   dirtyp 2 will put it in SFDIR                  */
{                         /*   dirtyp 3 will put it in SFDIR else in cur dir  */
                          /* returns normal fd, & sets global return filename */
                          /* called by anals,dumpf (typ 1), sfopenout (typ 3) */
    char *pathnam = filnam;
    int  outfd = -1;

    if (isfullpath(filnam))
      outfd = open(filnam, WR_OPTS);
    else switch (dirtyp) {
    case 3:
    case 2:
      if (sfdirpath != NULL) {
        pathnam = catpath(sfdirpath, filnam);
        outfd = open(pathnam, WR_OPTS);
        break;
      }
      else if (dirtyp == 2)
        dies(Str("cannot open %s, SFDIR undefined"), filnam);
      else
        printf(Str("SFDIR undefined.  using current directory\n"));
    case 1:
      outfd = open(filnam, WR_OPTS);
      break;
    default: die(Str("openout: illegal dirtyp"));
    }
    retfilnam = pathnam;
    return (outfd);
}

#ifdef mills_macintosh
int openoutforin(       /* open a file for writing.  If not fullpath, then  */
  char *filnam,         /*   dirtyp 1 will put it in the current directory  */
  int  dirtyp)          /*   dirtyp 2 will put it in SFDIR                  */
{                       /*   dirtyp 3 will put it in SFDIR else in cur dir  */
                        /* returns normal fd, & sets global return filename */
                        /* called by anals,dumpf (typ 1), sfopenout (typ 3) */
    char *pathnam = filnam;
    int  outfd;

    if (isfullpath(filnam))
      outfd = open(filnam, RD_OPTS);
    else switch (dirtyp) {
    case 3:
    case 2:
      if (sfdirpath != NULL) {
        pathnam = catpath(sfdirpath, filnam);
        outfd = open(pathnam, RD_OPTS);
        break;
      }
      else if (dirtyp == 2)
        dies(Str("cannot open %s, SFDIR undefined"), filnam);
      else printf(Str("SFDIR undefined.  using current directory\n"));
    case 1:
      outfd = open(filnam, RD_OPTS);
      break;
    default: die(Str("openout: illegal dirtyp"));
    }
    retfilnam = pathnam;
    return(outfd);
}

int openrdwr(           /* open a file for writing.  If not fullpath, then  */
  char *filnam,         /*   dirtyp 1 will put it in the current directory  */
  int  dirtyp)          /*   dirtyp 2 will put it in SFDIR                  */
{                       /*   dirtyp 3 will put it in SFDIR else in cur dir  */
                        /* returns normal fd, & sets global return filename */
                        /* called by anals,dumpf (typ 1), sfopenout (typ 3) */
    char *pathnam = filnam;
    int  outfd;

    if (isfullpath(filnam))
      outfd = open(filnam, O_RDWR);
    else switch(dirtyp) {
    case 3:
    case 2:
      if (sfdirpath != NULL) {
        pathnam = catpath(sfdirpath, filnam);
        outfd = open(pathnam, O_RDWR);
        break;
      }
      else if (dirtyp == 2)
        dies(Str("cannot open %s for read/write, SFDIR undefined"),
             filnam);
      else printf(Str("SFDIR undefined.  using current directory\n"));
    case 1:
      outfd = open(filnam, O_RDWR);
      break;
    default: die(Str("openwr: illegal dirtyp"));
    }
    retfilnam = pathnam;
    return(outfd);
}
#endif/*  mills_macintosh */

/* fopenin() - patches fopen calls, searching file in current dir, INCDIR,
   SSDIR or SFDIR, in that order. Modelled on openin() above. (re May 2000) */

FILE *fopenin(char *filnam)
{
    char *pathnam = filnam;
    FILE *infil;

    if (isfullpath(filnam))
      infil = fopen(filnam, "r");
    else {
      /* Check current directory */
      if ((infil = fopen(filnam, "r")) != NULL)
        goto done;
      /* ... INCDIR directory */
      if (incdirpath != NULL) {
        pathnam = catpath(incdirpath, filnam);
        if ((infil = fopen(pathnam, "r")) != NULL)
          goto done;
      }
      /* ... SSDIR */
      if (ssdirpath != NULL) {
        pathnam = catpath(ssdirpath, filnam);
        if ((infil = fopen(pathnam, "r")) != NULL)
          goto done;
      }
      /* ... and SFDIR */
      if (sfdirpath != NULL) {
        pathnam = catpath(sfdirpath, filnam);
        if ((infil = fopen(pathnam, "r")) != NULL)
          goto done;
      }
      dies(
        Str(
            "cannot open %s.  Not in cur dir, INCDIR SSDIR or SFDIR as defined"),
        filnam);
    }
    /* (global) retfilnam can be copied by caller if found name is needed */
 done:
    retfilnam = pathnam;
    return (infil);
}

void dies(char *s, char *t)
{
    sprintf(errmsg,s,t);
    die(errmsg);
}

#if (!defined(mills_macintosh) && !defined(GAB_RT))
void die(char *s)
{
    printf("%s\n",s);
    longjmp(cenviron.exitjmp_,1);

}
#endif
