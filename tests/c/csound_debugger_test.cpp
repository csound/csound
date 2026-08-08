/*
 * File:   csound_debugger_test.c
 * Author: mantaraya36
 */
#define __BUILDING_LIBCSOUND
#include <cmath>
#include <cstring>
#include <stdio.h>

#include "csoundCore.h"
#include "csdebug.h"
#include "gtest/gtest.h"


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
      csoundSetOption (csound, "-n");
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

TEST_F (DebuggerTests, testDebuggerFailParcs)
{
    csoundSetOption(csound, "-j 2");
    int32_t err = csoundDebuggerInit(csound);
    ASSERT_EQ(err, CSOUND_ERROR);
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
        csoundStart(csound);
   csoundEventString(csound, "i 1.1 0   1 440", 0);
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
        csoundStart(csound);
   csoundEventString(csound, "i 1.1 0   1 440", 0);
   csoundEventString(csound, "i 1.2 0   1 880", 0);
   csoundEventString(csound, "i 1.1 0.1 1 440", 0);
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

    csoundStart(csound);
       csoundEventString(csound, "i 1 0  1 440", 0);
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
      csoundStart(csound);
   csoundEventString(csound, "i 1 0  1.1 440", 0);

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
    csoundStart(csound);
     csoundEventString(csound, "i 1 0  1.1 440", 0);

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

    csoundStart(csound);
    csoundEventString(csound, "i 1 0  1.1 440", 0);
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

void brkpt_cb8(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *line_)
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

void brkpt_cb9(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *line_)
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

    csoundStart(csound);
    csoundEventString(csound, "i 1 0  0.1", 0);
    csoundEventString(csound, "i 1.2 0  0.1", 0);
    csoundEventString(csound, "i 30.1 0  0.01", 0);
    csoundEventString(csound, "i 30 0  0.01", 0);
    csoundEventString(csound, "i 30 1  0.11", 0);
    csoundEventString(csound, "i 1.3 0  0.1", 0);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb8, NULL);
    csoundSetInstrumentBreakpoint(csound, 1.2, 0);
    csoundPerformKsmps(csound);
    csoundPerformKsmps(csound);
    // Only the first call should have effect as we have already stopped
    csoundPerformKsmps(csound);  
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

    // step to next line 
    csoundDebugNext(csound);
    csoundPerformKsmps(csound);
    // step to next line 
    csoundDebugNext(csound);
    csoundPerformKsmps(csound);
    // step to next line 
    csoundDebugNext(csound);
    csoundPerformKsmps(csound);

    csoundDebuggerClean(csound);

    ASSERT_EQ(count, 5);
}

static int32_t kcycle_count = 0;

static void kcycle_cb_test(CSOUND *csound, void *userdata)
{
    int32_t *count = (int32_t *)userdata;
    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    if (instrs) {
        (*count)++;
        csoundDebugFreeInstrInstances(csound, instrs);
    }
    (void)csound;
}

TEST_F (DebuggerTests, testDebugCallbackFiresEachKcycle)
{
    int32_t i;
    kcycle_count = 0;

    /* The per-k-cycle debug callback only fires from kperf_debug, which is
       installed by csoundDebuggerInit(). Without it the plain kperf runs and
       the callback never fires. */
    csoundCompileOrc(csound, "instr 1\nkval init 1\nendin\n", 0);
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetDebugCallback(csound, kcycle_cb_test, &kcycle_count);
    csoundEventString(csound, "i 1 0 1", 0);

    for (i = 0; i < 8; i++) {
        csoundPerformKsmps(csound);
    }

    csoundRemoveDebugCallback(csound);
    csoundDebuggerClean(csound);
    ASSERT_GT(kcycle_count, 0);
}

static debug_variable_t *findDebugVar(debug_variable_t *vars, const char *name)
{
    while (vars) {
        if (vars->name && strcmp(vars->name, name) == 0) {
            return vars;
        }
        vars = vars->next;
    }
    return NULL;
}

static MYFLT readDebugScalar(debug_variable_t *var)
{
    return var && var->data ? *((MYFLT *)var->data) : 0;
}

TEST_F (DebuggerTests, testUdoFramesExposeInternalLocals)
{
    const char *orc =
        "opcode simpleGain, a, ak\n"
        "  ain, kGain xin\n"
        "  kInternal = kGain * 2\n"
        "  aOut = ain * kInternal\n"
        "  xout aOut\n"
        "endop\n"
        "instr 1\n"
        "  kGain init 0.25\n"
        "  aIn oscili 0.3, 440\n"
        "  aOut simpleGain aIn, kGain\n"
        "endin\n";

    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 1", 0);
    csoundPerformKsmps(csound);

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    ASSERT_NE(instrs, nullptr);
    debug_udo_frame_t *frames = csoundDebugGetUdoFrames(csound, instrs, NULL);
    ASSERT_NE(frames, nullptr);
    ASSERT_STREQ(frames->udoName, "simpleGain");
    ASSERT_GT(frames->callLine, 0);

    debug_variable_t *kInternal = findDebugVar(frames->varList, "kInternal");
    ASSERT_NE(kInternal, nullptr);
    ASSERT_DOUBLE_EQ(readDebugScalar(kInternal), 0.5);

    csoundDebugFreeUdoFrames(csound, frames);
    csoundDebugFreeInstrInstances(csound, instrs);
}

