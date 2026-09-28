#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <cstring>
#include <string>
#include <vector>

namespace {

class PvsGainTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;

    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csound->options_checked = 1;
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n -d -m0 --sample-accurate"), 0);
        ASSERT_EQ(csoundCompileOrc(csound,
            "sr=8192\nksmps=16\nnchnls=1\n0dbfs=1\n"
            "gfSource pvsinit 64, 16, 64, 1\n"
            "instr 1\n"
            "kGain chnget \"gain\"\n"
            "gfScaled pvsgain gfSource, kGain\n"
            "endin\n", 0), 0);
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

    void prepareSource(int hop = 16, int format = PVS_AMP_FREQ)
    {
        const auto source = "gfSource pvsinit 64, " + std::to_string(hop) +
            ", 64, 1, " + std::to_string(format) + "\n";
        ASSERT_EQ(csoundCompileOrc(csound, source.c_str(), 0), 0);
        auto *input = signal("gfSource");
        ASSERT_NE(input, nullptr);
        const int samples = input->sliding ? 16 : 1;
        for (int sample = 0; sample < samples; ++sample) {
            for (int bin = 0; bin < 33; ++bin) {
                const int index = sample * 66 + 2 * bin;
                // Binary fractions make every expected product exact.
                const cs_float magnitude = .5 + bin / 64.0 + sample / 32.0;
                const cs_float second = bin / 16.0 + sample / 64.0;
                if (input->sliding) {
                    auto *frame = static_cast<cs_float *>(input->frame.auxp);
                    frame[index] = magnitude;
                    frame[index + 1] = second;
                }
                else {
                    auto *frame = static_cast<float *>(input->frame.auxp);
                    frame[index] = magnitude;
                    frame[index + 1] = second;
                }
            }
        }
    }

    static cs_double value(PVSDAT *frame, int index)
    {
        return frame->sliding ? static_cast<cs_float *>(frame->frame.auxp)[index]
                              : static_cast<float *>(frame->frame.auxp)[index];
    }

    void checkGain(cs_double gain, int activeStart = 0, int activeEnd = 16)
    {
        auto *input = signal("gfSource");
        auto *output = signal("gfScaled");
        ASSERT_NE(output, nullptr);
        ASSERT_EQ(output->N, input->N);
        ASSERT_EQ(output->NB, input->N / 2 + 1);
        ASSERT_EQ(output->sliding, input->sliding);
        ASSERT_EQ(output->format, input->format);
        const int samples = input->sliding ? 16 : 1;
        for (int sample = 0; sample < samples; ++sample) {
            for (int bin = 0; bin < 33; ++bin) {
                const int index = sample * 66 + 2 * bin;
                const bool active = sample >= activeStart && sample < activeEnd;
                EXPECT_EQ(value(output, index), active ? gain * value(input, index) : 0)
                    << "sample " << sample << ", bin " << bin;
                EXPECT_EQ(value(output, index + 1), active ? value(input, index + 1) : 0)
                    << "frequency/phase at sample " << sample << ", bin " << bin;
            }
        }
    }
};

TEST_F(PvsGainTests, OrdinaryFramesScaleAllBinsAndPreserveFrequencies)
{
    ASSERT_NO_FATAL_FAILURE(prepareSource());
    csoundSetControlChannel(csound, "gain", 2);
    csoundEventString(csound, "i 1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(checkGain(2));
}

TEST_F(PvsGainTests, PhaseFramesKeepTheirPhaseValues)
{
    ASSERT_NO_FATAL_FAILURE(prepareSource(16, PVS_AMP_PHASE));
    csoundSetControlChannel(csound, "gain", .5);
    csoundEventString(csound, "i 1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(checkGain(.5));
}

TEST_F(PvsGainTests, GainChangesWaitForANewOrdinaryFrame)
{
    ASSERT_NO_FATAL_FAILURE(prepareSource());
    csoundSetControlChannel(csound, "gain", 2);
    csoundEventString(csound, "i 1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    const auto before = signal("gfScaled")->framecount;
    csoundSetControlChannel(csound, "gain", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(checkGain(2));
    EXPECT_EQ(signal("gfScaled")->framecount, before);
    signal("gfSource")->framecount++;
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(checkGain(0));
    EXPECT_GT(signal("gfScaled")->framecount, before);
}

TEST_F(PvsGainTests, SlidingFramesRespectBothNoteBoundaries)
{
    ASSERT_NO_FATAL_FAILURE(prepareSource(1));
    csoundSetControlChannel(csound, "gain", 2);
    // Start at sample 5, stop before sample 29 (sample 13 of the next block).
    csoundEventString(csound, "i 1 0.0006103515625 0.0029296875", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(checkGain(2, 5, 16));
    csoundSetControlChannel(csound, "gain", .5);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(checkGain(.5, 0, 13));
}

TEST_F(PvsGainTests, ReinitializationRefreshesOutputMetadata)
{
    for (const auto *configuration : {"128, 1, 128, 1, 0", "64, 16, 64, 1, 1"}) {
        SCOPED_TRACE(configuration);
        const auto source = std::string("gfSource pvsinit ") + configuration +
            "\ngfScaled pvsgain gfSource, 2\n";
        ASSERT_EQ(csoundCompileOrc(csound, source.c_str(), 0), 0);
        auto *input = signal("gfSource");
        auto *output = signal("gfScaled");
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

TEST_F(PvsGainTests, AChangedSourceSizeRequiresReinitialization)
{
    ASSERT_NO_FATAL_FAILURE(prepareSource());
    csoundSetControlChannel(csound, "gain", 2);
    csoundEventString(csound, "i 1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    auto *output = signal("gfScaled");
    auto *data = static_cast<float *>(output->frame.auxp);
    const std::vector<float> before(data, data + 66);
    // A smaller, valid source fits both allocations but changes the frame layout.
    ASSERT_EQ(csoundCompileOrc(csound, "gfSource pvsinit 32, 16, 32, 1\n", 0), 0);
    csoundPerformKsmps(csound);
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(messages.find("pvsgain: source format changed; reinitialise"),
              std::string::npos);
    EXPECT_EQ(std::vector<float>(data, data + 66), before);
}

} // namespace
