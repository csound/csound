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
    csoundCreateMessageBuffer(csound, 0);
    csoundDebuggerInit(csound);
    csoundDebuggerClean(csound);
    csoundDestroy(csound);
}

void test_add_bkpt(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundCreateMessageBuffer(csound, 0);
    csoundDebuggerInit(csound);
    csoundSetBreakpoint(csound, 3, 0, 0);
    csoundSetBreakpoint(csound, 5, 1, 0);
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
    csoundCreateMessageBuffer(csound, 0);
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
    csoundCreateMessageBuffer(csound, 0);
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
    csoundCreateMessageBuffer(csound, 0);
    csoundCompileOrc(csound, "instr 1\nasig oscil 1, p4\nendin\n");
    csoundInputMessage(csound, "i 1.1 0   1 440");
    csoundInputMessage(csound, "i 1.2 0   1 880");
    csoundInputMessage(csound, "i 1.1 0.1 1 440");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb2, (void *) &break_count);
    csoundSetInstrumentBreakpoint(csound, 1.1, 0);

    for (i = 0; i < 10; i++) {
        csoundPerformKsmps(csound);
        csoundDebugContinue(csound);
    }

    csoundRemoveInstrumentBreakpoint(csound, 1.1);
    for (i = 0; i < 10; i++) {
        csoundPerformKsmps(csound);
        csoundDebugContinue(csound);
    }
    CU_ASSERT(break_count == 1);

    csoundDebuggerClean(csound);
    csoundDestroy(csound);
}

static void brkpt_cb3(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
    debug_variable_t *vars = bkpt_info->instrVarList;

    MYFLT data = *((MYFLT *)vars->data);
    CU_ASSERT_EQUAL(data, 2.5);
    data = *((MYFLT *)vars->next->data);
    CU_ASSERT(data == 3.5);
    data = *((MYFLT *)vars->next->next->data);
    CU_ASSERT(data== 0.5);
    char *str = (char *) vars->next->next->next->data;
    CU_ASSERT_STRING_EQUAL(str, "hello");
}

void test_variables(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundCreateMessageBuffer(csound, 0);
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
    csoundCreateMessageBuffer(csound, 0);
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

int count = 0;
static void brkpt_cb5(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
    debug_opcode_t *debug_opcode = bkpt_info->currentOpcode;
    count++;
}

void test_line_breakpoint_add_remove(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundCreateMessageBuffer(csound, 0);
    count = 0;
    csoundCompileOrc(csound, "instr 1\n"
                     "Svar init \"hello\"\n"
                     "ksig line 0, p3, 1\n"
                     "ksig2 line 1, p3, 0\n"
                     "asig3 oscils 0.5, 440, 0.5\n"
                     "endin\n");

    csoundInputMessage(csound, "i 1 0  1.1 440");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb5, NULL);
    csoundSetBreakpoint(csound, 5, 1, 0);
    csoundPerformKsmps(csound);
    csoundDebugContinue(csound);
    csoundPerformKsmps(csound); // This block performs
    csoundPerformKsmps(csound); // This block breaks

    csoundRemoveBreakpoint(csound, 5, 1);
    csoundPerformKsmps(csound);
    csoundPerformKsmps(csound);
    csoundPerformKsmps(csound);
    csoundPerformKsmps(csound);
    csoundPerformKsmps(csound);

    csoundDebuggerClean(csound);
    csoundDestroy(csound);
    CU_ASSERT_EQUAL(count, 2);
}

static void brkpt_cb6(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
    debug_opcode_t *debug_opcode = bkpt_info->currentOpcode;
    if (count == 0) {
        CU_ASSERT_STRING_EQUAL(debug_opcode->opname, "oscils");
    } else if (count == 1) {
        CU_ASSERT_STRING_EQUAL(debug_opcode->opname, "line");
    } else if (count == 2) {
        CU_ASSERT_STRING_EQUAL(debug_opcode->opname, "oscils");
    }
    count++;
}