TEST_F (DebuggerTests, testUdoFramesDualCallSites)
{
    const char *orc =
        "opcode stereoGain, a, ak\n"
        "  ain, kGain xin\n"
        "  kScaled = kGain * 0.5\n"
        "  aOut = ain * kScaled\n"
        "  xout aOut\n"
        "endop\n"
        "instr 1\n"
        "  kGainL init 0.3\n"
        "  kGainR init 0.7\n"
        "  aIn oscili 0.4, 440\n"
        "  aL stereoGain aIn, kGainL\n"
        "  aR stereoGain aIn, kGainR\n"
        "endin\n";

    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 1", 0);
    csoundPerformKsmps(csound);

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    ASSERT_NE(instrs, nullptr);
    debug_udo_frame_t *frames = csoundDebugGetUdoFrames(csound, instrs, NULL);
    ASSERT_NE(frames, nullptr);

    int32_t count = 0;
    int32_t sawL = 0;
    int32_t sawR = 0;
    int32_t sawIndex0 = 0;
    int32_t sawIndex1 = 0;
    for (debug_udo_frame_t *f = frames; f != NULL; f = f->next) {
        count++;
        ASSERT_STREQ(f->udoName, "stereoGain");
        if (f->depth == 0) {
            if (f->frameIndex == 0) {
                sawIndex0 = 1;
            }
            if (f->frameIndex == 1) {
                sawIndex1 = 1;
            }
        }
        debug_variable_t *kScaled = findDebugVar(f->varList, "kScaled");
        ASSERT_NE(kScaled, nullptr);
        MYFLT val = readDebugScalar(kScaled);
        if (fabs(val - 0.15) < 1e-6) {
            sawL = 1;
        }
        if (fabs(val - 0.35) < 1e-6) {
            sawR = 1;
        }
    }
    ASSERT_EQ(count, 2);
    ASSERT_EQ(sawL, 1);
    ASSERT_EQ(sawR, 1);
    ASSERT_EQ(sawIndex0, 1);
    ASSERT_EQ(sawIndex1, 1);

    csoundDebugFreeUdoFrames(csound, frames);
    csoundDebugFreeInstrInstances(csound, instrs);
}

TEST_F (DebuggerTests, testUdoFramesNestedDepth)
{
    const char *orc =
        "opcode inner, k, k\n"
        "  kIn xin\n"
        "  kOut = kIn + 1\n"
        "  xout kOut\n"
        "endop\n"
        "opcode outer, k, k\n"
        "  kIn xin\n"
        "  kChild inner kIn\n"
        "  kOut = kChild + 10\n"
        "  xout kOut\n"
        "endop\n"
        "instr 1\n"
        "  kResult outer 0\n"
        "endin\n";

    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 1", 0);
    csoundPerformKsmps(csound);

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    debug_udo_frame_t *frames = csoundDebugGetUdoFrames(csound, instrs, NULL);
    ASSERT_NE(frames, nullptr);

    int32_t maxDepth = -1;
    int32_t sawInner = 0;
    for (debug_udo_frame_t *f = frames; f != NULL; f = f->next) {
        if (f->depth > maxDepth) {
            maxDepth = f->depth;
        }
        if (strcmp(f->udoName, "inner") == 0) {
            sawInner = 1;
            debug_variable_t *kOut = findDebugVar(f->varList, "kOut");
            ASSERT_NE(kOut, nullptr);
        }
    }
    ASSERT_GE(maxDepth, 1);
    ASSERT_EQ(sawInner, 1);

    csoundDebugFreeUdoFrames(csound, frames);
    csoundDebugFreeInstrInstances(csound, instrs);
}

