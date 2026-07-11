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

TEST_F (EngineTests, testCommandLineArgumentsApi)
{
    char firstArgument[] = "concert.orc";
    const char *arguments[] = {
      firstArgument,
      "first violin",
      "--quiet",
      ""
    };

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 4, arguments), CSOUND_SUCCESS);
    firstArgument[0] = 'X';

    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 4);
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 0), "concert.orc");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 1), "first violin");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 2), "--quiet");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 3), "");
    ASSERT_EQ(csoundGetCommandLineArg(csound, -1), nullptr);
    ASSERT_EQ(csoundGetCommandLineArg(csound, 4), nullptr);
    ASSERT_EQ(csoundEvalCode(csound, R"(
      Sarguments:S[] argv
      return lenarray(Sarguments)
    )"), 4);

    const char *invalidArguments[] = {nullptr};
    ASSERT_EQ(csoundSetCommandLineArgs(csound, 1, invalidArguments),
              CSOUND_ERROR);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 4);

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 0, nullptr), CSOUND_SUCCESS);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 0);
    ASSERT_EQ(csoundEvalCode(csound, R"(
      Sarguments:S[] argv
      return lenarray(Sarguments)
    )"), 0);

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 4, arguments), CSOUND_SUCCESS);
    csoundReset(csound);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 0);
}

TEST_F (EngineTests, testCommandLineArgumentSeparator)
{
    const char *staleArguments[] = {"stale"};
    const char *arguments[] = {
      "csound",
      "-n",
      "--suppress-version",
      "--code=instr 1\nendin",
      "--",
      "concert.orc",
      "first violin",
      "--logfile=ignored",
      ""
    };

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 1, staleArguments),
              CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompile(csound, 9, arguments), CSOUND_SUCCESS);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 4);
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 0), "concert.orc");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 1), "first violin");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 2), "--logfile=ignored");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 3), "");
}

TEST_F (EngineTests, testExplicitCommandLineArgumentsSurviveCompile)
{
    const char *applicationArguments[] = {"concert.orc", "first violin"};
    const char *compileArguments[] = {
      "csound",
      "-n",
      "--suppress-version",
      "--code=instr 1\nendin"
    };

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 2, applicationArguments),
              CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompile(csound, 4, compileArguments), CSOUND_SUCCESS);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 2);
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 0), "concert.orc");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 1), "first violin");
}

TEST_F (EngineTests, testEmptyCommandLineArgumentSeparator)
{
    const char *staleArguments[] = {"stale"};
    const char *compileArguments[] = {
      "csound",
      "-n",
      "--suppress-version",
      "--code=instr 1\nendin",
      "--"
    };

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 1, staleArguments),
              CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompile(csound, 5, compileArguments), CSOUND_SUCCESS);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 0);
}
