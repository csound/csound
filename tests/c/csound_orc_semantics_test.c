/*
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#define __BUILDING_LIBCSOUND


#include <stdio.h>
#include <stdlib.h>
#include "csoundCore.h"
#include "CUnit/Basic.h"

extern char* convertArrayName(CSOUND* csound, char* arrayName);


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
    
    char* result = convertArrayName(csound, NULL);
    CU_ASSERT_PTR_NULL(result);

    result = convertArrayName(csound, "");
    CU_ASSERT_PTR_NULL(result);
    
    result = convertArrayName(csound, "aSignals");
    CU_ASSERT_STRING_EQUAL(result, "[aSignals");

    result = convertArrayName(csound, "gkSignals");
    CU_ASSERT_STRING_EQUAL(result, "g[kSignals");
    
    result = convertArrayName(csound, "g[kSignals");
    CU_ASSERT_STRING_EQUAL(result, "g[[kSignals");
}


int main()
{
    CU_pSuite pSuite = NULL;
    
    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();
    
    /* add a suite to the registry */
    pSuite = CU_add_suite("csound_orc_semantics function tests", init_suite1, clean_suite1);
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

