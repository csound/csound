#if defined(WIN32) && !defined(__MINGW32__)
# include <Windows.h>
#else
# include "unistd.h"
#endif
#include <stdio.h>
#include <string.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "gtest/gtest.h"

#include "csound.hpp"
#include "csPerfThread.hpp"
#include "csPerfThread.h"

// A test hook of the library (H/prototyp.h): the blocks Csound holds.
extern "C" size_t csound_tracked_block_count(CSOUND *);

namespace {

struct EvalCodeResult {
    std::atomic<int> legacyCalls{0};
    std::atomic<int> userdataCalls{0};
    std::atomic<cs_float> legacyValue{0.0};
    std::atomic<cs_float> userdataValue{0.0};
    std::atomic<void *> userdataPtr{nullptr};
    std::atomic<int> nestedCalls{0};
    std::atomic<cs_float> nestedValue{0.0};
};

EvalCodeResult evalCodeResult;

// Lets the slow EvalCode callback signal that it has started and then hold
// until the test releases it, instead of relying on sleeps.
std::mutex evalCodeSyncMutex;
std::condition_variable evalCodeSyncCv;
bool evalCodeCallbackEntered = false;
bool evalCodeCallbackRelease = false;

void onEvalCodeDone(cs_float value) {
    evalCodeResult.legacyValue.store((cs_float) value);
    evalCodeResult.legacyCalls.fetch_add(1);
}

void onEvalCodeDoneWithData(cs_float value, void *userdata) {
    {
        std::unique_lock<std::mutex> lock(evalCodeSyncMutex);
        evalCodeCallbackEntered = true;
        evalCodeSyncCv.notify_all();
        evalCodeSyncCv.wait(lock, [] { return evalCodeCallbackRelease; });
    }
    evalCodeResult.userdataValue.store((cs_float) value);
    evalCodeResult.userdataPtr.store(userdata);
    evalCodeResult.userdataCalls.fetch_add(1);
}

void onNestedEvalCodeDone(cs_float value) {
    evalCodeResult.nestedValue.store((cs_float) value);
    evalCodeResult.nestedCalls.fetch_add(1);
}

// Runs on the performance thread and queues another message on the same
// performance thread. Only queueing methods (EvalCode, Play, ScoreEvent, ...)
// may be called this way; FlushMessageQueue() and Join() would self-deadlock.
void onEvalCodeQueuesMoreWork(cs_float value, void *userdata) {
    evalCodeResult.legacyValue.store((cs_float) value);
    evalCodeResult.legacyCalls.fetch_add(1);
    CsoundPerformanceThread *pt =
        static_cast<CsoundPerformanceThread *>(userdata);
    pt->EvalCode("i1 = 5 + 5\nreturn i1\n", onNestedEvalCodeDone);
}

// State for AudioBlockRunsBetweenBatches: the first EvalCode callback queues
// a second message, and we record whether the process callback ran before the
// second message was processed.
std::atomic<bool> firstBatchDone{false};
std::atomic<bool> processRanAfterFirst{false};
std::atomic<bool> secondSawProcess{false};

void onSecondBatchDone(cs_float value) {
    (void) value;
    secondSawProcess.store(processRanAfterFirst.load());
}

void onFirstBatchDone(cs_float value, void *userdata) {
    (void) value;
    firstBatchDone.store(true);
    CsoundPerformanceThread *pt =
        static_cast<CsoundPerformanceThread *>(userdata);
    pt->EvalCode("i1 = 2\nreturn i1\n", onSecondBatchDone);
}

void onProcessCallback(void *userdata) {
    (void) userdata;
    if (firstBatchDone.load())
        processRanAfterFirst.store(true);
}

// State for ProcessCallbackCanFlush: the process callback calls
// FlushMessageQueue() from the performance thread and must not deadlock.
std::atomic<bool> processFlushFinished{false};
std::atomic<bool> processFlushCallbackRan{false};
std::atomic<bool> processFlushReturned{false};

void onProcessCallbackFlush(void *userdata) {
    CsoundPerformanceThread *pt =
        static_cast<CsoundPerformanceThread *>(userdata);
    processFlushCallbackRan.store(true);
    pt->FlushMessageQueue();
    processFlushReturned.store(true);
    pt->Stop();
}

// State for ProcessCallbackJoinIsIgnored: the process callback flushes and
// joins its own thread at every cycle. The thread cannot wait for itself, and
// Join() must not free what the performance goes on using.
std::atomic<int> processJoinCalls{0};
std::atomic<int> processJoinResult{-1};

void onProcessCallbackJoin(void *userdata) {
    CsoundPerformanceThread *pt =
        static_cast<CsoundPerformanceThread *>(userdata);
    pt->FlushMessageQueue();
    int32_t result = pt->Join();
    if (processJoinCalls.fetch_add(1) == 0)
        processJoinResult.store(result);
}

// State for ProcessCallbackDestroyStopsIt: the process callback destroys the
// thread through the C API, which the thread cannot do to itself.
std::atomic<int> processDestroyCalls{0};

void onProcessCallbackDestroy(void *userdata) {
    if (processDestroyCalls.fetch_add(1) == 0)
        csoundDestroyPerformanceThread(userdata);
}

// State for SetProcessCallbackWhileRunning: the callback is set/cleared from
// another thread while the performance thread reads and invokes it.
struct ProcessCallbackSignal {
    std::mutex mutex;
    std::condition_variable cv;
    bool called = false;

