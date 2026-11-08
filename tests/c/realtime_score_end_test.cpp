#ifndef __BUILDING_LIBCSOUND
#define __BUILDING_LIBCSOUND
#endif
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

extern "C" int32_t csound_test_score_end_perform(CSOUND *, int32_t *);

namespace {
using namespace std::chrono_literals;

struct InitGate {
    std::mutex mutex;
    std::condition_variable changed;
    bool entered = false;
    bool released = false;
};

int32_t holdInit(CSOUND *csound, void *)
{
    auto *gate = static_cast<InitGate *>(csoundGetHostData(csound));
    std::unique_lock<std::mutex> lock(gate->mutex);
    gate->entered = true;
    gate->changed.notify_all();
    return gate->changed.wait_for(lock, 2s, [&] { return gate->released; })
        ? CSOUND_SUCCESS : CSOUND_ERROR;
}

class RealtimeScoreEndTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    InitGate gate;

    void SetUp() override
    {
        csound = csoundCreate(&gate, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n -d -m0 --realtime"), 0);
        ASSERT_EQ(csoundAppendOpcode(csound, "test_score_end_init", sizeof(OPDS),
                                    0, "", "", holdInit, nullptr, nullptr), 0);
    }

    void releaseInit()
    {
        std::lock_guard<std::mutex> lock(gate.mutex);
        gate.released = true;
        gate.changed.notify_all();
    }

    void TearDown() override
    {
        releaseInit();
        if (csound != nullptr)
            csoundDestroy(csound);
    }

    void start(const char *score)
    {
        const std::string source =
            "<CsoundSynthesizer>\n<CsInstruments>\n"
            "sr=48000\nksmps=64\nnchnls=1\n"
            "instr 1\n test_score_end_init\nendin\n"
            "</CsInstruments>\n<CsScore>\n" + std::string(score) +
            "</CsScore>\n</CsoundSynthesizer>\n";
        ASSERT_EQ(csoundCompileCSD(csound, source.c_str(), 1, 0), 0);
        ASSERT_EQ(csoundStart(csound), 0);
    }

    int32_t performWithoutSleep()
    {
        int32_t sleepCalls = -1;
        const int32_t result = csound_test_score_end_perform(csound, &sleepCalls);
        EXPECT_EQ(sleepCalls, 0);
        return result;
    }
};

TEST_F(RealtimeScoreEndTests, EmptyScoreEndsWithoutSleep)
{
    start("e\n");
    ASSERT_FALSE(HasFatalFailure());
    EXPECT_EQ(performWithoutSleep(), 2);
}

TEST_F(RealtimeScoreEndTests, SectionBoundaryAndScoreEndDoNotSleep)
{
    start("f0 0.002\ns\nf0 0.002\ne\n");
    ASSERT_FALSE(HasFatalFailure());
    ASSERT_EQ(csound->musmonStatics.sectno, 1);
    int32_t result = 0;
    for (int32_t cycle = 0; cycle < 32 && result == 0; ++cycle)
        result = performWithoutSleep();
    EXPECT_EQ(result, 2);
    EXPECT_EQ(csound->musmonStatics.sectno, 2);
}

TEST_F(RealtimeScoreEndTests, ScoreEndWaitsForRunningInitAndNoteExpiry)
{
    start("i1 0 0.01\ne\n");
    ASSERT_FALSE(HasFatalFailure());
    EXPECT_EQ(performWithoutSleep(), 0);
    {
        std::unique_lock<std::mutex> lock(gate.mutex);
        ASSERT_TRUE(gate.changed.wait_for(lock, 2s, [&] { return gate.entered; }));
    }
    ASSERT_TRUE(alloc_queue_has_pending(csound));
    EXPECT_EQ(performWithoutSleep(), 0);
    releaseInit();

    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (alloc_queue_has_pending(csound) &&
           std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(1ms);
    ASSERT_FALSE(alloc_queue_has_pending(csound));
    ASSERT_NE(csound->frstoff, nullptr);

    int32_t result = 0;
    for (int32_t cycle = 0; cycle < 32 && result == 0; ++cycle)
        result = performWithoutSleep();
    EXPECT_EQ(result, 2);
    EXPECT_EQ(csound->frstoff, nullptr);
}
} // namespace
