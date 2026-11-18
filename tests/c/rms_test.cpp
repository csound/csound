#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "ugens5.h"
#include "gtest/gtest.h"
#include <cmath>
#include <limits>

extern "C" {
int32_t rmsset(CSOUND *, RMS *);
int32_t rms(CSOUND *, RMS *);
}

namespace {
class RmsTests : public ::testing::Test {
protected:
    INSDS instrument{};
    RMS detector{};
    cs_float input = 0, output = 0, cutoff = 24000, keep = 0;

    void SetUp() override {
        instrument.ksmps = 1;
        instrument.pidsr = PI/48000;
        detector.h.insdshead = &instrument;
        detector.asig = &input;
        detector.kr = &output;
        detector.ihp = &cutoff;
        detector.istor = &keep;
        ASSERT_EQ(rmsset(nullptr, &detector), OK);
    }

    cs_float step(cs_float sample) {
        input = sample;
        EXPECT_EQ(rms(nullptr, &detector), OK);
        return output;
    }
};

TEST_F(RmsTests, DecayDoesNotRoundASubnormalScale)
{
    const cs_float unit = std::numeric_limits<cs_float>::denorm_min();
    // The first two levels are 2.73 and 1.13 units, rounded only at output.
    EXPECT_EQ(step(3*unit)/unit, 3);
    EXPECT_EQ(step(0)/unit, 1);
}
} // namespace
