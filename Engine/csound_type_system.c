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

CS_VARIABLE* csoundCreateVariableWithType(void* csound, TYPE_POOL* pool, CS_TYPE* type) {
    CS_TYPE_INSTANCE* current = pool->head;
    while (current != NULL) {
        if (strcmp(type->varTypeName, current->varType->varTypeName) == 0) {
            CS_VARIABLE* var = current->createVariable(csound, current->args);
            var->varType = type;
            return var;
        }
        current = current->next;
    }
    return NULL;
}