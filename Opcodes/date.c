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

typedef struct {
   OPDS h;
   MYFLT *time_;
} DATEMYFLT;

typedef struct {
   OPDS h;
   MYFLT *Stime_;
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
    *p->time_ = (MYFLT) (time(NULL)-base);
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
    ((char*) p->Stime_)[0] = '\0';
    if (UNLIKELY((int) strlen(time_string) >= csound->GetStrVarMaxLen(csound))) {
      return csound->InitError(csound, Str("dates: buffer overflow"));
    }
    /* q = strchr(time_string, '\n'); */
    /* if (q) *q='\0'; */
    strcpy((char*) p->Stime_, time_string);
    return OK;
}

typedef struct {
   OPDS h;
   MYFLT *Scd;
} GETCWD;

static int getcurdir(CSOUND *csound, GETCWD *p)
{
    if (UNLIKELY(
#if defined(__MACH__) || defined(LINUX) || defined(__HAIKU__)
                 getcwd
#else
                 _getcwd
#endif
                 (((char*) p->Scd), csound->GetStrVarMaxLen(csound))==NULL))
      return csound->InitError(csound, Str("cannot determine current directory"));
    return OK;
}

typedef struct {
  OPDS h;
  MYFLT *Sline;
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

static int readf_init(CSOUND *csound, READF *p)
{
    char name[1024];
    csound->strarg2name(csound, name, p->Sfile, "input.", p->XSTRCODE);
    p->fd = fopen(name, "r");
    p->lineno = 0;
    if (UNLIKELY(p->fd==NULL))
      return csound->InitError(csound, Str("readf: failed to open file"));
    return csound->RegisterDeinitCallback(csound, p, readf_delete);
}

static int readf(CSOUND *csound, READF *p)
{
    ((char*) p->Sline)[0] = '\0';
    if (UNLIKELY(fgets((char*) p->Sline,
                       csound->GetStrVarMaxLen(csound), p->fd)==NULL)) {
      int ff = feof(p->fd);
      fclose(p->fd);
      p->fd = NULL;
      if (ff) {
        *p->line = -1;
        return OK;
      }
      else
        return csound->PerfError(csound, Str("readf: read failure"));
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


static OENTRY date_localops[] =
{
    { "date",   sizeof(DATEMYFLT),  0, 1, "i",    "", (SUBR)datemyfltset   },
    { "dates",  sizeof(DATESTRING), 0, 1, "S",    "j", (SUBR)datestringset },
    { "pwd",    sizeof(GETCWD),     0, 1, "S",    "",  (SUBR)getcurdir     },
    { "readfi", sizeof(READF),      0, 1, "Si",   "T", (SUBR)readfi,       },
    { "readf",  sizeof(READF),      0, 3, "Sk",   "T", (SUBR)readf_init, (SUBR)readf }

};

LINKAGE_BUILTIN(date_localops)

