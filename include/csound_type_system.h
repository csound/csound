/*
 * File:   csound_type_system.h
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:04 PM
 */

#ifndef CSOUND_TYPE_SYSTEM_H
#define CSOUND_TYPE_SYSTEM_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "csound.h"

#define CS_ARG_TYPE_BOTH 0
#define CS_ARG_TYPE_IN 1
#define CS_ARG_TYPE_OUT 2

    struct csvariable;

    typedef struct cstype {
        char* varTypeName;
        char* varDescription;
        int argtype; // used to denote if allowed as in-arg, out-arg, or both
        struct csvariable* (*createVariable)(void*, void*);
        struct cstype** unionTypes;
    } CS_TYPE;

    typedef struct csvariable {
        char* varName;
        char* varSimpleName; // reduced from array name
        CS_TYPE* varType;
        int memBlockSize;
        int memBlockIndex;
        int dimensions;  // used by arrays
        int refCount;
        struct csvariable* next;
        CS_TYPE* subType;
        void (*updateMemBlockSize)(void*, struct csvariable*);
        void (*initializeVariableMemory)(struct csvariable*, MYFLT*);
        void *memBlock;
    } CS_VARIABLE;

//    typedef struct cstypeinstance {
//        CS_TYPE* varType;
//        CS_VARIABLE* (*createVariable)(void*, void*);
//        void* args ;
//        struct cstypeinstance* next;
//    } CS_TYPE_INSTANCE;

    typedef struct cstypeitem {
      CS_TYPE* cstype;
      struct cstypeitem* next;
    } CS_TYPE_ITEM;

    typedef struct typepool {
        CS_TYPE_ITEM* head;
    } TYPE_POOL;

    /* Adds a new type to Csound's type table
       Returns if variable type redefined */
    PUBLIC int csoundAddVariableType(CSOUND* csound, TYPE_POOL* pool, CS_TYPE* typeInstance);
    PUBLIC CS_VARIABLE* csoundCreateVariable(void* csound, TYPE_POOL* pool, CS_TYPE* type, char* name, void* typeArg);
    PUBLIC CS_TYPE* csoundGetTypeWithVarTypeName(TYPE_POOL* pool, char* typeName);
    PUBLIC CS_TYPE* csoundGetTypeForVarName(TYPE_POOL* pool, char* typeName);


    /* Csound Variable Pool - essentially a map<string,csvar>
       CSOUND contains one for global memory, InstrDef and UDODef
       contain a pool for local memory
     */

    typedef struct csvarpool {
        CS_VARIABLE* head;
        int poolSize;
    } CS_VAR_POOL;

    PUBLIC char* getVarSimpleName(CSOUND* csound, const char* name);
    PUBLIC CS_VARIABLE* csoundFindVariableWithName(CS_VAR_POOL* pool, const char* name);
    PUBLIC int csoundFindVariable(CS_VAR_POOL* pool, const char* name);
    PUBLIC int csoundAddVariable(CS_VAR_POOL* pool, CS_VARIABLE* var);
    PUBLIC void recalculateVarPoolMemory(void* csound, CS_VAR_POOL* pool);
    PUBLIC void initializeVarPool(MYFLT* memBlock, CS_VAR_POOL* pool);

#ifdef  __cplusplus
}
#endif

#endif  /* CSOUND_TYPE_SYSTEM_H */
