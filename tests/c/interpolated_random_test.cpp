#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cmath>
#include <string>
#include <vector>

namespace {
class InterpolatedRandomTests : public ::testing::TestWithParam<const char *> {
protected:
  std::vector<cs_float> render(int block, const std::string &rate,
                           bool audioRate = true, bool audioAmp = false,
                           bool alias = false, bool partial = false,
                           bool controlOutput = false)
  {
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0 --sample-accurate");
    const std::string rateName = audioRate ? "aRate" : "kRate";
    const std::string amp = audioAmp ? "aAmp" : "1";
    const std::string output = controlOutput ? (alias ? "kRate" : "kOut") : alias ? "aRate" : "aOut";
    const std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\nsr=1024\nksmps=" +
      std::to_string(block) + "\nnchnls=1\n0dbfs=1\nseed 12345\ninstr 1\n"
      "aPhase phasor 128\naAmp = .5 + aPhase\n" + rateName + " = " + rate +
      "\n" + output + " " + GetParam() + " 1, " + amp + ", " + rateName +
      "\n" + (controlOutput ? "aOut upsamp " + output + "\n" : "") +
      "out " + (controlOutput ? "aOut" : output) +
      "\nendin\n</CsInstruments>\n<CsScore>\n" +
      (partial ? "i 1 .0029296875 .01953125\n" : "i 1 0 .03125\n") +
      "f 0 .03125\n</CsScore>\n</CsoundSynthesizer>";
    int err = csoundCompileCSD(csound, csd.c_str(), 1, 0);
    EXPECT_EQ(0, err);
    if (err == 0) err = csoundStart(csound);
    EXPECT_EQ(0, err);
    std::vector<cs_float> result;
    if (err == 0) {
      for (int frame = 0; frame < 32; frame += block) {
        csoundPerformKsmps(csound);
        const cs_float *out = csoundGetSpout(csound);
        result.insert(result.end(), out, out + block);
      }
      EXPECT_EQ(0, csound->perferrcnt);
      EXPECT_EQ(0, csound->inerrcnt);
    }
    if (HasFailure()) {
      while (csoundGetMessageCnt(csound)) {
        ADD_FAILURE() << csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
      }
    }
    csoundDestroy(csound);
    return result;
  }

  void same(const std::vector<cs_float> &expected, const std::vector<cs_float> &actual)
  {
    ASSERT_EQ(32u, expected.size());
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      EXPECT_TRUE(std::isfinite(actual[i])) << i;
      EXPECT_EQ(expected[i], actual[i]) << "sample " << i;
    }
  }
};

TEST_P(InterpolatedRandomTests, AudioRateDoesNotDependOnBlockSize)
{
  same(render(1, "64 + 128*aPhase"), render(8, "64 + 128*aPhase"));
}

TEST_P(InterpolatedRandomTests, PartialBlockUsesFirstActiveRate)
{
  auto expected = render(1, "64 + 128*aPhase", true, true, false, true);
  auto actual = render(8, "64 + 128*aPhase", true, true, false, true);
  same(expected, actual);
  for (int i = 0; i < 32; ++i)
    if (i < 3 || i >= 23) EXPECT_EQ(0, actual[i]) << i;
}

TEST_P(InterpolatedRandomTests, FrequencyInputCanAliasOutput)
{
  same(render(8, "64 + 128*aPhase"),
       render(8, "64 + 128*aPhase", true, false, true));
}

TEST_P(InterpolatedRandomTests, ConstantAudioRateMatchesControlRate)
{
  same(render(8, "192", false), render(8, "192"));
}

TEST_P(InterpolatedRandomTests, AudioAndControlOutputsAgreeAtKsmpsOne)
{
  same(render(1, "192", false),
       render(1, "192", false, false, false, false, true));
}

TEST_P(InterpolatedRandomTests, ControlFrequencyCanAliasOutput)
{
  same(render(1, "192", false, false, false, false, true),
       render(1, "192", false, false, true, false, true));
}

TEST_P(InterpolatedRandomTests, NonpositiveRatesHoldTheCurrentValue)
{
  same(render(8, "0"), render(8, "-128"));
  same(render(8, "0", false), render(8, "-128", false));
}

TEST_P(InterpolatedRandomTests, LargeRatesRemainBounded)
{
  same(render(8, "1024"), render(8, "1e20"));
  same(render(1, "1024", false, false, false, false, true),
       render(1, "1e20", false, false, false, false, true));
}

INSTANTIATE_TEST_SUITE_P(Distributions, InterpolatedRandomTests,
                        ::testing::Values("gaussi", "cauchyi", "exprandi"));
}
