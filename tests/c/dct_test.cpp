#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cmath>
#include <string>
#include <vector>

namespace {
class DctTests : public ::testing::TestWithParam<int> {
protected:
  void run(const std::string &body, const std::vector<MYFLT> &expected,
           const char *error = nullptr, int cycles = 1)
  {
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    std::string options = "-n -d -m0 --fftlib=" + std::to_string(GetParam());
    csoundSetOption(csound, options.c_str());
    std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\nsr=1024\nksmps=1\n"
      "nchnls=1\n0dbfs=1\ninstr 1\n" + body +
      "\nkLength lenarray kOut\nchnset kLength, \"length\"\nkIndex=0\n"
      "while kIndex < kLength do\nSname sprintfk \"lag%d\", kIndex\n"
      "chnset kOut[kIndex], Sname\nkIndex+=1\nod\nendin\n"
      "</CsInstruments>\n<CsScore>\ni 1 0 .02\ne\n"
      "</CsScore>\n</CsoundSynthesizer>";
    int status = csoundCompileCSD(csound, csd.c_str(), 1, 0);
    EXPECT_EQ(0, status);
    if (status == 0) status = csoundStart(csound);
    EXPECT_EQ(0, status);
    if (status == 0) {
      for (int n = 0; n < cycles; ++n) csoundPerformKsmps(csound);
      if (error) EXPECT_GT(csound->inerrcnt + csound->perferrcnt, 0);
      else {
        EXPECT_EQ(0, csound->inerrcnt);
        EXPECT_EQ(0, csound->perferrcnt);
        EXPECT_EQ(expected.size(), csoundGetControlChannel(csound, "length", nullptr));
        for (size_t n = 0; n < expected.size(); ++n) {
          std::string name = "lag" + std::to_string(n);
          EXPECT_NEAR(expected[n], csoundGetControlChannel(csound, name.c_str(), nullptr),
                      1.e-4) << n;
        }
      }
    }
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
      messages += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    if (error) EXPECT_NE(std::string::npos, messages.find(error));
    if (HasFailure()) ADD_FAILURE() << messages;
    csoundDestroy(csound);
  }

  std::vector<MYFLT> transform(const std::vector<MYFLT> &in, bool inverse = false)
  {
    const double pi = std::acos(-1.0);
    std::vector<MYFLT> out(in.size());
    for (size_t k = 0; k < in.size(); ++k) {
      double sum = 0;
      for (size_t n = 0; n < in.size(); ++n)
        sum += inverse ? in[n] * std::cos(pi * n * (k + .5) / in.size()) * (n ? 1 : .5)
                       : in[n] * std::cos(pi * (n + .5) * k / in.size());
      out[k] = inverse ? sum / in.size() : 2 * sum;
    }
    return out;
  }
};

TEST_P(DctTests, ForwardMatchesDefinition)
{
  run("kIn[] fillarray 1,2,3,4,5,6,7,8\nkOut[] dct kIn", transform({1,2,3,4,5,6,7,8}));
}

TEST_P(DctTests, InverseMatchesDefinition)
{
  run("kIn[] fillarray 1,2,3,4,5,6,7,8\nkOut[] dctinv kIn", transform({1,2,3,4,5,6,7,8}, true));
}

TEST_P(DctTests, InitRateRoundTrip)
{
  run("iIn[] fillarray 1,2,3,4\niForward[] dct iIn\niBack[] dctinv iForward\nkOut[]=iBack",
      {1,2,3,4});
}

TEST_P(DctTests, SingletonRoundTrip)
{
  run("kIn[] fillarray 3\nkForward[] dct kIn\nkOut[] dctinv kForward", {3});
}

TEST_P(DctTests, TwoElementRoundTrip)
{
  run("kIn[] fillarray 3,5\nkForward[] dct kIn\nkOut[] dctinv kForward", {3,5});
}

TEST_P(DctTests, InPlaceRoundTrip)
{
  run("kOut[] fillarray 1,2,3,4\nkOut dct kOut\nkOut dctinv kOut", {1,2,3,4});
}

TEST_P(DctTests, GrowingInputRequiresReinit)
{
  run("kIn[] init 4\nkLarge[] init 8\nkOut[] dct kIn\nkIn=kLarge", {}, "input size changed");
}

TEST_P(DctTests, ShrinkingInverseInputRequiresReinit)
{
  run("kIn[] init 8\nkSmall[] init 4\nkOut[] dctinv kIn\nkIn=kSmall", {}, "input size changed");
}

TEST_P(DctTests, RestoresShorterOutputLength)
{
  run("kIn[] fillarray 1,2,3,4\nkSmall[] init 2\nkNever init 0\nkOut[] dct kIn\n"
      "if kNever == 1 then\nkOut=kSmall\nendif", transform({1,2,3,4}));
}

TEST_P(DctTests, RestoresLargerOutputLength)
{
  run("kIn[] fillarray 1,2,3,4\nkLarge[] init 8\nkNever init 0\nkOut[] dctinv kIn\n"
      "if kNever == 1 then\nkOut=kLarge\nendif", transform({1,2,3,4}, true));
}

TEST_P(DctTests, ReinitializationAcceptsNewSize)
{
  run("iSize init 4\nkCycle init 0\nkCycle+=1\nif kCycle==2 then\nreinit AGAIN\nendif\n"
      "AGAIN:\nkIn[] init iSize\nkIn=1\nkOut[] dct kIn\niSize=8\nrireturn",
      {16,0,0,0,0,0,0,0}, nullptr, 3);
}

TEST_P(DctTests, EmptyInputReportsError)
{
  run("kIn[] init 0\nkOut[] dct kIn", {}, "size must be a power of two");
}

TEST_P(DctTests, NonPowerOfTwoReportsError)
{
  run("kIn[] init 3\nkOut[] dctinv kIn", {}, "size must be a power of two");
}

TEST_P(DctTests, MatrixInputReportsError)
{
  run("kIn[] init 2,2\nkOut[] dct kIn", {}, "expected one-dimensional arrays");
}

TEST_P(DctTests, MatrixOutputReportsError)
{
  run("kIn[] init 4\nkOut[] init 2,2\nkOut dctinv kIn", {}, "expected one-dimensional arrays");
}

INSTANTIATE_TEST_SUITE_P(Backends, DctTests, ::testing::Values(0, 1
#ifdef __MACH__
                                                           , 2
#endif
                                                           ));
}
