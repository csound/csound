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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"
#include <time.h>

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

static int datemyfltset(CSOUND *csound, DATEMYFLT *p)
{
#ifdef USE_DOUBLE
    const time_t base = 0;
#else
    /*    time_t base = 946684800; */  /* 1 Jan 2000 */
    const time_t base = 1262304000;    /* 1 Jan 2010 */
#endif
#ifdef LINUX
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    *p->time_ = (MYFLT) (tp.tv_sec-base);
    *p->time_ += (MYFLT)(tp.tv_nsec)*1.0e-9;
    if (p->OUTOCOUNT==2) *p->nano =(MYFLT)tp.tv_nsec;
#else
  #ifdef __MACH
    struct timeval tp;
    int rv = gettimeofday(&tp, NULL);
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

static int datestringset(CSOUND *csound, DATESTRING *p)
{
    time_t temp_time;
    char *time_string;
    /* char *q; */
    int32 tmp;

#if defined(MSVC) || (defined(__GNUC__) && defined(__i386__))
   tmp = (int32) MYFLT2LRND(*(p->timstmp));
#else
  tmp = (int32) (*(p->timstmp) + FL(0.5));
#endif
    if (tmp <= 0) temp_time = time(NULL);
    else         temp_time = (time_t)tmp;

    time_string = ctime(&temp_time);
    /*    printf("Timestamp = %f\ntimestring=>%s<\n", *p->timstmp, time_string); */

    /* q = strchr(time_string, '\n'); */
    /* if (q) *q='\0'; */
    if (p->Stime_->data != NULL) csound->Free(csound, p->Stime_->data);
    p->Stime_->data = csound->Strdup(csound, time_string);
    p->Stime_->size = strlen(time_string)+1;
    return OK;
}

typedef struct {
   OPDS h;
   STRINGDAT *Scd;
} GETCWD;

static int getcurdir(CSOUND *csound, GETCWD *p)
{
    if (p->Scd->data == NULL) {
      p->Scd->size = 1024;
      p->Scd->data = csound->Calloc(csound, p->Scd->size);
    }

#if defined(__MACH__) || defined(LINUX) || defined(__HAIKU__) || defined(__CYGWIN__)
    if (UNLIKELY(getcwd(p->Scd->data, p->Scd->size-1)==NULL))
#else
    if (UNLIKELY( _getcwd(p->Scd->data, p->Scd->size-1)==NULL))
#endif
      return csound->InitError(csound, Str("cannot determine current directory"));
    return OK;
}

#ifndef MAXLINE
#define MAXLINE 1024
#endif

typedef struct {
  OPDS h;
  STRINGDAT *Sline;
  MYFLT *line;
  MYFLT *Sfile;
  FILE  *fd;
  int   lineno;
} READF;

static int readf_delete(CSOUND *csound, void *p)
{
    READF *pp = (READF*)p;

    if (pp->fd) fclose(pp->fd);
    return OK;
}

static int readf_init_(CSOUND *csound, READF *p, int isstring)
{
    char name[1024];
    if(isstring) strncpy(name, ((STRINGDAT *)p->Sfile)->data, 1023);
    else csound->strarg2name(csound, name, p->Sfile, "input.", 0);
    p->fd = fopen(name, "r");
    p->lineno = 0;
    if (p->Sline->data == NULL) {
      p->Sline->data = (char *) csound->Calloc(csound, MAXLINE);
    p->Sline->size = MAXLINE;
    }
    if (UNLIKELY(p->fd==NULL))
      return csound->InitError(csound, Str("readf: failed to open file"));
    return csound->RegisterDeinitCallback(csound, p, readf_delete);
}

static int readf_init(CSOUND *csound, READF *p){
    return readf_init_(csound,p,0);
}

static int readf_init_S(CSOUND *csound, READF *p){
    return readf_init_(csound,p,1);
}


static int readf(CSOUND *csound, READF *p)
{
    p->Sline->data[0] = '\0';
    if (UNLIKELY(fgets(p->Sline->data,
                       p->Sline->size-1, p->fd)==NULL)) {
      int ff = feof(p->fd);
      fclose(p->fd);
      p->fd = NULL;
      if (ff) {
        *p->line = -1;
        return OK;
      }
      else
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("readf: read failure"));
    }
    *p->line = ++p->lineno;
    return OK;
}

static int readfi(CSOUND *csound, READF *p)
{
    if (p->fd==NULL)
      if (UNLIKELY(readf_init(csound, p)!= OK))
        return csound->InitError(csound, Str("readi failed to initialise"));
    return readf(csound, p);
}

static int readfi_S(CSOUND *csound, READF *p)
{
    if (p->fd==NULL)
      if (UNLIKELY(readf_init_S(csound, p)!= OK))
        return csound->InitError(csound, Str("readi failed to initialise"));
    return readf(csound, p);
}


static OENTRY date_localops[] =
{
    { "date.i", sizeof(DATEMYFLT),  0, 1, "iI",   "", (SUBR)datemyfltset   },
    { "date.k", sizeof(DATEMYFLT),  0, 3, "kz",   "", (SUBR)datemyfltset,
      (SUBR)datemyfltset },
    { "dates",  sizeof(DATESTRING), 0, 1, "S",    "j", (SUBR)datestringset },
    { "pwd",    sizeof(GETCWD),     0, 1, "S",    "",  (SUBR)getcurdir     },
    { "readfi", sizeof(READF),      0, 1, "Si",   "i", (SUBR)readfi,       },
    { "readfi.S", sizeof(READF),    0, 1, "Si",   "S", (SUBR)readfi_S,       },
    { "readf",  sizeof(READF),      0, 3, "Sk",   "i", (SUBR)readf_init,
      (SUBR)readf         },
    { "readf.S",  sizeof(READF),      0, 3, "Sk",   "S", (SUBR)readf_init_S,
                                                       (SUBR)readf         }

};

LINKAGE_BUILTIN(date_localops)
