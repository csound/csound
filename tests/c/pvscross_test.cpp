#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <cstring>
#include <string>

namespace {

class PvscrossTests : public ::testing::Test {
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
            "gfLeft pvsinit 64, 32, 64, 1\n"
            "gfRight pvsinit 64, 32, 64, 1\n"
            "instr 1\n"
            "kLeft chnget \"leftGain\"\n"
            "kRight chnget \"rightGain\"\n"
            "kReset chnget \"reset\"\n"
            "if kReset == 1 then\nreinit MIX\nendif\n"
            "MIX:\n"
            "gfMixed pvscross gfLeft, gfRight, kLeft, kRight\n"
            "rireturn\nendin\n", 0), CSOUND_SUCCESS);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
        csoundSetControlChannel(csound, "leftGain", -.5);
        csoundSetControlChannel(csound, "rightGain", 2);
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

    void setInputs(int size, int hop, int format)
    {
        const auto parameters = std::to_string(size) + ", " + std::to_string(hop) +
            ", " + std::to_string(size) + ", 1, " + std::to_string(format) + "\n";
        const auto orchestra = "gfLeft pvsinit " + parameters +
                               "gfRight pvsinit " + parameters;
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
        for (const char *name : {"gfLeft", "gfRight"}) {
            auto *frame = signal(name);
            ASSERT_NE(frame, nullptr);
            const bool left = std::strcmp(name, "gfLeft") == 0;
            const int samples = frame->sliding ? 16 : 1;
            for (int sample = 0; sample < samples; ++sample)
                for (int bin = 0; bin <= size / 2; ++bin) {
                    const int index = sample * (size + 2) + 2 * bin;
                    const MYFLT amplitude = (left ? FL(.25) : FL(.5)) +
                        bin / FL(256.0) + sample / FL(64.0);
                    const MYFLT second = format == PVS_AMP_PHASE
                        ? (left ? FL(.125) : FL(.5)) + bin / FL(256.0) + sample / FL(128.0)
                        : (left ? FL(100.0) : FL(500.0)) + bin + sample / FL(4.0);
                    if (frame->sliding) {
                        auto *data = static_cast<MYFLT *>(frame->frame.auxp);
                        data[index] = amplitude;
                        data[index + 1] = second;
                    }
                    else {
                        auto *data = static_cast<float *>(frame->frame.auxp);
                        data[index] = amplitude;
                        data[index + 1] = second;
                    }
                }
        }
    }

    void checkMix()
    {
        auto *left = signal("gfLeft");
        auto *right = signal("gfRight");
        auto *mixed = signal("gfMixed");
        ASSERT_NE(mixed, nullptr);
        ASSERT_EQ(mixed->N, left->N);
        ASSERT_EQ(mixed->sliding, left->sliding);
        ASSERT_EQ(mixed->NB, left->NB);
        ASSERT_EQ(mixed->format, left->format);
        const size_t values = (left->N + 2) * (left->sliding ? 16 : 1);
        ASSERT_GE(mixed->frame.size, values *
                  (left->sliding ? sizeof(MYFLT) : sizeof(float)));
        for (size_t index = 0; index < values; ++index) {
            const double a = left->sliding
                ? static_cast<MYFLT *>(left->frame.auxp)[index]
                : static_cast<float *>(left->frame.auxp)[index];
            const double b = right->sliding
                ? static_cast<MYFLT *>(right->frame.auxp)[index]
                : static_cast<float *>(right->frame.auxp)[index];
            const double actual = mixed->sliding
                ? static_cast<MYFLT *>(mixed->frame.auxp)[index]
                : static_cast<float *>(mixed->frame.auxp)[index];
            // Even values are mixed amplitudes; odd values come from the left.
            EXPECT_DOUBLE_EQ(actual, index % 2 ? a : .5 * a + 2 * b) << index;
        }
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
};

TEST_F(PvscrossTests, FreshSlidingOutputAndReinitialization)
{
    ASSERT_NO_FATAL_FAILURE(setInputs(64, 1, PVS_AMP_FREQ));
    csoundEventString(csound, "i 1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkMix());

    // Reuse the same output across modes, sizes and both supported formats.
    struct Configuration { int size, hop, format; };
    for (const auto &config : {Configuration{128, 32, PVS_AMP_FREQ},
                              Configuration{96, 1, PVS_AMP_PHASE},
                              Configuration{64, 32, PVS_AMP_PHASE},
                              Configuration{64, 1, PVS_AMP_FREQ}}) {
        SCOPED_TRACE(std::to_string(config.size) + "/" +
                     std::to_string(config.hop) + "/" + std::to_string(config.format));
        ASSERT_NO_FATAL_FAILURE(setInputs(config.size, config.hop, config.format));
        csoundSetControlChannel(csound, "reset", 1);
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        ASSERT_NO_FATAL_FAILURE(checkMix());
        csoundSetControlChannel(csound, "reset", 0);
    }
}

TEST_F(PvscrossTests, OrdinaryFramesDoNotRequireEqualSourceCounters)
{
    ASSERT_NO_FATAL_FAILURE(setInputs(64, 32, PVS_AMP_FREQ));
    signal("gfLeft")->framecount = 3;
    signal("gfRight")->framecount = 7;
    csoundEventString(csound, "i 1 0 1", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkMix());
    EXPECT_EQ(signal("gfMixed")->framecount, 3u);
}

TEST_F(PvscrossTests, RejectsComplexFrames)
{
    ASSERT_NO_FATAL_FAILURE(setInputs(64, 32, PVS_COMPLEX));
    csoundEventString(csound, "i 1 0 1", 0);
    csoundPerformKsmps(csound);
    EXPECT_NE(messages().find("pvscross: signal format must be amp-phase or amp-freq"),
              std::string::npos);
}

TEST_F(PvscrossTests, RejectsMismatchedAnalysisProperties)
{
    ASSERT_EQ(csoundCompileOrc(csound, "gfRight pvsinit 64, 64, 64, 1\n", 0),
              CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 1", 0);
    csoundPerformKsmps(csound);
    EXPECT_NE(messages().find("pvscross: source and dest signals must have same format"),
              std::string::npos);
}

TEST_F(PvscrossTests, RejectsTrackFrames)
{
    ASSERT_EQ(csoundCompileOrc(csound,
        "gaInput init 0\n"
        "gfFrequency, gfPhase pvsifd gaInput, 128, 32, 1\n"
        "gfLeft partials gfFrequency, gfPhase, .01, 1, 2, 8\n"
        "gfRight partials gfFrequency, gfPhase, .01, 1, 2, 8\n", 0), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 1", 0);
    csoundPerformKsmps(csound);
    EXPECT_NE(messages().find("pvscross: signal format must be amp-phase or amp-freq"),
              std::string::npos);
}

} // namespace
