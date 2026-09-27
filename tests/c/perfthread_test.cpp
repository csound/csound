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
    std::atomic<double> legacyValue{0.0};
    std::atomic<double> userdataValue{0.0};
    std::atomic<void *> userdataPtr{nullptr};
    std::atomic<int> nestedCalls{0};
    std::atomic<double> nestedValue{0.0};
};

EvalCodeResult evalCodeResult;

// Lets the slow EvalCode callback signal that it has started and then hold
// until the test releases it, instead of relying on sleeps.
std::mutex evalCodeSyncMutex;
std::condition_variable evalCodeSyncCv;
bool evalCodeCallbackEntered = false;
bool evalCodeCallbackRelease = false;

void onEvalCodeDone(MYFLT value) {
    evalCodeResult.legacyValue.store((double) value);
    evalCodeResult.legacyCalls.fetch_add(1);
}

void onEvalCodeDoneWithData(MYFLT value, void *userdata) {
    {
        std::unique_lock<std::mutex> lock(evalCodeSyncMutex);
        evalCodeCallbackEntered = true;
        evalCodeSyncCv.notify_all();
        evalCodeSyncCv.wait(lock, [] { return evalCodeCallbackRelease; });
    }
    evalCodeResult.userdataValue.store((double) value);
    evalCodeResult.userdataPtr.store(userdata);
    evalCodeResult.userdataCalls.fetch_add(1);
}

void onNestedEvalCodeDone(MYFLT value) {
    evalCodeResult.nestedValue.store((double) value);
    evalCodeResult.nestedCalls.fetch_add(1);
}

// Runs on the performance thread and queues another message on the same
// performance thread. Only queueing methods (EvalCode, Play, ScoreEvent, ...)
// may be called this way; FlushMessageQueue() and Join() would self-deadlock.
void onEvalCodeQueuesMoreWork(MYFLT value, void *userdata) {
    evalCodeResult.legacyValue.store((double) value);
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

void onSecondBatchDone(MYFLT value) {
    (void) value;
    secondSawProcess.store(processRanAfterFirst.load());
}

void onFirstBatchDone(MYFLT value, void *userdata) {
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

    performanceThread.SetProcessCallback(nullptr, nullptr);
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
