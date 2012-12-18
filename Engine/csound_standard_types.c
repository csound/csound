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

const CS_TYPE CS_VAR_TYPE_A = {
  "a", NULL, "audio rate vector", CS_ARG_TYPE_BOTH, createAsig, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_K = {
  "k", NULL, "control rate var", CS_ARG_TYPE_BOTH, createMyflt, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_I = {
  "i", NULL, "init time var", CS_ARG_TYPE_BOTH, createMyflt, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_P = {
  "p", NULL, "p-field", CS_ARG_TYPE_BOTH, createMyflt, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_R = {
  "r", NULL, "reserved symbol", CS_ARG_TYPE_BOTH, createMyflt, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_C = {
  "c", NULL, "constant", CS_ARG_TYPE_IN, createMyflt, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_W = {
  "w", NULL, "spectral", CS_ARG_TYPE_BOTH, createWsig, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_F = {
  "f", NULL, "f-sig", CS_ARG_TYPE_BOTH, createFsig, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_B = {
  "B", NULL, "boolean", CS_ARG_TYPE_BOTH, createBool, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_b = {
  "b", NULL, "boolean", CS_ARG_TYPE_BOTH, createBool, NULL, NULL
};

void csoundAddStandardTypes(CSOUND* csound, TYPE_POOL* pool) {

    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_A);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_K);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_I);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_P);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_R);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_C);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_W);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_F);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_B);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_b);
}
