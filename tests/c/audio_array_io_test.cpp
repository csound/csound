#include "csound.h"
#include "csdebug.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <string>
#include <vector>

class AudioArrayIOTests : public ::testing::TestWithParam<int> {
protected:
  CSOUND *csound = nullptr;
  void SetUp() override {
    csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d --sample-accurate");
    csoundSetHostAudioIO(csound);
    if (GetParam() == 1) csoundSetOption(csound, "--num-threads=2");
  }
  void TearDown() override {
    if (GetParam() == 2) csoundDebuggerClean(csound);
    csoundDestroy(csound);
  }
  void start(const std::string &orchestra, const std::string &score) {
    const std::string csd = "<CsoundSynthesizer>\n<CsInstruments>\n"
      "sr=1024\nksmps=32\nnchnls=2\nnchnls_i=2\n0dbfs=1\n" +
      orchestra + "\n</CsInstruments>\n<CsScore>\n" + score +
      "\ne\n</CsScore>\n</CsoundSynthesizer>";
    ASSERT_EQ(0, csoundCompileCSD(csound, csd.c_str(), 1, 0));
    ASSERT_EQ(0, csoundStart(csound));
    if (GetParam() == 2) {
      ASSERT_EQ(0, csoundDebuggerInit(csound));
    }
  }
};

TEST_P(AudioArrayIOTests, OutputUsesArrayStrideAndKeepsMixerChannelsSeparate) {
  std::string orchestra;
  for (int instrument : {1, 2}) {
    orchestra += "instr " + std::to_string(instrument) + "\n";
    // Both fresh local arrays and arrays allocated at the global block size.
    orchestra += instrument == 1 ? "setksmps p4\naData[] init p5\n" :
                                   "aData[] init p5\nsetksmps p4\n";
    orchestra += R"(
 aRamp phasor 4, .125
 aData[0] = aRamp
 if p5 > 1 then
   aData[1] = -aRamp
 endif
 if p5 > 2 then
   aData[2] = .75
 endif
 out aData
endin
)";
  }
  std::vector<double> left, right;
  std::string score;
  for (int instrument : {1, 2}) {
    for (int block : {1, 4, 8, 16, 32}) {
      for (int channels : {1, 2, 3}) {
        for (int offset : {0, 3, 23}) {
          for (int length : {1, 16, 80}) {
            int start = static_cast<int>(left.size()) + offset;
            left.resize(left.size()+128, 0.0);
            right.resize(left.size(), 0.0);
            for (int i = 0; i < length; ++i) {
              left[start+i] = .125 + i/256.0;
              right[start+i] = channels > 1 ? -left[start+i] : 0.0;
            }
            char event[128];
            std::snprintf(event, sizeof(event), "i %d %.12f %.12f %d %d\n",
                          instrument, start/1024.0, length/1024.0, block, channels);
            score += event;
          }
        }
      }
    }
  }
  score += "f 0 " + std::to_string(left.size()/1024.0);
  ASSERT_NO_FATAL_FAILURE(start(orchestra, score));
  for (size_t base = 0; base < left.size(); base += 32) {
    ASSERT_EQ(0, csoundPerformKsmps(csound));
    const MYFLT *output = csoundGetSpout(csound);
    for (size_t i = 0; i < 32; ++i) {
      ASSERT_NEAR(output[2*i], left[base+i], 1e-6) << "sample " << base+i;
      ASSERT_NEAR(output[2*i+1], right[base+i], 1e-6) << "sample " << base+i;
    }
  }
}

TEST_P(AudioArrayIOTests, InputUsesStoredArrayStride) {
  std::string orchestra;
  for (int instrument : {1, 2}) {
    orchestra += "instr " + std::to_string(instrument) + "\n";
    if (instrument == 2) orchestra += "aData[] init 2\n";
    orchestra += "setksmps p4\naData[] in\nout aData[0], aData[1]\nendin\n";
  }
  std::vector<bool> active;
  std::string score;
  for (int instrument : {1, 2}) {
    for (int block : {1, 8, 32}) {
      for (int offset : {0, 3, 23}) {
        int start = static_cast<int>(active.size()) + offset;
        active.resize(active.size()+128, false);
        for (int i = 0; i < 48; ++i) active[start+i] = true;
        char event[128];
        std::snprintf(event, sizeof(event), "i %d %.12f %.12f %d\n",
                      instrument, start/1024.0, 48/1024.0, block);
        score += event;
      }
    }
  }
  score += "f 0 " + std::to_string(active.size()/1024.0);
  ASSERT_NO_FATAL_FAILURE(start(orchestra, score));
  for (size_t base = 0; base < active.size(); base += 32) {
    MYFLT *input = csoundGetSpin(csound);
    ASSERT_NE(input, nullptr);
    for (size_t i = 0; i < 32; ++i) {
      input[2*i] = (base%128+i+1)/256.0;
      input[2*i+1] = -input[2*i];
    }
    ASSERT_EQ(0, csoundPerformKsmps(csound));
    const MYFLT *output = csoundGetSpout(csound);
    for (size_t i = 0; i < 32; ++i) {
      double expected = active[base+i] ? (base%128+i+1)/256.0 : 0.0;
      ASSERT_NEAR(output[2*i], expected, 1e-6) << "sample " << base+i;
      ASSERT_NEAR(output[2*i+1], -expected, 1e-6) << "sample " << base+i;
    }
  }
}

TEST_P(AudioArrayIOTests, OutchIgnoresInvalidChannelsAndContinuesPerformance) {
  const std::string orchestra = R"(
instr 1
 setksmps 8
 aSignal = .125
 outch -1e30, aSignal, 0, aSignal, 3, aSignal, 1e30, aSignal
 outch 0, aSignal, 1.9, aSignal, 2, -aSignal, 1e30, aSignal
 out aSignal, -aSignal
endin
)";
  ASSERT_NO_FATAL_FAILURE(start(orchestra, "i 1 0 .25\nf 0 .5"));
  for (int block = 0; block < 8; ++block) {
    ASSERT_EQ(0, csoundPerformKsmps(csound));
    const MYFLT *output = csoundGetSpout(csound);
    for (int i = 0; i < 32; ++i) {
      ASSERT_NEAR(output[2*i], .25, 1e-6);
      ASSERT_NEAR(output[2*i+1], -.25, 1e-6);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(Dispatch, AudioArrayIOTests, ::testing::Values(0, 1, 2));
