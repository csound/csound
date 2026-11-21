#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cmath>
#include <limits>

extern "C" void csound_test_scale2_step(cs_double, cs_double, cs_float,
                                        cs_double *, cs_double *);

TEST(Scale2Tests, RetainsSmallUpdatesAcrossOverflowingDifference)
{
    const cs_float magnitude = (std::numeric_limits<cs_float>::max)()*FL(.6);
    for (int exponent : {25, 26, 27, 28}) {
        SCOPED_TRACE(exponent);
        const cs_double coefficient = std::ldexp(cs_double(1), -exponent);
        cs_double state = -magnitude, correction = 0;
        const cs_double initial = state;
        // Set a reached state directly, avoiding millions of control periods.
        // The first update for 2^-25 exceeds half an ULP of that state.
        csound_test_scale2_step(coefficient, 1-coefficient, magnitude,
                                &state, &correction);
        if (exponent == 25) EXPECT_GT(state, initial);
        for (int step = 1; step < 64; ++step)
            csound_test_scale2_step(coefficient, 1-coefficient, magnitude,
                                    &state, &correction);
        // Over 64 periods the curvature is below 2 ppm. Compare the stored
        // value and its correction in scaled units, retaining both parts.
        volatile cs_double fraction = 128*coefficient;
        volatile cs_double expected = magnitude*fraction;
        volatile cs_double moved = state-initial;
        volatile cs_double corrected = moved-correction;
        EXPECT_NEAR(corrected/expected, 1, .00001);
        EXPECT_GT(state, initial);
        EXPECT_TRUE(std::isfinite(state));
    }
}
