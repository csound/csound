/*
    getftargs.c:

    Copyright (C) 2016 Guillermo Senna.

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csdl.h"

typedef struct {
   OPDS      h;
   STRINGDAT *Scd;
   MYFLT     *ftable;
   MYFLT     *ktrig;
   MYFLT     prv_ktrig;
   int32_t       status;
} FTARGS;

static int32_t getftargs(CSOUND *, FTARGS *);

/*
    Inspiration for the implementation of this Opcode was taken
    from the following Csound Opcodes: "sprintf", "puts" and "pwd".
    Credit for that goes to their respective authors.
*/

static int32_t getftargs_init(CSOUND *csound, FTARGS *p)
{
    p->status = OK;
    if (*p->ktrig > FL(0.0))
      p->status = getftargs(csound, p);
    p->prv_ktrig = *p->ktrig;

    return p->status;
}

static int32_t getftargs_process(CSOUND *csound, FTARGS *p)
{
   if (*p->ktrig != p->prv_ktrig && *p->ktrig > FL(0.0)) {
     p->prv_ktrig = *p->ktrig;
     p->status = getftargs(csound, p);
   }

   return p->status;

}


static int32_t getftargs(CSOUND *csound, FTARGS *p)
{
    FUNC *src;
    int32 argcnt, i, strlen = 0;

    if (UNLIKELY((src = csound->FTnp2Find(csound, p->ftable)) == NULL)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("table: could not find ftable %d"),
                               (int32_t) *p->ftable);
    }

    argcnt = src->argcnt;

    for (i = 1; i != argcnt; i++)
      strlen += snprintf(NULL, 0, "%g ", src->args[i]);

    p->Scd->size = strlen;

    if (p->Scd->data == NULL) {
      p->Scd->data = (char*) csound->Calloc(csound, strlen);
    }
    else
      p->Scd->data = (char*) csound->ReAlloc(csound, p->Scd->data, strlen);

    {
      char* curr = p->Scd->data, *const end = curr + strlen;
      for (i = 1; curr != end && i != argcnt; i++) {
        curr += snprintf(curr, end-curr, "%g ", src->args[i]);
      }
    }

    return OK;
}


static OENTRY localops[] =
{
  { "getftargs",   sizeof(FTARGS),  0, 3, "S", "ik",
    (SUBR)getftargs_init, (SUBR)getftargs_process }
};

LINKAGE
