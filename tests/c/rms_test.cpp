#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "ugens5.h"
#include "gtest/gtest.h"
#include <cfenv>
#include <cmath>
#include <limits>

#if defined(__SSE__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#include <xmmintrin.h>
#define RMS_TEST_SSE
#endif

extern "C" {
int32_t rmsset(CSOUND *, RMS *);
int32_t rms(CSOUND *, RMS *);
int32_t gainset(CSOUND *, GAIN *);
int32_t gain(CSOUND *, GAIN *);
int32_t balnset(CSOUND *, BALANCE *);
int32_t balance(CSOUND *, BALANCE *);
int32_t balance2(CSOUND *, BALANCE *);
}

namespace {
class RmsTests : public ::testing::Test {
protected:
    INSDS instrument{};
    RMS detector{};
    cs_float input = 0, output = 0, cutoff = 24000, keep = 0;
    std::fenv_t environment{};
#ifdef RMS_TEST_SSE
    unsigned int mxcsr = 0;
#endif

    void SetUp() override {
        ASSERT_EQ(std::fegetenv(&environment), 0);
#ifdef RMS_TEST_SSE
        mxcsr = _mm_getcsr();
#endif
        // Exercise gradual underflow, as on wasm, even when the native build
        // starts with fast-math's flush-to-zero modes enabled.
        ASSERT_EQ(std::fesetenv(FE_DFL_ENV), 0);
#ifdef RMS_TEST_SSE
        _mm_setcsr(_mm_getcsr() & ~0x8040u); // Clear FTZ and DAZ.
#endif
        instrument.ksmps = 1;
        instrument.pidsr = PI/48000;
        detector.h.insdshead = &instrument;
        detector.asig = &input;
        detector.kr = &output;
        detector.ihp = &cutoff;
        detector.istor = &keep;
        ASSERT_EQ(rmsset(nullptr, &detector), OK);
    }

    void TearDown() override {
        std::fesetenv(&environment);
#ifdef RMS_TEST_SSE
        _mm_setcsr(mxcsr);
#endif
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
    EXPECT_EQ(step(3*unit), 3*unit);
    EXPECT_EQ(step(0), unit);
}

TEST_F(RmsTests, SmallestWeightsStillAccumulateEnergy)
{
    cutoff = std::numeric_limits<cs_float>::denorm_min()*8192;
    ASSERT_EQ(rmsset(nullptr, &detector), OK);
    const cs_float first = step(FL(1e15));
    ASSERT_GT(first, 0);
    for (int n = 0; n < 100; ++n) step(FL(6e14));
    EXPECT_NEAR(output/first, std::sqrt(FL(37.0)), .00001);
}

TEST_F(RmsTests, SilentDecayCanResumeAtTheSmallestInput)
{
    step(1);
    for (int n = 0; n < 3000; ++n) step(0);
    EXPECT_EQ(output, 0);
    const cs_float unit = std::numeric_limits<cs_float>::denorm_min();
    EXPECT_EQ(step(3*unit), 3*unit);
}

TEST_F(RmsTests, FullScaleInputsStayFinite)
{
    const cs_float maximum = (std::numeric_limits<cs_float>::max)();
    for (cs_float frequency : {FL(.0001), FL(10.0), FL(24000.0)}) {
        cutoff = frequency;
        ASSERT_EQ(rmsset(nullptr, &detector), OK);
        for (int n = 0; n < 1000; ++n) {
            step(maximum);
            ASSERT_TRUE(std::isfinite(output)) << frequency << ": " << n;
            ASSERT_LE(output, maximum);
        }
    }
}

TEST_F(RmsTests, LowCutoffPulseMatchesBothBalancePaths)
{
    cutoff = FL(.0001);
    ASSERT_EQ(rmsset(nullptr, &detector), OK);
    cs_float source = 1, chain = 0, balanced = 0, perSample = 0;
    GAIN chainOp{};
    chainOp.h.insdshead = &instrument;
    chainOp.asig = &source;
    chainOp.krms = &output;
    chainOp.ar = &chain;
    chainOp.ihp = &cutoff;
    chainOp.istor = &keep;
    ASSERT_EQ(gainset(nullptr, &chainOp), OK);
    BALANCE balanceOp{};
    balanceOp.h.insdshead = &instrument;
    balanceOp.asig = &source;
    balanceOp.csig = &input;
    balanceOp.ar = &balanced;
    balanceOp.ihp = &cutoff;
    balanceOp.istor = &keep;
    ASSERT_EQ(balnset(nullptr, &balanceOp), OK);
    BALANCE perSampleOp = balanceOp;
    perSampleOp.ar = &perSample;
    cs_float previousPerSample = 0;
    for (int n = 0; n < 48000; ++n) {
        step(n == 0 ? FL(1.0) : FL(0.0));
        ASSERT_EQ(gain(nullptr, &chainOp), OK);
        ASSERT_EQ(balance(nullptr, &balanceOp), OK);
        previousPerSample = perSample;
        ASSERT_EQ(balance2(nullptr, &perSampleOp), OK);
    }
    ASSERT_GT(chain, 0);
    EXPECT_NEAR(chain/balanced, 1, .000002);
    // gain and balance ramp from the preceding block's level; ksmps is one.
    EXPECT_NEAR(chain/previousPerSample, 1, .000002);
}
} // namespace
