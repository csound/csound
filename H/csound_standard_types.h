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

    PUBLIC extern const CS_TYPE CS_VAR_TYPE_A;
    PUBLIC extern const CS_TYPE CS_VAR_TYPE_K;
    PUBLIC extern const CS_TYPE CS_VAR_TYPE_I;
    PUBLIC extern const CS_TYPE CS_VAR_TYPE_S;
    PUBLIC extern const CS_TYPE CS_VAR_TYPE_P;
    PUBLIC extern const CS_TYPE CS_VAR_TYPE_R;
    PUBLIC extern const CS_TYPE CS_VAR_TYPE_C;
    PUBLIC extern const CS_TYPE CS_VAR_TYPE_W;
    PUBLIC extern const CS_TYPE CS_VAR_TYPE_F;
    PUBLIC extern const CS_TYPE CS_VAR_TYPE_B;
    PUBLIC extern const CS_TYPE CS_VAR_TYPE_b;
    PUBLIC extern const CS_TYPE CS_VAR_TYPE_ARRAY;
    
    typedef struct arrayVarInit {
        int dimensions;
        CS_TYPE* type;
    } ARRAY_VAR_INIT;

#ifdef	__cplusplus
}
#endif

#endif	/* CSOUND_STANDARD_TYPES_H */

