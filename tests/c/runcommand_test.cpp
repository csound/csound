#include "csound.h"
#include "csound_misc.h"
#include "gtest/gtest.h"

#if defined(CSOUND_TEST_NO_PROCESS_BACKEND)
TEST(RunCommandTests, UnsupportedBackendReturnsError)
{
    const char *command[] = {"/bin/sh", "-c", "exit 0", nullptr};
    EXPECT_LT(csoundRunCommand(command, 0), 0);
    EXPECT_LT(csoundRunCommand(command, 1), 0);
}
#elif GTEST_HAS_DEATH_TEST
#include <cstdlib>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

namespace {
volatile sig_atomic_t alarms = 0;

void recordAlarm(int) { ++alarms; }
void unexpectedCleanup() { _exit(42); }
}

TEST(RunCommandTests, RetriesInterruptedWaitsAndReportsWaitErrors)
{
    // Isolate signal dispositions and timers from the rest of the test suite.
    ASSERT_EXIT(([] {
        struct sigaction action = {};
        action.sa_handler = recordAlarm;
        sigemptyset(&action.sa_mask);
        if (sigaction(SIGALRM, &action, nullptr) != 0) _exit(1);
        struct itimerval timer = {};
        timer.it_value.tv_usec = timer.it_interval.tv_usec = 10000;
        if (setitimer(ITIMER_REAL, &timer, nullptr) != 0) _exit(2);
        const char *command[] = {"/bin/sh", "-c", "sleep 0.1; exit 7", nullptr};
        long result = csoundRunCommand(command, 0);
        timer = {};
        if (setitimer(ITIMER_REAL, &timer, nullptr) != 0) _exit(3);
        if (result != 7 || alarms == 0) _exit(4);

        // An auto-reaped child makes waitpid fail with ECHILD.
        action.sa_handler = SIG_IGN;
        if (sigaction(SIGCHLD, &action, nullptr) != 0) _exit(5);
        const char *done[] = {"/bin/sh", "-c", "exit 0", nullptr};
        _exit(csoundRunCommand(done, 0) < 0 ? 0 : 6);
    }()), ::testing::ExitedWithCode(0), "");
}

TEST(RunCommandTests, FailedExecDoesNotRunHostExitHandlers)
{
    ASSERT_EXIT(([] {
        if (std::atexit(unexpectedCleanup) != 0) _exit(1);
        const char *command[] = {"/csound-test-missing-directory/command", nullptr};
        _exit(csoundRunCommand(command, 0) == 255 ? 0 : 2);
    }()), ::testing::ExitedWithCode(0), "");
}
#endif
