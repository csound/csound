#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cmath>
#include <string>
#include <vector>

namespace {
class NreverbTests : public ::testing::TestWithParam<const char *> {
protected:
  std::string filter(const std::string &time = ".4",
                     const std::string &skip = "0",
                     const std::string &tables = "1, 1, 1, 2")
  {
    return std::string("aOut ") + GetParam() + " aIn, " + time +
      ", .25, " + skip + ", " + tables + "\n";
  }

  std::vector<cs_float> render(const std::string &body,
                           const std::string &comb = "-3, .8",
                           const std::string &allpass = "-5, .7",
                           int frames = 64, int block = 1,
                           const std::string &extraScore = "",
                           bool errorExpected = false)
  {
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0 --sample-accurate");
    const std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\nsr=1024\nksmps=" +
      std::to_string(block) + "\nnchnls=1\n0dbfs=1\n"
      "giC ftgen 1, 0, -2, -2, " + comb +
      "\ngiA ftgen 2, 0, -2, -2, " + allpass +
      "\ninstr 1\naIn mpulse 1, 0\n" + body +
      "\nout aOut\nendin\ninstr 2\nftfree 1, 0\nftfree 2, 0\nendin\n"
      "</CsInstruments>\n<CsScore>\ni 1 0 " +
      std::to_string(frames / 1024.0) + "\n" + extraScore +
      "e\n</CsScore>\n</CsoundSynthesizer>";
    int err = csoundCompileCSD(csound, csd.c_str(), 1, 0);
    EXPECT_EQ(0, err);
    if (err == 0) err = csoundStart(csound);
    EXPECT_EQ(0, err);
    std::vector<cs_float> out;
    if (err == 0) {
      for (int n = 0; n < frames; n += block) {
        csoundPerformKsmps(csound);
        const cs_float *samples = csoundGetSpout(csound);
        out.insert(out.end(), samples, samples + block);
      }
      if (errorExpected) EXPECT_GT(csound->inerrcnt, 0);
      else {
        EXPECT_EQ(0, csound->inerrcnt);
        EXPECT_EQ(0, csound->perferrcnt);
      }
    }
    if (HasFailure()) {
      std::string messages;
      while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
      }
      ADD_FAILURE() << messages;
    }
    csoundDestroy(csound);
    return out;
  }

  void same(const std::vector<cs_float> &expected, const std::vector<cs_float> &actual)
  {
    ASSERT_FALSE(expected.empty());
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      EXPECT_TRUE(std::isfinite(actual[i])) << i;
      EXPECT_EQ(expected[i], actual[i]) << "sample " << i;
    }
  }
};

TEST_P(NreverbTests, FirstUseWithSkipStillInitializes)
{
  same(render(filter()), render(filter(".4", "1")));
}

TEST_P(NreverbTests, ReinitWithSkipPreservesTail)
{
  same(render(filter()), render(
    "iSkip init 0\nkCycle init 0\nkCycle += 1\n"
    "if kCycle == 17 then\nreinit AGAIN\nendif\nAGAIN:\n" +
    filter(".4", "iSkip") + "iSkip = 1\nrireturn\n"));
}

TEST_P(NreverbTests, DefaultFiltersPreserveTailOnReinit)
{
  same(render(filter(".4", "0", "0, 0, 0, 0")), render(
    "iSkip init 0\nkCycle init 0\nkCycle += 1\n"
    "if kCycle == 17 then\nreinit AGAIN\nendif\nAGAIN:\n" +
    filter(".4", "iSkip", "0, 0, 0, 0") + "iSkip = 1\nrireturn\n"));
}

TEST_P(NreverbTests, ReinitWithoutSkipClearsTheTail)
{
  auto expected = render(filter());
  for (size_t i = 16; i < expected.size(); ++i) expected[i] = 0;
  same(expected, render("kCycle init 0\nkCycle += 1\n"
    "if kCycle == 17 then\nreinit AGAIN\nendif\nAGAIN:\n" +
    filter() + "rireturn\n"));
}

TEST_P(NreverbTests, ChangedControlSurvivesSkippedReinit)
{
  const std::string control = "kTime init .4\nkCycle init 0\nkCycle += 1\n"
    "if kCycle == 17 then\nkTime = .2\nendif\n";
  same(render(control + filter("kTime")), render("iSkip init 0\n" + control +
    "if kCycle == 17 then\nreinit AGAIN\nendif\nAGAIN:\n" +
    filter("kTime", "iSkip") + "iSkip = 1\nrireturn\n"));
}

TEST_P(NreverbTests, NonpositiveInitialTimeUsesDocumentedFallback)
{
  auto expected = render(filter(".01"));
  same(expected, render(filter("-1")));
  same(expected, render(filter("0")));
}

TEST_P(NreverbTests, NonpositiveControlTimeUsesDocumentedFallback)
{
  const std::string control = "kTime init .4\nkCycle init 0\nkCycle += 1\n"
    "if kCycle == 17 then\nkTime = ";
  same(render(control + ".01\nendif\n" + filter("kTime")),
       render(control + "-1\nendif\n" + filter("kTime")));
}

TEST_P(NreverbTests, TablesCanBeFreedAfterInitialization)
{
  const std::string body = "kTime init .4\nkCycle init 0\nkCycle += 1\n"
    "if kCycle == 33 then\nkTime = .2\nendif\n" + filter("kTime");
  same(render(body), render(body, "-3, .8", "-5, .7", 64, 1,
                           "i 2 .015625 .001\n"));
}

TEST_P(NreverbTests, BlockSizeDoesNotChangeTheTail)
{
  same(render(filter()), render(filter(), "-3, .8", "-5, .7", 64, 8));
}

TEST_P(NreverbTests, PrimeLengthsExcludePerfectSquares)
{
  same(render(filter(), "-3727, .8", "-5, .7", 3776),
       render(filter(), "3721/1024, .8", "-5, .7", 3776));
}

TEST_P(NreverbTests, InvalidLengthsFailAtInitialization)
{
  for (const char *delay : {"-.5, .8", "1e20, .8", "-1e20, .8"}) {
    render(filter(), delay, "-5, .7", 8, 1, "", true);
    render(filter(), "-3, .8", delay, 8, 1, "", true);
  }
}

TEST_P(NreverbTests, InvalidCountsFailAtInitialization)
{
  render(filter(".4", "0", "2, 1, 1, 2"), "-3, .8", "-5, .7", 8, 1, "", true);
  render(filter(".4", "0", "1e20, 1, 1, 2"), "-3, .8", "-5, .7", 8, 1, "", true);
  render(filter(".4", "0", "1, 1, 2, 2"), "-3, .8", "-5, .7", 8, 1, "", true);
}

INSTANTIATE_TEST_SUITE_P(Reverbs, NreverbTests, ::testing::Values("nreverb", "reverb2"));
}
