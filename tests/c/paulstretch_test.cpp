#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

struct FFTProbe {
    int forwards = 0;
    int inverses = 0;
};

void probeFFT(CSOUND *csound, void *setup, MYFLT *buffer)
{
    auto *probe = static_cast<FFTProbe *>(csoundGetHostData(csound));
    auto *fft = static_cast<CSOUND_FFT_SETUP *>(setup);
    if (fft->d == FFT_FWD) {
        ++probe->forwards;
        std::fill(buffer, buffer + fft->N, FL(0.0));
        buffer[1] = FL(8.0);  // Nyquist, not DC's imaginary component.
        buffer[2] = FL(3.0);
        buffer[3] = FL(4.0);
    }
    else {
        ++probe->inverses;
        EXPECT_EQ(buffer[0], FL(0.0));
        EXPECT_GT(std::abs(buffer[1]), FL(0.0));
        EXPECT_LE(std::abs(buffer[1]), FL(8.0));
        const double tolerance = sizeof(MYFLT) == sizeof(float) ? 1e-5 : 1e-12;
        EXPECT_NEAR(std::hypot(buffer[2], buffer[3]), 5.0, tolerance);
        std::fill(buffer, buffer + fft->N, FL(0.0));
    }
}

class PaulstretchTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        csound = csoundCreate(&probe, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
        ASSERT_EQ(csoundSetOption(csound, "--fftlib=0"), CSOUND_SUCCESS);
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

    void compile(const std::string &stretch, const std::string &window)
    {
        std::string orchestra =
            "sr = 1024\nksmps = 8\nnchnls = 1\n0dbfs = 1\n"
            "giSource ftgen 1, 0, 32, 10, 1\n"
            "instr 1\naout paulstretch " + stretch + ", " + window +
            ", giSource\nout aout\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS)
            << messages();
        csoundEventString(csound, "i 1 0 0.1", 0);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS) << messages();
    }

    FFTProbe probe;
    CSOUND *csound = nullptr;
};

TEST_F(PaulstretchTests, UnpacksRealFFTBeforeRandomizingPhase)
{
    ASSERT_NO_FATAL_FAILURE(compile("2", "16/1024"));
    csound->RealFFT = probeFFT;
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
    EXPECT_EQ(probe.forwards, 1);
    EXPECT_EQ(probe.inverses, 1);
}

TEST_F(PaulstretchTests, RoundsOddWindowsToEvenSamples)
{
    ASSERT_NO_FATAL_FAILURE(compile("2", "17/1024"));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
}

TEST_F(PaulstretchTests, TinyStretchReachesEndAndStaysSilent)
{
    ASSERT_NO_FATAL_FAILURE(compile("1e-30", "16/1024"));
    bool heardSource = false;
    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
        const MYFLT *output = csoundGetSpout(csound);
        for (uint32_t j = 0; j < csoundGetKsmps(csound); ++j) {
            if (i < 2) {
                EXPECT_TRUE(std::isfinite(output[j]));
                heardSource |= output[j] != FL(0.0);
            }
            else
                EXPECT_EQ(output[j], FL(0.0));
        }
    }
    EXPECT_TRUE(heardSource);
}

TEST_F(PaulstretchTests, RejectsZeroStretch)
{
    ASSERT_NO_FATAL_FAILURE(compile("0", "16/1024"));
    csoundPerformKsmps(csound);
    EXPECT_NE(messages().find("stretch must be finite and positive"),
              std::string::npos);
}

TEST_F(PaulstretchTests, RejectsOversizedWindow)
{
    ASSERT_NO_FATAL_FAILURE(compile("2", "1e20"));
    csoundPerformKsmps(csound);
    EXPECT_NE(messages().find("invalid window size"), std::string::npos);
}

} // namespace
