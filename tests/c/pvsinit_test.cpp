#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace {

class PvsinitTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;

    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
        ASSERT_EQ(csoundSetOption(csound, "--sample-accurate"), CSOUND_SUCCESS);
        ASSERT_EQ(csoundCompileOrc(csound,
            "sr = 8192\nksmps = 16\nnchnls = 1\n0dbfs = 1\n"
            "gfSignal pvsinit 64, 16, 64, 1\n", 0), CSOUND_SUCCESS);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    }

    void TearDown() override { csoundDestroy(csound); }

    PVSDAT *signal()
    {
        auto *variables = csoundDebugGetGlobalVariables(csound);
        PVSDAT *result = nullptr;
        for (auto *variable = variables; variable; variable = variable->next)
            if (std::strcmp(variable->name, "gfSignal") == 0)
                result = static_cast<PVSDAT *>(variable->data);
        csoundDebugFreeVariables(csound, variables);
        return result;
    }

    void checkFrame(int size, bool sliding, int format)
    {
        auto *frame = signal();
        ASSERT_NE(frame, nullptr);
        ASSERT_NE(frame->frame.auxp, nullptr);
        ASSERT_EQ(frame->N, size);
        ASSERT_EQ(frame->NB, size / 2 + 1);
        ASSERT_EQ(frame->sliding, sliding);
        ASSERT_EQ(frame->format, format);
        ASSERT_EQ(frame->framecount, 1u);
        const size_t samples = sliding ? 16 : 1;
        const size_t values = samples * (size + 2);
        ASSERT_GE(frame->frame.size,
                  values * (sliding ? sizeof(MYFLT) : sizeof(float)));
        for (size_t sample = 0; sample < samples; ++sample) {
            for (int bin = 0; bin <= size / 2; ++bin) {
                const size_t index = sample * (size + 2) + 2 * bin;
                const double amplitude = sliding
                    ? static_cast<MYFLT *>(frame->frame.auxp)[index]
                    : static_cast<float *>(frame->frame.auxp)[index];
                const double second = sliding
                    ? static_cast<MYFLT *>(frame->frame.auxp)[index + 1]
                    : static_cast<float *>(frame->frame.auxp)[index + 1];
                EXPECT_EQ(amplitude, 0.0) << sample << ":" << bin;
                EXPECT_EQ(second, format == PVS_AMP_FREQ ? bin * 8192.0 / size : 0.0)
                    << sample << ":" << bin;
            }
        }
    }

    void seedFrame()
    {
        auto *frame = signal();
        ASSERT_NE(frame, nullptr);
        ASSERT_NE(frame->frame.auxp, nullptr);
        // Model a producer leaving nonzero data before the next initialization.
        if (frame->sliding) {
            auto *data = static_cast<MYFLT *>(frame->frame.auxp);
            std::fill(data, data + 16 * (frame->N + 2), FL(0.5));
        }
        else {
            auto *data = static_cast<float *>(frame->frame.auxp);
            std::fill(data, data + frame->N + 2, 0.5f);
        }
    }
};

TEST_F(PvsinitTests, FreshFrame)
{
    checkFrame(64, false, PVS_AMP_FREQ);
}

TEST_F(PvsinitTests, ReusedFramesAcrossSizesModesAndFormats)
{
    struct Configuration { int size, overlap, format; };
    const Configuration configurations[] = {
        {64, 16, PVS_AMP_FREQ}, {64, 1, PVS_AMP_FREQ},
        {128, 1, PVS_AMP_FREQ}, {64, 1, PVS_AMP_FREQ},
        {64, 16, PVS_AMP_FREQ}, {128, 32, PVS_AMP_PHASE},
        {64, 16, PVS_COMPLEX}, {64, 1, PVS_AMP_PHASE},
        {64, 1, PVS_COMPLEX}, {64, 16, PVS_AMP_FREQ}
    };
    for (const auto &configuration : configurations) {
        SCOPED_TRACE(std::to_string(configuration.size) + "/" +
                     std::to_string(configuration.overlap) + "/" +
                     std::to_string(configuration.format));
        ASSERT_NO_FATAL_FAILURE(seedFrame());
        const auto statement = "gfSignal pvsinit " + std::to_string(configuration.size) +
            ", " + std::to_string(configuration.overlap) + ", 0, 1, " +
            std::to_string(configuration.format) + "\n";
        ASSERT_EQ(csoundCompileOrc(csound, statement.c_str(), 0), CSOUND_SUCCESS);
        ASSERT_NO_FATAL_FAILURE(checkFrame(configuration.size,
            configuration.overlap == 1, configuration.format));
    }
}

TEST_F(PvsinitTests, SlidingSampleOffset)
{
    ASSERT_EQ(csoundCompileOrc(csound,
        "instr 1\ngfSignal pvsinit 64, 1, 64, 1\nendin\n", 0), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0.0006103515625 0.125", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    auto *frame = signal();
    ASSERT_NE(frame, nullptr);
    ASSERT_TRUE(frame->sliding);
    ASSERT_GE(frame->frame.size, 16 * 66 * sizeof(MYFLT));
    auto *data = static_cast<MYFLT *>(frame->frame.auxp);
    for (int sample = 0; sample < 16; ++sample)
        for (int bin = 0; bin < 33; ++bin) {
            const int index = sample * 66 + 2 * bin;
            EXPECT_EQ(data[index], FL(0.0));
            EXPECT_EQ(data[index + 1],
                      sample >= 5 ? bin * FL(128.0) : FL(0.0))
                << sample << ":" << bin;
        }
}

} // namespace
