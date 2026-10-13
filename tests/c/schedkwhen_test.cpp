#include "gtest/gtest.h"
#include <string>

#define __BUILDING_LIBCSOUND
#include "csoundCore.h"

namespace {

struct SchedulerForm {
    const char *opcode;
    const char *target;
    const char *definition;
};

class SchedkwhenTests : public ::testing::TestWithParam<SchedulerForm> {
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

    void run(const std::string &source, const std::string &receiver,
             const char *score)
    {
        const std::string orc =
            "sr=1000\nksmps=1\nnchnls=1\n0dbfs=1\ngiEvents init 0\n"
            "instr " +
            std::string(GetParam().definition) + "\n" + receiver +
            "\ngiEvents += 1\nchnset giEvents,\"events\"\nendin\n"
            "instr 1\n" + source + "\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), CSOUND_SUCCESS)
            << messages();
        csoundEventString(csound, score, 0);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS) << messages();
        int status = 0;
        for (int cycle = 0; cycle < 1000 && status == 0; ++cycle)
            status = csoundPerformKsmps(csound);
        EXPECT_GT(status, 0) << messages();
        EXPECT_EQ(csound->inerrcnt, 0) << messages();
        EXPECT_EQ(csound->perferrcnt, 0) << messages();
    }

    cs_float channel(const std::string &name)
    {
        int error = 0;
        cs_float value = csoundGetControlChannel(csound, name.c_str(), &error);
        EXPECT_EQ(error, CSOUND_SUCCESS);
        return value;
    }

    std::string call(const std::string &prefix, const char *duration = "0")
    {
        return std::string(GetParam().opcode) + " " + prefix + "," +
            GetParam().target + ",0," + duration;
    }

    CSOUND *csound = nullptr;
};

TEST_P(SchedkwhenTests, PreservesPfieldsAtInitAndPerformance)
{
    const int count = 640;
    std::string source =
        "kTrig init p4\nkTrig = (p4 == 0 && timeinstk() == 2 ? 1 : 0)\n" +
        call("kTrig,0,0");
    for (int i = 1; i <= count; ++i)
        source += "," + std::to_string(i);
    ASSERT_NO_FATAL_FAILURE(run(source,
        "iFields[] passign 4\n"
        "iBad = (lenarray(iFields) == 640 ? 0 : 1)\n"
        "iIndex = 0\nwhile iIndex < lenarray(iFields) do\n"
        "if iFields[iIndex] != iIndex + 1 then\niBad += 1\nendif\n"
        "iIndex += 1\nod\n"
        "Sname sprintf \"bad%d\", giEvents\nchnset iBad,Sname\n",
        "i 1 0 .01 1\ni 1 .02 .01 0\ne"));
    EXPECT_EQ(channel("events"), 2);
    EXPECT_EQ(channel("bad0"), 0);
    EXPECT_EQ(channel("bad1"), 0);
}

TEST_P(SchedkwhenTests, ChangingMinimumIntervalAdjustsPendingCountdown)
{
    // Start at ten samples, extend to twenty, then shorten to five.
    ASSERT_NO_FATAL_FAILURE(run(
        "kTrig init 0\nkMin init .01\nkTrig = 1\n"
        "kCycle timeinstk\nif kCycle == 3 then\nkMin = .02\n"
        "elseif kCycle == 25 then\nkMin = .005\nendif\n" +
        call("kTrig,kMin,0"),
        "Sname sprintf \"time%d\", giEvents\nchnset p2,Sname\n",
        "i 1 0 .04\ni 1 .1 .04\ne"));
    ASSERT_EQ(channel("events"), 10);
    const cs_double gaps[] = {0, .02, .025, .03, .035};
    for (int note = 0; note < 2; ++note) {
        const cs_float first = channel("time" + std::to_string(note * 5));
        for (int i = 1; i < 5; ++i)
            EXPECT_NEAR(channel("time" + std::to_string(note * 5 + i)) - first,
                        gaps[i], 1e-6);
    }
}

TEST_P(SchedkwhenTests, LimitsActiveInstances)
{
    ASSERT_NO_FATAL_FAILURE(run(
        "kTrig init 0\nkTrig = 1\n" + call("kTrig,0,1", ".01"),
        "", "i 1 0 .035\ne"));
    EXPECT_EQ(channel("events"), 4);
}

INSTANTIATE_TEST_SUITE_P(Forms, SchedkwhenTests, ::testing::Values(
    SchedulerForm{"schedkwhen", "2", "2"},
    SchedulerForm{"schedkwhen", "\"Target\"", "Target"},
    SchedulerForm{"schedkwhennamed", "2", "2"},
    SchedulerForm{"schedkwhennamed", "\"Target\"", "Target"},
    SchedulerForm{"schedkwhen", "Target", "Target"}));

} // namespace
