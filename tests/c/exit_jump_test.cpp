#if defined(CSOUND_TEST_PTHREADS) && !defined(__EMSCRIPTEN__) && !defined(__wasi__)
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>

/* Under --realtime, init runs on a thread of its own. A fatal jump made there,
 * such as exitnow's, went to exitjmp, the frame of the thread that performs,
 * and the init thread went on running that thread's frames: the process
 * crashed. Each performance below must end as it does without --realtime.
 */

namespace {

const char *const header = "sr = 48000\nksmps = 16\nnchnls = 1\n0dbfs = 1\n";

/* Each case performs until its end, or for ten seconds. */
const auto patience = std::chrono::seconds(10);

/* On develop, the init thread went on running the other thread's frames:
 * the process crashed, and now and then hung. A case still running five
 * seconds after its patience ends the process, so it fails at once rather
 * than at ctest's timeout.
 */
class Watchdog {
 public:
  Watchdog() : watcher([this] {
    std::unique_lock<std::mutex> lock(mutex);
    if (!changed.wait_for(lock, patience + std::chrono::seconds(5),
                          [this] { return done; })) {
      std::fputs("exit_jump_test: the case hung\n", stderr);
      std::_Exit(1);
    }
  }) {}

  ~Watchdog()
  {
    {
      std::lock_guard<std::mutex> lock(mutex);
      done = true;
    }
    changed.notify_all();
    watcher.join();
  }

 private:
  std::mutex mutex;
  std::condition_variable changed;
  bool done = false;
  std::thread watcher;
};

class RealtimeInitJumpTests : public ::testing::TestWithParam<bool> {
 protected:
  Watchdog watchdog;
  CSOUND *csound = nullptr;

