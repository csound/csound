#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "csound.h"
#include "gtest/gtest.h"

#define csoundCompileOrc(a,b) csoundCompileOrc(a,b,0)
#define csoundReadScore(a,b) csoundEventString(a,b,0)

class MessageBufferTests : public ::testing::Test {
public:
    MessageBufferTests ()
    {   
    }

    virtual ~MessageBufferTests ()
    {
    }

    virtual void SetUp ()
    {
      csound = csoundCreate (NULL,NULL);
      csoundCreateMessageBuffer (csound, 0);
        //csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
    }

    CSOUND* csound = nullptr;
};

TEST_F (MessageBufferTests, testCreateBuffer)
{
    int32_t argc = 2;
    const char *argv[] = {"csound", "-v"};
    csoundCompile(csound, argc, argv);

    int32_t cnt = csoundGetMessageCnt(csound);
    ASSERT_TRUE (cnt > 0);
    const char * msg = csoundGetFirstMessage(csound);
    ASSERT_TRUE (msg != NULL);
    int32_t newcnt = csoundGetMessageCnt(csound);
    ASSERT_EQ (cnt, newcnt);
    csoundPopFirstMessage(csound);
    newcnt = csoundGetMessageCnt(csound);
    ASSERT_EQ (cnt - 1, newcnt);
}

TEST_F (MessageBufferTests, testBufferRun)
{
    int32_t result = csoundCompileOrc(csound, "instr 1\n"
                                  "asig oscil 0.1, 440\n"
                                  "out asig\n"
                                  "endin\n");
    ASSERT_EQ(result, CSOUND_SUCCESS);
    csoundReadScore(csound, "i 1 0 0.1\n");
    csoundStart(csound);

    while(csoundPerformKsmps(csound) == 0);

    while (csoundGetMessageCnt(csound)) {
        const char * msg = csoundGetFirstMessage(csound);
        ASSERT_TRUE (msg != NULL);
        printf("CSOUND MESSAGE: %s\n", msg);
        csoundPopFirstMessage(csound);
    }
}

#if GTEST_HAS_STREAM_REDIRECTION
TEST_F (MessageBufferTests, testBufferedAndPrintedMessagesMatch)
{
    csoundDestroyMessageBuffer(csound);
    csoundCreateMessageBuffer(csound, 1);
    testing::internal::CaptureStdout();
    csoundMessage(csound, "value=%d, text=%s!", 42, "hello");
    const std::string printed = testing::internal::GetCapturedStdout();
    EXPECT_EQ(printed, "value=42, text=hello!");
    ASSERT_EQ(csoundGetMessageCnt(csound), 1);
    EXPECT_STREQ(csoundGetFirstMessage(csound), printed.c_str());
}

TEST_F (MessageBufferTests, testEmptyAndLongMessages)
{
    for (int echo = 0; echo <= 1; ++echo) {
        SCOPED_TRACE(echo);
        csoundDestroyMessageBuffer(csound);
        csoundCreateMessageBuffer(csound, echo);
        for (size_t size : {size_t(0), size_t(1), size_t(16383),
                            size_t(16384), size_t(20000)}) {
            SCOPED_TRACE(size);
            const std::string message(size, 'x');
            testing::internal::CaptureStdout();
            csoundMessage(csound, "%s", message.c_str());
            const std::string printed = testing::internal::GetCapturedStdout();
            EXPECT_EQ(printed, echo ? message : "");
            ASSERT_EQ(csoundGetMessageCnt(csound), 1);
            EXPECT_STREQ(csoundGetFirstMessage(csound), message.c_str());
            csoundPopFirstMessage(csound);
        }
    }
}

TEST_F (MessageBufferTests, testBufferedAttributesAndOrder)
{
    csoundDestroyMessageBuffer(csound);
    csoundCreateMessageBuffer(csound, 1);
    testing::internal::CaptureStderr();
    csoundMessageS(csound, CSOUNDMSG_ERROR, "error %d\n", 1);
    csoundMessageS(csound, CSOUNDMSG_WARNING, "warning %d\n", 2);
    const std::string printed = testing::internal::GetCapturedStderr();
    EXPECT_EQ(printed, "error 1\nwarning 2\n");
    ASSERT_EQ(csoundGetMessageCnt(csound), 2);
    EXPECT_EQ(csoundGetFirstMessageAttr(csound), CSOUNDMSG_ERROR);
    EXPECT_STREQ(csoundGetFirstMessage(csound), "error 1\n");
    csoundPopFirstMessage(csound);
    EXPECT_EQ(csoundGetFirstMessageAttr(csound), CSOUNDMSG_WARNING);
    EXPECT_STREQ(csoundGetFirstMessage(csound), "warning 2\n");
}

#endif

TEST_F (MessageBufferTests, testBufferPreservesHostData)
{
    int hostData = 42;
    csoundSetHostData(csound, &hostData);
    csoundCreateMessageBuffer(csound, 1);
    EXPECT_EQ(csoundGetHostData(csound), &hostData);
    csoundDestroyMessageBuffer(csound);
    EXPECT_EQ(csoundGetHostData(csound), &hostData);
    csoundSetHostData(csound, nullptr);
}

#if GTEST_HAS_DEATH_TEST
TEST_F (MessageBufferTests, testConsoleWriteFailureStillBuffersMessage)
{
    // Isolate stdout's FILE state from the rest of the test process.
    ASSERT_EXIT({
        if (freopen(__FILE__, "r", stdout) == nullptr)
            exit(2);
        csoundCreateMessageBuffer(csound, 1);
        csoundMessage(csound, "message %d", 42);
        const char *message = csoundGetFirstMessage(csound);
        exit(csoundGetMessageCnt(csound) == 1 && message != nullptr &&
             strcmp(message, "message 42") == 0 ? 0 : 1);
    }, ::testing::ExitedWithCode(0), "");
}
#endif
