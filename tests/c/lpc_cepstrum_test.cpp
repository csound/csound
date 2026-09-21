#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cmath>

namespace {
class LPCCepstrumTests : public ::testing::Test {
protected:
    CSOUND *csound;
    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
    }
    void TearDown() override { csoundDestroy(csound); }
};

TEST_F(LPCCepstrumTests, OnePoleHasCepstralTermsBeyondItsOrder) {
    MYFLT coefficients[] = {2, -.5};
    MYFLT cepstrum[8] = {}, restored[2] = {};
    csound->LPCeps(csound, cepstrum, coefficients, 8, 1);
    EXPECT_NEAR(cepstrum[0], std::log(2.), 1e-6);
    // -log(1 - .5 z^-1) = sum(.5^n z^-n / n).
    for (int n = 1; n < 8; ++n)
        EXPECT_NEAR(cepstrum[n], std::pow(.5, n) / n, 1e-6) << n;
    csound->CepsLP(csound, restored, cepstrum, 1, 8);
    EXPECT_NEAR(restored[0], coefficients[0], 1e-6);
    EXPECT_NEAR(restored[1], coefficients[1], 1e-6);
}

TEST_F(LPCCepstrumTests, ConjugatePolesRoundTripWithPredictionError) {
    MYFLT coefficients[] = {.25, -1, .5};
    MYFLT cepstrum[8] = {}, restored[3] = {};
    csound->LPCeps(csound, cepstrum, coefficients, 8, 2);
    EXPECT_NEAR(cepstrum[0], std::log(.25), 1e-6);
    // Poles have radius sqrt(.5) and angles +/- pi/4.
    for (int n = 1; n < 8; ++n) {
        double expected = 2 * std::pow(std::sqrt(.5), n) *
                          std::cos(n * std::acos(-1.) / 4) / n;
        EXPECT_NEAR(cepstrum[n], expected, 1e-6) << n;
    }
    csound->CepsLP(csound, restored, cepstrum, 2, 8);
    for (int n = 0; n < 3; ++n)
        EXPECT_NEAR(restored[n], coefficients[n], 1e-6) << n;
}
}
