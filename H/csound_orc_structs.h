/*
  csound_orc_structs.h:

  Copyright (C) 2023
  John ffitch, Steven Yi

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

#ifndef CSOUND_ORC_STRUCTS_H
#define CSOUND_ORC_STRUCTS_H

#include "csound.h"
#include "csound_type_system.h"

/* Public struct layout for plugins and engine */
typedef struct csstructvar {
  CS_VAR_MEM** members;      /* Pointer array of member memory blocks (owned or aliased) */
  int32_t      memberCount;  /* Number of members; needed for deallocation without type */
  int32_t      ownsMembers;  /* 1 if this instance owns members storage and should free */
} CS_STRUCT_VAR;

void csound_free_struct_members(CSOUND *csound, CS_STRUCT_VAR *var);

/* Engine-only declarations: visible when building the core library,
   hidden for external plugins. Core builds define __BUILDING_LIBCSOUND.
   Some core targets also define BUILD_PLUGINS, so include when either
   building core or when BUILD_PLUGINS is not defined. */
#if defined(__BUILDING_LIBCSOUND) || !defined(BUILD_PLUGINS)
#include "csoundCore.h"
#include "csound_orc.h"
#define MAX_STRUCT_ARG_SIZE 1024

typedef struct initstructvar {
  OPDS h;
  MYFLT* out;
  MYFLT* inArgs[VARGMAX];
} INIT_STRUCT_VAR;

char* getStructPathFromTree(CSOUND* csound, TREE* structValueTree);
int findStructMemberIndex(CONS_CELL* members, char* memberName);
CS_VARIABLE* getStructMember(CONS_CELL* members, char* memberName);
int32_t initStructVar(CSOUND* csound, void* p);
void initializeStructVar(CSOUND* csound, CS_VARIABLE* var, MYFLT* mem);
/* Note: signatures must match CS_TYPE callbacks in csound_type_system.h */
CS_VARIABLE* createStructVar(void* cs, void* p, INSDS* ctx);
void copyStructVar(CSOUND* csound, const CS_TYPE* structType, void* dest,
                   const void* src, INSDS* p);
OENTRY* new_struct_init_oentry(CSOUND* csound, CS_TYPE* type);
int32_t add_struct_definition(CSOUND* csound, TREE* structDefTree);
void freeStructVarMemory(void *csnd, void *p);
#endif

#endif /* CSOUND_ORC_STRUCTS_H */
