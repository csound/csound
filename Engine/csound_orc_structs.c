/*
  csound_orc_semantics.c:

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


char* getStructPathFromTree(
  CSOUND* csound,
  TREE* structValueTree
) {
    char tmp[1024];
    char* path = tmp;
    TREE* current = structValueTree->right;
    {
      size_t remaining = sizeof(tmp);
      int n = snprintf(path, remaining, "%s.", structValueTree->left->value->lexeme);
      if (n < 0) n = 0; if ((size_t)n >= remaining) n = (int)remaining - 1;
      path += n; remaining -= n;

      while(current != NULL) {
        int hasNext = current->next != NULL;
        n = snprintf(path, remaining, hasNext ? "%s." : "%s", current->value->lexeme);
        if (n < 0) n = 0; if ((size_t)n >= remaining) n = (int)remaining - 1;
        path += n; remaining -= n;
        current = current->next;
      }
    }


    size_t len = (size_t)(path - tmp);
    char* result = csound->Calloc(csound, len + 1);
    memcpy(result, tmp, len);
    result[len] = '\0';
    return result;
}

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