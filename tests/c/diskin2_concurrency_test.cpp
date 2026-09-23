#include <cstdint>
#include <atomic>
#include <thread>
#include "csound.h"
#include "csound_circular_buffer.h"
#include "gtest/gtest.h"

extern "C" int64_t csound_test_diskin_pitch_step(int32_t use_array);
extern "C" int32_t csound_test_diskin_turnoff_during_release(void);
extern "C" void *csound_test_diskin_control_create(void);
extern "C" void csound_test_diskin_control_destroy(void *context);
extern "C" void csound_test_diskin_control_publish(void *context, double pitch);
extern "C" double csound_test_diskin_control_read(void *context, int32_t *reset);

TEST(Diskin2ConcurrencyTests, ScalarStepDuringPitchReadInvalidatesOldHead)
{
    EXPECT_EQ(csound_test_diskin_pitch_step(0), INT64_C(4) << 28);
}

TEST(Diskin2ConcurrencyTests, ArrayStepDuringPitchReadInvalidatesOldHead)
{
    EXPECT_EQ(csound_test_diskin_pitch_step(1), INT64_C(4) << 28);
}

TEST(Diskin2ConcurrencyTests, TurnoffDuringReleaseKeepsTheDeferredClose)
{
    EXPECT_EQ(csound_test_diskin_turnoff_during_release(), 1);
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

TEST(Diskin2ConcurrencyTests, CircularBufferPublishesSamplesBeforePositions)
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
        for (int i = 0; i < read; ++i)
            EXPECT_EQ(samples[i], next++);
        std::this_thread::yield();
    }
    writer.join();
    csoundDestroyCircularBuffer(csound, buffer);
    csoundDestroy(csound);
}
