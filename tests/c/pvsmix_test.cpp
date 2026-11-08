#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <cstring>
#include <string>
#include <vector>

namespace {

class PvsMixTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;

    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csound->options_checked = 1;
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), 0);
        ASSERT_EQ(csoundCompileOrc(csound,
            "sr=8192\nksmps=16\nnchnls=1\n0dbfs=1\n"
            "gfFirst pvsinit 64, 16, 64, 1\n"
            "gfSecond pvsinit 64, 16, 64, 1\n"
            "instr 1\ngfMixed pvsmix gfFirst, gfSecond\nendin\n", 0), 0);
        ASSERT_EQ(csoundStart(csound), 0);
    }

    void TearDown() override { csoundDestroy(csound); }

    PVSDAT *signal(const char *name)
    {
        auto *variables = csoundDebugGetGlobalVariables(csound);
        PVSDAT *result = nullptr;
        for (auto *variable = variables; variable; variable = variable->next)
            if (std::strcmp(variable->name, name) == 0)
                result = static_cast<PVSDAT *>(variable->data);
        csoundDebugFreeVariables(csound, variables);
        return result;
    }

    void seedInputs(int format = PVS_AMP_FREQ)
    {
        const auto configuration = "64, 16, 64, 1, " + std::to_string(format);
        const auto orchestra = "gfFirst pvsinit " + configuration +
            "\ngfSecond pvsinit " + configuration + "\n";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), 0);
        auto *first = static_cast<float *>(signal("gfFirst")->frame.auxp);
        auto *second = static_cast<float *>(signal("gfSecond")->frame.auxp);
        for (int bin = 0; bin < 33; ++bin) {
            // Alternate which input wins, with a tie in every third bin.
            first[2 * bin] = bin % 3 == 0 ? .25f : bin % 3 == 1 ? .75f : .5f;
            second[2 * bin] = .5f;
            first[2 * bin + 1] = format == PVS_AMP_FREQ ? 100 + bin : bin / 32.0f;
            second[2 * bin + 1] = format == PVS_AMP_FREQ ? 200 + bin : -bin / 32.0f;
        }
    }

    void startNote()
    {
        csoundEventString(csound, "i 1 0 1", 0);
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
    }

    void checkSelectedBins()
    {
        auto *first = signal("gfFirst");
        auto *second = signal("gfSecond");
        auto *mixed = signal("gfMixed");
        ASSERT_NE(mixed, nullptr);
        ASSERT_EQ(mixed->N, first->N);
        ASSERT_EQ(mixed->NB, first->N / 2 + 1);
        ASSERT_EQ(mixed->format, first->format);
        const auto *a = static_cast<float *>(first->frame.auxp);
        const auto *b = static_cast<float *>(second->frame.auxp);
        const auto *out = static_cast<float *>(mixed->frame.auxp);
        for (int bin = 0; bin < 33; ++bin) {
            const auto *winner = a[2 * bin] >= b[2 * bin] ? a : b;
            EXPECT_EQ(out[2 * bin], winner[2 * bin]) << "magnitude at bin " << bin;
            EXPECT_EQ(out[2 * bin + 1], winner[2 * bin + 1])
                << "frequency/phase at bin " << bin;
        }
    }

    std::vector<float> snapshot()
    {
        auto *mixed = signal("gfMixed");
        const auto *data = static_cast<float *>(mixed->frame.auxp);
        return {data, data + mixed->N + 2};
    }
};

TEST_F(PvsMixTests, EachBinKeepsTheWinningMagnitudeAndFrequency)
{
    ASSERT_NO_FATAL_FAILURE(seedInputs());
    ASSERT_NO_FATAL_FAILURE(startNote());
    ASSERT_NO_FATAL_FAILURE(checkSelectedBins());
}

TEST_F(PvsMixTests, EqualMagnitudesKeepTheFirstInputsPhase)
{
    ASSERT_NO_FATAL_FAILURE(seedInputs(PVS_AMP_PHASE));
    ASSERT_NO_FATAL_FAILURE(startNote());
    ASSERT_NO_FATAL_FAILURE(checkSelectedBins());
}

TEST_F(PvsMixTests, EitherInputCanPublishANewFrame)
{
    ASSERT_NO_FATAL_FAILURE(seedInputs());
    ASSERT_NO_FATAL_FAILURE(startNote());
    for (const char *name : {"gfSecond", "gfFirst"}) {
        SCOPED_TRACE(name);
        auto *input = signal(name);
        auto *data = static_cast<float *>(input->frame.auxp);
        const auto oldOutput = snapshot();
        const auto oldCount = signal("gfMixed")->framecount;
        // Editing the buffer alone must not publish another frame.
        data[0] = 2;
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        EXPECT_EQ(snapshot(), oldOutput);
        EXPECT_EQ(signal("gfMixed")->framecount, oldCount);
        input->framecount++;
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        ASSERT_NO_FATAL_FAILURE(checkSelectedBins());
        EXPECT_GT(signal("gfMixed")->framecount, oldCount);
    }
}

TEST_F(PvsMixTests, ReinitializationRefreshesOutputMetadata)
{
    for (const auto *configuration : {"128, 1, 128, 1, 0", "64, 16, 64, 1, 1"}) {
        SCOPED_TRACE(configuration);
        const auto orchestra = std::string("gfFirst pvsinit ") + configuration +
            "\ngfSecond pvsinit " + configuration +
            "\ngfMixed pvsmix gfFirst, gfSecond\n";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), 0);
        auto *input = signal("gfFirst");
        auto *output = signal("gfMixed");
        ASSERT_NE(output, nullptr);
        EXPECT_EQ(output->N, input->N);
        EXPECT_EQ(output->NB, input->N / 2 + 1);
        EXPECT_EQ(output->sliding, input->sliding);
        EXPECT_EQ(output->format, input->format);
        EXPECT_EQ(output->overlap, input->overlap);
        EXPECT_EQ(output->winsize, input->winsize);
        EXPECT_EQ(output->wintype, input->wintype);
    }
}

TEST_F(PvsMixTests, MatchingNewInputsStillRequireMixerReinitialization)
{
    ASSERT_NO_FATAL_FAILURE(seedInputs());
    ASSERT_NO_FATAL_FAILURE(startNote());
    const auto before = snapshot();
    // Both sources still match each other, but no longer match the output.
    // The smaller frames fit the existing allocations.
    ASSERT_EQ(csoundCompileOrc(csound,
        "gfFirst pvsinit 32, 16, 32, 1\n"
        "gfSecond pvsinit 32, 16, 32, 1\n", 0), 0);
    csoundPerformKsmps(csound);
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(messages.find("pvsmix: source format changed; reinitialise"),
              std::string::npos);
    EXPECT_EQ(snapshot(), before);
}

} // namespace