    bool wait() {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_for(lock, std::chrono::seconds(5),
                           [this] { return called; });
    }
};

void onSignalProcessCallback(void *userdata) {
    auto *signal = static_cast<ProcessCallbackSignal *>(userdata);
    {
        std::lock_guard<std::mutex> lock(signal->mutex);
        signal->called = true;
    }
    signal->cv.notify_one();
}

// A score of 0.1 s at 48 kHz, mono, whose take a recording with numbufs
// 100 holds whole: 100 buffers of 256 frames.
const char *const kRecordedOrchestra =
    "sr=48000\nksmps=64\nnchnls=1\n0dbfs=1\n"
    "instr 1\nout(oscili:a(0.5, 440))\nendin\n";
const char *const kRecordedScore = "i 1 0 0.1\ne 0.1\n";
const int32_t kRecordBuffers = 100;
const long kWholeTake = 44 + 2 * 4800;  // a WAV header and 16-bit frames

// The length of the data chunk of the WAV file at path, or -1.
long recordedBytes(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == nullptr)
        return -1;
    std::string wav;
    char chunk[4096];
    size_t got;
    while ((got = fread(chunk, 1, sizeof chunk, file)) > 0)
        wav.append(chunk, got);
    fclose(file);
    const size_t data = wav.find("data", 12);
    if (data == std::string::npos || data + 8 > wav.size())
        return -1;
    const unsigned char *length = (const unsigned char *) wav.data() + data + 4;
    return (long) length[0] | (long) length[1] << 8 | (long) length[2] << 16 |
           (long) length[3] << 24;
}

// The size of the file at path, or -1 if there is none.
long fileSize(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == nullptr)
        return -1;
    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fclose(file);
    return size;
}

void startRecordedScore(Csound &csound, const char *output) {
    csound.SetOption("-W");
    csound.SetOption("-b256");
    csound.SetOption(output);
    ASSERT_EQ(csound.CompileOrc(kRecordedOrchestra), 0);
    csound.EventString(kRecordedScore);
    ASSERT_EQ(csound.Start(), 0);
}

}  // namespace

TEST(PerfThreadsTests, PerfThread) {
    const char *instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    Csound csound;
    csound.SetOption((char*)"-odac");
    csound.CompileOrc(instrument);
    csound.EventString((char*)"i 1 0  3 10000 5000\n");
    csound.Start();

    CsoundPerformanceThread performanceThread1(csound.GetCsound());
    performanceThread1.Play();
    performanceThread1.Join();
    csound.Reset();

    CsoundPerformanceThread performanceThread2(csound.GetCsound());
    csound.SetOption((char*)"-odac");
    csound.CompileOrc(instrument);
    csound.EventString((char*)"i 1 0  3 10000 5000\n");
    csound.Start();

    performanceThread2.Play();
    performanceThread2.Join();

    csound.Reset();
}

