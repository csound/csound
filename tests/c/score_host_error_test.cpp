#include "csoundCore.h"
#include "gtest/gtest.h"
#include <atomic>
#include <string>
#include <thread>
#include <tuple>

namespace {
const char *orchestra =
    "sr=48000\nksmps=32\nnchnls=1\n0dbfs=1\n"
    "instr 1\nchnset chnget:i(\"played\") + 1, \"played\"\nendin\n"
    "instr 2\nchnset readscore(\"}\"), \"read\"\nendin\n";

enum Path { BeforeStart, Blocking, Queued };
const char *pathNames[] = {"BeforeStart", "Blocking", "Queued"};

class Host {
public:
    Host()
    {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        csoundSetOption(csound, "-d");
        EXPECT_EQ(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra, 0));
    }
    ~Host() { csoundDestroy(csound); }

    /* In a function of its own, as a host does it: the frame that csoundStart
       leaves in exitjmp is gone when a line is sent. */
    void start() { ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound)); }
    void perform(int cycles = 8)
    {
        for (int i = 0; i < cycles; i++)
            ASSERT_EQ(0, csoundPerformKsmps(csound)) << said();
    }
    /* Before csoundStart a line is the score, and may end. */
    void performToScoreEnd()
    {
        for (int i = 0; i < 8 && csoundPerformKsmps(csound) == 0; i++)
            ;
    }
    cs_float played() { return csoundGetControlChannel(csound, "played", nullptr); }
    std::string said()
    {
        while (csoundGetMessageCnt(csound)) {
            messages += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return messages;
    }
    void send(const char *line, Path path)
    {
        if (path == BeforeStart) {
            csoundEventString(csound, line, 0);
            start();
        }
        else {
            start();
            csoundEventString(csound, line, path == Queued);
        }
    }

    CSOUND *csound;

private:
    std::string messages;
};

struct Refused {
    const char *line;
    const char *said;
};

/* Each is a line the reader cannot take: nothing of it plays. */
const Refused refused[] = {
    {"}", "unmatched } in score"},
    {"{a,b}", "invalid repeat count"},
    {"#loop", "#loop outside a repeat"},
    {"{ 2 N\n#loop\n}", "#loop outside a repeat"},
    {"i 1 0 0 \"x", "unexpected EOF"},
    {"i 1 0 0 \"a\\\" \" } \"", "unmatched } in score"},
    {"i 1 0 0 [1", "illegal character"},
    {"i 1 0 0 [", "illegal character"},
    {"i 1 0 0 []", "missing operand"},
    {"i 1 0 0 [e]", "illegal character"},
    {"i 1 0 0 [2*/3]", "illegal placement of operator"},
    {"i 1 0 0 [2/*3]", "unexpected EOF"},
    {"i 1 0 0 1 /* x", "unexpected EOF"},
    {"/*", "unexpected EOF"},
    {"r 2\n#define X #r 3#\n$X", "push-back overflow"},
    {"#define R(a) #$R(1)#\n$R(1)", "nested too deep"},
    {"m a\nn a", "marked sections nested too deeply"},
    {"#undef X", "Undefining undefined macro"},
    {"#define Y #1#\n#undef X", "Undefining undefined macro"},
    {"i 1 0 0\n}\ni 1 0 0", "unmatched } in score"},
};

class HostScoreErrors
    : public ::testing::TestWithParam<std::tuple<Refused, Path>> {};

TEST_P(HostScoreErrors, AreRefusedAndThePerformanceGoesOn)
{
    const Refused &line = std::get<0>(GetParam());
    Host host;
    host.send(line.line, std::get<1>(GetParam()));
    host.perform();
    EXPECT_EQ(0, host.played());
    const std::string said = host.said();
    EXPECT_NE(std::string::npos, said.find(line.said)) << said;
    EXPECT_NE(std::string::npos, said.find("score refused")) << said;
    EXPECT_EQ(0, csoundErrCnt(host.csound));

    csoundEventString(host.csound, "i 1 0 0", 0);
    host.perform();
    EXPECT_EQ(1, host.played());
}

std::string lineName(
    const ::testing::TestParamInfo<HostScoreErrors::ParamType> &info)
{
    return pathNames[std::get<1>(info.param)] + std::string("_") +
           std::to_string(info.index);
}

