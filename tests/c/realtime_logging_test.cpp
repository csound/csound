#if defined(CSOUND_TEST_PTHREADS) && !defined(__EMSCRIPTEN__) && !defined(__wasi__)
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {
class RealtimeLoggingTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;
  std::mutex gateMutex, messagesMutex;
  std::condition_variable gateChanged;
  bool entered = false, released = false;
  std::atomic<bool> logFromCallback{false};
  std::atomic<long> droppedFromCallback{0};
  std::vector<std::string> messages;

  static RealtimeLoggingTests &host(CSOUND *cs)
  { return *static_cast<RealtimeLoggingTests *>(csoundGetHostData(cs)); }
  static int32_t holdInit(CSOUND *cs, void *)
  {
    auto &self = host(cs);
    std::unique_lock<std::mutex> lock(self.gateMutex);
    self.entered = true;
    self.gateChanged.notify_all();
    return self.gateChanged.wait_for(lock, std::chrono::seconds(10),
                                    [&self] { return self.released; }) ? 0 : -1;
  }
  static void message(CSOUND *cs, int32_t, const char *str)
  {
    auto &self = host(cs);
    if (std::string(str).find("queue-test ") == 0) {
      {
        std::lock_guard<std::mutex> lock(self.messagesMutex);
        self.messages.emplace_back(str);
      }
      if (self.logFromCallback.exchange(false)) {
        long before = ATOMIC_GET(cs->message_string_queue_dropped);
        csoundMessage(cs, "queue-test reentrant 1\n");
        csoundMessage(cs, "queue-test reentrant 2\n");
        self.droppedFromCallback =
          ATOMIC_GET(cs->message_string_queue_dropped) - before;
      }
    }
  }
  void SetUp() override
  {
#if !CSOUND_SPINLOCK_AVAILABLE || \
    (!defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN))
    GTEST_SKIP() << "Realtime mode requires native atomics and a spinlock backend";
#endif
    csound = csoundCreate(this, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundSetMessageStringCallback(csound, message);
    ASSERT_EQ(csoundSetOption(csound, "-n -d --daemon --realtime"), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_queue_wait", sizeof(OPDS), 0,
                                "", "", holdInit, nullptr, nullptr), 0);
    ASSERT_EQ(csoundCompileOrc(csound, R"(
      sr = 48000
      ksmps = 16
      nchnls = 1
      0dbfs = 1
      instr 9
        test_queue_wait
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
    if (csound != nullptr) csoundDestroy(csound);
  }
  bool drain()
  {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (alloc_queue_has_pending(csound)) {
      if (std::chrono::steady_clock::now() > deadline) return false;
      csoundSleep(1);
    }
    return true;
  }
};

TEST_F(RealtimeLoggingTests, PendingInitDoesNotPrintPollingSegments)
{
  // The worker stays inside holdInit while performance polls the score end.
  // Those control cycles are not score boundaries and must not fill the log.
  csound->oparms->daemon = 0;
  csound->evt.opcod = 'e';
  csound->cyclesRemaining = 0;
  csound->musmonStatics.segamps = 1;
  csound->musmonStatics.sormsg = 1;
  const unsigned long queued = csound->message_string_queue_items;
  const long dropped = ATOMIC_GET(csound->message_string_queue_dropped);
  csound->maxamp[0] = FL(0.75);
  csound->rngflg = 1;
  csound->rngcnt[0] = 2;
  for (int cycle = 0; cycle < 128; ++cycle)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(csound->message_string_queue_items, queued);
  EXPECT_EQ(ATOMIC_GET(csound->message_string_queue_dropped), dropped);
  EXPECT_EQ(csound->smaxamp[0], FL(0.75));
  EXPECT_EQ(csound->musmonStatics.srngcnt[0], 2);
}

TEST_F(RealtimeLoggingTests, RealScoreBoundaryStillPrintsAmplitude)
{
  csound->oparms->daemon = 0;
  csound->evt.opcod = 'e';
  csound->cyclesRemaining = 1;
  csound->musmonStatics.segamps = 1;
  const unsigned long queued = csound->message_string_queue_items;
  const unsigned long wp = csound->message_string_queue_wp;
  csound->maxamp[0] = FL(0.5);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_GT(csound->message_string_queue_items, queued);
  EXPECT_EQ(csound->message_string_queue->messages[wp].str[0], 'B');
  EXPECT_EQ(csound->smaxamp[0], FL(0.5));
}

TEST_F(RealtimeLoggingTests, FullLogQueueDoesNotOverwriteOrReplayMessages)
{
  for (int i = 0; i < 128; ++i) csoundMessage(csound, "queue-test %d\n", i);
  release();
  ASSERT_TRUE(drain());
  csoundReset(csound); // Join and flush the worker before checking its output.
  std::lock_guard<std::mutex> lock(messagesMutex);
  ASSERT_FALSE(messages.empty());
  EXPECT_LE(messages.size(), 64u);
  std::sort(messages.begin(), messages.end());
  EXPECT_EQ(std::adjacent_find(messages.begin(), messages.end()), messages.end());
}

TEST_F(RealtimeLoggingTests, ConcurrentLogWritersPublishWholeMessages)
{
  std::vector<std::thread> writers;
  for (int writer = 0; writer < 4; ++writer)
    writers.emplace_back([this, writer] {
      for (int i = 0; i < 32; ++i)
        csoundMessage(csound, "queue-test %d:%d:%d\n", writer, i, writer * 32 + i);
    });
  for (auto &writer : writers) writer.join();
  release();
  ASSERT_TRUE(drain());
  csoundReset(csound);
  std::lock_guard<std::mutex> lock(messagesMutex);
  ASSERT_FALSE(messages.empty());
  EXPECT_LE(messages.size(), 64u);
  for (const auto &line : messages) {
    int writer = -1, index = -1, combined = -1;
    ASSERT_EQ(std::sscanf(line.c_str(), "queue-test %d:%d:%d", &writer, &index, &combined), 3);
    EXPECT_EQ(combined, writer * 32 + index);
  }
  std::sort(messages.begin(), messages.end());
  EXPECT_EQ(std::adjacent_find(messages.begin(), messages.end()), messages.end());
}

TEST_F(RealtimeLoggingTests, FinalLogDrainRejectsCallbackEnqueue)
{
  csoundMessage(csound, "queue-test final\n");
  logFromCallback = true;
  std::thread stop([this] { csoundReset(csound); });
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (ATOMIC_GET(csound->event_insert_loop) &&
         std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  EXPECT_EQ(ATOMIC_GET(csound->event_insert_loop), 0);
  release();
  stop.join();
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages.front(), "queue-test final\n");
  EXPECT_EQ(droppedFromCallback, 2);
}

TEST(MessageFormattingTests, ConcurrentStringCallbacksKeepTheirOwnText)
{
  struct Capture {
    std::mutex mutex;
    std::condition_variable changed;
    bool entered = false, released = false;
    std::string first, second;
  } capture;
  CSOUND *csound = csoundCreate(&capture, nullptr);
  ASSERT_NE(csound, nullptr);
  csoundSetMessageStringCallback(csound, [](CSOUND *cs, int32_t, const char *str) {
    auto &state = *static_cast<Capture *>(csoundGetHostData(cs));
    const std::string entry(str);
    if (entry == "queue-format first") {
      std::unique_lock<std::mutex> lock(state.mutex);
      state.entered = true;
      state.changed.notify_all();
      state.changed.wait_for(lock, std::chrono::seconds(5),
                             [&state] { return state.released; });
      state.first = str;
    }
    else if (entry == "queue-format second") {
      state.second = str;
    }
  });
  std::thread first([csound] { csoundMessage(csound, "queue-format first"); });
  bool entered;
  {
    std::unique_lock<std::mutex> lock(capture.mutex);
    entered = capture.changed.wait_for(lock, std::chrono::seconds(5),
                                      [&capture] { return capture.entered; });
  }
  if (entered) csoundMessage(csound, "queue-format second");
  {
    std::lock_guard<std::mutex> lock(capture.mutex);
    capture.released = true;
    capture.changed.notify_all();
  }
  first.join();
  EXPECT_TRUE(entered);
  EXPECT_EQ(capture.first, "queue-format first");
  EXPECT_EQ(capture.second, "queue-format second");
  csoundDestroy(csound);
}
} // namespace

#endif
