/*
 * File:   csound_debugger_test.c
 * Author: mantaraya36
 */

#include <stdio.h>

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
    csoundSetInstrumentBreakpoint(csound, 3.4);
    csoundSetInstrumentBreakpoint(csound, 1.1);
    csoundClearBreakpoints(csound);
    csoundDebuggerClean(csound);
    csoundDestroy(csound);
}

static void brkpt_cb(CSOUND *csound, int line, double instr, void *userdata)
{
//    INSDS *i = csoundDebugGetInstrument(csound);
    int *count = (int *) userdata;
    printf("bkpt line %i instr %f\n", line, instr);
    *count = *count + 1;
    csoundRemoveInstrumentBreakpoint(csound, instr);
    csoundDebugContinue(csound);
}

void test_add_callback(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb, NULL);
    csoundDebuggerClean(csound);
    csoundDestroy(csound);
}

void test_breakpoint(void)
{
    int i;
    int break_count = 0;
    CSOUND* csound = csoundCreate(NULL);
    csoundCompileOrc(csound, "instr 1\nasig oscil 1, p4\nendin\n");
    csoundInputMessage(csound, "i 1.1 0   1 440");
    csoundInputMessage(csound, "i 1.2 0   1 880");
    csoundInputMessage(csound, "i 1.1 0.1 1 440");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb, (void *) &break_count);
    csoundSetInstrumentBreakpoint(csound, 1.1);
    csoundSetInstrumentBreakpoint(csound, 1.2);

    for (i = 0; i < 1000; i++) {
        csoundPerformKsmps(csound);
    }
    CU_ASSERT(break_count == 2);

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
            || (NULL == CU_add_test(pSuite, "Test add callback", test_add_callback))
            || (NULL == CU_add_test(pSuite, "Test breakpoint", test_breakpoint))
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

