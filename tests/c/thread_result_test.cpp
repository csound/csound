#include "csound.h"
#include "csound_threads.h"
#include "gtest/gtest.h"
#include <cstdint>

#if !defined(BARE_METAL) && !defined(__wasi__) && !defined(__EMSCRIPTEN__)
#if defined(CSOUND_TEST_PTHREADS) || defined(WIN32) || !defined(__STDC_NO_THREADS__)
static uintptr_t returnValue(void *arg)
{
    return *static_cast<uintptr_t *>(arg);
}

TEST(ThreadResultTests, BothCreatorsPreservePointerSizedResults)
{
    uintptr_t values[] = {0, UINTPTR_MAX, UINTPTR_MAX / 3};
    for (uintptr_t &value : values) {
        void *thread = csoundCreateThread(returnValue, &value);
        ASSERT_NE(thread, nullptr);
        EXPECT_EQ(csoundJoinThread(thread), value);
        thread = csoundCreateThread2(returnValue, 2 * 1024 * 1024, &value);
        ASSERT_NE(thread, nullptr);
        EXPECT_EQ(csoundJoinThread(thread), value);
    }
}
#endif

#if defined(CSOUND_TEST_PTHREADS) && (defined(__APPLE__) || defined(__linux__))
#include <pthread.h>

static uintptr_t stackSize(void *)
{
#if defined(__APPLE__)
    return pthread_get_stacksize_np(pthread_self());
#else
    pthread_attr_t attr;
    size_t size = 0;
    if (pthread_getattr_np(pthread_self(), &attr) != 0) return 0;
    pthread_attr_getstacksize(&attr, &size);
    pthread_attr_destroy(&attr);
    return size;
#endif
}

TEST(ThreadResultTests, PthreadCreatorUsesRequestedStackSize)
{
    const uint32_t requested = 16 * 1024 * 1024;
    void *thread = csoundCreateThread2(stackSize, requested, nullptr);
    ASSERT_NE(thread, nullptr);
    EXPECT_GE(csoundJoinThread(thread), requested);
}
#endif
#endif
