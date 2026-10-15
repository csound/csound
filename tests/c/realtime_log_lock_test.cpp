#ifndef __BUILDING_LIBCSOUND
#define __BUILDING_LIBCSOUND
#endif
#include "csoundCore.h"
#include "gtest/gtest.h"

extern "C" {
void csound_test_realtime_locks_begin(void);
void csound_test_realtime_after_log_format(void (*)(void *), void *);
void csound_test_realtime_locks_end(int32_t *, int32_t *, int32_t *);
void csound_test_realtime_before_log_lock(void (*)(void *), void *);
void csound_test_realtime_on_log_wait(void (*)(void *), void *);
int32_t csound_test_realtime_log_dequeue(CSOUND *, unsigned long *,
                                        message_string_t *);
}

namespace {
class RealtimeLogLockTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundSetMessageStringCallback(csound, [](CSOUND *, int32_t, const char *) {});
  }
  void TearDown() override
  {
    csound_test_realtime_before_log_lock(nullptr, nullptr);
    csound_test_realtime_on_log_wait(nullptr, nullptr);
    if (csound != nullptr) {
      realtime_message_queue_stop(csound);
      csoundDestroy(csound);
    }
  }
  void expectNoPlatformLocks()
  {
    int32_t locks, tries, unlocks;
    csound_test_realtime_locks_end(&locks, &tries, &unlocks);
    EXPECT_EQ(locks, 0);
    EXPECT_EQ(tries, 0);
    EXPECT_EQ(unlocks, 0);
  }
};

TEST_F(RealtimeLogLockTests, LogPublicationAvoidsPlatformLocks)
{
  csound->oparms_.msglevel = 1;
  ASSERT_EQ(realtime_message_queue_start(csound), CSOUND_SUCCESS);
  csound_test_realtime_locks_begin();
  csoundMessage(csound, "queued log message\n");
  expectNoPlatformLocks();
  EXPECT_EQ(csound->message_string_queue_items, 1u);
  EXPECT_EQ(ATOMIC_GET(csound->message_string_queue_dropped), 0);
}

TEST_F(RealtimeLogLockTests, BusyLogQueueDropsWithoutWaiting)
{
  csound->oparms_.msglevel = 1;
  ASSERT_EQ(realtime_message_queue_start(csound), CSOUND_SUCCESS);
  ATOMIC_SET(csound->message_string_queue_lock, 1);
  csound_test_realtime_locks_begin();
  csoundMessage(csound, "contended log message\n");
  expectNoPlatformLocks();
  EXPECT_EQ(csound->message_string_queue_items, 0u);
  EXPECT_EQ(ATOMIC_GET(csound->message_string_queue_dropped), 1);
  ATOMIC_SET(csound->message_string_queue_lock, 0);
}

TEST_F(RealtimeLogLockTests, CallbackSelectedBeforeStopCanRunAfterStop)
{
  csound->oparms_.msglevel = 1;
  ASSERT_EQ(realtime_message_queue_start(csound), CSOUND_SUCCESS);
  auto enqueue = csound->csoundMessageStringCallback;
  realtime_message_queue_stop(csound);
  ASSERT_EQ(csound->message_string_queue, nullptr);
  csound_test_realtime_locks_begin();
  enqueue(csound, 0, "late log message\n");
  expectNoPlatformLocks();
  EXPECT_EQ(ATOMIC_GET(csound->message_string_queue_dropped), 1);
}

TEST_F(RealtimeLogLockTests, StopBeforePublicationDropsPendingMessage)
{
  csound->oparms_.msglevel = 1;
  ASSERT_EQ(realtime_message_queue_start(csound), CSOUND_SUCCESS);
  auto enqueue = csound->csoundMessageStringCallback;
  // Stop at the CAS boundary, after the callback has entered but before
  // it can acquire the queue. The hook runs once, including nested calls.
  csound_test_realtime_before_log_lock([](void *data) {
    realtime_message_queue_stop(static_cast<CSOUND *>(data));
  }, csound);
  enqueue(csound, 0, "interrupted log message\n");
  EXPECT_EQ(csound->message_string_queue, nullptr);
  EXPECT_EQ(ATOMIC_GET(csound->message_string_queue_dropped), 1);
}

