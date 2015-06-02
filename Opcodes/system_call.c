/*
  system_call.c:

  Copyright (C) 2007 John ffitch

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
  OPDS  h;
  MYFLT *res;
  MYFLT *ktrig;
  STRINGDAT *commandLine;
  MYFLT *nowait;

  MYFLT prv_ktrig;
} SYSTEM;

#if defined(WIN32)

static void threadroutine(void *command)
{
    system( (char *)command );
    free( command );
}

static int call_system(CSOUND *csound, SYSTEM *p)
{
    _flushall();
    if ( (int)*p->nowait != 0 ) {
      char *command = strdup(p->commandLine->data);
      _beginthread( threadroutine, 0, command);
      *p->res = OK;
    }
    else {
      *p->res = (MYFLT) system( (char *)p->commandLine->data );
    }
    return OK;
}

#else
#include <unistd.h>

static int call_system(CSOUND *csound, SYSTEM *p)
{
    IGN(csound);
    if ((int)*p->nowait!=0) {
      if ((*p->res = fork()))
        return OK;
      else {
        if (UNLIKELY(system((char*)p->commandLine->data)<0)) exit(1);
        exit(0);
      }
    }
    else {
      *p->res = (MYFLT)system((char*)p->commandLine->data);
      return OK;
    }
}

#endif

int call_system_i(CSOUND *csound, SYSTEM *p)
{
    if (*p->ktrig <= FL(0.0)) {
      *p->res=FL(0.0);
      return OK;
    }
    else
      return call_system(csound, p);
}

int call_system_set(CSOUND *csound, SYSTEM *p)
{
    IGN(csound);
    p->prv_ktrig = FL(0.0);
    return OK;
}

int call_system_k(CSOUND *csound, SYSTEM *p)
{
    if (*p->ktrig == p->prv_ktrig)
      return OK;
    p->prv_ktrig = *p->ktrig;
    if (p->prv_ktrig > FL(0.0))
      return (call_system(csound, p));
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY system_localops[] = {
  { "system", S(SYSTEM), 0, 3, "k", "kSO",
                       (SUBR)call_system_set,(SUBR)call_system_k},
  { "system_i", S(SYSTEM), 0, 1, "i", "iSo", (SUBR)call_system_i}
};

LINKAGE_BUILTIN(system_localops)
