#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace {

class PvsmaskaTests : public ::testing::Test {
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
            "giMask ftgen 1, 0, 128, -7, 0, 128, 4\n"
            "gfInput pvsinit 64, 32, 64, 1\n"
            "instr 1\n"
            "kDepth chnget \"depth\"\n"
            "kReset chnget \"reset\"\n"
            "if kReset == 1 then\nreinit MASK\nendif\n"
            "MASK:\ngfOutput pvsmaska gfInput, giMask, kDepth\n"
            "rireturn\nendin\n", 0), CSOUND_SUCCESS);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
        csoundEventString(csound, "i 1 0 1", 0);
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
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

    void prepareInput(int size, int hop, int format)
    {
        const auto statement = "gfInput pvsinit " + std::to_string(size) + ", " +
            std::to_string(hop) + ", " + std::to_string(size) + ", 1, " +
            std::to_string(format) + "\n";
        ASSERT_EQ(csoundCompileOrc(csound, statement.c_str(), 0), CSOUND_SUCCESS);
        auto *input = signal("gfInput");
        ASSERT_NE(input, nullptr);
        const int samples = input->sliding ? 16 : 1;
        for (int sample = 0; sample < samples; ++sample)
            for (int bin = 0; bin <= size / 2; ++bin) {
                const int index = sample * (size + 2) + 2 * bin;
                const MYFLT amplitude = FL(1.0) + sample / FL(16.0);
                const MYFLT second = FL(.125) + bin / FL(128.0);
                if (input->sliding) {
                    auto *data = static_cast<MYFLT *>(input->frame.auxp);
                    data[index] = amplitude;
                    data[index + 1] = second;
                }
                else {
                    auto *data = static_cast<float *>(input->frame.auxp);
                    data[index] = amplitude;
                    data[index + 1] = second;
                }
            }
    }

    void checkOutput(double requestedDepth)
    {
        auto *input = signal("gfInput");
        auto *output = signal("gfOutput");
        ASSERT_NE(output, nullptr);
        ASSERT_EQ(output->N, input->N);
        ASSERT_EQ(output->NB, input->NB);
        ASSERT_EQ(output->sliding, input->sliding);
        ASSERT_EQ(output->format, input->format);
        const size_t samples = input->sliding ? 16 : 1;
        ASSERT_GE(output->frame.size, samples * (input->N + 2) *
                  (input->sliding ? sizeof(MYFLT) : sizeof(float)));
        MYFLT *mask = nullptr;
        ASSERT_EQ(csoundGetTable(csound, &mask, 1), 128);
        const double depth = std::clamp(requestedDepth, 0.0, 1.0);
        for (size_t sample = 0; sample < samples; ++sample)
            for (int bin = 0; bin <= input->N / 2; ++bin) {
                const size_t index = sample * (input->N + 2) + 2 * bin;
                const double amplitude = input->sliding
                    ? static_cast<MYFLT *>(output->frame.auxp)[index]
                    : static_cast<float *>(output->frame.auxp)[index];
                const double second = input->sliding
                    ? static_cast<MYFLT *>(output->frame.auxp)[index + 1]
                    : static_cast<float *>(output->frame.auxp)[index + 1];
                // Depth blends unity gain with the mask; frequency/phase stays intact.
                EXPECT_DOUBLE_EQ(amplitude, (1.0 + sample / 16.0) *
                    ((1.0 - depth) + depth * mask[bin])) << sample << ":" << bin;
                EXPECT_DOUBLE_EQ(second, .125 + bin / 128.0) << sample << ":" << bin;
            }
    }
};

TEST_F(PvsmaskaTests, ReinitializeAcrossSizesModesAndFormats)
{
    struct Configuration { int size, hop, format; double depth; };
    for (const auto &config : {Configuration{64, 1, PVS_AMP_FREQ, .5},
                              Configuration{128, 32, PVS_AMP_PHASE, 1},
                              Configuration{256, 1, PVS_AMP_PHASE, 0},
                              Configuration{64, 32, PVS_AMP_FREQ, .25},
                              Configuration{256, 1, PVS_AMP_FREQ, 1}}) {
        SCOPED_TRACE(std::to_string(config.size) + "/" + std::to_string(config.hop));
        ASSERT_NO_FATAL_FAILURE(prepareInput(config.size, config.hop, config.format));
        csoundSetControlChannel(csound, "depth", config.depth);
        csoundSetControlChannel(csound, "reset", 1);
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        ASSERT_NO_FATAL_FAILURE(checkOutput(config.depth));
        csoundSetControlChannel(csound, "reset", 0);
    }
}

TEST_F(PvsmaskaTests, SlidingDepthChangesAndClamping)
{
    ASSERT_NO_FATAL_FAILURE(prepareInput(128, 1, PVS_AMP_FREQ));
    csoundSetControlChannel(csound, "reset", 1);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    csoundSetControlChannel(csound, "reset", 0);
    // Sliding data and depth can change every block without advancing a frame counter.
    for (double depth : {0.0, .5, 1.0, -1.0, 2.0}) {
        csoundSetControlChannel(csound, "depth", depth);
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        ASSERT_NO_FATAL_FAILURE(checkOutput(depth));
    }
}

TEST_F(PvsmaskaTests, ChangedInputLayoutRequiresReinitialization)
{
    ASSERT_NO_FATAL_FAILURE(prepareInput(128, 1, PVS_AMP_FREQ));
    // Leave the existing ordinary-frame output in place. The opcode must report
    // the changed layout before reading or writing any sample frames.
    csoundPerformKsmps(csound);
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(messages.find("pvsmaska: input format changed; reinitialise pvsmaska"),
              std::string::npos);
}

} // namespace
