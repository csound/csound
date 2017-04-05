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

void test_udp_server(void)
{
    CSOUND  *csound;
    int     result, compile_again=0;

    csound = csoundCreate(NULL);
    csoundSetIsGraphable(csound, 1);
    csoundSetOption(csound,"-odac");
    csoundSetOption(csound,"--port=12345");
    result = csoundStart(csound);
    sleep(1);
    /* delete Csound instance */
    csoundStop(csound);
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
    if ((NULL == CU_add_test(pSuite, "Test UDP Server", test_udp_server))
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

