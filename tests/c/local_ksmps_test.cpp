#include "csound.h"
#include "csdebug.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <string>
#include <vector>

class LocalKsmpsTests : public ::testing::TestWithParam<int> {
protected:
    CSOUND *csound = nullptr;

    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        csoundSetOption(csound, "-d");
        csoundSetOption(csound, "--sample-accurate");
        csoundSetHostAudioIO(csound);
        if (GetParam() == 1)
            csoundSetOption(csound, "--num-threads=2");
    }

    void TearDown() override {
        if (GetParam() == 2)
            csoundDebuggerClean(csound);
        csoundDestroy(csound);
    }
};

TEST_P(LocalKsmpsTests, NoteBoundariesPreserveEveryActiveSample) {
    const std::string orchestra = R"(
sr = 1024
ksmps = 32
nchnls = 2
0dbfs = 1

opcode LocalRamp, a, i
 iBlock xin
 setksmps iBlock
 aRamp phasor 4, .125
 xout aRamp
endop

instr 1
 setksmps p4
 aRamp phasor 4, .125
 out aRamp, -aRamp
endin

instr 2
 aRamp LocalRamp p4
 out aRamp, -aRamp
endin

instr 3
 aLeft, aRight subinstr 1, p4
 out aLeft, aRight
endin
)";

    // Cover whole skipped sub-blocks, nonzero remainders, one-sample
    // notes, aligned boundaries, and notes spanning several global blocks.
    std::vector<double> expected;
    std::string score;
    for (int instrument : {1, 2, 3}) {
        for (int block : {1, 4, 8, 16}) {
            for (int offset : {0, 3, 11, 23}) {
                for (int length : {1, 5, 16, 48, 80}) {
                    const int start = static_cast<int>(expected.size()) + offset;
                    expected.resize(expected.size() + 128, 0.0);
                    for (int i = 0; i < length; ++i)
                        expected[start + i] = .125 + i / 256.0;
                    char event[128];
                    std::snprintf(event, sizeof(event), "i %d %.12f %.12f %d\n",
                                  instrument, start / 1024.0,
                                  length / 1024.0, block);
                    score += event;
                }
            }
        }
    }
    const std::string csd = "<CsoundSynthesizer>\n<CsInstruments>\n" +
        orchestra + "\n</CsInstruments>\n<CsScore>\n" + score +
        "e\n</CsScore>\n</CsoundSynthesizer>\n";
    ASSERT_EQ(csoundCompileCSD(csound, csd.c_str(), 1, 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    if (GetParam() == 2)
        ASSERT_EQ(csoundDebuggerInit(csound), 0);

    for (size_t base = 0; base < expected.size(); base += 32) {
        ASSERT_EQ(csoundPerformKsmps(csound), 0) << "block " << base / 32;
        const MYFLT *output = csoundGetSpout(csound);
        for (size_t i = 0; i < 32; ++i) {
            ASSERT_NEAR(output[2 * i], expected[base + i], 1e-6)
                << "sample " << base + i;
            ASSERT_NEAR(output[2 * i + 1], -expected[base + i], 1e-6)
                << "sample " << base + i << ", right channel";
        }
    }
}

INSTANTIATE_TEST_SUITE_P(Dispatch, LocalKsmpsTests, ::testing::Values(0, 1, 2));
