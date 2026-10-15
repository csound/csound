#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

extern "C" void message_dequeue(CSOUND *csound);
extern "C" void input_message_async(CSOUND *, const char *);
extern "C" void read_score_async(CSOUND *, const char *);
extern "C" void score_event_async(CSOUND *, char, const cs_float *, long);
extern "C" int32_t csoundKillInstance(CSOUND *, cs_float, char *, int32_t,
                                      int32_t, int32_t);
extern "C" void free_inactive_instances(CSOUND *csound);

#if defined(CSOUND_TEST_PTHREADS)
namespace {
using namespace std::chrono_literals;
constexpr int queueCapacity = 1024;

class ApiMessageQueueTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        csound = csoundCreate(this, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
        ASSERT_EQ(csoundCompileOrc(csound,
            "sr=48000\nksmps=32\nnchnls=1\ngiTable ftgen 1, 0, -1, -2, 1\n", 0),
            CSOUND_SUCCESS);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
        originalFree = csound->Free;
        originalCalloc = csound->Calloc;
    }

    void TearDown() override
    {
        csound->Free = originalFree;
        csound->Calloc = originalCalloc;
        csoundDestroy(csound);
    }

    static void stalledFree(CSOUND *csound, void *ptr)
    {
        auto *self = static_cast<ApiMessageQueueTests *>(csoundGetHostData(csound));
        if (stallThisProducer) {
            std::unique_lock<std::mutex> lock(self->mutex);
            self->entered = true;
            self->changed.notify_all();
            self->changed.wait(lock, [&] { return self->released; });
            stallThisProducer = false;
        }
        self->originalFree(csound, ptr);
    }

    static void *stalledCalloc(CSOUND *csound, size_t size)
    {
        auto *self = static_cast<ApiMessageQueueTests *>(csoundGetHostData(csound));
        if (stallAllocation) {
            std::unique_lock<std::mutex> lock(self->mutex);
            ++self->allocationWaiters;
            self->changed.notify_all();
            self->changed.wait(lock, [&] { return self->released; });
            stallAllocation = false;
        }
        return self->originalCalloc(csound, size);
    }

    CSOUND *csound = nullptr;
    void (*originalFree)(CSOUND *, void *) = nullptr;
    void *(*originalCalloc)(CSOUND *, size_t) = nullptr;
    static thread_local bool stallThisProducer;
    static thread_local bool stallAllocation;
    int allocationWaiters = 0;
    std::mutex mutex;
    std::condition_variable changed;
    bool entered = false;
    bool released = false;
};
thread_local bool ApiMessageQueueTests::stallThisProducer = false;
thread_local bool ApiMessageQueueTests::stallAllocation = false;

TEST_F(ApiMessageQueueTests, FullQueueReturnsAndAsyncCompileReportsFailure)
{
    std::array<cs_float, queueCapacity> copies{};
    for (auto &copy : copies)
        csoundTableCopyOut(csound, 1, &copy, 1);

    std::atomic<bool> finished{false};
    int result = CSOUND_SUCCESS;
    cs_float overflow = 0;
    std::thread producer([&] {
        csoundTableCopyOut(csound, 1, &overflow, 1);
        result = csoundCompileOrc(csound, "instr 10\nendin\n", 1);
        finished.store(true);
        changed.notify_all();
    });
    bool returned;
    {
        std::unique_lock<std::mutex> lock(mutex);
        returned = changed.wait_for(lock, 1s, [&] { return finished.load(); });
    }
    // Let the old busy-wait implementation exit, so a regression fails safely.
    if (!returned)
        message_dequeue(csound);
    producer.join();
    EXPECT_TRUE(returned);
    EXPECT_EQ(result, CSOUND_ERROR);
    message_dequeue(csound);
    EXPECT_EQ(overflow, 0);
    for (const auto copy : copies)
        EXPECT_EQ(copy, 1);
}

