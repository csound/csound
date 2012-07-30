/* 
 * File:   csound_type_system.h
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:04 PM
 */

#ifndef CSOUND_TYPE_SYSTEM_H
#define	CSOUND_TYPE_SYSTEM_H

#ifdef	__cplusplus
extern "C" {
#endif

#define CS_ARG_TYPE_BOTH 0
#define CS_ARG_TYPE_IN 1    
#define CS_ARG_TYPE_OUT 2
    
    struct csvariable;
    
    typedef struct cstype {
        char* varTypeName;
        char* varMemberName; /* Used when member of aggregate type */
        char* varDescription;
        int argtype; // used to denote if allowed as in-arg, out-arg, or both        
        struct csvariable* (*createVariable)(void*, void*);
        void* args ; /* arg for createVariable */
        struct cstype* members;        
        struct cstype* next;        
    } CS_TYPE;

    typedef struct csvariable {
        char* varName;
        CS_TYPE* varType;
        int memBlockSize;
        int memBlockIndex;
        /* void* memblock; */
        int refCount;
        struct csvariable* next;
    } CS_VARIABLE;

//    typedef struct cstypeinstance {
//        CS_TYPE* varType;
//        CS_VARIABLE* (*createVariable)(void*, void*);
//        void* args ;
//        struct cstypeinstance* next;
//    } CS_TYPE_INSTANCE;
    
    typedef struct typepool {
        CS_TYPE* head;
    } TYPE_POOL;

    /* Adds a new type to Csound's type table
       Returns if variable type redefined */
    int csoundAddVariableType(TYPE_POOL* pool, CS_TYPE* typeInstance);
    CS_VARIABLE* csoundCreateVariable(void* csound, TYPE_POOL* pool, CS_TYPE* type, char* name);
    CS_TYPE* csoundGetTypeWithVarTypeName(TYPE_POOL* pool, char* typeName);
    
    
    /* Csound Variable Pool - essentially a map<string,csvar> 
       CSOUND contains one for global memory, InstrDef and UDODef
       contain a pool for local memory
     */

    typedef struct csvarpool {
        CS_VARIABLE* head;
        int poolSize;
    } CS_VAR_POOL;

    CS_VARIABLE* csoundFindVariableWithName(CS_VAR_POOL* pool, const char* name);
    int csoundFindVariable(CS_VAR_POOL* pool, const char* name);
    int csoundAddVariable(CS_VAR_POOL* pool, CS_VARIABLE* var);
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* CSOUND_TYPE_SYSTEM_H */

