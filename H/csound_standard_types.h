/* 
 * File:   csound_standard_types.h
 * Author: stevenyi
 *
 * Created on June 8, 2012, 1:48 PM
 */

#ifndef CSOUND_STANDARD_TYPES_H
#define	CSOUND_STANDARD_TYPES_H

#include "csound_type_system.h"
#include "csound.h"

#ifdef	__cplusplus
extern "C" {
#endif

    PUBLIC void csoundAddStandardTypes(CSOUND* csound, TYPE_POOL* pool);

    extern const CS_TYPE CS_VAR_TYPE_A;
    extern const CS_TYPE CS_VAR_TYPE_K;
    extern const CS_TYPE CS_VAR_TYPE_I;
    extern const CS_TYPE CS_VAR_TYPE_P;
    extern const CS_TYPE CS_VAR_TYPE_R;
    extern const CS_TYPE CS_VAR_TYPE_C;
    extern const CS_TYPE CS_VAR_TYPE_W;
    extern const CS_TYPE CS_VAR_TYPE_F;
    extern const CS_TYPE CS_VAR_TYPE_B;
    extern const CS_TYPE CS_VAR_TYPE_b;
    extern const CS_TYPE CS_VAR_TYPE_ARRAY;
    
    typedef struct arrayVarInit {
        int dimensions;
        CS_TYPE* type;
    } ARRAY_VAR_INIT;

#ifdef	__cplusplus
}
#endif

#endif	/* CSOUND_STANDARD_TYPES_H */