class ApiQueueRejectionTests : public ApiMessageQueueTests,
                               public ::testing::WithParamInterface<int> {
protected:
    void fillQueue()
    {
        for (int i = 0; i < queueCapacity; ++i)
            csoundTableCopyOut(csound, 1, &output, 1);
    }

    void rejectRequest(int api)
    {
        const cs_float fields[] = { 1, 0, 1 };
        switch (api) {
        case 0: csoundTableCopyOut(csound, 1, &output, 1); break;
        case 1: csoundTableCopyIn(csound, 1, input, 1); break;
        case 2: csoundEvent(csound, CS_INSTR_EVENT, fields, 3, 1); break;
        case 3: csoundEventString(csound, "i 1 0 1", 1); break;
        case 4: input_message_async(csound, "i 1 0 1"); break;
        case 5: read_score_async(csound, "i 1 0 1"); break;
        case 6: score_event_async(csound, 'i', fields, 3); break;
        }
    }

    int takeWarnings()
    {
        int count = 0;
        while (csoundGetMessageCnt(csound) > 0) {
            if (std::strstr(csoundGetFirstMessage(csound),
                            "async API request could not be queued") != nullptr) {
                EXPECT_EQ(csoundGetFirstMessageAttr(csound), CSOUNDMSG_WARNING);
                ++count;
            }
            csoundPopFirstMessage(csound);
        }
        return count;
    }

    static void reenterFromWarning(CSOUND *csound, int32_t, const char *message)
    {
        if (std::strstr(message, "async API request could not be queued") == nullptr)
            return;
        auto *self = static_cast<ApiQueueRejectionTests *>(csoundGetHostData(csound));
        // Bound a broken implementation's recursion so the assertion can fail.
        if (++self->callbackWarnings < 3)
            input_message_async(csound, "i 1 0 1");
    }

    cs_float output = 0;
    cs_float input[2] = { 2, 3 };
    int callbackWarnings = 0;
};

TEST_P(ApiQueueRejectionTests, VoidApiWarnsOnceUntilReset)
{
    fillQueue();
    EXPECT_EQ(takeWarnings(), 0);
    rejectRequest(GetParam());
    EXPECT_EQ(takeWarnings(), 1);
    for (int api = 0; api < 7; ++api)
        rejectRequest(api);
    EXPECT_EQ(takeWarnings(), 0);

    message_dequeue(csound);
    EXPECT_EQ(output, 1);
    fillQueue();
    rejectRequest(GetParam());
    EXPECT_EQ(takeWarnings(), 0);
    message_dequeue(csound);
}

INSTANTIATE_TEST_SUITE_P(EachVoidApi, ApiQueueRejectionTests,
                        ::testing::Range(0, 7));

TEST_F(ApiQueueRejectionTests, WarningCallbackCanRejectAnotherRequest)
{
    fillQueue();
    csoundDestroyMessageBuffer(csound);
    csoundSetMessageStringCallback(csound, reenterFromWarning);
    rejectRequest(0);
    EXPECT_EQ(callbackWarnings, 1);
    message_dequeue(csound);
    EXPECT_EQ(output, 1);
}

TEST_F(ApiQueueRejectionTests, ResetAllowsAnotherWarning)
{
    fillQueue();
    rejectRequest(0);
    EXPECT_EQ(takeWarnings(), 1);
    message_dequeue(csound);

    csoundDestroyMessageBuffer(csound);
    csoundReset(csound);
    csoundCreateMessageBuffer(csound, 0);
    // The queue does not exist yet. Start must not re-arm this reset's warning.
    rejectRequest(0);
    EXPECT_EQ(takeWarnings(), 1);
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound,
        "sr=48000\nksmps=32\nnchnls=1\ngiTable ftgen 1, 0, -1, -2, 1\n", 0),
        CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    fillQueue();
    EXPECT_EQ(takeWarnings(), 0);
    rejectRequest(0);
    EXPECT_EQ(takeWarnings(), 0);
    message_dequeue(csound);
}

TEST_F(ApiQueueRejectionTests, DisabledWarningsDoNotConsumeLimit)
{
    fillQueue();
    csoundSetMessageLevel(csound, 0); // Same warning level as -m0.
    rejectRequest(0);
    EXPECT_EQ(takeWarnings(), 0);

    csoundSetMessageLevel(csound, CS_WARNMSG);
    rejectRequest(0);
    EXPECT_EQ(takeWarnings(), 1);
    rejectRequest(1);
    EXPECT_EQ(takeWarnings(), 0);
    message_dequeue(csound);
}

TEST_F(ApiQueueRejectionTests, ConcurrentRejectionsWarnOnce)
{
#if !defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN)
    GTEST_SKIP() << "Concurrent API calls require native atomics";
#endif
    fillQueue();
    std::array<std::thread, 4> producers;
    std::atomic<bool> start{false};
    for (auto &producer : producers) {
        producer = std::thread([&] {
            while (!start.load())
                std::this_thread::yield();
            for (int i = 0; i < 16; ++i)
                rejectRequest(0);
        });
    }
    start.store(true);
    for (auto &producer : producers)
        producer.join();
    EXPECT_EQ(takeWarnings(), 1);
    message_dequeue(csound);
}

