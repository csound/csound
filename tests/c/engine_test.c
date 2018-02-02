#include "csound.h"
#include <stdio.h>
#include <CUnit/Basic.h>

#include "time.h"

int init_suite1(void)
{
    return 0;
}

int clean_suite1(void)
{
    return 0;
}

void test_daemon(void)
{
    CSOUND  *csound;
    csound = csoundCreate(NULL);
    csoundSetIsGraphable(csound, 1);
    csoundSetOption(csound,"-odac");
    csoundSetOption(csound,"--daemon");
    csoundStart(csound);
    csoundSleep(1000);
    /* delete Csound instance */
    csoundStop(csound);
    csoundDestroy(csound);
}

void test_eval_code(void)
{
    CSOUND  *csound;
    MYFLT res;
    csound = csoundCreate(NULL);
    res = csoundEvalCode(csound, "i1 init 1 \n"
			 "print i1 \n"
			 "return i1 \n");
    CU_ASSERT_EQUAL(res, 1.0);
    csoundDestroy(csound);
}

void test_compile_async(void)
{
    CSOUND  *csound;
    csound = csoundCreate(NULL);
    csoundStart(csound);
    csoundCompileOrcAsync(csound, "instr 1\n"
		               "i1 = 1 \n"
		                "print i1 \n"
                                "endin\n"
		                "schedule 1,0,1");
    csoundPerformBuffer(csound);
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
    if ((NULL == CU_add_test(pSuite, "Test daemon mode", test_daemon))
        || (NULL == CU_add_test(pSuite, "Test evalcode", test_eval_code))
	|| (NULL == CU_add_test(pSuite, "Test compileAsync", test_compile_async)) 
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

