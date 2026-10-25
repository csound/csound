#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "fftlib.h"
#include "gtest/gtest.h"
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace {
class FftBackendTests : public ::testing::TestWithParam<int> {
protected:
    CSOUND *csound;

    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        const std::string option = "--fftlib=" + std::to_string(GetParam());
        ASSERT_EQ(0, csoundSetOption(csound, option.c_str()));
    }

    void TearDown() override { csoundDestroy(csound); }
};

TEST_P(FftBackendTests, PackedBinsAndInverseScale)
{
    // Include the small and non-power-of-two fallback paths, and a size
    // whose scratch buffer is too large for a small audio-thread stack.
    for (int size : {16, 32, 64, 96, 1024, 65536}) {
        SCOPED_TRACE(size);
        std::vector<cs_float> storage(size + 3);
        // Callers promise sample alignment, not SIMD alignment.
        cs_float *data = storage.data() + 1;
        const cs_double pi = std::acos(-1.0);
        for (int i = 0; i < size; ++i)
            data[i] = FL(0.25) + FL(0.125) * (i % 2 ? -1 : 1) +
                      FL(0.0625) * std::cos(2 * pi * 3 * i / size);
        const std::vector<cs_float> input(data, data + size);
        void *forward = csound->RealFFTSetup(csound, size, FFT_FWD);
        void *inverse = csound->RealFFTSetup(csound, size, FFT_INV);
        csound->RealFFT(csound, forward, data);
        const cs_double tolerance = 32 * std::numeric_limits<cs_float>::epsilon();
        EXPECT_NEAR(size * .25, data[0], size * tolerance);
        EXPECT_NEAR(size * .125, data[1], size * tolerance);
        EXPECT_NEAR(size * .03125, data[6], size * tolerance);
        EXPECT_NEAR(0, data[7], size * tolerance);
        csound->RealFFT(csound, inverse, data);
        for (int i = 0; i < size; ++i)
            ASSERT_NEAR(input[i], data[i], tolerance) << "sample " << i;
    }
}

#ifdef USE_DOUBLE
TEST_P(FftBackendTests, KeepsDifferencesBelowFloatPrecision)
{
    const int size = 64;
    const cs_float step = std::ldexp(FL(1.0), -30);
    cs_float data[size + 2] = {};
    for (int i = 0; i < size; ++i) data[i] = 1 + (i % 2) * step;
    void *forward = csound->RealFFTSetup(csound, size, FFT_FWD);
    void *inverse = csound->RealFFTSetup(csound, size, FFT_INV);
    csound->RealFFT(csound, forward, data);
    EXPECT_NEAR(-size * step / 2, data[1], 1.e-14);
    csound->RealFFT(csound, inverse, data);
    for (int i = 0; i < size; ++i)
        EXPECT_NEAR(1 + (i % 2) * step, data[i], 1.e-14);
}

TEST_P(FftBackendTests, DctAlsoKeepsDoublePrecision)
{
    const int size = 16;
    const cs_float step = std::ldexp(FL(1.0), -30);
    cs_float data[size];
    for (int i = 0; i < size; ++i) data[i] = 1 + (i % 2) * step;
    void *forward = csound->DCTSetup(csound, size, FFT_FWD);
    void *inverse = csound->DCTSetup(csound, size, FFT_INV);
    csound->DCT(csound, forward, data);
    EXPECT_NEAR(2 * size + size * step, data[0], 1.e-13);
    csound->DCT(csound, inverse, data);
    for (int i = 0; i < size; ++i)
        EXPECT_NEAR(1 + (i % 2) * step, data[i], 1.e-14);
}
#endif

#if defined(__MACH__)
INSTANTIATE_TEST_SUITE_P(Backends, FftBackendTests, ::testing::Values(0, 1, 2));
#else
INSTANTIATE_TEST_SUITE_P(Backends, FftBackendTests, ::testing::Values(0, 1));
#endif
}
