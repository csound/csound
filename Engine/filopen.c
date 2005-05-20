/*
    filopen.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch
              (C) 2005 Istvan Varga

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

#if defined(mac_classic) && defined(__MWERKS__)
#include <unix.h>
#endif

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

int openin(ENVIRON *csound, char *filnam)
                    /* open a file for reading. If not fullpath, will search: */
                    /* current directory, then SSDIR (if defined), then SFDIR */
{                   /* returns normal fd, also sets a global return filename  */
                    /* called by sndgetset (for soundin, gen01), and sfopenin */
    char    *pathnam;
    int     infd;

    pathnam = csoundFindInputFile(csound, filnam, "SFDIR;SSDIR");
    infd = -1;
    if (csound->retfilnam != NULL)
      mfree(csound, csound->retfilnam);
    csound->retfilnam = pathnam;
    if (pathnam != NULL)
      infd = open(pathnam, RD_OPTS);
    if (infd < 0)
      csoundDie(csound, Str("cannot open %s.  "
                            "Not in cur dir, SSDIR or SFDIR as defined"),
                        filnam);
    return infd;
}

int openout(ENVIRON *csound, char *filnam, int  dirtyp)
{                         /* open a file for writing.  If not fullpath, then  */
    char *pathnam = NULL; /*   dirtyp 1 will put it in the current directory  */
    int  outfd = -1;      /*   dirtyp 2 will put it in SFDIR                  */
                          /*   dirtyp 3 will put it in SFDIR else in cur dir  */
                          /* returns normal fd, & sets global return filename */
                          /* called by anals,dumpf (typ 1), sfopenout (typ 3) */
    if (dirtyp == 2) {
      pathnam = csoundGetEnv(csound, "SFDIR");
      if (pathnam == NULL || pathnam[0] == '\0')
        csoundDie(csound, Str("cannot open %s, SFDIR undefined"), filnam);
    }
    else if (dirtyp < 1 || dirtyp > 3)
      csoundDie(csound, Str("openout: illegal dirtyp"));
    if (dirtyp == 2 || dirtyp == 3)
      pathnam = csoundFindOutputFile(csound, filnam, "SFDIR");
    else
      pathnam = csoundFindOutputFile(csound, filnam, NULL);
    if (csound->retfilnam != NULL)
      mfree(csound, csound->retfilnam);
    csound->retfilnam = pathnam;
    if (pathnam != NULL)
      outfd = open(pathnam, WR_OPTS);
    if (outfd < 0)
      csoundDie(csound, Str("cannot open %s."), filnam);
    return outfd;
}

/* fopenin() - patches fopen calls, searching file in current dir, INCDIR,
   SSDIR or SFDIR, in that order. Modelled on openin() above. (re May 2000) */

FILE *fopenin(ENVIRON *csound, char *filnam)
{
    char    *pathnam;
    FILE    *infil;

    pathnam = csoundFindInputFile(csound, filnam, "SFDIR;SSDIR;INCDIR");
    infil = NULL;
    if (csound->retfilnam != NULL)
      mfree(csound, csound->retfilnam);
    csound->retfilnam = pathnam;
    if (pathnam != NULL)
      infil = fopen(pathnam, "r");
    if (infil == NULL)
      csoundDie(csound, Str("cannot open %s.  Not in cur dir, "
                            "INCDIR SSDIR or SFDIR as defined"), filnam);
    return infil;
}

typedef struct CSFILE_ {
    struct CSFILE_  *nxt;
    struct CSFILE_  *prv;
    int             type;
    int             fd;
    FILE            *f;
    SNDFILE         *sf;
    char            fullName[1];
} CSFILE;

/**
 * Open a file and return handle.
 *
 * void *csound:
 *   Csound instance pointer
 * void *fd:
 *   pointer a variable of type int, FILE*, or SNDFILE*, depending on 'type',
 *   for storing handle to be passed to file read/write functions
 * int type:
 *   file type, one of the following:
 *     CSFILE_FD_R:     read file using low level interface (open())
 *     CSFILE_FD_W:     write file using low level interface (open())
 *     CSFILE_STD:      use ANSI C interface (fopen())
 *     CSFILE_SND_R:    read sound file
 *     CSFILE_SND_W:    write sound file
 * const char *name:
 *   file name
 * void *param:
 *   parameters, depending on type:
 *     CSFILE_FD_R:     unused (should be NULL)
 *     CSFILE_FD_W:     unused (should be NULL)
 *     CSFILE_STD:      mode parameter (of type char*) to be passed to fopen()
 *     CSFILE_SND_R:    SF_INFO* parameter for sf_open(), with defaults for
 *                      raw file; the actual format paramaters of the opened
 *                      file will be stored in this structure
 *     CSFILE_SND_W:    SF_INFO* parameter for sf_open(), output file format
 * const char *env:
 *   list of environment variables for search path (see csoundFindInputFile()
 *   for details); if NULL, the specified name is used as it is, without any
 *   conversion or search.
 * return value:
 *   opaque handle to the opened file, for use with csoundGetFileName() or
 *   csoundFileClose(), or storing in FDCH.fp.
 *   On failure, NULL is returned.
 */

