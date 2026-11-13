#include "csoundCore.h"
#include "gtest/gtest.h"
#include <atomic>
#include <string>
#include <thread>
#include <tuple>
#if defined(__GLIBC__) && (__GLIBC__ > 2 || __GLIBC_MINOR__ >= 33)
#include <malloc.h>
#define HEAP_IN_USE() (mallinfo2().uordblks)
#endif

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
        for (int i = 0; i < 8; i++) {
            const int result = csoundPerformKsmps(csound);
            ASSERT_GE(result, 0) << said();
            if (result > 0)
                return;
        }
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
    {"{ [1)] N\ni 1 0 0\n}", "unmatched ')'"},
    {"{ [3+[1)]]\ni 1 0 0\n}", "unmatched ')'"},
    {"{ [(1] N\ni 1 0 0\n}", "unmatched ']'"},
    {"{ [1/0] N\ni 1 0 0\n}", "division error"},
    {"{ [1%0] N\ni 1 0 0\n}", "division error"},
    {"{ [(0-2147483647-1)/(0-1)] N\ni 1 0 0\n}", "division error"},
    {"r 2\n{ 2 N\ns\n}", "repeat and section cross"},
    /* A comment hides an s or r from the lexer until the repeat plays. */
    {"r 2\n/**/r 2\n{ 2 N\n/**/s\n}\n", "repeat and section cross"},
    {"r 2\n{ 2 A\n{ 2 B\ni 1 0 0\n/**/s\n}\n}\n", "repeat and section cross"},
    {"#define M #{ 2 N\n/**/s\n}\n#\nr 2\n$/**/M\ni 1 0 0\ns\n}",
     "repeat and section cross"},
    {"{ 2 N\nr 2\n}\ns", "repeat and section cross"},
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
    "n", "n none", "b", "B", "C", "v", "y { 2 N", "a", "{ 2 N\ni 1 0 $N(1)\n}",
    "d 1 0", "f -1", "f 0 0",
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
    int plays;
};

/* The reader went past the end of these, into what followed them: a lone carry
   letter lost its own note, and a statement with no p-field the next one. */
const Warned overrun[] = {
    {"i 1 0 0 n", "illegal reference to n   Zero substituted", 1},
    {"i 1 0 0 p", "illegal reference to p   Zero substituted", 1},
    {"q\ni 1 0 0", "too few pfields", 1},
    {"d\ni 1 0 0", "too few pfields", 1},
    {"i\ni 1 0 0", "too few pfields", 1},
    {"f\ni 1 0 0", "too few pfields", 1},
    {"i 1 0 0\nq", "too few pfields", 1},
    /* Two notes that refer to each other went round until the stack ran out. */
    {"i 1 0 0 np4\ni 1 0 0 pp4", "illegal reference to pp4   Zero substituted", 2},
    /* #undef freed a counter its repeat goes on using, however it was spelled. */
    {"{ 2 N\ni 1 0 0\n#undef N\n}", "#undef of a repeat's counter ignored", 2},
    {"{ 2 N\n#un/**/def N\ni 1 0 0\n}", "#undef of a repeat's counter ignored", 2},
    {"r 2 N\ni 1 0 0\n#undef N\ns", "#undef of a repeat's counter ignored", 2},
    /* A statement lacking its number took the next one with it. */
    {"b\ni 1 0 0", "b without a number", 1},
    {"C\ni 1 0 0", "C without a number", 1},
    {"v\ni 1 0 0", "v without a number", 1},
    /* A macro called with () deleted the macro before it. */
    {"#define B #1#\n#define A(x) #2#\ni 1 0 0 $B()", "", 1},
    /* Valid: what ends a macro's text ends at the end of its buffer. */
    {"#define S #s#\ni 1 0 0\n$S\ni 1 0 0", "", 2},
    /* An r begun in a repeat's playing outlives it, and crosses nothing. */
    {"{ 1 A\n/**/r 2\n}\ni 1 0 0\n{ 2 B\ni 1 0 0\n}\ns\n", "", 6},
    /* An unclosed repeat or section went without a word. */
    {"{ 2 N\ni 1 0 0", "unclosed repeat", 0},
    {"r 2\ni 1 0 0", "unclosed repeat", 0},
};

