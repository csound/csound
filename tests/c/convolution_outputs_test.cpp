#include "csound.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <string>

namespace {
class ConvolutionOutputsTests : public ::testing::TestWithParam<const char *> {
protected:
  CSOUND *csound = nullptr;
  void SetUp() override {
    csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0 --sample-accurate");
    csoundSetHostAudioIO(csound);
  }
  void TearDown() override {
    if (HasFailure()) ADD_FAILURE() << messages();
    csoundDestroy(csound);
  }
  std::string messages() {
    std::string text;
    while (csoundGetMessageCnt(csound)) {
      text += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    return text;
  }
  bool fft() const { return std::string(GetParam()) == "ftconv"; }
  static double input(int sample) { return (sample % 11 - 5) / 8.0; }
  static double tap(int frame, int ch) {
    return (ch + 1) * (frame % 5 - 2) / 4096.0;
  }
  std::string args(const std::string &channels, int skip = 0, int length = 13) {
    std::string result = fft() ? "aIn, iIR, 8" : "aIn, " + std::to_string(length) + ", iIR";
    if (!channels.empty()) result += ", " + channels;
    if (fft()) result += ", " + std::to_string(skip) + ", " + std::to_string(length);
    return result;
  }
  int compile(const std::string &body, int channels = 1,
              const std::string &score = "i 1 0 .25\nf 0 .5") {
    std::string csd = "<CsoundSynthesizer>\n<CsInstruments>\n"
      "sr=1024\nksmps=32\nnchnls=" + std::to_string(channels) +
      "\nnchnls_i=1\n0dbfs=1\ninstr 1\n" + body +
      "\nendin\n</CsInstruments>\n<CsScore>\n" + score +
      "\ne\n</CsScore>\n</CsoundSynthesizer>";
    return csoundCompileCSD(csound, csd.c_str(), 1, 0);
  }
  void checkSignal(int channels, bool array, int localBlock = 32,
                   int offset = 0, int duration = 160, int skip = 0,
                   int length = 13, bool inPlace = false) {
    std::string count = std::to_string(channels);
    std::string body = "iIR ftgen 0, 0, -" + std::to_string(19 * channels) +
      ", -2, 0\niFrame=0\nwhile iFrame < 19 do\niCh=0\nwhile iCh < " + count +
      " do\ntableiw (iCh+1)*(iFrame%5-2)/4096, iFrame*" + count +
      "+iCh, iIR\niCh+=1\nod\niFrame+=1\nod\n";
    // Keep a larger stored stride when local ksmps is smaller than global ksmps.
    if (array) body += "aResult[] init " + count + "\n";
    body += "setksmps " + std::to_string(localBlock) + "\naIn inch 1\n";
    if (array) body += "aResult[] ";
    else {
      for (int ch = 0; ch < channels; ++ch)
        body += (ch ? ", " : "") +
          (inPlace && ch == 0 ? std::string("aIn") : "aOut" + std::to_string(ch));
      body += " ";
    }
    body += GetParam() + std::string(" ") + args(array ? count : "", skip, length) + "\n";
    if (array) body += "out aResult\n";
    else for (int ch = 0; ch < channels; ++ch)
      body += "outch " + std::to_string(ch + 1) + ", " +
        (inPlace && ch == 0 ? std::string("aIn") : "aOut" + std::to_string(ch)) + "\n";
    ASSERT_EQ(0, compile(body, channels, "i 1 " + std::to_string(offset / 1024.0) +
      " " + std::to_string(duration / 1024.0) + "\nf 0 .5"));
    ASSERT_EQ(0, csoundStart(csound));
    for (int base = 0; base < 256; base += 32) {
      MYFLT *spin = csoundGetSpin(csound);
      ASSERT_NE(nullptr, spin);
      for (int n = 0; n < 32; ++n) spin[n] = input(base + n);
      ASSERT_EQ(0, csoundPerformKsmps(csound));
      const MYFLT *out = csoundGetSpout(csound);
      for (int n = 0; n < 32; ++n) {
        int time = base + n;
        for (int ch = 0; ch < channels; ++ch) {
          double expected = 0;
          if (time >= offset && time < offset + duration) {
            int age = time - offset - (fft() ? 8 : 0);
            int frames = std::min(length, 19 - skip);
            for (int i = 0; i < frames && i <= age; ++i)
              if (skip + i >= 0)
                expected += input(time - (fft() ? 8 : 0) - i) * tap(skip + i, ch);
          }
          ASSERT_NEAR(expected, out[n * channels + ch], 2e-6)
            << "sample " << time << ", channel " << ch;
        }
      }
    }
  }
};

TEST_P(ConvolutionOutputsTests, MonoKeepsItsExistingMeaning) { checkSignal(1, false); }
TEST_P(ConvolutionOutputsTests, Stereo) { checkSignal(2, false); }
TEST_P(ConvolutionOutputsTests, ThirtyTwoSeparateOutputs) { checkSignal(32, false); }
TEST_P(ConvolutionOutputsTests, SeparateOutputsCanReuseInput) {
  checkSignal(32, false, 32, 5, 151, 0, 13, true);
}
TEST_P(ConvolutionOutputsTests, ArrayBeyondSeparateOutputLimit) { checkSignal(65, true); }
TEST_P(ConvolutionOutputsTests, ArrayWithLocalBlockSizeAndPartialBlocks) {
  checkSignal(33, true, 8, 5, 151);
}
TEST_P(ConvolutionOutputsTests, ArrayWithOneSampleBlocks) { checkSignal(2, true, 1); }
TEST_P(ConvolutionOutputsTests, LongRequestStopsAtLastCompleteFrame) {
  checkSignal(3, true, 32, 0, 160, 0, 1000);
}
TEST_P(ConvolutionOutputsTests, ArraySkipFrames) {
  if (!fft()) GTEST_SKIP() << "dconv has no skip argument";
  checkSignal(33, true, 32, 5, 151, 3);
}
TEST_P(ConvolutionOutputsTests, ArrayNegativeSkipPadsWithSilence) {
  if (!fft()) GTEST_SKIP() << "dconv has no skip argument";
  checkSignal(33, true, 32, 0, 160, -3);
}
TEST_P(ConvolutionOutputsTests, InvalidArrayChannelCount) {
  for (const char *count : {"0", "-1", "1.5", "1e30", "sqrt(-1)", "exp(1000)"}) {
    csoundReset(csound);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    ASSERT_EQ(0, compile("iIR ftgen 0,0,64,-2,1\naIn=1\naResult[] " +
      std::string(GetParam()) + " " + args(count)));
    csoundStart(csound);
    csoundPerformKsmps(csound);
    EXPECT_NE(std::string::npos, messages().find("invalid number of channels")) << count;
  }
}
TEST_P(ConvolutionOutputsTests, TooFewTableSamplesForChannels) {
  ASSERT_EQ(0, compile("iIR ftgen 0,0,4,-2,1\naIn=1\naResult[] " +
    std::string(GetParam()) + " " + args("65")));
  csoundStart(csound);
  csoundPerformKsmps(csound);
  EXPECT_NE(std::string::npos, messages().find("insufficient IR data"));
}

TEST_P(ConvolutionOutputsTests, ReinitCanGrowAndShrinkTheChannelCount) {
  std::string body =
    "iIR ftgen 0,0,512,-7,.125,512,.125\n"
    "kCycle init 0\nkChannels init 2\n"
    "if kCycle == 2 || kCycle == 4 then\n"
    "kChannels = (kCycle == 2 ? 65 : 1)\nreinit CONV\nendif\naIn=1\n"
    "CONV:\naResult[] " + std::string(GetParam()) + " " + args("i(kChannels)", 0, 3);
  if (fft()) body += ", 1"; // A channel change must reset even with iskipinit.
  body += "\nrireturn\nout aResult\nkCycle+=1\n";
  ASSERT_EQ(0, compile(body, 65));
  ASSERT_EQ(0, csoundStart(csound));
  for (int block = 0; block < 6; ++block) {
    ASSERT_EQ(0, csoundPerformKsmps(csound));
    int channels = block < 2 ? 2 : block < 4 ? 65 : 1;
    const MYFLT *out = csoundGetSpout(csound);
    for (int n = 0; n < 32; ++n) {
      int age = (block % 2) * 32 + n - (fft() ? 8 : 0);
      double expected = .125 * std::max(0, std::min(3, age + 1));
      for (int ch = 0; ch < 65; ++ch)
        ASSERT_NEAR(ch < channels ? expected : 0, out[n * 65 + ch], 1e-6)
          << "block " << block << ", sample " << n << ", channel " << ch;
    }
  }
}

TEST_P(ConvolutionOutputsTests, ShrunkOutputArrayReportsAnError) {
  ASSERT_EQ(0, compile("iIR ftgen 0,0,64,-2,1\naIn=1\naResult[] " +
    std::string(GetParam()) + " " + args("2") + "\ntrim_i aResult, 1"));
  csoundStart(csound);
  csoundPerformKsmps(csound);
  EXPECT_NE(std::string::npos, messages().find("output array is too small"));
}

INSTANTIATE_TEST_SUITE_P(Opcodes, ConvolutionOutputsTests,
                        ::testing::Values("ftconv", "dconv"));
} // namespace
