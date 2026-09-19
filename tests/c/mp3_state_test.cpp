#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "mp3dec.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

namespace {
class MP3StateTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        csound = csoundCreate(this, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n -d -m0");
        openFile = csound->FileOpen;
        closeFile = csound->FileClose;
        csound->FileOpen = trackedOpen;
        csound->FileClose = trackedClose;
        path = __FILE__;
        std::replace(path.begin(), path.end(), '\\', '/');
        path = path.substr(0, path.find_last_of('/') + 1) +
               "../commandline/beats.mp3";
    }

    void TearDown() override { csoundDestroy(csound); }

    static void* trackedOpen(CSOUND* cs, void* file, int32_t type,
                            const char* name, void* mode, const char* env,
                            int32_t format, int32_t temporary)
    {
        auto* self = static_cast<MP3StateTests*>(csoundGetHostData(cs));
        void* fd = self->openFile(cs, file, type, name, mode, env, format, temporary);
        if (fd) {
            self->files.insert(fd);
            ++self->opens;
        }
        return fd;
    }

    static int32_t trackedClose(CSOUND* cs, void* fd, uint32_t flags)
    {
        auto* self = static_cast<MP3StateTests*>(csoundGetHostData(cs));
        EXPECT_EQ(1u, self->files.erase(fd));
        return self->closeFile(cs, fd, flags);
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

    void reset()
    {
        csoundReset(csound);
        EXPECT_TRUE(files.empty());
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n -d -m0");
    }

    void start(const std::string& body, const std::string& score)
    {
        std::string csd = "<CsoundSynthesizer>\n<CsInstruments>\n"
          "sr=44100\nksmps=32\nnchnls=2\n0dbfs=1\ninstr 1\n" + body +
          "\nendin\n</CsInstruments>\n<CsScore>\n" + score +
          "\n</CsScore>\n</CsoundSynthesizer>";
        ASSERT_EQ(CSOUND_SUCCESS, csoundCompileCSD(csound, csd.c_str(), 1, 0))
          << messages();
        ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound)) << messages();
    }

    CSOUND* csound = nullptr;
    decltype(CSOUND::FileOpen) openFile;
    decltype(CSOUND::FileClose) closeFile;
    std::unordered_set<void*> files;
    int opens = 0;
    std::string path;
};

TEST_F(MP3StateTests, DecoderClosesFileBeforeStreamInitialization)
{
    for (bool reset : {false, true}) {
        mp3dec_t decoder = mp3dec_init(csound);
        ASSERT_NE(nullptr, decoder);
        FILE* file = nullptr;
        ASSERT_NE(nullptr, mp3dec_open_file(decoder, &path[0], &file));
        ASSERT_EQ(1u, files.size());
        if (reset) {
            ASSERT_EQ(MP3DEC_RETCODE_OK, mp3dec_reset(decoder));
            EXPECT_TRUE(files.empty());
        }
        EXPECT_EQ(MP3DEC_RETCODE_OK, mp3dec_uninit(decoder));
        EXPECT_TRUE(files.empty());
    }
}

TEST_F(MP3StateTests, FailedOpenLeavesNoDecoderForNoteCleanup)
{
    for (const char* opcode : {"mp3scal", "mp3in"}) {
        std::string body = std::string(opcode) == "mp3scal" ?
          "aL,aR,kTime mp3scal \"missing-mp3-state-test.mp3\",1,1,1" :
          "aL,aR mp3in \"missing-mp3-state-test.mp3\"";
        ASSERT_NO_FATAL_FAILURE(start(body, "i 1 0 .01"));
        csoundPerformKsmps(csound);
        EXPECT_NE(std::string::npos, messages().find("failed to open file"));
        EXPECT_TRUE(files.empty());
        reset();
    }
}

TEST_F(MP3StateTests, ReinitClosesOldDecoderAndSkipInitKeepsIt)
{
    for (int mode = 0; mode < 3; ++mode) {
        std::string opcode = mode == 0 ?
          "aL,aR,kTime mp3scal \"" + path + "\",1,1,1,0,256" :
          "aL,aR mp3in \"" + path + "\",0,0," + std::to_string(mode - 1);
        std::string body = "kCount timeinstk\nif kCount == 3 then\n"
          "reinit RESTART\nendif\nRESTART:\n" + opcode + "\nrireturn\nout aL,aR";
        ASSERT_NO_FATAL_FAILURE(start(body, "i 1 0 .05"));
        opens = 0;
        for (int i = 0; i < 75; ++i) {
            csoundPerformKsmps(csound);
            EXPECT_LE(files.size(), 1u);
        }
        EXPECT_EQ(mode == 2 ? 1 : 2, opens);
        EXPECT_TRUE(files.empty());
        reset();
    }
}

TEST_F(MP3StateTests, ReusedNotesMatchFreshNotesAfterChangingFFTSize)
{
    for (const char* previousSkip : {"0", "1000"}) {
        std::vector<MYFLT> reused;
        std::vector<MYFLT> reference;
        for (bool fresh : {false, true}) {
            std::string body = "aL,aR,kTime mp3scal \"" + path +
              "\",1,1,1,p5,p4\nout aL,aR";
            // Each note and gap spans an exact number of control blocks.
            // Skipping past EOF also checks that the next note resumes reading.
            std::string score = fresh ? "i 1 0 0.092879818594 256 0" :
              std::string("i 1 0 0.092879818594 2048 ") + previousSkip +
              "\ni 1 0.185759637188 0.092879818594 256 0";
            ASSERT_NO_FATAL_FAILURE(start(body, score + "\nf 0 0.371519274376"));
            auto& output = fresh ? reference : reused;
            for (int block = 0; block < (fresh ? 128 : 384); ++block) {
                ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound)) << messages();
                if (fresh || block >= 256) {
                    const MYFLT* samples = csoundGetSpout(csound);
                    output.insert(output.end(), samples, samples + 64);
                }
            }
            reset();
        }
        ASSERT_EQ(reference.size(), reused.size());
        EXPECT_TRUE(std::any_of(reference.begin(), reference.end(),
                               [](MYFLT value) { return value != FL(0.0); }));
        for (size_t i = 0; i < reference.size(); ++i)
            ASSERT_EQ(reference[i], reused[i]) << "sample " << i;
    }
}

TEST_F(MP3StateTests, FailedStreamInitializationClosesFile)
{
    // This source file is an ordinary readable file, but not an MP3 stream.
    std::string filename = __FILE__;
    std::replace(filename.begin(), filename.end(), '\\', '/');
    for (const char* opcode : {"mp3scal", "mp3in"}) {
        std::string body = std::string(opcode) == "mp3scal" ?
          "aL,aR,kTime mp3scal \"" + filename + "\",1,1,1" :
          "aL,aR mp3in \"" + filename + "\"";
        ASSERT_NO_FATAL_FAILURE(start(body, "i 1 0 .01"));
        csoundPerformKsmps(csound);
        EXPECT_NE(std::string::npos, messages().find("Not an MPEG audio stream"));
        EXPECT_TRUE(files.empty());
        reset();
    }
}

TEST_F(MP3StateTests, RejectsInvalidFFTAndHopSizes)
{
    for (const char* args : {"-1,4", "1,4", "256,-1", "256,512"}) {
        ASSERT_NO_FATAL_FAILURE(start("aL,aR,kTime mp3scal \"" + path +
          "\",1,1,1,0," + args, "i 1 0 .01"));
        csoundPerformKsmps(csound);
        EXPECT_NE(std::string::npos, messages().find("mp3scal: invalid"));
        EXPECT_TRUE(files.empty());
        reset();
    }
}
} // namespace
