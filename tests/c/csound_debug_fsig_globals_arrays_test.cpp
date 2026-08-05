/*
 * csound_debug_fsig_globals_arrays_test.cpp
 *
 * Tests for the debugger extensions that expose f-signals, global variables
 * and numeric arrays:
 *   - csoundDebugGetGlobalVariables()
 *   - csoundDebugSerializeFsig()
 *   - csoundDebugSerializeArray()
 */
#define __BUILDING_LIBCSOUND
#include <cmath>
#include <cstring>
#include <stdio.h>

#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

class DebugFsigGlobalsArraysTests : public ::testing::Test {
public:
    DebugFsigGlobalsArraysTests () {}
    virtual ~DebugFsigGlobalsArraysTests () {}

    virtual void SetUp () {
        csound = csoundCreate (NULL, NULL);
        csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "-n");
    }

    virtual void TearDown () {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

static debug_variable_t *findVar(debug_variable_t *vars, const char *name)
{
    while (vars) {
        if (vars->name && strcmp(vars->name, name) == 0) {
            return vars;
        }
        vars = vars->next;
    }
    return NULL;
}

/* ---------------------------------------------------------------- globals -- */

TEST_F (DebugFsigGlobalsArraysTests, testGlobalScalarsEnumerated)
{
    const char *orc =
        "gkTempo init 120\n"
        "giBase init 7\n"
        "instr 1\n"
        "  kLocal init 3\n"
        "endin\n";
    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 1", 0);
    csoundPerformKsmps(csound);

    debug_variable_t *globals = csoundDebugGetGlobalVariables(csound);
    ASSERT_NE(globals, nullptr);

    debug_variable_t *gkTempo = findVar(globals, "gkTempo");
    ASSERT_NE(gkTempo, nullptr);
    ASSERT_STREQ(gkTempo->typeName, "k");
    ASSERT_NE(gkTempo->data, nullptr);
    ASSERT_DOUBLE_EQ(*((MYFLT *)gkTempo->data), 120.0);

    debug_variable_t *giBase = findVar(globals, "giBase");
    ASSERT_NE(giBase, nullptr);
    ASSERT_NE(giBase->data, nullptr);
    ASSERT_DOUBLE_EQ(*((MYFLT *)giBase->data), 7.0);

    /* instrument-local variables must NOT appear in the global pool */
    ASSERT_EQ(findVar(globals, "kLocal"), nullptr);

    csoundDebugFreeVariables(csound, globals);
}

TEST_F (DebugFsigGlobalsArraysTests, testInternalGlobalsEnumerated)
{
    csoundCompileOrc(csound, "instr 1\nendin\n", 0);
    csoundStart(csound);

    debug_variable_t *globals = csoundDebugGetGlobalVariables(csound);
    ASSERT_NE(globals, nullptr);

    debug_variable_t *sr = findVar(globals, "sr");
    ASSERT_NE(sr, nullptr);
    ASSERT_NE(sr->data, nullptr);
    ASSERT_GT(*((MYFLT *)sr->data), 0.0);

    csoundDebugFreeVariables(csound, globals);
}

TEST_F (DebugFsigGlobalsArraysTests, testGlobalVariablesNullCsound)
{
    ASSERT_EQ(csoundDebugGetGlobalVariables(NULL), nullptr);
}

TEST_F (DebugFsigGlobalsArraysTests, testGlobalArraySerializable)
{
    const char *orc =
        "giArr[] fillarray 1, 2, 3\n"
        "instr 1\n"
        "endin\n";
    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundPerformKsmps(csound);

    debug_variable_t *globals = csoundDebugGetGlobalVariables(csound);
    ASSERT_NE(globals, nullptr);
    debug_variable_t *arr = findVar(globals, "giArr");
    ASSERT_NE(arr, nullptr);
    ASSERT_STREQ(arr->typeName, "[");
    ASSERT_NE(arr->data, nullptr);

    debug_array_info_t info;
    MYFLT buf[8];
    int32_t total = csoundDebugSerializeArray(csound, arr->data, buf, 8, &info);
    ASSERT_EQ(total, 3);
    ASSERT_EQ(info.dimensions, 1);
    ASSERT_DOUBLE_EQ(buf[0], 1.0);
    ASSERT_DOUBLE_EQ(buf[1], 2.0);
    ASSERT_DOUBLE_EQ(buf[2], 3.0);

    csoundDebugFreeVariables(csound, globals);
}

/* --------------------------------------------------------------- f-signal -- */

TEST_F (DebugFsigGlobalsArraysTests, testSerializeFsigInstrumentLocal)
{
    const char *orc =
        "instr 1\n"
        "  aOsc oscili 0.5, 440\n"
        "  fSig pvsanal aOsc, 1024, 256, 1024, 1\n"
        "  aOut pvsynth fSig\n"
        "  out aOut\n"
        "endin\n";
    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 4", 0);
    /* run enough k-cycles to allocate + fill at least one analysis frame */
    for (int i = 0; i < 400; i++) {
        csoundPerformKsmps(csound);
    }

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    ASSERT_NE(instrs, nullptr);
    debug_variable_t *vars = csoundDebugGetVariables(csound, instrs);
    ASSERT_NE(vars, nullptr);

    debug_variable_t *fSig = findVar(vars, "fSig");
    ASSERT_NE(fSig, nullptr);
    ASSERT_STREQ(fSig->typeName, "f");
    ASSERT_NE(fSig->data, nullptr);

    const int32_t expectedNB = 1024 / 2 + 1;
    debug_fsig_info_t info;
    float buf[2 * (1024 / 2 + 1)];
    int32_t total = csoundDebugSerializeFsig(
        csound, fSig->data, buf,
        (int32_t)(sizeof(buf) / sizeof(buf[0])), &info, 0);

    ASSERT_EQ(info.N, 1024);
    ASSERT_EQ(info.NB, expectedNB);
    ASSERT_EQ(info.overlap, 256);
    ASSERT_EQ(total, 2 * expectedNB);

    /* amplitudes are the even-indexed values; must be finite and >= 0 */
    for (int32_t i = 0; i < total; i += 2) {
        ASSERT_FALSE(std::isnan(buf[i]));
        ASSERT_GE(buf[i], 0.0f);
    }

    csoundDebugFreeVariables(csound, vars);
    csoundDebugFreeInstrInstances(csound, instrs);
}

TEST_F (DebugFsigGlobalsArraysTests, testSerializeFsigSlidingLocalKsmps)
{
    const char *orc =
        "sr = 44100\n"
        "ksmps = 32\n"
        "nchnls = 1\n"
        "0dbfs = 1\n"
        "opcode slidingAnal, a, a\n"
        "  setksmps 1\n"
        "  ain xin\n"
        "  fSig pvsanal ain, 512, 1, 512, 1\n"
        "  aOut pvsynth fSig\n"
        "  xout aOut\n"
        "endop\n"
        "instr 1\n"
        "  aIn oscili 0.5, 440\n"
        "  aOut slidingAnal aIn\n"
        "  out aOut\n"
        "endin\n";
    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 4", 0);
    for (int i = 0; i < 400; i++) {
        csoundPerformKsmps(csound);
    }

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    ASSERT_NE(instrs, nullptr);
    debug_udo_frame_t *frames = csoundDebugGetUdoFrames(csound, instrs, NULL);
    ASSERT_NE(frames, nullptr);
    ASSERT_STREQ(frames->udoName, "slidingAnal");

    debug_variable_t *fSig = findVar(frames->varList, "fSig");
    ASSERT_NE(fSig, nullptr);
    ASSERT_STREQ(fSig->typeName, "f");
    ASSERT_NE(fSig->data, nullptr);

    const int32_t expectedNB = 512 / 2 + 1;
    debug_fsig_info_t info;
    float buf[2 * (512 / 2 + 1)];
    int32_t total = csoundDebugSerializeFsig(
        csound, fSig->data, buf,
        (int32_t)(sizeof(buf) / sizeof(buf[0])), &info, 1);

    ASSERT_EQ(info.sliding, 1);
    ASSERT_EQ(info.N, 512);
    ASSERT_EQ(info.NB, expectedNB);
    ASSERT_EQ(total, 2 * expectedNB);

    for (int32_t i = 0; i < total; i += 2) {
        ASSERT_FALSE(std::isnan(buf[i]));
        ASSERT_GE(buf[i], 0.0f);
    }

    csoundDebugFreeUdoFrames(csound, frames);
    csoundDebugFreeInstrInstances(csound, instrs);
}

TEST_F (DebugFsigGlobalsArraysTests, testSerializeFsigSlidingReuseAfterKsmpsChange)
{
    /* Sequential notes through one call site: first note allocates a 32-slot
       sliding buffer, then a later note recycles the UDO instance with
       setksmps 1. Capacity alone would select a stale subframe. */
    const char *orc =
        "sr = 44100\n"
        "ksmps = 32\n"
        "nchnls = 1\n"
        "0dbfs = 1\n"
        "opcode slidingAnal, a, ai\n"
        "  ain, iKsmps xin\n"
        "  setksmps iKsmps\n"
        "  fSig pvsanal ain, 512, 1, 512, 1\n"
        "  aOut pvsynth fSig\n"
        "  xout aOut\n"
        "endop\n"
        "instr 1\n"
        "  iKsmps = p4\n"
        "  aIn oscili 0.5, 440\n"
        "  aOut slidingAnal aIn, iKsmps\n"
        "  out aOut\n"
        "endin\n";
    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);

    /* Phase A: short note with local ksmps 32 (allocates capacity=32). */
    csoundEventString(csound, "i 1 0 0.05 32", 0);
    for (int i = 0; i < 100; i++) {
        csoundPerformKsmps(csound);
    }

    /* Phase B: new note with local ksmps 1; recycled instance keeps capacity. */
    csoundEventString(csound, "i 1 0 4 1", 0);
    for (int i = 0; i < 400; i++) {
        csoundPerformKsmps(csound);
    }

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    ASSERT_NE(instrs, nullptr);
    debug_udo_frame_t *frames = csoundDebugGetUdoFrames(csound, instrs, NULL);
    ASSERT_NE(frames, nullptr);
    ASSERT_STREQ(frames->udoName, "slidingAnal");

    debug_variable_t *fSig = findVar(frames->varList, "fSig");
    ASSERT_NE(fSig, nullptr);

    const int32_t expectedNB = 512 / 2 + 1;
    debug_fsig_info_t info;
    float bufActive[2 * (512 / 2 + 1)];
    float bufStale[2 * (512 / 2 + 1)];
    int32_t total = csoundDebugSerializeFsig(
        csound, fSig->data, bufActive,
        (int32_t)(sizeof(bufActive) / sizeof(bufActive[0])), &info, 1);
    ASSERT_EQ(info.sliding, 1);
    ASSERT_EQ(total, 2 * expectedNB);
    for (int32_t i = 0; i < total; i += 2) {
        ASSERT_FALSE(std::isnan(bufActive[i]));
        ASSERT_GE(bufActive[i], 0.0f);
    }

    /* Capacity still reflects the first note; localKsmps=32 reads a stale
       subframe that differs from the active localKsmps=1 subframe. */
    total = csoundDebugSerializeFsig(
        csound, fSig->data, bufStale,
        (int32_t)(sizeof(bufStale) / sizeof(bufStale[0])), &info, 32);
    ASSERT_EQ(total, 2 * expectedNB);
    ASSERT_NE(bufActive[0], bufStale[0]);

    csoundDebugFreeUdoFrames(csound, frames);
    csoundDebugFreeInstrInstances(csound, instrs);
}

