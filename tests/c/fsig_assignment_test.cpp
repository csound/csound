#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csdebug.h"
#include "pstream.h"
#include "gtest/gtest.h"

#include <cstring>
#include <string>
#include <vector>

namespace {

class FsigAssignmentTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;

    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    }

    void TearDown() override { csoundDestroy(csound); }

    void start(const std::string &source)
    {
        const auto orchestra =
            "sr = 8192\nksmps = 32\nnchnls = 1\n0dbfs = 1\n" + source +
            "\ngfCopy init gfSource\n"
            "instr 1\n"
            "kCopy chnget \"copy\"\n"
            "if kCopy == 1 then\ngfCopy = gfSource\nendif\n"
            "endin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
        csoundSetControlChannel(csound, "copy", 1);
        csoundEventString(csound, "i 1 0 1", 0);
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    }

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

    std::vector<unsigned char> contents(const PVSDAT *frame)
    {
        auto *bytes = static_cast<unsigned char *>(frame->frame.auxp);
        return {bytes, bytes + frame->frame.size};
    }

    void checkIndependentCopy()
    {
        auto *source = signal("gfSource");
        auto *copy = signal("gfCopy");
        ASSERT_NE(source, nullptr);
        ASSERT_NE(copy, nullptr);
        ASSERT_NE(source->frame.auxp, copy->frame.auxp);
        ASSERT_EQ(contents(copy), contents(source));

        // Use every byte, including the end of a sliding block or track list.
        // No DSP opcode reads these test frames.
        auto *bytes = static_cast<unsigned char *>(source->frame.auxp);
        for (size_t i = 0; i < source->frame.size; ++i)
            bytes[i] = static_cast<unsigned char>(i % 251);
        source->framecount += 5;
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        EXPECT_EQ(contents(copy), contents(source));
        const auto held = contents(copy);
        const auto heldCount = copy->framecount;

        csoundSetControlChannel(csound, "copy", 0);
        bytes[source->frame.size - 1] ^= 1;
        source->framecount += 3;
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        EXPECT_EQ(contents(copy), held);
        EXPECT_EQ(copy->framecount, heldCount);

        csoundSetControlChannel(csound, "copy", 1);
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        EXPECT_EQ(contents(copy), contents(source));
        EXPECT_GT(copy->framecount, heldCount);

        // Restarting a source must still publish an update to downstream users.
        const auto beforeRestart = copy->framecount;
        bytes[0] ^= 1;
        source->framecount = 1;
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        EXPECT_EQ(contents(copy), contents(source));
        EXPECT_GT(copy->framecount, beforeRestart);
    }
};

TEST_F(FsigAssignmentTests, OrdinaryFrames)
{
    ASSERT_NO_FATAL_FAILURE(start("gfSource pvsinit 128, 32, 128, 1\n"));
    ASSERT_NO_FATAL_FAILURE(checkIndependentCopy());
    auto *copy = signal("gfCopy");
    const auto count = copy->framecount;
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_EQ(copy->framecount, count) << "An unchanged frame is not a new frame";
}

TEST_F(FsigAssignmentTests, SlidingFrames)
{
    ASSERT_NO_FATAL_FAILURE(start("gfSource pvsinit 64, 1, 64, 1\n"));
    ASSERT_NO_FATAL_FAILURE(checkIndependentCopy());
    auto *source = signal("gfSource");
    auto *copy = signal("gfCopy");
    EXPECT_EQ(copy->NB, source->NB);
    // Sliding producers need not advance a frame counter between blocks.
    static_cast<MYFLT *>(source->frame.auxp)[32 * 66 - 1] = FL(42.0);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_EQ(contents(copy), contents(source));
}

TEST_F(FsigAssignmentTests, TrackFrames)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "gaInput init 0\n"
        "gfFrequency, gfPhase pvsifd gaInput, 128, 32, 1\n"
        "gfSource partials gfFrequency, gfPhase, .01, 1, 2, 8\n"));
    ASSERT_EQ(signal("gfSource")->format, PVS_TRACKS);
    ASSERT_NO_FATAL_FAILURE(checkIndependentCopy());
}

TEST_F(FsigAssignmentTests, InitializationCopiesAndSelfAssignmentPreservesData)
{
    ASSERT_NO_FATAL_FAILURE(start("gfSource pvsinit 128, 32, 128, 1\n"));
    const auto initial = contents(signal("gfSource"));
    ASSERT_EQ(csoundCompileOrc(csound, "gfSource = gfSource\n", 0), CSOUND_SUCCESS);
    EXPECT_EQ(contents(signal("gfSource")), initial);

    auto *source = signal("gfSource");
    static_cast<float *>(source->frame.auxp)[0] = .75f;
    ASSERT_EQ(csoundCompileOrc(csound, "gfCopy init gfSource\n", 0), CSOUND_SUCCESS);
    EXPECT_EQ(contents(signal("gfCopy")), contents(source));
}

TEST_F(FsigAssignmentTests, ReinitializationChangesFrameLayout)
{
    ASSERT_NO_FATAL_FAILURE(start("gfSource pvsinit 128, 32, 128, 1\n"));
    for (const auto *configuration : {"64, 1", "256, 64", "128, 32"}) {
        const auto orchestra = std::string("gfSource pvsinit ") + configuration +
            "\ngfCopy init gfSource\n";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        auto *source = signal("gfSource");
        auto *copy = signal("gfCopy");
        const size_t bytes = (source->N + 2) *
            (source->sliding ? 32 * sizeof(MYFLT) : sizeof(float));
        EXPECT_EQ(copy->frame.size, bytes);
        EXPECT_EQ(std::memcmp(copy->frame.auxp, source->frame.auxp, bytes), 0);
    }
}

TEST_F(FsigAssignmentTests, IncompatibleSourceReportsAnError)
{
    ASSERT_NO_FATAL_FAILURE(start("gfSource pvsinit 128, 32, 128, 1\n"));
    const auto held = contents(signal("gfCopy"));
    ASSERT_EQ(csoundCompileOrc(csound, "gfSource pvsinit 256, 64, 256, 1\n", 0),
              CSOUND_SUCCESS);
    csoundPerformKsmps(csound);
    EXPECT_EQ(contents(signal("gfCopy")), held);
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(messages.find("fsig = : incompatible frames"), std::string::npos);
}

} // namespace
