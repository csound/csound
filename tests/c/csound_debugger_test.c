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
    csoundSetBreakpoint(csound, 3, 0);
    csoundSetBreakpoint(csound, 5, 0);
    csoundSetInstrumentBreakpoint(csound, 3.4, 0);
    csoundSetInstrumentBreakpoint(csound, 1.1, 0);
    csoundClearBreakpoints(csound);
    csoundDebuggerClean(csound);
    csoundDestroy(csound);
}

static void brkpt_cb(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
    int *count = (int *) userdata;
    *count = *count + 1;
}

void test_add_callback(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb, NULL);
    csoundDebuggerClean(csound);
    csoundDestroy(csound);
}


void test_breakpoint_once(void)
{
    int i;
    int break_count = 0;
    CSOUND* csound = csoundCreate(NULL);
    csoundCompileOrc(csound, "instr 1\nasig oscil 1, p4\nendin\n");
    csoundInputMessage(csound, "i 1.1 0   1 440");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb, (void *) &break_count);
    csoundSetInstrumentBreakpoint(csound, 1.1, 0);

    for (i = 0; i < 1000; i++) {
        csoundPerformKsmps(csound);
    }
    CU_ASSERT(break_count == 1);

    csoundDebuggerClean(csound);
    csoundDestroy(csound);
}

static void brkpt_cb2(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
//    INSDS *i = csoundDebugGetInstrument(csound);
    int *count = (int *) userdata;
    *count = *count + 1;
    csoundRemoveInstrumentBreakpoint(csound, bkpt_info->breakpointInstr->p1);
    csoundDebugContinue(csound);
}

void test_breakpoint_remove(void)
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
    csoundSetBreakpointCallback(csound, brkpt_cb2, (void *) &break_count);
    csoundSetInstrumentBreakpoint(csound, 1.1, 0);
    csoundSetInstrumentBreakpoint(csound, 1.2, 0);

    for (i = 0; i < 1000; i++) {
        csoundPerformKsmps(csound);
    }
    CU_ASSERT(break_count == 2);

    csoundDebuggerClean(csound);
    csoundDestroy(csound);
}

static void brkpt_cb3(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
    debug_variable_t *vars = bkpt_info->instrVarList;

    CU_ASSERT_EQUAL(*((MYFLT *)vars->data), 2.5);
    CU_ASSERT(*((MYFLT *)vars->next->data) == 3.5);
    CU_ASSERT(*((MYFLT *)vars->next->next->data)== 0.5);
    CU_ASSERT_STRING_EQUAL((char *) vars->next->next->next->data, "hello");
}

void test_variables(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundCompileOrc(csound, "instr 1\n ivar init 2.5\n kvar init 3.5\n asig init 0.5\nSvar init \"hello\"\n endin\n");
    csoundInputMessage(csound, "i 1 0  1 440");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb3, NULL);
    csoundSetInstrumentBreakpoint(csound, 1, 1);

    csoundPerformKsmps(csound);

    csoundDebuggerClean(csound);
    csoundDestroy(csound);
}

static void brkpt_cb4(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
    debug_instr_t *debug_instr = bkpt_info->breakpointInstr;
    CU_ASSERT_EQUAL(debug_instr->p1, 1);
    CU_ASSERT_EQUAL(debug_instr->p2, 0);
    CU_ASSERT_EQUAL(debug_instr->p3, 1.1);
    CU_ASSERT_EQUAL(debug_instr->kcounter, 0);
}

void test_bkpt_instrument(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundCompileOrc(csound, "instr 1\n Svar init \"hello\"\n endin\n");
    csoundInputMessage(csound, "i 1 0  1.1 440");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb4, NULL);
    csoundSetInstrumentBreakpoint(csound, 1, 0);

    csoundPerformKsmps(csound);

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
    if (NULL == CU_add_test(pSuite, "Test Breakpoint instrument", test_bkpt_instrument)
         ||(NULL == CU_add_test(pSuite, "Test variables", test_variables)
         || (NULL == CU_add_test(pSuite, "Test debugger init", test_debugger_init))
         || (NULL == CU_add_test(pSuite, "Test add breakpoint", test_add_bkpt))
         || (NULL == CU_add_test(pSuite, "Test add callback", test_add_callback))
         || (NULL == CU_add_test(pSuite, "Test breakpoint", test_breakpoint_once))
         || (NULL == CU_add_test(pSuite, "Test breakpoint remove", test_breakpoint_remove))
         )
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