TEST_F(ApiMessageQueueTests, UnpublishedHeadKeepsLaterProducerPending)
{
#if !defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN)
    GTEST_SKIP() << "Concurrent API calls require native atomics";
#endif
    std::array<cs_float, queueCapacity> warmup{};
    for (auto &copy : warmup)
        csoundTableCopyOut(csound, 1, &copy, 1);
    message_dequeue(csound);

    // Reuse makes the producer call Free after reserving its slot. Pause there
    // to exercise publication order without relying on thread scheduling.
    csound->Free = stalledFree;
    cs_float first = 0, second = 0;
    std::thread producer([&] {
        stallThisProducer = true;
        csoundTableCopyOut(csound, 1, &first, 1);
    });
    bool paused;
    {
        std::unique_lock<std::mutex> lock(mutex);
        paused = changed.wait_for(lock, 1s, [&] { return entered; });
    }
    if (paused) {
        csoundTableCopyOut(csound, 1, &second, 1);
        message_dequeue(csound);
        EXPECT_EQ(first, 0);
        EXPECT_EQ(second, 0);
    }
    {
        std::lock_guard<std::mutex> lock(mutex);
        released = true;
    }
    changed.notify_all();
    producer.join();
    csound->Free = originalFree;
    ASSERT_TRUE(paused);
    message_dequeue(csound);
    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 1);
}

TEST_F(ApiMessageQueueTests, ConcurrentProducersAndConsumerReuseSlots)
{
#if !defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN)
    GTEST_SKIP() << "Concurrent API calls require native atomics";
#endif
    constexpr int producers = 4, perProducer = 64, rounds = 24;
    std::array<cs_float, producers * perProducer> copies{};
    std::atomic<int> currentRound{0}, finished{0};
    std::array<std::thread, producers> threads;
    for (int p = 0; p < producers; ++p) {
        threads[p] = std::thread([&, p] {
            for (int round = 0; round < rounds; ++round) {
                while (currentRound.load() != round)
                    std::this_thread::yield();
                for (int i = 0; i < perProducer; ++i)
                    csoundTableCopyOut(csound, 1, &copies[p * perProducer + i], 1);
                finished.fetch_add(1);
            }
        });
    }
    for (int round = 0; round < rounds; ++round) {
        while (finished.load() != producers) {
            message_dequeue(csound);
            std::this_thread::yield();
        }
        message_dequeue(csound);
        for (const auto copy : copies)
            EXPECT_EQ(copy, 1) << "round " << round;
        copies.fill(0);
        finished.store(0);
        currentRound.store(round + 1);
    }
    for (auto &thread : threads)
        thread.join();
}

TEST_F(ApiMessageQueueTests, TwoProducersCannotClaimTheLastSlot)
{
#if !defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN)
    GTEST_SKIP() << "Concurrent API calls require native atomics";
#endif
    ASSERT_EQ(csoundCompileOrc(csound, "instr 1\nendin\n", 0), CSOUND_SUCCESS);
    const cs_float fields[] = { 1, 0, -1 };
    csoundEvent(csound, CS_INSTR_EVENT, fields, 3, 0);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NE(csound->actanchor.nxtact, nullptr);
    std::array<cs_float, queueCapacity - 1> copies{};
    for (auto &copy : copies)
        csoundTableCopyOut(csound, 1, &copy, 1);

    csound->Calloc = stalledCalloc;
    std::array<int, 2> results{};
    std::array<std::thread, 2> producers;
    for (int i = 0; i < 2; ++i) {
        producers[i] = std::thread([&, i] {
            stallAllocation = true;
            results[i] = csoundKillInstance(csound, 1, nullptr, 0, 0, 1);
        });
    }
    bool bothPrepared;
    {
        std::unique_lock<std::mutex> lock(mutex);
        bothPrepared = changed.wait_for(lock, 1s,
                                        [&] { return allocationWaiters == 2; });
        released = true;
    }
    changed.notify_all();
    for (auto &producer : producers)
        producer.join();
    csound->Calloc = originalCalloc;
    ASSERT_TRUE(bothPrepared);
    EXPECT_EQ((results[0] == CSOUND_SUCCESS) + (results[1] == CSOUND_SUCCESS), 1);
    EXPECT_EQ((results[0] == CSOUND_ERROR) + (results[1] == CSOUND_ERROR), 1);
    message_dequeue(csound);
    for (const auto copy : copies)
        EXPECT_EQ(copy, 1);
    EXPECT_EQ(csound->actanchor.nxtact, nullptr);
}

TEST_F(ApiMessageQueueTests, PositionCountersWrapWithoutLosingMessages)
{
    // Exercise both the unsigned rollover and the signed storage conversion.
    csound->msg_queue_write = -4;
    csound->msg_queue_read = -4;
    std::array<cs_float, 8> copies{};
    for (auto &copy : copies)
        csoundTableCopyOut(csound, 1, &copy, 1);
    message_dequeue(csound);
    for (const auto copy : copies)
        EXPECT_EQ(copy, 1);
    EXPECT_EQ(csound->msg_queue_write, 4);
    EXPECT_EQ(csound->msg_queue_read, 4);
}

