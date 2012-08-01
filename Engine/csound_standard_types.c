#include "csoundCore.h"
#include "csound_standard_types.h" 
#include "pstream.h"
#include <stdlib.h>

#define Wfloats (((int) sizeof(SPECDAT) + 7) / (int) sizeof(MYFLT))
#define Pfloats (((int) sizeof(PVSDAT) + 7) / (int) sizeof(MYFLT))


CS_TYPE* createTypeInstance(CSOUND* csound, char* varTypeName,  char* varDescription,
        CS_VARIABLE* (*createVarFunc)(void*, void*), void* args, int argtype) {
    CS_TYPE* type = mcalloc(csound, sizeof (CS_TYPE));
    type->varTypeName = varTypeName;
    type->varDescription = varDescription;
    type->argtype = argtype;    
    type->createVariable = createVarFunc;    
    type->args = args;

    return type;
}

void updateAsigMemBlock(void* csound, CS_VARIABLE* var) {
    CSOUND* cs = (CSOUND*)csound;
    int ksmps = cs->ksmps;    
    var->memBlockSize = ksmps * sizeof (MYFLT);
}

CS_VARIABLE* createAsig(void* csound, void* args) {
    CSOUND* cs = (CSOUND*)csound;
    int ksmps = cs->ksmps;
    CS_VARIABLE* var = mcalloc(cs, sizeof (CS_VARIABLE));
    var->memBlockSize = ksmps * sizeof (MYFLT);
    var->updateMemBlockSize = &updateAsigMemBlock;
    return var;
}

CS_VARIABLE* createMyflt(void* csound, void* args) {
    CSOUND* cs = (CSOUND*)csound;    
    CS_VARIABLE* var = mcalloc(cs, sizeof (CS_VARIABLE));
    var->memBlockSize = sizeof (MYFLT);
    return var;
}

CS_VARIABLE* createBool(void* csound, void* args) {
    CSOUND* cs = (CSOUND*)csound;    
    CS_VARIABLE* var = mcalloc(cs, sizeof (CS_VARIABLE));
    var->memBlockSize = sizeof (MYFLT);
    return var;
}

CS_VARIABLE* createWsig(void* csound, void* args) {
    CSOUND* cs = (CSOUND*)csound;    
    CS_VARIABLE* var = mcalloc(cs, sizeof (CS_VARIABLE));
    var->memBlockSize = Wfloats;
    return var;
}

CS_VARIABLE* createFsig(void* csound, void* args) {
    CSOUND* cs = (CSOUND*)csound;    
    CS_VARIABLE* var = mcalloc(cs, sizeof (CS_VARIABLE));
    var->memBlockSize = Pfloats;
    return var;
}

//#define ARGTYP_S        0x00000040L     /* string constant or variable */
//#define ARGTYP_l        0x00000800L     /* label */

void csoundAddStandardTypes(CSOUND* csound, TYPE_POOL* pool) {

    csoundAddVariableType(pool, 
                          createTypeInstance(csound, "a", "audio rate vector", &createAsig, NULL, CS_ARG_TYPE_BOTH));
    csoundAddVariableType(pool, 
                          createTypeInstance(csound, "k", "control rate var", &createMyflt, NULL, CS_ARG_TYPE_BOTH));
    csoundAddVariableType(pool, 
                          createTypeInstance(csound, "i", "init time var", &createMyflt, NULL, CS_ARG_TYPE_BOTH));
    csoundAddVariableType(pool, 
                          createTypeInstance(csound, "p", "p-field", &createMyflt, NULL,CS_ARG_TYPE_BOTH));
    csoundAddVariableType(pool, 
                          createTypeInstance(csound, "r", "reserved symbol", &createMyflt, NULL, CS_ARG_TYPE_BOTH));
    csoundAddVariableType(pool, 
                          createTypeInstance(csound, "c", "constant", &createMyflt, NULL, CS_ARG_TYPE_IN));
    csoundAddVariableType(pool, 
                          createTypeInstance(csound, "w", "constant", &createWsig, NULL, CS_ARG_TYPE_BOTH));
    csoundAddVariableType(pool, 
                          createTypeInstance(csound, "f", "constant", &createFsig, NULL, CS_ARG_TYPE_BOTH));
    csoundAddVariableType(pool, 
                          createTypeInstance(csound, "B", "constant", &createBool, NULL, CS_ARG_TYPE_BOTH));
    csoundAddVariableType(pool, 
                          createTypeInstance(csound, "b", "constant", &createBool, NULL, CS_ARG_TYPE_BOTH));
}
