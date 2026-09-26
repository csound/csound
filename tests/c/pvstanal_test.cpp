#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"
#include <sndfile.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace {

constexpr int fftSize = 64;
constexpr int hop = 16;
constexpr int sampleRate = 8192;
constexpr double twoPi = 6.2831853071795864769;

std::string number(double value)
{
    std::ostringstream text;
    text << std::setprecision(17) << value;
    return text.str();
}

struct Analysis {
    double position = 1024;
    double pitch = 1;
    double gain = 1;
    double speed = 0;
    int detect = 0;
    double threshold = 1;
    int fileRate = sampleRate;
    int wrap = 1;
    std::string sizes = "64, 16";
    std::string initialOutput;
};

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

    void start(const Analysis &settings = {})
    {
        SF_INFO info = {};
        info.samplerate = settings.fileRate;
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
            settings.initialOutput + "instr 1\n" + outputs + " pvstanal " +
            number(settings.speed) + ", " + number(settings.gain) + ", " +
            number(settings.pitch) + ", giSource, " + number(settings.detect) +
            ", " + number(settings.wrap) + ", " +
            number(settings.position / settings.fileRate) + ", " + settings.sizes +
            ", " + number(settings.threshold) + "\nendin\n";
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

    // Independent reference: interpolate the source, apply Hann, then use a DFT.
    std::complex<double> referenceBin(int bin, double position,
                                      double pitch = 1, int channel = 0)
    {
        const int frames = samples.size() / channels;
        std::complex<double> result = 0;
        for (int n = 0; n < fftSize; ++n) {
            double read = std::fmod(position + n * pitch, frames);
            if (read < 0) read += frames;
            const int index = static_cast<int>(read);
            const double fraction = read - index;
            const double first = samples[index * channels + channel];
            const double next = samples[((index + 1) % frames) * channels + channel];
            const double value = first + fraction * (next - first);
            const double window = (1 - std::cos(twoPi * n / fftSize)) * 2 / fftSize;
            result += value * window * std::polar(1.0, -twoPi * bin * n / fftSize);
        }
        return result;
    }

    void checkSpectrum(double position, double pitch = 1, int channel = 0,
                       double gain = 1)
    {
        auto *frame = output(channel == 0 ? "gfLeft" : "gfRight");
        ASSERT_NE(frame, nullptr);
        ASSERT_FALSE(frame->sliding);
        ASSERT_EQ(frame->format, PVS_AMP_FREQ);
        ASSERT_EQ(frame->N, fftSize);
        const auto *data = static_cast<const float *>(frame->frame.auxp);
        for (int bin = 0; bin <= fftSize / 2; ++bin) {
            const auto front = referenceBin(bin, position, pitch, channel);
            EXPECT_NEAR(data[2 * bin], std::abs(gain * front), 2e-6) << "bin " << bin;
            if (bin == 0 || bin == fftSize / 2) {
                EXPECT_EQ(data[2 * bin + 1], bin * sampleRate / fftSize);
            } else if (std::abs(front) > .001) {
                const auto back = referenceBin(bin, position - hop * pitch,
                                               pitch, channel);
                const double difference = std::remainder(
                    std::arg(front) - std::arg(back) - twoPi * hop * bin / fftSize,
                    twoPi);
                const double frequency = difference * sampleRate / (twoPi * hop) +
                                         bin * sampleRate / fftSize;
                EXPECT_NEAR(data[2 * bin + 1], frequency, .01) << "bin " << bin;
            }
        }
    }

    void makeTone(int count = 4096)
    {
        samples.resize(count * channels);
        for (int n = 0; n < count; ++n)
            for (int c = 0; c < channels; ++c)
                samples[n * channels + c] = (.1 + n / 32768.0) *
                                             std::cos(twoPi * 8 * n / fftSize);
    }
};

TEST_F(PvsTanalTests, RetainsDcAndNyquistAlongsideInteriorBins)
{
    samples.resize(4096);
    for (int n = 0; n < 4096; ++n)
        samples[n] = .125 + .125 * (n % 2 ? -1 : 1) +
                     .125 * std::cos(twoPi * 8 * n / fftSize);
    ASSERT_NO_FATAL_FAILURE(start());
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(1024));
}

TEST_F(PvsTanalTests, IdenticalStereoChannelsAnalyzeTheSameTime)
{
    channels = 2;
    makeTone();
    ASSERT_NO_FATAL_FAILURE(start());
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(1024, 1, 0));
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(1024, 1, 1));
}

TEST_F(PvsTanalTests, FractionalPitchInterpolatesEachWindowAtItsOwnPosition)
{
    makeTone();
    // The back window starts half a sample away from the front window's grid.
    Analysis settings;
    settings.position = 1024.25;
    settings.pitch = 1.03125;
    ASSERT_NO_FATAL_FAILURE(start(settings));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(1024.25, 1.03125));
}

TEST_F(PvsTanalTests, NegativeGainDoesNotChangeFrequencies)
{
    makeTone();
    Analysis settings;
    settings.gain = -1;
    ASSERT_NO_FATAL_FAILURE(start(settings));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(1024, 1, 0, -1));
}

