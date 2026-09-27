#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>

namespace {

class PvsanalSampleRangeTests : public ::testing::TestWithParam<
    std::tuple<int, int, int>> {
protected:
    CSOUND *blocked = nullptr;
    CSOUND *reference = nullptr;

    void Create(CSOUND **engine, int blockSize, int window)
    {
        *engine = csoundCreate(nullptr, nullptr);
        ASSERT_NE(*engine, nullptr);
        csoundCreateMessageBuffer(*engine, 0);
        ASSERT_EQ(csoundSetOption(*engine, "-n -d -m0 --sample-accurate"), 0);
        const std::string orchestra =
            "sr = 8192\nksmps = " + std::to_string(blockSize) + R"(
            nchnls = 1
            0dbfs = 1
            gfAnalysis pvsinit 64, 1, 64, 1
            instr 1
                aTone oscili .5, 1024
                aInput = aTone + .125
                gfAnalysis pvsanal aInput, 64, 1, 64, )" +
                std::to_string(window) + "\nendin\n";
        ASSERT_EQ(csoundCompileOrc(*engine, orchestra.c_str(), 0), 0);
        ASSERT_EQ(csoundStart(*engine), 0);
    }

    void TearDown() override
    {
        if (blocked) csoundDestroy(blocked);
        if (reference) csoundDestroy(reference);
    }

    PVSDAT *Analysis(CSOUND *engine)
    {
        auto *variables = csoundDebugGetGlobalVariables(engine);
        PVSDAT *result = nullptr;
        for (auto *variable = variables; variable; variable = variable->next)
            if (std::strcmp(variable->name, "gfAnalysis") == 0)
                result = static_cast<PVSDAT *>(variable->data);
        csoundDebugFreeVariables(engine, variables);
        return result;
    }
};

TEST_P(PvsanalSampleRangeTests, CopiesActiveSpectraAndClearsEveryInactiveBin)
{
    const auto [start, end, window] = GetParam();
    ASSERT_NO_FATAL_FAILURE(Create(&blocked, 16, window));
    ASSERT_NO_FATAL_FAILURE(Create(&reference, 1, window));
    std::ostringstream score;
    score << std::setprecision(17) << "i1 " << start / 8192.0 << " "
          << (end - start) / 8192.0;
    csoundEventString(blocked, score.str().c_str(), 0);
    csoundEventString(reference, score.str().c_str(), 0);

    // The reference has no partial blocks. Compare all active bins against
    // it, then check that the blocked output contains no stale tail spectra.
    for (int blockStart = 0; blockStart < end; blockStart += 16) {
        ASSERT_EQ(csoundPerformKsmps(blocked), 0);
        auto *output = Analysis(blocked);
        ASSERT_NE(output, nullptr);
        ASSERT_TRUE(output->sliding);
        ASSERT_EQ(output->NB, 33);
        ASSERT_GE(output->frame.size, 16 * 33 * sizeof(CMPLX));
        const auto *frames = static_cast<CMPLX *>(output->frame.auxp);
        for (int sample = 0; sample < 16; ++sample) {
            ASSERT_EQ(csoundPerformKsmps(reference), 0);
            const int absolute = blockStart + sample;
            const bool active = absolute >= start && absolute < end;
            const auto *expected = static_cast<CMPLX *>(Analysis(reference)->frame.auxp);
            for (int bin = 0; bin < 33; ++bin) {
                SCOPED_TRACE(::testing::Message() << "sample " << absolute
                                                 << ", bin " << bin);
                const auto &actual = frames[sample * 33 + bin];
                EXPECT_EQ(actual.re, active ? expected[bin].re : 0);
                EXPECT_EQ(actual.im, active ? expected[bin].im : 0);
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(NoteBoundaries, PvsanalSampleRangeTests,
    ::testing::Combine(
        ::testing::Values(0, 5),
        ::testing::Values(12, 45, 48),
        ::testing::Values(0, 1, 6, 9)));

struct InvalidAnalysis {
    const char *arguments;
    const char *error;
};

class PvsanalSetupTests : public ::testing::TestWithParam<InvalidAnalysis> {
protected:
    CSOUND *csound = nullptr;

    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), 0);
    }

    void TearDown() override { csoundDestroy(csound); }
};

TEST_P(PvsanalSetupTests, RejectsInvalidSettingsBeforeConvertingOrAllocating)
{
    const auto config = GetParam();
    SCOPED_TRACE(config.arguments);
    const std::string orchestra = R"(
        sr = 8192
        ksmps = 16
        nchnls = 1
        0dbfs = 1
        instr 1
            aInput init .5
            fAnalysis pvsanal aInput, )" + std::string(config.arguments) +
        "\nendin\n";
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    csoundEventString(csound, "i1 0 1", 0);
    csoundPerformKsmps(csound);
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(messages.find(config.error), std::string::npos) << messages;
}

INSTANTIATE_TEST_SUITE_P(InvalidSettings, PvsanalSetupTests, ::testing::Values(
    InvalidAnalysis{"64, -1, 64, 1", "pvsanal: invalid hop size"},
    InvalidAnalysis{"64, 1e30, 64, 1", "pvsanal: invalid hop size"},
    InvalidAnalysis{"64, sqrt(-1), 64, 1", "pvsanal: invalid hop size"},
    InvalidAnalysis{"-1, 16, 64, 1", "pvsanal: invalid FFT size"},
    InvalidAnalysis{"1e30, 16, 64, 1", "pvsanal: invalid FFT size"},
    InvalidAnalysis{"sqrt(-1), 16, 64, 1", "pvsanal: invalid FFT size"},
    InvalidAnalysis{"64, 16, -1, 1", "pvsanal: invalid window size"},
    InvalidAnalysis{"64, 16, exp(1000), 1", "pvsanal: invalid window size"},
    InvalidAnalysis{"64, 16, 1000000000, 1", "pvsanal: window size too large"},
    // An FFT larger than the requested window also expands the input buffer.
    InvalidAnalysis{"1000000000, 16, 64, 1", "pvsanal: window size too large"},
    InvalidAnalysis{"64, 16, 64, -2147483648", "pvsanal: invalid window type"},
    InvalidAnalysis{"64, 1, 64, 1e30", "pvsanal: invalid window type"},
    InvalidAnalysis{"64, 1, 64, sqrt(-1)", "pvsanal: invalid window type"}));

} // namespace
