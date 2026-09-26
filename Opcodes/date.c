/*
    date.c:

    Copyright (C) 2006 John ffitch

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include <time.h>
#include <limits.h>

#ifdef USE_DOUBLE
#define DATE_EPOCH 0
#else
#define DATE_EPOCH 1262304000    /* 1 Jan 2010 */
#endif

#ifndef __wasi__
#include <errno.h>
#endif

#if defined(WIN32)
#include "direct.h"
#endif

#if defined(__MACH__)
#include <unistd.h>
#endif

typedef struct {
   OPDS h;
   MYFLT *time_;
   MYFLT *nano;
} DATEMYFLT;

typedef struct {
   OPDS h;
   STRINGDAT *Stime_;
   MYFLT *timstmp;
} DATESTRING;

static int32_t datemyfltset(CSOUND *csound, DATEMYFLT *p)
{
    IGN(csound);
    const time_t base = DATE_EPOCH;
#ifdef LINUX
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    *p->time_ = (MYFLT) (tp.tv_sec-base);
    *p->time_ += (MYFLT)(tp.tv_nsec)*1.0e-9;
    if (p->OUTOCOUNT==2) *p->nano =(MYFLT)tp.tv_nsec;
#else
  #ifdef __MACH__
    // There may be more accurate methods.....
    struct timeval tp;
    gettimeofday(&tp, NULL);
    *p->time_  = (MYFLT)(tp.tv_sec-base);
    *p->time_ += (MYFLT)(tp.tv_usec)*1.0e-6;
    if (p->OUTOCOUNT==2) *p->nano =(MYFLT)(tp.tv_usec * 1000);
  #else
    *p->time_ = (MYFLT) (time(NULL)-base);
    if (p->OUTOCOUNT==2) *p->nano = FL(0.0);
  #endif
#endif
    return OK;
}

static int32_t datestringset(CSOUND *csound, DATESTRING *p)
{
    time_t  temp_time;
    char    *time_string;
    if (*p->timstmp < FL(0.0)) temp_time = time(NULL);
    else {
      /* Match date's epoch and round before converting to the host time type. */
      double seconds = floor((double)*p->timstmp + 0.5) + DATE_EPOCH;
      double limit = ldexp(1.0, sizeof(time_t) * CHAR_BIT -
                          ((time_t)-1 < (time_t)0));
      if (UNLIKELY(!(seconds >= 0.0 && seconds < limit)))
        return csound->InitError(csound, "%s", Str("dates: time out of range"));
      temp_time = (time_t)seconds;
    }

    time_string = ctime(&temp_time);
    if (UNLIKELY(time_string == NULL))
      return csound->InitError(csound, "%s", Str("dates: time out of range"));
    if (p->Stime_->data != NULL) csound->Free(csound, p->Stime_->data);
    p->Stime_->data = csound->Strdup(csound, time_string);
    p->Stime_->size = strlen(time_string)+1;
    return OK;
}

typedef struct {
   OPDS       h;
   STRINGDAT *Scd;
} GETCWD;

static int32_t getcurdir(CSOUND *csound, GETCWD *p)
{
  if (p->Scd->size < 1024) {
    p->Scd->size = 1024;
    p->Scd->data = csound->ReAlloc(csound,  p->Scd->data, p->Scd->size);
  }
  if (p->Scd->data == NULL) {
      p->Scd->size = 1024;
      p->Scd->data = csound->Calloc(csound, p->Scd->size);
    }
#ifndef BARE_METAL   
#if defined(__MACH__) || defined(LINUX) || defined(__HAIKU__) || defined(__CYGWIN__) || defined(__GNUC__)
    if (UNLIKELY(getcwd(p->Scd->data, p->Scd->size-1)==NULL))
#else
    if (UNLIKELY( _getcwd(p->Scd->data, p->Scd->size-1)==NULL))
#endif
      {
        strncpy(p->Scd->data, Str("**Unknown**"), p->Scd->size);
        #ifndef __wasi__
        return csound->InitError(csound,
                                 Str("cannot determine current directory: %s\n"),
                                 strerror(errno));
        #else
        return -1;
        #endif
      }
#endif 
    return OK;
}

