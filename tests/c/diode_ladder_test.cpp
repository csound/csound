#include "csound.h"
#include "gtest/gtest.h"

#include <cfenv>
#include <cmath>
#include <initializer_list>

#if defined(__SSE__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#include <xmmintrin.h>
#define DIODE_TEST_SSE
#endif

class DiodeLadderTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;
  std::fenv_t environment{};
#ifdef DIODE_TEST_SSE
  unsigned int mxcsr;
#endif

  void SetUp() override
  {
    ASSERT_EQ(std::fegetenv(&environment), 0);
#ifdef DIODE_TEST_SSE
    mxcsr = _mm_getcsr();
#endif
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), 0);
    // csoundCreate enables DAZ on SSE. Restore gradual underflow for this test,
    // including both input (DAZ) and result (FTZ) handling, before compilation.
    ASSERT_EQ(std::fesetenv(FE_DFL_ENV), 0);
#ifdef DIODE_TEST_SSE
    _mm_setcsr(_mm_getcsr() & ~0x8040u); // Clear FTZ (bit 15) and DAZ (bit 6).
#endif
  }

  void TearDown() override
  {
    csoundDestroy(csound);
    std::fesetenv(&environment);
#ifdef DIODE_TEST_SSE
    _mm_setcsr(mxcsr);
#endif
  }
};

TEST_F(DiodeLadderTests, NormalizedSubnormalSaturationKeepsTheLinearLimit)
{
  if (sizeof(cs_float) != 8)
    GTEST_SKIP() << "These saturation values require double-precision cs_float";

  volatile cs_double tiny = 1e-310;
  volatile cs_double preserved = tiny * 2.0;
  ASSERT_GT(preserved, 0.0) << "Subnormal inputs or results were flushed to zero";
  ASSERT_LT(preserved, 1e-308);

  ASSERT_EQ(csoundCompileOrc(csound, R"(
    sr = 8000
    ksmps = 16
    nchnls = 6
    0dbfs = 1
    instr 1
      kSaturation chnget "saturation"
      aInput oscili .5, 1000
      aCutoff = 1000
      aFeedback = 2
      aReference diode_ladder aInput, 1000, 2, 0
      aKK diode_ladder aInput, 1000, 2, 1, kSaturation
      aAK diode_ladder aInput, aCutoff, 2, 1, kSaturation
      aKA diode_ladder aInput, 1000, aFeedback, 1, kSaturation
      aAA diode_ladder aInput, aCutoff, aFeedback, 1, kSaturation
      aInput diodeladder aInput, aCutoff, aFeedback, 1, kSaturation
      out aReference, aKK, aAK, aKA, aAA, aInput
    endin
  )", 0), 0);
  csoundEventString(csound, "i1 0 1\n", 0);
  ASSERT_EQ(csoundStart(csound), 0);

  cs_double peak = 0.0;
  for (cs_double saturation : {0.0, 1e-310, -1e-310, 1e-308, -1e-308, 0.0}) {
    SCOPED_TRACE(saturation);
    csoundSetControlChannel(csound, "saturation", saturation);
    for (int block = 0; block < 3; ++block) {
      ASSERT_EQ(csoundPerformKsmps(csound), 0);
#ifdef DIODE_TEST_SSE
      ASSERT_EQ(_mm_getcsr() & 0x8040u, 0u);
#endif
      const cs_float *out = csoundGetSpout(csound);
      for (int n = 0; n < 16; ++n) {
        peak = std::fmax(peak, std::fabs(out[n * 6]));
        for (int ch = 1; ch < 6; ++ch) {
          ASSERT_TRUE(std::isfinite(out[n * 6 + ch]));
          ASSERT_NEAR(out[n * 6 + ch], out[n * 6], 3e-6)
              << "block " << block << ", sample " << n << ", channel " << ch;
        }
      }
    }
  }
  EXPECT_GT(peak, 1e-3); // A silent reference must not make the comparison pass.
}