TEST_F (DebugFsigGlobalsArraysTests, testSerializeFsigReportsSizeWithNullBuffer)
{
    const char *orc =
        "instr 1\n"
        "  aOsc oscili 0.5, 440\n"
        "  fSig pvsanal aOsc, 1024, 256, 1024, 1\n"
        "  aOut pvsynth fSig\n"
        "  out aOut\n"
        "endin\n";
    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 4", 0);
    for (int i = 0; i < 400; i++) {
        csoundPerformKsmps(csound);
    }

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    debug_variable_t *vars = csoundDebugGetVariables(csound, instrs);
    debug_variable_t *fSig = findVar(vars, "fSig");
    ASSERT_NE(fSig, nullptr);

    /* NULL outBuf -> report total size without copying */
    int32_t total = csoundDebugSerializeFsig(csound, fSig->data, NULL, 0, NULL, 0);
    ASSERT_EQ(total, 2 * (1024 / 2 + 1));

    csoundDebugFreeVariables(csound, vars);
    csoundDebugFreeInstrInstances(csound, instrs);
}

TEST_F (DebugFsigGlobalsArraysTests, testSerializeFsigNullSafe)
{
    debug_fsig_info_t info;
    float buf[8];
    ASSERT_EQ(csoundDebugSerializeFsig(csound, NULL, buf, 8, &info, 0), 0);
    ASSERT_EQ(info.NB, 0);
    /* NULL info pointer must also be tolerated */
    ASSERT_EQ(csoundDebugSerializeFsig(csound, NULL, buf, 8, NULL, 0), 0);
}

