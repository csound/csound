/*
 * File:   csound_debugger_test.c
 * Author: mantaraya36
 */
#define __BUILDING_LIBCSOUND
#include <stdio.h>

#include "csoundCore.h"
#include "csdebug.h"
#include "gtest/gtest.h"



extern "C"  void csoundInputMessage(CSOUND *csound, const char * sc);

//#define csoundInputMessage(a,b) csoundEventString(a,b,0)

class DebuggerTests : public ::testing::Test {
public:
    DebuggerTests ()
    {
    }

    virtual ~DebuggerTests ()
    {
    }

    virtual void SetUp ()
    {
      csound = csoundCreate (NULL,NULL);
      csoundCreateMessageBuffer (csound, 0);
      //csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

TEST_F (DebuggerTests, testDebuggerInit)
{
    csoundDebuggerInit(csound);
    csoundDebuggerClean(csound);
}

TEST_F (DebuggerTests, testAddBreakpoint)
{
    csoundDebuggerInit(csound);
    csoundSetBreakpoint(csound, 3, 0, 0);
    csoundSetBreakpoint(csound, 5, 1, 0);
    csoundSetInstrumentBreakpoint(csound, 3.4, 0);
    csoundSetInstrumentBreakpoint(csound, 1.1, 0);
    csoundClearBreakpoints(csound);
    csoundDebuggerClean(csound);
}

static void brkpt_cb(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
    int32_t *count = (int32_t *) userdata;
    *count = *count + 1;
}

TEST_F (DebuggerTests, testAddCallback)
{
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb, NULL);
    csoundDebuggerClean(csound);
}

TEST_F (DebuggerTests, testBreakpointOnce)
{
    int32_t i;
    int32_t break_count = 0;

    csoundCompileOrc(csound, "instr 1\nasig oscil 1, p4\nendin\n", 0);
    csoundInputMessage(csound, "i 1.1 0   1 440");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb, (void *) &break_count);
    csoundSetInstrumentBreakpoint(csound, 1.1, 0);

    for (i = 0; i < 1000; i++) {
        csoundPerformKsmps(csound);
    }

    ASSERT_EQ (break_count, 1);
    csoundDebuggerClean(csound);
}

static void brkpt_cb2(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
//    INSDS *i = csoundDebugGetInstrument(csound);
    int32_t *count = (int32_t *) userdata;
    *count = *count + 1;
    csoundRemoveInstrumentBreakpoint(csound, bkpt_info->breakpointInstr->p1);
    csoundDebugContinue(csound);
}

TEST_F (DebuggerTests, testBreakpointRemove)
{
    int32_t i;
    int32_t break_count = 0;

    csoundCompileOrc(csound, "instr 1\nasig oscil 1, p4\nendin\n", 0);
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

    csoundDebuggerClean(csound);
}

static void brkpt_cb3(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
    debug_variable_t *vars = bkpt_info->instrVarList;

    MYFLT data = *((MYFLT *)vars->data);
    ASSERT_EQ (data, 2.5);
    data = *((MYFLT *)vars->next->data);
    ASSERT_EQ (data, 3.5);
    data = *((MYFLT *)vars->next->next->data);
    ASSERT_EQ (data, 0.5);
    char *str = (char *) vars->next->next->next->data;
    ASSERT_STREQ (str, "hello");
}

TEST_F (DebuggerTests, testVariables)
{
  csoundCompileOrc(csound, "instr 1\n ivar init 2.5\n kvar init 3.5\n asig init 0.5\nSvar init \"hello\"\n endin\n", 0);
    csoundInputMessage(csound, "i 1 0  1 440");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb3, NULL);
    csoundSetInstrumentBreakpoint(csound, 1, 1);
    csoundPerformKsmps(csound);
    csoundDebuggerClean(csound);
}

static void brkpt_cb4(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
    debug_instr_t *debug_instr = bkpt_info->breakpointInstr;
    ASSERT_EQ (debug_instr->p1, 1);
    ASSERT_EQ (debug_instr->p2, 0);
    ASSERT_EQ (debug_instr->p3, (MYFLT) 1.1);

    ASSERT_EQ (debug_instr->kcounter, 0);
}

TEST_F (DebuggerTests, testBreakpointInstrument)
{
  csoundCompileOrc(csound, "instr 1\n Svar init \"hello\"\n endin\n", 0);
    csoundInputMessage(csound, "i 1 0  1.1 440");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb4, NULL);
    csoundSetInstrumentBreakpoint(csound, 1, 0);
    csoundPerformKsmps(csound);
    csoundDebuggerClean(csound);
}

int32_t count = 0;
static void brkpt_cb5(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
    count++;
}

TEST_F (DebuggerTests, testLineBreakpointAddRemove)
{
    count = 0;
    csoundCompileOrc(csound, "instr 1\n"
                     "Svar init \"hello\"\n"
                     "ksig line 0, p3, 1\n"
                     "ksig2 line 1, p3, 0\n"
                     "asig3 oscils 0.5, 440, 0.5\n"
                     "endin\n", 0);

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
    ASSERT_EQ(count, 2);
}