#ifndef MAXLINE
#define MAXLINE 8192
#endif

typedef struct {
  OPDS      h;
  STRINGDAT *Sline;
  MYFLT     *line;
  MYFLT     *Sfile;
  FILE      *fd;
  int32_t   lineno;
} READF;

static int32_t readf_delete(CSOUND *csound, void *p)
{
    IGN(csound);
    READF *pp = (READF*)p;

    if (pp->fd) fclose(pp->fd);
    pp->fd = NULL;
    pp->lineno = 0;
    return OK;
}

static int32_t readf_init_(CSOUND *csound, READF *p, int32_t isstring)
{
    char name[1024];
    if (isstring) {
      strncpy(name, ((STRINGDAT *)p->Sfile)->data, 1023);
      name[1023] = '\0';
    }
    else csound->StringArg2Name(csound, name, p->Sfile, "input.", 0);
    readf_delete(csound, p);
    p->fd = fopen(name, "r");
    p->lineno = 0;
    if (p->Sline->size < MAXLINE) {
      if (p->Sline->data != NULL) csound->Free(csound, p->Sline->data);
      p->Sline->data = (char *) csound->Calloc(csound, MAXLINE);
      p->Sline->size = MAXLINE;
    }
    if (UNLIKELY(p->fd==NULL))
      return csound->InitError(csound, "%s", Str("readf: failed to open file"));
    return OK;
}

static int32_t readf_init(CSOUND *csound, READF *p){
    return readf_init_(csound,p,0);
}

static int32_t readf_init_S(CSOUND *csound, READF *p){
    return readf_init_(csound,p,1);
}


static int32_t readf(CSOUND *csound, READF *p)
{
    p->Sline->data[0] = '\0';
    if (p->fd == NULL) {
      *p->line = -1;
      return OK;
    }
    if (UNLIKELY(fgets(p->Sline->data,
                       (int32_t)p->Sline->size-1, p->fd)==NULL)) {
      int32_t ff = feof(p->fd);
      fclose(p->fd);
      p->fd = NULL;
      /* Keep EOF distinct from a reader that has not opened its file yet. */
      p->lineno = -1;
      if (ff) {
        *p->line = -1;
        return OK;
      }
      else
        return csound->PerfError(csound, &(p->h),
                                 "%s", Str("readf: read failure"));
    }
    *p->line = ++p->lineno;
    return OK;
}

static int32_t readfi(CSOUND *csound, READF *p)
{
    if (p->fd==NULL && p->lineno==0)
      if (UNLIKELY(readf_init(csound, p)!= OK))
        return NOTOK;
    return readf(csound, p);
}

static int32_t readfi_S(CSOUND *csound, READF *p)
{
    if (p->fd==NULL && p->lineno==0)
      if (UNLIKELY(readf_init_S(csound, p)!= OK))
        return NOTOK;
    return readf(csound, p);
}


static OENTRY date_localops[] =
{
    { "date.i", sizeof(DATEMYFLT),  0,  "iI",   "", (SUBR)datemyfltset   },
    { "date.k", sizeof(DATEMYFLT),  0,  "kz",   "", (SUBR)datemyfltset,
      (SUBR)datemyfltset },
    { "dates",  sizeof(DATESTRING), 0,  "S",    "j", (SUBR)datestringset },
    { "pwd",    sizeof(GETCWD),     0,  "S",    "",  (SUBR)getcurdir     },
    { "readfi", sizeof(READF),      0,  "Si",   "i", (SUBR)readfi,
      NULL, (SUBR)readf_delete },
    { "readfi.S", sizeof(READF),    0,  "Si",   "S", (SUBR)readfi_S,
      NULL, (SUBR)readf_delete },
    { "readf",  sizeof(READF),      0,  "Sk",   "i", (SUBR)readf_init,
      (SUBR)readf, (SUBR)readf_delete                                                          },
    { "readf.S",  sizeof(READF),    0,  "Sk",   "S", (SUBR)readf_init_S,
      (SUBR)readf, (SUBR)readf_delete         }

};

LINKAGE_BUILTIN(date_localops)
