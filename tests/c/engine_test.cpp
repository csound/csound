#include "csound.h"
#include "csound_graph_display.h"
#include <stdio.h>
#include "gtest/gtest.h"
#include "time.h"

class EngineTests : public ::testing::Test {
public:
    EngineTests ()
    {
    }

    virtual ~EngineTests ()
    {
    }

    virtual void SetUp ()
    {
      csound = csoundCreate (NULL,NULL);
      csoundCreateMessageBuffer (csound, 0);
      csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

TEST_F (EngineTests, testUdpServer)
{
    csoundSetIsGraphable(csound, 1);
    csoundSetOption(csound,"--port=12345");
    csoundStart(csound);
    csoundSleep(1000);
}

TEST_F (EngineTests, testScoreOffset)
{
    csoundSetOption(csound,"-n");
    csoundEventString(csound, R"(
    i 1 0 1
    )", 0);
    csoundCompileOrc(csound, R"(
      instr 1
      endin
    )", 0);
                      
    csoundStart(csound);
    csoundSetScoreOffsetSeconds(csound, 1);
    ASSERT_TRUE(csoundPerformKsmps(csound) != 0); 
}

TEST_F (EngineTests, testScoreRewind)
{
    csoundSetOption(csound,"-n");
    csoundEventString(csound, R"(
    i 1 0 1
    )", 0);
    csoundCompileOrc(csound, R"(
      instr 1
      prints "Top...\n";
      endin
    )", 0);
                      
    csoundStart(csound);
    while(csoundPerformKsmps(csound) == 0)
      ;
    csoundRewindScore(csound);
    ASSERT_TRUE(csoundPerformKsmps(csound) == 0); 
}
