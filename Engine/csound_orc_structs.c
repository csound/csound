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

char* getStructPathFromTree(
  CSOUND* csound,
  TREE* structValueTree
) {
    char tmp[1024];
    char* path = tmp;
    TREE* current = structValueTree->right;
    path += sprintf(path, "%s.", structValueTree->left->value->lexeme);

    while(current != NULL) {
      int hasNext = current->next != NULL;
      path += sprintf(
        path,
        hasNext ? "%s." : "%s",
        current->value->lexeme
      );
      current = current->next;
    }

    char* result = csound->Calloc(csound, path - tmp + 1);
    strncpy(result, tmp, path - tmp);
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
    int i = 0;
    while(members != NULL) {
        CS_VARIABLE* member = (CS_VARIABLE*)members->value;
        if (!strcmp(member->varName, memberName)) {
            return member;
        }
        i++;
        members = members->next;
    }
    return NULL;
}

int initStructVar(CSOUND* csound, void* p) {
  INIT_STRUCT_VAR* init = (INIT_STRUCT_VAR*)p;
  CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)init->out;
  CS_TYPE* type = csoundGetTypeForArg(init->out);
  int len = cs_cons_length(type->members);
  int i;
  //    csound->Message(csound, "Initializing Struct...\n");
  //    csound->Message(csound, "Struct Type: %s\n", type->varTypeName);
  for (i = 0; i < len; i++) {
    CS_VAR_MEM* mem = structVar->members[i];
    int memberDimensions = *structVar->dimensions[i];

    if (memberDimensions > 0) {

      ARRAYDAT* arraySrc = (ARRAYDAT*) init->inArgs[i];
      ARRAYDAT* arrayDst = (ARRAYDAT*) &mem->value;
      arrayDst->allocated = arraySrc->allocated;
      arrayDst->arrayMemberSize = arraySrc->arrayMemberSize;
      arrayDst->dimensions = arraySrc->dimensions;
      arrayDst->sizes = arraySrc->sizes;
      arrayDst->data = arraySrc->data;
    } else {
      mem->varType->copyValue(csound, mem->varType, &mem->value, init->inArgs[i]);
    }
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
  *structVar->dimensions = csound->Calloc(csound, VARGMAX * sizeof(int));

  //    csound->Message(csound, "Initializing Struct...\n");
  //    csound->Message(csound, "Struct Type: %s\n", type->varTypeName);

  for (i = 0; i < len; i++) {
    CS_VARIABLE* memberVar = members->value;
    size_t size = (sizeof(CS_VAR_MEM) - sizeof(MYFLT)) + memberVar->memBlockSize;
    CS_VAR_MEM* mem = csound->Calloc(csound, size);

    if (memberVar->initializeVariableMemory != NULL) {
      memberVar->initializeVariableMemory(csound, memberVar, &mem->value);
    }
    mem->varType = memberVar->varType;
    structVar->members[i] = mem;
    structVar->dimensions[i] = &memberVar->dimensions;
    members = members->next;
  }
}

CS_VARIABLE* createStructVar(void* cs, void* p, int dimensions) {
  CSOUND* csound = (CSOUND*)cs;
  CS_TYPE* type = (CS_TYPE*)p;
  if (dimensions > 0) {
        CS_VARIABLE* arrayVar = createArray(cs, p, dimensions);
        arrayVar->dimensions = dimensions;
        arrayVar->varType = type;
        return createArray(cs, p, dimensions);
  }

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
  int i, count, dimensions;

  if (varDest->members == NULL) {
    varDest->members = varSrc->members;
    *varDest->dimensions = *varSrc->dimensions;
    return;
  }

  count = cs_cons_length(structType->members);
  for (i = 0; i < count; i++) {
    dimensions = *varSrc->dimensions[i];
    CS_VAR_MEM* d = varDest->members[i];
    CS_VAR_MEM* s = varSrc->members[i];

    if (dimensions > 0) {
        ARRAYDAT* arraySrc = (ARRAYDAT*) &s->value;
        ARRAYDAT* arrayDst = (ARRAYDAT*) &d->value;
        arrayDst->allocated = arraySrc->allocated;
        arrayDst->arrayMemberSize = arraySrc->arrayMemberSize;
        arrayDst->data = arraySrc->data;
        arrayDst->dimensions = arraySrc->dimensions;
        arrayDst->sizes = arraySrc->sizes;
        arrayDst->arrayType = arraySrc->arrayType;
    } else {
      d->varType->copyValue(csound, d->varType, &d->value, &s->value);
    }

  }
}

typedef struct {
  OPDS      h;
  MYFLT*    out;
  ARRAYDAT* arrayDat;
  MYFLT*    indicies[VARGMAX];
} STRUCT_ARRAY_GET;


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

    int dimensions = 0;
    TREE* currentDimension = current->right;

    while(
      currentDimension != NULL && \
      *currentDimension->value->lexeme == '['
    ) {
      dimensions += 1;
      currentDimension = currentDimension->next;
    }

    CS_TYPE* memberType = typedIdentArg == NULL ?
      csoundFindStandardTypeWithChar(memberName[0]) :
      csoundGetTypeWithVarTypeName(
        csound->typePool, typedIdentArg
    );

    // check if it's recursive type
    if (
      memberType == NULL &&
      strcmp(typedIdentArg, type->varTypeName) == 0
    ) {
      memberType = type;
    }

    if (memberType == NULL) {
      csound->Message(csound, Str(
        "ERROR: type %s used before defined\n"
        ),
        typedIdentArg
      );
      return 0;
    }

    CS_VARIABLE* var = memberType->createVariable(
      csound, type, dimensions
    );
    var->varName = cs_strdup(csound, memberName);
    var->varType = memberType;

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

    if (dimensions > 0) {
      for (int idx = 0; idx < dimensions; idx++) {
        temp[index++] = '[';
        temp[index++] = ']';
      }
    }

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
