/* 
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "csound.h"

/*
 * 
 */
int main(int argc, char** argv) {

    TYPE_POOL* pool = malloc(sizeof(TYPE_POOL));
    CS_VAR_POOL* varPool = malloc(sizeof(CS_VAR_POOL));
    
    CSOUND* csound = csoundCreate(NULL);
    
    csoundAddStandardTypes(pool);
   
    CS_TYPE* aType = csoundGetTypeWithVarTypeName(pool, "a");
    CS_VARIABLE* var = csoundCreateVariable(csound, pool, aType, "a1");
    printf("Var type created: %s\n", var->varType->varTypeName);
    
    csoundAddVariable(varPool, var);

    CS_VARIABLE* var2 = csoundFindVariableWithName(varPool, "a1");
    printf("Var2 found: %s : %s\n", var2->varType->varTypeName, var2->varName);
    
    printf("Testing var name a2 does not exist: %d\n", (csoundFindVariableWithName(varPool, "a2") == NULL));
    
    return (EXIT_SUCCESS);
}

