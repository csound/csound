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
   MYFLT *ans;
} DATE;

static int dateinit(CSOUND *csound, DATE *p)
{
    time_t tt = time(NULL);
    *p->ans = (MYFLT) tt;
    return OK;
}

int datesinit(CSOUND *csound, DATE *p)
{
    time_t tt = time(NULL);
    char *s = ctime(&tt);

    ((char*) p->ans)[0] = '\0';
    if ((int) strlen(s) >= csound->strVarMaxLen)
      return csound->InitError(csound, Str("dates: buffer overflow"));
    strcpy((char*) p->ans, s);
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "date",    S(DATE),     1,     "i",    "",(SUBR)dateinit, NULL, NULL },
{ "dates",   S(DATE),     1,     "S",    "",(SUBR)datesinit, NULL, NULL },
};

int date_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