INSTANTIATE_TEST_SUITE_P(
    Lines, HostScoreErrors,
    ::testing::Combine(::testing::ValuesIn(refused),
                       ::testing::Values(BeforeStart, Blocking, Queued)),
    lineName);

/* These lack what they need: the reader says so and goes on, as it does for a
   .csd. The last three read and write past an event's p-fields at develop,
   which only a sanitizer shows. */
const char *warned[] = {
    "n", "n none", "b", "B", "C", "v", "y { 2 N", "a", "d 1 0", "f -1", "f 0 0",
};

class HostScoreWarnings
    : public ::testing::TestWithParam<std::tuple<const char *, Path>> {};

TEST_P(HostScoreWarnings, LeaveThePerformanceGoingOn)
{
    const Path path = std::get<1>(GetParam());
    Host host;
    host.send(std::get<0>(GetParam()), path);
    if (path == BeforeStart) {
        host.performToScoreEnd();
        return;
    }
    host.perform();
    csoundEventString(host.csound, "i 1 0 0", 0);
    host.perform();
    EXPECT_GE(host.played(), 1);
}

std::string warnedName(
    const ::testing::TestParamInfo<HostScoreWarnings::ParamType> &info)
{
    return pathNames[std::get<1>(info.param)] + std::string("_") +
           std::to_string(info.index);
}

INSTANTIATE_TEST_SUITE_P(
    Lines, HostScoreWarnings,
    ::testing::Combine(::testing::ValuesIn(warned),
                       ::testing::Values(BeforeStart, Blocking, Queued)),
    warnedName);

struct Warned {
    const char *line;
    const char *said;
};

/* The reader went past the end of these, into what followed them: a lone carry
   letter lost its own note, and a statement with no p-field the next one. */
const Warned overrun[] = {
    {"i 1 0 0 n", "illegal reference to n   Zero substituted"},
    {"i 1 0 0 p", "illegal reference to p   Zero substituted"},
    {"q\ni 1 0 0", "too few pfields"},
    {"d\ni 1 0 0", "too few pfields"},
    {"i\ni 1 0 0", "too few pfields"},
    {"f\ni 1 0 0", "too few pfields"},
    {"i 1 0 0\nq", "too few pfields"},
};

class HostScoreOverruns
    : public ::testing::TestWithParam<std::tuple<Warned, Path>> {};

TEST_P(HostScoreOverruns, StopAtTheirOwnText)
{
    const Warned &line = std::get<0>(GetParam());
    Host host;
    host.send(line.line, std::get<1>(GetParam()));
    host.perform();
    EXPECT_EQ(1, host.played());
    const std::string said = host.said();
    EXPECT_NE(std::string::npos, said.find(line.said)) << said;
}

std::string overrunName(
    const ::testing::TestParamInfo<HostScoreOverruns::ParamType> &info)
{
    return pathNames[std::get<1>(info.param)] + std::string("_") +
           std::to_string(info.index);
}

INSTANTIATE_TEST_SUITE_P(Lines, HostScoreOverruns,
                         ::testing::Combine(::testing::ValuesIn(overrun),
                                            ::testing::Values(Blocking, Queued)),
                         overrunName);

TEST(HostScoreErrors, LongNamesStayInTheirBuffers)
{
    const std::string name(300, 'a');
    Host host;
    host.start();
    for (const std::string &line :
         {"m " + name, "n " + name, "{ $" + name + "\ni 1 0 0\n}"}) {
        SCOPED_TRACE(line);
        csoundEventString(host.csound, line.c_str(), 0);
        host.perform();
    }
    csoundEventString(host.csound, "i 1 0 0", 0);
    host.perform();
    EXPECT_EQ(1, host.played());
}

TEST(HostScoreErrors, MarksEndWithTheirNames)
{
    Host host;
    host.start();
    std::string marks;
    for (int i = 0; i < 3000; i++)
        marks += "m a" + std::to_string(i) + "\n";
    EXPECT_EQ(CSOUND_ERROR, host.csound->ReadScore(host.csound, marks.c_str()));
    EXPECT_NE(std::string::npos, host.said().find("too many marked sections"));
    host.perform();
    csoundEventString(host.csound, "i 1 0 0", 0);
    host.perform();
    EXPECT_EQ(1, host.played());
}

