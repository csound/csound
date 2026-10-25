#if (defined(CSOUND_TEST_PTHREADS) || defined(WIN32)) && \
    !defined(__EMSCRIPTEN__) && !defined(__wasi__)
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <chrono>
#include <future>
#include <string>

class ChannelBindingTests : public ::testing::Test {
protected:
    CSOUND *csound;

    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
    }

    void TearDown() override {
        csoundDestroy(csound);
    }
};

TEST_F(ChannelBindingTests, UnchangedNamesDoNotWaitForTheRegistry)
{
    ASSERT_EQ(0, csoundCompileOrc(csound, R"ORC(
        sr = 48000
        ksmps = 64
        nchnls = 1
        struct Value value:k
        instr 1
          Stext init "hello"
          chnset Stext, "text"
          Sread chnget "text"

          value:Value init 1
          chnset value, "object"
          read:Value chnget "object"

          Snames[] fillarray "left", "right"
          kvalues[] fillarray 1, 2
          chnsetk kvalues, Snames

          Sstrings[] fillarray "first", "second"
          SstringNames[] fillarray "first-string", "second-string"
          chnsets Sstrings, SstringNames
          Sreads[] chngets SstringNames
        endin
        schedule(1, 0, -1)
    )ORC", 0));
    ASSERT_EQ(0, csoundStart(csound));
    ASSERT_EQ(0, csoundPerformKsmps(csound));

    // Growing the table must not invalidate the bindings saved by the opcodes.
    for (int i = 0; i < 256; ++i)
        csoundSetControlChannel(csound, ("extra" + std::to_string(i)).c_str(), i);

    // Hold the registry as a host thread might while listing channels. The
    // next audio block should use its saved bindings and finish without it.
    csoundSpinLock(&csound->channel_spinlock);
    auto block = std::async(std::launch::async, [this] {
        return csoundPerformKsmps(csound);
    });
    auto status = block.wait_for(std::chrono::seconds(2));
    csoundSpinUnLock(&csound->channel_spinlock);
    EXPECT_EQ(std::future_status::ready, status);
    EXPECT_EQ(0, block.get());
}

TEST_F(ChannelBindingTests, CachedStringSeesHostBufferGrowth)
{
    ASSERT_EQ(0, csoundCompileOrc(csound, R"ORC(
        instr 1
          Stext chnget "input"
          chnset Stext, "output"
        endin
        schedule(1, 0, -1)
    )ORC", 0));
    ASSERT_EQ(0, csoundStart(csound));
    csoundSetStringChannel(csound, "input", "short");
    ASSERT_EQ(0, csoundPerformKsmps(csound));

    // Replacing the payload must not leave a cached reader on the old buffer.
    std::string expected(1024, 'x');
    csoundSetStringChannel(csound, "input", expected.c_str());
    ASSERT_EQ(0, csoundPerformKsmps(csound));
    std::string result(expected.size() + 1, '\0');
    csoundGetStringChannel(csound, "output", result.data());
    EXPECT_STREQ(expected.c_str(), result.c_str());
}
#endif
