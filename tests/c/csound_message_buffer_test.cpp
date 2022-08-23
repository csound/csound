#include <stdio.h>
#include <string.h>
#include "csound.h"
#include "gtest/gtest.h"

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
        csoundSetGlobalEnv ("OPCODE6DIR64", "../../");
        csound = csoundCreate (0);
        csoundCreateMessageBuffer (csound, 0);
        //csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundCleanup (csound);
        csoundDestroyMessageBuffer (csound);
        csoundDestroy (csound);
    }

    CSOUND* csound = nullptr;
};

TEST_F (MessageBufferTests, testCreateBuffer)
{
    int argc = 2;
    const char *argv[] = {"csound", "-v"};
    csoundCompile(csound, argc, argv);

    int cnt = csoundGetMessageCnt(csound);
    ASSERT_TRUE (cnt > 0);
    const char * msg = csoundGetFirstMessage(csound);
    ASSERT_TRUE (msg != NULL);
    int newcnt = csoundGetMessageCnt(csound);
    ASSERT_EQ (cnt, newcnt);
    csoundPopFirstMessage(csound);
    newcnt = csoundGetMessageCnt(csound);
    ASSERT_EQ (cnt - 1, newcnt);
}

TEST_F (MessageBufferTests, testBufferRun)
{
    int result = csoundCompileOrc(csound, "instr 1\n"
                                  "asig oscil 0.1, 440\n"
                                  "out asig\n"
                                  "endin\n");
    csoundReadScore(csound, "i 1 0 0.1\n");
    csoundStart(csound);

    csoundPerform(csound);

    while (csoundGetMessageCnt(csound)) {
        const char * msg = csoundGetFirstMessage(csound);
        ASSERT_TRUE (msg != NULL);
        csoundPopFirstMessage(csound);
        printf("CSOUND MESSAGE: %s", msg);
    }
}
