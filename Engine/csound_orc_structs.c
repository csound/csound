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

int32_t initStructVar(CSOUND* csound, void* p) {
  INIT_STRUCT_VAR* init = (INIT_STRUCT_VAR*)p;
  CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)init->out;
  CS_TYPE* type = csoundGetTypeForArg(init->out);
  int32_t len = cs_cons_length(type->members);
  int32_t i;
  if(csoundGetDebug(csound) & DEBUG_SEMANTICS) {
     csound->Message(csound, "Initializing Struct...\n");
     csound->Message(csound, "Struct Type: %s\n", type->varTypeName);
  }
  for (i = 0; i < len; i++) {
    CS_VAR_MEM* mem = structVar->members[i];
    mem->varType->copyValue(csound, mem->varType, &mem->value,
                            init->inArgs[i], init->h.insdshead);
  }

  return CSOUND_SUCCESS;
}

void initializeStructVar(CSOUND* csound, CS_VARIABLE* var, MYFLT* mem) {
  CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)mem;
  const CS_TYPE* type = var->varType;
  CONS_CELL* members = type->members;

  int32_t len = cs_cons_length(members);
  int32_t i;

  structVar->members = csound->Calloc(csound, len * sizeof(CS_VAR_MEM*));
  structVar->memberCount = len;  // Set the member count
  structVar->ownsMembers = 1;    // This struct owns its members
  if(csoundGetDebug(csound) & DEBUG_SEMANTICS) {
      csound->Message(csound, "Initializing Struct...\n");
      csound->Message(csound, "Struct Type: %s\n", type->varTypeName);
  }
  for (i = 0; i < len; i++) {
    CS_VARIABLE* var = members->value;
    size_t size = (sizeof(CS_VAR_MEM) - sizeof(MYFLT)) + var->memBlockSize;
    CS_VAR_MEM* mem = csound->Calloc(csound, size);
    if (var->initializeVariableMemory != NULL) {
      var->initializeVariableMemory(csound, var, &mem->value);
    }
    mem->varType = var->varType;
    structVar->members[i] = mem;

    members = members->next;
  }
}

CS_VARIABLE* createStructVar(void* cs, const CS_TYPE *p, INSDS *ctx) {
  CSOUND* csound = (CSOUND*)cs;
  
  if (p == NULL) {
    csound->Message(csound, "ERROR: no type given for struct creation\n");
    return NULL;
  }

  CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
  var->memBlockSize = sizeof(CS_STRUCT_VAR);
  var->initializeVariableMemory = initializeStructVar;
  var->varType = p;
  var->ctx = ctx;

  //FIXME - implement
  return var;
}

void copyStructVar(CSOUND* csound, const CS_TYPE* structType, void* dest, const
                   void* src, INSDS *p) {
  CS_STRUCT_VAR* varDest = (CS_STRUCT_VAR*)dest;
  CS_STRUCT_VAR* varSrc = (CS_STRUCT_VAR*)src;
  int32_t i, count;

  // Don't copy to itself
  if (dest == src) {
    return;
  }

  if (varDest->members == NULL || varSrc->members == NULL) {
    csound->Message(csound, "struct not initialised - cannot copy\n");
    return;  // Can't copy if members aren't initialized
  }

  count = cs_cons_length(structType->members);
  for (i = 0; i < count; i++) {
    CS_VAR_MEM* d = varDest->members[i];
    CS_VAR_MEM* s = varSrc->members[i];
    if (d != NULL && s != NULL) {
      // Check if d and s are the same (aliased)
      if (d == s) {
        // Already aliased, nothing to copy
        continue;
      }
      d->varType->copyValue(csound, d->varType, &d->value, &s->value, p);
    }
  }
}