TEST_F (DebuggerTests, testUdoFramesSiblingAfterNestedCall)
{
    const char *orc =
        "opcode inner, k, k\n"
        "  kIn xin\n"
        "  kOut = kIn * 2\n"
        "  xout kOut\n"
        "endop\n"
        "opcode gainOp, a, ak\n"
        "  ain, kGain xin\n"
        "  kInner inner kGain\n"
        "  aOut = ain * kInner\n"
        "  xout aOut\n"
        "endop\n"
        "instr 1\n"
        "  kL init 0.2\n"
        "  kR init 0.5\n"
        "  aIn oscili 0.4, 440\n"
        "  aL gainOp aIn, kL\n"
        "  aR gainOp aIn, kR\n"
        "endin\n";

    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 1", 0);
    csoundPerformKsmps(csound);

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    ASSERT_NE(instrs, nullptr);
    debug_udo_frame_t *frames = csoundDebugGetUdoFrames(csound, instrs, NULL);
    ASSERT_NE(frames, nullptr);

    int32_t frameCount = 0;
    int32_t sawGainL = 0;
    int32_t sawGainR = 0;
    int32_t sawInner = 0;
    for (debug_udo_frame_t *f = frames; f != NULL; f = f->next) {
        frameCount++;
        if (strcmp(f->udoName, "gainOp") == 0) {
            debug_variable_t *kGain = findDebugVar(f->varList, "kGain");
            ASSERT_NE(kGain, nullptr);
            MYFLT val = readDebugScalar(kGain);
            if (fabs(val - 0.2) < 1e-6) {
                sawGainL = 1;
            }
            if (fabs(val - 0.5) < 1e-6) {
                sawGainR = 1;
            }
        }
        if (strcmp(f->udoName, "inner") == 0) {
            sawInner = 1;
            ASSERT_GE(f->depth, 1);
        }
    }
    ASSERT_EQ(frameCount, 4);
    ASSERT_EQ(sawGainL, 1);
    ASSERT_EQ(sawGainR, 1);
    ASSERT_EQ(sawInner, 1);

    csoundDebugFreeUdoFrames(csound, frames);
    csoundDebugFreeInstrInstances(csound, instrs);
}

TEST_F (DebuggerTests, testUdoFramesDoubleInnerPerOuter)
{
    const char *orc =
        "opcode inner, k, k\n"
        "  kIn xin\n"
        "  kOut = kIn * 2\n"
        "  xout kOut\n"
        "endop\n"
        "opcode outer, k, k\n"
        "  kIn xin\n"
        "  kA inner kIn\n"
        "  kB inner kIn + 1\n"
        "  kOut = kA + kB\n"
        "  xout kOut\n"
        "endop\n"
        "instr 1\n"
        "  kA outer 0\n"
        "  kB outer 10\n"
        "endin\n";

    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 1", 0);
    csoundPerformKsmps(csound);

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    ASSERT_NE(instrs, nullptr);
    int32_t truncated = -1;
    debug_udo_frame_t *frames = csoundDebugGetUdoFrames(csound, instrs,
                                                          &truncated);
    ASSERT_NE(frames, nullptr);
    ASSERT_EQ(truncated, 0);

    int32_t frameCount = 0;
    int32_t outerCount = 0;
    int32_t innerCount = 0;
    for (debug_udo_frame_t *f = frames; f != NULL; f = f->next) {
        frameCount++;
        if (strcmp(f->udoName, "outer") == 0) {
            outerCount++;
            ASSERT_EQ(f->depth, 0);
        }
        if (strcmp(f->udoName, "inner") == 0) {
            innerCount++;
            ASSERT_EQ(f->depth, 1);
        }
    }
    ASSERT_EQ(frameCount, 6);
    ASSERT_EQ(outerCount, 2);
    ASSERT_EQ(innerCount, 4);

    csoundDebugFreeUdoFrames(csound, frames);
    csoundDebugFreeInstrInstances(csound, instrs);
}

TEST_F (DebuggerTests, testUdoFramesRecursiveSelfCall)
{
    const char *orc =
        "opcode selfCall, k, kpp\n"
        "  kIn, iDepth, iCnt xin\n"
        "  if (iCnt >= iDepth) goto leaf\n"
        "  kChild selfCall kIn, iDepth, iCnt + 1\n"
        "  kOut = kChild\n"
        "  goto done\n"
        "leaf:\n"
        "  kOut = 1\n"
        "done:\n"
        "  xout kOut\n"
        "endop\n"
        "instr 1\n"
        "  kResult selfCall 0, 3, 0\n"
        "endin\n";

    int32_t compileErr = csoundCompileOrc(csound, orc, 0);
    ASSERT_EQ(compileErr, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 1", 0);
    int32_t perfErr = csoundPerformKsmps(csound);
    ASSERT_EQ(perfErr, 0);

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    ASSERT_NE(instrs, nullptr);
    int32_t truncated = -1;
    debug_udo_frame_t *frames = csoundDebugGetUdoFrames(csound, instrs,
                                                          &truncated);
    ASSERT_NE(frames, nullptr);
    ASSERT_EQ(truncated, 0);

    int32_t frameCount = 0;
    int32_t maxDepth = -1;
    for (debug_udo_frame_t *f = frames; f != NULL; f = f->next) {
        frameCount++;
        ASSERT_STREQ(f->udoName, "selfCall");
        if (f->depth > maxDepth) {
            maxDepth = f->depth;
        }
    }
    ASSERT_GE(frameCount, 3);
    ASSERT_GE(maxDepth, 2);

    csoundDebugFreeUdoFrames(csound, frames);
    csoundDebugFreeInstrInstances(csound, instrs);
}
