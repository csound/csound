#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

namespace {
class MultitapTests : public ::testing::Test {
protected:
  CSOUND *csound = nullptr;
  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0 --sample-accurate");
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

  void start(const std::string &body, int block = 1,
             const std::string &score = "i 1 0 .03125\nf 0 .03125\n")
  {
    const std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\nsr=1024\nksmps=" +
      std::to_string(block) + "\nnchnls=1\n0dbfs=1\ninstr 1\n" + body +
      "\nendin\n</CsInstruments>\n<CsScore>\n" + score +
      "</CsScore>\n</CsoundSynthesizer>";
    ASSERT_EQ(0, csoundCompileCSD(csound, csd.c_str(), 1, 0)) << messages();
    ASSERT_EQ(0, csoundStart(csound)) << messages();
  }

  std::vector<cs_float> render(int block = 1)
  {
    std::vector<cs_float> result;
    for (int sample = 0; sample < 32; sample += block) {
      csoundPerformKsmps(csound);
      const cs_float *out = csoundGetSpout(csound);
      result.insert(result.end(), out, out + block);
    }
    EXPECT_EQ(0, csound->inerrcnt) << messages();
    EXPECT_EQ(0, csound->perferrcnt) << messages();
    return result;
  }

  void impulse(const std::string &taps, int delay)
  {
    start("aIn mpulse 1, 0\naOut multitap aIn, " + taps + "\nout aOut");
    std::vector<cs_float> expected(32, 0);
    expected[delay] = 1;
    EXPECT_EQ(expected, render());
  }
};

TEST_F(MultitapTests, LongestTapHasExactDelay) { impulse("4/sr, 1", 4); }
TEST_F(MultitapTests, OneSampleDelay) { impulse("1/sr, 1", 1); }
TEST_F(MultitapTests, ZeroDelay) { impulse("0, 1", 0); }
TEST_F(MultitapTests, SubsampleDelay) { impulse(".5/sr, 1", 0); }
TEST_F(MultitapTests, FractionalDelayKeepsTruncation) { impulse("3.75/sr, 1", 3); }
TEST_F(MultitapTests, ZeroTapWithLongerBuffer) { impulse("0, 1, 4/sr, 0", 0); }

TEST_F(MultitapTests, TapsSumAcrossBufferWraps)
{
  start("aIn mpulse 1, 8/sr\naOut multitap aIn, 0, 1, 3/sr, .5, 3/sr, -.25, 9/sr, 2\nout aOut", 8);
  std::vector<cs_float> expected(32, 0);
  for (int source = 0; source < 32; source += 8) {
    expected[source] += 1;
    if (source + 3 < 32) expected[source + 3] += .25;
    if (source + 9 < 32) expected[source + 9] += 2;
  }
  EXPECT_EQ(expected, render(8));
}

TEST_F(MultitapTests, InputCanAliasOutput)
{
  start("aIn mpulse 1, 0\naIn multitap aIn, 0, .5, 4/sr, 1\nout aIn", 8);
  std::vector<cs_float> expected(32, 0);
  expected[0] = .5;
  expected[4] = 1;
  EXPECT_EQ(expected, render(8));
}

TEST_F(MultitapTests, PartialBlockStartsAndEnds)
{
  start("aIn mpulse 1, 0\naOut multitap aIn, 0, 1, 4/sr, .5, 20/sr, 1\nout aOut",
        8, "i 1 .0029296875 .01953125\nf 0 .03125\n");
  std::vector<cs_float> expected(32, 0);
  expected[3] = 1;
  expected[7] = .5;
  EXPECT_EQ(expected, render(8));
}

TEST_F(MultitapTests, ReusedNotesClearAndResizeTheBuffer)
{
  start("aIn mpulse 1, 0\naOut multitap aIn, p4/sr, 1\nout aOut", 1,
        "i 1 0 .0078125 2\ni 1 .0078125 .0078125 4\n"
        "i 1 .015625 .0078125 1\nf 0 .03125\n");
  std::vector<cs_float> expected(32, 0);
  expected[2] = expected[12] = expected[17] = 1;
  EXPECT_EQ(expected, render());
}

TEST_F(MultitapTests, NoTapsProduceSilence)
{
  start("aIn mpulse 1, 0\naOut multitap aIn\nout aOut");
  EXPECT_EQ(std::vector<cs_float>(32, 0), render());
}

TEST_F(MultitapTests, InvalidDelaysFailAtInitialization)
{
  for (const char *delay : {"-1", "-0.5/sr", "1e20"}) {
    csoundReset(csound);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    start(std::string("aIn init 0\naOut multitap aIn, ") + delay + ", 1\nout aOut");
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->inerrcnt, 0) << delay << messages();
  }
}
}
