#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cmath>
#include <string>
#include <vector>

namespace {
std::vector<MYFLT> render(const std::string &body, int samples = 100,
                          const char *score = "i1 0 10")
{
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n");
    const std::string orc =
        "sr=1000\nksmps=1\nnchnls=1\n0dbfs=1\n"
        "giFlat ftgen 0,0,4,-2,1,1,1,1\n"
        "giWave ftgen 0,0,4,-2,0,1,0,-1\n"
        "giOdd ftgen 0,0,-5,-2,0,1,0,-1,0\n"
        "instr 1\nseed 12345\n" + body +
        "\nchnset kValue, \"value\"\nendin\n";
    std::vector<MYFLT> result;
    int status = csoundCompileOrc(csound, orc.c_str(), 0);
    EXPECT_EQ(status, 0);
    if (status == 0) {
        csoundEventString(csound, score, 0);
        status = csoundStart(csound);
        EXPECT_EQ(status, 0);
        for (int i = 0; status == 0 && i < samples; ++i) {
            status = csoundPerformKsmps(csound);
            EXPECT_EQ(status, 0);
            int error = 0;
            result.push_back(csoundGetControlChannel(csound, "value", &error));
            EXPECT_EQ(error, 0);
        }
    }
    EXPECT_EQ(csound->inerrcnt, 0);
    EXPECT_EQ(csound->perferrcnt, 0);
    csoundDestroy(csound);
    return result;
}

TEST(VibratoTests, IndependentDeviationRates)
{
    const auto amplitude = render(
        "kValue vibrato 1,0,1,0,100,100,0,0,giFlat");
    ASSERT_EQ(amplitude.size(), 100u);
    EXPECT_EQ(amplitude.front(), 1);
    bool changed = false;
    for (auto value : amplitude) {
        changed |= std::abs(value - 1) > .01;
        EXPECT_GE(value, .5);
        EXPECT_LE(value, 2);
    }
    EXPECT_TRUE(changed);
    const auto frequency = render(
        "kValue vibrato 1,10,0,1,0,0,100,100,giWave");
    const auto plain = render(
        "kValue vibrato 1,10,0,0,0,0,0,0,giWave");
    ASSERT_EQ(frequency.size(), plain.size());
    changed = false;
    for (size_t i = 0; i < frequency.size(); ++i)
        changed |= std::abs(frequency[i] - plain[i]) > .01;
    EXPECT_TRUE(changed);
}

TEST(VibratoTests, FractionalPhaseForAnyTableLength)
{
    for (const auto *args : {"giWave,.125", "giOdd,.1"}) {
        const auto values = render(
            std::string("kValue vibrato 1,0,0,0,0,0,0,0,") + args);
        ASSERT_EQ(values.size(), 100u);
        for (auto value : values) EXPECT_NEAR(value, .5, 1e-6);
    }
}

TEST(VibratoTests, NegativePhasePreservesTablePositionOnReinit)
{
    const auto values = render(
        "kCycle timeinstk\n"
        "if kCycle == 21 then\nreinit AGAIN\nendif\n"
        "AGAIN:\nkValue vibrato 1,10,0,0,0,0,0,0,giWave,-1\nrireturn");
    const auto reference = render("kValue vibrato 1,10,0,0,0,0,0,0,giWave");
    EXPECT_EQ(values, reference);
}

TEST(VibratoTests, ReusedNotesResetRandomState)
{
    for (const auto *body : {
             "kValue vibrato 1,10,1,1,100,100,150,150,giWave",
             "kValue vibr 1,10,giWave"}) {
        const auto values = render(body, 4100, "i1 0 2\ni1 2.1 2\nf0 5");
        ASSERT_EQ(values.size(), 4100u);
        for (int i = 0; i < 2000; ++i)
            ASSERT_EQ(values[i], values[i + 2100]) << "sample " << i;
    }
}

TEST(VibratoTests, DeviationRatesStayBounded)
{
    for (const auto *rate : {"-100", "1000000", "1e30"}) {
        const std::string r(rate);
        const auto values = render("kValue vibrato 1,0,1,0," + r + "," +
                                   r + ",0,0,giFlat");
        ASSERT_EQ(values.size(), 100u);
        for (auto value : values) {
            EXPECT_GE(value, .5);
            EXPECT_LE(value, 2);
        }
    }
}
} // namespace
