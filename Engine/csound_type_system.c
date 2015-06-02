/*
 csound_type_system.c:

 Copyright (C) 2012,2013 Steven Yi

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

#include "csound_type_system.h"
#include <string.h>
#include <stdio.h>
#include "csoundCore.h"

int csTypeExistsWithSameName(TYPE_POOL* pool, CS_TYPE* typeInstance) {
    CS_TYPE_ITEM* current = pool->head;
    while (current != NULL) {

      /* printf("Search if type [%s] == [%s]",
         current->varTypeName, typeInstance->varTypeName); */

      if (strcmp(current->cstype->varTypeName,
                 typeInstance->varTypeName) == 0) {
        return 1;
      }
      current = current->next;
    }

    return 0;
}

CS_TYPE* csoundGetTypeWithVarTypeName(TYPE_POOL* pool, char* typeName) {
    CS_TYPE_ITEM* current = pool->head;
    while (current != NULL) {
      if (strcmp(typeName, current->cstype->varTypeName) == 0) {
        return current->cstype;
      }
      current = current->next;
    }
    return NULL;
}

CS_TYPE* csoundGetTypeForVarName(TYPE_POOL* pool, char* varName) {
    CS_TYPE_ITEM* current = pool->head;
    char temp[2];
    temp[0] = varName[0];
    temp[1] = 0;
    while (current != NULL) {
      if (strcmp(temp, current->cstype->varTypeName) == 0) {
        return current->cstype;
      }
      current = current->next;
    }
    return NULL;
}

int csoundAddVariableType(CSOUND* csound, TYPE_POOL* pool, CS_TYPE* typeInstance)
{
    CS_TYPE_ITEM* item;
    if (csTypeExistsWithSameName(pool, typeInstance)) {
      return 0;
    }

    item = (CS_TYPE_ITEM*)csound->Calloc(csound, sizeof(CS_TYPE_ITEM));
    item->cstype = typeInstance;

    if (pool->head == NULL) {
      pool->head = item;
    } else {
      CS_TYPE_ITEM* current = pool->head;
      while (current->next) {
        current = current->next;
      }
      current->next = item;
      item->next = NULL;
    }

    /* printf("Adding type with type name: %s\n", typeInstance->varTypeName); */

    return 1;
}

/* VAR POOL FUNCTIONS */


CS_VAR_POOL* csoundCreateVarPool(CSOUND* csound) {
    CS_VAR_POOL* varPool = csound->Calloc(csound, sizeof(CS_VAR_POOL));
    varPool->table = cs_hash_table_create(csound);
    return varPool;
}

void csoundFreeVarPool(CSOUND* csound, CS_VAR_POOL* pool) {
    cs_hash_table_mfree_complete(csound, pool->table);
    csound->Free(csound, pool);
}

char* getVarSimpleName(CSOUND* csound, const char* varName) {
    char* retVal;

    if (varName[0] != '[') {
      retVal = (char*)csound->Calloc(csound, sizeof(char) * (strlen(varName) + 1));
      strcpy(retVal, varName);
    } else {
      int start = 0;
      int typeEnd = 0;
      int len = strlen(varName);
      int newFirstLen, newSecondLen, newTotalLen;
      char* t = (char*) varName;
      char* t2;

      while(*t == '[') {
        t++;
        start++;
      }
      typeEnd = start;
      t2 = t;
      while(*t2 != ']' && *t2 != (char)0) {
        t2++;
        typeEnd++;
      }
      t2++;
      typeEnd++;

      newFirstLen = (typeEnd - start - 1);
      newSecondLen = (len - typeEnd);
      newTotalLen = newFirstLen + newSecondLen;

      retVal = (char*)csound->Calloc(csound, sizeof(char) * (newTotalLen + 1));
      strncpy(retVal, t, newFirstLen);
      strncpy(retVal + newFirstLen, t2, newSecondLen);
    }

    return retVal;
}

