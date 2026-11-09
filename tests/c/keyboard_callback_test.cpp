/* A keyboard callback removed or registered again from inside a keyboard
   callback, which is on the thread performing. The list is newest first. */
#include <string>
#include "gtest/gtest.h"
#include "csound.h"

namespace {
struct Calls {
    CSOUND *csound;
    std::string order;
};

// Csound tells callbacks apart by address, so each must be a function of its own.
template <char Name>
int32_t named(void *data, void *, uint32_t)
{
    static_cast<Calls *>(data)->order += Name;
    return CSOUND_SUCCESS;
}

int32_t removesItself(void *data, void *, uint32_t)
{
    auto *calls = static_cast<Calls *>(data);
    calls->order += 'r';
    csoundRemoveKeyboardCallback(calls->csound, removesItself);
    return CSOUND_SUCCESS;
}

int32_t registersItself(void *data, void *, uint32_t type)
{
    auto *calls = static_cast<Calls *>(data);
    calls->order += 'm';
    csoundRegisterKeyboardCallback(calls->csound, registersItself, data, type);
    return CSOUND_SUCCESS;
}

int32_t removesItselfAndTheNext(void *data, void *, uint32_t)
{
    auto *calls = static_cast<Calls *>(data);
    calls->order += 'r';
    csoundRemoveKeyboardCallback(calls->csound, removesItselfAndTheNext);
    csoundRemoveKeyboardCallback(calls->csound, named<'a'>);
    return CSOUND_SUCCESS;
}

int32_t removesThePrevious(void *data, void *, uint32_t)
{
    auto *calls = static_cast<Calls *>(data);
    calls->order += 'b';
    csoundRemoveKeyboardCallback(calls->csound, named<'a'>);
    return CSOUND_SUCCESS;
}

int32_t replacesEverything(void *data, void *, uint32_t type)
{
    auto *calls = static_cast<Calls *>(data);
    calls->order += 'x';
    csoundRemoveKeyboardCallback(calls->csound, replacesEverything);
    csoundRemoveKeyboardCallback(calls->csound, named<'a'>);
    csoundRegisterKeyboardCallback(calls->csound, named<'c'>, data, type);
    return CSOUND_SUCCESS;
}

class KeyboardCallbackTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    Calls calls{};

    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        calls.csound = csound;
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

    void add(int32_t (*function)(void *, void *, uint32_t))
    {
        ASSERT_EQ(csoundRegisterKeyboardCallback(csound, function, &calls,
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
    add(named<'a'>);
    add(removesItself);
    perform(3);
    EXPECT_EQ(calls.order, "ra" "a" "a");
}

TEST_F(KeyboardCallbackTests, RegistersItselfAgainDuringItsCall)
{
    add(named<'a'>);
    add(registersItself);
    perform(3);
    EXPECT_EQ(calls.order, "ma" "ma" "ma");
}

TEST_F(KeyboardCallbackTests, RegistersItselfAgainFromTheMiddle)
{
    add(named<'a'>);
    add(registersItself);
    add(named<'b'>);
    perform(3);
    // Moved to the front, it is not called twice in the round that moved it.
    EXPECT_EQ(calls.order, "bma" "mba" "mba");
}

TEST_F(KeyboardCallbackTests, RemovesItselfAndTheOneAfterIt)
{
    add(named<'c'>);
    add(named<'a'>);
    add(removesItselfAndTheNext);
    perform(3);
    EXPECT_EQ(calls.order, "rc" "c" "c");
}

TEST_F(KeyboardCallbackTests, RemovesTheOneBeforeIt)
{
    add(named<'c'>);
    add(removesThePrevious);
    add(named<'a'>);
    perform(2);
    // Already called this round, so nothing is skipped or repeated.
    EXPECT_EQ(calls.order, "abc" "bc");
}

TEST_F(KeyboardCallbackTests, RegistersANewOneAfterRemovingEverything)
{
    add(named<'a'>);
    add(replacesEverything);
    perform(3);
    // What is registered during a round has its first call in the next one.
    EXPECT_EQ(calls.order, "x" "c" "c");
}

TEST_F(KeyboardCallbackTests, ListStillWorksAfterReset)
{
    add(named<'a'>);
    perform(1);
    csoundReset(csound);
    start();
    perform(1);
    add(named<'b'>); // newer than 'a', across the reset
    perform(1);
    csoundRemoveKeyboardCallback(csound, named<'a'>);
    perform(1);
    EXPECT_EQ(calls.order, "a" "a" "ba" "b");
}
} // namespace
