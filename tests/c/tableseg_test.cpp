#include "gtest/gtest.h"

#include <algorithm>
#include <string>

#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "../../Opcodes/pvoc.h"

namespace {

class TablesegTests : public ::testing::TestWithParam<const char *> {
protected:
    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    }

    void TearDown() override { csoundDestroy(csound); }

    std::string messages()
    {
        std::string result;
        while (csoundGetMessageCnt(csound)) {
            result += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return result;
    }

    void compile(const std::string &segments, const std::string &extra = "",
                 const std::string &prefix = "",
                 const std::string &events = "i 1 0 .25")
    {
        // Explicit guard points differ from the first element of each table.
        std::string orchestra =
            "sr=8192\nksmps=16\nnchnls=1\n0dbfs=1\n"
            "giA ftgen 1,0,9,-7,0,8,8\n"
            "giB ftgen 2,0,9,-7,10,8,18\n"
            "giC ftgen 3,0,9,-7,20,8,28\n"
            "giShort ftgen 4,0,4,-2,1,1,1,1\n"
            "instr 1\n" + prefix + std::string(GetParam()) + " " + segments +
            "\n" + extra + "\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS)
            << messages();
        csoundEventString(csound, events.c_str(), 0);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS) << messages();
    }

    TABLESEG *envelope()
    {
        auto *globals = static_cast<PVOC_GLOBALS *>(
            csound->QueryGlobalVariable(csound, "pvocGlobals"));
        return globals ? globals->tbladr : nullptr;
    }

    void expectTable(double base)
    {
        TABLESEG *p = envelope();
        ASSERT_NE(p, nullptr);
        ASSERT_EQ(p->outfunc->flen, 8u);
        for (uint32_t i = 0; i <= 8; ++i)
            EXPECT_NEAR(p->outfunc->ftable[i], base + i, 1e-5) << "index " << i;
    }

    bool quadratic() { return std::string(GetParam()) == "tablexseg"; }
    CSOUND *csound = nullptr;
};

TEST_P(TablesegTests, RoundedDurationsReachEachBreakpoint)
{
    // 4.75 cycles rounds up to 5; 3.25 cycles rounds down to 3.
    ASSERT_NO_FATAL_FAILURE(compile("1,.00927734375,2,.00634765625,3"));
    for (int cycle = 0; cycle < 12; ++cycle) {
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
        double fraction = cycle < 5 ? cycle / 5.0 : (cycle - 5) / 3.0;
        fraction = (std::min)(fraction, 1.0);
        if (quadratic()) fraction *= fraction;
        const double base = cycle < 5 ? 10 * fraction : 10 + 10 * fraction;
        ASSERT_NO_FATAL_FAILURE(expectTable(base));
    }
}

TEST_P(TablesegTests, ZeroAndSubcycleStagesAdvanceImmediately)
{
    ASSERT_NO_FATAL_FAILURE(compile("1,0,2,.000244140625,3,.0078125,1"));
    for (int cycle = 0; cycle < 8; ++cycle) {
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
        double fraction = (std::min)(cycle / 4.0, 1.0);
        if (quadratic()) fraction *= fraction;
        ASSERT_NO_FATAL_FAILURE(expectTable(20 * (1 - fraction)));
    }
}

TEST_P(TablesegTests, AllZeroStagesHoldTheLastTable)
{
    ASSERT_NO_FATAL_FAILURE(compile("1,0,2,0,3"));
    for (int cycle = 0; cycle < 3; ++cycle) {
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
        ASSERT_NO_FATAL_FAILURE(expectTable(20));
    }
}

TEST_P(TablesegTests, ReinitRestartsAndReusesOutputStorage)
{
    ASSERT_NO_FATAL_FAILURE(compile("1,.0078125,2,.0078125,3",
        "rireturn\nkCycle init 0\nkCycle += 1\n"
        "if kCycle == 7 then\nreinit Envelope\nendif\n", "Envelope:\n",
        "i 1 0 .25\ni 1 .5 .25"));
    // Reused note instances must start at the first table as well.
    MYFLT *storage = nullptr;
    for (int cycle = 0; cycle <= 256; ++cycle) {
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
        if (cycle == 0) {
            ASSERT_NE(envelope(), nullptr);
            storage = envelope()->outfunc->ftable;
        }
        if (cycle == 0 || cycle == 6 || cycle == 7 || cycle == 256) {
            ASSERT_NO_FATAL_FAILURE(expectTable(0));
            EXPECT_EQ(envelope()->outfunc->ftable, storage);
        }
        if (cycle == 8) {
            ASSERT_NO_FATAL_FAILURE(expectTable(quadratic() ? .625 : 2.5));
        }
    }
}

TEST_P(TablesegTests, RejectsMissingTables)
{
    ASSERT_NO_FATAL_FAILURE(compile("1,.01,99"));
    EXPECT_NE(csoundPerformKsmps(csound), CSOUND_SUCCESS);
}

TEST_P(TablesegTests, RejectsDifferentTableSizes)
{
    ASSERT_NO_FATAL_FAILURE(compile("1,.01,4"));
    EXPECT_NE(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_NE(messages().find("tables must have the same size"), std::string::npos);
}

TEST_P(TablesegTests, RejectsNegativeDurations)
{
    ASSERT_NO_FATAL_FAILURE(compile("1,-.01,2"));
    EXPECT_NE(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_NE(messages().find("invalid segment duration"), std::string::npos);
}

TEST_P(TablesegTests, RejectsUnrepresentableDurations)
{
    ASSERT_NO_FATAL_FAILURE(compile("1,1e12,2"));
    EXPECT_NE(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_NE(messages().find("invalid segment duration"), std::string::npos);
}

int32_t loadTestPvx(CSOUND *, const char *, PVOCEX_MEMFILE *file)
{
    static float frame[130] = {};
    *file = PVOCEX_MEMFILE{};
    file->fftsize = 128;
    file->overlap = 16;
    file->winsize = 128;
    file->chans = 1;
    file->srate = 8192;
    file->nframes = 1;
    file->data = frame;
    return OK;
}

TEST_P(TablesegTests, VpvocRejectsAnEnvelopeShorterThanItsSpectrum)
{
    csound->PVOCEX_LoadFile = loadTestPvx;
    ASSERT_NO_FATAL_FAILURE(compile("1,.01,2", "aSig vpvoc 0,1,\"test.pvx\""));
    EXPECT_NE(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_NE(messages().find("spectral envelope table is too short"),
              std::string::npos);
}

TEST_P(TablesegTests, VpvocAcceptsAnEnvelopeCoveringItsSpectrum)
{
    csound->PVOCEX_LoadFile = loadTestPvx;
    ASSERT_NO_FATAL_FAILURE(compile("1,.01,2",
        "iEnvelope ftgen 5,0,64,-7,1,64,1\n"
        "aSig vpvoc 0,1,\"test.pvx\",0,iEnvelope"));
    EXPECT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
}

INSTANTIATE_TEST_SUITE_P(TableEnvelopes, TablesegTests,
                        ::testing::Values("tableseg", "tablexseg"));

} // namespace