void test_line_breakpoint(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundCreateMessageBuffer(csound, 0);
    count = 0;
    csoundCompileOrc(csound, "instr 1\n"
                     "Svar init \"hello\"\n"
                     "ksig line 0, p3, 1\n"
                     "ksig2 line 1, p3, 0\n"
                     "asig3 oscils 0.5, 440, 0.5\n"
                     "endin\n");

    csoundInputMessage(csound, "i 1 0  1.1 440");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb6, NULL);
    csoundSetBreakpoint(csound, 5, 1, 0);
    csoundPerformKsmps(csound);

    csoundDebugContinue(csound);
    csoundPerformKsmps(csound);
    csoundSetBreakpoint(csound, 4, 1, 0);
    csoundPerformKsmps(csound);

    csoundDebugContinue(csound);
    csoundPerformKsmps(csound);
    csoundRemoveBreakpoint(csound, 4, 1);
    csoundPerformKsmps(csound);

    csoundDebugContinue(csound);
    csoundSetBreakpoint(csound, 1, 1, 0); // This breakpoint shouldn't be triggered as it's an init opcode
    csoundPerformKsmps(csound);

    csoundDebugContinue(csound);
    csoundSetBreakpoint(csound, 2, 2, 0); // This breakpoint shouldn't be triggered as instr 2 is not defined
    csoundPerformKsmps(csound);

    csoundDebuggerClean(csound);

    csoundDestroy(csound);

    CU_ASSERT_EQUAL(count, 2);
}

static void brkpt_cb7(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *line_)
{
    debug_opcode_t *debug_opcode = bkpt_info->currentOpcode;
    int *line = (int *) line_;
    CU_ASSERT(debug_opcode->line == *line);
    if (*line == 5) {
        CU_ASSERT_STRING_EQUAL(debug_opcode->opname, "line");
    } else if (*line == 6) {
        CU_ASSERT_STRING_EQUAL(debug_opcode->opname, "oscils");
    } else {
        CU_ASSERT(0 == 1); // Wrong line number
    }
    count++;
}

void test_line_breakpoint_orc_file(void)
{
    FILE *f = fopen("debug.orc", "w");
    CU_ASSERT_PTR_NOT_NULL(f);

    const char *orc = "\n"
                      "instr 1\n"
                      "Svar init \"hello\"\n"
                      "ksig line 0, p3, 1\n"
                      "ksig2 line 1, p3, 0\n"
                      "asig3 oscils 0.5, 440, 0.5\n"
                      "endin\n";
    fprintf(f, "%s", orc);
    fclose(f);
    f = fopen("debug.sco", "w");
    CU_ASSERT_PTR_NOT_NULL(f);

    const char *sco = "i 1 0 1\n";
    fprintf(f, "%s", sco);
    fclose(f);
    count = 0;
    CSOUND* csound = csoundCreate(NULL);

    csoundCreateMessageBuffer(csound, 0);
    const char* argv[] = {"csound", "debug.orc", "debug.sco"};
    csoundCompile(csound, 3, (char **) argv);

    csoundDebuggerInit(csound);
    int line = 5;
    csoundSetBreakpointCallback(csound, brkpt_cb7, &line);
    csoundSetBreakpoint(csound, line, 0, 0);
    csoundPerformKsmps(csound);
    csoundDebugContinue(csound);
    csoundRemoveBreakpoint(csound, line, 0);
    csoundPerformKsmps(csound);
    csoundDebugContinue(csound);
    csoundPerformKsmps(csound);
    csoundDebugContinue(csound);
    csoundPerformKsmps(csound);
    csoundDebugContinue(csound);

    line = 6;
    csoundSetBreakpoint(csound, line, 0, 0);
    csoundPerformKsmps(csound);
    csoundDebugContinue(csound);
//    csoundPerformKsmps(csound);

    csoundDebuggerClean(csound);
    CU_ASSERT_EQUAL(count, 2);
    csoundDestroy(csound);
}

void test_no_callback(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundCreateMessageBuffer(csound, 0);
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetInstrumentBreakpoint(csound, 1, 0);

    csoundPerformKsmps(csound);

    csoundDebuggerClean(csound);
    csoundDestroy(csound);
}

static void brkpt_cb8(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *line_)
{
    switch (count) {
    case 0:
        CU_ASSERT_EQUAL(bkpt_info->breakpointInstr->p1, 1.2);
        break;
    case 1:
        CU_ASSERT_EQUAL(bkpt_info->breakpointInstr->p1, 1.3);
        break;
    case 2:
        CU_ASSERT_EQUAL(bkpt_info->breakpointInstr->p1, 30);
        break;
    case 3:
         CU_ASSERT_EQUAL(bkpt_info->breakpointInstr->p1, 30.1);
        break;
    case 4:
         CU_ASSERT_EQUAL(bkpt_info->breakpointInstr->p1, 1);
        break;
    case 5:
         CU_ASSERT_EQUAL(bkpt_info->breakpointInstr->p1, 1.2);
        break;
    }
    count++;
}


