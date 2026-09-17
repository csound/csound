#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cmath>
#include <string>
#include <vector>

namespace {
class PitchTests : public ::testing::Test {
protected:
    CSOUND *csound;
    struct Frame { MYFLT pitch, amplitude; };
    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
    }
    void TearDown() override { csoundDestroy(csound); }
    std::vector<Frame> render(const std::string &args,
                             const char *score = "i1 0 1 1\nf0 1",
                             int blocks = 1000) {
        std::string orc =
            "sr=44100\nksmps=32\nnchnls=1\n0dbfs=1\n"
            "instr 1\naInput oscili p4,440\n"
            "kPitch,kAmp pitch aInput," + args +
            "\nchnset kPitch,\"pitch\"\nchnset kAmp,\"amp\"\nendin";
        std::vector<Frame> frames;
        EXPECT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0);
        csoundEventString(csound, score, 0);
        EXPECT_EQ(csoundStart(csound), 0);
        for (int i = 0; i < blocks; ++i) {
            if (csoundPerformKsmps(csound) != 0) break;
            frames.push_back({csoundGetControlChannel(csound, "pitch", nullptr),
                              csoundGetControlChannel(csound, "amp", nullptr)});
        }
        return frames;
    }
    void expectFinite(const std::vector<Frame> &frames) {
        ASSERT_FALSE(frames.empty());
        EXPECT_EQ(csound->inerrcnt, 0);
        EXPECT_EQ(csound->perferrcnt, 0);
        for (auto frame : frames) {
            EXPECT_TRUE(std::isfinite(frame.pitch));
            EXPECT_TRUE(std::isfinite(frame.amplitude));
            EXPECT_GE(frame.amplitude, 0);
        }
    }
};

TEST_F(PitchTests, TracksAnOrdinaryTone) {
    const auto frames = render(".01,6,10,0");
    expectFinite(frames);
    ASSERT_EQ(frames.size(), 1000u);
    EXPECT_NEAR(frames.back().pitch, 8.75, .1);
    EXPECT_GT(frames.back().amplitude, 0);
}

class PitchRangeTests : public PitchTests,
                        public ::testing::WithParamInterface<const char *> {};
TEST_P(PitchRangeTests, ClampsSearchToAvailableBins) {
    expectFinite(render(GetParam()));
}
INSTANTIATE_TEST_SUITE_P(SearchRanges, PitchRangeTests, ::testing::Values(
    ".01,6,10,0,12,10,30",
    ".01,6,10,0,12,10,0",
    ".01,6,30,0",
    ".01,-1e30,1e30,0",
    ".01,8.7,8.8,0,12,10,8.75",
    ".01,6,10,0,120,10,8.75,8,15,10,1",
    ".01,6,10,0,12,1e30"));

TEST_F(PitchTests, SilenceWithAnEmptySearchHasZeroOutput) {
    const auto frames = render(".01,6,10,0,12,10,30", "i1 0 1 0\nf0 1");
    expectFinite(frames);
    for (auto frame : frames) {
        EXPECT_EQ(frame.pitch, 0);
        EXPECT_EQ(frame.amplitude, 0);
    }
}

TEST_F(PitchTests, ReusedNoteDoesNotAnalyzePreviousAudio) {
    const auto frames = render(".01,6,10,0",
                              "i1 0 .3 1\ni1 .4 .3 0\nf0 1");
    expectFinite(frames);
    ASSERT_EQ(frames.size(), 1000u);
    for (int i = 560; i < 960; ++i) {
        EXPECT_EQ(frames[i].pitch, 0) << i;
        EXPECT_EQ(frames[i].amplitude, 0) << i;
    }
}

class PitchInvalidTests : public PitchTests,
                          public ::testing::WithParamInterface<const char *> {};
TEST_P(PitchInvalidTests, RejectsUnsupportedAnalysisParameters) {
    render(GetParam(), "i1 0 1 1\nf0 1", 4);
    EXPECT_GT(csound->inerrcnt, 0);
}
INSTANTIATE_TEST_SUITE_P(InvalidParameters, PitchInvalidTests, ::testing::Values(
    ".01,6,10,0,12,10,8,6,15,.5",
    ".01,6,10,0,12,10,8,6,15,1e30",
    ".01,6,10,0,1e30",
    ".01,6,10,0,12,10,8,1e30",
    ".01,6,10,0,12,10,8,6,.01",
    ".01,6,10,0,12,10,8,6,1e30",
    ".01,6,10,0,12,10,8,8,100000000",
    "1e30,6,10,0",
    ".01,10,6,0",
    ".01,20,30,0"));
} // namespace