TEST_F(ApiMessageQueueTests, AsyncKillPreservesFractionalInstrumentNumber)
{
    ASSERT_EQ(csoundCompileOrc(csound, "instr 1\nendin\n", 0), CSOUND_SUCCESS);
    const cs_float fields[] = { FL(1.25), 0, -1 };
    csoundEvent(csound, CS_INSTR_EVENT, fields, 3, 0);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NE(csound->actanchor.nxtact, nullptr);
    ASSERT_EQ(csoundKillInstance(csound, FL(1.25), nullptr, 4, 0, 1),
              CSOUND_SUCCESS);
    message_dequeue(csound);
    EXPECT_EQ(csound->actanchor.nxtact, nullptr);
}

TEST_F(ApiMessageQueueTests, AsyncKillIgnoresTargetRemovedBeforeDrain)
{
    ASSERT_EQ(csoundCompileOrc(csound, "instr 1\nendin\n", 0), CSOUND_SUCCESS);
    const cs_float fields[] = { 1, 0, -1 };
    csoundEvent(csound, CS_INSTR_EVENT, fields, 3, 0);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    ASSERT_NE(csound->actanchor.nxtact, nullptr);
    ASSERT_EQ(csoundKillInstance(csound, 1, nullptr, 0, 0, 1), CSOUND_SUCCESS);
    ASSERT_EQ(csoundKillInstance(csound, 1, nullptr, 0, 0, 0), CSOUND_SUCCESS);
    ASSERT_EQ(csound->actanchor.nxtact, nullptr);
    free_inactive_instances(csound);
    message_dequeue(csound);
    EXPECT_EQ(csound->actanchor.nxtact, nullptr);
    EXPECT_EQ(csound->engineState.instrtxtp[1]->active, 0);
}

TEST_F(ApiMessageQueueTests, BusyInitMutexDoesNotDelayOrderedTableCopies)
{
    void *initMutex = csoundCreateMutex(1);
    ASSERT_NE(initMutex, nullptr);
    csound->init_pass_threadlock = initMutex;
    csound->oparms->realtime = 1;
    std::thread holder([&] {
        csoundLockMutex(initMutex);
        std::unique_lock<std::mutex> lock(mutex);
        entered = true;
        changed.notify_all();
        changed.wait(lock, [&] { return released; });
        csoundUnlockMutex(initMutex);
    });
    {
        std::unique_lock<std::mutex> lock(mutex);
        changed.wait(lock, [&] { return entered; });
    }
    cs_float input[] = { 2, 3 };
    cs_float output = 0;
    csoundTableCopyIn(csound, 1, input, 1);
    csoundTableCopyOut(csound, 1, &output, 1);
    std::atomic<bool> finished{false};
    std::thread consumer([&] {
        message_dequeue(csound);
        finished.store(true);
        changed.notify_all();
    });
    bool returned;
    {
        std::unique_lock<std::mutex> lock(mutex);
        returned = changed.wait_for(lock, 1s, [&] { return finished.load(); });
        released = true;
    }
    if (returned)
        EXPECT_EQ(output, 2);
    changed.notify_all();
    holder.join();
    consumer.join();
    EXPECT_TRUE(returned);
    message_dequeue(csound);
    EXPECT_EQ(output, 2);
    csound->oparms->realtime = 0;
    csound->init_pass_threadlock = nullptr;
    csoundDestroyMutex(initMutex);
}

struct NestedMessage {
    int calls = 0;
    bool enqueue = true;
};
void enqueueFromMessage(CSOUND *csound, int32_t, const char *)
{
    auto *state = static_cast<NestedMessage *>(csoundGetHostData(csound));
    if (!state->enqueue)
        return;
    ++state->calls;
    const cs_float fields[] = { 1, 0, 1 };
    if (state->calls < 3)
        csoundEvent(csound, CS_INSTR_EVENT, fields, 3, 1);
}

TEST(ApiMessageQueueCallbackTests, NestedEnqueueWaitsForNextDrain)
{
    NestedMessage state;
    CSOUND *csound = csoundCreate(&state, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundSetMessageStringCallback(csound, enqueueFromMessage);
    const cs_float fields[] = { 1, 0, 1 };
    // An event before Start emits a message; its host callback queues another.
    csoundEvent(csound, CS_INSTR_EVENT, fields, 3, 1);
    for (int calls = 1; calls <= 3; ++calls) {
        message_dequeue(csound);
        EXPECT_EQ(state.calls, calls);
    }
    state.enqueue = false;
    csoundDestroy(csound);
}
} // namespace
#endif
