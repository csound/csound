#if defined(CSOUND_TEST_PTHREADS) && !defined(__EMSCRIPTEN__) && !defined(__wasi__)
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

/* Under --realtime, init runs on a thread of its own. A fatal jump made there,
 * such as exitnow's, went to exitjmp, the frame of the thread that performs,
 * and the init thread went on running that thread's frames: the process
 * crashed. Each performance below must end as it does without --realtime.
 */

// Android's OpenSL ES backend performs through this.
extern "C" int32_t perform_buffer(CSOUND *csound);

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
     the place of csoundPerformKsmps's first call, so a test calls this once,
     and does what comes between the first k-cycle and the next ones in
     afterFirst.
   */
  int32_t perform(const char *until = nullptr,
                  const std::function<void()> &afterFirst = nullptr)
  {
    const auto deadline = std::chrono::steady_clock::now() + patience;
    int32_t more = -1;
    bool first = true;
    while (more != 0 && std::chrono::steady_clock::now() < deadline) {
      int32_t result = csoundPerformKsmps(csound);
      if (result != 0)
        return result;
      if (first && afterFirst)
        afterFirst();
      first = false;
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

  /* Whether API_lock is free, from another thread: the thread that performs
     takes it recursively, without --realtime. */
  bool apiLockFree()
  {
    bool free = false;
    std::thread([this, &free] {
      free = csoundLockMutexNoWait(csound->API_lock) == 0;
      if (free)
        csoundUnlockMutex(csound->API_lock);
    }).join();
    return free;
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
 * Without --realtime, the jump cleared the note's init context, and its init
 * pass went on through a NULL opcode.
 */
TEST_P(RealtimeInitJumpTests, ACompileErrorInCompilestrIsCompilestrs)
{
  start("instr 1\n"
        " iResult = compilestr(\"instr 2\\n kMissing[0] = 1\\nendin\\n\")\n"
        " chnset iResult, \"result\"\n"
        " chnset 1, \"after\"\n"
        "endin\n", "i1 0 -1");
  EXPECT_EQ(perform("after"), 0);
  EXPECT_NE(csoundGetControlChannel(csound, "result", nullptr), 0);
  EXPECT_EQ(csoundGetControlChannel(csound, "after", nullptr), 1);
}

/* The same for compilecsd, with a CSD that asks for a Csound before 1.0. */
TEST_P(RealtimeInitJumpTests, AnErrorInCompilecsdIsCompilecsds)
{
  const std::string csd = (std::filesystem::temp_directory_path() /
                           ("exit_jump_test_" +
                            std::to_string(GetParam()) + ".csd")).generic_string();
  std::ofstream(csd) << "<CsoundSynthesizer>\n<CsVersion>\nBefore 1.0\n"
                        "</CsVersion>\n<CsInstruments>\ninstr 2\nendin\n"
                        "</CsInstruments>\n</CsoundSynthesizer>\n";
  start(("instr 1\n"
         " iResult = compilecsd(\"" + csd + "\")\n"
         " chnset iResult, \"result\"\n"
         " chnset 1, \"after\"\n"
         "endin\n").c_str(), "i1 0 -1");
  EXPECT_EQ(perform("after"), 0);
  std::remove(csd.c_str());
  EXPECT_NE(csoundGetControlChannel(csound, "result", nullptr), 0);
  EXPECT_EQ(csoundGetControlChannel(csound, "after", nullptr), 1);
}

/* A jump from the global code that compilestr compiles is compilestr's too.
 * It left the lock its merge held, API_lock or the init mutex, and the note
 * went on from the global code's opcodes, or from NULL.
 */
TEST_P(RealtimeInitJumpTests, AJumpInCompiledGlobalCodeIsCompilestrs)
{
  start("instr 1\n"
        " iResult = compilestr(\"exitnow 3\\n\")\n"
        " chnset iResult, \"result\"\n"
        " chnset 1, \"after\"\n"
        "endin\n", "i1 0 -1");
  EXPECT_EQ(perform("after"), 0);
  EXPECT_EQ(csoundGetControlChannel(csound, "result", nullptr),
            CSOUND_EXITJMP_SUCCESS - 3);
  EXPECT_EQ(csoundGetControlChannel(csound, "after", nullptr), 1);
  EXPECT_TRUE(apiLockFree());
  EXPECT_TRUE(initMutexFree());
}

/* And the pass that called compilestr still takes a jump of its own: under
 * --realtime, the compile set the init thread's landing aside, so the passes
 * inside it left none behind, where a second compile would overwrite it.
 */
TEST_P(RealtimeInitJumpTests, AJumpAfterCompiledGlobalCodeEndsThePerformance)
{
  start("instr 1\n"
        " iResult = compilestr(\"exitnow 3\\n\")\n"
        " iResult = compilestr(\"giValue init 1\\n\")\n"
        " exitnow 4\n"
        "endin\n", "i1 0 -1");
  EXPECT_EQ(perform(), -4);
  EXPECT_TRUE(initMutexFree());
}

/* csoundCompileTree, from the host, takes the jumps of the global code it
 * runs as csoundCompileOrc does. It had no frame of its own, and the jump
 * went to whatever exitjmp held.
 */
TEST_P(RealtimeInitJumpTests, ACompiledTreeTakesItsJumps)
{
  start("instr 1\n chnset 1, \"after\"\nendin\n", "i1 0 -1");
  TREE *tree = csoundParseOrc(csound, "exitnow 3\n");
  ASSERT_NE(tree, nullptr);
  /* exitnow 3's jump: CSOUND_EXITJMP_SUCCESS - 3 */
  EXPECT_EQ(csoundCompileTree(csound, tree, 0), CSOUND_EXITJMP_SUCCESS - 3);
  csoundDeleteTree(csound, tree);
  EXPECT_EQ(perform("after"), 0);
  EXPECT_EQ(csoundGetControlChannel(csound, "after", nullptr), 1);
  EXPECT_TRUE(apiLockFree());
  EXPECT_TRUE(initMutexFree());
}

/* A jump from a UDO's body, in global code compiled after the start, lands
 * in the UDO's own init, which ends as after an init error: its recursion
 * count comes back down, and the next call is not refused. It skipped the
 * UDO's end, and with --recursion-depth=1 the next call was refused.
 */
TEST_P(RealtimeInitJumpTests, AUdoThatJumpsInCompiledGlobalCodeEndsItsInit)
{
  ASSERT_EQ(csoundSetOption(csound, "--recursion-depth=1"), 0);
  start("opcode Maybe, 0, i\n"
        " iQuit xin\n"
        " if iQuit == 1 then\n"
        "  exitnow 3\n"
        " endif\n"
        "endop\n"
        "instr 1\n"
        " Maybe 0\n"
        " chnset 1, \"after\"\n"
        "endin\n", nullptr);
  EXPECT_EQ(csoundCompileOrc(csound, "Maybe 1\n", 0),
            CSOUND_EXITJMP_SUCCESS - 3);
  csoundEventString(csound, "i1 0 -1", 0);
  EXPECT_EQ(perform("after"), 0);
  EXPECT_EQ(csoundGetControlChannel(csound, "after", nullptr), 1);
  EXPECT_TRUE(apiLockFree());
  EXPECT_TRUE(initMutexFree());
}

/* The same for subinstr: its instrument's init ends, so its instance can be
 * taken back. Its init stayed open for good.
 */
TEST_P(RealtimeInitJumpTests, ASubinstrThatJumpsInCompiledGlobalCodeEndsItsInit)
{
  start("instr 2\n exitnow 3\nendin\n", nullptr);
  EXPECT_EQ(csoundCompileOrc(csound, "subinstrinit 2\n", 0),
            CSOUND_EXITJMP_SUCCESS - 3);
  INSDS *child = csound->engineState.instrtxtp[2]->instance;
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(ATOMIC_GET(child->init_running), 0);
  EXPECT_TRUE(apiLockFree());
  EXPECT_TRUE(initMutexFree());
}

/* A host's csoundCompileOrc on a thread of its own, held in its parse: the
 * first allocation it makes through the instance's table waits until the
 * test lets it go, or for patience.
 */
std::atomic<std::thread::id> heldThread;
std::atomic<int> heldStep{0};          /* 1: held, 2: let go */
void *(*callocOfCsound)(CSOUND *, size_t);

void *holdTheCompile(CSOUND *csound, size_t size)
{
  if (std::this_thread::get_id() == heldThread.load()) {
    heldThread = std::thread::id();
    heldStep = 1;
    const auto deadline = std::chrono::steady_clock::now() + patience;
    while (heldStep.load() < 2 && std::chrono::steady_clock::now() < deadline)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return callocOfCsound(csound, size);
}

class HeldCompile {
 public:
  HeldCompile(CSOUND *csound, const char *code) : csound(csound)
  {
    heldStep = 0;
    callocOfCsound = csound->Calloc;
    csound->Calloc = holdTheCompile;
    std::atomic<bool> named{false};
    compiler = std::thread([this, code, &named] {
      heldThread = std::this_thread::get_id();
      named = true;
      result = csoundCompileOrc(this->csound, code, 0);
    });
    while (!named.load()) {
    }
  }

  ~HeldCompile()
  {
    if (compiler.joinable())
      join();
  }

  /* Whether the compile is held, within two seconds: it takes the first
     allocation of its parse at once. */
  bool held()
  {
    const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (heldStep.load() < 1 && std::chrono::steady_clock::now() < deadline)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return heldStep.load() == 1;
  }

  /* Lets it go, and gives what it returned. */
  int32_t join()
  {
    heldStep = 2;
    compiler.join();
    csound->Calloc = callocOfCsound;
    return result;
  }

 private:
  CSOUND *csound;
  std::thread compiler;
  int32_t result = -1;
};

/* A jump in the performance while a host compiles is the performance's. The
 * compile set exitjmp to a frame of its own for its length, on its thread,
 * and the jump went there: the thread that performed went on in the
 * compile's frames.
 */
TEST_P(RealtimeInitJumpTests, AJumpWhileAHostCompilesIsThePerformances)
{
  start("instr 1\n exitnowk 3\nendin\n", nullptr);
  std::unique_ptr<HeldCompile> compile;
  bool held = false;
  EXPECT_EQ(perform(nullptr, [&] {
    compile.reset(new HeldCompile(csound, "instr 2\nendin\n"));
    held = compile->held();
    csoundEventString(csound, "i1 0 -1", 0);
  }), -3);
  ASSERT_NE(compile, nullptr);
  EXPECT_TRUE(held);
  EXPECT_EQ(compile->join(), 0);
}

/* A host's compile that begins before the first k-cycle and ends after it
 * leaves the performance its landing. It put back the exitjmp it had found,
 * csoundStart's, whose frame was gone, over the one the first k-cycle had
 * set, and the performance's jump went there.
 */
TEST_P(RealtimeInitJumpTests, AHostsCompileLeavesThePerformanceItsLanding)
{
  start("instr 1\n exitnowk 3\nendin\n", nullptr);
  HeldCompile compile(csound, "instr 2\nendin\n");
  ASSERT_TRUE(compile.held());
  EXPECT_EQ(perform(nullptr, [&] {
    EXPECT_EQ(compile.join(), 0);
    csoundEventString(csound, "i1 0 -1", 0);
  }), -3);
}

INSTANTIATE_TEST_SUITE_P(Modes, RealtimeInitJumpTests, ::testing::Bool(),
                         [](const ::testing::TestParamInfo<bool> &info) {
                           return info.param ? "Realtime" : "Default";
                         });

/* Without --realtime, a performance holds API_lock through its k-cycles. A
 * jump that ended it returned with the lock held, and the thread that had
 * performed kept it: a CsoundPerformanceThread then ended holding it, and a
 * csoundDestroy from any other thread waited for ever.
 */
class ExitJumpApiLockTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), 0);
    std::string code = std::string(header) + "instr 1\n exitnow 3\nendin\n";
    ASSERT_EQ(csoundCompileOrc(csound, code.c_str(), 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    csoundEventString(csound, "i1 0 -1", 0);
  }

  void TearDown() override
  {
    if (csound != nullptr)
      csoundDestroy(csound);
  }

  /* Performs on a thread of its own until the jump, and says whether
     API_lock was still held then, from this thread. That thread gives back
     what it kept before it ends, so Csound can be destroyed either way. */
  bool performUntilTheJump(int32_t (*performCall)(CSOUND *), int32_t &result)
  {
    std::mutex mutex;
    std::condition_variable changed;
    int32_t stage = 0;
    bool held = false;
    std::thread performer([&] {
      const auto deadline = std::chrono::steady_clock::now() + patience;
      while (result == 0 && std::chrono::steady_clock::now() < deadline)
        result = performCall(csound);
      std::unique_lock<std::mutex> lock(mutex);
      stage = 1;
      changed.notify_all();
      changed.wait(lock, [&] { return stage == 2; });
      if (held)
        csoundUnlockMutex(csound->API_lock);
    });
    {
      std::unique_lock<std::mutex> lock(mutex);
      changed.wait(lock, [&] { return stage == 1; });
      held = csoundLockMutexNoWait(csound->API_lock) != 0;
      if (!held)
        csoundUnlockMutex(csound->API_lock);
      stage = 2;
      changed.notify_all();
    }
    performer.join();
    return held;
  }
};

TEST_F(ExitJumpApiLockTests, PerformKsmps)
{
  int32_t result = 0;
  EXPECT_FALSE(performUntilTheJump(csoundPerformKsmps, result));
  EXPECT_EQ(result, -3);
}

TEST_F(ExitJumpApiLockTests, PerformBuffer)
{
  int32_t result = 0;
  EXPECT_FALSE(performUntilTheJump(perform_buffer, result));
  EXPECT_EQ(result, -3);
}

/* A utility the host adds, which calls the API as a utility may, run by
 * csoundRunUtility(), or by csoundCompile() given -U. A jump in code it
 * evaluates is the run's, as one of its own is: an init pass inside the run
 * took it, the utility went on, and the run gave 0. Each call it makes
 * takes the jumps made inside it, as when the host makes it, and leaves the
 * landing of the call around it as it found it: a call that set exitjmp
 * set it inside the utility, to a frame gone once the utility returned, and
 * the jump that ends csoundCompile() after -U went there; and such a call's
 * jumps went to the run's landing, so the utility stopped.
 */
enum class Nested { EvalCode, SetOption, ScoreSort, Compile, Start, Perform };

std::atomic<bool> utilityWentOn{false};
std::atomic<int32_t> nestedResult{0};
Nested nestedCall;

int32_t nestedUtility(CSOUND *csound, int32_t, char **)
{
  int32_t result = 0;
  switch (nestedCall) {
  case Nested::EvalCode:
    csoundEvalCode(csound, "exitnow 3\n");
    break;
  case Nested::SetOption:
    result = csoundSetOption(csound, "-d");
    break;
  case Nested::ScoreSort: {
    FILE *in = std::tmpfile(), *out = std::tmpfile();
    std::fputs("#include \"exit_jump_test_no_such_score\"\ne\n", in);
    std::rewind(in);
    result = csoundScoreSort(csound, in, out);
    std::fclose(in);
    std::fclose(out);
    break;
  }
  case Nested::Compile: {
    const char *argv[] = {"csound", "--exit-jump-test-no-such-option"};
    result = csoundCompile(csound, 2, argv);
    break;
  }
  case Nested::Start:
    csoundCompileOrc(csound, "exitnow 3\n", 0);
    result = csoundStart(csound);
    break;
  case Nested::Perform: {
    csoundCompileOrc(csound, (std::string(header) +
                              "instr 1\n exitnowk 3\nendin\n").c_str(), 0);
    if (csoundStart(csound) != 0)
      return 1;
    csoundEventString(csound, "i1 0 -1", 0);
    for (int i = 0; i < 10000 && result == 0; i++)
      result = csoundPerformKsmps(csound);
    break;
  }
  }
  nestedResult = result;
  utilityWentOn = true;
  return 0;
}

class NestedCallTests : public ::testing::TestWithParam<Nested> {
 protected:
  Watchdog watchdog;
  CSOUND *csound = nullptr;

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), 0);
    ASSERT_EQ(csoundAddUtility(csound, "exit_jump_test_utility",
                               nestedUtility), 0);
    nestedCall = GetParam();
    utilityWentOn = false;
    nestedResult = 0;
  }

  void TearDown() override
  {
    if (csound != nullptr)
      csoundDestroy(csound);
  }
};

