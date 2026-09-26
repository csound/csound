#include "csound.h"
#include "gtest/gtest.h"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

class ClipTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), 0);
    ASSERT_EQ(csoundCompileOrc(csound, R"(
      sr = 8192
      ksmps = 16
      nchnls = 2
      0dbfs = 1
      instr 1
        kInput chnget "input"
        aInput = kInput
        kReset chnget "reset"
        if changed(kReset) == 1 && kReset != 0 then
          reinit SETUP
        endif
      SETUP:
        iMethod chnget "method"
        iLimit chnget "limit"
        iShape chnget "shape"
        aSeparate clip aInput, iMethod, iLimit, iShape
        aInput clip aInput, iMethod, iLimit, iShape
        rireturn
        out aSeparate, aInput
      endin
    )", 0), 0);
  }

  void TearDown() override { csoundDestroy(csound); }

  void StartClip(MYFLT method, MYFLT limit, MYFLT shape = .5)
  {
    csoundSetControlChannel(csound, "method", method);
    csoundSetControlChannel(csound, "limit", limit);
    csoundSetControlChannel(csound, "shape", shape);
    csoundEventString(csound, "i1 0 1\n", 0);
    ASSERT_EQ(csoundStart(csound), 0);
  }

  // Evaluate the documented curves at unit limit, then scale the answer.
  // This keeps the reference independent of the opcode's coefficients.
  static double UnitCurve(double input, int method, double shape)
  {
    double magnitude = std::fabs(input);
    double output;
    if (method == 0) {
      if (magnitude <= shape)
        output = magnitude;
      else if (magnitude >= 1)
        output = (1 + shape) / 2;
      else {
        double distance = magnitude - shape;
        double fraction = distance / (1 - shape);
        output = shape + distance / (1 + fraction * fraction);
      }
    }
    else if (magnitude >= 1)
      output = 1;
    else if (method == 1)
      output = std::sin(std::acos(-1.0) * magnitude / 2);
    else
      output = std::tanh(magnitude) / std::tanh(1.0);
    return std::copysign(output, input);
  }

  void ExpectCurve(int method, MYFLT limit, MYFLT shape = .5)
  {
    // Cover the straight segment, both knees, the curved segment, and clipping.
    for (double input : {-1.125, -1.0, -.75, -.5, 0.0, .5, .75, 1.0, 1.125}) {
      SCOPED_TRACE(input);
      csoundSetControlChannel(csound, "input", (limit == 0 ? 1 : limit) * input);
      ASSERT_EQ(csoundPerformKsmps(csound), 0);
      const MYFLT *out = csoundGetSpout(csound);
      const double expected = limit * UnitCurve(input, method, shape);
      for (int n = 0; n < 16; ++n) {
        for (int channel = 0; channel < 2; ++channel) {
          SCOPED_TRACE(::testing::Message() << "sample " << n
                       << ", channel " << channel);
          ASSERT_TRUE(std::isfinite(out[2 * n + channel]));
          ASSERT_NEAR(out[2 * n + channel], expected, limit * 2e-6);
        }
      }
    }
  }
};

struct ClipCase {
  int method;
  MYFLT limit;
  MYFLT shape;
};

class ClippingCurves : public ClipTests,
                       public ::testing::WithParamInterface<ClipCase> {};

TEST_P(ClippingCurves, MatchesTheCurveWithSeparateAndInPlaceOutput)
{
  const ClipCase &test = GetParam();
  SCOPED_TRACE(::testing::Message() << "method " << test.method
               << ", limit " << test.limit << ", shape " << test.shape);
  StartClip(test.method, test.limit, test.shape);
  ExpectCurve(test.method, test.limit, test.shape);
}

static std::vector<ClipCase> CurveCases()
{
  const MYFLT small = sizeof(MYFLT) == sizeof(float) ? 1e-20 : 1e-200;
  const MYFLT large = sizeof(MYFLT) == sizeof(float) ? 1e20 : 1e200;
  const MYFLT nearMaximum = std::numeric_limits<MYFLT>::max() * .875;
  std::vector<ClipCase> cases;
  for (int method : {0, 1, 2})
    for (MYFLT limit : {MYFLT(0), MYFLT(1), small, large, nearMaximum})
      for (MYFLT shape : {MYFLT(0), MYFLT(.5), MYFLT(1)})
        cases.push_back({method, limit, shape});
  return cases;
}

INSTANTIATE_TEST_SUITE_P(Clip, ClippingCurves,
                        ::testing::ValuesIn(CurveCases()));

TEST_F(ClipTests, UnsupportedMethodUsesBramCurve)
{
  StartClip(3, 1);
  ExpectCurve(0, 1);
}

TEST_F(ClipTests, ReinitializingWithUnsupportedMethodReplacesOldCoefficients)
{
  StartClip(2, 1);
  ExpectCurve(2, 1);
  csoundSetControlChannel(csound, "method", 3);
  csoundSetControlChannel(csound, "reset", 1);
  ExpectCurve(0, 1);
}

class ClipMethodFallback : public ClipTests,
                           public ::testing::WithParamInterface<MYFLT> {};

TEST_P(ClipMethodFallback, UnsupportedValueUsesBramCurve)
{
  StartClip(GetParam(), 1);
  ExpectCurve(0, 1);
}

INSTANTIATE_TEST_SUITE_P(Clip, ClipMethodFallback, ::testing::Values(
    MYFLT(-1), std::numeric_limits<MYFLT>::max(),
    std::numeric_limits<MYFLT>::infinity(),
    std::numeric_limits<MYFLT>::quiet_NaN()));

class ClipInvalidLimit : public ClipTests,
                         public ::testing::WithParamInterface<MYFLT> {};

TEST_P(ClipInvalidLimit, ReportsAnInitializationError)
{
  StartClip(0, GetParam());
  csoundPerformKsmps(csound);
  std::string messages;
  while (csoundGetMessageCnt(csound)) {
    messages += csoundGetFirstMessage(csound);
    csoundPopFirstMessage(csound);
  }
  EXPECT_NE(messages.find("clip: limit must be finite and non-negative"),
            std::string::npos) << messages;
}

INSTANTIATE_TEST_SUITE_P(Clip, ClipInvalidLimit, ::testing::Values(
    MYFLT(-1), std::numeric_limits<MYFLT>::infinity(),
    std::numeric_limits<MYFLT>::quiet_NaN()));
