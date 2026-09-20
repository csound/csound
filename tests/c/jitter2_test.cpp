#include "gtest/gtest.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#define __BUILDING_LIBCSOUND
#include "csoundCore.h"

namespace {

class Jitter2Tests : public ::testing::TestWithParam<int> {
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

    std::string jitter(const std::string &controls)
    {
        return "kValue jitter2 " + controls + "," +
               std::to_string(GetParam()) + "\n";
    }

    void start(const std::string &body, const char *score = "i 1 0 .2")
    {
        const std::string orc =
            "sr=1000\nksmps=1\nnchnls=1\n0dbfs=1\ninstr 1\n" + body +
            "\nchnset kValue,\"value\"\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), CSOUND_SUCCESS)
            << messages();
        csoundEventString(csound, score, 0);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS) << messages();
    }

    std::vector<MYFLT> trace(int count)
    {
        std::vector<MYFLT> result;
        for (int i = 0; i < count; ++i) {
            EXPECT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
            int error = 0;
            result.push_back(csoundGetControlChannel(csound, "value", &error));
            EXPECT_EQ(error, CSOUND_SUCCESS);
        }
        EXPECT_EQ(csound->inerrcnt, 0) << messages();
        EXPECT_EQ(csound->perferrcnt, 0) << messages();
        return result;
    }

    CSOUND *csound = nullptr;
};

TEST_P(Jitter2Tests, ReusedNotesMatchTheSameSeed)
{
    ASSERT_NO_FATAL_FAILURE(start("seed 12345\n" +
        jitter("1,.5,100,.3,200,.2,400"),
        "i 1 0 .032\ni 1 .04 .032\nf 0 .08"));
    const auto first = trace(32);
    trace(8);
    EXPECT_EQ(trace(32), first);
    EXPECT_TRUE(std::any_of(first.begin(), first.end(),
                           [](MYFLT value) { return value != 0; }));
}

TEST_P(Jitter2Tests, ReinitMatchesTheSameSeed)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "if timeinstk() == 33 then\nreinit Restart\nendif\nRestart:\n"
        "seed 12345\n" + jitter("1,.5,100,.3,200,.2,400") + "rireturn"));
    const auto first = trace(32);
    EXPECT_EQ(trace(32), first);
}

TEST_P(Jitter2Tests, ThirdRateDoesNotSelectDefaultAmplitudes)
{
    ASSERT_NO_FATAL_FAILURE(start("seed 12345\n" + jitter("1,0,0,0,0,0,10")));
    for (MYFLT value : trace(32)) EXPECT_EQ(value, 0);
}

TEST_P(Jitter2Tests, DefaultSelectionFollowsControlChanges)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "seed 12345\nkRate init 0\n"
        "kRate = (timeinstk() >= 5 && timeinstk() <= 10 ? 10 : 0)\n" +
        jitter("1,0,0,0,0,0,kRate")));
    const auto values = trace(16);
    for (int i = 4; i < 10; ++i) EXPECT_EQ(values[i], 0);
    if (GetParam() != 0) {
        EXPECT_NE(values[3], 0);
        EXPECT_NE(values[10], 0);
    }
}

TEST_P(Jitter2Tests, LargeAndNegativeRatesKeepOutputBounded)
{
    ASSERT_NO_FATAL_FAILURE(start("seed 12345\n" +
        jitter("1,.5,1e30,.3,2500,.2,-10")));
    for (MYFLT value : trace(128)) {
        EXPECT_TRUE(std::isfinite(value));
        EXPECT_LE(std::abs(value), FL(1.0));
    }
}

TEST_P(Jitter2Tests, HistoricalDefaultsMatchExplicitControls)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "seed 12345\nkAmp1 init p4 * .5\nkAmp2 init p4 * .3\n"
        "kAmp3 init p4 * .2\nkCps1 init p4 * .82071231913\n"
        "kCps2 init p4 * 7.009019029039107\nkCps3 init p4 * 10\n" +
        jitter("1,kAmp1,kCps1,kAmp2,kCps2,kAmp3,kCps3"),
        "i 1 0 .128 0\ni 1 .128 .128 1\nf 0 .26"));
    const auto defaults = trace(128);
    EXPECT_EQ(trace(128), defaults);
}

INSTANTIATE_TEST_SUITE_P(OnsetModes, Jitter2Tests, ::testing::Values(0, 1));

} // namespace