/* A mark is a place in the text it was read from. */
TEST(HostScoreErrors, MarksEndWithTheirRead)
{
    std::string text = "m a\n";
    for (int i = 0; i < 500; i++)
        text += "i 1 100 0\n";
    Host host;
    host.start();
    csoundEventString(host.csound, (text + "m b\ns").c_str(), 0);
    csoundEventString(host.csound, "n b", 0);
    EXPECT_NE(std::string::npos, host.said().find("Name b not found"));
    host.perform();
}

TEST(HostScoreErrors, ReadScoreReturnsTheRefusal)
{
    Host host;
    host.start();
    EXPECT_EQ(CSOUND_ERROR, host.csound->ReadScore(host.csound, "}"));
    EXPECT_EQ(CSOUND_SUCCESS, host.csound->ReadScore(host.csound, "i 1 0 0"));
    host.perform();
    EXPECT_EQ(1, host.played());
}

TEST(HostScoreErrors, RefusedBeforeStartLeavesTheScoreToTheNextLine)
{
    Host host;
    csoundEventString(host.csound, "}", 0);
    csoundEventString(host.csound, "i 1 0 0", 0);
    host.start();
    host.performToScoreEnd();
    EXPECT_EQ(1, host.played());
}

TEST(HostScoreErrors, ReadscoreGetsTheRefusalAndItsInstrumentGoesOn)
{
    Host host;
    host.start();
    csoundEventString(host.csound, "i 2 0 0\ni 1 0 0", 0);
    host.perform();
    EXPECT_EQ(-1, csoundGetControlChannel(host.csound, "read", nullptr));
    EXPECT_EQ(1, host.played());
    EXPECT_EQ(0, csoundErrCnt(host.csound));
}

/* The reader's jump is its own thread's: the other one never sees it. */
TEST(HostScoreErrors, RefusedBesideARunningPerformance)
{
    Host host;
    host.start();
    std::atomic<bool> stop{false};
    std::atomic<int> result{0};
    std::thread performance([&] {
        while (!stop && result == 0)
            result = csoundPerformKsmps(host.csound);
    });
    for (int i = 0; i < 100; i++) {
        csoundEventString(host.csound, "}", 0);
        csoundEventString(host.csound, "{a,b}", 0);
        csoundEventString(host.csound, "i 1 0 0 [", 0);
    }
    stop = true;
    performance.join();
    EXPECT_EQ(0, result);
    EXPECT_EQ(0, csoundErrCnt(host.csound));
}

/* The guards refuse nothing that was read before. */
TEST(HostScoreErrors, RepeatsStillPlay)
{
    Host host;
    host.start();
    csoundEventString(host.csound, "{ 2 N\ni 1 0 0\n}", 0);
    host.perform();
    EXPECT_EQ(2, host.played());
    csoundEventString(host.csound, "{ 2 N\n{ 3 M\ni 1 0 0\n}\n}", 0);
    host.perform();
    EXPECT_EQ(8, host.played());
}

TEST(HostScoreErrors, ExitEndsWhatItsCallSent)
{
    Host host;
    host.start();
    csoundEventString(host.csound, "i 1 0 0\n#exit\n}", 0);
    host.perform();
    EXPECT_EQ(1, host.played());
}

/* A .csd is no host's line: it stops, as it did. */
class CsdScoreErrors : public ::testing::TestWithParam<Refused> {};

TEST_P(CsdScoreErrors, StopTheCompile)
{
    const Refused &line = GetParam();
    Host host;
    const std::string csd =
        std::string("<CsoundSynthesizer>\n<CsInstruments>\n") + orchestra +
        "</CsInstruments>\n<CsScore>\n" + line.line +
        "\n</CsScore>\n</CsoundSynthesizer>\n";
    EXPECT_NE(CSOUND_SUCCESS, csoundCompileCSD(host.csound, csd.c_str(), 1, 0));
    const std::string said = host.said();
    EXPECT_NE(std::string::npos, said.find(line.said)) << said;
    EXPECT_EQ(std::string::npos, said.find("score refused")) << said;
}

INSTANTIATE_TEST_SUITE_P(Lines, CsdScoreErrors, ::testing::ValuesIn(refused));
} // namespace