class HostScoreOverruns
    : public ::testing::TestWithParam<std::tuple<Warned, Path>> {};

TEST_P(HostScoreOverruns, StopAtTheirOwnText)
{
    const Warned &line = std::get<0>(GetParam());
    Host host;
    host.send(line.line, std::get<1>(GetParam()));
    host.perform();
    EXPECT_EQ(line.plays, host.played());
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
         {"m " + name, "n " + name, "{ $" + name + "\ni 1 0 0\n}",
          "{ [$" + name + "] N\ni 1 0 0\n}",
          "#define " + std::string(1000, 'a') + "(x) #$x#",
          "{ [" + std::string(45, '(') + "1] N\ni 1 0 0\n}",
          "{ " + std::string(120000, '[')}) {
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

/* The score's marks live past the end of the 7.0.0 layout, in storage of
   their own: 1024 fit, n finds the last, far past the 30 they once had, and
   one more does not fit. */
TEST(HostScoreErrors, MarksFitTo1024AndNoFurther)
{
    std::string marks;
    for (int i = 0; i < 1024; i++)
        marks += "m a" + std::to_string(i) + "\n";
    Host host;
    host.start();
    EXPECT_EQ(CSOUND_SUCCESS,
              host.csound->ReadScore(host.csound,
                                     (marks + "i 1 0 0\ns\nn a1023\n").c_str()));
    EXPECT_EQ(std::string::npos, host.said().find("too many marked sections"));
    host.perform(16);
    EXPECT_EQ(2, host.played()) << host.said();
    EXPECT_EQ(CSOUND_ERROR,
              host.csound->ReadScore(host.csound,
                                     (marks + "m a1024\n").c_str()));
    EXPECT_NE(std::string::npos, host.said().find("too many marked sections"));
}

/* Engine/memalloc.c's block header: a Debug build defines BETA, which turns
   MEMDEBUG on. */
struct Block {
#if defined(BETA) || defined(MEMDEBUG)
    int32_t magic;
    void *ptr;
#endif
    Block *prv, *nxt;
    void *base;
};

/* Csound's own count of what it holds. */
size_t trackedBlocks(CSOUND *csound)
{
    size_t n = 0;
    for (auto *b = static_cast<Block *>(csound->memalloc_db); b; b = b->nxt)
        n++;
    return n;
}

/* Whether p is a block Csound holds, which a reset frees. */
bool isTracked(CSOUND *csound, const void *p)
{
    const size_t header = (sizeof(Block) + 15) & ~size_t(15);
    for (auto *b = static_cast<Block *>(csound->memalloc_db); b; b = b->nxt)
        if (reinterpret_cast<const char *>(b) + header == p)
            return true;
    return false;
}

/* The buffer sense_line takes lines into is Csound's memory, so a reset frees
   it, and the reset clears the pointer with it. */
TEST(HostScoreErrors, LineEventBufferDoesNotOutliveAReset)
{
    Host host;
    host.start();
    host.csound->InputMessage(host.csound, "i 1 0 0\n");
    host.perform();
    EXPECT_EQ(1, host.played()) << host.said();
    EXPECT_TRUE(isTracked(host.csound, host.csound->line_event_work));
    csoundReset(host.csound);
    EXPECT_EQ(nullptr, host.csound->line_event_work);
    EXPECT_EQ(0, host.csound->line_event_work_size);
    csoundSetOption(host.csound, "-n");
    csoundSetOption(host.csound, "-d");
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileOrc(host.csound, orchestra, 0));
    host.start();
    host.csound->InputMessage(host.csound, "i 1 0 0\n");
    host.perform();
    EXPECT_EQ(1, host.played()) << host.said();
}

/* A refused line takes what it held with it, whatever it was reading when it
   stopped, so a host can go on sending bad lines. */
