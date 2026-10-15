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
class RealtimeLockTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundSetMessageStringCallback(csound, [](CSOUND *, int32_t, const char *) {});
    ASSERT_EQ(alloc_queue_lock_init(csound), CSOUND_SUCCESS);
  }
  void TearDown() override
  {
    if (csound != nullptr) {
      alloc_queue_lock_destroy(csound);
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

TEST_F(RealtimeLockTests, MidiReadinessAvoidsPlatformLocks)
{
  csound_test_realtime_locks_begin();
  EXPECT_TRUE(alloc_queue_midi_ready(csound, 0));
  EXPECT_TRUE(alloc_queue_midi_ready(csound, 1));
  ATOMIC_SET(csound->alloc_queue_midi, 1);
  EXPECT_TRUE(alloc_queue_midi_ready(csound, 0));
  EXPECT_FALSE(alloc_queue_midi_ready(csound, 1));
  ATOMIC_SET(csound->alloc_queue_items, MAX_ALLOC_QUEUE);
  EXPECT_FALSE(alloc_queue_midi_ready(csound, 0));
  ATOMIC_SET(csound->alloc_queue_items, 0);
  ATOMIC_SET(csound->alloc_queue_stopping, 1);
  EXPECT_FALSE(alloc_queue_midi_ready(csound, 0));
  expectNoPlatformLocks();
}

TEST_F(RealtimeLockTests, QueueStopAvoidsPlatformLocks)
{
  ATOMIC_SET(csound->event_insert_loop, 1);
  csound_test_realtime_locks_begin();
  alloc_queue_stop(csound);
  expectNoPlatformLocks();
  EXPECT_EQ(ATOMIC_GET(csound->alloc_queue_stopping), 1);
  EXPECT_EQ(ATOMIC_GET(csound->event_insert_loop), 0);
}
} // namespace
