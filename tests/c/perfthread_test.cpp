#if defined(WIN32) && !defined(__MINGW32__)
# include <Windows.h>
#else
# include "unistd.h"
#endif
#include <stdio.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "gtest/gtest.h"

#include "csound.hpp"
#include "csPerfThread.hpp"

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