static void brkpt_cb6(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
    debug_opcode_t *debug_opcode = bkpt_info->currentOpcode;

    if (count == 0) {
        ASSERT_STREQ (debug_opcode->opname, "oscils");
    } else if (count == 1) {
        ASSERT_STREQ (debug_opcode->opname, "line");
    } else if (count == 2) {
        ASSERT_STREQ (debug_opcode->opname, "line");
    }

    count++;
}

TEST_F (DebuggerTests, testLineBreakpoint)
{
    count = 0;
    csoundCompileOrc(csound, "instr 1\n"
                     "Svar init \"hello\"\n"
                     "ksig line 0, p3, 1\n"
                     "ksig2 line 1, p3, 0\n"
                     "asig3 oscils 0.5, 440, 0.5\n"
                     "endin\n", 0);

    csoundInputMessage(csound, "i 1 0  1.1 440");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb6, NULL);
    csoundSetBreakpoint(csound, 5, 1, 0);
    csoundPerformKsmps(csound);

    ASSERT_EQ (count, 1);

    csoundRemoveBreakpoint(csound, 5, 1);
    csoundDebugContinue(csound);
    csoundPerformKsmps(csound);
    csoundSetBreakpoint(csound, 4, 1, 0);
    csoundPerformKsmps(csound);

    ASSERT_EQ (count, 2);

    csoundDebugContinue(csound);
    csoundPerformKsmps(csound); // This completes the k-pass

    csoundPerformKsmps(csound); // This triggers the breakpoint again

    ASSERT_EQ (count, 3);
    csoundRemoveBreakpoint(csound, 4, 1);
    csoundDebugContinue(csound);
    csoundPerformKsmps(csound);

    csoundDebugContinue(csound);
    csoundSetBreakpoint(csound, 1, 1, 0); // This breakpoint shouldn't be triggered as it's an init opcode
    csoundPerformKsmps(csound);

    ASSERT_EQ (count, 3);
    csoundDebugContinue(csound);
    csoundSetBreakpoint(csound, 2, 2, 0); // This breakpoint shouldn't be triggered as instr 2 is not defined
    csoundPerformKsmps(csound);

    ASSERT_EQ (count, 3);

    csoundDebuggerClean(csound);

    ASSERT_EQ(count, 3);
}

static void brkpt_cb7(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *line_)
{
    debug_opcode_t *debug_opcode = bkpt_info->currentOpcode;
    int32_t *line = (int32_t *) line_;
    ASSERT_EQ (debug_opcode->line, *line);

    if (*line == 5) {
        ASSERT_STREQ (debug_opcode->opname, "line");
    } else if (*line == 6) {
        ASSERT_STREQ (debug_opcode->opname, "oscils");
    } else {
        ASSERT_EQ (0, 1); // Wrong line number
    }

    count++;
}

TEST_F (DebuggerTests, testLineBreakpointOrcFile)
{
    FILE *f = fopen("debug.orc", "w");
    ASSERT_TRUE (f != NULL);

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
    ASSERT_TRUE (f !=NULL);

    const char *sco = "i 1 0 1\n";
    fprintf(f, "%s", sco);
    fclose(f);
    count = 0;

    const char* argv[] = {"csound", "debug.orc", "debug.sco"};
    csoundCompile(csound, 3, argv);
    csoundStart(csound);

    csoundDebuggerInit(csound);
    int32_t line = 5;
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
    ASSERT_EQ(count, 2);
}

TEST_F (DebuggerTests, testNoCallback)
{
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetInstrumentBreakpoint(csound, 1, 0);
    csoundPerformKsmps(csound);
    csoundDebuggerClean(csound);
}

static void brkpt_cb8(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *line_)
{
    switch (count) {
    case 0:
        ASSERT_EQ (bkpt_info->breakpointInstr->p1, (MYFLT) 1.2);
        break;
    case 1:
        ASSERT_EQ (bkpt_info->breakpointInstr->p1, (MYFLT) 1.3);
        break;
    case 2:
        ASSERT_EQ (bkpt_info->breakpointInstr->p1, (MYFLT) 30);
        break;
    case 3:
         ASSERT_EQ (bkpt_info->breakpointInstr->p1, (MYFLT) 30.1);
        break;
    case 4:
         ASSERT_EQ (bkpt_info->breakpointInstr->p1, (MYFLT) 1);
        break;
    case 5:
         ASSERT_EQ (bkpt_info->breakpointInstr->p1, (MYFLT)1.2);
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
    ASSERT_DOUBLE_EQ (val, 10);
}

TEST_F (DebuggerTests, testNext)
{
    count = 0;
    csoundCompileOrc(csound, "instr 1\n"
                             "Svar init \"hello\"\n"
                             "ksig line 0, p3, 1\n"
                             "ksig2 line 1, p3, 0\n"
                             "asig3 oscils 0.5, 440, 0.5\n"
                             "endin\n"
                             "instr 30\n"
                             "kvar init 10\n"
                             "kvar = kvar + 1\n"
                             "ksig2 line 1, p3, 0\n"
                             "kvar = kvar + 1\n"
                             "endin\n", 0);

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

    int32_t i;
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

    ASSERT_EQ(count, 5);
}

