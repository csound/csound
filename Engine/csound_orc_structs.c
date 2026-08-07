/*
  csound_orc_structs.c:

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

#include "csoundCore.h"
#include "csound_orc_structs.h"
#include "csound_type_system.h"
#include "csound_orc_semantics.h"
#include "csound_standard_types.h"

typedef struct {
  OPDS      h;
  MYFLT*    out;
  ARRAYDAT* arrayDat;
  MYFLT*    indicies[VARGMAX];
} STRUCT_ARRAY_GET;

int findStructMemberIndex(CONS_CELL* members, char* memberName) {
    int i = 0;
    while(members != NULL) {
        CS_VARIABLE* member = (CS_VARIABLE*)members->value;
        if (!strcmp(member->varName, memberName)) {
            return i;
        }
        i++;
        members = members->next;
    }
    return -1;
}

CS_VARIABLE* getStructMember(CONS_CELL* members, char* memberName) {

    while(members != NULL) {
        CS_VARIABLE* member = (CS_VARIABLE*)members->value;
        if (!strcmp(member->varName, memberName)) {
            return member;
        }

        members = members->next;
    }
    return NULL;
}

typedef struct csound_type_path {
  const CS_TYPE *type;
  const struct csound_type_path *parent;
} CSOUND_TYPE_PATH;

static int32_t type_contains_type(const CS_TYPE *type,
                                  const CS_TYPE *target,
                                  const CSOUND_TYPE_PATH *path);

static int32_t variable_contains_type(const CS_VARIABLE *var,
                                      const CS_TYPE *target,
                                      const CSOUND_TYPE_PATH *path) {
  if (var == NULL || var->varType == NULL || target == NULL)
    return 0;
  if (var->varType == target)
    return 1;
  return type_contains_type(var->varType == &CS_VAR_TYPE_ARRAY
                              ? var->subType : var->varType,
                            target, path);
}

static int32_t type_contains_type(const CS_TYPE *type,
                                  const CS_TYPE *target,
                                  const CSOUND_TYPE_PATH *path) {
  const CSOUND_TYPE_PATH *current = path;
  const CONS_CELL *members;

  if (type == NULL || target == NULL)
    return 0;
  if (type == target)
    return 1;
  if (!type->userDefinedType)
    return 0;
  while (current != NULL) {
    if (current->type == type)
      return 0;
    current = current->parent;
  }

  CSOUND_TYPE_PATH next = { type, path };
  members = type->members;
  while (members != NULL) {
    if (variable_contains_type((const CS_VARIABLE *) members->value,
                               target, &next))
      return 1;
    members = members->next;
  }
  return 0;
}

int32_t csound_variable_contains_type(const CS_VARIABLE *var,
                                      const CS_TYPE *target) {
  return variable_contains_type(var, target, NULL);
}

/* Deep-free struct members if this instance owns them.
   Safe to call on aliases; does nothing if ownsMembers==0 or members==NULL. */
void csound_free_struct_members(CSOUND *csound, CS_STRUCT_VAR *var) {
  if (var == NULL || var->members == NULL || var->ownsMembers == 0) {
    return;
  }

  int32_t i;
  for (i = 0; i < var->memberCount; i++) {
    CS_VAR_MEM *mem = var->members[i];
    if (mem != NULL) {
      if (mem->varType && mem->varType->freeVariableMemory) {
        mem->varType->freeVariableMemory(csound, &mem->value);
      }
      csound->Free(csound, mem);
      var->members[i] = NULL;  // Prevent double-free
    }
  }
  csound->Free(csound, var->members);
  var->members = NULL;
  var->memberCount = 0;
  var->ownsMembers = 0;
}

void freeStructVarMemory(void *csnd, void *p) {
  CSOUND *csound = (CSOUND *)csnd;
  CS_STRUCT_VAR *var = (CS_STRUCT_VAR *)p;
  csound_free_struct_members(csound, var);
}
