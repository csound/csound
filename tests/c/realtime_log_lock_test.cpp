#ifndef __BUILDING_LIBCSOUND
#define __BUILDING_LIBCSOUND
#endif
#include "csoundCore.h"
#include "gtest/gtest.h"

extern "C" {
void csound_test_realtime_locks_begin(void);
void csound_test_realtime_locks_end(int32_t *, int32_t *, int32_t *);
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
} // namespace
