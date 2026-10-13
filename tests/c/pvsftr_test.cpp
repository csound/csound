#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace {

class PvsftrTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    PVSDAT *signal = nullptr;
    cs_float *amplitudes = nullptr, *frequencies = nullptr;
    static constexpr int bins = 33;

    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csound->options_checked = 1;
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), CSOUND_SUCCESS);
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

    void prepare(const std::string &producer = "gfSignal pvsinit 64, 16, 64, 1\n",
                 int blockSize = 16)
    {
        const auto orchestra =
            "sr = 8192\nksmps = " + std::to_string(blockSize) +
            "\nnchnls = 1\n0dbfs = 1\n"
            "giAmps ftgen 1, 0, -33, -2, 0\n"
            "giFreqs ftgen 2, 0, -33, -2, 0\n" + producer +
            "instr 1\npvsftr gfSignal, p4, p5\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS)
            << messages();
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS) << messages();
        ASSERT_EQ(csoundGetTable(csound, &amplitudes, 1), bins);
        ASSERT_EQ(csoundGetTable(csound, &frequencies, 2), bins);
        std::fill(amplitudes, amplitudes + bins, FL(.5));
        std::fill(frequencies, frequencies + bins, FL(123.0));

        auto *variables = csoundDebugGetGlobalVariables(csound);
        for (auto *v = variables; v; v = v->next)
            if (std::strcmp(v->name, "gfSignal") == 0)
                signal = static_cast<PVSDAT *>(v->data);
        csoundDebugFreeVariables(csound, variables);
        ASSERT_NE(signal, nullptr);
        ASSERT_NE(signal->frame.auxp, nullptr);
    }

    void step()
    {
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
        ASSERT_EQ(csound->inerrcnt, 0) << messages();
        ASSERT_EQ(csound->perferrcnt, 0) << messages();
    }

    void start(int amplitudeTable = 1, int frequencyTable = 2)
    {
        const auto event = "i 1 0 1 " + std::to_string(amplitudeTable) +
            " " + std::to_string(frequencyTable);
        csoundEventString(csound, event.c_str(), 0);
        ASSERT_NO_FATAL_FAILURE(step());
    }

    void checkFrame(float amplitude, bool importedFrequencies = true)
    {
        const auto *frame = static_cast<const float *>(signal->frame.auxp);
        for (int bin = 0; bin < bins; ++bin) {
            EXPECT_EQ(frame[2 * bin], amplitude) << "amplitude bin " << bin;
            EXPECT_EQ(frame[2 * bin + 1], importedFrequencies ? 123.0f : bin * 128.0f)
                << "frequency bin " << bin;
        }
    }
};

TEST_F(PvsftrTests, ImportsAllBinsOnlyWhenTheProducerPublishesAFrame)
{
    ASSERT_NO_FATAL_FAILURE(prepare());
    ASSERT_NO_FATAL_FAILURE(start());
    checkFrame(.5f);

    // Editing the table alone must not overwrite the current spectral frame.
    std::fill(amplitudes, amplitudes + bins, FL(.75));
    ASSERT_NO_FATAL_FAILURE(step());
    checkFrame(.5f);
    signal->framecount++;
    ASSERT_NO_FATAL_FAILURE(step());
    checkFrame(.75f);
}

TEST_F(PvsftrTests, ImportsResumeWhenTheProducerIsReinitialized)
{
    ASSERT_NO_FATAL_FAILURE(prepare());
    ASSERT_NO_FATAL_FAILURE(start());
    signal->framecount = 10;
    ASSERT_NO_FATAL_FAILURE(step());

    // Restart the producer without reinitializing the table reader.
    ASSERT_EQ(csoundCompileOrc(csound, "gfSignal pvsinit 64, 16, 64, 1\n", 0),
              CSOUND_SUCCESS) << messages();
    ASSERT_EQ(signal->framecount, 1u);
    ASSERT_NO_FATAL_FAILURE(step());
    checkFrame(.5f);
}

