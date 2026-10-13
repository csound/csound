#include "csound.h"
#include "csound_threads.h"
#include "gtest/gtest.h"
#include <cstdlib>

#if !defined(BARE_METAL) && !defined(__wasi__) && !defined(__EMSCRIPTEN__)
#if defined(CSOUND_TEST_PTHREADS) || defined(WIN32) || !defined(__STDC_NO_THREADS__)
TEST(ThreadIdTests, CallerOwnsEachReturnedId)
{
    void *first = csoundGetCurrentThreadId();
    ASSERT_NE(first, nullptr);
    void *second = csoundGetCurrentThreadId();
    ASSERT_NE(second, nullptr);
    // Each result must be independently freeable, even on the same thread.
    ASSERT_NE(first, second);
    std::free(first);
    std::free(second);

    void *again = csoundGetCurrentThreadId();
    ASSERT_NE(again, nullptr);
    std::free(again);
}
#endif
#endif
