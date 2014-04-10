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
#include "CUnit/Basic.h"


int init_suite1(void)
{
  return 0;
}

int clean_suite1(void)
{
  return 0;
}

void test_type_system(void)
{
  CSOUND* csound = csoundCreate(NULL);
  //    csoundAddStandardTypes(csound, pool);
  
  TYPE_POOL* pool = csound->typePool;
  CS_VAR_POOL* varPool = csound->engineState.varPool;
  
  CS_VARIABLE* var = csoundCreateVariable(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_A, "a1", NULL);
  CU_ASSERT_PTR_NOT_NULL(var);
  //printf("Var type created: %s\n", var->varType->varTypeName);
  
  csoundAddVariable(csound, varPool, var);
  
  CS_VARIABLE* var2 = csoundFindVariableWithName(csound, varPool, "a1");
  CU_ASSERT_PTR_NOT_NULL(var2);
  CU_ASSERT_STRING_EQUAL(var2->varType->varTypeName, "a");
  CU_ASSERT_STRING_EQUAL(var2->varName, "a1");
  
  CU_ASSERT_PTR_NULL(csoundFindVariableWithName(csound, varPool, "a2"));
  
}

void test_get_var_simple_name(void) {
    CSOUND* csound = csoundCreate(NULL);

    CU_ASSERT_STRING_EQUAL("a1", getVarSimpleName(csound, "a1"));
    CU_ASSERT_STRING_EQUAL("a1", getVarSimpleName(csound, "[a]1"));
    CU_ASSERT_STRING_EQUAL("StestString", getVarSimpleName(csound, "StestString"));
    CU_ASSERT_STRING_EQUAL("StestString", getVarSimpleName(csound, "[S]testString"));
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

int main(int argc, char** argv)
{
  CU_pSuite pSuite = NULL;
  
  /* initialize the CUnit test registry */
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();
  
  /* add a suite to the registry */
  pSuite = CU_add_suite("Type System Tests", init_suite1, clean_suite1);
  if (NULL == pSuite) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  
  /* add the tests to the suite */
  if ((NULL == CU_add_test(pSuite, "Test Type System", test_type_system))
//        || (NULL == CU_add_test(pSuite, "test of array name variable clashing", test_array_name_variable_clashing))
        || (NULL == CU_add_test(pSuite, "Test getVarSimpleName", test_get_var_simple_name))
      )
  {
    CU_cleanup_registry();
    return CU_get_error();
  }
  
  /* Run all tests using the CUnit Basic interface */
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}

