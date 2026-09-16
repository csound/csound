#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <utility>

namespace {
uint32_t clockSeed;

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
