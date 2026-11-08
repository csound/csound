/* A keyboard callback removed or registered again while Csound is calling the
   callbacks: from inside one, and from another thread. */
#include <atomic>
#include <thread>
#include "gtest/gtest.h"
#include "csound.h"

namespace {
struct Counts {
    CSOUND *csound;
    int calls[3];
};

// Csound tells callbacks apart by address, so each must be a function of its own.
template <int N>
int32_t counted(void *data, void *, uint32_t)
{
    static_cast<Counts *>(data)->calls[N]++;
    return CSOUND_SUCCESS;
}

int32_t removesItself(void *data, void *, uint32_t)
{
    auto *counts = static_cast<Counts *>(data);
    counts->calls[0]++;
    csoundRemoveKeyboardCallback(counts->csound, removesItself);
    return CSOUND_SUCCESS;
}

int32_t registersItself(void *data, void *, uint32_t type)
{
    auto *counts = static_cast<Counts *>(data);
    counts->calls[0]++;
    csoundRegisterKeyboardCallback(counts->csound, registersItself, data, type);
    return CSOUND_SUCCESS;
}

int32_t removesItselfAndTheNext(void *data, void *, uint32_t)
{
    auto *counts = static_cast<Counts *>(data);
    counts->calls[0]++;
    csoundRemoveKeyboardCallback(counts->csound, removesItselfAndTheNext);
    csoundRemoveKeyboardCallback(counts->csound, counted<1>);
    return CSOUND_SUCCESS;
}

struct Held {
    std::atomic<bool> entered{false};
    std::atomic<bool> release{false};
    std::atomic<bool> gaveUp{false};
    std::atomic<int> calls{0};
};

const int patience = 5000; // milliseconds: long enough never to be reached

int32_t heldUntilReleased(void *data, void *, uint32_t)
{
    auto *held = static_cast<Held *>(data);
    held->calls++;
    held->entered = true;
    int waited = 0;
    while (!held->release && waited++ < patience)
        csoundSleep(1);
    if (!held->release)
        held->gaveUp = true;
    return CSOUND_SUCCESS;
}

class KeyboardCallbackTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    Counts counts{};

    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        counts.csound = csound;
        start();
    }

    void start()
    {
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
        ASSERT_EQ(csoundCompileOrc(csound,
            "sr = 1000\nksmps = 1\nnchnls = 1\n"
            "instr 1\n  kkey sensekey\nendin\n", 0), CSOUND_SUCCESS);
        csoundEventString(csound, "i 1 0 1000", 0);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    }

    void TearDown() override
    {
        csoundDestroy(csound);
    }

    // The last one registered is called first.
    void add(int32_t (*function)(void *, void *, uint32_t), void *data)
    {
        ASSERT_EQ(csoundRegisterKeyboardCallback(csound, function, data,
                                                 CSOUND_CALLBACK_KBD_TEXT),
                  CSOUND_SUCCESS);
    }

    void perform(int periods)
    {
        for (int i = 0; i < periods; i++)
            ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    }
};

TEST_F(KeyboardCallbackTests, RemovesItselfDuringItsCall)
{
    add(counted<1>, &counts);
    add(removesItself, &counts);
    perform(3);
    EXPECT_EQ(counts.calls[0], 1);
    EXPECT_EQ(counts.calls[1], 3); // the walk went on past the one removed
}

TEST_F(KeyboardCallbackTests, RegistersItselfAgainDuringItsCall)
{
    add(counted<1>, &counts);
    add(registersItself, &counts);
    perform(3);
    EXPECT_EQ(counts.calls[0], 3); // once a period, and still registered
    EXPECT_EQ(counts.calls[1], 3);
}

TEST_F(KeyboardCallbackTests, RemovesItselfAndTheOneAfterIt)
{
    add(counted<2>, &counts);
    add(counted<1>, &counts);
    add(removesItselfAndTheNext, &counts);
    perform(3);
    EXPECT_EQ(counts.calls[0], 1);
    EXPECT_EQ(counts.calls[1], 0); // removed before its turn, so never called
    EXPECT_EQ(counts.calls[2], 3);
}

TEST_F(KeyboardCallbackTests, ListStillWorksAfterReset)
{
    add(counted<1>, &counts);
    perform(1);
    csoundReset(csound);
    start();
    perform(1);
    EXPECT_EQ(counts.calls[1], 2); // preserved by csoundReset(), as documented
    add(counted<2>, &counts);
    csoundRemoveKeyboardCallback(csound, counted<1>);
    perform(1);
    EXPECT_EQ(counts.calls[1], 2);
    EXPECT_EQ(counts.calls[2], 1);
}

TEST_F(KeyboardCallbackTests, AnotherThreadRemovesOneDuringItsCall)
{
    Held held;
    add(counted<1>, &counts);
    add(heldUntilReleased, &held);
    std::thread performer([this] { csoundPerformKsmps(csound); });
    for (int waited = 0; !held.entered && waited < patience; waited++)
        csoundSleep(1);
    bool entered = held.entered;
    csoundRemoveKeyboardCallback(csound, heldUntilReleased);
    // Not reached until the callback has given up, if the removal waits for it.
    held.release = true;
    performer.join();
    ASSERT_TRUE(entered);
    EXPECT_FALSE(held.gaveUp);
    perform(2);
    EXPECT_EQ(held.calls, 1);
    EXPECT_EQ(counts.calls[1], 3); // the walk in progress went on to the next
}

TEST_F(KeyboardCallbackTests, AnotherThreadRegistersAndRemovesDuringPerformance)
{
    const int periods = 20000;
    std::atomic<bool> done{false};
    add(counted<0>, &counts);
    std::thread churn([this, &done] {
        while (!done) {
            csoundRegisterKeyboardCallback(csound, counted<1>, &counts,
                                           CSOUND_CALLBACK_KBD_TEXT);
            csoundRemoveKeyboardCallback(csound, counted<1>);
        }
    });
    perform(periods);
    done = true;
    churn.join();
    EXPECT_EQ(counts.calls[0], periods); // whatever else came and went
}
} // namespace