  void SetUp() override
  {
    if (GetParam()) {
#if !CSOUND_SPINLOCK_AVAILABLE || \
    (!defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN))
      GTEST_SKIP() << "Realtime mode requires native spinlocks and atomics";
#endif
    }
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), 0);
    if (GetParam())
      ASSERT_EQ(csoundSetOption(csound, "--realtime"), 0);
  }

  void TearDown() override
  {
    if (csound != nullptr)
      csoundDestroy(csound);
  }

  /* With no score, the performance ends only by a jump. Under --realtime,
     the init thread may run a note's init many k-cycles late, as -n
     performs as fast as it can. */
  void start(const char *orchestra, const char *event)
  {
    std::string code = std::string(header) + orchestra;
    ASSERT_EQ(csoundCompileOrc(csound, code.c_str(), 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    if (event != nullptr)
      csoundEventString(csound, event, 0);
  }

  /* What a k-cycle returns that is not 0. Or 0: after a thousand k-cycles
     more once the channel named is 1, or after patience. A jump returns to
     the place of csoundPerformKsmps's first call, so a test calls this once.
   */
  int32_t perform(const char *until = nullptr)
  {
    const auto deadline = std::chrono::steady_clock::now() + patience;
    int32_t more = -1;
    while (more != 0 && std::chrono::steady_clock::now() < deadline) {
      int32_t result = csoundPerformKsmps(csound);
      if (result != 0)
        return result;
      if (more > 0)
        more--;
      else if (until != nullptr &&
               csoundGetControlChannel(csound, until, nullptr) == 1)
        more = 1000;
    }
    return 0;
  }

  /* Without --realtime, true. Under it, whether the init thread, which
     takes the init mutex for each piece of work, leaves it free within a
     second: a jump out of a nested pass left a level held for ever. */
  bool initMutexFree()
  {
    if (!GetParam())
      return true;
    const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
    do {
      if (csoundLockMutexNoWait(csound->init_pass_threadlock) == 0) {
        csoundUnlockMutex(csound->init_pass_threadlock);
        return true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
  }
};

/* exitnow 3 gives -3. */
TEST_P(RealtimeInitJumpTests, ANoteInitEndsThePerformance)
{
  start("instr 1\n exitnow 3\nendin\n", "i1 0 -1");
  EXPECT_EQ(perform(), -3);
  EXPECT_TRUE(initMutexFree());
}

/* After the jump, the init thread runs no more init: the performance is
 * over. Here it still merges what is compiled, and its global code runs
 * nothing.
 */
TEST_P(RealtimeInitJumpTests, NoInitRunsAfterTheJump)
{
  start("instr 1\n exitnow 3\nendin\n", "i1 0 -1");
  EXPECT_EQ(perform(), -3);
  ASSERT_EQ(csoundCompileOrc(csound, "chnset 1, \"after\"\n", 1), 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_EQ(csoundGetControlChannel(csound, "after", nullptr), 0);
}

/* init runs Child's init pass inside the note's, and gives Child's errors
 * to its output. The note's pass ends there too, as without --realtime.
 */
TEST_P(RealtimeInitJumpTests, AnInstanceInitEndsThePerformance)
{
  start("instr Child\n exitnow 3\nendin\n"
        "instr 1\n"
        " child:Instr = create(Child)\n"
        " iErr = init(child)\n"
        " chnset 1, \"after\"\n"
        "endin\n", "i1 0 -1");
  EXPECT_EQ(perform(), -3);
  EXPECT_EQ(csoundGetControlChannel(csound, "after", nullptr), 0);
  EXPECT_TRUE(initMutexFree());
}

/* The same inside a UDO, whose body runs its init opcodes itself. */
TEST_P(RealtimeInitJumpTests, AnInstanceInitInAUdoEndsThePerformance)
{
  start("instr Child\n exitnow 3\nendin\n"
        "opcode Run():()\n"
        " child:Instr = create(Child)\n"
        " iErr = init(child)\n"
        " chnset 1, \"after\"\n"
        "endop\n"
        "instr 1\n Run()\nendin\n", "i1 0 -1");
  EXPECT_EQ(perform(), -3);
  EXPECT_EQ(csoundGetControlChannel(csound, "after", nullptr), 0);
  EXPECT_TRUE(initMutexFree());
}

TEST_P(RealtimeInitJumpTests, AReinitEndsThePerformance)
{
  start("giPasses init 0\n"
        "instr 1\n"
        " kCycle init 0\n"
        " kCycle += 1\n"
        " if kCycle == 2 then\n"
        "  reinit again\n"
        " endif\n"
        "again:\n"
        " iPass = giPasses\n"
        " giPasses += 1\n"
        " if iPass == 1 then\n"
        "  exitnow 3\n"
        " endif\n"
        " rireturn\n"
        "endin\n", "i1 0 -1");
  EXPECT_EQ(perform(), -3);
  EXPECT_TRUE(initMutexFree());
}

/* Under --realtime the init thread merges this and runs its global code. */
TEST_P(RealtimeInitJumpTests, GlobalCodeCompiledLaterEndsThePerformance)
{
  start("instr 1\nendin\n", nullptr);
  ASSERT_EQ(csoundCompileOrc(csound, "exitnow 3\n", 1), 0);
  EXPECT_EQ(perform(), -3);
  EXPECT_TRUE(initMutexFree());
}

/* A jump inside compilestr's compile, here a missing array, is that call's
 * own: compilestr returns an error, and the note and the performance go on.
 */
TEST_P(RealtimeInitJumpTests, ACompileErrorInCompilestrIsCompilestrs)
{
  if (!GetParam())
    GTEST_SKIP() << "without --realtime this crashes, a defect of its own";
  start("instr 1\n"
        " iResult = compilestr(\"instr 2\\n kMissing[0] = 1\\nendin\\n\")\n"
        " chnset iResult, \"result\"\n"
        " chnset 1, \"after\"\n"
        "endin\n", "i1 0 -1");
  EXPECT_EQ(perform("after"), 0);
  EXPECT_NE(csoundGetControlChannel(csound, "result", nullptr), 0);
  EXPECT_EQ(csoundGetControlChannel(csound, "after", nullptr), 1);
}

INSTANTIATE_TEST_SUITE_P(Modes, RealtimeInitJumpTests, ::testing::Bool(),
                         [](const ::testing::TestParamInfo<bool> &info) {
                           return info.param ? "Realtime" : "Default";
                         });

} // namespace
#endif
