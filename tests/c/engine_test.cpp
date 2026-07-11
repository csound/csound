#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
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

TEST_F (EngineTests, testRealtimeAsyncCompileMergesOnEventThread)
{
    csoundSetOption(csound, "-n");
    csoundSetOption(csound, "-d");
    csoundSetOption(csound, "--realtime");
    ASSERT_EQ(csoundCompileOrc(csound, R"(
      sr = 48000
      ksmps = 64
      nchnls = 1
      0dbfs = 1
      instr 1
      endin
    )", 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);

    ASSERT_EQ(csoundCompileOrc(csound, R"(
      instr AsyncMerged
      endin
    )", 1), CSOUND_SUCCESS);

    int32_t insno = 0;
    for (int32_t i = 0; i < 200 && insno <= 0; ++i) {
      csoundSleep(10);
      insno = csoundGetInstrNumber(csound, "AsyncMerged");
    }

    ASSERT_GT(insno, 0);
}

TEST_F (EngineTests, testRealtimeAllocQueueRejectsOverflow)
{
    csound->alloc_queue = static_cast<ALLOC_DATA *>(
      csound->Calloc(csound, sizeof(ALLOC_DATA) * MAX_ALLOC_QUEUE));
    ASSERT_NE(csound->alloc_queue, nullptr);
    ASSERT_EQ(csoundSpinLockInit(&csound->alloc_queue_spinlock),
              CSOUND_SUCCESS);
    ATOMIC_SET(csound->alloc_queue_items, 0);
    csound->alloc_queue_wp = 0;

    for (int32_t i = 0; i < MAX_ALLOC_QUEUE; ++i) {
      ALLOC_DATA data = { 0 };
      data.insno = i + 1;
      ASSERT_EQ(alloc_queue_enqueue(csound, &data), CSOUND_SUCCESS)
        << "queue item " << i;
    }

    ASSERT_EQ(ATOMIC_GET(csound->alloc_queue_items), MAX_ALLOC_QUEUE);
    ASSERT_EQ(csound->alloc_queue_wp, 0u);
    for (int32_t i = 0; i < MAX_ALLOC_QUEUE; ++i)
      ASSERT_EQ(csound->alloc_queue[i].insno, i + 1);

    ALLOC_DATA overflow = { 0 };
    overflow.insno = MAX_ALLOC_QUEUE + 1;
    ASSERT_EQ(alloc_queue_enqueue(csound, &overflow), CSOUND_ERROR);
    ASSERT_EQ(ATOMIC_GET(csound->alloc_queue_items), MAX_ALLOC_QUEUE);
    ASSERT_EQ(csound->alloc_queue_wp, 0u);
    ASSERT_EQ(csound->alloc_queue[0].insno, 1);

    int32_t realtime = csound->oparms->realtime;
    EVTBLK event = { 0 };
    MCHNBLK channel = { 0 };
    MEVENT midi = { 0 };
    csound->oparms->realtime = 1;
    ASSERT_EQ(insert_event(csound, 1, &event), CSOUND_ERROR);
    ASSERT_EQ(insert_midi_event(csound, 1, &channel, &midi), CSOUND_ERROR);
    csound->oparms->realtime = realtime;

    ATOMIC_SET(csound->alloc_queue_items, 0);
    csound->alloc_queue_wp = 0;
    csound->Free(csound, csound->alloc_queue);
    csound->alloc_queue = nullptr;
}
