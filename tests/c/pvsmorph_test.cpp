#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <cstring>
#include <string>
#include <tuple>

namespace {

class PvsmorphTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;

    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n -d -m0 --sample-accurate"), 0);
        ASSERT_EQ(csoundCompileOrc(csound, R"(
            sr = 8192
            ksmps = 16
            nchnls = 1
            0dbfs = 1
            gfFirst pvsinit 64, 16, 64, 1
            gfSecond pvsinit 64, 16, 64, 1
            instr 1
                kAmplitude chnget "amplitudeBlend"
                kFrequency chnget "frequencyBlend"
                kReset chnget "reset"
                if kReset == 1 then
                    reinit MORPH
                endif
            MORPH:
                gfResult pvsmorph gfFirst, gfSecond, kAmplitude, kFrequency
                rireturn
            endin
        )", 0), 0);
        ASSERT_EQ(csoundStart(csound), 0);
    }

    void TearDown() override { csoundDestroy(csound); }

    PVSDAT *Signal(const char *name)
    {
        auto *variables = csoundDebugGetGlobalVariables(csound);
        PVSDAT *result = nullptr;
        for (auto *variable = variables; variable; variable = variable->next)
            if (std::strcmp(variable->name, name) == 0)
                result = static_cast<PVSDAT *>(variable->data);
        csoundDebugFreeVariables(csound, variables);
        return result;
    }

    static double Value(PVSDAT *signal, size_t index)
    {
        return signal->sliding
            ? static_cast<MYFLT *>(signal->frame.auxp)[index]
            : static_cast<float *>(signal->frame.auxp)[index];
    }

    void PrepareInputs(int size, int hop, int format = PVS_AMP_FREQ)
    {
        const auto arguments = std::to_string(size) + ", " + std::to_string(hop) +
            ", " + std::to_string(size) + ", 1, " + std::to_string(format) + "\n";
        const auto orchestra = "gfFirst pvsinit " + arguments +
                               "gfSecond pvsinit " + arguments;
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), 0);
        for (const char *name : {"gfFirst", "gfSecond"}) {
            auto *signal = Signal(name);
            ASSERT_NE(signal, nullptr);
            const bool first = std::strcmp(name, "gfFirst") == 0;
            const int samples = signal->sliding ? 16 : 1;
            for (int sample = 0; sample < samples; ++sample) {
                for (int bin = 0; bin <= size / 2; ++bin) {
                    const int index = sample * (size + 2) + 2 * bin;
                    // Distinct bins and samples expose incomplete frame processing.
                    const MYFLT amplitude = (first ? .25 : .5) +
                        bin / 256.0 + sample / 64.0;
                    const MYFLT second = format == PVS_AMP_PHASE
                        ? (first ? -.5 : .75) + bin / 256.0 + sample / 128.0
                        : (first ? 100 : 500) + bin + sample / 4.0;
                    if (signal->sliding) {
                        auto *frame = static_cast<MYFLT *>(signal->frame.auxp);
                        frame[index] = amplitude;
                        frame[index + 1] = second;
                    }
                    else {
                        auto *frame = static_cast<float *>(signal->frame.auxp);
                        frame[index] = amplitude;
                        frame[index + 1] = second;
                    }
                }
            }
        }
    }

    void SetBlend(MYFLT amplitude, MYFLT frequency)
    {
        csoundSetControlChannel(csound, "amplitudeBlend", amplitude);
        csoundSetControlChannel(csound, "frequencyBlend", frequency);
    }

    void CheckResult(double amplitude, double frequency,
                     int activeStart = 0, int activeEnd = 16)
    {
        auto *first = Signal("gfFirst");
        auto *second = Signal("gfSecond");
        auto *result = Signal("gfResult");
        ASSERT_NE(result, nullptr);
        ASSERT_EQ(result->N, first->N);
        ASSERT_EQ(result->NB, first->N / 2 + 1);
        ASSERT_EQ(result->sliding, first->sliding);
        ASSERT_EQ(result->format, first->format);
        ASSERT_EQ(result->overlap, first->overlap);
        ASSERT_EQ(result->winsize, first->winsize);
        ASSERT_EQ(result->wintype, first->wintype);
        const int samples = first->sliding ? 16 : 1;
        ASSERT_GE(result->frame.size, samples * (first->N + 2) *
                  (first->sliding ? sizeof(MYFLT) : sizeof(float)));
        for (int sample = 0; sample < samples; ++sample) {
            for (int slot = 0; slot < first->N + 2; ++slot) {
                const size_t index = sample * (first->N + 2) + slot;
                const double blend = slot % 2 ? frequency : amplitude;
                const double expected = sample < activeStart || sample >= activeEnd
                    ? 0 : (1-blend) * Value(first, index) + blend * Value(second, index);
                ASSERT_DOUBLE_EQ(Value(result, index), expected)
                    << "sample " << sample << ", bin " << slot / 2
                    << (slot % 2 ? ", frequency/phase" : ", amplitude");
            }
        }
    }

    std::string Messages()
    {
        std::string result;
        while (csoundGetMessageCnt(csound)) {
            result += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return result;
    }
};

class PvsmorphFormats : public PvsmorphTests,
    public ::testing::WithParamInterface<std::tuple<int, int>> {};