TEST_F(PvsftrTests, ZeroAmplitudeTablePreservesTheSignalsAmplitudes)
{
    ASSERT_NO_FATAL_FAILURE(prepare());
    ASSERT_NO_FATAL_FAILURE(start(0, 2));
    checkFrame(0.0f);
}

TEST_F(PvsftrTests, ZeroFrequencyTablePreservesTheSignalsFrequencies)
{
    ASSERT_NO_FATAL_FAILURE(prepare());
    ASSERT_NO_FATAL_FAILURE(start(1, 0));
    checkFrame(.5f, false);
}

TEST_F(PvsftrTests, AcceptsOrdinaryFramesProducedWithSmallerLocalKsmps)
{
    ASSERT_NO_FATAL_FAILURE(prepare(
        "opcode MakeFrame, f, 0\n"
        "setksmps 8\n"
        "aIn init 0\n"
        "fOut pvsanal aIn, 64, 16, 64, 1\n"
        "xout fOut\n"
        "endop\n"
        "gfSignal MakeFrame\n", 32));
    ASSERT_FALSE(signal->sliding);
    ASSERT_EQ(signal->overlap, 16);
    ASSERT_NO_FATAL_FAILURE(start());
    checkFrame(.5f);
}

TEST_F(PvsftrTests, InvalidFrequencyTableDoesNotPartlyModifyTheSignal)
{
    ASSERT_NO_FATAL_FAILURE(prepare());
    csoundEventString(csound, "i 1 0 1 1 99", 0);
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->inerrcnt, 0);
    // Frequency-table lookup fails. The valid amplitude table must not be read.
    checkFrame(0.0f, false);
}

TEST_F(PvsftrTests, ChangedFftSizeRequiresReaderReinitialization)
{
    ASSERT_NO_FATAL_FAILURE(prepare());
    ASSERT_NO_FATAL_FAILURE(start());
    signal->framecount = 10;
    ASSERT_NO_FATAL_FAILURE(step());
    ASSERT_EQ(csoundCompileOrc(csound, "gfSignal pvsinit 128, 32, 128, 1\n", 0),
              CSOUND_SUCCESS) << messages();
    csoundPerformKsmps(csound);
    EXPECT_EQ(csound->perferrcnt, 1);
    EXPECT_NE(messages().find("pvsftr: signal format changed"), std::string::npos);
    const auto *frame = static_cast<const float *>(signal->frame.auxp);
    for (int bin = 0; bin <= 64; ++bin) {
        EXPECT_EQ(frame[2 * bin], 0.0f) << bin;
        EXPECT_EQ(frame[2 * bin + 1], bin * 64.0f) << bin;
    }
}

TEST_F(PvsftrTests, RejectsActualSlidingFrames)
{
    ASSERT_NO_FATAL_FAILURE(prepare("gfSignal pvsinit 64, 1, 64, 1\n"));
    ASSERT_TRUE(signal->sliding);
    csoundEventString(csound, "i 1 0 1 1 2", 0);
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->inerrcnt, 0);
    EXPECT_NE(messages().find("Sliding version not yet available"), std::string::npos);
}

TEST_F(PvsftrTests, FrequencyTableCanSupplyPhaseValues)
{
    ASSERT_NO_FATAL_FAILURE(prepare("gfSignal pvsinit 64, 16, 64, 1, 1\n"));
    ASSERT_EQ(signal->format, PVS_AMP_PHASE);
    std::fill(frequencies, frequencies + bins, FL(.25));
    ASSERT_NO_FATAL_FAILURE(start(0, 2));
    const auto *frame = static_cast<const float *>(signal->frame.auxp);
    for (int bin = 0; bin < bins; ++bin) {
        EXPECT_EQ(frame[2 * bin], 0.0f) << bin;
        EXPECT_EQ(frame[2 * bin + 1], .25f) << bin;
    }
}

} // namespace
