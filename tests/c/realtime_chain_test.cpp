#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>
#include <initializer_list>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace {
class ActiveChainTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), 0);
    }
    void TearDown() override { if (csound) csoundDestroy(csound); }
    void start(const char *action, const char *condition = "kdone == 0")
    {
        const std::string source =
            "<CsoundSynthesizer>\n<CsInstruments>\n"
            "sr=48000\nksmps=64\nnchnls=1\n"
            "instr 1\nkdone init 0\nktrigger chnget \"trigger\"\nif " +
            std::string(condition) + " then\n" +
            std::string(action) + "\nkdone=1\nendif\nendin\n"
            "instr 2\nkcount init 0\nkcount += 1\nendin\n"
            "instr 3\nkcount init 0\nkcount += 1\n"
            "chnset kcount, \"cycles\"\nendin\n"
            "</CsInstruments>\n<CsScore>\n"
            "i1 0 1\ni2 0 1\ni3 0 1\ne\n"
            "</CsScore>\n</CsoundSynthesizer>\n";
        ASSERT_EQ(csoundCompileCSD(csound, source.c_str(), 1, 0), 0);
        ASSERT_EQ(csoundStart(csound), 0);
    }
};

TEST_F(ActiveChainTests, TurningOffNextInstanceKeepsItsSuccessor)
{
    start("turnoff2 2, 0, 0");
    ASSERT_FALSE(HasFatalFailure());
    for (int cycle = 1; cycle <= 3; ++cycle) {
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        int error = 0;
        EXPECT_EQ(csoundGetControlChannel(csound, "cycles", &error), cycle);
        EXPECT_EQ(error, 0);
    }
}

TEST_F(ActiveChainTests, TurningOffCurrentAndNextKeepsLaterInstances)
{
    start("turnoff2 2, 0, 0\nturnoff");
    ASSERT_FALSE(HasFatalFailure());
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    int error = 0;
    EXPECT_EQ(csoundGetControlChannel(csound, "cycles", &error), 1);
    EXPECT_EQ(error, 0);
}


class SpliceChainTests : public ActiveChainTests {
protected:
    void checkSplice(const char *call, std::initializer_list<int32_t> order)
    {
        const std::string source =
            "<CsoundSynthesizer>\n<CsInstruments>\n"
            "sr=48000\nksmps=64\nnchnls=1\n"
            "instr 1\nfirst@global:Instr = this\nendin\n"
            "instr 2\nsecond@global:Instr = this\nendin\n"
            "instr 3\nthird@global:Instr = this\nendin\n"
            "instr 4\nresult:i = " + std::string(call) + "\nendin\n"
            "</CsInstruments>\n<CsScore>\n"
            "i1 0 1\ni2 0 1\ni3 0 1\ni4 0 1\ne\n"
            "</CsScore>\n</CsoundSynthesizer>\n";
        ASSERT_EQ(csoundCompileCSD(csound, source.c_str(), 1, 0), 0);
        ASSERT_EQ(csoundStart(csound), 0);
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
        INSDS *previous = &csound->actanchor;
        INSDS *current = previous->nxtact;
        for (int32_t insno : order) {
            EXPECT_NE(current, nullptr);
            if (current == nullptr)
                break;
            EXPECT_EQ(current->insno, insno);
            EXPECT_EQ(current->prvact, previous);
            // Repair after checking so a bad backlink cannot trap cleanup.
            current->prvact = previous;
            previous = current;
            current = current->nxtact;
        }
        EXPECT_EQ(current, nullptr);
    }
};

TEST_F(SpliceChainTests, SplicingBeforeRepairsBothDirections)
{
    checkSplice("splice(third, first, 0)", {3, 1, 2, 4});
}

TEST_F(SpliceChainTests, SplicingAfterRepairsBothDirections)
{
    checkSplice("splice(first, second, 1)", {2, 1, 3, 4});
}

#if defined(CSOUND_TEST_PTHREADS)
struct RenderGate {
    std::mutex mutex;
    std::condition_variable changed;
    bool entered = false;
    bool released = false;
};

int32_t holdRender(CSOUND *csound, void *)
{
    auto *gate = static_cast<RenderGate *>(csoundGetHostData(csound));
    std::unique_lock<std::mutex> lock(gate->mutex);
    gate->entered = true;
    gate->changed.notify_all();
    return gate->changed.wait_for(lock, std::chrono::seconds(2),
                                  [&] { return gate->released; })
        ? CSOUND_SUCCESS : CSOUND_ERROR;
}

