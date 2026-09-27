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

instr 4
 setksmps p4
 aLeft, aRight subinstr 1, p4
 out aLeft, aRight
endin

instr 5
 setksmps p4
 aLeft, aRight subinstr 4, p4
 out aLeft, aRight
endin
)";

    // Cover whole skipped sub-blocks, nonzero remainders, one-sample
    // notes, aligned boundaries, and notes spanning several global blocks.
    std::vector<double> expected;
    std::string score;
    for (int instrument : {1, 2, 3, 4, 5}) {
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
    if (GetParam() == 2) {
        ASSERT_EQ(csoundDebuggerInit(csound), 0);
    }

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

TEST_P(LocalKsmpsTests, SubinstrReadsTheCurrentParentInputBlock) {
    const std::string orchestra = R"(
sr = 1024
ksmps = 32
nchnls = 2
nchnls_i = 2
0dbfs = 1
instr 1
 aLeft, aRight subinstr 10, p4
 out aLeft, aRight
endin
instr 2
 setksmps p4
 aLeft, aRight subinstr 10, p4
 out aLeft, aRight
endin
instr 3
 setksmps p4
 aLeft, aRight subinstr 2, p4
 out aLeft, aRight
endin
instr 10
 setksmps p4
 aLeft, aRight ins
 out aLeft, aRight
endin
)";
    std::vector<bool> active;
    std::string score;
    for (int instrument : {1, 2, 3}) {
        for (int block : {1, 8, 16}) {
            for (int offset : {0, 3, 11}) {
                int start = static_cast<int>(active.size()) + offset;
                active.resize(active.size() + 128, false);
                for (int i = 0; i < 48; ++i) active[start + i] = true;
                char event[128];
                std::snprintf(event, sizeof(event), "i %d %.12f %.12f %d\n",
                              instrument, start / 1024.0, 48 / 1024.0, block);
                score += event;
            }
        }
    }
    const std::string csd = "<CsoundSynthesizer>\n<CsInstruments>\n" +
        orchestra + "\n</CsInstruments>\n<CsScore>\n" + score +
        "f 0 " + std::to_string(active.size()/1024.0) +
        "\ne\n</CsScore>\n</CsoundSynthesizer>\n";
    ASSERT_EQ(csoundCompileCSD(csound, csd.c_str(), 1, 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    if (GetParam() == 2) {
        ASSERT_EQ(csoundDebuggerInit(csound), 0);
    }
    for (size_t base = 0; base < active.size(); base += 32) {
        MYFLT *input = csoundGetSpin(csound);
        ASSERT_NE(input, nullptr);
        for (size_t i = 0; i < 32; ++i) {
            input[2*i] = (base % 128 + i + 1) / 256.0;
            input[2*i+1] = -input[2*i];
        }
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        const MYFLT *output = csoundGetSpout(csound);
        for (size_t i = 0; i < 32; ++i) {
            const double expected = active[base+i] ?
                (base % 128 + i + 1) / 256.0 : 0.0;
            ASSERT_NEAR(output[2*i], expected, 1e-6) << "sample " << base+i;
            ASSERT_NEAR(output[2*i+1], -expected, 1e-6) << "sample " << base+i;
        }
    }
}

TEST_P(LocalKsmpsTests, SubinstrCountsItsOwnControlCycles) {
    const std::string csd = R"(
<CsoundSynthesizer>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 2
0dbfs = 1
instr 1
 setksmps p4
 aLeft, aRight subinstr 10, p5
 out aLeft, aRight
endin
instr 10
 setksmps p4
 kCycle timeinstk
 aCycle = kCycle
 out aCycle, -aCycle
endin
</CsInstruments>
<CsScore>
i 1 0 .125 32 4
i 1 .125 .125 8 8
i 1 .25 .125 32 32
f 0 .5
e
</CsScore>
</CsoundSynthesizer>
)";
    ASSERT_EQ(csoundCompileCSD(csound, csd.c_str(), 1, 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    if (GetParam() == 2) {
        ASSERT_EQ(csoundDebuggerInit(csound), 0);
    }
    const int childBlocks[] = {4, 8, 32};
    for (int block = 0; block < 12; ++block) {
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        const MYFLT *output = csoundGetSpout(csound);
        for (int i = 0; i < 32; ++i) {
            const int cycle = ((block % 4)*32+i)/childBlocks[block/4]+1;
            ASSERT_EQ(output[2*i], cycle) << "block " << block;
            ASSERT_EQ(output[2*i+1], -cycle) << "block " << block;
        }
    }
}

TEST_P(LocalKsmpsTests, FailedSubinstrClearsSamplesAndLeavesParentRunning) {
    const std::string orchestra = R"(
sr = 1024
ksmps = 32
nchnls = 2
0dbfs = 1
instr 1
 setksmps p4
 aLeft, aRight subinstr 10, p5
 out aLeft+.25, aRight+.5
endin
instr 10
 setksmps p4
 aSignal = 1
 out aSignal, aSignal
 kValues[] fillarray 0
 kIndex init 1
 kValue = kValues[kIndex]
endin
)";
    const std::string csd = "<CsoundSynthesizer>\n<CsInstruments>\n" +
        orchestra + "\n</CsInstruments>\n<CsScore>\n"
        "i 1 0 .125 32 32\ni 1 .125 .125 32 4\n"
        "i 1 .25 .125 8 8\nf 0 .5\ne\n"
        "</CsScore>\n</CsoundSynthesizer>\n";
    ASSERT_EQ(csoundCompileCSD(csound, csd.c_str(), 1, 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    if (GetParam() == 2) {
        ASSERT_EQ(csoundDebuggerInit(csound), 0);
    }
    for (int block = 0; block < 12; ++block) {
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        const MYFLT *output = csoundGetSpout(csound);
        for (int i = 0; i < 32; ++i) {
            ASSERT_NEAR(output[2*i], .25, 1e-6) << "block " << block;
            ASSERT_NEAR(output[2*i+1], .5, 1e-6) << "block " << block;
        }
    }
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(messages.find("PERF ERROR"), std::string::npos);
}

INSTANTIATE_TEST_SUITE_P(Dispatch, LocalKsmpsTests, ::testing::Values(0, 1, 2));