static void brkpt_cb9(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *line_)
{
    debug_variable_t *vars = bkpt_info->instrVarList;
    MYFLT val = -1;
    while (vars) {
        if (strcmp(vars->name, "kvar") == 0) {
            val = *((MYFLT *) vars->data);
            break;
        }
        vars = vars->next;
    }
    CU_ASSERT_DOUBLE_EQUAL(val, 10, 0.000001);
}

void test_next(void)
{
    CSOUND* csound = csoundCreate(NULL);
    csoundCreateMessageBuffer(csound, 0);
    count = 0;
    csoundCompileOrc(csound, "instr 1\n"
                             "Svar init \"hello\"\n"
                             "ksig line 0, p3, 1\n"
                             "ksig2 line 1, p3, 0\n"
                             "asig3 oscils 0.5, 440, 0.5\n"
                             "endin\n");
    csoundCompileOrc(csound, "instr 30\n"
                             "kvar init 10\n"
                             "kvar = kvar + 1\n"
                             "ksig2 line 1, p3, 0\n"
                             "kvar = kvar + 1\n"
                             "endin\n");

    csoundInputMessage(csound, "i 1 0  0.1");
    csoundInputMessage(csound, "i 1.2 0  0.1");
    csoundInputMessage(csound, "i 30.1 0  0.01");
    csoundInputMessage(csound, "i 30 0  0.01");
    csoundInputMessage(csound, "i 30 1  0.11");
    csoundInputMessage(csound, "i 1.3 0  0.1");

    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb8, NULL);
    csoundSetInstrumentBreakpoint(csound, 1.2, 0);
    csoundPerformKsmps(csound);
    csoundPerformKsmps(csound);
    csoundPerformKsmps(csound);  // Only the first call should have effect as we have already stopped

    csoundDebugNext(csound);
    csoundPerformKsmps(csound);
    csoundPerformKsmps(csound); // Ignored
    csoundDebugNext(csound);
    csoundPerformKsmps(csound);
    csoundPerformKsmps(csound); // Ignored
    csoundDebugNext(csound);
    csoundPerformKsmps(csound);
    csoundPerformKsmps(csound); // Ignored
    csoundDebugNext(csound);
    csoundPerformKsmps(csound);
    csoundPerformKsmps(csound); // Ignored
    csoundRemoveInstrumentBreakpoint(csound, 1.2);
    csoundDebugContinue(csound);

    int i;
    for (i = 0; i < 200; i++) {
        csoundPerformKsmps(csound);
    }

    csoundSetBreakpointCallback(csound, brkpt_cb9, NULL);
    csoundSetInstrumentBreakpoint(csound, 30.1, 0);
    for (i = 0; i < 1000; i++) {
        csoundPerformKsmps(csound);
    }

    /* step to next line */
    csoundDebugNext(csound);
    csoundPerformKsmps(csound);
    /* step to next line */
    csoundDebugNext(csound);
    csoundPerformKsmps(csound);
    /* step to next line */
    csoundDebugNext(csound);
    csoundPerformKsmps(csound);

    csoundDebuggerClean(csound);
    csoundDestroy(csound);
    CU_ASSERT_EQUAL(count, 5);
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
    if ( (NULL == CU_add_test(pSuite, "Test Next Command", test_next))
         || (NULL == CU_add_test(pSuite, "Test Line breakpoints for orc file", test_line_breakpoint_orc_file))
         || (NULL == CU_add_test(pSuite, "Test no callback", test_no_callback))
         || (NULL == CU_add_test(pSuite, "Test Line breakpoints", test_line_breakpoint))
         || (NULL == CU_add_test(pSuite, "Test Line breakpoints add/remove", test_line_breakpoint_add_remove))
         || (NULL == CU_add_test(pSuite, "Test Breakpoint instrument", test_bkpt_instrument))
         ||(NULL == CU_add_test(pSuite, "Test variables", test_variables))
         || (NULL == CU_add_test(pSuite, "Test debugger init", test_debugger_init))
         || (NULL == CU_add_test(pSuite, "Test add breakpoint", test_add_bkpt))
         || (NULL == CU_add_test(pSuite, "Test add callback", test_add_callback))
         || (NULL == CU_add_test(pSuite, "Test breakpoint", test_breakpoint_once))
         || (NULL == CU_add_test(pSuite, "Test breakpoint remove", test_breakpoint_remove))
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

