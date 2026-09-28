#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <string>

namespace {

struct FillBlock {
    OPDS h;
    cs_float *output, *value;
};

int32_t fillBlock(CSOUND *, void *data)
{
    auto *p = static_cast<FillBlock *>(data);
    // Model an input buffer whose inactive samples still contain data.
    std::fill(p->output, p->output + p->h.insdshead->ksmps, *p->value);
    return OK;
}

struct CheckBlock {
    OPDS h;
    cs_float *input, *value, *start, *end;
};

int32_t checkBlock(CSOUND *csound, void *data)
{
    auto *p = static_cast<CheckBlock *>(data);
    for (uint32_t i = 0; i < p->h.insdshead->ksmps; ++i) {
        cs_float expected = i >= *p->start && i < *p->end ? *p->value : FL(0.0);
        EXPECT_EQ(p->input[i], expected) << "sample " << i;
    }
    ++*static_cast<int *>(csoundGetHostData(csound));
    return OK;
}

class MixerTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        csound = csoundCreate(&checks, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundAppendOpcode(csound, "testfillblock", sizeof(FillBlock),
                                    0, "a", "i", nullptr, fillBlock, nullptr), OK);
        ASSERT_EQ(csoundAppendOpcode(csound, "testcheckblock", sizeof(CheckBlock),
                                    0, "", "aiii", nullptr, checkBlock, nullptr), OK);
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

    int32_t run(const std::string &instruments, const std::string &score)
    {
        std::string csd =
            "<CsoundSynthesizer>\n<CsOptions>\n-n -d -m0 --sample-accurate\n"
            "</CsOptions>\n<CsInstruments>\nsr=8192\nksmps=16\n"
            "nchnls=2\n0dbfs=1\n" + instruments +
            "\n</CsInstruments>\n<CsScore>\n" + score +
            "\ne\n</CsScore>\n</CsoundSynthesizer>\n";
        int32_t result = csoundCompileCSD(csound, csd.c_str(), 1, 0);
        if (result == OK) result = csoundStart(csound);
        if (result != OK) return result;
        while (csoundPerformKsmps(csound) == OK) {}
        return csound->inerrcnt + csound->perferrcnt;
    }

    CSOUND *csound = nullptr;
    int checks = 0;
};

TEST_F(MixerTests, ReceiveZerosPaddingWithoutChangingTheBus)
{
    EXPECT_EQ(run(R"(
instr 1
 MixerSetLevel 0,0,1
 aSignal testfillblock .125
 MixerSend aSignal,0,0,0
 mixersend aSignal,0,0,1
endin
instr 2
 aLeft MixerReceive 0,0
 aRight mixerreceive 0,1
 testcheckblock aLeft,.125,3,11
 testcheckblock aRight,.125,3,11
endin
instr 3
 aSignal MixerReceive 0,0
 testcheckblock aSignal,.125,0,16
 MixerClear
 aSilent mixerreceive 0,0
 testcheckblock aSilent,0,0,16
endin
)", R"(
i 1 0 .001953125
i 2 .0003662109375 .0009765625
i 3 0 .001953125
i 1 .125 .001953125
i 2 .1253662109375 .0009765625
i 3 .125 .001953125
)"), 0) << messages();
    EXPECT_EQ(checks, 8);
}

TEST_F(MixerTests, SendMixesOnlyActiveSamplesAndKeepsOtherSends)
{
    EXPECT_EQ(run(R"(
instr 1
 MixerSetLevel 0,0,.5
 aSignal testfillblock .25
 MixerSend aSignal,0,0,0
endin
instr 2
 mixersetleveli 1,0,2
 aSignal testfillblock .0625
 mixersend aSignal,1,0,0
endin
instr 3
 aSignal MixerReceive 0,0
 aSignal -= .125
 testcheckblock aSignal,.125,3,11
 MixerClear
endin
)", R"(
i 1 0 .001953125
i 2 .0003662109375 .0009765625
i 3 0 .001953125
)"), 0) << messages();
    EXPECT_EQ(checks, 1);
}

class MixerInvalidInputTests : public MixerTests,
                              public ::testing::WithParamInterface<const char *> {};

TEST_P(MixerInvalidInputTests, RejectsInvalidIndicesAtInitialization)
{
    EXPECT_NE(run(std::string("instr 1\naSignal init .1\n") + GetParam() +
                  "\nendin", "i 1 0 .01"), 0);
    EXPECT_NE(messages().find("mixer:"), std::string::npos);
}

INSTANTIATE_TEST_SUITE_P(MixerIndices, MixerInvalidInputTests, ::testing::Values(
    "MixerSend aSignal,0,0,-1", "MixerSend aSignal,0,0,2",
    "aResult MixerReceive 0,-1", "aResult mixerreceive 0,2",
    "MixerSetLevel -1,0,1", "mixersetleveli 0,1e30,1",
    "kGain MixerGetLevel -1,0", "kGain mixergetlevel 0,1e30",
    "MixerSend aSignal,1e30,0,0", "aResult MixerReceive 1e30,0"));

TEST_F(MixerTests, RejectsDifferentBlockSizesOnTheSameBus)
{
    EXPECT_NE(run(R"(
instr 1
 setksmps 8
 MixerSetLevel 0,0,1
endin
instr 2
 aSignal MixerReceive 0,0
endin
)", "i 1 0 .01\ni 2 0 .01"), 0);
    EXPECT_NE(messages().find("different ksmps values"), std::string::npos);
}

} // namespace
