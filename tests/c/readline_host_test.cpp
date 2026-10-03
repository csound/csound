#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "csound.h"

extern "C" int32_t csoundKillInstance(CSOUND *, cs_float, char *, int32_t,
                                      int32_t, int32_t);

namespace {
struct PromptEvent {
    uint32_t id;
    bool open;
    std::string prompt;
};

class ReadlineHostTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    std::vector<PromptEvent> events;
    bool answerImmediately = false;
    int32_t submitResult = CSOUND_ERROR;

    static void onPrompt(CSOUND *csound, void *data, uint32_t id, const char *prompt)
    {
        auto *self = static_cast<ReadlineHostTests *>(data);
        self->events.push_back({id, prompt != nullptr, prompt ? prompt : ""});
        if (prompt != nullptr && self->answerImmediately) {
            self->answerImmediately = false;
            self->submitResult = csoundReadlineSubmit(csound, id, "immediate");
        }
    }

    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetReadlineCallback(csound, onPrompt, this);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    }

    void TearDown() override
    {
        csoundDestroy(csound);
    }

    void start(const char *prompt = "name> ", const char *score = "i 1 0 1")
    {
        std::string orc =
            "sr = 1000\nksmps = 1\nnchnls = 1\n"
            "chnS \"answer\", 2\n"
            "instr 1\n Sline, kstatus readline \"";
        orc += prompt;
        orc += "\"\n if kstatus == 1 then\n chnset Sline, \"answer\"\n"
               " endif\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), CSOUND_SUCCESS);
        csoundEventString(csound, score, 0);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    }

    void waitForEvents(size_t count)
    {
        for (int cycle = 0; cycle < 128 && events.size() < count; ++cycle)
            ASSERT_GE(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        ASSERT_EQ(events.size(), count);
    }
};

TEST_F(ReadlineHostTests, LatePromptAndRepeatedLines)
{
    start("名前> ", "i 1 0.01 1");
    ASSERT_TRUE(events.empty());
    for (int cycle = 0; cycle < 5; ++cycle)
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_TRUE(events.empty());
    waitForEvents(1);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_TRUE(events[0].open);
    EXPECT_EQ(events[0].prompt, "名前> ");
    uint32_t first = events[0].id;
    EXPECT_NE(first, 0u);
    for (int cycle = 0; cycle < 5; ++cycle)
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_EQ(events.size(), 1u);

    ASSERT_EQ(csoundReadlineSubmit(csound, first, "héllo"), CSOUND_SUCCESS);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_FALSE(events[1].open);
    EXPECT_EQ(events[1].id, first);
    EXPECT_EQ(csoundReadlineSubmit(csound, first, "duplicate"), CSOUND_ERROR);
    waitForEvents(3);
    ASSERT_EQ(events.size(), 3u);
    EXPECT_TRUE(events[2].open);
    EXPECT_GT(events[2].id, first);
    char answer[32];
    csoundGetStringChannel(csound, "answer", answer);
    EXPECT_STREQ(answer, "héllo");
    EXPECT_EQ(csoundReadlineSubmit(csound, first, "late"), CSOUND_ERROR);
    EXPECT_EQ(csoundReadlineSubmit(csound, events[2].id, ""), CSOUND_SUCCESS);
    waitForEvents(5);
    csoundGetStringChannel(csound, "answer", answer);
    EXPECT_STREQ(answer, "");
}

TEST_F(ReadlineHostTests, CanSubmitInsideOpenCallback)
{
    answerImmediately = true;
    start();
    waitForEvents(3);
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(submitResult, CSOUND_SUCCESS);
    EXPECT_TRUE(events[0].open);
    EXPECT_FALSE(events[1].open);
    EXPECT_EQ(events[0].id, events[1].id);
    EXPECT_TRUE(events[2].open);
    char answer[32];
    csoundGetStringChannel(csound, "answer", answer);
    EXPECT_STREQ(answer, "immediate");
}