TEST_P(NestedCallTests, TakesItsJumpsAndLeavesTheOuterLanding)
{
  if (GetParam() == Nested::SetOption) {
    /* -U: once the utility returns, csoundCompile() ends by a jump to its
       own frame, which the utility's csoundSetOption() replaced by its own */
    const char *argv[] = {"csound", "-U", "exit_jump_test_utility"};
    EXPECT_EQ(csoundCompile(csound, 3, argv), CSOUND_EXITJMP_SUCCESS);
    EXPECT_TRUE(utilityWentOn);
    EXPECT_EQ(nestedResult, 0);
    return;
  }
  const int32_t result =
    csoundRunUtility(csound, "exit_jump_test_utility", 0, nullptr);
  if (GetParam() == Nested::EvalCode) {
    /* the jump in the code it evaluated before the start is the utility's,
       as a jump of its own is: it stops there, and its run gives the jump */
    EXPECT_EQ(result, -3);
    EXPECT_FALSE(utilityWentOn);
    return;
  }
  /* the call it made took the jump, and the utility went on */
  EXPECT_EQ(result, 0);
  EXPECT_TRUE(utilityWentOn);
  if (GetParam() == Nested::Perform)
    EXPECT_EQ(nestedResult, -3);
  else
    EXPECT_NE(nestedResult, 0);
}