TEST(PerfThreadsTests, EvalCodeCallbacks) {
    const char *instrument =
        "instr 1 \n"
        "a1 oscili 0.1, 440 \n"
        "out a1 \n"
        "endin \n";

    Csound csound;
    csound.SetOption("-odac");
    csound.CompileOrc(instrument);
    csound.EventString((char*)"i 1 0 30\n");
    csound.Start();

    evalCodeResult.legacyCalls.store(0);
    evalCodeResult.userdataCalls.store(0);
    evalCodeResult.legacyValue.store(0.0);
    evalCodeResult.userdataValue.store(0.0);
    evalCodeResult.userdataPtr.store(nullptr);

    {
        std::lock_guard<std::mutex> lock(evalCodeSyncMutex);
        evalCodeCallbackEntered = false;
        evalCodeCallbackRelease = false;
    }

    CsoundPerformanceThread performanceThread(csound.GetCsound());
    performanceThread.Play();

    int marker = 0;
    performanceThread.EvalCode("i1 = 2 + 2\nreturn i1\n", onEvalCodeDone);
    performanceThread.EvalCode("i1 = 3 + 4\nreturn i1\n",
                               onEvalCodeDoneWithData, (void *) &marker);

    // Wait until the second callback is running: its message has been
    // detached from the queue, but the callback is still held.
    bool entered;
    {
        std::unique_lock<std::mutex> lock(evalCodeSyncMutex);
        entered = evalCodeSyncCv.wait_for(lock, std::chrono::seconds(5),
                                          [] { return evalCodeCallbackEntered; });
        if (!entered)
            evalCodeCallbackRelease = true;
    }
    if (!entered)
        evalCodeSyncCv.notify_all();
    ASSERT_TRUE(entered);

    // FlushMessageQueue() must wait for the in-flight (detached) callback,
    // even though the message queue is empty. Run it from another thread so
    // we can observe that it does not return early.
    std::atomic<bool> flushDone{false};
    std::thread flusher([&] {
        performanceThread.FlushMessageQueue();
        flushDone.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(flushDone.load());

    {
        std::lock_guard<std::mutex> lock(evalCodeSyncMutex);
        evalCodeCallbackRelease = true;
    }
    evalCodeSyncCv.notify_all();
    flusher.join();
    EXPECT_TRUE(flushDone.load());

    EXPECT_EQ(evalCodeResult.legacyCalls.load(), 1);
    EXPECT_EQ(evalCodeResult.userdataCalls.load(), 1);
    EXPECT_DOUBLE_EQ(evalCodeResult.legacyValue.load(), 4.0);
    EXPECT_DOUBLE_EQ(evalCodeResult.userdataValue.load(), 7.0);
    EXPECT_EQ(evalCodeResult.userdataPtr.load(), (void *) &marker);

    performanceThread.Stop();
    performanceThread.Join();
    csound.Reset();
}

TEST(PerfThreadsTests, EvalCodeCallbackQueuesMessage) {
    const char *instrument =
        "instr 1 \n"
        "a1 oscili 0.1, 440 \n"
        "out a1 \n"
        "endin \n";

    Csound csound;
    csound.SetOption("-odac");
    csound.CompileOrc(instrument);
    csound.EventString((char*)"i 1 0 30\n");
    csound.Start();

    evalCodeResult.legacyCalls.store(0);
    evalCodeResult.legacyValue.store(0.0);
    evalCodeResult.nestedCalls.store(0);
    evalCodeResult.nestedValue.store(0.0);

    CsoundPerformanceThread performanceThread(csound.GetCsound());
    performanceThread.Play();

    // The callback queues another EvalCode on the same performance thread.
    // This must not block, i.e. the callback is invoked with the message
    // queue lock released.
    performanceThread.EvalCode("i1 = 7 + 1\nreturn i1\n",
                               onEvalCodeQueuesMoreWork,
                               (void *) &performanceThread);

    // Called from outside any callback, so it waits for both the outer and
    // the nested message (and their callbacks) to be processed.
    performanceThread.FlushMessageQueue();

    EXPECT_EQ(evalCodeResult.legacyCalls.load(), 1);
    EXPECT_DOUBLE_EQ(evalCodeResult.legacyValue.load(), 8.0);
    EXPECT_EQ(evalCodeResult.nestedCalls.load(), 1);
    EXPECT_DOUBLE_EQ(evalCodeResult.nestedValue.load(), 10.0);

    performanceThread.Stop();
    performanceThread.Join();
    csound.Reset();
}

TEST(PerfThreadsTests, AudioBlockRunsBetweenBatches) {
    const char *instrument =
        "instr 1 \n"
        "a1 oscili 0.1, 440 \n"
        "out a1 \n"
        "endin \n";

    Csound csound;
    csound.SetOption("-odac");
    csound.CompileOrc(instrument);
    csound.EventString((char*)"i 1 0 30\n");
    csound.Start();

    firstBatchDone.store(false);
    processRanAfterFirst.store(false);
    secondSawProcess.store(false);

    CsoundPerformanceThread performanceThread(csound.GetCsound());
    performanceThread.SetProcessCallback(onProcessCallback, nullptr);
    performanceThread.Play();
    performanceThread.EvalCode("i1 = 1\nreturn i1\n", onFirstBatchDone,
                               (void *) &performanceThread);
    performanceThread.FlushMessageQueue();

    // The process callback must run after the first batch and before the
    // message queued by its callback is processed.
    EXPECT_TRUE(firstBatchDone.load());
    EXPECT_TRUE(secondSawProcess.load());

    performanceThread.Stop();
    performanceThread.Join();
    // The performance thread has exited, so it can no longer read the
    // callback fields: clearing them here is race-free.
    performanceThread.SetProcessCallback(nullptr, nullptr);
    csound.Reset();
}

TEST(PerfThreadsTests, ProcessCallbackCanFlush) {
    const char *instrument =
        "sr=48000\n"
        "ksmps=64\n"
        "nchnls=1\n"
        "instr 1\n"
        "endin\n";

    processFlushFinished.store(false);
    processFlushCallbackRan.store(false);
    processFlushReturned.store(false);

    // Run the whole thing on a worker so a regression can be detected with a
    // timeout instead of hanging the test process forever.
    std::thread runner([instrument] {
        Csound csound;
        csound.SetOption("-n");
        if (csound.CompileOrc(instrument)) {
            processFlushFinished.store(true);
            return;
        }
        csound.EventString((char *) "i 1 0 3600\n");
        if (csound.Start()) {
            processFlushFinished.store(true);
            return;
        }
        CsoundPerformanceThread performanceThread(csound.GetCsound());
        performanceThread.SetProcessCallback(onProcessCallbackFlush,
                                             &performanceThread);
        performanceThread.Play();
        performanceThread.Join();
        csound.Reset();
        processFlushFinished.store(true);
    });

    for (int i = 0; i < 100 && !processFlushFinished.load(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (!processFlushFinished.load()) {
        // Leak the stuck worker: joining it would deadlock too. The process
        // exits when the test run finishes.
        runner.detach();
        FAIL() << "FlushMessageQueue() from the process callback deadlocked";
    }
    runner.join();

    EXPECT_TRUE(processFlushCallbackRan.load());
    EXPECT_TRUE(processFlushReturned.load());
}

TEST(PerfThreadsTests, ProcessCallbackJoinIsIgnored) {
    const char *instrument =
        "sr=48000\n"
        "ksmps=64\n"
        "nchnls=1\n"
        "instr 1\n"
        "endin\n";

    processJoinCalls.store(0);
    processJoinResult.store(-1);

    Csound csound;
    csound.SetOption("-n");
    csound.CreateMessageBuffer(0);
    ASSERT_EQ(csound.CompileOrc(instrument), 0);
    csound.EventString("f 0 z\ni 1 0 -1\n");
    ASSERT_EQ(csound.Start(), 0);

    CsoundPerformanceThread performanceThread(csound.GetCsound());
    performanceThread.SetProcessCallback(onProcessCallbackJoin,
                                         &performanceThread);
    performanceThread.Play();

    // The performance goes on after the call, through later cycles.
    for (int i = 0; i < 500 && processJoinCalls.load() < 10; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_GE(processJoinCalls.load(), 10);
    // Returned at once, with the status of a performance still playing.
    EXPECT_EQ(processJoinResult.load(), 0);

    performanceThread.Stop();
    EXPECT_GT(performanceThread.Join(), 0);
    performanceThread.SetProcessCallback(nullptr, nullptr);

    // Each refusal is said once, the first time.
    const char *flush = "FlushMessageQueue() called from the performance";
    const char *join = "Join() called from the performance thread";
    int flushes = 0, joins = 0;
    for (; csound.GetMessageCnt() > 0; csound.PopFirstMessage()) {
        flushes += strstr(csound.GetFirstMessage(), flush) != nullptr;
        joins += strstr(csound.GetFirstMessage(), join) != nullptr;
    }
    EXPECT_EQ(flushes, 1);
    EXPECT_EQ(joins, 1);
    csound.DestroyMessageBuffer();
    csound.Reset();
}

TEST(PerfThreadsTests, ProcessCallbackDestroyStopsIt) {
    const char *instrument =
        "sr=48000\n"
        "ksmps=64\n"
        "nchnls=1\n"
        "instr 1\n"
        "endin\n";

    processDestroyCalls.store(0);

    Csound csound;
    csound.SetOption("-n");
    ASSERT_EQ(csound.CompileOrc(instrument), 0);
    csound.EventString("f 0 z\ni 1 0 -1\n");
    ASSERT_EQ(csound.Start(), 0);

    CS_PERF_THREAD *pt = csoundCreatePerformanceThread(csound.GetCsound());
    csoundPerformanceThreadSetProcessCB(pt, onProcessCallbackDestroy, pt);
    csoundPerformanceThreadPlay(pt);

    // The call stops the performance, and leaves the object to this thread.
    for (int i = 0; i < 500 && csoundPerformanceThreadIsRunning(pt); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(csoundPerformanceThreadIsRunning(pt));
    csoundPerformanceThreadStop(pt);
    EXPECT_GT(csoundPerformanceThreadJoin(pt), 0);
    EXPECT_EQ(processDestroyCalls.load(), 1);
    csoundDestroyPerformanceThread(pt);
    csoundDestroyPerformanceThread(nullptr);  // as delete would: nothing
    csound.Reset();
}

TEST(PerfThreadsTests, SetProcessCallbackWhileRunning) {
    const char *instrument =
        "sr=48000\n"
        "ksmps=64\n"
        "nchnls=1\n"
        "instr 1\n"
        "endin\n";

    ProcessCallbackSignal initialCallback, finalCallback;

    Csound csound;
    csound.SetOption("-n");
    ASSERT_EQ(csound.CompileOrc(instrument), 0);
    // With -n, even an hour of score time can finish before the setter loop.
    // Keep the score and note alive until the test stops the thread.
    csound.EventString("f 0 z\ni 1 0 -1\n");
    ASSERT_EQ(csound.Start(), 0);

    CsoundPerformanceThread performanceThread(csound.GetCsound());
    performanceThread.SetProcessCallback(onSignalProcessCallback, &initialCallback);
    performanceThread.Play();

    // Wait until the process callback is actually being invoked.
    ASSERT_TRUE(initialCallback.wait());

    // Hammer the setter from this thread while the performance thread reads
    // the callback. This is the access pattern that used to be unsynchronized.
    for (int i = 0; i < 1000; ++i) {
        performanceThread.SetProcessCallback(
            (i & 1) ? onSignalProcessCallback : nullptr, &initialCallback);
        std::this_thread::yield();
    }

    // Fresh user data ensures an earlier in-flight callback cannot satisfy
    // this check. The last setter call must take effect.
    performanceThread.SetProcessCallback(onSignalProcessCallback, &finalCallback);
    EXPECT_TRUE(finalCallback.wait());

    performanceThread.Stop();
    performanceThread.Join();
    csound.Reset();
}

TEST(PerfThreadsTests, Record) {
    const char *instrument =
        "0dbfs = 1.0\n"
        "ksmps = 64\n"
        "instr 1 \n"
        "a1 linen p4,0.1, p3, 0.1   \n"
        "out  oscili(a1,p5)   \n"
        "endin \n";

    Csound csound;
    csound.SetOption("-odac");
    csound.SetOption("-W");
    csound.CompileOrc(instrument);
    csound.Start();
    csound.EventString((char*)"i 1 0 1 0.5 440");

    CsoundPerformanceThread performanceThread1(csound.GetCsound());
    performanceThread1.Play();
    performanceThread1.Record("");
    performanceThread1.StopRecord();
    performanceThread1.FlushMessageQueue();
    performanceThread1.Record("testrec.wav");
    csoundSleep(2000);
    performanceThread1.StopRecord();
    performanceThread1.Stop();
    performanceThread1.Join();
    csound.Reset();
}

TEST(PerfThreadsTests, RecordOnceThePerformanceHasEndedLeavesTheTake) {
    const char *take = "perfthread_record_take.wav";
    remove(take);
    long recorded = -1;
    {
        Csound csound;
        ASSERT_NO_FATAL_FAILURE(
            startRecordedScore(csound, "-operfthread_record_output.wav"));
        CsoundPerformanceThread performanceThread(csound.GetCsound());
        performanceThread.Record(take, 16, kRecordBuffers);
        performanceThread.Play();
        for (int i = 0; i < 1000 && performanceThread.GetStatus() == 0; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ASSERT_NE(performanceThread.GetStatus(), 0);
        EXPECT_GT(performanceThread.Join(), 0);
        recorded = fileSize(take);
        ASSERT_GE(recorded, kWholeTake);

        // Nothing would record into it: it must not open, and so empty, the
        // take, nor the destructor close it.
        performanceThread.Record(take, 16, kRecordBuffers);
        EXPECT_EQ(fileSize(take), recorded);
    }
    EXPECT_EQ(fileSize(take), recorded);
    remove(take);
    remove("perfthread_record_output.wav");
}

// The recording thread may not have run yet when the recording is stopped,
// all the more with every core busy: what the performance wrote stays in
// the take all the same. A race, so it is tried many times.
TEST(PerfThreadsTests, RecordingStoppedBeforeItsThreadRanKeepsTheTake) {
    const char *take = "perfthread_race_take.wav";
    std::atomic<bool> spin{true};
    std::vector<std::thread> busy;
    const unsigned cores = std::thread::hardware_concurrency();
    for (unsigned i = 0; i < (cores > 2 ? cores : 2); ++i)
        busy.emplace_back([&spin] { while (spin.load()) {} });
    int lost = 0;
    // A lambda, so that a failed assertion still stops the spinners below.
    auto takes = [&] {
        for (int i = 0; i < 40; ++i) {
            remove(take);
            Csound csound;
            ASSERT_NO_FATAL_FAILURE(
                startRecordedScore(csound, "-operfthread_race_output.wav"));
            CsoundPerformanceThread performanceThread(csound.GetCsound());
            performanceThread.Record(take, 16, kRecordBuffers);
            performanceThread.Play();
            for (int j = 0; j < 10000 && performanceThread.GetStatus() == 0; ++j)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            ASSERT_NE(performanceThread.GetStatus(), 0);
            ASSERT_GT(performanceThread.Join(), 0);
            lost += fileSize(take) < kWholeTake;
        }
    };
    takes();
    spin = false;
    for (auto &thread : busy)
        thread.join();
    EXPECT_EQ(lost, 0);
    remove(take);
    remove("perfthread_race_output.wav");
}

TEST(PerfThreadsTests, JoinRecordsThePerformanceToItsEnd) {
    const char *take = "perfthread_join_take.wav";
    remove(take);
    {
        Csound csound;
        ASSERT_NO_FATAL_FAILURE(
            startRecordedScore(csound, "-operfthread_join_output.wav"));
        CsoundPerformanceThread performanceThread(csound.GetCsound());
        performanceThread.Record(take, 16, kRecordBuffers);
        performanceThread.Play();
        EXPECT_GT(performanceThread.Join(), 0);
        EXPECT_GE(fileSize(take), kWholeTake);
    }
    remove(take);
    remove("perfthread_join_output.wav");
}

TEST(PerfThreadsTests, RecordingHoldsEachBlockOnce) {
    const char *take = "perfthread_blocks_take.wav";
    remove(take);
    {
        Csound csound;
        ASSERT_NO_FATAL_FAILURE(
            startRecordedScore(csound, "-operfthread_blocks_output.wav"));
        CsoundPerformanceThread performanceThread(csound.GetCsound());
        performanceThread.Record(take, 16, kRecordBuffers);
        performanceThread.Play();
        for (int i = 0; i < 1000 && performanceThread.GetStatus() == 0; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ASSERT_NE(performanceThread.GetStatus(), 0);
        EXPECT_GT(performanceThread.Join(), 0);
        // 75 blocks of 64 frames, not a 76th that repeats the last.
        EXPECT_EQ(recordedBytes(take), 2 * 4800);
    }
    remove(take);
    remove("perfthread_blocks_output.wav");
}

TEST(PerfThreadsTests, EachTakeFreesTheBufferOfTheLast) {
    const char *take = "perfthread_takes.wav";
    const int takes = 20;
    remove(take);
    Csound csound;
    csound.SetOption("-+rtaudio=null");
    csound.SetOption("-odac");
    csound.SetOption("-b256");
    ASSERT_EQ(csound.CompileOrc(kRecordedOrchestra), 0);
    csound.EventString("f 0 z\ni 1 0 -1\n");
    ASSERT_EQ(csound.Start(), 0);
    CSOUND *cs = csound.GetCsound();
    // Declared before the thread, which may still call back as it goes.
    std::atomic<int> cycles{0};
    CsoundPerformanceThread performanceThread(cs);
    // The callback runs before each k-cycle: its fifth call follows four.
    performanceThread.SetProcessCallback(
        [](void *data) { ++*static_cast<std::atomic<int> *>(data); }, &cycles);
    performanceThread.Play();
    // The held note's instance is made in the first k-cycles: count after them.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (cycles.load() < 5) {
        ASSERT_LT(std::chrono::steady_clock::now(), deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const size_t idle = csound_tracked_block_count(cs);
    performanceThread.Record(take, 16, kRecordBuffers);
    // Record() makes the take's buffer, two blocks, on this thread at once:
    // a take that was refused, or failed, leaves the count as it was.
    ASSERT_GE(csound_tracked_block_count(cs), idle + 2);
    performanceThread.StopRecord();
    performanceThread.FlushMessageQueue();
    const size_t before = csound_tracked_block_count(cs);
    for (int i = 1; i < takes; ++i) {
        performanceThread.Record(take, 16, kRecordBuffers);
        performanceThread.StopRecord();
        performanceThread.FlushMessageQueue();
    }
    // Each take frees the last take's buffer: the count is where it was.
    EXPECT_EQ(before, csound_tracked_block_count(cs));
    performanceThread.Stop();
    EXPECT_GT(performanceThread.Join(), 0);
    remove(take);
}

// A block of 1024 frames of 6 channels is more than the 4096 samples the
// recording thread reads at a time, and 4096 is no whole number of frames.
TEST(PerfThreadsTests, RecordingOfSixChannelsKeepsEveryFrame) {
    const char *take = "perfthread_channels_take.wav";
    remove(take);
    {
        Csound csound;
        csound.SetOption("-W");
        csound.SetOption("-b1024");
        csound.SetOption("-operfthread_channels_output.wav");
        ASSERT_EQ(csound.CompileOrc("sr=48000\nksmps=1024\nnchnls=6\n0dbfs=1\n"
                                    "instr 1\nendin\n"), 0);
        csound.EventString("i 1 0 0.1\ne 0.1\n");
        ASSERT_EQ(csound.Start(), 0);
        CsoundPerformanceThread performanceThread(csound.GetCsound());
        performanceThread.Record(take, 16, kRecordBuffers);
        performanceThread.Play();
        EXPECT_GT(performanceThread.Join(), 0);
        // 0.1 s is 5 blocks of 1024 frames, 6 samples of 2 bytes each.
        EXPECT_EQ(recordedBytes(take), 5 * 1024 * 6 * 2);
    }
    remove(take);
    remove("perfthread_channels_output.wav");
}
