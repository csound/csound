/*
 * csound_debug_callback_test.cpp
 *
 * Tests for csoundSetDebugCallback() / csoundRemoveDebugCallback().
 *
 * The callback fires inside kperf_debug() only — it requires
 * csoundDebuggerInit() to have been called first (which activates
 * kperf_debug). It does NOT fire in the normal kperf() loop.
 */
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "gtest/gtest.h"

/* Counts every time the callback fires */
static void debug_count_cb(CSOUND * /*csound*/, void *userdata)
{
    int32_t *count = (int32_t *) userdata;
    *count += 1;
}

/* Breakpoint callback used in the "stopped" test */
static void brkpt_noop_cb(CSOUND * /*csound*/,
                           debug_bkpt_info_t * /*info*/,
                           void * /*userdata*/)
{
}

class DebugCallbackTests : public ::testing::Test {
public:
    virtual void SetUp()
    {
        csound = csoundCreate(NULL, NULL);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n"); /* no audio output */
    }

    virtual void TearDown()
    {
        csoundDestroy(csound);
        csound = nullptr;
    }

    CSOUND *csound{nullptr};
};

/* ------------------------------------------------------------------ */
/* 1. Set and remove without debugger active — must not crash           */
/* ------------------------------------------------------------------ */
TEST_F(DebugCallbackTests, testSetAndRemoveNoDebugger)
{
    int32_t count = 0;
    csoundSetDebugCallback(csound, debug_count_cb, (void *) &count);
    csoundRemoveDebugCallback(csound);
    ASSERT_EQ(count, 0);
}

/* ------------------------------------------------------------------ */
/* 2. Callback fires once per k-cycle when kperf_debug is active        */
/* ------------------------------------------------------------------ */
TEST_F(DebugCallbackTests, testCallbackFiresInDebugMode)
{
    const int32_t CYCLES = 20;
    int32_t count = 0;

    csoundCompileOrc(csound, "instr 1\nasig oscil 1, p4\nendin\n", 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 2 440", 0);

    csoundDebuggerInit(csound);
    csoundSetDebugCallback(csound, debug_count_cb, (void *) &count);

    for (int32_t i = 0; i < CYCLES; i++) {
        csoundPerformKsmps(csound);
    }

    ASSERT_EQ(count, CYCLES);
    csoundDebuggerClean(csound);
}

/* ------------------------------------------------------------------ */
/* 3. Callback must NOT fire when csoundDebuggerInit has NOT been called */
/*    (csoundPerformKsmps dispatches to kperf, not kperf_debug)         */
/* ------------------------------------------------------------------ */
TEST_F(DebugCallbackTests, testCallbackNotFiredWithoutDebugger)
{
    int32_t count = 0;

    csoundCompileOrc(csound, "instr 1\nasig oscil 1, p4\nendin\n", 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 2 440", 0);

    /* Deliberately skip csoundDebuggerInit — kperf() will be used */
    csoundSetDebugCallback(csound, debug_count_cb, (void *) &count);

    for (int32_t i = 0; i < 20; i++) {
        csoundPerformKsmps(csound);
    }

    ASSERT_EQ(count, 0);
}

/* ------------------------------------------------------------------ */
/* 4. After csoundRemoveDebugCallback the callback stops firing         */
/* ------------------------------------------------------------------ */
TEST_F(DebugCallbackTests, testRemoveStopsFiring)
{
    const int32_t FIRST = 10;
    const int32_t SECOND = 10;
    int32_t count = 0;

    csoundCompileOrc(csound, "instr 1\nasig oscil 1, p4\nendin\n", 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 4 440", 0);

    csoundDebuggerInit(csound);
    csoundSetDebugCallback(csound, debug_count_cb, (void *) &count);

    for (int32_t i = 0; i < FIRST; i++) {
        csoundPerformKsmps(csound);
    }
    ASSERT_EQ(count, FIRST);

    csoundRemoveDebugCallback(csound);

    for (int32_t i = 0; i < SECOND; i++) {
        csoundPerformKsmps(csound);
    }
    /* count must not have grown after removal */
    ASSERT_EQ(count, FIRST);

    csoundDebuggerClean(csound);
}

/* ------------------------------------------------------------------ */
/* 5. Callback must NOT fire on a k-cycle where execution is stopped    */
/*    at a breakpoint; it resumes firing after csoundDebugContinue()    */
/* ------------------------------------------------------------------ */
TEST_F(DebugCallbackTests, testCallbackNotFiredWhenStopped)
{
    int32_t debug_count = 0;

    csoundCompileOrc(csound, "instr 1\nasig oscil 1, p4\nendin\n", 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 2 440", 0);

    csoundDebuggerInit(csound);
    csoundSetDebugCallback(csound, debug_count_cb, (void *) &debug_count);

    /* Set breakpoint on instr 1 with skip=0 (fires immediately) */
    csoundSetBreakpointCallback(csound, brkpt_noop_cb, NULL);
    csoundSetInstrumentBreakpoint(csound, 1, 0);

    /* First ksmps: kperf_debug hits the breakpoint and returns early —
       the debug callback is never reached. */
    csoundPerformKsmps(csound);
    ASSERT_EQ(debug_count, 0);

    /* Continue execution and run one k-cycle without a breakpoint stop —
       callback should now fire exactly once. */
    csoundDebugContinue(csound);
    csoundPerformKsmps(csound);
    ASSERT_EQ(debug_count, 1);

    csoundDebuggerClean(csound);
}
