/* 
 * File:   csound_type_system.h
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:04 PM
 */

#ifndef CSOUND_TYPE_SYSTEM_H
#define	CSOUND_TYPE_SYSTEM_H

#include "csoundCore.h"

#ifdef	__cplusplus
extern "C" {
#endif

    typedef struct cstype {
        char* varTypeName;
        char* varMemberName; /* Used when member of aggregate type */
        char* varDescription;
        struct cstype* members;
    } CS_TYPE;

    typedef struct csvariable {
        char* varName;
        CS_TYPE* varType;
        void* memblock;
        int refCount;
    } CS_VARIABLE;

    typedef struct cstypeinstance {
        CS_TYPE* varType;
        CS_VARIABLE* (*createVariable)(CSOUND*, void*);
        void* args ;
        struct cstypeinstance* next;
    } CS_TYPE_INSTANCE;
    
    typedef struct typepool {
        CS_TYPE_INSTANCE* head;
    } TYPE_POOL;

    /* Adds a new type to Csound's type table
       Returns if variable type redefined */
    int csoundAddVariableType(TYPE_POOL* pool, CS_TYPE_INSTANCE* typeInstance);
    CS_VARIABLE* csoundCreateVariableWithType(CSOUND* csound, TYPE_POOL* pool, CS_TYPE* type);
    CS_TYPE* csoundGetTypeWithVarTypeName(TYPE_POOL* pool, char* typeName);
    
    
    /* Csound Variable Pool - essentially a map<string,csvar> 
       CSOUND contains one for global memory, InstrDef and UDODef
       contain a pool for local memory
     */

    typedef struct csvarpool {
        CS_VARIABLE* head;
    } CS_VAR_POOL;



#ifdef	__cplusplus
}
#endif

#endif	/* CSOUND_TYPE_SYSTEM_H */

