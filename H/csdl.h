/*
    csdl.h:

    Copyright (C) 2002 John ffitch

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

#ifndef CSOUND_CSDL_H
#define CSOUND_CSDL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __BUILDING_LIBCSOUND
#undef __BUILDING_LIBCSOUND
#endif

#include "csoundCore.h"
#include <limits.h>

#ifdef Str
#undef Str
#endif
#define Str(x) (((CSOUND*) csound)->LocalizeString(x))

PUBLIC  long    opcode_size(void);
PUBLIC  OENTRY  *opcode_init(CSOUND *);
PUBLIC  NGFENS  *fgen_init(CSOUND *);

PUBLIC  int     csoundModuleCreate(CSOUND *);
PUBLIC  int     csoundModuleInit(CSOUND *);
PUBLIC  int     csoundModuleDestroy(CSOUND *);
PUBLIC  char    *csoundModuleErrorCodeToString(int);

PUBLIC  int     csoundModuleMYFLTSize(void);

#define LINKAGE                         \
PUBLIC long opcode_size(void)           \
{   return (long) sizeof(localops); }   \
PUBLIC OENTRY *opcode_init(CSOUND *xx)  \
{   (void) xx; return localops;     }   \
PUBLIC int csoundModuleMYFLTSize(void)  \
{   return (int) sizeof(MYFLT);     }

#define FLINKAGE                        \
PUBLIC long opcode_size(void)           \
{   if (localops == NULL) return LONG_MIN;              \
    else return ((long) sizeof(localops) | LONG_MIN); } \
PUBLIC OENTRY *opcode_init(CSOUND *xx)  \
{   (void) xx; return localops;     }   \
PUBLIC NGFENS *fgen_init(CSOUND *xx)    \
{   (void) xx; return localfgens;   }   \
PUBLIC int csoundModuleMYFLTSize(void)  \
{   return (int) sizeof(MYFLT);     }

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_CSDL_H */

