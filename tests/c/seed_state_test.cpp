#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cmath>
#include <utility>

namespace {
uint32_t clockSeed;

TEST(RandomStateTests, GaussianPairSkipsZeroUniformDraw) {
    for (int sigma : {0, 2}) {
        CSOUND *csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        ASSERT_EQ(csoundCompileOrc(csound,
            "sr=48000\nksmps=2\nnchnls=1\n0dbfs=1\n"
            "instr 1\naValue gauss 3, p4\nout aValue\nendin\n", 0),
            CSOUND_SUCCESS);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
        csoundEventString(csound, sigma ? "i 1 0 .01 2" : "i 1 0 .01 0", 0);
        csound->randState_.mti = 0;
        csound->randState_.mt[0] = 0;
        // The next two words temper to 0x80000000, giving uniforms near 0.5.
        csound->randState_.mt[1] = 0x80102204u;
        csound->randState_.mt[2] = 0x80102204u;
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        EXPECT_EQ(csound->randState_.mti, 3);
        EXPECT_NEAR(csoundGetSpout(csound)[0],
                    3.0 - sigma * std::sqrt(2.0 * std::log(2.0)), 1e-5);
        EXPECT_NEAR(csoundGetSpout(csound)[1], 3.0, 1e-5);
        csoundDestroy(csound);
    }
}

TEST(RandomStateTests, BilateralExponentialHandlesMinimumSignedDraw) {
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n");
    ASSERT_EQ(csoundCompileOrc(csound,
        "sr=48000\nksmps=1\nnchnls=1\n0dbfs=1\n"
        "instr 1\naValue bexprnd 1\nout aValue\nendin\n", 0),
        CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 .01", 0);
    // MT tempering maps this word to 0x80000000, which becomes INT32_MIN.
    csound->randState_.mti = 0;
    csound->randState_.mt[0] = 0x80102204u;
    CsoundRandMTState expected = csound->randState_;
    ASSERT_EQ(csoundRandMT(&expected), 0x80000000u);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(csound->randState_.mti, 1);
    EXPECT_EQ(csoundGetSpout(csound)[0], FL(0.0));
    csoundDestroy(csound);
}

class SeedStateTests : public ::testing::TestWithParam<
                           std::pair<uint32_t, int32_t>> {
protected:
    CSOUND *csound = nullptr;

    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        csound->GetRandomSeedFromTime = []() -> uint32_t { return clockSeed; };
    }

    void TearDown() override {
        if (csound != nullptr) csoundDestroy(csound);
    }
};

TEST_P(SeedStateTests, ClockSeedPreservesIntegerBoundaryState) {
    clockSeed = GetParam().first;
    // The clock supplies uint32_t directly, without rounding through MYFLT.
    ASSERT_EQ(csoundCompileOrc(csound, "seed 0\n", 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    EXPECT_EQ(csound->randSeed1, GetParam().second);

    CsoundRandMTState expected{};
    csoundSeedRandMT(&expected, nullptr, clockSeed);
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(csoundRandMT(&csound->randState_), csoundRandMT(&expected));

    int32_t state = csound->randSeed1;
    EXPECT_GT(csoundRand31(&state), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Boundaries, SeedStateTests,
    ::testing::Values(std::make_pair(0u, 1),
                      std::make_pair(1u, 1),
                      std::make_pair(0x7ffffffeu, 1),
                      std::make_pair(0x7fffffffu, 1),
                      std::make_pair(0x80000000u, 2),
                      std::make_pair(0xffffff00u, 2147483394),
                      std::make_pair(0xfffffffcu, 1),
                      std::make_pair(0xffffffffu, 3)));
} // namespace