TEST_F(RealtimeLogLockTests, LogWorkerYieldsWhilePublicationIsBusy)
{
  struct WaitingProducer {
    CSOUND *csound;
    int waits = 0;
  } producer{csound};
  csound->oparms_.msglevel = 1;
  ASSERT_EQ(realtime_message_queue_start(csound), CSOUND_SUCCESS);
  csoundMessage(csound, "message after contention\n");
  ATOMIC_SET(csound->message_string_queue_lock, 1);
  csound_test_realtime_on_log_wait([](void *data) {
    auto &producer = *static_cast<WaitingProducer *>(data);
    ++producer.waits;
    ATOMIC_SET(producer.csound->message_string_queue_lock, 0);
  }, &producer);
  unsigned long rp = 0;
  message_string_t message;
  EXPECT_EQ(csound_test_realtime_log_dequeue(csound, &rp, &message), 1);
  EXPECT_EQ(producer.waits, 1);
  EXPECT_STREQ(message.str, "message after contention\n");
}

TEST_F(RealtimeLogLockTests, QueueCanRestartAfterStop)
{
  csound->oparms_.msglevel = 1;
  for (int run = 0; run < 3; ++run) {
    ASSERT_EQ(realtime_message_queue_start(csound), CSOUND_SUCCESS);
    EXPECT_EQ(csound->message_string_queue_items, 0u);
    EXPECT_EQ(ATOMIC_GET(csound->message_string_queue_dropped), 0);
    csoundMessage(csound, "new performance log message\n");
    EXPECT_EQ(csound->message_string_queue_items, 1u);
    realtime_message_queue_stop(csound);
    EXPECT_EQ(csound->message_string_queue, nullptr);
  }
}

TEST_F(RealtimeLogLockTests, StopDuringFormattingDropsPendingMessage)
{
  // A fresh instance has only the variadic callback; stop restores NULL
  // in the string slot while a producer may still be formatting its text.
  csoundSetMessageCallback(csound, [](CSOUND *, int32_t, const char *, va_list) {});
  csound->csoundMessageStringCallback = nullptr;
  csound->oparms_.msglevel = 1;
  for (int api = 0; api < 3; ++api) {
    ASSERT_EQ(realtime_message_queue_start(csound), CSOUND_SUCCESS);
    csound_test_realtime_after_log_format([](void *data) {
      realtime_message_queue_stop(static_cast<CSOUND *>(data));
    }, csound);
    if (api == 0)
      csoundMessage(csound, "late %s\n", "message");
    else if (api == 1)
      csoundMessageS(csound, CSOUNDMSG_WARNING, "late %s\n", "message");
    else
      csoundErrorMsg(csound, "late %s\n", "message"); // Uses csoundMessageV.
    EXPECT_EQ(csound->message_string_queue, nullptr);
    EXPECT_EQ(csound->csoundMessageStringCallback, nullptr);
    EXPECT_EQ(ATOMIC_GET(csound->message_string_queue_dropped), 1);
  }
}

TEST_F(RealtimeLogLockTests, StartupPreservesQueueAllocationError)
{
  struct AllocationFailure {
    void *(*allocate)(CSOUND *, size_t);
    bool reached = false;
  } failure{csound->Calloc};
  ASSERT_EQ(csoundSetOption(csound, "-n -d --daemon --realtime"), 0);
  ASSERT_EQ(csoundCompileOrc(csound, "sr=48000\nksmps=16\nnchnls=1\n", 0), 0);
  csoundSetHostData(csound, &failure);
  csound->Calloc = [](CSOUND *cs, size_t size) -> void * {
    auto &failure = *static_cast<AllocationFailure *>(csoundGetHostData(cs));
    if (size == sizeof(message_string_queue_t)) {
      failure.reached = true;
      return nullptr;
    }
    return failure.allocate(cs, size);
  };
  const int32_t result = csoundStart(csound);
  csound->Calloc = failure.allocate;
  csoundSetHostData(csound, nullptr);
  EXPECT_TRUE(failure.reached);
  EXPECT_EQ(result, CSOUND_MEMORY);
  EXPECT_EQ(csound->init_pass_threadlock, nullptr);
  EXPECT_EQ(csound->alloc_queue, nullptr);
  EXPECT_EQ(csound->event_insert_thread, nullptr);
}

} // namespace
