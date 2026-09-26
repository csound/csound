#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"
#include <sndfile.h>

#include <chrono>
#include <cmath>
#include <complex>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

constexpr int fftSize = 64;
constexpr int hop = 16;
constexpr int sampleRate = 8192;
constexpr double twoPi = 6.2831853071795864769;

class PvsTanalTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    std::filesystem::path directory;
    std::vector<float> samples;
    int channels = 1;

    void SetUp() override
    {
        const auto id = std::chrono::steady_clock::now().time_since_epoch().count();
        directory = std::filesystem::temp_directory_path() /
                    ("csound-pvstanal-" + std::to_string(id));
        ASSERT_TRUE(std::filesystem::create_directory(directory));
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), CSOUND_SUCCESS);
    }

    void TearDown() override
    {
        if (csound) csoundDestroy(csound);
        std::error_code error;
        std::filesystem::remove_all(directory, error);
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

    void start(const std::string &sizes = "64, 16")
    {
        SF_INFO info = {};
        info.samplerate = sampleRate;
        info.channels = channels;
        info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
        const auto path = (directory / "source.wav").generic_string();
        SNDFILE *file = sf_open(path.c_str(), SFM_WRITE, &info);
        ASSERT_NE(file, nullptr);
        const auto frames = samples.size() / channels;
        EXPECT_EQ(sf_writef_float(file, samples.data(), frames), frames);
        ASSERT_EQ(sf_close(file), 0);

        const std::string outputs = channels == 1 ? "gfLeft" : "gfLeft, gfRight";
        const auto orchestra =
            "sr=8192\nksmps=16\nnchnls=1\n0dbfs=1\n"
            "giSource ftgen 1, 0, 0, -1, \"" + path + "\", 0, 0, 0\n" +
            "instr 1\n" + outputs + " pvstanal 0, 1, 1, giSource, 0, 1, " +
            "0.125, " + sizes + "\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS)
            << messages();
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS) << messages();
        csoundEventString(csound, "i 1 0 1", 0);
    }

    PVSDAT *output(const char *name = "gfLeft")
    {
        auto *variables = csoundDebugGetGlobalVariables(csound);
        PVSDAT *result = nullptr;
        for (auto *variable = variables; variable; variable = variable->next)
            if (std::strcmp(variable->name, name) == 0)
                result = static_cast<PVSDAT *>(variable->data);
        csoundDebugFreeVariables(csound, variables);
        return result;
    }

    // Independent reference: apply Hann to the source, then use a DFT.
    std::complex<double> referenceBin(int bin, int channel, int firstSample)
    {
        std::complex<double> result = 0;
        for (int n = 0; n < fftSize; ++n) {
            const double value = samples[(firstSample + n) * channels + channel];
            const double window = (1 - std::cos(twoPi * n / fftSize)) * 2 / fftSize;
            result += value * window * std::polar(1.0, -twoPi * bin * n / fftSize);
        }
        return result;
    }

    void checkSpectrum(int channel = 0)
    {
        auto *frame = output(channel == 0 ? "gfLeft" : "gfRight");
        ASSERT_NE(frame, nullptr);
        ASSERT_FALSE(frame->sliding);
        ASSERT_EQ(frame->format, PVS_AMP_FREQ);
        ASSERT_EQ(frame->N, fftSize);
        const auto *data = static_cast<const float *>(frame->frame.auxp);
        for (int bin = 0; bin <= fftSize / 2; ++bin) {
            const auto front = referenceBin(bin, channel, 1024);
            EXPECT_NEAR(data[2 * bin], std::abs(front), 2e-6) << "bin " << bin;
            if (bin == 0 || bin == fftSize / 2) {
                EXPECT_EQ(data[2 * bin + 1], bin * sampleRate / fftSize);
            } else if (std::abs(front) > .001) {
                const auto back = referenceBin(bin, channel, 1024 - hop);
                const double difference = std::remainder(
                    std::arg(front) - std::arg(back) - twoPi * hop * bin / fftSize,
                    twoPi);
                const double frequency = difference * sampleRate / (twoPi * hop) +
                                         bin * sampleRate / fftSize;
                EXPECT_NEAR(data[2 * bin + 1], frequency, .01) << "bin " << bin;
            }
        }
    }

    void makeTone()
    {
        samples.resize(4096 * channels);
        for (int n = 0; n < 4096; ++n)
            for (int c = 0; c < channels; ++c)
                samples[n * channels + c] = (c + 1) *
                    (.125 + .125 * (n % 2 ? -1 : 1) +
                     .125 * std::cos(twoPi * 8 * n / fftSize));
    }
};

TEST_F(PvsTanalTests, RetainsDcAndNyquistAlongsideInteriorBins)
{
    makeTone();
    ASSERT_NO_FATAL_FAILURE(start());
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkSpectrum());
}

TEST_F(PvsTanalTests, StereoRetainsDcAndNyquistForEachChannel)
{
    channels = 2;
    // The periodic source keeps this check independent of channel read timing.
    makeTone();
    ASSERT_NO_FATAL_FAILURE(start());
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(0));
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(1));
}

class PvsTanalInvalidTests : public PvsTanalTests,
                            public ::testing::WithParamInterface<const char *> {};

TEST_P(PvsTanalInvalidTests, RejectsInvalidFrameSizesAtInitialization)
{
    makeTone();
    ASSERT_NO_FATAL_FAILURE(start(GetParam()));
    csoundPerformKsmps(csound);
    EXPECT_NE(messages().find("pvstanal:"), std::string::npos);
    EXPECT_GT(csoundErrCnt(csound), 0);
}

INSTANTIATE_TEST_SUITE_P(FrameSizes, PvsTanalInvalidTests,
    ::testing::Values("1, 16", "63, 16", "64.5, 16", "1e30, 16", "64, 8",
                      "64, 1e30"));

} // namespace
