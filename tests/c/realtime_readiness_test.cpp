#if defined(CSOUND_TEST_PTHREADS) && !defined(__EMSCRIPTEN__) && !defined(__wasi__)
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
extern "C" {
#include "udo.h"
}

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace {
class RealtimeReadinessTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;
  std::mutex gateMutex;
  std::condition_variable gateChanged;
  bool entered = false, released = false;

  static int32_t holdInit(CSOUND *csound, void *)
  {
    auto &self = *static_cast<RealtimeReadinessTests *>(csoundGetHostData(csound));
    std::unique_lock<std::mutex> lock(self.gateMutex);
    self.entered = true;
    self.gateChanged.notify_all();
    return self.gateChanged.wait_for(lock, std::chrono::seconds(10),
                                    [&self] { return self.released; }) ? 0 : -1;
  }

  void SetUp() override
  {
#if !CSOUND_SPINLOCK_AVAILABLE || \
    (!defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN))
    GTEST_SKIP() << "Realtime mode requires native spinlocks and atomics";
#endif
    csound = csoundCreate(this, nullptr);
    ASSERT_NE(csound, nullptr);
    ASSERT_EQ(csoundSetOption(csound, "-n -d -m0 --daemon --realtime"), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_readiness_wait", sizeof(OPDS), 0,
                                "", "", holdInit, nullptr, nullptr), 0);
    ASSERT_EQ(csoundCompileOrc(csound, R"(
      sr = 48000
      ksmps = 16
      nchnls = 1
      0dbfs = 1
      instr 8
        asig init 0.25
        out asig
      endin
      instr 9
        test_readiness_wait
      endin
    )", 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    csoundEventString(csound, "i9 0 0", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    std::unique_lock<std::mutex> lock(gateMutex);
    ASSERT_TRUE(gateChanged.wait_for(lock, std::chrono::seconds(5),
                                    [this] { return entered; }));
  }

  void release()
  {
    std::lock_guard<std::mutex> lock(gateMutex);
    released = true;
    gateChanged.notify_all();
  }

  void TearDown() override
  {
    release();
    if (csound != nullptr)
      csoundDestroy(csound);
  }

  bool drain()
  {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (alloc_queue_has_pending(csound)) {
      if (std::chrono::steady_clock::now() > deadline)
        return false;
      csoundSleep(1);
    }
    return true;
  }
};

TEST_F(RealtimeReadinessTests, ExpiredShortNoteIsNotPublishedReady)
{
  csoundEventString(csound, "i8 0 0.000001", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  release();
  ASSERT_TRUE(drain());
  INSDS *note = csound->engineState.instrtxtp[8]->instance;
  ASSERT_NE(note, nullptr);
  EXPECT_EQ(ATOMIC_GET8(note->actflg), 0);
  EXPECT_EQ(ATOMIC_GET(note->init_done), 0);
}

/* Holds a UDO's reinit pass on the init thread, and only that pass. */
struct ReinitGate {
  std::mutex mutex;
  std::condition_variable changed;
  bool entered = false, released = false;

  void release()
  {
    std::lock_guard<std::mutex> lock(mutex);
    released = true;
    changed.notify_all();
  }
};

int32_t holdReinit(CSOUND *csound, void *p)
{
  if (!static_cast<OPDS *>(p)->insdshead->reinitflag)
    return 0;
  auto &gate = *static_cast<ReinitGate *>(csoundGetHostData(csound));
  std::unique_lock<std::mutex> lock(gate.mutex);
  gate.entered = true;
  gate.changed.notify_all();
  return gate.changed.wait_for(lock, std::chrono::seconds(10),
                               [&gate] { return gate.released; }) ? 0 : -1;
}

/* A note can end while a reinit inside its UDO waits on the init thread. Its
   deactivation freed the UDO's converters then, and the reinit pass went on
   to write the converter arrays: a heap-use-after-free under ASan. */
TEST(RealtimeUdoTurnoff, PendingReinitKeepsTheUdoBuffers)
{
#if !CSOUND_SPINLOCK_AVAILABLE || \
    (!defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN))
  GTEST_SKIP() << "Realtime mode requires native spinlocks and atomics";
#endif
  ReinitGate gate;
  CSOUND *csound = csoundCreate(&gate, nullptr);
  ASSERT_NE(csound, nullptr);
  /* Whatever fails, the init thread is let go before Csound is destroyed. */
  struct Done {
    ReinitGate &gate;
    CSOUND *csound;
    ~Done() { gate.release(); csoundDestroy(csound); }
  } done{gate, csound};
  ASSERT_EQ(csoundSetOption(csound, "-n -d -m0 --daemon --realtime"), 0);
  ASSERT_EQ(csoundAppendOpcode(csound, "test_reinit_wait", sizeof(OPDS), 0,
                               "", "", holdReinit, nullptr, nullptr), 0);
  ASSERT_EQ(csoundCompileOrc(csound, R"(
    sr = 48000
    ksmps = 16
    nchnls = 1
    0dbfs = 1
    opcode Again, a, a
      oversample 2, 4
      aIn xin
      kCycle timeinstk
      if kCycle == 4 then
        reinit Out
      endif
    Out:
      test_reinit_wait
      xout aIn * 0.5
      rireturn
    endop
    instr 1
      aSig init 0.25
      out Again(aSig)
      if chnget:k("stop") == 1 then
        turnoff
      endif
    endin
  )", 0), 0);
  ASSERT_EQ(csoundStart(csound), 0);
  csoundEventString(csound, "i1 0 10", 0);

  /* Run until the UDO's reinit waits in the gate. */
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  for (;;) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    std::lock_guard<std::mutex> lock(gate.mutex);
    if (gate.entered)
      break;
    ASSERT_LT(std::chrono::steady_clock::now(), deadline);
  }
  INSDS *note = csound->engineState.instrtxtp[1]->instance;
  ASSERT_NE(note, nullptr);
  auto *call = static_cast<UOPCODE *>(note->opcod_deact);
  ASSERT_NE(call, nullptr);
  INSDS *udo = call->ip;
  ASSERT_NE(udo, nullptr);
  ASSERT_NE(call->cvt_out, nullptr);
  ASSERT_NE(call->cvt_in, nullptr);

  /* The note ends with the reinit pass still waiting. */
  csoundSetControlChannel(csound, "stop", 1);
  for (int i = 0; i < 20 && call->ip != nullptr; i++)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(call->ip, nullptr) << "the note did not end";
  EXPECT_GT(ATOMIC_GET(udo->init_running), 0);
  EXPECT_NE(call->cvt_out, nullptr);
  EXPECT_NE(call->cvt_in, nullptr);

  gate.release();
  const auto drained = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (alloc_queue_has_pending(csound)) {
    ASSERT_LT(std::chrono::steady_clock::now(), drained);
    csoundSleep(1);
  }
  EXPECT_EQ(ATOMIC_GET(udo->init_running), 0);
}
} // namespace
#endif
