#if (defined(CSOUND_TEST_PTHREADS) || defined(WIN32)) && \
    !defined(__EMSCRIPTEN__) && !defined(__wasi__)
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <chrono>
#include <future>
#include <string>
#include <thread>

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
          kreads[] chngetk Snames

          SaudioNames[] fillarray "audio-left", "audio-right"
          avalues[] init 2
          avalues[0] = 1
          avalues[1] = 2
          chnseta avalues, SaudioNames
          areads[] chngeta SaudioNames

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
    std::promise<void> started;
    auto running = started.get_future();
    csoundSpinLock(&csound->channel_spinlock);
    auto block = std::async(std::launch::async, [this, &started] {
        started.set_value();
        return csoundPerformKsmps(csound);
    });
    running.wait();
    // Allow slow sanitizer workers ample time after the thread starts.
    auto status = block.wait_for(std::chrono::seconds(30));
    csoundSpinUnLock(&csound->channel_spinlock);
    EXPECT_EQ(std::future_status::ready, status);
    EXPECT_EQ(0, block.get());
}

TEST_F(ChannelBindingTests, AudioArrayWritesLockEachChannelSeparately)
{
    ASSERT_EQ(0, csoundCompileOrc(csound, R"ORC(
        ksmps = 8
        instr 1
          SNames[] fillarray "first", "second"
          kCycle timeinstk
          aValues[] init 2
          aValues[0] = kCycle
          aValues[1] = kCycle + 10
          chnseta aValues, SNames
        endin
        schedule(1, 0, -1)
    )ORC", 0));
    ASSERT_EQ(0, csoundStart(csound));
    ASSERT_EQ(0, csoundPerformKsmps(csound));

    // Blocking the second channel must still let the first receive its block.
    // Reusing the last member's lock for both copies would stop that write.
    csoundLockChannel(csound, "second");
    auto block = std::async(std::launch::async, [this] {
        return csoundPerformKsmps(csound);
    });
    cs_float first[8] = {};
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    do {
        csoundGetAudioChannel(csound, "first", first);
        if (first[0] == 2)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < deadline);
    auto status = block.wait_for(std::chrono::seconds(0));
    csoundUnlockChannel(csound, "second");

    EXPECT_EQ(std::future_status::timeout, status);
    EXPECT_EQ(0, block.get());
    for (cs_float sample : first)
        EXPECT_EQ(2, sample);
    cs_float second[8] = {};
    csoundGetAudioChannel(csound, "second", second);
    for (cs_float sample : second)
        EXPECT_EQ(12, sample);
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

TEST_F(ChannelBindingTests, MissingBindingStorageReportsAnInitError)
{
    ASSERT_EQ(0, csoundCompileOrc(csound, R"ORC(
        instr 1
          SNames[] fillarray "first", "second"
          kValues[] chngetk SNames
        endin
        schedule(1, 0, -1)
    )ORC", 0));
    ASSERT_EQ(0, csoundStart(csound));

    // Simulate AuxAlloc returning without storage for the channel bindings.
    auto auxAlloc = csound->AuxAlloc;
    csound->AuxAlloc = [](CSOUND *, size_t, AUXCH *storage) {
        storage->auxp = nullptr;
        storage->size = 0;
    };
    csoundPerformKsmps(csound);
    csound->AuxAlloc = auxAlloc;
    EXPECT_GT(csound->inerrcnt, 0);

    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(std::string::npos, messages.find("memory allocation failure"));
}
#endif
