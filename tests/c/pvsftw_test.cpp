#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace {

class PvsftwTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    PVSDAT *source = nullptr;
    cs_float *amplitudes = nullptr;
    cs_float *frequencies = nullptr;
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

    void step()
    {
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
        ASSERT_EQ(csound->inerrcnt, 0) << messages();
        ASSERT_EQ(csound->perferrcnt, 0) << messages();
    }

    void start(const std::string &tables, int format = PVS_AMP_FREQ)
    {
        const std::string orchestra =
            "sr = 8192\nksmps = 16\nnchnls = 1\n0dbfs = 1\n"
            "giAmps ftgen 1, 0, -33, -2, 0\n"
            "giFreqs ftgen 2, 0, -33, -2, 0\n"
            "gfSource pvsinit 64, 16, 64, 1, " + std::to_string(format) + "\n"
            "instr 1\n"
            "kNew pvsftw gfSource, " + tables + "\n"
            "iAmp table 1, giAmps\n"
            "iFreq table 1, giFreqs\n"
            "chnset iAmp, \"initial_amplitude\"\n"
            "chnset iFreq, \"initial_frequency\"\n"
            "chnset kNew, \"new_frame\"\n"
            "endin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS)
            << messages();
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS) << messages();
        ASSERT_EQ(csoundGetTable(csound, &amplitudes, 1), bins);
        ASSERT_EQ(csoundGetTable(csound, &frequencies, 2), bins);
        // Disabled tables must retain these values at both init and performance.
        std::fill(amplitudes, amplitudes + bins, FL(-1.0));
        std::fill(frequencies, frequencies + bins, FL(-1.0));

        auto *variables = csoundDebugGetGlobalVariables(csound);
        for (auto *variable = variables; variable; variable = variable->next)
            if (std::strcmp(variable->name, "gfSource") == 0)
                source = static_cast<PVSDAT *>(variable->data);
        csoundDebugFreeVariables(csound, variables);
        ASSERT_NE(source, nullptr);

        csoundEventString(csound, "i 1 0 1", 0);
        ASSERT_NO_FATAL_FAILURE(step());
    }

    cs_float channel(const char *name)
    {
        return csoundGetControlChannel(csound, name, nullptr);
    }

    void publishFrame(uint32_t frameNumber)
    {
        auto *frame = static_cast<float *>(source->frame.auxp);
        for (int bin = 0; bin < bins; ++bin) {
            frame[2 * bin] = bin + .5f;
            frame[2 * bin + 1] = bin * 128.0f + 1;
        }
        source->framecount = frameNumber;
    }

    void checkExport(bool writeAmplitudes, bool writeFrequencies)
    {
        for (int bin = 0; bin < bins; ++bin) {
            EXPECT_EQ(amplitudes[bin], writeAmplitudes ? bin + FL(.5) : FL(-1.0))
                << "amplitude bin " << bin;
            EXPECT_EQ(frequencies[bin], writeFrequencies ? bin * FL(128.0) + 1 : FL(-1.0))
                << "frequency bin " << bin;
        }
        EXPECT_EQ(channel("new_frame"), FL(1.0));
    }
};

TEST_F(PvsftwTests, ZeroAmplitudeTableExportsOnlyFrequencies)
{
    ASSERT_NO_FATAL_FAILURE(start("0, 2"));
    EXPECT_EQ(channel("initial_amplitude"), FL(-1.0));
    EXPECT_EQ(channel("initial_frequency"), FL(128.0));
    publishFrame(2);
    ASSERT_NO_FATAL_FAILURE(step());
    checkExport(false, true);
}

TEST_F(PvsftwTests, OmittedFrequencyTableExportsOnlyAmplitudes)
{
    ASSERT_NO_FATAL_FAILURE(start("1"));
    EXPECT_EQ(channel("initial_amplitude"), FL(0.0));
    EXPECT_EQ(channel("initial_frequency"), FL(-1.0));
    publishFrame(2);
    ASSERT_NO_FATAL_FAILURE(step());
    checkExport(true, false);
}

TEST_F(PvsftwTests, BothTablesIncludeDcAndNyquistAndHoldBetweenFrames)
{
    ASSERT_NO_FATAL_FAILURE(start("1, 2"));
    publishFrame(2);
    ASSERT_NO_FATAL_FAILURE(step());
    checkExport(true, true);

    // A table edit must survive until the producer publishes another frame.
    amplitudes[0] = FL(42.0);
    frequencies[bins - 1] = FL(99.0);
    ASSERT_NO_FATAL_FAILURE(step());
    EXPECT_EQ(channel("new_frame"), FL(0.0));
    EXPECT_EQ(amplitudes[0], FL(42.0));
    EXPECT_EQ(frequencies[bins - 1], FL(99.0));
}

TEST_F(PvsftwTests, BothTablesDisabledStillReportsNewFrames)
{
    ASSERT_NO_FATAL_FAILURE(start("0, 0"));
    publishFrame(2);
    ASSERT_NO_FATAL_FAILURE(step());
    checkExport(false, false);
    ASSERT_NO_FATAL_FAILURE(step());
    EXPECT_EQ(channel("new_frame"), FL(0.0));
}

TEST_F(PvsftwTests, ReinitializingTheSourceDoesNotFreezeTableUpdates)
{
    ASSERT_NO_FATAL_FAILURE(start("1, 2"));
    publishFrame(10);
    ASSERT_NO_FATAL_FAILURE(step());
    checkExport(true, true);

    // Restart the producer without reinitializing its table writer.
    ASSERT_EQ(csoundCompileOrc(csound, "gfSource pvsinit 64, 16, 64, 1\n", 0),
              CSOUND_SUCCESS) << messages();
    ASSERT_EQ(source->framecount, 1u);
    ASSERT_NO_FATAL_FAILURE(step());
    EXPECT_EQ(channel("new_frame"), FL(1.0));
    for (int bin = 0; bin < bins; ++bin) {
        EXPECT_EQ(amplitudes[bin], FL(0.0)) << bin;
        EXPECT_EQ(frequencies[bin], bin * FL(128.0)) << bin;
    }
}

TEST_F(PvsftwTests, FrequencyTableCanCarryPhaseData)
{
    ASSERT_NO_FATAL_FAILURE(start("0, 2", PVS_AMP_PHASE));
    EXPECT_EQ(channel("initial_frequency"), FL(0.0));
    auto *frame = static_cast<float *>(source->frame.auxp);
    for (int bin = 0; bin < bins; ++bin)
        frame[2 * bin + 1] = (bin - 16) / 16.0f;
    source->framecount++;
    ASSERT_NO_FATAL_FAILURE(step());
    for (int bin = 0; bin < bins; ++bin) {
        EXPECT_EQ(amplitudes[bin], FL(-1.0)) << bin;
        EXPECT_EQ(frequencies[bin], (bin - 16) / FL(16.0)) << bin;
    }
}

TEST_F(PvsftwTests, ChangingFftSizeRequiresWriterReinitialization)
{
    ASSERT_NO_FATAL_FAILURE(start("1, 2"));
    publishFrame(10);
    ASSERT_NO_FATAL_FAILURE(step());

    ASSERT_EQ(csoundCompileOrc(csound, "gfSource pvsinit 128, 32, 128, 1\n", 0),
              CSOUND_SUCCESS) << messages();
    csoundPerformKsmps(csound);
    EXPECT_EQ(csound->perferrcnt, 1);
    EXPECT_NE(messages().find("pvsftw: source format changed"), std::string::npos);
    // Reject the new layout before changing either destination table.
    checkExport(true, true);
}

} // namespace