std::string nestedName(const ::testing::TestParamInfo<Nested> &info)
{
  static const char *const names[] = {"EvalCode", "SetOption", "ScoreSort",
                                      "Compile", "Start", "Perform"};
  return names[static_cast<int>(info.param)];
}

/* csoundScoreSort() and csoundScoreExtract(), which a started engine does
 * not refuse, leave exitjmp as they found it, a refused score too: they set
 * it to a frame of their own, gone once they returned, under a performance
 * that holds it.
 */
TEST(ScoreUtilityJumpTests, LeaveExitjmpAsTheyFoundIt)
{
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  ASSERT_NE(csound, nullptr);
  ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), 0);
  jmp_buf found;
  std::memcpy(found, csound->exitjmp, sizeof(jmp_buf));
  for (int extract = 0; extract < 2; extract++) {
    FILE *in = std::tmpfile(), *out = std::tmpfile(), *x = std::tmpfile();
    std::fputs("#include \"exit_jump_test_no_such_score\"\ne\n", in);
    std::rewind(in);
    std::fputs("i1\n", x);
    std::rewind(x);
    EXPECT_NE(extract ? csoundScoreExtract(csound, in, out, x)
                      : csoundScoreSort(csound, in, out), 0);
    EXPECT_EQ(std::memcmp(found, csound->exitjmp, sizeof(jmp_buf)), 0)
      << (extract ? "csoundScoreExtract" : "csoundScoreSort");
    std::fclose(in);
    std::fclose(out);
    std::fclose(x);
  }
  csoundDestroy(csound);
}

INSTANTIATE_TEST_SUITE_P(Calls, NestedCallTests,
                         ::testing::Values(Nested::EvalCode, Nested::SetOption,
                                           Nested::ScoreSort, Nested::Compile,
                                           Nested::Start, Nested::Perform),
                         nestedName);

} // namespace
#endif
