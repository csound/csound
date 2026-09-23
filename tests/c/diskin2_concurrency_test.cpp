#include <cstdint>
#include <atomic>
#include <thread>
#include "csound.h"
#include "csound_circular_buffer.h"
#include "gtest/gtest.h"

extern "C" int64_t csound_test_diskin_pitch_step(int32_t use_array);
extern "C" void *csound_test_diskin_control_create(void);
extern "C" void csound_test_diskin_control_destroy(void *context);
extern "C" void csound_test_diskin_control_publish(void *context, double pitch);
extern "C" double csound_test_diskin_control_read(void *context, int32_t *reset);
extern "C" void csound_test_diskin_pause_control_read(void *context);

TEST(Diskin2ConcurrencyTests, ScalarStepDuringPitchReadInvalidatesOldHead)
{
    EXPECT_EQ(csound_test_diskin_pitch_step(0), INT64_C(4) << 28);
}

TEST(Diskin2ConcurrencyTests, ArrayStepDuringPitchReadInvalidatesOldHead)
{
    EXPECT_EQ(csound_test_diskin_pitch_step(1), INT64_C(4) << 28);
}

class Diskin2ControlTests : public ::testing::Test {
protected:
    void *control = nullptr;
    void SetUp() override {
        control = csound_test_diskin_control_create();
        ASSERT_NE(control, nullptr);
    }
    void TearDown() override {
        if (control != nullptr)
            csound_test_diskin_control_destroy(control);
    }
    void expectControl(double pitch, int32_t expectedReset) {
        int32_t reset = -1;
        EXPECT_EQ(csound_test_diskin_control_read(control, &reset), pitch);
        EXPECT_EQ(reset, expectedReset);
    }
    void publish(double pitch) {
        csound_test_diskin_control_publish(control, pitch);
    }
};

TEST_F(Diskin2ControlTests, ExtraPollsDoNotTurnARampIntoSteps)
{
    expectControl(1, 0);
    publish(2);
    expectControl(2, 1);
    expectControl(2, 0);
    publish(3);
    expectControl(3, 0);
    expectControl(3, 0);
    publish(4);
    expectControl(4, 0);
    publish(4);
    publish(-1);
    expectControl(-1, 1);
}

TEST_F(Diskin2ControlTests, StepSurvivesRampAndReturnBeforeWorkerPoll)
{
    publish(2);
    publish(3);
    publish(1);
    expectControl(1, 1);
    expectControl(1, 0);
}

TEST_F(Diskin2ControlTests, PublisherCannotOverwriteTheReadersSlot)
{
    publish(2);
    // Pause after the worker takes its slot but before it copies the record.
    // Perf can publish three times without waiting or changing that record.
    csound_test_diskin_pause_control_read(control);
    expectControl(2, 1);
    expectControl(4, 1);
    expectControl(4, 0);
}

TEST_F(Diskin2ControlTests, PublisherCanRunWhileWorkerIsIdle)
{
    for (int pitch = 2; pitch <= 20000; ++pitch)
        publish(pitch);
    expectControl(20000, 1);
    expectControl(20000, 0);
}

TEST_F(Diskin2ControlTests, ConcurrentStepsKeepPitchAndResetTogether)
{
    std::atomic<bool> start{false};
    std::atomic<bool> done{false};
    constexpr int lastPitch = 20000;
    std::thread perf([&] {
        while (!start.load())
            std::this_thread::yield();
        for (int pitch = 2; pitch <= lastPitch; ++pitch) {
            publish(pitch);
            publish(pitch); // A steady period makes the next change a step.
        }
        done.store(true);
    });
    double previous = 1;
    start.store(true);
    do {
        int32_t reset;
        double pitch = csound_test_diskin_control_read(control, &reset);
        EXPECT_GE(pitch, previous);
        if (pitch != previous)
            EXPECT_EQ(reset, 1);
        previous = pitch;
    } while (!done.load());
    perf.join();
    int32_t reset;
    double pitch = csound_test_diskin_control_read(control, &reset);
    EXPECT_EQ(pitch, lastPitch);
    if (pitch != previous)
        EXPECT_EQ(reset, 1);
}

extern "C" int32_t csound_test_diskin_audio_locks(int32_t use_array, int32_t stop);
TEST(Diskin2ConcurrencyTests, AudioReadTakesNoLocks)
{
    EXPECT_EQ(csound_test_diskin_audio_locks(0, 0), 0);
    EXPECT_EQ(csound_test_diskin_audio_locks(1, 0), 0);
}
TEST(Diskin2ConcurrencyTests, TurnoffTakesNoLocks)
{
    EXPECT_EQ(csound_test_diskin_audio_locks(0, 1), 0);
    EXPECT_EQ(csound_test_diskin_audio_locks(1, 1), 0);
}

extern "C" int32_t csound_test_diskin_retirement(int32_t borrowed, int32_t reinit);
TEST(Diskin2ConcurrencyTests, StopBetweenWorkerPollsRetainsTheOwnerUntilCleanup)
{
    EXPECT_EQ(csound_test_diskin_retirement(0, 0), 1);
}
TEST(Diskin2ConcurrencyTests, TwoWorkersFinishBeforeTheOwnerFilesClose)
{
    EXPECT_EQ(csound_test_diskin_retirement(1, 0), 1);
}
TEST(Diskin2ConcurrencyTests, ReinitWaitsForBorrowAndReusesRegistration)
{
    EXPECT_EQ(csound_test_diskin_retirement(1, 1), 1);
}

TEST(Diskin2ConcurrencyTests, CircularBufferKeepsSamplesInOrderAcrossWraps)
{
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    void *buffer = csoundCreateCircularBuffer(csound, 31, sizeof(int32_t));
    ASSERT_NE(buffer, nullptr);
    constexpr int count = 20000;
    std::thread writer([&] {
        int32_t next = 0;
        while (next < count) {
            int32_t samples[11];
            int32_t size = count - next < 11 ? count - next : 11;
            for (int i = 0; i < size; ++i)
                samples[i] = next + i;
            next += csoundWriteCircularBuffer(csound, buffer, samples, size);
            std::this_thread::yield();
        }
    });
    int32_t next = 0;
    while (next < count) {
        int32_t peeked;
        if (csoundPeekCircularBuffer(csound, buffer, &peeked, 1))
            EXPECT_EQ(peeked, next);
        int32_t samples[7];
        int32_t read = csoundReadCircularBuffer(csound, buffer, samples, 7);
        for (int i = 0; i < read; ++i) {
            EXPECT_EQ(samples[i], next);
            ++next;
        }
        std::this_thread::yield();
    }
    writer.join();
    csoundDestroyCircularBuffer(csound, buffer);
    csoundDestroy(csound);
}
