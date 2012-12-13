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
  csoundReset(csound);
  //    csoundAddStandardTypes(csound, pool);
  
  TYPE_POOL* pool = csound->typePool;
  CS_VAR_POOL* varPool = csound->varPool;
  
  CS_VARIABLE* var = csoundCreateVariable(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_A, "a1");
  CU_ASSERT_PTR_NOT_NULL(var);
  //printf("Var type created: %s\n", var->varType->varTypeName);
  
  csoundAddVariable(varPool, var);
  
  CS_VARIABLE* var2 = csoundFindVariableWithName(varPool, "a1");
  CU_ASSERT_PTR_NOT_NULL(var2);
  CU_ASSERT_STRING_EQUAL(var2->varType->varTypeName, "a");
  CU_ASSERT_STRING_EQUAL(var2->varName, "a1");
  
  CU_ASSERT_PTR_NULL(csoundFindVariableWithName(varPool, "a2"));
  
}


int main()
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
      //           || (NULL == CU_add_test(pSuite, "test of fread()", testFREAD))
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

