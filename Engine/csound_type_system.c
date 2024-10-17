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
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 02110-1301 USA
 */

#include "csound_type_system.h"
#include <string.h>
#include <stdio.h>
#include "csoundCore.h"
#include "aops.h"

int32_t csTypeExistsWithSameName(TYPE_POOL* pool, CS_TYPE* typeInstance) {
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

const CS_TYPE* csoundGetTypeWithVarTypeName(const TYPE_POOL* pool, const char* typeName) {

    CS_TYPE_ITEM* current = pool->head;    
    while (current != NULL) {
      if (strcmp(typeName, current->cstype->varTypeName) == 0) {
        return current->cstype;
      }
      current = current->next;
    }
    return NULL;
}

int32_t csoundAddVariableType(CSOUND* csound, TYPE_POOL* pool, CS_TYPE* typeInstance)
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

TYPE_POOL *csoundGetTypePool(CSOUND* csound) {
  return csound->typePool;
}


/* VAR POOL FUNCTIONS */


CS_VAR_POOL* csoundCreateVarPool(CSOUND* csound) {
    CS_VAR_POOL* varPool = csound->Calloc(csound, sizeof(CS_VAR_POOL));
    varPool->table = cs_hash_table_create(csound);
    return varPool;
}

void csoundFreeVarPool(CSOUND* csound, CS_VAR_POOL* pool) {
    if(pool->table) cs_hash_table_mfree_complete(csound, pool->table);
    csound->Free(csound, pool);
}

char* getVarSimpleName(CSOUND* csound, const char* varName) {
    char* retVal;

    if (varName[0] != '[') {
      retVal = (char*)csound->Calloc(csound, sizeof(char) * (strlen(varName) + 1));
      strcpy(retVal, varName);
    } else {
      int32_t start = 0;
      int32_t typeEnd = 0;
      int32_t len = (int32_t) strlen(varName);
      int32_t newFirstLen, newSecondLen, newTotalLen;
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

/** Create variable outside an instrument context */
CS_VARIABLE* csoundCreateVariable(CSOUND* csound, TYPE_POOL* pool,
                                  const CS_TYPE* type, char* name, void* typeArg)
{
    CS_TYPE_ITEM* current = pool->head;
    if (LIKELY(type != NULL))
      while (current != NULL) {
        if (strcmp(type->varTypeName, current->cstype->varTypeName) == 0) {
          CS_VARIABLE* var = current->cstype->createVariable(csound, typeArg, NULL);
          var->varType = type;
          var->varName = cs_strdup(csound, name);
          return var;
        }
        current = current->next;
      }
    else ((CSOUND *)csound)->ErrorMsg(csound,
                                      Str("cannot create variable %s: NULL type"),
                                      name);
    return NULL;
}

CS_VARIABLE* csoundFindVariableWithName(CSOUND* csound, CS_VAR_POOL* pool,
                                        const char* name)
{

    CS_VARIABLE* returnValue = cs_hash_table_get(csound, pool->table, (char*)name);

    if (returnValue == NULL && pool->parent != NULL) {
      returnValue = csoundFindVariableWithName(csound, pool->parent, name);
    }

    return returnValue;
}

CS_VARIABLE* csoundGetVariable(CS_VAR_POOL* pool, int32_t index) {

    CS_VARIABLE* current = pool->head;
    int32_t i;

    for(i = 0; i < index || current != NULL; i++) {
      /* THIS WAS WRONG!! && or || meant foR , ?? */
      current = current->next;
    }

    return current;
}

//int csoundGetVariableIndex(CS_VAR_POOL* pool, CS_VARIABLE* var) {
//    CS_VARIABLE* current = pool->head;
//    int32_t index = 0;
//
//    if (current == NULL) {
//        return -1;
//    }
//
//    for (index = 0; current != NULL; index++) {
//        if (current == var) {
//            return index;
//        }
//    }
//    return -1;
//}

int32_t csoundAddVariable(CSOUND* csound, CS_VAR_POOL* pool, CS_VARIABLE* var) {
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
    var->memBlockIndex = (pool->poolSize / sizeof(MYFLT)) +
      ((pool->varCount + 1) * (CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET) / sizeof(MYFLT)));
    pool->poolSize += var->memBlockSize;
    pool->varCount += 1;
    return 0;
  } else return -1;
}

void recalculateVarPoolMemory(CSOUND* csound, CS_VAR_POOL* pool)
{
    CS_VARIABLE* current = pool->head;
    int32_t varCount = 1;
    pool->poolSize = 0;

    while (current != NULL) {
      /* VL 26-12-12: had to revert these lines to avoid memory crashes
         with higher ksmps */
      if(current->updateMemBlockSize != NULL) {
        current->updateMemBlockSize(csound, current);
      }

      current->memBlockIndex = (pool->poolSize / sizeof(MYFLT)) +
        (varCount * CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET) / sizeof(MYFLT));
      pool->poolSize += current->memBlockSize;

      current = current->next;
      varCount++;
    }
}

