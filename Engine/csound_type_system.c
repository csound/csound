#include "csound_type_system.h"
#include <string.h> 
#include <stdio.h>

int csTypeExistsWithSameName(TYPE_POOL* pool, CS_TYPE_INSTANCE* typeInstance) {
    CS_TYPE_INSTANCE* current = pool->head;
    while (current != NULL) {

        /*
                printf("Search if type [%s] == [%s]", current->varType->varTypeName, typeInstance->varType->varTypeName);
         */

        if (strcmp(current->varType->varTypeName,
                typeInstance->varType->varTypeName) == 0) {
            return 1;
        }
        current = current->next;
    }

    return 0;
}

CS_TYPE* csoundGetTypeWithVarTypeName(TYPE_POOL* pool, char* typeName) {
    CS_TYPE_INSTANCE* current = pool->head;
    while (current != NULL) {
        CS_TYPE* type = current->varType;
        if (strcmp(typeName, type->varTypeName) == 0) {
            return type;
        }
        current = current->next;
    }
    return NULL;
}

int csoundAddVariableType(TYPE_POOL* pool, CS_TYPE_INSTANCE * typeInstance) {
    if (csTypeExistsWithSameName(pool, typeInstance)) {
        return 0;
    }

    if (pool->head == NULL) {
        pool->head = typeInstance;
    } else {
        CS_TYPE_INSTANCE* current = pool->head;
        while (current->next) {
            current = current->next;
        }
        current->next = typeInstance;
        typeInstance->next = NULL;
    }
    
    printf("Adding type with type name: %s\n", typeInstance->varType->varTypeName);


    return 1;
}

CS_VARIABLE* csoundCreateVariable(void* csound, TYPE_POOL* pool, CS_TYPE* type, char* name) {
    CS_TYPE_INSTANCE* current = pool->head;
    while (current != NULL) {
        if (strcmp(type->varTypeName, current->varType->varTypeName) == 0) {
            CS_VARIABLE* var = current->createVariable(csound, current->args);
            var->varType = type;
            var->varName = name;
            return var;
        }
        current = current->next;
    }
    return NULL;
}

CS_VARIABLE* csoundFindVariableWithName(CS_VAR_POOL* pool, char* name) {
    
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
    
    return 0;
}