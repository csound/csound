#include "gtest/gtest.h"
#include <string>
#include <vector>

#define __BUILDING_LIBCSOUND
#include "csoundCore.h"

namespace {

class TrigseqTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    }
    void TearDown() override { csoundDestroy(csound); }

    std::string messages()
    {
        std::string result;
        while (csoundGetMessageCnt(csound)) {
            result += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return result;
    }

    void start(const std::string &body, const char *score = "i 1 0 .05")
    {
        const std::string orc =
            "sr=1000\nksmps=1\nnchnls=1\n0dbfs=1\n"
            "giTable ftgen 1,0,-12,-2,10,11,20,21,30,31,40,41,50,51,60,61\n"
            "giShort ftgen 2,0,-6,-2,100,101,200,201,300,301\n"
            "giPartial ftgen 3,0,-5,-2,10,11,20,21,999\n"
            "instr 1\nkA init -1\nkB init -2\n" + body +
            "\nchnset kA,\"a\"\nchnset kB,\"b\"\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), CSOUND_SUCCESS)
            << messages();
        csoundEventString(csound, score, 0);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS) << messages();
    }

    MYFLT value(const char *name)
    {
        int error = 0;
        MYFLT result = csoundGetControlChannel(csound, name, &error);
        EXPECT_EQ(error, CSOUND_SUCCESS);
        return result;
    }

    void expect(const std::vector<int> &values)
    {
        for (int expected : values) {
            ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
            EXPECT_EQ(value("a"), expected);
            EXPECT_EQ(value("b"), expected + 1);
        }
        EXPECT_EQ(csound->inerrcnt, 0) << messages();
        EXPECT_EQ(csound->perferrcnt, 0) << messages();
    }

    CSOUND *csound = nullptr;
};

TEST_F(TrigseqTests, OneShotIncludesFinalGroup)
{
    ASSERT_NO_FATAL_FAILURE(start("trigseq 1,3,3,0,giTable,kA,kB"));
    expect({10,20,30,40,40,40});
}

TEST_F(TrigseqTests, OneShotCanEndAtGroupZero)
{
    ASSERT_NO_FATAL_FAILURE(start("trigseq 1,0,0,0,giTable,kA,kB"));
    expect({10,10,10});
}

TEST_F(TrigseqTests, OneShotCanStartAtFinalGroup)
{
    ASSERT_NO_FATAL_FAILURE(start("trigseq 1,5,5,5,giTable,kA,kB"));
    expect({60,60,60});
}

TEST_F(TrigseqTests, ForwardLoopPreservesIntroAndExclusiveEnd)
{
    ASSERT_NO_FATAL_FAILURE(start("trigseq 1,2,5,0,giTable,kA,kB"));
    expect({10,20,30,40,50,30,40,50,30});
}

TEST_F(TrigseqTests, ReverseLoopPreservesOrder)
{
    ASSERT_NO_FATAL_FAILURE(start("trigseq 1,2,-5,4,giTable,kA,kB"));
    expect({50,40,30,50,40,30,50});
}

TEST_F(TrigseqTests, ReverseLoopWrapsAnInitialGroupBelowStart)
{
    ASSERT_NO_FATAL_FAILURE(start("trigseq 1,2,-5,0,giTable,kA,kB"));
    expect({10,30,50,40,30,50});
}

TEST_F(TrigseqTests, ZeroTriggersHoldOutputsAndPosition)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "kTrig = timeinstk() % 2\ntrigseq kTrig,1,4,0,giTable,kA,kB"));
    expect({10,10,20,20,30,30,40,40,20});
}

TEST_F(TrigseqTests, TableChangeUsesTheNewGroups)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "kTable init 1\nif timeinstk() == 3 then\nkTable = 2\nendif\n"
        "trigseq 1,0,3,0,kTable,kA,kB"));
    expect({10,20,300,100,200,300});
}

TEST_F(TrigseqTests, ChangingLoopDirectionPreservesPosition)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "kLoop init 5\nif timeinstk() == 3 then\nkLoop = -5\nendif\n"
        "trigseq 1,1,kLoop,0,giTable,kA,kB"));
    expect({10,20,30,20,50,40,30});
}

TEST_F(TrigseqTests, TableChangeUsesTheLookupRoundingRule)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "kTable init 1.25\nif timeinstk() == 2 then\nkTable = 1.75\nendif\n"
        "trigseq 1,0,3,0,kTable,kA,kB"));
    const bool roundedUp = round(FL(1.75)) == 2;
    expect({10, roundedUp ? 200 : 20, roundedUp ? 300 : 30});
}

TEST_F(TrigseqTests, ReusedNotesRestartCompletedOneShot)
{
    ASSERT_NO_FATAL_FAILURE(start("trigseq 1,2,2,0,giTable,kA,kB",
        "i 1 0 .004\ni 1 .004 .02"));
    expect({10,20,30,30,10,20,30,30});
}

TEST_F(TrigseqTests, ReinitRestartsCompletedOneShot)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "if timeinstk() == 5 then\nreinit Restart\nendif\n"
        "Restart:\ntrigseq 1,2,2,0,giTable,kA,kB\nrireturn"));
    expect({10,20,30,30,10,20,30,30});
}

TEST_F(TrigseqTests, RejectsPartialInitialGroup)
{
    ASSERT_NO_FATAL_FAILURE(start("trigseq 1,0,2,2,giPartial,kA,kB"));
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->inerrcnt, 0);
    EXPECT_NE(messages().find("trigseq: initial group out of range"),
              std::string::npos);
}

TEST_F(TrigseqTests, RejectsPendingGroupAfterTableShrinks)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "kTable init 1\nkLoop init 6\n"
        "if timeinstk() == 4 then\nkTable = 2\nkLoop = 3\nendif\n"
        "trigseq 1,0,kLoop,0,kTable,kA,kB"));
    ASSERT_NO_FATAL_FAILURE(expect({10,20,30}));
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->perferrcnt, 0);
    EXPECT_NE(messages().find("trigseq: group or loop out of range"),
              std::string::npos);
}

class TrigseqRangeTests : public TrigseqTests,
                          public ::testing::WithParamInterface<const char *> {};

TEST_P(TrigseqRangeTests, RejectsInvalidLoop)
{
    ASSERT_NO_FATAL_FAILURE(start(std::string("trigseq 1,") + GetParam() +
                                  ",0,giTable,kA,kB"));
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->perferrcnt, 0);
    EXPECT_NE(messages().find("trigseq: group or loop out of range"),
              std::string::npos);
}

INSTANTIATE_TEST_SUITE_P(Ranges, TrigseqRangeTests, ::testing::Values(
    "-1,3", "0,7", "3,2", "2,-2", "4,-3", "0,-7", "0,1e30"));

} // namespace
