#include "csound.h"
#include "csound_threads.h"
#include "gtest/gtest.h"
#include <future>
#include <thread>

#if !defined(BARE_METAL) && !defined(__wasi__) && !defined(__EMSCRIPTEN__)
#if defined(CSOUND_TEST_PTHREADS) || defined(WIN32) || !defined(__STDC_NO_THREADS__)
TEST(ThreadMonitorTests, NotificationsAreStoredAndConsumedOnce)
{
    void *monitor = csoundCreateThreadLock();
    ASSERT_NE(monitor, nullptr);
    EXPECT_EQ(csoundWaitThreadLock(monitor, 0), 0); // Initially signaled.
    EXPECT_NE(csoundWaitThreadLock(monitor, 0), 0);
    EXPECT_NE(csoundWaitThreadLock(monitor, 10), 0);

    csoundNotifyThreadLock(monitor);
    csoundNotifyThreadLock(monitor); // Multiple notifications coalesce.
    csoundWaitThreadLockNoTimeout(monitor);
    EXPECT_NE(csoundWaitThreadLock(monitor, 0), 0);
    csoundDestroyThreadLock(monitor);
}

TEST(ThreadMonitorTests, AnotherThreadCanNotifyAWaiter)
{
    void *monitor = csoundCreateThreadLock();
    ASSERT_NE(monitor, nullptr);
    EXPECT_EQ(csoundWaitThreadLock(monitor, 0), 0);
    std::promise<void> ready;
    auto started = ready.get_future();
    int result = -1;
    std::thread waiter([&] {
        ready.set_value();
        result = csoundWaitThreadLock(monitor, 2000);
    });
    started.wait();
    csoundNotifyThreadLock(monitor);
    waiter.join();
    EXPECT_EQ(result, 0);
    EXPECT_NE(csoundWaitThreadLock(monitor, 0), 0);
    csoundDestroyThreadLock(monitor);
}
TEST(ThreadMonitorTests, MutexTryLockUsesTheCsoundStatusConvention)
{
    void *mutex = csoundCreateMutex(0);
    ASSERT_NE(mutex, nullptr);
    EXPECT_EQ(csoundLockMutexNoWait(mutex), 0);
    csoundUnlockMutex(mutex);
    csoundDestroyMutex(mutex);
}

#endif
#endif