TEST_F(ReadlineHostTests, EmptyPromptAndReset)
{
    start("");
    waitForEvents(1);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_TRUE(events[0].open);
    EXPECT_TRUE(events[0].prompt.empty());
    uint32_t first = events[0].id;
    csoundReset(csound);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_FALSE(events[1].open);
    EXPECT_EQ(events[1].id, first);
    EXPECT_EQ(csoundReadlineSubmit(csound, first, "late"), CSOUND_ERROR);
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    start("");
    waitForEvents(3);
    ASSERT_EQ(events.size(), 3u);
    EXPECT_GT(events[2].id, first);
    EXPECT_EQ(csoundReadlineSubmit(csound, first, "late"), CSOUND_ERROR);
}

TEST_F(ReadlineHostTests, RejectInvalidLinesWithoutClosingRequest)
{
    start();
    waitForEvents(1);
    ASSERT_EQ(events.size(), 1u);
    auto id = events[0].id;
    EXPECT_EQ(csoundReadlineSubmit(csound, 0, "test"), CSOUND_ERROR);
    EXPECT_EQ(csoundReadlineSubmit(csound, id, nullptr), CSOUND_ERROR);
    EXPECT_EQ(csoundReadlineSubmit(csound, id, "one\ntwo"), CSOUND_ERROR);
    EXPECT_EQ(csoundReadlineSubmit(csound, id, "\r"), CSOUND_ERROR);
    EXPECT_EQ(csoundReadlineSubmit(csound, id, "\004"), CSOUND_ERROR);
    EXPECT_EQ(csoundReadlineSubmit(csound, id, "\177"), CSOUND_ERROR);
    std::string tooLong(65536, 'a');
    EXPECT_EQ(csoundReadlineSubmit(csound, id, tooLong.c_str()), CSOUND_ERROR);
    EXPECT_EQ(events.size(), 1u);
    EXPECT_EQ(csoundReadlineSubmit(csound, id, "valid\tline"), CSOUND_SUCCESS);
}

TEST_F(ReadlineHostTests, TurnoffDiscardsUnconsumedAnswer)
{
    start();
    waitForEvents(1);
    ASSERT_EQ(events.size(), 1u);
    auto first = events[0].id;
    ASSERT_EQ(csoundReadlineSubmit(csound, first, "discard me"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundKillInstance(csound, 1, nullptr, 0, 0, 1), CSOUND_SUCCESS);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 1", 0);
    waitForEvents(3);
    ASSERT_EQ(events.size(), 3u);
    for (int cycle = 0; cycle < 20; ++cycle)
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_EQ(events.size(), 3u);
    EXPECT_EQ(csoundReadlineSubmit(csound, first, "late"), CSOUND_ERROR);
    EXPECT_EQ(csoundReadlineSubmit(csound, events[2].id, "new"), CSOUND_SUCCESS);
    waitForEvents(5);
    char answer[32];
    csoundGetStringChannel(csound, "answer", answer);
    EXPECT_STREQ(answer, "new");
}

TEST_F(ReadlineHostTests, EofClosesOnce)
{
    start();
    waitForEvents(1);
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(csoundReadlinePushText(csound, "\004"), CSOUND_SUCCESS);
    waitForEvents(2);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_FALSE(events[1].open);
    EXPECT_EQ(events[1].id, events[0].id);
    csoundReset(csound);
    EXPECT_EQ(events.size(), 2u);
}

TEST_F(ReadlineHostTests, ErrorClosesOnce)
{
    start();
    waitForEvents(1);
    auto fail = [](void *, void *, uint32_t) -> int32_t { return -1; };
    ASSERT_EQ(csoundRegisterKeyboardCallback(csound, fail, nullptr,
                                            CSOUND_CALLBACK_KBD_TEXT), 0);
    (void) csoundPerformKsmps(csound);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_FALSE(events[1].open);
    EXPECT_EQ(events[1].id, events[0].id);
    csoundReset(csound);
    EXPECT_EQ(events.size(), 2u);
}

TEST_F(ReadlineHostTests, InstrumentEndClosesRequest)
{
    start("end> ", "i 1 0 0.01");
    waitForEvents(2);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_TRUE(events[0].open);
    EXPECT_FALSE(events[1].open);
    EXPECT_EQ(events[1].id, events[0].id);
}
} // namespace
