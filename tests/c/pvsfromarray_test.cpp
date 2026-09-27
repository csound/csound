#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <cstring>
#include <string>

namespace {

class PvsFromArrayTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;

    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
        ASSERT_EQ(csoundCompileOrc(csound,
            "sr = 8192\nksmps = 16\nnchnls = 1\n0dbfs = 1\n"
            "gkInterleaved[] init 66\n"
            "gkMagnitudes[] init 33\ngkFrequencies[] init 33\n"
            "gfOutput pvsinit 64, 1, 64, 1\n"
            "instr 1\n"
            "if p4 == 0 then\n"
            "gfOutput pvsfromarray gkInterleaved, 16\n"
            "else\n"
            "gfOutput pvsfromarray gkMagnitudes, gkFrequencies, 16\n"
            "endif\nendin\n", 0), CSOUND_SUCCESS);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    }

    void TearDown() override { csoundDestroy(csound); }

    PVSDAT *output()
    {
        auto *variables = csoundDebugGetGlobalVariables(csound);
        PVSDAT *result = nullptr;
        for (auto *variable = variables; variable; variable = variable->next)
            if (std::strcmp(variable->name, "gfOutput") == 0)
                result = static_cast<PVSDAT *>(variable->data);
        csoundDebugFreeVariables(csound, variables);
        return result;
    }

    std::string messages()
    {
        std::string result;
        while (csoundGetMessageCnt(csound)) {
            result += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return result;
    }

    void checkOrdinaryOutput(int size)
    {
        auto *frame = output();
        ASSERT_NE(frame, nullptr);
        ASSERT_FALSE(frame->sliding);
        EXPECT_EQ(frame->N, size);
        EXPECT_EQ(frame->NB, size / 2 + 1);
        EXPECT_EQ(frame->format, PVS_AMP_FREQ);
        EXPECT_EQ(frame->overlap, 16);
        EXPECT_EQ(frame->winsize, size);
        EXPECT_EQ(frame->wintype, 1);
        EXPECT_EQ(frame->framecount, 1u);
        ASSERT_GE(frame->frame.size, (size + 2) * sizeof(float));
        for (int i = 0; i < size + 2; ++i)
            EXPECT_EQ(static_cast<float *>(frame->frame.auxp)[i], 0.0f) << i;
    }
};

TEST_F(PvsFromArrayTests, ReusedSlidingOutputBecomesAnOrdinaryFrame)
{
    for (bool split : {false, true}) {
        for (int bins : {2, 33, 65, 17}) {
            SCOPED_TRACE(std::to_string(bins) + (split ? " split" : " interleaved"));
            const auto length = std::to_string(bins);
            const auto orchestra =
                "gkInterleaved[] init " + std::to_string(2 * bins) + "\n" +
                "gkMagnitudes[] init " + length + "\n" +
                "gkFrequencies[] init " + length + "\n" +
                "gfOutput pvsinit 64, 1, 64, 1\n" +
                (split ? "gfOutput pvsfromarray gkMagnitudes, gkFrequencies, 16\n"
                       : "gfOutput pvsfromarray gkInterleaved, 16\n");
            ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
            ASSERT_NO_FATAL_FAILURE(checkOrdinaryOutput(2 * bins - 2));
        }
    }
}

class PvsFromArrayResizeTests : public PvsFromArrayTests,
                              public ::testing::WithParamInterface<int> {};

TEST_P(PvsFromArrayResizeTests, ResizingRequiresReinitialization)
{
    const int mode = GetParam();
    csoundEventString(csound, mode == 0 ? "i 1 0 1 0" : "i 1 0 1 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    const char *resize = mode == 0 ? "gkInterleaved[] init 130\n" :
                         mode == 1 ? "gkMagnitudes[] init 65\n" :
                                     "gkFrequencies[] init 17\n";
    ASSERT_EQ(csoundCompileOrc(csound, resize, 0), CSOUND_SUCCESS);
    csoundPerformKsmps(csound);
    EXPECT_NE(messages().find("tab2pvs: array shape changed; reinitialise"),
              std::string::npos);
}

INSTANTIATE_TEST_SUITE_P(ArrayForms, PvsFromArrayResizeTests,
                        ::testing::Values(0, 1, 2));

struct InvalidInput {
    const char *arrays;
    const char *arguments;
    const char *error;
};

class PvsFromArrayInvalidTests : public PvsFromArrayTests,
                               public ::testing::WithParamInterface<InvalidInput> {};

TEST_P(PvsFromArrayInvalidTests, ReportsInvalidInputAtInitialization)
{
    const auto &input = GetParam();
    const auto orchestra = std::string("instr 2\n") + input.arrays +
        "fOutput pvsfromarray " + input.arguments + "\nendin\n";
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    csoundEventString(csound, "i 2 0 .1", 0);
    csoundPerformKsmps(csound);
    EXPECT_NE(messages().find(input.error), std::string::npos);
}

INSTANTIATE_TEST_SUITE_P(Validation, PvsFromArrayInvalidTests, ::testing::Values(
    InvalidInput{"kData[] init 2\n", "kData, 16", "at least two amplitude/frequency pairs"},
    InvalidInput{"kData[] init 5\n", "kData, 16", "at least two amplitude/frequency pairs"},
    InvalidInput{"kData[][] init 8, 8\n", "kData, 16", "one-dimensional array"},
    InvalidInput{"kM[] init 1\nkF[] init 1\n", "kM, kF, 16", "at least two bins"},
    InvalidInput{"kM[] init 3\nkF[] init 2\n", "kM, kF, 16", "matching one-dimensional"},
    InvalidInput{"kM[][] init 8, 8\nkF[][] init 8, 8\n", "kM, kF, 16", "matching one-dimensional"},
    InvalidInput{"kData[] init 66\n", "kData, -1", "hop size must be at least ksmps"},
    InvalidInput{"kData[] init 66\n", "kData, 8", "hop size must be at least ksmps"},
    InvalidInput{"kData[] init 66\n", "kData, 1e30", "hop size must be at least ksmps"},
    InvalidInput{"kData[] init 66\n", "kData, 16, -1", "invalid window size or type"},
    InvalidInput{"kData[] init 66\n", "kData, 16, 64, 1e30", "invalid window size or type"}
));

} // namespace