TEST_P(PvsmorphFormats, InterpolatesEveryBinWithIndependentControls)
{
    const auto [hop, format] = GetParam();
    ASSERT_NO_FATAL_FAILURE(PrepareInputs(64, hop, format));
    csoundEventString(csound, "i1 0 1", 0);
    for (const auto &blend : {std::pair<double, double>{0, 0}, {1, 1},
                             {0, 1}, {1, 0}, {.25, .75}}) {
        SCOPED_TRACE(::testing::Message() << blend.first << ", " << blend.second);
        SetBlend(blend.first, blend.second);
        ++Signal("gfFirst")->framecount;
        // Separate producers do not need identical frame counters.
        Signal("gfSecond")->framecount = 42;
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        ASSERT_NO_FATAL_FAILURE(CheckResult(blend.first, blend.second));
    }
    SetBlend(-1, 2);
    ++Signal("gfFirst")->framecount;
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(CheckResult(0, 1)); // Controls clamp to [0, 1].

    // Ordinary frames hold until the first input advances. Sliding frames
    // must apply new controls on every block, even with a fixed frame counter.
    SetBlend(1, 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(CheckResult(hop == 1 ? 1 : 0, hop == 1 ? 0 : 1));
}

INSTANTIATE_TEST_SUITE_P(OrdinaryAndSliding, PvsmorphFormats,
    ::testing::Combine(::testing::Values(16, 1),
                       ::testing::Values(PVS_AMP_FREQ, PVS_AMP_PHASE)));

TEST_F(PvsmorphTests, ReinitializesAcrossFrameSizesModesAndFormats)
{
    SetBlend(.25, .75);
    csoundEventString(csound, "i1 0 1", 0);
    struct Configuration { int size, hop, format; };
    for (const auto &config : {Configuration{64, 1, PVS_AMP_FREQ},
                              {128, 32, PVS_AMP_FREQ}, {96, 1, PVS_AMP_PHASE},
                              {64, 16, PVS_AMP_PHASE}, {64, 1, PVS_AMP_FREQ}}) {
        ASSERT_NO_FATAL_FAILURE(PrepareInputs(config.size, config.hop, config.format));
        csoundSetControlChannel(csound, "reset", 1);
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        ASSERT_NO_FATAL_FAILURE(CheckResult(.25, .75));
    }
}

TEST_F(PvsmorphTests, ClearsReusedOutputUntilTheFirstOrdinaryFrame)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInputs(64, 16));
    csoundEventString(csound, "i1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_GT(Value(Signal("gfResult"), 0), 0);
    Signal("gfFirst")->framecount = 0;
    csoundSetControlChannel(csound, "reset", 1);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    auto *result = Signal("gfResult");
    for (int slot = 0; slot < result->N + 2; ++slot)
        EXPECT_EQ(Value(result, slot), 0) << slot;
}

TEST_F(PvsmorphTests, ProcessesOrdinaryFramesAfterTheCounterRestarts)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInputs(64, 16));
    Signal("gfFirst")->framecount = 10;
    csoundEventString(csound, "i1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    SetBlend(1, 1);
    Signal("gfFirst")->framecount = 1;
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(CheckResult(1, 1));
}

TEST_F(PvsmorphTests, ClearsSlidingSamplesOutsideTheNote)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInputs(64, 1));
    SetBlend(.25, .75);
    // Start at sample 5 and play 40 samples: the last active sample is 44.
    csoundEventString(csound, "i1 0.0006103515625 0.0048828125", 0);
    for (int block = 0; block < 3; ++block) {
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        ASSERT_NO_FATAL_FAILURE(CheckResult(.25, .75,
            block == 0 ? 5 : 0, block == 2 ? 13 : 16));
    }
}

class PvsmorphMismatches : public PvsmorphTests,
    public ::testing::WithParamInterface<const char *> {};

TEST_P(PvsmorphMismatches, RejectsDifferentAnalysisSettingsAtInitialization)
{
    const auto orchestra = std::string("gfSecond pvsinit ") + GetParam() + "\n";
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), 0);
    csoundEventString(csound, "i1 0 1", 0);
    csoundPerformKsmps(csound);
    EXPECT_NE(Messages().find("pvsmorph: inputs must have matching analysis settings"),
              std::string::npos);
}

INSTANTIATE_TEST_SUITE_P(AnalysisSettings, PvsmorphMismatches, ::testing::Values(
    "32, 16, 32, 1",    // Smaller FFT.
    "128, 16, 128, 1",  // Larger FFT.
    "64, 32, 64, 1",    // Different hop size.
    "64, 16, 128, 1",   // Different window size.
    "64, 16, 64, 0",    // Different window type.
    "64, 16, 64, 1, 1", // Phase instead of frequency.
    "64, 1, 64, 1"));   // Sliding instead of ordinary frames.

TEST_F(PvsmorphTests, RequiresReinitializationAfterAnInputChangesFormat)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInputs(64, 16));
    csoundEventString(csound, "i1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_EQ(csoundCompileOrc(csound, "gfSecond pvsinit 128, 32, 128, 1\n", 0), 0);
    csoundPerformKsmps(csound);
    EXPECT_NE(Messages().find("pvsmorph: analysis settings changed; reinitialise"),
              std::string::npos);
}

TEST_F(PvsmorphTests, RejectsComplexFrames)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInputs(64, 16, PVS_COMPLEX));
    csoundEventString(csound, "i1 0 1", 0);
    csoundPerformKsmps(csound);
    EXPECT_NE(Messages().find("pvsmorph: input format must be amp-freq or amp-phase"),
              std::string::npos);
}

TEST_F(PvsmorphTests, RejectsUsingAnInputAsTheOutput)
{
    ASSERT_EQ(csoundCompileOrc(csound, R"(
        instr 2
            gfFirst pvsmorph gfFirst, gfSecond, 0, 0
        endin
    )", 0), 0);
    csoundEventString(csound, "i2 0 1", 0);
    csoundPerformKsmps(csound);
    EXPECT_NE(Messages().find("pvsmorph: output must differ from inputs"),
              std::string::npos);
}

} // namespace