void reallocateVarPoolMemory(CSOUND* csound, CS_VAR_POOL* pool) {
    CS_VARIABLE* current = pool->head;
    CS_VAR_MEM* varMem = NULL;
    size_t memSize;
    pool->poolSize = 0;

    while (current != NULL) {
      varMem = current->memBlock;
      memSize = current->memBlockSize;

      if(current->updateMemBlockSize != NULL) {
        current->updateMemBlockSize(csound, current);
      }
      // VL 14-3-2015 only realloc if we need to
      if(memSize < (size_t)current->memBlockSize) {
          memSize = CS_VAR_TYPE_OFFSET + current->memBlockSize;
          varMem =
               (CS_VAR_MEM *)((CSOUND *)csound)->ReAlloc(csound,varMem,
                                             memSize);
          current->memBlock = varMem;
       }
       pool->poolSize += current->memBlockSize;
       current = current->next;
    }
}

void deleteVarPoolMemory(CSOUND* csound, CS_VAR_POOL* pool) {
    CS_VARIABLE* current = pool->head, *tmp;
    const CS_TYPE* type;
    while (current != NULL) {
      tmp = current;
      type = current->subType;
      if (type->freeVariableMemory != NULL) {
        type->freeVariableMemory(csound, current->memBlock);
      }
      csound->Free(csound, current->memBlock);
      current = current->next;
      csound->Free(csound, tmp);
    }
}



void initializeVarPool(CSOUND* csound, MYFLT* memBlock, CS_VAR_POOL* pool) {
    CS_VARIABLE* current = pool->head;
    //int varNum = 1;

    while (current != NULL) {
      if (current->initializeVariableMemory != NULL) {
        current->initializeVariableMemory(csound, current,
                                          memBlock + current->memBlockIndex);
      }
      //varNum++;
      current = current->next;
    }
}

void debug_print_varpool(CSOUND* csound, CS_VAR_POOL* pool) {
    CS_VARIABLE* gVar = pool->head;
    int32_t count = 0;
    while(gVar != NULL) {
      csound->Message(csound, "  %d) %s:%s\n", count++,
                      gVar->varName, gVar->varType->varTypeName);
      gVar = gVar->next;
    }
}

int32_t copyVarNoOp(CSOUND *csound, void *p) {
  return OK;
}

/* GENERIC VARIABLE COPYING */
int32_t copyVarGeneric(CSOUND *csound, void *p) {
    ASSIGN* assign = (ASSIGN*)p;
    CS_TYPE* typeR = csoundGetTypeForArg(assign->r);
    CS_TYPE* typeA = csoundGetTypeForArg(assign->a);

    if(typeR != typeA) {
      if(assign->h.perf != copyVarNoOp)
        return csound->PerfError(csound,&(assign->h),
        Str("Opcode given variables "
            "with two different types: %s : %s"), 
        typeR->varTypeName, typeA->varTypeName);
       else return csound->InitError(csound,
        Str("Opcode given variables "
            "with two different types: %s : %s"),
        typeR->varTypeName, typeA->varTypeName);
    }

    typeR->copyValue(csound, typeR, assign->r, assign->a, &(assign->h));
    return OK;
}



#include "csound_standard_types.h"

int32_t copyVarGenericInit(CSOUND *csound, void *p) {
    ASSIGN* assign = (ASSIGN*)p;
    int32_t flag = 0;
    CS_TYPE* type = csoundGetTypeForArg(assign->a);

    if(type == &CS_VAR_TYPE_ARRAY) {
      ARRAYDAT* adat = (ARRAYDAT*) assign->a;
      if(adat->arrayType == &CS_VAR_TYPE_I) flag = 1;
    } else if(type == &CS_VAR_TYPE_I) flag = 1;

    if (flag) {
      assign->h.perf = copyVarNoOp;
      copyVarGeneric(csound, p);
    }
    return OK;
}


