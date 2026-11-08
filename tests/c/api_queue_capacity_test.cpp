#include "csoundCore.h"
#include "gtest/gtest.h"

extern "C" {
void message_dequeue(CSOUND *);
int32_t csoundKillInstance(CSOUND *, cs_float, char *, int32_t, int32_t, int32_t);
void csound_test_api_mutex_begin(void);
void csound_test_api_mutex_end(int32_t *, int32_t *, int32_t *);
void csound_test_after_api_write_load(CSOUND *, int32_t, void (*)(CSOUND *));
}

namespace {
class ApiQueueCapacityTests : public ::testing::TestWithParam<int> {
protected:
    CSOUND *csound = nullptr;
    cs_float interveningCopy = 0;
    void SetUp() override
    {
        csound = csoundCreate(this, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
        ASSERT_EQ(csoundCompileOrc(csound,
            "sr=48000\nksmps=32\nnchnls=1\ngiTable ftgen 1, 0, -1, -2, 1\n", 0),
            CSOUND_SUCCESS);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    }
    void TearDown() override
    {
        csound_test_after_api_write_load(nullptr, 0, nullptr);
        csoundDestroy(csound);
    }
    static void advanceQueue(CSOUND *csound)
    {
        auto *self = static_cast<ApiQueueCapacityTests *>(csoundGetHostData(csound));
        csoundTableCopyOut(csound, 1, &self->interveningCopy, 1);
        message_dequeue(csound);
    }
};

TEST_P(ApiQueueCapacityTests, ConsumerPassingAnOldWriteSnapshotDoesNotMeanFull)
{
    cs_float copy = 0;
    // Another producer and the consumer advance together after this producer
    // reads write. Neither the precheck nor reservation may call that full.
    csound_test_after_api_write_load(csound, GetParam(), advanceQueue);
    csoundTableCopyOut(csound, 1, &copy, 1);
    message_dequeue(csound);
    EXPECT_EQ(interveningCopy, 1);
    EXPECT_EQ(copy, 1);
}

INSTANTIATE_TEST_SUITE_P(PrecheckAndReservation, ApiQueueCapacityTests,
                        ::testing::Values(1, 2));
class ApiTableMutexTests : public ApiQueueCapacityTests {
protected:
    void SetUp() override
    {
        ApiQueueCapacityTests::SetUp();
        originalInitMutex = csound->init_pass_threadlock;
        csound->init_pass_threadlock = csoundCreateMutex(1);
        ASSERT_NE(csound->init_pass_threadlock, nullptr);
    }
    void TearDown() override
    {
        csound->oparms->realtime = 0;
        csoundDestroyMutex(csound->init_pass_threadlock);
        csound->init_pass_threadlock = originalInitMutex;
        ApiQueueCapacityTests::TearDown();
    }
    void checkCopies(int realtime)
    {
        // Set only the dispatch flag: no worker can add unrelated mutex calls.
        csound->oparms->realtime = realtime;
        cs_float input[] = { 2, 3 };
        cs_float output[] = { 0, 99 };
        int32_t locks, tries, unlocks;
        csound_test_api_mutex_begin();
        csoundTableCopyIn(csound, 1, input, GetParam());
        csoundTableCopyOut(csound, 1, output, GetParam());
        if (GetParam())
            message_dequeue(csound);
        csound_test_api_mutex_end(&locks, &tries, &unlocks);
        EXPECT_EQ(locks, !realtime && !GetParam() ? 2 : 0);
        EXPECT_EQ(tries, 0);
        EXPECT_EQ(unlocks, !realtime && !GetParam() ? 2 : 0);
        EXPECT_EQ(output[0], 2);
        EXPECT_EQ(output[1], 99);
        cs_float *table;
        ASSERT_EQ(csoundGetTable(csound, &table, 1), 1);
        EXPECT_EQ(table[1], 3);
    }
    void *originalInitMutex = nullptr;
};

TEST_P(ApiTableMutexTests, RealtimeCopiesDoNotCallMutexFunctions)
{
    checkCopies(1);
}

TEST_P(ApiTableMutexTests, NonRealtimeCopiesKeepSynchronousApiLock)
{
    checkCopies(0);
}

TEST_P(ApiTableMutexTests, SynchronousKillUsesApiLockOnlyOutsideRealtime)
{
    ASSERT_EQ(csoundCompileOrc(csound, "instr 1\nendin\n", 0), CSOUND_SUCCESS);
    const cs_float fields[] = { 1, 0, -1 };
    csoundEvent(csound, CS_INSTR_EVENT, fields, 3, 0);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    csound->oparms->realtime = GetParam();
    int32_t locks, tries, unlocks;
    csound_test_api_mutex_begin();
    const int32_t result = csoundKillInstance(csound, 1, nullptr, 0, 0, 0);
    csound_test_api_mutex_end(&locks, &tries, &unlocks);
    EXPECT_EQ(result, CSOUND_SUCCESS);
    EXPECT_EQ(locks, GetParam() ? 0 : 1);
    EXPECT_EQ(tries, 0);
    EXPECT_EQ(unlocks, GetParam() ? 0 : 1);
}

INSTANTIATE_TEST_SUITE_P(DispatchOptions, ApiTableMutexTests,
                        ::testing::Values(0, 1));
} // namespace