/* ----------------------------------------------------------------- arrays -- */

TEST_F (DebugFsigGlobalsArraysTests, testSerializeArrayNumeric)
{
    const char *orc =
        "instr 1\n"
        "  iArr[] fillarray 10, 20, 30, 40\n"
        "endin\n";
    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 1", 0);
    csoundPerformKsmps(csound);

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    ASSERT_NE(instrs, nullptr);
    debug_variable_t *vars = csoundDebugGetVariables(csound, instrs);
    debug_variable_t *arr = findVar(vars, "iArr");
    ASSERT_NE(arr, nullptr);
    ASSERT_STREQ(arr->typeName, "[");
    ASSERT_NE(arr->data, nullptr);

    debug_array_info_t info;
    MYFLT buf[16];
    int32_t total = csoundDebugSerializeArray(csound, arr->data, buf, 16, &info);
    ASSERT_EQ(total, 4);
    ASSERT_EQ(info.dimensions, 1);
    ASSERT_STREQ(info.elementTypeName, "i");
    ASSERT_DOUBLE_EQ(buf[0], 10.0);
    ASSERT_DOUBLE_EQ(buf[1], 20.0);
    ASSERT_DOUBLE_EQ(buf[2], 30.0);
    ASSERT_DOUBLE_EQ(buf[3], 40.0);

    csoundDebugFreeVariables(csound, vars);
    csoundDebugFreeInstrInstances(csound, instrs);
}

