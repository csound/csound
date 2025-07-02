#include <stdio.h>
#include <string.h>
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