TEST(HostScoreErrors, RefusalsKeepNothing)
{
    std::string notes;
    for (int i = 0; i < 100; i++)
        notes += "i 1 100 0\n";
    const std::string lines[] = {
        "#define M #" + std::string(1000, ' ') + "#\n}", /* the macros, the sort */
        notes + "}",                                    /* the text expanded */
        "{ [1)] N\ni 1 0 0\n}",                         /* a counter being read */
        "#define X(a #",                                /* a definition cut off */
        "#define Y(a) #$a#\ni 1 0 0 $Y(" + std::string(90, '0'), /* an argument */
        "i 1 0 0\n{ 2 N\ni 1 0 0\n{ 3 M\ni 1 0 0\n}\n}\n}", /* a repeat's body */
        "r 2 N\n{ 2 M\ni 1 0 0\n#un/**/def M\n}\n}",     /* an r's and a repeat's */
    };
    Host host;
    host.start();
    size_t seen = 0;
    auto refuse = [&](const std::string &line) {
        csoundEventString(host.csound, line.c_str(), 0);
        csoundPerformKsmps(host.csound);
        const std::string said = host.said();
        EXPECT_NE(std::string::npos, said.find("score refused", seen));
        seen = said.size();
    };
    const size_t blocks = trackedBlocks(host.csound);
    void *block = host.csound->Malloc(host.csound, 16);
    EXPECT_EQ(blocks + 1, trackedBlocks(host.csound)) << "the walk is wrong";
    host.csound->Free(host.csound, block);
    for (const std::string &line : lines) {
        SCOPED_TRACE(line.substr(0, 40));
        refuse(line);
        const size_t before = trackedBlocks(host.csound);
        for (int i = 0; i < 5; i++)
            refuse(line);
        EXPECT_EQ(before, trackedBlocks(host.csound));
    }
}

/* A read that succeeds keeps nothing for the next: the expanded text and the
   input stack are reused, and its marks are freed. Each read kept its own
   until a reset, about 700 bytes for a short line. */
TEST(HostScoreReads, KeepNothing)
{
    std::string notes, again = "m a\ni 1 0 0\ns\n";
    for (int i = 0; i < 100; i++)
        notes += "i 1 0 0\n";
    for (int i = 0; i < 30; i++)
        again += "n a\n";
    const std::pair<std::string, int> reads[] = {
        {"i 1 0 0", 1},              /* the expanded text, the input stack */
        {"m a\ni 1 0 0\ns\nn a", 2}, /* a mark, and the stack n reads on */
        {again, 31},                 /* n after n: one at a time on the stack */
        {notes, 100},                /* more text than the reads before */
        {"i 1 0 0", 1},              /* less: none of the longer one plays */
    };
    Host host;
    host.start();
    csoundEventString(host.csound, "i 1 0 0", 0); /* what a first read makes */
    for (const auto &[line, notesRead] : reads) {
        SCOPED_TRACE(line.substr(0, 40));
        const int32_t stack = host.csound->sread.input_size;
        csoundEventString(host.csound, line.c_str(), 0);
        host.perform();
        const size_t before = trackedBlocks(host.csound);
        const int32_t text = host.csound->expanded_sco->len;
        const cs_float played = host.played();
        for (int i = 0; i < 5; i++) {
            csoundEventString(host.csound, line.c_str(), 0);
            host.perform();
        }
        EXPECT_EQ(before, trackedBlocks(host.csound));
        EXPECT_EQ(text, host.csound->expanded_sco->len);
        EXPECT_EQ(stack, host.csound->sread.input_size);
        EXPECT_EQ(played + 5 * notesRead, host.played()) << host.said();
    }
}

/* A .csd compiled into a running instance keeps no more for its score than
   for none: the score's sorted copy was kept too. Compiling the orchestra
   keeps blocks of its own, some of the time, so each kind is counted at its
   least over five compiles. */
