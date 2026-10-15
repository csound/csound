#if defined(CSOUND_TEST_PTHREADS) && !defined(__EMSCRIPTEN__) && !defined(__wasi__)
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"

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
#if !CSOUND_SPINLOCK_AVAILABLE
    GTEST_SKIP() << "Realtime mode requires a native spinlock backend";
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
} // namespace
#endif
