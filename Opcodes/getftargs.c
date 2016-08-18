/*
    getfttrgs.c:

    Copyright (C) 2016 Guillermo Senna 

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

typedef struct {
   OPDS h;
   STRINGDAT *Scd;
   MYFLT *ftable;
   MYFLT *ktrig;
   MYFLT  prv_ktrig;
   int status;
} FTARGS;

static int getftargs(CSOUND *, FTARGS *);

static int getftargs_init(CSOUND *csound, FTARGS *p)
{
    p->status = OK;
    if (*p->ktrig > FL(0.0))
      p->status = getftargs(csound, p);
    p->prv_ktrig = *p->ktrig;

    return p->status;
}

static int getftargs_process(CSOUND *csound, FTARGS *p)
{
   if (*p->ktrig != p->prv_ktrig && *p->ktrig > FL(0.0)) {
     p->prv_ktrig = *p->ktrig;
     p->status = getftargs(csound, p);
   }

   return p->status;

}


static int getftargs(CSOUND *csound, FTARGS *p)
{
    FUNC *src;
    int32 len, i;
    char *curr;
    char *const end;
    
    p->Scd->size = 1024;
    if (p->Scd->data == NULL) {
      p->Scd->data = (char*) csound->Calloc(csound, p->Scd->size);
    }
    else p->Scd->data = (char*) csound->ReAlloc(csound, p->Scd->data, p->Scd->size);

    if (UNLIKELY((src = csound->FTnp2Find(csound, p->ftable)) == NULL)) {
      return csound->PerfError(csound, p->h.insdshead,
                              Str("table: could not find ftable %d"),
                              (int) *p->ftable);
    }

    len = src->argcnt;
    {
      char* curr = p->Scd->data, *const end = curr + 1024;
      for (i = 1; curr != end && i != len;i++) {
        curr += snprintf(curr, end-curr, "%f ", src->args[i]);
      }
    }

    return OK;
}


static OENTRY localops[] =
{
  { "getftargs",   sizeof(FTARGS),  0, 3, "S", "ik", (SUBR)getftargs_init, (SUBR)getftargs_process }
};

LINKAGE