PUBLIC void *csoundFileOpen(void *csound, void *fd, int type,
                            const char *name, void *param, const char *env)
{
    CSFILE  *p = NULL;
    char    *fullName = NULL;
    SF_INFO sfinfo;
    int     nbytes = (int) sizeof(CSFILE);

    /* check file type */
    switch (type) {
      case CSFILE_FD_R:
      case CSFILE_FD_W:
        *((int*) fd) = -1;
        break;
      case CSFILE_STD:
        *((FILE**) fd) = (FILE*) NULL;
        break;
      case CSFILE_SND_R:
        memcpy(&sfinfo, (SF_INFO*) param, sizeof(SF_INFO));
        memset((SF_INFO*) param, 0, sizeof(SF_INFO));
      case CSFILE_SND_W:
        *((SNDFILE**) fd) = (SNDFILE*) NULL;
        break;
      default:
        csoundMessageS(csound, CSOUNDMSG_ERROR,
                               Str("internal error: csoundFileOpen(): "
                                   "invalid type: %d\n"), type);
        return NULL;
    }
    /* get full name */
    if (env == NULL) {
      fullName = (char*) name;
    }
    else {
      if (type == CSFILE_FD_W || type == CSFILE_SND_W ||
          (type == CSFILE_STD && ((char*) param)[0] == 'w'))
        fullName = csoundFindOutputFile(csound, name, env);
      else
        fullName = csoundFindInputFile(csound, name, env);
      if (fullName == NULL)
        return NULL;
    }
    nbytes += (int) strlen(fullName);
    /* allocate file structure */
    p = (CSFILE*) malloc((size_t) nbytes);
    if (p == NULL)
      goto err_return;
    p->nxt = (CSFILE*) ((ENVIRON*) csound)->open_files;
    p->prv = (CSFILE*) NULL;
    p->type = type;
    p->fd = -1;
    p->f = (FILE*) NULL;
    p->sf = (SNDFILE*) NULL;
    strcpy(&(p->fullName[0]), fullName);
    if (env != NULL)
      mfree(csound, fullName);
    fullName = &(p->fullName[0]);
    /* open file */
    switch (type) {
      case CSFILE_FD_R:                         /* low level read */
        p->fd = *((int*) fd) = open(fullName, RD_OPTS);
        if (*((int*) fd) < 0)
          goto err_return;
        break;
      case CSFILE_FD_W:                         /* low level write */
        p->fd = *((int*) fd) = open(fullName, WR_OPTS);
        if (*((int*) fd) < 0)
          goto err_return;
        break;
      case CSFILE_STD:                          /* stdio */
        p->f = *((FILE**) fd) = fopen(fullName, (char*) param);
        if (*((FILE**) fd) == (FILE*) NULL)
          goto err_return;
        break;
      case CSFILE_SND_R:                        /* sound file read */
        *((SNDFILE**) fd) = sf_open(fullName, SFM_READ, (SF_INFO*) param);
        if (*((SNDFILE**) fd) == (SNDFILE*) NULL) {
          /* open failed: maybe raw file ? */
          memcpy((SF_INFO*) param, &sfinfo, sizeof(SF_INFO));
          *((SNDFILE**) fd) = sf_open(fullName, SFM_READ, (SF_INFO*) param);
        }
        if (*((SNDFILE**) fd) == (SNDFILE*) NULL)
          goto err_return;
        p->sf = *((SNDFILE**) fd);
        break;
      case CSFILE_SND_W:                        /* sound file write */
        *((SNDFILE**) fd) = sf_open(fullName, SFM_WRITE, (SF_INFO*) param);
        if (*((SNDFILE**) fd) == (SNDFILE*) NULL)
          goto err_return;
        sf_command(*((SNDFILE**) fd), SFC_SET_CLIPPING, NULL, SF_TRUE);
        p->sf = *((SNDFILE**) fd);
        break;
    }
    /* link into chain of open files */
    if (((ENVIRON*) csound)->open_files != NULL)
      ((CSFILE*) ((ENVIRON*) csound)->open_files)->prv = p;
    ((ENVIRON*) csound)->open_files = (void*) p;
    /* return with opaque file handle */
    return (void*) p;

 err_return:
    if (p != NULL)
      free(p);
    if (fullName != NULL && env != NULL)
      mfree(csound, fullName);
    return NULL;
}

/**
 * Get the full name of a file previously opened with csoundFileOpen().
 */

PUBLIC char *csoundGetFileName(void *csound, void *fd)
{
    csound = csound;
    return &(((CSFILE*) fd)->fullName[0]);
}

/**
 * Close a file previously opened with csoundFileOpen().
 */

PUBLIC int csoundFileClose(void *csound, void *fd)
{
    CSFILE  *p = (CSFILE*) fd;
    int     retval = -1;

    /* close file */
    switch (p->type) {
      case CSFILE_FD_R:
      case CSFILE_FD_W:
        retval = close(p->fd);
        break;
      case CSFILE_STD:
        retval = fclose(p->f);
        break;
      case CSFILE_SND_R:
      case CSFILE_SND_W:
        retval = sf_close(p->sf);
        break;
    }
    /* unlink from chain of open files */
    if (p->prv == NULL)
      ((ENVIRON*) csound)->open_files = (void*) p->nxt;
    else
      p->prv->nxt = p->nxt;
    /* free allocated memory */
    free(fd);
    /* return with error value */
    return retval;
}

/* Close all open files; called by csoundReset(). */

void close_all_files(void *csound)
{
    while (((ENVIRON*) csound)->open_files != NULL)
      csoundFileClose(csound, ((ENVIRON*) csound)->open_files);
}

