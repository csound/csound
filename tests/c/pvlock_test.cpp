#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <array>
#include <cmath>

extern "C" void csound_test_pvlock(int32_t, int32_t, cs_float, cs_float *,
                                  cs_float *, cs_float *, void *);

namespace {
constexpr int size = 16;
constexpr int bins = size / 2 + 1;
using Frame = std::array<cs_float, size + 2>;

Frame lock(const Frame &input, int mode, cs_float beta)
{
    Frame front = input, back = {}, previous = {};
    // Match the production scratch layout and alignment without a Csound instance.
    alignas(cs_float) char scratch[bins * (sizeof(cs_float) + 2*sizeof(int32_t))];
    for (int b = 0; b < bins; ++b) back[2*b] = previous[2*b] = FL(1.0);
    csound_test_pvlock(size, mode, beta, back.data(), front.data(),
                       previous.data(), scratch);
    return front;
}

TEST(PvlockTests, KeepsSilentBinUnwrapConvention)
{
    const cs_float pi = PI_F;
    const cs_float angles[bins] = {FL(.2), FL(2.0), pi, FL(-2.0),
        FL(-1.4), FL(-.7), FL(.2), FL(.9), FL(1.8)};
    // The zero at bin 2 has atan2(+0, -0) = pi. Unwrap from peak bin 4.
    const cs_float unwrapped[bins] = {FL(.2)-2*pi, FL(2.0)-2*pi, -pi,
        FL(-2.0), FL(-1.4), FL(-.7), FL(.2), FL(.9), FL(1.8)};
    const cs_float amplitude[bins] = {FL(.1), FL(.2), 0, FL(.4), 1,
        FL(.4), FL(.3), FL(.2), FL(.1)};
    Frame input = {};
    for (int b = 0; b < bins; ++b) {
        input[2*b] = amplitude[b] * COS(angles[b]);
        input[2*b+1] = amplitude[b] * SIN(angles[b]);
    }
    input[4] = -FL(0.0);
    input[5] = FL(0.0);
    for (cs_float beta : {FL(.5), FL(1.0), FL(1.5), FL(2.0)}) {
        const auto result = lock(input, 3, beta);
        for (int b = 0; b < bins; ++b) {
            const cs_float angle = angles[b] +
                (beta-FL(1.0))*(unwrapped[b]-unwrapped[4]);
            EXPECT_NEAR(result[2*b], amplitude[b]*COS(angle), 3e-6);
            EXPECT_NEAR(result[2*b+1], amplitude[b]*SIN(angle), 3e-6);
        }
    }
}

TEST(PvlockTests, PreservesScaledPhaseAndMagnitude)
{
    // A smooth phase ramp crosses atan2's cut on both sides of peak bin 4.
    for (cs_float offset : {FL(-2.9), FL(.2), FL(2.9)}) {
        Frame input = {};
        for (int b = 0; b < bins; ++b) {
            const cs_float amplitude = FL(1.0)/(1+std::abs(b-4));
            const cs_float angle = offset + FL(1.1)*(b-4);
            input[2*b] = amplitude*COS(angle);
            input[2*b+1] = amplitude*SIN(angle);
        }
        for (cs_float beta : {FL(0.0), FL(.5), FL(1.0), FL(1.5), FL(2.0)}) {
            const auto result = lock(input, 3, beta);
            for (int b = 0; b < bins; ++b) {
                const cs_float amplitude = FL(1.0)/(1+std::abs(b-4));
                const cs_float angle = offset + beta*FL(1.1)*(b-4);
                EXPECT_NEAR(result[2*b], amplitude*COS(angle), 3e-6);
                EXPECT_NEAR(result[2*b+1], amplitude*SIN(angle), 3e-6);
            }
        }
        EXPECT_EQ(lock(input, 2, FL(.5)), lock(input, 3, FL(1.0)));
    }
}

#ifdef USE_FLOAT
TEST(PvlockTests, UnwrapsTheSignOfANearHalfTurn)
{
    Frame input = {FL(.1), 0, FL(.2), 0, FL(.3), 0, FL(.4), 0, 1, 0,
                   FL(.25), FL(.125), FL(-.125),
                   std::nextafter(FL(-.0625), FL(0.0)), FL(.07), 0, FL(.03), 0};
    const auto result = lock(input, 3, FL(.5));
    // The turn from bin 5 to bin 6 is positive and just below pi.
    EXPECT_NEAR(result[12], FL(-.03210894), 1e-7);
    EXPECT_NEAR(result[13], FL(.13601568), 1e-7);
}
#endif

TEST(PvlockTests, KeepsSilenceAndNoPeakFallback)
{
    Frame silence = {};
    EXPECT_EQ(lock(silence, 3, FL(.5)), silence);
    Frame flat = {};
    for (int b = 0; b < bins; ++b) flat[2*b] = FL(.5);
    EXPECT_EQ(lock(flat, 3, FL(.5)), flat);
}
}