TEST_F (DebugFsigGlobalsArraysTests, testSerializeArrayNonNumericReturnsZero)
{
    const char *orc =
        "instr 1\n"
        "  SArr[] init 2\n"
        "  SArr[0] = \"hello\"\n"
        "  SArr[1] = \"world\"\n"
        "endin\n";
    csoundCompileOrc(csound, orc, 0);
    csoundStart(csound);
    csoundEventString(csound, "i 1 0 1", 0);
    csoundPerformKsmps(csound);

    debug_instr_t *instrs = csoundDebugGetInstrInstances(csound);
    debug_variable_t *vars = csoundDebugGetVariables(csound, instrs);
    debug_variable_t *arr = findVar(vars, "SArr");
    ASSERT_NE(arr, nullptr);
    ASSERT_STREQ(arr->typeName, "[");

    debug_array_info_t info;
    MYFLT buf[16];
    int32_t total = csoundDebugSerializeArray(csound, arr->data, buf, 16, &info);
    ASSERT_EQ(total, 0);
    ASSERT_STREQ(info.elementTypeName, "S");

    csoundDebugFreeVariables(csound, vars);
    csoundDebugFreeInstrInstances(csound, instrs);
}

TEST_F (DebugFsigGlobalsArraysTests, testSerializeArrayNullSafe)
{
    debug_array_info_t info;
    MYFLT buf[4];
    ASSERT_EQ(csoundDebugSerializeArray(csound, NULL, buf, 4, &info), 0);
    ASSERT_EQ(csoundDebugSerializeArray(csound, NULL, buf, 4, NULL), 0);
}
