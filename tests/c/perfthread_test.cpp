#if defined(WIN32) && !defined(__MINGW32__)
# include <Windows.h>
#else
# include "unistd.h"
#endif
#include <stdio.h>
#include <atomic>
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

void onEvalCodeDone(MYFLT value) {
    evalCodeResult.legacyValue.store((double) value);
    evalCodeResult.legacyCalls.fetch_add(1);
}

void onEvalCodeDoneWithData(MYFLT value, void *userdata) {
    csoundSleep(100);
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

    CsoundPerformanceThread performanceThread(csound.GetCsound());
    performanceThread.Play();

    int marker = 0;
    performanceThread.EvalCode("i1 = 2 + 2\nreturn i1\n", onEvalCodeDone);
    performanceThread.EvalCode("i1 = 3 + 4\nreturn i1\n",
                               onEvalCodeDoneWithData, (void *) &marker);

    // Give the performance thread time to start running the (slow) second
    // callback; FlushMessageQueue() must then wait for it to finish even
    // though the message is no longer in the queue.
    csoundSleep(20);
    performanceThread.FlushMessageQueue();

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
