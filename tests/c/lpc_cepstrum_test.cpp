#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cmath>
#include <string>

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

TEST_F(LPCCepstrumTests, TwoRealPolesHaveCepstralTermsBeyondTheirOrder) {
    cs_float coefficients[] = {2, -.75, .125};
    cs_float cepstrum[8] = {}, restored[3] = {};
    csound->LPCeps(csound, cepstrum, coefficients, 8, 2);
    EXPECT_NEAR(cepstrum[0], std::log(2.), 1e-6);
    // A(z) = (1 - .5 z^-1)(1 - .25 z^-1).
    for (int n = 1; n < 8; ++n)
        EXPECT_NEAR(cepstrum[n], (std::pow(.5, n) + std::pow(.25, n)) / n, 1e-6) << n;
    csound->CepsLP(csound, restored, cepstrum, 2, 8);
    EXPECT_NEAR(restored[0], coefficients[0], 1e-6);
    EXPECT_NEAR(restored[1], coefficients[1], 1e-6);
    EXPECT_NEAR(restored[2], coefficients[2], 1e-6);
}

TEST_F(LPCCepstrumTests, ConjugatePolesRoundTripWithPredictionError) {
    cs_float coefficients[] = {.25, -1, .5};
    cs_float cepstrum[8] = {}, restored[3] = {};
    csound->LPCeps(csound, cepstrum, coefficients, 8, 2);
    EXPECT_NEAR(cepstrum[0], std::log(.25), 1e-6);
    // Poles have radius sqrt(.5) and angles +/- pi/4.
    for (int n = 1; n < 8; ++n) {
        cs_double expected = 2 * std::pow(std::sqrt(.5), n) *
                          std::cos(n * std::acos(-1.) / 4) / n;
        EXPECT_NEAR(cepstrum[n], expected, 1e-6) << n;
    }
    csound->CepsLP(csound, restored, cepstrum, 2, 8);
    for (int n = 0; n < 3; ++n)
        EXPECT_NEAR(restored[n], coefficients[n], 1e-6) << n;
}

TEST_F(LPCCepstrumTests, RejectsOrdersBelowTwoWithoutWritingOutput) {
    cs_float coefficients[] = {2, -.5};
    cs_float cepstrum[] = {0, .5};
    for (int order : {-1, 0, 1}) {
        cs_float output[] = {17, 23};
        EXPECT_EQ(csound->LPCeps(csound, output, coefficients, 2, order), nullptr);
        EXPECT_EQ(output[0], 17);
        EXPECT_EQ(output[1], 23);
        EXPECT_EQ(csound->CepsLP(csound, output, cepstrum, order, 2), nullptr);
        EXPECT_EQ(output[0], 17);
        EXPECT_EQ(output[1], 23);
    }
}

TEST_F(LPCCepstrumTests, RejectsCepstrumShorterThanOrderPlusOne) {
    cs_float cepstrum[] = {0, .5, .125};
    cs_float output[] = {17, 23, 29};
    EXPECT_EQ(csound->CepsLP(csound, output, cepstrum, 2, 2), nullptr);
    EXPECT_EQ(output[0], 17);
    EXPECT_EQ(output[1], 23);
    EXPECT_EQ(output[2], 29);
}

class PVSCepstrumOrderTests : public LPCCepstrumTests,
                            public ::testing::WithParamInterface<int> {};

TEST_P(PVSCepstrumOrderTests, RejectsInvalidOrderAtInit) {
    const std::string csd = R"(<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=1024
ksmps=8
nchnls=1
instr 1
 aIn init 0
 fIn pvsanal aIn, 64, 16, 64, 1
 kCoefs[], kRms, kErr pvscfs fIn, )" + std::to_string(GetParam()) + R"(, 0
endin
</CsInstruments>
<CsScore>
i1 0 .125
</CsScore>
</CsoundSynthesizer>)";
    ASSERT_EQ(csoundCompileCSD(csound, csd.c_str(), 1, 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    for (int n = 0; n < 20 && csoundPerformKsmps(csound) == 0; ++n) {}
    EXPECT_GT(csound->inerrcnt, 0);
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(messages.find("pvscfs: order must be at least 2"), std::string::npos);
}

INSTANTIATE_TEST_SUITE_P(InvalidOrders, PVSCepstrumOrderTests,
                        ::testing::Values(0, 1, 64));

}
