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

int initStructVar(CSOUND* csound, void* p) {
  INIT_STRUCT_VAR* init = (INIT_STRUCT_VAR*)p;
  CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)init->out;
  printf("structVar out %p\n", structVar);
  CS_TYPE* type = csoundGetTypeForArg(init->out);
  int len = cs_cons_length(type->members);
  int i;
    printf("init->inArgs %p\n", init->inArgs);
  //    csound->Message(csound, "Initializing Struct...\n");
  //    csound->Message(csound, "Struct Type: %s\n", type->varTypeName);
  for (i = 0; i < len; i++) {
    CS_VAR_MEM* mem = structVar->members[i];
    printf("initStructVar[%d] mem %p mem->value %p\n", i, mem, &mem->value);
    mem->varType->copyValue(csound, mem->varType, &mem->value, init->inArgs[i]);
  }

  return CSOUND_SUCCESS;
}

void initializeStructVar(CSOUND* csound, CS_VARIABLE* var, MYFLT* mem) {
  CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)mem;
  CS_TYPE* type = var->varType;
  CONS_CELL* members = type->members;
  int len = cs_cons_length(members);
  int i;

  structVar->members = csound->Calloc(csound, len * sizeof(CS_VAR_MEM*));

  //    csound->Message(csound, "Initializing Struct...\n");
  //    csound->Message(csound, "Struct Type: %s\n", type->varTypeName);

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

CS_VARIABLE* createStructVar(void* cs, void* p, int dimensions) {
  CSOUND* csound = (CSOUND*)cs;
  CS_TYPE* type = (CS_TYPE*)p;

  if (type == NULL) {
    csound->Message(csound, "ERROR: no type given for struct creation\n");
    return NULL;
  }

  CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
  IGN(p);
  var->memBlockSize = sizeof(CS_STRUCT_VAR);
  var->initializeVariableMemory = initializeStructVar;
  var->varType = type;

  //FIXME - implement
  return var;
}

void copyStructVar(CSOUND* csound, CS_TYPE* structType, void* dest, void* src) {
  CS_STRUCT_VAR* varDest = (CS_STRUCT_VAR*)dest;
  CS_STRUCT_VAR* varSrc = (CS_STRUCT_VAR*)src;
  int i, count;

  count = cs_cons_length(structType->members);
  for (i = 0; i < count; i++) {
    CS_VAR_MEM* d = varDest->members[i];
    CS_VAR_MEM* s = varSrc->members[i];
    d->varType->copyValue(csound, d->varType, &d->value, &s->value);
  }
}

static CS_TYPE* new_struct_cs_type(
  CSOUND* csound,
  char* varTypeName
) {
  CS_TYPE* type = csound->Calloc(csound, sizeof(CS_TYPE));
  type->varTypeName = cs_strdup(csound, varTypeName);
  type->varDescription = "user-defined struct";
  type->argtype = CS_ARG_TYPE_BOTH;
  type->createVariable = createStructVar;
  type->copyValue = copyStructVar;
  type->userDefinedType = strlen(varTypeName) > 1;
  return type;
}

OENTRY* new_struct_init_oentry(
  CSOUND* csound,
  CS_TYPE* type
) {
  OENTRY* oentry = csound->Calloc(csound, sizeof(OENTRY));
  char temp[MAX_STRUCT_ARG_SIZE];
  memset(temp, '\0', MAX_STRUCT_ARG_SIZE);
  cs_sprintf(temp, "init.%s", type->varTypeName);
  oentry->opname = cs_strdup(csound, temp);
  oentry->dsblksiz = sizeof(INIT_STRUCT_VAR);
  oentry->flags = 0;
  oentry->thread = 1;
  oentry->iopadr = initStructVar;
  oentry->kopadr = NULL;
  oentry->aopadr = NULL;
  oentry->useropinfo = NULL;
  memset(temp, '\0', MAX_STRUCT_ARG_SIZE);
  cs_sprintf(temp, ":%s;", type->varTypeName);
  oentry->outypes = cs_strdup(csound, temp);
  return oentry;
}

int add_struct_definition(CSOUND* csound, TREE* structDefTree) {
  char temp[MAX_STRUCT_ARG_SIZE];
  TREE* current = structDefTree->right;

  CS_TYPE* type = new_struct_cs_type(
    csound,
    structDefTree->left->value->lexeme
  );
  OENTRY* oentry = new_struct_init_oentry(
    csound,
    type
  );

  int index = 0;
  while (current != NULL) {
    char* memberName = current->value->lexeme;
    char* typedIdentArg = current->value->optype;

    CS_TYPE* memberType = typedIdentArg == NULL ?
        csoundFindStandardTypeWithChar(memberName[0]) :
        csoundGetTypeWithVarTypeName(
            csound->typePool, typedIdentArg
        );

    CS_VARIABLE* var = memberType->createVariable(csound, type, 0);
    var->varName = cs_strdup(csound, memberName);
    var->varType = memberType;
    int dimensions = 0;
    if (memberType == (CS_TYPE*) &CS_VAR_TYPE_A) {
      TREE* currentDimension = current->right;
      while(
        currentDimension != NULL && \
        *currentDimension->value->lexeme == '['
      ) {
        dimensions += 1;
        currentDimension = currentDimension->next;
      }
    }
    var->dimensions = dimensions;

    int len = strlen(memberType->varTypeName);

    if (len > 1) {
      temp[index++] = ':';
    }

    memcpy(
      temp + index,
      memberType->varTypeName,
      len
    );
    index += len;

    if (len > 1) {
      temp[index++] = ';';
    }

    CONS_CELL* member = csound->Calloc(csound, sizeof(CONS_CELL));
    member->value = var;
    type->members = cs_cons_append(type->members, member);
    member = member->next;
    current = current->next;
  }

  // we don't worry about non-zero returns here
  // since already defined variables can happen when
  // 2 or more files include another file
  csoundAddVariableType(csound, csound->typePool, type);

  temp[index] = '\0';
  oentry->intypes = cs_strdup(csound, temp);
  csoundAppendOpcodes(csound, oentry, 1);
  return 1;
}
