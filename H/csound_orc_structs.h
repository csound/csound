/*
  csound_orc_structs.h:

  Copyright (C) 2025
  John ffitch, Steven Yi, Hlöðver Sigurðsson

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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifndef CSOUND_ORC_STRUCTS_H
#define CSOUND_ORC_STRUCTS_H

#include "csound.h"
#include "csound_type_system.h"
#include "csoundCore.h"

typedef struct initstructvar {
  OPDS h;
  cs_float* out;
  cs_float* inArgs[VARGMAX];
} INIT_STRUCT_VAR;

#include "csound_structs.h"

/* Owned member blocks stored with the member pointer table. */
#define CSOUND_STRUCT_MEMBERS_POOLED 2

typedef struct {
    OPDS          h;
    CS_STRUCT_VAR*   out;
    cs_float*        args[VARGMAX];
} STRUCT_INIT;

/* Returns a dynamically allocated string; caller must free using csound->Free() */
int findStructMemberIndex(CONS_CELL* members, char* memberName);
CS_VARIABLE* getStructMember(CONS_CELL* members, char* memberName);
/* Checks the variable and nested struct or array members for target. */
int32_t csound_variable_contains_type(const CS_VARIABLE *var,
                                      const CS_TYPE *target);
int32_t initStructVar(CSOUND* csound, void* p);
void initializeStructVar(CSOUND* csound, CS_VARIABLE* var, cs_float* mem);
/* Note: signatures must match CS_TYPE callbacks in csound_type_system.h */
CS_VARIABLE* createStructVar(void* cs, const CS_TYPE* type,
                             const void* typeArg, INSDS* ctx);
void copyStructVar(CSOUND* csound, const CS_TYPE* structType, void* dest,
                   const void* src, INSDS* p);
typedef enum {
  CSOUND_STRUCT_COPY_SHARED_ARRAYS,
  CSOUND_STRUCT_COPY_INDEPENDENT_NO_ALLOCATION,
  CSOUND_STRUCT_COPY_INDEPENDENT_ALLOW_ALLOCATION
} CSOUND_STRUCT_COPY_MODE;
int32_t csound_copy_struct_value(CSOUND *csound,
                                 const CS_TYPE *structType,
                                 void *dest, const void *src, INSDS *ctx,
                                 CSOUND_STRUCT_COPY_MODE mode);
int32_t add_struct_definition(CSOUND* csound, TREE* structDefTree);
int32_t process_struct_definitions_two_phase(CSOUND *csound, TREE *structDefList);
int32_t process_struct_definitions_two_phase_tracked(CSOUND *csound,
                                                     TREE *structDefList,
                                                     CONS_CELL **newTypesOut);
struct type_table;
void rollback_struct_definitions(CSOUND *csound, CONS_CELL **newTypes);
/* Retain provisional struct types created or adopted by a parsed tree until
   compilation commits them or tree deletion releases the last reference. */
void retain_parsed_provisional_structs(CSOUND *csound, TREE *astTree,
                                       struct type_table *typeTable,
                                       CONS_CELL **newTypes);
void commit_provisional_structs(CSOUND *csound, CONS_CELL **retainedTypes);
void release_provisional_structs(CSOUND *csound, CONS_CELL **retainedTypes);
void freeStructVarMemory(void *csnd, void *p);
void csound_free_struct_members(CSOUND *csound, CS_STRUCT_VAR *var);

#endif /* CSOUND_ORC_STRUCTS_H */
