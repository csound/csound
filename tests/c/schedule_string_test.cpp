#include "gtest/gtest.h"
#include <string>
#include <vector>

#define __BUILDING_LIBCSOUND
#include "csoundCore.h"

namespace {

struct ScheduleForm {
    const char *opcode;
    const char *target;
    bool control;
    double fraction;
};

class ScheduleStringTests : public ::testing::TestWithParam<ScheduleForm> {
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

    void run(const std::string &first, const std::string &second)
    {
        const auto form = GetParam();
        std::string calls = std::string(form.opcode) + " " + form.target +
            ",.02,.008,Sfirst,0.123456789123,Ssecond,1e-20\n" +
            form.opcode + " 3,.02,.008\n";
        if (form.control)
            calls = "kOnce init 0\nif kOnce == 0 then\n" + calls +
                    "endif\nkOnce = 1\n";
        const std::string orc =
            "sr=1000\nksmps=4\nnchnls=1\n0dbfs=1\ngiCalls init 0\n"
            "instr Target\nSfirst = p4\nSsecond = p6\n"
            "chnset Sfirst,\"first\"\nchnset Ssecond,\"second\"\n"
            "chnset p5,\"number\"\nchnset p7,\"small\"\n"
            "chnset p1,\"identity\"\nchnset p2,\"time\"\n"
            "giCalls += 1\nchnset giCalls,\"calls\"\nendin\n"
            "instr 3\nchnset p2,\"reference\"\nendin\n"
            "instr 1\nSfirst chnget \"input1\"\nSsecond chnget \"input2\"\n"
            "kTarget init nstrnum(\"Target\") + .25\n" + calls +
            "Sfirst strcpyk \"changed\"\nSsecond strcpyk \"changed\"\n"
            "endin\nchnset nstrnum(\"Target\"),\"targetNumber\"\n";
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), CSOUND_SUCCESS)
            << messages();
        csoundSetStringChannel(csound, "input1", first.c_str());
        csoundSetStringChannel(csound, "input2", second.c_str());
        csoundEventString(csound, "i 1 0 .05\ni 1 .1 .05\ne", 0);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS) << messages();
        int result = 0;
        for (int cycle = 0; cycle < 100 && result == 0; ++cycle)
            result = csoundPerformKsmps(csound);
        EXPECT_GT(result, 0) << messages();
        EXPECT_EQ(csound->inerrcnt, 0) << messages();
        EXPECT_EQ(csound->perferrcnt, 0) << messages();
    }

    MYFLT number(const char *name)
    {
        int error = 0;
        MYFLT value = csoundGetControlChannel(csound, name, &error);
        EXPECT_EQ(error, CSOUND_SUCCESS);
        return value;
    }

    std::string string(const char *name)
    {
        int size = csoundGetChannelDatasize(csound, name);
        EXPECT_GT(size, 0);
        if (size <= 0) return {};
        std::vector<char> text(size);
        csoundGetStringChannel(csound, name, text.data());
        return text.data();
    }
    CSOUND *csound = nullptr;
};

TEST_P(ScheduleStringTests, PreservesStringsNumbersAndEventTime)
{
    const std::string first = "quotes: \"text\"\npath: C:\\audio\\";
    // Two strings exercise the event's string indices and independent lengths.
    const std::string second = std::string(20000, 'x') + "\\\"end\\";
    ASSERT_NO_FATAL_FAILURE(run(first, second));
    EXPECT_EQ(string("first"), first);
    EXPECT_EQ(string("second"), second);
    EXPECT_EQ(number("number"), static_cast<MYFLT>(0.123456789123));
    EXPECT_EQ(number("small"), static_cast<MYFLT>(1e-20));
    EXPECT_EQ(number("identity"), number("targetNumber") + GetParam().fraction);
    EXPECT_EQ(number("time"), number("reference"));
    EXPECT_EQ(number("calls"), 2);
}

TEST_P(ScheduleStringTests, PreservesEmptyStringFields)
{
    ASSERT_NO_FATAL_FAILURE(run("", ""));
    EXPECT_EQ(string("first"), "");
    EXPECT_EQ(string("second"), "");
    EXPECT_EQ(number("calls"), 2);
}

TEST_P(ScheduleStringTests, StringPfieldDoesNotReplaceInstrumentIdentity)
{
    ASSERT_NO_FATAL_FAILURE(run("Target.75", ""));
    EXPECT_EQ(string("first"), "Target.75");
    EXPECT_EQ(number("identity"), number("targetNumber") + GetParam().fraction);
    EXPECT_EQ(number("calls"), 2);
}

INSTANTIATE_TEST_SUITE_P(Forms, ScheduleStringTests, ::testing::Values(
    ScheduleForm{"schedule", "nstrnum(\"Target\") + .25", false, .25},
    ScheduleForm{"schedule", "\"Target.25\"", false, .25},
    ScheduleForm{"schedule", "Target", false, 0},
    ScheduleForm{"schedulek", "kTarget", true, .25},
    ScheduleForm{"schedulek", "\"Target.25\"", true, .25},
    ScheduleForm{"schedulek", "Target", true, 0}));

} // namespace
