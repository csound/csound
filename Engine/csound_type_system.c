#include "csound_type_system.h"
#include <string.h> 
#include <stdio.h>
#include "csound.h"

int csTypeExistsWithSameName(TYPE_POOL* pool, CS_TYPE* typeInstance) {
    CS_TYPE* current = pool->head;
    while (current != NULL) {

        /*
                printf("Search if type [%s] == [%s]", current->varType->varTypeName, typeInstance->varType->varTypeName);
         */

        if (strcmp(current->varTypeName,
                typeInstance->varTypeName) == 0) {
            return 1;
        }
        current = current->next;
    }

    return 0;
}

CS_TYPE* csoundGetTypeWithVarTypeName(TYPE_POOL* pool, char* typeName) {
    CS_TYPE* current = pool->head;
    while (current != NULL) {
        if (strcmp(typeName, current->varTypeName) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int csoundAddVariableType(TYPE_POOL* pool, CS_TYPE* typeInstance) {
    if (csTypeExistsWithSameName(pool, typeInstance)) {
        return 0;
    }

    if (pool->head == NULL) {
        pool->head = typeInstance;
    } else {
        CS_TYPE* current = pool->head;
        while (current->next) {
            current = current->next;
        }
        current->next = typeInstance;
        typeInstance->next = NULL;
    }
    
    printf("Adding type with type name: %s\n", typeInstance->varTypeName);


    return 1;
}

CS_VARIABLE* csoundCreateVariable(void* csound, TYPE_POOL* pool, CS_TYPE* type, char* name) {
    CS_TYPE* current = pool->head;
    while (current != NULL) {
        if (strcmp(type->varTypeName, current->varTypeName) == 0) {
            CS_VARIABLE* var = current->createVariable(csound, current->args);
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

	for(i = 0; i < index, current != NULL; i++) {
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