CS_VARIABLE* csoundCreateVariable(void* csound, TYPE_POOL* pool,
                                  CS_TYPE* type, char* name, void* typeArg)
{
    CS_TYPE_ITEM* current = pool->head;
    if(type != NULL)
    while (current != NULL) {
      if (strcmp(type->varTypeName, current->cstype->varTypeName) == 0) {
        CS_VARIABLE* var = current->cstype->createVariable(csound, typeArg);
        var->varType = type;
        var->varName = cs_strdup(csound, name);
        return var;
      }
      current = current->next;
    }
    else ((CSOUND *)csound)->ErrorMsg(csound,
            "cannot create variable %s: NULL type", name);
    return NULL;
}

//CS_VARIABLE* csoundFindVariableWithName(CSOUND* csound, CS_VAR_POOL* pool,
//                                        const char* name) {
CS_VARIABLE* csoundFindVariableWithName(CSOUND* csound, CS_VAR_POOL* pool,
                                        const char* name)
{

    CS_VARIABLE* returnValue = cs_hash_table_get(csound, pool->table, (char*)name);

    if (returnValue == NULL && pool->parent != NULL) {
      returnValue = csoundFindVariableWithName(csound, pool->parent, name);
    }

    return returnValue;
}

CS_VARIABLE* csoundGetVariable(CS_VAR_POOL* pool, int index) {

    CS_VARIABLE* current = pool->head;
    int i;

    for(i = 0; i < index || current != NULL; i++) {
      /* THIS WAS WRONG!! && or || meant foR , ?? */
      current = current->next;
    }

    return current;
}


int csoundAddVariable(CSOUND* csound, CS_VAR_POOL* pool, CS_VARIABLE* var) {
  if(var != NULL) {
    if(pool->head == NULL) {
      pool->head = var;
      pool->tail = var;
    } else {
      pool->tail->next = var;
      pool->tail = var;
    }
    cs_hash_table_put(csound, pool->table, var->varName, var);
    // may need to revise this; var pools are accessed as MYFLT*,
    // so need to ensure all memory is aligned to sizeof(MYFLT)
    // boundaries maybe should align block size here to +7 before dividing?
    var->memBlockIndex = pool->poolSize / sizeof(MYFLT);
    pool->poolSize += var->memBlockSize;
    return 0;
  } else return -1;
}

void recalculateVarPoolMemory(void* csound, CS_VAR_POOL* pool)
{
    CS_VARIABLE* current = pool->head;
    pool->poolSize = 0;

    while (current != NULL) {
      /* VL 26-12-12: had to revert these lines to avoid memory crashes
         with higher ksmps */
      if(current->updateMemBlockSize != NULL) {
        current->updateMemBlockSize(csound, current);
      }

      current->memBlockIndex = pool->poolSize / sizeof(MYFLT);
      pool->poolSize += current->memBlockSize;

      current = current->next;
    }
}

void reallocateVarPoolMemory(void* csound, CS_VAR_POOL* pool) {
    CS_VARIABLE* current = pool->head;
    pool->poolSize = 0;

    while (current != NULL) {
      if(current->updateMemBlockSize != NULL) {
        current->updateMemBlockSize(csound, current);
      }
      current->memBlock =
        (MYFLT *)((CSOUND *)csound)->ReAlloc(csound,current->memBlock,
                                             current->memBlockSize);
      pool->poolSize += current->memBlockSize;
      current = current->next;
    }
}

void deleteVarPoolMemory(void* csnd, CS_VAR_POOL* pool) {
    CS_VARIABLE* current = pool->head, *tmp;
    CSOUND *csound = (CSOUND *)csnd;
    while (current != NULL) {
      tmp = current;
      csound->Free(csound, current->memBlock);
      current = current->next;
      csound->Free(csound, tmp);
    }
}



void initializeVarPool(MYFLT* memBlock, CS_VAR_POOL* pool) {
    CS_VARIABLE* current = pool->head;

    while (current != NULL) {
      if (current->initializeVariableMemory != NULL) {
        current->initializeVariableMemory(current,
                                          memBlock + current->memBlockIndex);
      }
      current = current->next;
    }
}

void debug_print_varpool(CSOUND* csound, CS_VAR_POOL* pool) {
    CS_VARIABLE* gVar = pool->head;
    int count = 0;
    while(gVar != NULL) {
      csound->Message(csound, "  %d) %s:%s\n", count++,
                      gVar->varName, gVar->varType->varTypeName);
      gVar = gVar->next;
    }
}