TEST_F(ActiveChainTests, RetiredSuccessorCannotBeReusedDuringRender)
{
    using namespace std::chrono_literals;
    RenderGate gate;
    csoundSetHostData(csound, &gate);
    ASSERT_EQ(csoundSetOption(csound, "--realtime"), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_hold_render", sizeof(OPDS),
                                0, "", "", nullptr, holdRender, nullptr), 0);
    start("turnoff2 2, 0, 0\ntest_hold_render",
          "kdone == 0 && ktrigger > 0");
    ASSERT_FALSE(HasFatalFailure());
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (alloc_queue_has_pending(csound) &&
           std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(1ms);
    ASSERT_FALSE(alloc_queue_has_pending(csound));

    INSDS *retired = csound->actanchor.nxtact;
    while (retired != nullptr && retired->insno != 2)
        retired = retired->nxtact;
    ASSERT_NE(retired, nullptr);
    INSTRTXT *instrument = retired->instr;
    csoundSetControlChannel(csound, "trigger", 1);
    int result = CSOUND_ERROR;
    std::thread renderer([&] { result = csoundPerformKsmps(csound); });
    bool paused;
    {
        std::unique_lock<std::mutex> lock(gate.mutex);
        paused = gate.changed.wait_for(lock, 2s, [&] { return gate.entered; });
    }
    INSDS *fresh = nullptr;
    if (paused) {
        EXPECT_EQ(retired->actflg, 0);
        // Use the same free-list admission as the init worker. A render
        // callback still has this retired successor in its local traversal.
        fresh = allocate_or_take_instance(csound, instrument, 2);
        EXPECT_NE(fresh, nullptr);
        EXPECT_NE(fresh, retired);
    }
    {
        std::lock_guard<std::mutex> lock(gate.mutex);
        gate.released = true;
    }
    gate.changed.notify_all();
    renderer.join();
    EXPECT_TRUE(paused);
    EXPECT_EQ(result, 0);
    if (fresh != nullptr && fresh != retired) {
        INSDS *reused = allocate_or_take_instance(csound, instrument, 2);
        EXPECT_EQ(reused, retired);
        if (reused != nullptr)
            recycle_inactive_instance(csound, reused);
    }
    if (fresh != nullptr)
        recycle_inactive_instance(csound, fresh);
    csoundSetHostData(csound, nullptr);
}
#endif

struct NestedEntry {
    int32_t result = CSOUND_SUCCESS;
    long during = 0;
};

int32_t nestedPerformance(CSOUND *csound, void *)
{
    auto *entry = static_cast<NestedEntry *>(csoundGetHostData(csound));
    entry->during = ATOMIC_GET(csound->performance_epoch);
    entry->result = csoundPerformKsmps(csound);
    return CSOUND_SUCCESS;
}

int32_t exitPerformance(CSOUND *csound, void *)
{
    csoundLongJmp(csound, 1);
    return CSOUND_ERROR;
}

TEST_F(ActiveChainTests, NestedPerformanceCannotCloseTheOuterInterval)
{
    NestedEntry entry;
    csoundSetHostData(csound, &entry);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_nested_performance", sizeof(OPDS),
                                0, "", "", nullptr, nestedPerformance, nullptr), 0);
    start("test_nested_performance");
    ASSERT_FALSE(HasFatalFailure());
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(entry.during & 1L, 1);
    EXPECT_EQ(entry.result, CSOUND_ERROR);
    EXPECT_EQ(ATOMIC_GET(csound->performance_epoch), entry.during + 1);
    EXPECT_EQ(csound->global_kcounter, 1);
    csoundSetHostData(csound, nullptr);
}

TEST_F(ActiveChainTests, LongJumpClosesTheRenderInterval)
{
    ASSERT_EQ(csoundAppendOpcode(csound, "test_exit_performance", sizeof(OPDS),
                                0, "", "", nullptr, exitPerformance, nullptr), 0);
    start("test_exit_performance");
    ASSERT_FALSE(HasFatalFailure());
    EXPECT_NE(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(ATOMIC_GET(csound->performance_epoch) & 1L, 0);
    csoundReset(csound);
    EXPECT_EQ(ATOMIC_GET(csound->performance_epoch), 0);
}
} // namespace
