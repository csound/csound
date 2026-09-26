#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace {

class PvsgendyTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;

    void Start(int seed = 12345)
    {
        if (csound) csoundDestroy(csound);
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n -d -m0 --sample-accurate"), 0);
        const std::string orchestra = "seed " + std::to_string(seed) + R"(
            sr = 8192
            ksmps = 16
            nchnls = 1
            0dbfs = 1
            gfInput pvsinit 64, 16, 64, 1
            instr 1
                kAmplitude chnget "amplitude"
                kFrequency chnget "frequency"
                kReset chnget "reset"
                if kReset == 1 then
                    reinit PROCESS
                endif
            PROCESS:
                gfResult pvsgendy gfInput, kAmplitude, kFrequency
                rireturn
            endin
        )";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), 0);
        ASSERT_EQ(csoundStart(csound), 0);
    }

    void SetUp() override { Start(); }
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

    void PrepareInput(int size, int hop, int format = PVS_AMP_FREQ)
    {
        const auto orchestra = "gfInput pvsinit " + std::to_string(size) + ", " +
            std::to_string(hop) + ", " + std::to_string(size) + ", 1, " +
            std::to_string(format) + "\n";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), 0);
        auto *input = Signal("gfInput");
        ASSERT_NE(input, nullptr);
        const int samples = input->sliding ? 16 : 1;
        for (int sample = 0; sample < samples; ++sample) {
            for (int bin = 0; bin <= size / 2; ++bin) {
                const int index = sample * (size + 2) + 2 * bin;
                // Nonzero, distinct values expose omitted bins and stale samples.
                const MYFLT amplitude = .5 + bin / 256.0 + sample / 64.0;
                const MYFLT frequency = 100 + bin + sample / 4.0;
                if (input->sliding) {
                    auto *frame = static_cast<MYFLT *>(input->frame.auxp);
                    frame[index] = amplitude;
                    frame[index + 1] = frequency;
                }
                else {
                    auto *frame = static_cast<float *>(input->frame.auxp);
                    frame[index] = amplitude;
                    frame[index + 1] = frequency;
                }
            }
        }
    }

    void CheckCopy(int activeStart = 0, int activeEnd = 16)
    {
        auto *input = Signal("gfInput");
        auto *result = Signal("gfResult");
        ASSERT_NE(result, nullptr);
        ASSERT_EQ(result->N, input->N);
        ASSERT_EQ(result->NB, input->N / 2 + 1);
        ASSERT_EQ(result->sliding, input->sliding);
        ASSERT_EQ(result->format, input->format);
        ASSERT_EQ(result->overlap, input->overlap);
        ASSERT_EQ(result->winsize, input->winsize);
        ASSERT_EQ(result->wintype, input->wintype);
        const int samples = input->sliding ? 16 : 1;
        ASSERT_GE(result->frame.size, samples * (input->N + 2) *
                  (input->sliding ? sizeof(MYFLT) : sizeof(float)));
        for (int sample = 0; sample < samples; ++sample) {
            for (int slot = 0; slot < input->N + 2; ++slot) {
                const size_t index = sample * (input->N + 2) + slot;
                const double expected = sample < activeStart || sample >= activeEnd
                    ? 0 : Value(input, index);
                ASSERT_EQ(Value(result, index), expected)
                    << "sample " << sample << ", bin " << slot / 2
                    << (slot % 2 ? ", frequency" : ", amplitude");
            }
        }
    }

    std::vector<double> Snapshot()
    {
        auto *result = Signal("gfResult");
        std::vector<double> values;
        const int slots = (result->sliding ? 16 : 1) * (result->N + 2);
        for (int index = 0; index < slots; ++index)
            values.push_back(Value(result, index));
        return values;
    }

    std::string Messages()
    {
        std::string messages;
        while (csoundGetMessageCnt(csound)) {
            messages += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return messages;
    }
};

class PvsgendyModes : public PvsgendyTests,
    public ::testing::WithParamInterface<int> {};

TEST_P(PvsgendyModes, ZeroControlsCopyEveryBinIncludingNyquist)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInput(64, GetParam()));
    csoundEventString(csound, "i1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(CheckCopy());
}

TEST_P(PvsgendyModes, RandomOffsetsRespectTheDocumentedRanges)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInput(64, GetParam()));
    csoundSetControlChannel(csound, "amplitude", .25);
    csoundSetControlChannel(csound, "frequency", 100);
    csoundEventString(csound, "i1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    auto *input = Signal("gfInput");
    auto *result = Signal("gfResult");
    const int samples = input->sliding ? 16 : 1;
    bool changedAmplitude = false, changedFrequency = false;
    for (int sample = 0; sample < samples; ++sample) {
        for (int bin = 0; bin <= input->N / 2; ++bin) {
            const int index = sample * (input->N + 2) + 2 * bin;
            const double amplitude = Value(result, index) - Value(input, index);
            const double frequency = Value(result, index + 1) - Value(input, index + 1);
            const double frequencyBound = 50.0 / (input->sliding ? bin + 1 : 2*bin + 1);
            EXPECT_LE(std::abs(amplitude), input->sliding ? .125 : 0);
            EXPECT_LE(std::abs(frequency), frequencyBound + 1e-5);
            changedAmplitude |= amplitude != 0;
            changedFrequency |= frequency != 0;
        }
    }
    EXPECT_EQ(changedAmplitude, input->sliding != 0);
    EXPECT_TRUE(changedFrequency);
}

