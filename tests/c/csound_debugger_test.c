/*
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#include "csound.h"
#include "pthread.h"
#include "csdebug.h"
#include "CUnit/Basic.h"


int init_suite1(void)
{
    return 0;
}

int clean_suite1(void)
{
    return 0;
}

void test_debugger_init(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundDebuggerInit(csound);
    csoundDebuggerClean(csound);
    csoundDestroy(csound);
}

void test_add_bkpt(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundDebuggerInit(csound);
    csoundSetBreakpoint(csound, 3);
    csoundSetBreakpoint(csound, 5);
    csoundClearBreakpoints(csound);
    csoundDebuggerClean(csound);
    csoundDestroy(csound);
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
    if ((NULL == CU_add_test(pSuite, "Test debugger init", test_debugger_init))
            || (NULL == CU_add_test(pSuite, "Test add breakpoint", test_add_bkpt))
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

