#include "csoundCore.h"
#include "csound_standard_types.h" 
#include <stdlib.h>

CS_TYPE_INSTANCE* createTypeInstance(char* varTypeName,  char* varDescription,
        CS_VARIABLE* (*createVarFunc)(void*, void*), void* args) {
    CS_TYPE* type = malloc(sizeof (CS_TYPE));
    type->varTypeName = varTypeName;
    type->varDescription = varDescription;

    CS_TYPE_INSTANCE* typeInstance = malloc(sizeof (CS_TYPE_INSTANCE));
    typeInstance->varType = type;
    typeInstance->args = args;
    typeInstance->createVariable = createVarFunc;

    return typeInstance;
}

CS_VARIABLE* createAsig(void* csound, void* args) {
    int ksmps = ((CSOUND*)csound)->ksmps;
    CS_VARIABLE* var = malloc(sizeof (CS_VARIABLE));
    var->memBlockSize = ksmps * sizeof (MYFLT);
    /* var->memblock = malloc(var->memBlockSize); */
    return var;
}

void csoundAddStandardTypes(TYPE_POOL* pool) {

    CS_TYPE_INSTANCE* typeInstance = createTypeInstance("a", "a-sig", &createAsig, NULL);
    csoundAddVariableType(pool, typeInstance);

}
