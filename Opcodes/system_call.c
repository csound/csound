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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/




#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#if !(defined(__wasi__))

typedef struct {
  OPDS  h;
  MYFLT *res;
  MYFLT *ktrig;
  STRINGDAT *commandLine;
  MYFLT *nowait;
  char *command;
  MYFLT prv_ktrig;
  CSOUND *csound;
} SYSTEM;

#if defined(WIN32)
#include <process.h>

static void threadroutine(void *p)
{
    SYSTEM *pp = (SYSTEM *) p;
    system(pp->command);
    pp->csound->Free(pp->csound,pp->command);
}

static int32_t call_system(CSOUND *csound, SYSTEM *p)
{
    _flushall();
    if ( (int32_t)*p->nowait != 0 ) {
       p->command = csound->Strdup(csound, p->commandLine->data);
       p->csound = csound;
      _beginthread( threadroutine, 0, p);
      *p->res = OK;
    }
    else {
      *p->res = (MYFLT) system( (char *)p->commandLine->data );
    }
    return OK;
}

#else
#include <unistd.h>

#ifdef __APPLE__  
#include <TargetConditionals.h>
#endif

static int32_t call_system(CSOUND *csound, SYSTEM *p)
{
    IGN(csound);

#if TARGET_OS_IPHONE || __wasm__
return OK;
#else
    if ((int32_t)*p->nowait!=0) {
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
#endif


}

#endif

int32_t call_system_i(CSOUND *csound, SYSTEM *p)
{
    if (*p->ktrig <= FL(0.0)) {
      *p->res=FL(0.0);
      return OK;
    }
    else
      return call_system(csound, p);
}

int32_t call_system_set(CSOUND *csound, SYSTEM *p)
{
    IGN(csound);
    p->prv_ktrig = FL(0.0);
    return OK;
}

int32_t
call_system_k(CSOUND *csound, SYSTEM *p)
{
    if (*p->ktrig == p->prv_ktrig)
      return OK;
    p->prv_ktrig = *p->ktrig;
    if (p->prv_ktrig > FL(0.0))
      return (call_system(csound, p));
    return OK;
}

#else

int32_t call_system_i(CSOUND *csound, void *p)
{
  IGN(csound); IGN(p);
    return OK;
}

int32_t call_system_set(CSOUND *csound, void *p)
{
  IGN(csound); IGN(p);
    return OK;
}

int32_t
call_system_k(CSOUND *csound, void *p)
{
  IGN(csound); IGN(p);
    return OK;

}

#endif // !wasi

#define S(x)    sizeof(x)

static OENTRY system_localops[] = {
  { "system", S(SYSTEM), 0,  "k", "kSO",
                       (SUBR)call_system_set,(SUBR)call_system_k},
  { "system_i", S(SYSTEM), 0,  "i", "iSo", (SUBR)call_system_i}
};

LINKAGE_BUILTIN(system_localops)
