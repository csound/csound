#include "csound_type_system.h"
#include <string.h> 
#include <stdio.h>
#include "csoundCore.h"

int csTypeExistsWithSameName(TYPE_POOL* pool, CS_TYPE* typeInstance) {
    CS_TYPE_ITEM* current = pool->head;
    while (current != NULL) {
        
        /* printf("Search if type [%s] == [%s]", current->varTypeName, typeInstance->varTypeName); */

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

int csoundAddVariableType(CSOUND* csound, TYPE_POOL* pool, CS_TYPE* typeInstance) {
    if (csTypeExistsWithSameName(pool, typeInstance)) {
        return 0;
    }
  
    CS_TYPE_ITEM* item = mcalloc(csound, sizeof(CS_TYPE_ITEM));
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

CS_VARIABLE* csoundCreateVariable(void* csound, TYPE_POOL* pool, CS_TYPE* type, char* name, void* typeArg) {
    CS_TYPE_ITEM* current = pool->head;
    while (current != NULL) {
        if (strcmp(type->varTypeName, current->cstype->varTypeName) == 0) {
            CS_VARIABLE* var = current->cstype->createVariable(csound, typeArg);
            var->varType = type;
            var->varName = name;
            return var;
        }
        current = current->next;
    }
    return NULL;
}

CS_VARIABLE* csoundFindVariableWithName(CS_VAR_POOL* pool, const char* name) {
    
    CS_VARIABLE* current = pool->head;
    CS_VARIABLE* returnValue = NULL;
    
    if(current != NULL && name != NULL) {
        while(current != NULL) {
            if (strcmp(current->varName, name) == 0) {
                returnValue = current;
                break;
            }
            current = current->next;
        }
    }
    
    return returnValue;
}

CS_VARIABLE* csoundGetVariable(CS_VAR_POOL* pool, int index) {

    CS_VARIABLE* current = pool->head;
    int i;

    for(i = 0; i < index || current != NULL; i++) { /* THIS WAS WRONG!! && or || meant foR , ?? */
        current = current->next;
    }

    return current;
}

int csoundFindVariable(CS_VAR_POOL* pool, const char* name) {
	CS_VARIABLE* current = pool->head;
	int returnValue = -1;
	int counter = 0;

	if(current != NULL && name != NULL) {
		while(current != NULL) {
			if (strcmp(current->varName, name) == 0) {
				returnValue = counter;
				break;
			}
			current = current->next;
			counter++;
		}
	}

	return returnValue;
}


int csoundAddVariable(CS_VAR_POOL* pool, CS_VARIABLE* var) {
    if(pool->head == NULL) {
        pool->head = var;
    } else {
        CS_VARIABLE* varCurrent = pool->head;
        while(varCurrent->next != NULL) { 
            varCurrent = varCurrent->next; 
        }
        varCurrent->next = var;
    }
    // may need to revise this; var pools are accessed as MYFLT*, so need to ensure all
    // memory is aligned to sizeof(MYFLT) boundaries
    var->memBlockIndex = pool->poolSize / sizeof(MYFLT);
    pool->poolSize += var->memBlockSize;
    
    return 0;
}

void recalculateVarPoolMemory(void* csound, CS_VAR_POOL* pool) {
    CS_VARIABLE* current = pool->head;
    pool->poolSize = 0;
    
	while (current != NULL) {
	  /* VL 26-12-12: had to revert these lines to avoid memory crashes with higher ksmps */
        if(current->updateMemBlockSize != NULL) {
            current->updateMemBlockSize(csound, current);
        }

        current->memBlockIndex = pool->poolSize / sizeof(MYFLT);
        pool->poolSize += current->memBlockSize;
        
        current = current->next;
    }
}

void initializeVarPool(MYFLT* memBlock, CS_VAR_POOL* pool) {
    CS_VARIABLE* current = pool->head;

    while(current != NULL) {
        if(current->initializeVariableMemory != NULL) {
            current->initializeVariableMemory(current, memBlock + current->memBlockIndex);
        }
        current = current->next;
    }
    
}
