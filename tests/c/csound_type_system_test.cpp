/* 
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#define __BUILDING_LIBCSOUND

#include <stdio.h>
#include <stdlib.h>
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "csoundCore.h"
#include "gtest/gtest.h"

class TypeSystemTests : public ::testing::Test {
public:
    TypeSystemTests ()
    {
    }

    virtual ~TypeSystemTests ()
    {
    }

    virtual void SetUp ()
    {
        csoundSetGlobalEnv ("OPCODE6DIR64", "../../");
        csound = csoundCreate (0);
        csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundCleanup (csound);
        csoundDestroyMessageBuffer (csound);
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

TEST_F (TypeSystemTests, testTypeSystem)
{
  TYPE_POOL* pool = csound->typePool;
  CS_VAR_POOL* varPool = csound->engineState.varPool;
  
  CS_VARIABLE* var = csoundCreateVariable(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_A, "a1", NULL);
  ASSERT_TRUE (var != NULL);
  
  csoundAddVariable(csound, varPool, var);
  
  CS_VARIABLE* var2 = csoundFindVariableWithName(csound, varPool, "a1");
  ASSERT_TRUE (var2 != NULL);
  ASSERT_STREQ (var2->varType->varTypeName, "a");
  ASSERT_STREQ (var2->varName, "a1");
  
  ASSERT_TRUE (csoundFindVariableWithName(csound, varPool, "a2") == NULL);
}

TEST_F (TypeSystemTests, testGetVarSimpleName)
{
    ASSERT_STREQ ("a1", getVarSimpleName(csound, "a1"));
    ASSERT_STREQ ("a1", getVarSimpleName(csound, "[a]1"));
    ASSERT_STREQ ("StestString", getVarSimpleName(csound, "StestString"));
    ASSERT_STREQ ("StestString", getVarSimpleName(csound, "[S]testString"));
}

//void test_array_name_variable_clashing(void)
//{
//    CSOUND* csound = csoundCreate(NULL);
//    
//    TYPE_POOL* pool = csound->typePool;
//    CS_VAR_POOL* varPool = csound->engineState.varPool;
//
//    csoundAddStandardTypes(csound, pool);
//    
//    CS_VARIABLE* var = csoundCreateVariable(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_A, "a1", NULL);
//    CU_ASSERT_PTR_NOT_NULL(var);
//    //printf("Var type created: %s\n", var->varType->varTypeName);
//
//    csoundAddVariable(varPool, var);
//    
//    CS_VARIABLE* var2 = csoundFindVariableWithName(csound, varPool, "a1");
//    CU_ASSERT_PTR_EQUAL(var, var2);
//    // should return "a1", as "[a;1" is originally a1[]
//    var2 = csoundFindVariableWithName(csound, varPool, "[a;1");
//    CU_ASSERT_PTR_EQUAL(var, var2);
//    
//}