TEST_F(PvsTanalTests, StereoInterpolationWrapsWithinEachChannel)
{
    channels = 2;
    makeTone();
    // Give the right channel a distinct amplitude to expose channel mixing.
    for (size_t n = 1; n < samples.size(); n += 2) samples[n] *= .5f;
    Analysis settings;
    settings.position = 4095.5;
    settings.pitch = 1.03125;
    ASSERT_NO_FATAL_FAILURE(start(settings));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(4095.5, 1.03125, 0));
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(4095.5, 1.03125, 1));
}

TEST_F(PvsTanalTests, ReversePitchWrapsAtTheStartOfTheTable)
{
    makeTone();
    Analysis settings;
    settings.position = .25;
    settings.pitch = -1.03125;
    ASSERT_NO_FATAL_FAILURE(start(settings));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(.25, -1.03125));
}

TEST_F(PvsTanalTests, OffsetSecondsUseTheSourceSampleRate)
{
    makeTone();
    // A source recorded at twice sr needs twice as many samples per second.
    Analysis settings;
    settings.fileRate = 2 * sampleRate;
    ASSERT_NO_FATAL_FAILURE(start(settings));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(1024, 2));
}

TEST_F(PvsTanalTests, UnwrappedReversePlaybackStopsAtTheStart)
{
    makeTone();
    Analysis settings;
    settings.position = 0;
    settings.speed = -1;
    settings.wrap = 0;
    ASSERT_NO_FATAL_FAILURE(start(settings));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(0));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    auto *frame = output();
    ASSERT_NE(frame, nullptr);
    const auto *data = static_cast<float *>(frame->frame.auxp);
    for (int i = 0; i < fftSize + 2; ++i) EXPECT_EQ(data[i], 0);
}

TEST_F(PvsTanalTests, ReusedSlidingOutputHasOrdinaryFrameMetadata)
{
    makeTone();
    Analysis settings;
    settings.initialOutput = "gfLeft pvsinit 64, 1, 64, 1\n";
    ASSERT_NO_FATAL_FAILURE(start(settings));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    auto *frame = output();
    ASSERT_NE(frame, nullptr);
    EXPECT_FALSE(frame->sliding);
    EXPECT_EQ(frame->NB, 33);
    EXPECT_EQ(frame->format, PVS_AMP_FREQ);
    ASSERT_NO_FATAL_FAILURE(checkSpectrum(1024));
}

class PvsTanalOnsetTests : public PvsTanalTests,
    public ::testing::WithParamInterface<std::tuple<double, int, int>> {};

TEST_P(PvsTanalOnsetTests, PowerThresholdAndChannelTimingAgree)
{
    const auto [threshold, fileRate, channelCount] = GetParam();
    const double rate = double(fileRate) / sampleRate;
    channels = channelCount;
    samples.resize(4096 * channels);
    for (int n = 0; n < 4096; ++n) {
        const int ramp = std::max(0, std::min(n - 1024, 512));
        // Each hop raises the left channel's power by 3 dB. The right is steady.
        const double amplitude = .02 * std::pow(10.0, 3.0 * ramp / (20 * hop * rate));
        // Keep the carrier away from exact phase-unwrapping boundaries.
        const double tone = std::cos(twoPi * 8.25 * n / fftSize);
        samples[n * channels] = amplitude * tone;
        if (channels == 2) samples[n * channels + 1] = .02 * tone;
    }
    Analysis settings;
    settings.speed = .5;
    settings.detect = 1;
    settings.threshold = threshold;
    settings.fileRate = fileRate;
    ASSERT_NO_FATAL_FAILURE(start(settings));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    // 2 dB detects the onset; 4 dB keeps the half-speed step. Both channels agree.
    const double nextPosition = 1024 + (threshold < 3 ? hop : hop / 2) * rate;
    for (int channel = 0; channel < channels; ++channel)
        ASSERT_NO_FATAL_FAILURE(checkSpectrum(nextPosition, rate, channel));
}

INSTANTIATE_TEST_SUITE_P(ThresholdsRatesAndChannels, PvsTanalOnsetTests,
    ::testing::Combine(::testing::Values(2.0, 4.0),
                       ::testing::Values(sampleRate, 2 * sampleRate),
                       ::testing::Values(1, 2)));

class PvsTanalInvalidTests : public PvsTanalTests,
                            public ::testing::WithParamInterface<const char *> {};

TEST_P(PvsTanalInvalidTests, RejectsInvalidFrameSizesAtInitialization)
{
    makeTone();
    Analysis settings;
    settings.sizes = GetParam();
    ASSERT_NO_FATAL_FAILURE(start(settings));
    csoundPerformKsmps(csound);
    EXPECT_NE(messages().find("pvstanal:"), std::string::npos);
    EXPECT_GT(csoundErrCnt(csound), 0);
}

INSTANTIATE_TEST_SUITE_P(FrameSizes, PvsTanalInvalidTests,
    ::testing::Values("1, 16", "63, 16", "64.5, 16", "1e30, 16", "64, 8",
                      "64, 1e30"));

} // namespace
