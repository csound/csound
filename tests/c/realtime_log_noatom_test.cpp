#ifndef __BUILDING_LIBCSOUND
#define __BUILDING_LIBCSOUND
#endif
#include "csoundCore.h"
#include "gtest/gtest.h"

namespace {
class RealtimeLogNoAtomTests : public ::testing::TestWithParam<int32_t> {
 protected:
  CSOUND *csound = nullptr;
  void *(*originalCalloc)(CSOUND *, size_t) = nullptr;
  int32_t allocations = 0;

  static void *countAllocation(CSOUND *cs, size_t size)
  {
    auto &self = *static_cast<RealtimeLogNoAtomTests *>(csoundGetHostData(cs));
    ++self.allocations;
    return self.originalCalloc(cs, size);
  }

  void SetUp() override
  {
    csound = csoundCreate(this, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundSetMessageStringCallback(csound, [](CSOUND *, int32_t, const char *) {});
    originalCalloc = csound->Calloc;
    csound->Calloc = countAllocation;
  }

  void TearDown() override
  {
    if (csound != nullptr) {
      csound->Calloc = originalCalloc;
      realtime_message_queue_stop(csound);
      csoundDestroy(csound);
    }
  }
};

TEST_P(RealtimeLogNoAtomTests, RejectsStartupBeforeChangingQueueOrCallbacks)
{
  csound->oparms_.msglevel = GetParam();
  const auto callback = csound->csoundMessageCallback_;
  const auto stringCallback = csound->csoundMessageStringCallback;
  EXPECT_EQ(realtime_message_queue_start(csound), CSOUND_ERROR);
  EXPECT_EQ(allocations, 0);
  EXPECT_EQ(csound->message_string_queue, nullptr);
  EXPECT_EQ(csound->csoundMessageCallback_, callback);
  EXPECT_EQ(csound->csoundMessageStringCallback, stringCallback);
}

INSTANTIATE_TEST_SUITE_P(MessageLevels, RealtimeLogNoAtomTests,
                         ::testing::Values(0, 1));
} // namespace
