#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <sndfile.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <string>
#include <tuple>
#include <vector>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {
class MemplayTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    std::filesystem::path directory, file;
    std::atomic<int> ioCalls{0}, opens{0};
    static std::atomic<int> threadCalls;
    static void *trackedThread(uintptr_t (*routine)(void *), void *data) {
        ++threadCalls;
        return csoundCreateThread(routine, data);
    }
    decltype(CSOUND::FileOpen) openFile;
    decltype(CSOUND::FileClose) closeFile;
    decltype(CSOUND::SndfileRead) readFrames;
    decltype(CSOUND::SndfileReadSamples) readSamples;
    decltype(CSOUND::SndfileSeek) seek;
    static MemplayTests &owner(CSOUND *cs) {
        return *static_cast<MemplayTests *>(csoundGetHostData(cs));
    }
    static void *trackedOpen(CSOUND *cs, void *handle, int32_t type,
                             const char *name, void *info, const char *env,
                             int32_t fileType, int32_t temporary) {
        auto &p = owner(cs);
        ++p.ioCalls; ++p.opens;
        return p.openFile(cs, handle, type, name, info, env, fileType, temporary);
    }
    static int32_t trackedClose(CSOUND *cs, void *handle, uint32_t flags) {
        auto &p = owner(cs); ++p.ioCalls;
        return p.closeFile(cs, handle, flags);
    }
    static int64_t trackedRead(CSOUND *cs, void *handle, cs_float *out, int64_t n) {
        auto &p = owner(cs); ++p.ioCalls;
        return p.readFrames(cs, handle, out, n);
    }
    static int64_t trackedSamples(CSOUND *cs, void *handle, cs_float *out, int64_t n) {
        auto &p = owner(cs); ++p.ioCalls;
        return p.readSamples(cs, handle, out, n);
    }
    static int64_t trackedSeek(CSOUND *cs, void *handle, int64_t pos, int32_t whence) {
        auto &p = owner(cs); ++p.ioCalls;
        return p.seek(cs, handle, pos, whence);
    }
    void SetUp() override {
#if defined(_WIN32)
        const auto pid = _getpid();
#else
        const auto pid = getpid();
#endif
        auto root = std::filesystem::temp_directory_path();
        for (int n = 0; n < 100; ++n) {
            auto candidate = root / ("csound-memplay-" + std::to_string(pid) + "-" +
                              std::to_string(reinterpret_cast<uintptr_t>(this)) +
                              "-" + std::to_string(n));
            if (std::filesystem::create_directory(candidate)) {
                directory = candidate; break;
            }
        }
        ASSERT_FALSE(directory.empty());
        file = directory / "input.wav";
        csound = csoundCreate(this, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n -d -m0");
        openFile = csound->FileOpen; csound->FileOpen = trackedOpen;
        closeFile = csound->FileClose; csound->FileClose = trackedClose;
        readFrames = csound->SndfileRead; csound->SndfileRead = trackedRead;
        readSamples = csound->SndfileReadSamples; csound->SndfileReadSamples = trackedSamples;
        seek = csound->SndfileSeek; csound->SndfileSeek = trackedSeek;
        threadCalls = 0;
        csound->CreateThread = trackedThread;
    }
    void TearDown() override {
        if (csound) csoundDestroy(csound);
        if (!directory.empty()) {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }
    }
    void writeFile(int channels, int format = SF_FORMAT_FLOAT, int frames = 521) {
        SF_INFO info = {};
        info.samplerate = 12000;
        info.channels = channels;
        info.format = SF_FORMAT_WAV | format;
        SNDFILE *sf = sf_open(file.string().c_str(), SFM_WRITE, &info);
        ASSERT_NE(sf, nullptr);
        std::vector<double> samples(frames * channels);
        for (int n = 0; n < frames; ++n)
            for (int c = 0; c < channels; ++c)
                samples[n * channels + c] = .1 * (c + 1) + .15 * std::sin(n * .137);
        EXPECT_EQ(sf_writef_double(sf, samples.data(), frames), frames);
        EXPECT_EQ(sf_close(sf), 0);
    }
    std::string messages() {
        std::string text;
        while (csoundGetMessageCnt(csound)) {
            text += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return text;
    }
    void start(const std::string &orc, const std::string &score = "i1 0 .24") {
        const std::string csd = "<CsoundSynthesizer>\n<CsInstruments>\n"
            "sr=8000\nksmps=32\nnchnls=8\n0dbfs=2\n" + orc +
            "\n</CsInstruments>\n<CsScore>\n" + score +
            "\ne\n</CsScore>\n</CsoundSynthesizer>";
        ASSERT_EQ(csoundCompileCSD(csound, csd.c_str(), 1, 0), 0) << messages();
        ASSERT_EQ(csoundStart(csound), 0) << messages();
    }
};

std::atomic<int> MemplayTests::threadCalls{0};

using Playback = std::tuple<int, int, int, bool>;
class MemplayPlaybackTests : public MemplayTests,
                           public ::testing::WithParamInterface<Playback> {};

TEST_P(MemplayPlaybackTests, MatchesDiskin2) {
    const auto [window, wrap, channels, array] = GetParam();
    writeFile(channels, window == 2 ? SF_FORMAT_PCM_16 : SF_FORMAT_FLOAT);
    std::string orc = "instr 1\n kSpeed linseg 1, .04, 1, .001, -.8, .04, -.8, .08, 1.3\n";
    for (const std::string prefix : {"M", "D"}) {
        std::string outputs;
        for (int c = 0; c < channels; ++c) {
            if (c) outputs += ", ";
            outputs += "a" + prefix + std::to_string(c);
        }
        if (array) outputs = "a" + prefix + "[]";
        orc += outputs + (prefix == "M" ? " memplay \"" : " diskin2 \"") +
            file.generic_string() + "\", kSpeed, .004, " + std::to_string(wrap) +
            ", 0, " + std::to_string(window) + ", 128, 0, 1, .028\n";
    }
    for (int c = 0; c < channels; ++c) {
        auto suffix = array ? "[" + std::to_string(c) + "]" : std::to_string(c);
        orc += "outch " + std::to_string(c + 1) + ", aM" + suffix + ", " +
            std::to_string(c + 5) + ", aD" + suffix + "\n";
    }
    orc += "endin\n";
    // Sample-accurate starts and ends also exercise partially active blocks.
    csoundSetOption(csound, "--sample-accurate");
    start(orc, "i1 .000625 .22925");
    cs_double peak = 0;
    int blocks = 0;
    while (csoundPerformKsmps(csound) == 0 && blocks++ < 100) {
        const cs_float *out = csoundGetSpout(csound);
        for (int n = 0; n < 32; ++n)
            for (int c = 0; c < channels; ++c) {
                ASSERT_NEAR(out[n * 8 + c], out[n * 8 + c + 4], 1e-6)
                    << "block " << blocks << " sample " << n << " channel " << c;
                peak = (std::max)(peak, std::abs(cs_double(out[n * 8 + c + 4])));
            }
    }
    EXPECT_LT(blocks, 100);
    EXPECT_GT(peak, .1);
    EXPECT_EQ(csound->inerrcnt, 0) << messages();
    EXPECT_EQ(csound->perferrcnt, 0) << messages();
}

INSTANTIATE_TEST_SUITE_P(Modes, MemplayPlaybackTests,
    ::testing::Combine(::testing::Values(1, 2, 4, 16), ::testing::Values(0, 1, 16),
                       ::testing::Values(1, 2, 4), ::testing::Bool()));

TEST_F(MemplayTests, CacheStoresAllChannelsAndReusesTheFile) {
    writeFile(4);
    const std::string name = file.generic_string();
    auto *first = csound->LoadSoundFile(csound, name.c_str(), nullptr);
    ASSERT_NE(first, nullptr);
    ASSERT_EQ(first->nFrames, 521u);
    ASSERT_EQ(first->nChannels, 4);
    for (int n : {0, 130, 520})
        for (int c = 0; c < 4; ++c)
            EXPECT_NEAR(first->data[n * 4 + c], .1 * (c + 1) + .15 * std::sin(n * .137), 1e-6);
    EXPECT_EQ(first->data[521 * 4], 0);
    const int before = ioCalls;
    std::filesystem::remove(file);
    EXPECT_EQ(csound->LoadSoundFile(csound, name.c_str(), nullptr), first);
    EXPECT_EQ(ioCalls.load(), before);
}

TEST_F(MemplayTests, RealtimePlaybackAndReinitUseOnlyCachedMemory) {
    writeFile(2);
    csoundSetOption(csound, "--realtime");
    const std::string name = file.generic_string();
    // Preload once. Neither new readers nor reinit may reopen this file.
    ASSERT_NE(csound->LoadSoundFile(csound, name.c_str(), nullptr), nullptr);
    std::filesystem::remove(file);
    start("instr 1\n kCycle timeinstk\n if kCycle == 5 then\n reinit AGAIN\n endif\n"
          "AGAIN:\n aL, aR memplay \"" + name + "\", 1, .004, 16, 0, 4, 128, 0, 0, .028\n"
          "aBoth[] memplay \"" + name + "\", -1, .004, 16, 0, 4, 128, 0, 0, .028\n"
          "outch 1, aL, 2, aR, 3, aBoth[0], 4, aBoth[1]\n rireturn\n endin",
          "i1 0 .1\ni1 .12 .1");
    const int before = ioCalls;
    const int threadsBefore = threadCalls;
    cs_double peak = 0;
    for (int n = 0; n < 1000 && csoundPerformKsmps(csound) == 0; ++n) {
        const cs_float *out = csoundGetSpout(csound);
        for (int i = 0; i < 32 * 8; ++i) peak = (std::max)(peak, std::abs(cs_double(out[i])));
        csoundSleep(1); // Allow the realtime event thread to finish initialization.
    }
    EXPECT_GT(peak, .1);
    EXPECT_EQ(ioCalls.load(), before);
    EXPECT_EQ(threadCalls.load(), threadsBefore);
    EXPECT_EQ(csound->inerrcnt, 0) << messages();
    EXPECT_EQ(csound->perferrcnt, 0) << messages();
}

TEST_F(MemplayTests, ReinitCanPreserveTheReadPositionAndNumericNamesWork) {
    writeFile(1);
    start("strset 17, \"" + file.generic_string() + "\"\n"
          "instr 1\n kCycle timeinstk\n if kCycle == 5 then\n reinit AGAIN\n endif\n"
          "AGAIN:\n aM memplay 17, 1, 0, 1, 0, 4, 128, 1\n"
          "aD diskin2 17, 1, 0, 1, 0, 4, 128, 1, 1\n"
          "aA[] memplay 17, 1, 0, 1, 0, 4, 128, 1\n"
          "outch 1, aM, 2, aD, 3, aA[0]\n rireturn\n endin");
    cs_double peak = 0;
    for (int n = 0; n < 100 && csoundPerformKsmps(csound) == 0; ++n) {
        const cs_float *out = csoundGetSpout(csound);
        for (int i = 0; i < 32; ++i) {
            EXPECT_NEAR(out[i * 8], out[i * 8 + 1], 1e-6);
            EXPECT_NEAR(out[i * 8], out[i * 8 + 2], 1e-6);
            peak = (std::max)(peak, std::abs(cs_double(out[i * 8])));
        }
    }
    EXPECT_GT(peak, .1);
    EXPECT_EQ(csound->inerrcnt, 0) << messages();
    EXPECT_EQ(csound->perferrcnt, 0) << messages();
}

TEST_F(MemplayTests, ExtraScalarOutputsAreSilent) {
    writeFile(1);
    start("instr 1\n aL, aR memplay \"" + file.generic_string() +
          "\", 1, 0, 1\n outch 1, aL, 2, aR\n endin");
    cs_double peak = 0;
    for (int n = 0; n < 100 && csoundPerformKsmps(csound) == 0; ++n) {
        const cs_float *out = csoundGetSpout(csound);
        for (int i = 0; i < 32; ++i) {
            EXPECT_EQ(out[i * 8 + 1], 0);
            peak = (std::max)(peak, std::abs(cs_double(out[i * 8])));
        }
    }
    EXPECT_GT(peak, .1);
}

TEST_F(MemplayTests, MissingFileReportsAnInitError) {
    start("instr 1\n aL memplay \"" + file.generic_string() +
          "\"\n outch 1, aL\n endin");
    for (int n = 0; n < 100 && csoundPerformKsmps(csound) == 0; ++n) {}
    EXPECT_GT(csound->inerrcnt, 0);
    EXPECT_NE(messages().find("memplay: could not load"), std::string::npos);
}

}