TEST(HostScoreReads, CompiledScoresKeepNothing)
{
    const char *withScore = "<CsoundSynthesizer>\n<CsInstruments>\n"
                            "</CsInstruments>\n<CsScore>\ni 1 0 0\n"
                            "</CsScore>\n</CsoundSynthesizer>\n";
    const char *without = "<CsoundSynthesizer>\n<CsInstruments>\n"
                          "</CsInstruments>\n</CsoundSynthesizer>\n";
    Host host;
    host.start();
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileCSD(host.csound, withScore, 1, 0));
    host.perform();
    auto least = [&](const char *csd) {
        size_t fewest = SIZE_MAX;
        for (int i = 0; i < 5; i++) {
            const size_t before = trackedBlocks(host.csound);
            EXPECT_EQ(CSOUND_SUCCESS, csoundCompileCSD(host.csound, csd, 1, 0));
            host.perform();
            if (trackedBlocks(host.csound) - before < fewest)
                fewest = trackedBlocks(host.csound) - before;
        }
        return fewest;
    };
    EXPECT_EQ(least(without), least(withScore));
    /* Queued, the score is still read from the queue's own copy. */
    EXPECT_EQ(CSOUND_SUCCESS, csoundCompileCSD(host.csound, withScore, 1, 1));
    host.perform();
    EXPECT_EQ(7, host.played()) << host.said();
}

/* An r that ends an r left open puts back the text of an s. In a macro, flex
   may have no room for it, and the read is refused: a copy of the r's text,
   made with malloc, was kept each time. */
TEST(HostScoreErrors, PushBackOverflowKeepsNothing)
{
#ifndef HEAP_IN_USE
    GTEST_SKIP() << "counts glibc's heap";
#else
    const char *line = "#define X #r 3\n#\nr 2\ni 1 0 0\n$X\ns";
    Host host;
    host.start();
    csoundEventString(host.csound, line, 0);
    EXPECT_NE(std::string::npos, host.said().find("push-back overflow"));
    auto refuse = [&](int times) {
        for (int i = 0; i < times; i++) {
            csoundEventString(host.csound, line, 0);
            csoundPerformKsmps(host.csound);
            while (csoundGetMessageCnt(host.csound) > 0)
                csoundPopFirstMessage(host.csound);
        }
    };
    refuse(20);
    const size_t before = HEAP_IN_USE();
    refuse(200);
    const long long kept = (long long) HEAP_IN_USE() - (long long) before;
    EXPECT_LT(kept, 1000) << "bytes kept by 200 refusals";
    EXPECT_EQ(0, host.played());
#endif
}

/* A reset frees what the reads reused, and the next read makes its own. */
TEST(HostScoreReads, AfterAResetMakeTheirOwn)
{
    Host host;
    host.start();
    csoundEventString(host.csound, "i 1 0 0", 0);
    host.perform();
    csoundReset(host.csound);
    EXPECT_EQ(nullptr, host.csound->expanded_sco);
    EXPECT_EQ(nullptr, host.csound->sread.inputs);
    csoundSetOption(host.csound, "-n");
    csoundSetOption(host.csound, "-d");
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileOrc(host.csound, orchestra, 0));
    host.start();
    csoundEventString(host.csound, "i 1 0 0", 0);
    host.perform();
    EXPECT_EQ(1, host.played()) << host.said();
    EXPECT_TRUE(isTracked(host.csound, host.csound->expanded_sco));
    EXPECT_TRUE(isTracked(host.csound, host.csound->sread.inputs));
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
    csoundEventString(host.csound, "r 2\n{ 2 N\ni 1 0 0\n}\ns", 0);
    host.perform();
    EXPECT_EQ(12, host.played());
    csoundEventString(host.csound, "{ 2 N\nr 2\ni 1 0 0\ns\n}", 0);
    host.perform();
    EXPECT_EQ(16, host.played());
    /* An expression that fills its stacks exactly. */
    csoundEventString(host.csound, ("{ [" + std::string(29, '(') + "3" +
                                    std::string(29, ')') + "]\ni 1 0 0\n}").c_str(), 0);
    host.perform();
    EXPECT_EQ(19, host.played());
}

/* An r ended over a repeat nothing closes crosses nothing that goes on: it
   plays, as it did. */
TEST(CsdScores, PlayWithARepeatOpenInASection)
{
    Host host;
    const std::string csd =
        std::string("<CsoundSynthesizer>\n<CsInstruments>\n") + orchestra +
        "</CsInstruments>\n<CsScore>\nr 2\n{ 2 N\ni 1 0 0\n"
        "</CsScore>\n</CsoundSynthesizer>\n";
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileCSD(host.csound, csd.c_str(), 1, 0));
    host.start();
    host.performToScoreEnd();
    EXPECT_EQ(2, host.played());
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
