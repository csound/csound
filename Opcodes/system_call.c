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

typedef struct	{
  OPDS	h;
  MYFLT *res;
  MYFLT	*commandLine;
  MYFLT *nowait;
} SYSTEM;

#if defined(WIN32)

static int call_system(CSOUND *csound, SYSTEM *p)
{
  _flushall();
  if ( (int)*p->nowait != 0 ) {
    char *string = strdup( (char *)p->commandLine );
    char *separators = " \t\n\r";
    const char *argv[0xff];
    int arg = 0;
    argv[arg] = strtok( string, separators ); 
    while ( argv[arg++] ) {
      argv[arg] = strtok( 0, separators ); 
    }
    _spawnv(_P_NOWAIT, argv[0], argv);
    free( string );
    *p->res = (MYFLT) OK;
  } else {
    *p->res = (MYFLT) system( (char *)p->commandLine );
  }
  return OK;
}

#else

static int call_system(CSOUND *csound, SYSTEM *p)
{
  char command[256];
  if ((int)*p->nowait!=0) {
    if (fork()) {
      *p->res = 0;
      return OK;
    } else {
      system((char*)p->commandLine);
      exit(1);
    }
  } else {
    *p->res = (MYFLT)system((char*)p->commandLine);
    return OK;
  }
}

#endif
  
#define S(x)    sizeof(x)
  
  static OENTRY localops[] = {
    { "system",  S(SYSTEM), 1,   "i",   "So", (SUBR)call_system}
  };
  
  LINKAGE
    
    
    