TEST_P(PvsgendyModes, SeedRepeatsTheSequenceWithoutSharingPerformanceState)
{
    std::vector<std::vector<double>> runs;
    for (int run = 0; run < 4; ++run) {
        ASSERT_NO_FATAL_FAILURE(Start(run == 3 ? 54321 : 12345));
        ASSERT_NO_FATAL_FAILURE(PrepareInput(64, GetParam()));
        csoundSetControlChannel(csound, "amplitude", .25);
        csoundSetControlChannel(csound, "frequency", 100);
        csoundEventString(csound, "i1 0 1", 0);
        std::vector<double> sequence;
        for (int block = 0; block < 3; ++block) {
            // Other random opcodes may consume the engine's seed after init.
            // This must not change this instance's ongoing random sequence.
            if (run == 2 && block > 0)
                for (int draw = 0; draw < 10; ++draw)
                    csound->Rand31(csound->RandSeed31(csound));
            ++Signal("gfInput")->framecount;
            ASSERT_EQ(csoundPerformKsmps(csound), 0);
            auto frame = Snapshot();
            sequence.insert(sequence.end(), frame.begin(), frame.end());
        }
        runs.push_back(sequence);
    }
    EXPECT_EQ(runs[0], runs[1]); // Same seed, same sequence.
    EXPECT_EQ(runs[0], runs[2]); // Unrelated draws do not change it.
    EXPECT_NE(runs[0], runs[3]); // A different seed changes the sequence.
}

TEST_P(PvsgendyModes, UpdatesAtTheRateOfItsInput)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInput(64, GetParam()));
    csoundSetControlChannel(csound, "frequency", 100);
    csoundEventString(csound, "i1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    const auto first = Snapshot();
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    if (GetParam() == 1) EXPECT_NE(Snapshot(), first);
    else EXPECT_EQ(Snapshot(), first);
    ++Signal("gfInput")->framecount;
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_NE(Snapshot(), first);
    EXPECT_EQ(Signal("gfResult")->framecount, Signal("gfInput")->framecount);
}

INSTANTIATE_TEST_SUITE_P(OrdinaryAndSliding, PvsgendyModes,
                        ::testing::Values(16, 1));

TEST_F(PvsgendyTests, ClearsSlidingSamplesOutsideTheNote)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInput(64, 1));
    // Start at sample 5 and play 40 samples: the last active sample is 44.
    csoundEventString(csound, "i1 0.0006103515625 0.0048828125", 0);
    for (int block = 0; block < 3; ++block) {
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        ASSERT_NO_FATAL_FAILURE(CheckCopy(block == 0 ? 5 : 0,
                                         block == 2 ? 13 : 16));
    }
}

TEST_F(PvsgendyTests, ReinitializesAcrossFrameSizesAndModes)
{
    csoundEventString(csound, "i1 0 1", 0);
    for (const auto &config : {std::pair<int, int>{64, 1}, {128, 32},
                              {96, 1}, {64, 16}, {64, 1}}) {
        ASSERT_NO_FATAL_FAILURE(PrepareInput(config.first, config.second));
        csoundSetControlChannel(csound, "reset", 1);
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        ASSERT_NO_FATAL_FAILURE(CheckCopy());
    }
}

TEST_F(PvsgendyTests, ProcessesOrdinaryFramesAfterTheCounterRestarts)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInput(64, 16));
    Signal("gfInput")->framecount = 10;
    csoundEventString(csound, "i1 0 1", 0);
    csoundSetControlChannel(csound, "frequency", 100);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSetControlChannel(csound, "frequency", 0);
    Signal("gfInput")->framecount = 1;
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(CheckCopy());
}

TEST_F(PvsgendyTests, ClearsReusedOutputUntilTheFirstOrdinaryFrame)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInput(64, 16));
    csoundEventString(csound, "i1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_GT(Value(Signal("gfResult"), 0), 0);
    Signal("gfInput")->framecount = 0;
    csoundSetControlChannel(csound, "reset", 1);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    for (double value : Snapshot()) EXPECT_EQ(value, 0);
}

TEST_F(PvsgendyTests, RequiresReinitializationWhenAnalysisSettingsChange)
{
    ASSERT_NO_FATAL_FAILURE(PrepareInput(64, 16));
    csoundEventString(csound, "i1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_NO_FATAL_FAILURE(PrepareInput(128, 32));
    csoundPerformKsmps(csound);
    EXPECT_NE(Messages().find("pvsgendy: analysis settings changed; reinitialise"),
              std::string::npos);
}

TEST_F(PvsgendyTests, RejectsPhaseAndComplexFrames)
{
    for (int format : {PVS_AMP_PHASE, PVS_COMPLEX}) {
        ASSERT_NO_FATAL_FAILURE(Start());
        ASSERT_NO_FATAL_FAILURE(PrepareInput(64, 16, format));
        csoundEventString(csound, "i1 0 1", 0);
        csoundPerformKsmps(csound);
        EXPECT_NE(Messages().find("pvsgendy: input format must be amp-freq"),
                  std::string::npos);
    }
}

TEST_F(PvsgendyTests, RejectsUsingTheInputAsTheOutput)
{
    ASSERT_EQ(csoundCompileOrc(csound, R"(
        instr 2
            gfInput pvsgendy gfInput, 0, 0
        endin
    )", 0), 0);
    csoundEventString(csound, "i2 0 1", 0);
    csoundPerformKsmps(csound);
    EXPECT_NE(Messages().find("pvsgendy: input and output must differ"),
              std::string::npos);
}

} // namespace
