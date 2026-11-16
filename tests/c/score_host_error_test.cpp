#include "csoundCore.h"
#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <tuple>
#include <vector>
#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif
#if defined(__GLIBC__) && (__GLIBC__ > 2 || __GLIBC_MINOR__ >= 33)
#include <malloc.h>
#define HEAP_IN_USE() (mallinfo2().uordblks)
#endif

namespace {
const char *orchestra =
    "sr=48000\nksmps=32\nnchnls=1\n0dbfs=1\n"
    "instr 1\nchnset chnget:i(\"played\") + 1, \"played\"\nendin\n"
    "instr 2\nchnset readscore(\"}\"), \"read\"\nendin\n"
    "instr 3\nchnset pcount(), \"fields\"\nchnset p(pcount()), \"last\"\nendin\n"
    "instr 4\nS1 = p4\nchnset S1, \"text\"\nendin\n";

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
    /* An error inside a nested [] gave 0, and the count went on with it: the
       first two played once. */
    {"{ [1+[*2]] N\ni 1 0 0\n}", "illegal placement of operator *"},
    {"{ [3*[x]+1] N\ni 1 0 0\n}", "illegal character x"},
    {"{ [2+[$U]] N\ni 1 0 0\n}", "Undefined macro: 'U'"},
    /* A ~ ended the count, and what followed it was lost: this played 3 times. */
    {"{ [3~+1] N\ni 1 0 0\n}", "illegal character ~"},
    /* A number right after another, a ) or a ] was one operand too many, and
       a value was lost: at develop these played 5, 5 and 3 times. */
    {"{ [1+2 3] N\ni 1 0 0\n}", "illegal placement of number"},
    {"{ [[5]2] N\ni 1 0 0\n}", "illegal placement of number"},
    {"{ [(2)3] N\ni 1 0 0\n}", "illegal placement of number"},
    /* A count past INT_MAX wrapped round: each of these played 2 or 5 times. */
    {"{ 4294967298 N\ni 1 0 0\n}", "invalid repeat count"},
    {"#define C #4294967298#\n{ $C N\ni 1 0 0\n}", "invalid repeat count"},
    {"#define C #-4294967294#\n{ $C N\ni 1 0 0\n}", "invalid repeat count"},
    {"{ [4294967298] N\ni 1 0 0\n}", "integer overflow in [] expression"},
    {"{ [2147483647+2147483647+4] N\ni 1 0 0\n}", "integer overflow"},
    {"{ [0-2147483647-2147483647+3] N\ni 1 0 0\n}", "integer overflow"},
    {"{ [65536*65536+2] N\ni 1 0 0\n}", "integer overflow"},
    {"r 4294967298\ni 1 0 0\ns", "r: invalid repeat count"},
    /* A minus sign was dropped: these played 13 and 5 times. A p-field's []
       refuses them too. */
    {"{ [-(2+3)+8] N\ni 1 0 0\n}", "illegal placement of '('"},
    {"{ [-[2]+5] N\ni 1 0 0\n}", "illegal placement of '['"},
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
    /* Valid: counts that fit play as they did, INT_MAX among their terms. */
    {"{ [2147483647-2147483645] N\ni 1 0 0\n}", "", 2},
    {"#define C #2#\n{ $C N\ni 1 0 0\n}", "", 2},
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

/* A statement goes on into a line that begins with a p-field, a [] too: an n
   after the [] is no statement, and the rest of a b's line is dropped after
   it, as after a number. */
TEST(HostScoreReads, ABracketBeginsAContinuedLineAsAField)
{
    Host host;
    host.start();
    EXPECT_EQ(CSOUND_SUCCESS,
              host.csound->ReadScore(host.csound,
                                     "m a\ni 1 0 0\ns\ni 1 0 0\n[0] n a\n"
                                     "b\n[0] 7\ni 1 0 0\n"));
    host.perform(16);
    const std::string said = host.said();
    EXPECT_EQ(3, host.played()) << said;
    EXPECT_EQ(std::string::npos, said.find("illegal opcode")) << said;
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

/* What the reader says of n and y reaches the host's messages, as m's does,
   not the process's stdout. */
TEST(HostScoreReads, SayNAndYThroughTheMessages)
{
    Host host;
    host.start();
    EXPECT_EQ(CSOUND_SUCCESS,
              host.csound->ReadScore(host.csound,
                                     "m a\ni 1 0 0\ns\nn a\ny 5\ny\n"));
    const std::string said = host.said();
    EXPECT_NE(std::string::npos, said.find("m Named section >>>a<<<")) << said;
    EXPECT_NE(std::string::npos, said.find("n Named section a")) << said;
    EXPECT_NE(std::string::npos, said.find("seed from score 5")) << said;
    EXPECT_NE(std::string::npos, said.find("seed from clock")) << said;
}

/* A score error says where it is, each part on a line of its own, where it
   said there was an internal error and ran on into the next message. */
TEST(HostScoreErrors, SayTheirLineAndPosition)
{
    Host host;
    host.start();
    EXPECT_EQ(CSOUND_ERROR,
              host.csound->ReadScore(host.csound, "i 1 0 0\n\ni 1 0 0 [1 2]\n"));
    const std::string said = host.said();
    EXPECT_NE(std::string::npos,
              said.find("illegal placement of number in [] expression\n")) << said;
    EXPECT_NE(std::string::npos,
              said.find(":  at position 11\n  in line 3 of the score\n")) << said;
    EXPECT_EQ(std::string::npos, said.find("Internal error")) << said;
}

/* A warning's place is the character it is about, or the statement that lacks
   something, not a blank line read after it. */
TEST(HostScoreErrors, WarningsSayTheirLineAndPosition)
{
    Host host;
    host.start();
    host.csound->ReadScore(host.csound, "i 1 0 0 x\n");
    host.csound->ReadScore(host.csound, "i 1 0 0\nb\n\ni 1 0 0\n");
    const std::string said = host.said();
    EXPECT_NE(std::string::npos,
              said.find("unexpected char x\n  section ")) << said;
    EXPECT_NE(std::string::npos,
              said.find(":  at position 8\n  in line 1 of the score\n")) << said;
    EXPECT_NE(std::string::npos,
              said.find("b without a number ignored\n  section ")) << said;
    EXPECT_NE(std::string::npos,
              said.find(":  at position 0\n  in line 2 of the score\n")) << said;
}

/* A malformed number says where it is while the reader reads it, here in a
   [], and not when the sort reads it again after the reader, here a t
   statement's, the last of its read: the reader's place is elsewhere then. */
TEST(HostScoreErrors, NumbersSayTheirPlaceOnlyWhileRead)
{
    Host host;
    host.start();
    host.csound->ReadScore(host.csound, "i 1 0 0 [1.2.3]\n");
    host.csound->ReadScore(host.csound, "i 1 0 0\nt 0 6e\n");
    const std::string said = host.said();
    EXPECT_NE(std::string::npos,
              said.find("1.2.3   zero substituted.\n  section")) << said;
    const std::string number = "6e   zero substituted.\n";
    const size_t at = said.find(number);
    ASSERT_NE(std::string::npos, at) << said;
    EXPECT_NE(0, said.compare(at + number.size(), 9, "  section")) << said;
}

/* A mark read again, here by a repeat of the section it is in, moves to where
   it is read: it takes no entry of its own, so 1024 marks still fit. */
TEST(HostScoreErrors, MarksReadAgainTakeNoNewEntry)
{
    std::string marks;
    for (int i = 0; i < 1024; i++)
        marks += "m a" + std::to_string(i) + "\n";
    Host host;
    host.start();
    EXPECT_EQ(CSOUND_SUCCESS,
              host.csound->ReadScore(host.csound,
                                     (marks + "i 1 0 0\ns\nn a0\n").c_str()));
    EXPECT_EQ(std::string::npos, host.said().find("too many marked sections"));
    host.perform(16);
    EXPECT_EQ(2, host.played()) << host.said();
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
    EXPECT_TRUE(
        csound_is_tracked_block(host.csound, host.csound->line_event_work));
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
    const size_t blocks = csound_tracked_block_count(host.csound);
    void *block = host.csound->Malloc(host.csound, 16);
    EXPECT_EQ(blocks + 1, csound_tracked_block_count(host.csound));
    EXPECT_TRUE(csound_is_tracked_block(host.csound, block));
    EXPECT_FALSE(csound_is_tracked_block(host.csound, &seen));
    host.csound->Free(host.csound, block);
    EXPECT_EQ(blocks, csound_tracked_block_count(host.csound));
    for (const std::string &line : lines) {
        SCOPED_TRACE(line.substr(0, 40));
        refuse(line);
        const size_t before = csound_tracked_block_count(host.csound);
        for (int i = 0; i < 5; i++)
            refuse(line);
        EXPECT_EQ(before, csound_tracked_block_count(host.csound));
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
        const size_t before = csound_tracked_block_count(host.csound);
        const int32_t text = host.csound->expanded_sco->len;
        const cs_float played = host.played();
        for (int i = 0; i < 5; i++) {
            csoundEventString(host.csound, line.c_str(), 0);
            host.perform();
        }
        EXPECT_EQ(before, csound_tracked_block_count(host.csound));
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
            const size_t before = csound_tracked_block_count(host.csound);
            EXPECT_EQ(CSOUND_SUCCESS, csoundCompileCSD(host.csound, csd, 1, 0));
            host.perform();
            if (csound_tracked_block_count(host.csound) - before < fewest)
                fewest = csound_tracked_block_count(host.csound) - before;
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
    EXPECT_TRUE(
        csound_is_tracked_block(host.csound, host.csound->expanded_sco));
    EXPECT_TRUE(
        csound_is_tracked_block(host.csound, host.csound->sread.inputs));
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

/* Before the start, a read after the first is queued, and its notes play at
   their times. The e that the read was given ended the performance at once,
   and so did one of its own. */
TEST(HostScoreReads, LaterReadsBeforeStartPlay)
{
    for (const char *later : {"i 1 0.01 0\ni 1 0.02 0",
                              "i 1 0.01 0\ni 1 0.02 0\ne"}) {
        SCOPED_TRACE(later);
        Host host;
        csoundEventString(host.csound, "i 1 0 0.1", 0);
        csoundEventString(host.csound, later, 0);
        host.start();
        int cycles = 0;
        while (cycles < 1000 && csoundPerformKsmps(host.csound) == 0)
            cycles++;
        EXPECT_EQ(3, host.played()) << host.said();
        EXPECT_EQ(150, cycles); /* the first read's 0.1 s */
    }
    /* On a started engine, an e still ends it. */
    Host host;
    host.start();
    csoundEventString(host.csound, "i 1 0 1\ne", 0);
    EXPECT_NE(0, csoundPerformKsmps(host.csound));
}

#ifndef _WIN32
/* While it lives, this process's stdin is /dev/null: -L stdin reads stdin,
   and leaves it non-blocking until the performance's cleanup. */
struct NullStdin {
    int saved = dup(0);
    bool replaced = false;
    NullStdin()
    {
        const int null = open("/dev/null", O_RDONLY);
        if (null > 0) {
            replaced = dup2(null, 0) == 0;
            close(null);
        }
        else
            replaced = null == 0; /* stdin was closed, and this took its place */
    }
    ~NullStdin()
    {
        if (saved != -1) {
            dup2(saved, 0);
            close(saved);
        }
        else
            close(0);
    }
};
#endif

/* With -L, they play too. The start made a new line buffer for -L, and
   what was queued before it was lost; it registered the line reader a
   second time; and with -L stdin it died on the descriptor that the queued
   line had set to none. */
TEST(HostScoreReads, LaterReadsBeforeStartPlayWithLineInput)
{
    const std::string file = ::testing::TempDir() + "csound-later-reads.sco";
    std::ofstream(file).close(); /* -L reads nothing from it */
    std::vector<std::string> inputs = {"\"" + file + "\""};
#ifndef _WIN32
    inputs.push_back("stdin");
#endif
    for (const auto &input : inputs) {
        SCOPED_TRACE(input);
#ifndef _WIN32
        std::optional<NullStdin> nullStdin;
        if (input == "stdin" && !nullStdin.emplace().replaced)
            continue; /* never the real stdin */
#endif
        Host host;
        csoundSetOption(host.csound, ("-L" + input).c_str());
        csoundEventString(host.csound, "i 1 0 0.1", 0);
        csoundEventString(host.csound, "i 1 0.01 0\ni 1 0.02 0", 0);
        ASSERT_NO_FATAL_FAILURE(host.start()) << host.said();
        int readers = 0;
        for (auto *fp = (EVT_CB_FUNC *) host.csound->evtFuncChain; fp != nullptr;
             fp = fp->nxt)
            readers++;
        EXPECT_EQ(1, readers);
        int cycles = 0;
        while (cycles < 1000 && csoundPerformKsmps(host.csound) == 0)
            cycles++;
        EXPECT_EQ(3, host.played()) << host.said();
        EXPECT_EQ(150, cycles);
    }
    std::remove(file.c_str());
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

/* 200 short notes fill most of the reader's first 16 KB of text. */
std::string shortNotes(int count = 200)
{
    std::string notes;
    for (int i = 0; i < count; i++)
        notes += "i 1 0 0\n";
    return notes;
}

/* "i 3 0 0", count copies of field, then 7. */
std::string longNote(int count, const std::string &field = "1")
{
    std::string note = "i 3 0 0";
    for (int i = 0; i < count; i++)
        note += " " + field;
    return note + " 7";
}

void readAll(Host &host, const std::string &score, Path path)
{
    host.send(score.c_str(), path);
    if (path == BeforeStart)
        host.performToScoreEnd();
    else
        host.perform();
}

/* A b, B, C or v whose number grows the text space goes on from its own
   statement, not from the space the growth freed. */
class HostScoreGrowth
    : public ::testing::TestWithParam<std::tuple<const char *, Path>> {};

TEST_P(HostScoreGrowth, LeavesBCAndVInPlace)
{
    const std::string score = shortNotes() + std::get<0>(GetParam()) +
                              std::string(1000, '0') + "\n" +
                              longNote(10000, "22");
    Host host;
    readAll(host, score, std::get<1>(GetParam()));
    EXPECT_EQ(200, host.played()) << host.said();
    EXPECT_EQ(10004, csoundGetControlChannel(host.csound, "fields", nullptr));
    EXPECT_EQ(7, csoundGetControlChannel(host.csound, "last", nullptr));
}

INSTANTIATE_TEST_SUITE_P(Numbers, HostScoreGrowth,
                         ::testing::Combine(::testing::Values("b 0", "B 0",
                                                              "C 1", "v 1."),
                                            ::testing::Values(BeforeStart,
                                                              Blocking, Queued)));

TEST(CsdScores, LeaveBInPlaceWhenTheSpaceGrows)
{
    Host host;
    const std::string csd =
        std::string("<CsoundSynthesizer>\n<CsInstruments>\n") + orchestra +
        "</CsInstruments>\n<CsScore>\n" + shortNotes() + "b 0" +
        std::string(1000, '0') + "\n" + longNote(10000, "22") +
        "\n</CsScore>\n</CsoundSynthesizer>\n";
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileCSD(host.csound, csd.c_str(), 1, 0));
    host.start();
    host.performToScoreEnd();
    EXPECT_EQ(200, host.played());
    EXPECT_EQ(10004, csoundGetControlChannel(host.csound, "fields", nullptr));
}

/* The reader leaves a margin of 4 KB past its text space at each statement,
   and checked only long fields against it: each of these outran it. */
struct LongStatement {
    const char *name;
    std::string score;
    int played; /* instr 1's notes */
    int fields; /* the last instr 3 note's p-fields */
};

std::vector<LongStatement> longStatements()
{
    return {
        {"OneCharacterFields", longNote(12000), 0, 12004},
        {"AfterNotes", shortNotes() + longNote(3000), 200, 3004},
        {"Expressions", longNote(3000, "[1]"), 0, 3004},
        {"SectionLine", "i 1 0 0\ns" + std::string(30000, ' ') + "\n" + longNote(1), 1, 5},
        {"CarriedFields", longNote(10000, "11") + "\ni 3 0", 0, 10004},
        /* p-fields 65537 to 65539 were taken for p1 to p3 */
        {"Over65535Fields", longNote(66000, "22"), 0, 66004},
        /* and past 32767 the count was negative to the carry */
        {"Over32767FieldsAfterANote",
         "i 3 0 0 5\ni 3 0.002" + longNote(40000, "22").substr(5), 0, 40004},
        /* each one's end in the string buffer, which only characters grew */
        {"EmptyStrings", longNote(2000, "\"\""), 0, 2004},
    };
}

class HostLongStatements
    : public ::testing::TestWithParam<std::tuple<LongStatement, Path>> {};

TEST_P(HostLongStatements, AreReadInFull)
{
    const LongStatement &statement = std::get<0>(GetParam());
    Host host;
    readAll(host, statement.score, std::get<1>(GetParam()));
    EXPECT_EQ(statement.played, host.played()) << host.said();
    EXPECT_EQ(statement.fields, csoundGetControlChannel(host.csound, "fields", nullptr));
    EXPECT_EQ(7, csoundGetControlChannel(host.csound, "last", nullptr));
}

std::string longName(
    const ::testing::TestParamInfo<HostLongStatements::ParamType> &info)
{
    return std::get<0>(info.param).name + std::string("_") +
           pathNames[std::get<1>(info.param)];
}

INSTANTIATE_TEST_SUITE_P(Statements, HostLongStatements,
                         ::testing::Combine(::testing::ValuesIn(longStatements()),
                                            ::testing::Values(BeforeStart,
                                                              Blocking, Queued)),
                         longName);

/* Statements that end on either side of the first text space's end and of
   its margin's, a field at a time, from its start and from near its end. */
TEST(HostScoreReads, LongStatementsAroundTheEdges)
{
    const std::tuple<int, int, int> ranges[] = {
        {0, 10190, 10215}, {200, 140, 170}, {200, 2190, 2215}};
    Host host;
    host.start();
    for (const auto &[notes, from, to] : ranges)
        for (int count = from; count <= to; count++) {
            SCOPED_TRACE(std::to_string(notes) + " notes, " +
                         std::to_string(count) + " fields");
            const std::string score = shortNotes(notes) + longNote(count);
            csoundEventString(host.csound, score.c_str(), 0);
            host.perform(1);
            ASSERT_EQ(count + 4,
                      csoundGetControlChannel(host.csound, "fields", nullptr));
        }
}

TEST(CsdScores, ReadALongStatementInFull)
{
    Host host;
    const std::string csd =
        std::string("<CsoundSynthesizer>\n<CsInstruments>\n") + orchestra +
        "</CsInstruments>\n<CsScore>\n" + longNote(12000) +
        "\n</CsScore>\n</CsoundSynthesizer>\n";
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileCSD(host.csound, csd.c_str(), 1, 0));
    host.start();
    host.performToScoreEnd();
    EXPECT_EQ(12004, csoundGetControlChannel(host.csound, "fields", nullptr));
}

/* A line with no newline, as scoreline sends one, that grows the line-event
   buffer, from 32 KB: the buffer grew to its end, and its newline went past. */
TEST(HostScoreReads, LinesWithoutANewlineFitTheBufferTheyGrow)
{
    Host host;
    host.start();
    for (int size = 32760; size <= 32800; size++) {
        std::string line(size, ' ');
        line.replace(0, 7, "i 1 0 0");
        host.csound->InputMessage(host.csound, line.c_str());
        host.perform(1);
        ASSERT_EQ(size - 32759, host.played()) << size;
    }
}

/* A string p-field arrives whole, however long: from a host's line, the
   line event's buffer grew under it, and the string went on from the
   wrong place. */
class HostLongStrings : public ::testing::TestWithParam<std::tuple<int, Path>> {};

TEST_P(HostLongStrings, ArriveWhole)
{
    std::string text;
    for (int i = 0; i < std::get<0>(GetParam()); i++)
        text += (char) ('a' + i % 26);
    Host host;
    readAll(host, "i 4 0 0 \"" + text + "\"", std::get<1>(GetParam()));
    std::string got(csoundGetChannelDatasize(host.csound, "text") + 1, '\0');
    csoundGetStringChannel(host.csound, "text", got.data());
    got.resize(strlen(got.c_str()));
    EXPECT_EQ(text.size(), got.size()) << host.said();
    EXPECT_TRUE(text == got);
}

INSTANTIATE_TEST_SUITE_P(Lengths, HostLongStrings,
                         ::testing::Combine(::testing::Values(1013, 1014, 3000,
                                                              100000),
                                            ::testing::Values(BeforeStart,
                                                              Blocking, Queued)));

/* A file name of its own in the temporary directory, removed with it: the
   cases run side by side under ctest -j. */
struct TempFile {
    std::string name;
    TempFile(const char *stem, const char *extension)
        : name(::testing::TempDir() + stem +
               std::to_string(
                   std::chrono::steady_clock::now().time_since_epoch().count()) +
               "-" + std::to_string(std::random_device{}()) + extension)
    {
    }
    TempFile(const TempFile &) = delete;
    TempFile &operator=(const TempFile &) = delete;
    ~TempFile() { std::remove(name.c_str()); }
};

std::string channelText(Host &host)
{
    std::string text(csoundGetChannelDatasize(host.csound, "text") + 1, '\0');
    csoundGetStringChannel(host.csound, "text", text.data());
    text.resize(strlen(text.c_str()));
    return text;
}

/* A line event's string goes on over the next lines to its closing quote,
   but no further than the lines taken: one with no closing quote is refused,
   and the next line plays. The string read on past the lines' end, and
   whatever it took in and the lines after it were lost, or the process died. */
TEST(HostLineEvents, AnUnclosedQuoteIsRefusedAndTheNextLinePlays)
{
    Host host;
    host.start();
    host.csound->InputMessage(host.csound, "i 1 0 0 \"abc");
    host.csound->InputMessage(host.csound, "i 1 0 0 7");
    host.perform(1);
    EXPECT_EQ(1, host.played()) << host.said();
    EXPECT_NE(std::string::npos, host.said().find("unmatched quote"));
    host.csound->InputMessage(host.csound, "i 4 0 0 \"ab\ncd\"");
    host.perform(1);
    EXPECT_EQ("ab\ncd", channelText(host)) << host.said();
}

/* The event copies a line's strings: the buffer they were read into was kept
   until a reset, 1 KB or more for each line with a string, played or
   refused. */
TEST(HostLineEvents, StringsKeepNothing)
{
    Host host;
    host.start();
    for (const char *line : {"i 4 0 0 \"x\"", "i 4 0 0 \"x\" y"}) {
        SCOPED_TRACE(line);
        host.csound->InputMessage(host.csound, line);
        host.perform(1);
        const size_t before = csound_tracked_block_count(host.csound);
        for (int i = 0; i < 5; i++) {
            host.csound->InputMessage(host.csound, line);
            host.perform(1);
        }
        EXPECT_EQ(before, csound_tracked_block_count(host.csound));
    }
    EXPECT_EQ("x", channelText(host)) << host.said();
}

cs_float channel(Host &host, const char *name)
{
    return csoundGetControlChannel(host.csound, name, nullptr);
}

/* An orchestra block, from a line starting with { to its }, is compiled
   from a host's lines, sent before the start or after it, with -L or
   without, one case each. Only -L made the block's buffer: without it, the
   process died at the block's first line. */
class HostOrchestraBlocks
    : public ::testing::TestWithParam<std::tuple<bool, bool>> {};

TEST_P(HostOrchestraBlocks, AreCompiledWithOrWithoutLineInput)
{
    const bool lineInput = std::get<0>(GetParam());
    const bool beforeStart = std::get<1>(GetParam());
    const TempFile file("csound-block-lines-", ".sco");
    std::ofstream(file.name).close(); /* -L reads nothing from it */
    Host host;
    if (lineInput)
        csoundSetOption(host.csound, ("-L\"" + file.name + "\"").c_str());
    if (!beforeStart) {
        ASSERT_NO_FATAL_FAILURE(host.start());
    }
    if (!lineInput && !beforeStart) {
        /* A { further on in a line makes no buffer: a host may send one in
           quotes, and the queued line made it on the performance thread. */
        csoundEventString(host.csound, "i 4 0 0 \"{a}\"", 1);
        host.perform();
        EXPECT_EQ("{a}", channelText(host)) << host.said();
        host.csound->InputMessage(host.csound, "i 4 0 0 \"{b}\"");
        host.perform();
        EXPECT_EQ("{b}", channelText(host)) << host.said();
        EXPECT_EQ(nullptr, host.csound->lineventStatics.orchestrab);
    }
    host.csound->InputMessage(host.csound, "{\ninstr 100\nchnset 1, \"block\"\n"
                                           "endin\n}\ni 100 0 0\n");
    if (beforeStart) {
        ASSERT_NO_FATAL_FAILURE(host.start());
    }
    host.perform(1);
    EXPECT_EQ(1, channel(host, "block")) << host.said();
}

INSTANTIATE_TEST_SUITE_P(
    Inputs, HostOrchestraBlocks,
    ::testing::Combine(::testing::Bool(), ::testing::Bool()),
    [](const ::testing::TestParamInfo<std::tuple<bool, bool>> &info) {
        return std::string(std::get<0>(info.param) ? "LineInput" : "Host") +
               (std::get<1>(info.param) ? "BeforeTheStart" : "AfterIt");
    });

/* A block of no lines compiles nothing: it compiled the block before it
   again, and that block's global code ran a second time. */
TEST(HostLineEvents, AnEmptyOrchestraBlockCompilesNothing)
{
    Host host;
    host.start();
    host.csound->InputMessage(host.csound,
                              "{\nchnset chnget:i(\"ran\") + 1, \"ran\"\n}\n");
    host.perform(1);
    for (const char *empty : {"{}\n", "{\n}\n"}) {
        host.csound->InputMessage(host.csound, empty);
        host.perform(1);
    }
    EXPECT_EQ(1, channel(host, "ran")) << host.said();
}

/* What follows the { on its line is the block's first line: the { went
   into the block too, and its compile failed. A } first on a line of a {{
   string is the string's, when a later } ends the block. */
TEST(HostLineEvents, TheRestOfTheBraceLineIsTheBlocks)
{
    Host host;
    host.start();
    host.csound->InputMessage(host.csound, "{ instr 104\nchnset 4, \"block\"\n"
                                           "endin\n}\ni 104 0 0\n");
    host.perform(1);
    EXPECT_EQ(4, channel(host, "block")) << host.said();
    host.csound->InputMessage(host.csound,
                              "{\ninstr 103\nS1 = {{ {\"a\": 1\n} }}\n"
                              "chnset S1, \"text\"\nendin\n}\ni 103 0 0\n");
    host.perform(1);
    EXPECT_EQ(" {\"a\": 1\n} ", channelText(host)) << host.said();
}

/* An orchestra block whose text, the lines between its { line and its }
   line, is size bytes: instr 105, then comment lines. */
std::string blockOf(size_t size)
{
    std::string text = "instr 105\nchnset 5, \"block\"\nendin\n";
    while (text.size() + 1002 < size)
        text += ";" + std::string(998, 'x') + "\n";
    text += ";" + std::string(size - text.size() - 2, 'x') + "\n";
    return "{\n" + text + "}\n";
}

/* A block holds 1 MB less a byte, from a host or from -L, one case each: a
   longer one is refused, its text dropped up to its }, and what follows it
   plays. It ran past its buffer, and the process died. */
class HostOrchestraBlockSizes
    : public ::testing::TestWithParam<std::tuple<bool, size_t>> {};

TEST_P(HostOrchestraBlockSizes, PastTheBufferAreRefused)
{
    const bool lineInput = std::get<0>(GetParam());
    const size_t size = std::get<1>(GetParam());
    const bool fits = size < 1048576;
    const TempFile file("csound-block-size-", ".sco");
    const std::string lines = blockOf(size) + "i 1 0 0\ni 105 0 0\n";
    Host host;
    if (lineInput) {
        std::ofstream(file.name, std::ios::binary) << lines;
        csoundSetOption(host.csound, ("-L\"" + file.name + "\"").c_str());
        ASSERT_NO_FATAL_FAILURE(host.start());
    }
    else {
        ASSERT_NO_FATAL_FAILURE(host.start());
        host.csound->InputMessage(host.csound, lines.c_str());
    }
    host.perform(100);
    EXPECT_EQ(1, host.played()) << host.said();
    EXPECT_EQ(fits ? 5 : 0, channel(host, "block"));
    EXPECT_EQ(fits, host.said().find("orchestra block longer than "
                                     "1048575 bytes: not compiled\n") ==
                        std::string::npos);
    host.csound->InputMessage(host.csound, "{\ninstr 106\nchnset 6, \"block\"\n"
                                           "endin\n}\ni 106 0 0\n");
    host.perform(1);
    EXPECT_EQ(6, channel(host, "block")) << host.said();
}

INSTANTIATE_TEST_SUITE_P(
    InputsAndSizes, HostOrchestraBlockSizes,
    ::testing::Combine(::testing::Bool(),
                       ::testing::Values((size_t) 1048575, (size_t) 1048576,
                                         (size_t) 1100000)),
    [](const ::testing::TestParamInfo<std::tuple<bool, size_t>> &info) {
        return std::string(std::get<0>(info.param) ? "LineInput_" : "Host_") +
               std::to_string(std::get<1>(info.param));
    });

/* What a refused line says ends its own lines: "unknown opcode x" ran into
   "illegal RT scoreline:", and the ^ under the line into the next message. */
TEST(HostLineEvents, RefusalsEndTheirLines)
{
    const std::pair<const char *, const char *> lines[] = {
        {"i 1 0 0 .", "dot carry has no reference"},
        {"x 1 0 0", "unknown opcode x"},
        {"i 1", "too few pfields (2)"},
        {"i 1 -1 0", "-L with negative p2 illegal"},
        {"i 1 0 0 \"a", "unmatched quote"},
        {"i 4 0 0 \"a\"\ni 4 0 0 .", "cannot carry string p-field"},
    };
    Host host;
    host.start();
    size_t seen = 0;
    for (const auto &[line, what] : lines) {
        SCOPED_TRACE(line);
        host.csound->InputMessage(host.csound, line);
        host.perform(1);
        const std::string said = host.said().substr(seen);
        seen += said.size();
        const char *last = strrchr(line, '\n');
        const size_t at =
            said.find(std::string(what) + "\nillegal RT scoreline:\n" +
                      (last ? last + 1 : line) + "\n");
        ASSERT_NE(std::string::npos, at) << said;
        EXPECT_EQ("^\n", said.substr(said.find('^', at), 2)) << said;
    }
}

/* Set by the message the start gives just before it opens the -L input,
   which then waits, 100 ms at most, for the sender to be about to send: the
   two go on together, on a loaded machine too. What the thread that starts
   and performs says is kept; another thread's is not looked at. */
std::atomic<bool> openingLineInput{false}, lineGoing{false};
std::string startSaid;
std::thread::id startThread;

void watchTheStart(CSOUND *, int32_t, const char *format, va_list args)
{
    char text[256];
    if (std::this_thread::get_id() != startThread)
        return;
    vsnprintf(text, sizeof text, format, args);
    startSaid += text;
    if (strstr(text, "A4 tuning") != nullptr) {
        openingLineInput = true;
        const auto from = std::chrono::steady_clock::now();
        while (!lineGoing.load() && std::chrono::steady_clock::now() - from <
                                        std::chrono::milliseconds(100)) {
        }
    }
}

/* A line another thread sends while csoundStart opens the -L input plays,
   and so does the -L input. The start set the line buffer up without its
   lock, and one or both lines were lost, the line reader was registered
   twice, or the process died. The line goes as the start goes on from its
   last message before it opens -L: at develop the first start went wrong.
   Six starts, as one can take a second where many tests run at once. */
TEST(HostLineEvents, ALineSentAsTheStartOpensLineInputPlays)
{
    const TempFile file("csound-start-lines-", ".sco");
    std::ofstream(file.name, std::ios::binary) << "i 1 0 0\n";
    int duringTheStart = 0;
    startThread = std::this_thread::get_id();
    for (int run = 0; run < 6; run++) {
        SCOPED_TRACE(run);
        Host host;
        csoundSetMessageCallback(host.csound, watchTheStart);
        csoundSetOption(host.csound, ("-L\"" + file.name + "\"").c_str());
        openingLineInput = false;
        lineGoing = false;
        startSaid.clear();
        std::atomic<bool> started{false};
        bool sentDuringTheStart = false;
        std::thread sender([&] {
            while (!started.load() && !openingLineInput.load()) {
            }
            lineGoing = true;
            host.csound->InputMessage(host.csound, "i 1 0 0\n");
            sentDuringTheStart = !started.load();
        });
        const int result = csoundStart(host.csound);
        started = true;
        sender.join();
        duringTheStart += sentDuringTheStart;
        ASSERT_EQ(CSOUND_SUCCESS, result) << startSaid;
        for (int cycle = 0; cycle < 4; cycle++)
            ASSERT_EQ(0, csoundPerformKsmps(host.csound)) << startSaid;
        int readers = 0;
        for (auto *fp = (EVT_CB_FUNC *) host.csound->evtFuncChain; fp != nullptr;
             fp = fp->nxt)
            readers++;
        EXPECT_EQ(1, readers) << startSaid;
        ASSERT_EQ(2, host.played()) << startSaid;
    }
    /* a machine where no line was in before the start returned did not
       test the race: say so rather than pass */
    if (duringTheStart == 0)
        GTEST_SKIP() << "no line went out before csoundStart returned";
}

/* A p-field of a sign or a dot with no number after it is refused, and the
   next line plays: the sign was read again and again as a new p-field, and
   the p-fields grew until memory ran out. */
TEST(HostLineEvents, ASignWithNoNumberIsRefused)
{
    Host host;
    host.start();
    for (const char *line : {"i 1 0 0 +", "i 1 0 0 - 5", "i 1 0 0 .x", "i 1 0 0 +."}) {
        SCOPED_TRACE(line);
        const cs_float played = host.played();
        host.csound->InputMessage(host.csound, line);
        host.csound->InputMessage(host.csound, "i 1 0 0");
        host.perform(1);
        EXPECT_EQ(played + 1, host.played()) << host.said();
        EXPECT_NE(std::string::npos,
                  host.said().find(std::string("illegal RT scoreline:\n") + line));
    }
}

/* The ^ under a refused line stands under its error in the error's own
   line, after a string over two lines too: it was placed by the distance
   from the first line's start, past the end of the last. */
TEST(HostLineEvents, TheMarkStandsUnderTheErrorAfterAStringOverLines)
{
    Host host;
    host.start();
    host.csound->InputMessage(host.csound, "i 4 0 0 \"ab\ncd\" x");
    host.perform(1);
    EXPECT_NE(std::string::npos,
              host.said().find("illegal RT scoreline:\ni 4 0 0 \"ab\ncd\" x\n"
                               "    ^\n"))
        << host.said();
}

/* The start below is held at its last message until the first line's
   set-up, held in its p-field allocation, has finished; then the start dies
   on a MIDI file that cannot be loaded. */
std::atomic<int> setUpStep{0};
std::atomic<bool> startHeld{false}, setUpHeld{false};
std::atomic<std::thread::id> setUpThread;
void *(*callocOfCsound)(CSOUND *, size_t);

void *holdTheFirstLine(CSOUND *csound, size_t size)
{
    if (std::this_thread::get_id() == setUpThread.load() &&
        size == sizeof(cs_float) * (PMAX + 1)) {
        setUpStep = 1;
        const auto from = std::chrono::steady_clock::now();
        while (setUpStep.load() < 2 && std::chrono::steady_clock::now() - from <
                                           std::chrono::seconds(10)) {
        }
        setUpHeld = setUpStep.load() >= 2;
    }
    return callocOfCsound(csound, size);
}

void holdTheStart(CSOUND *, int32_t, const char *format, va_list args)
{
    char text[256];
    vsnprintf(text, sizeof text, format, args);
    if (strstr(text, "A4 tuning") != nullptr && setUpStep.load() == 1) {
        startHeld = true;
        setUpStep = 2;
        const auto from = std::chrono::steady_clock::now();
        while (setUpStep.load() < 3 && std::chrono::steady_clock::now() - from <
                                           std::chrono::seconds(10)) {
        }
    }
}

/* A host's first line, sent as the start begins, leaves the start's jump
   alone: its set-up saved csound->exitjmp, set its own setjmp there and
   put back what it had saved, after the start had taken exitjmp, and the
   start's fatal error then jumped into a frame that was gone. */
TEST(HostLineEvents, AFirstLineLeavesTheStartsJumpAlone)
{
    Host host;
    csoundSetMessageCallback(host.csound, holdTheStart);
    const TempFile missing("csound-no-such-", ".mid"); /* never made */
    csoundSetOption(host.csound, ("-F\"" + missing.name + "\"").c_str());
    setUpStep = 0;
    startHeld = setUpHeld = false;
    callocOfCsound = host.csound->Calloc;
    host.csound->Calloc = holdTheFirstLine;
    std::atomic<bool> go{false};
    std::thread sender([&] {
        while (!go.load()) {
        }
        host.csound->InputMessage(host.csound, "i 1 0 0\n");
        setUpStep = 3;
    });
    setUpThread = sender.get_id();
    go = true;
    const auto from = std::chrono::steady_clock::now();
    while (setUpStep.load() < 1 && std::chrono::steady_clock::now() - from <
                                       std::chrono::seconds(10)) {
    }
    const int result = csoundStart(host.csound);
    sender.join();
    host.csound->Calloc = callocOfCsound;
    EXPECT_NE(CSOUND_SUCCESS, result);
    /* both held each other as meant, or nothing was tested */
    if (!startHeld.load() || !setUpHeld.load())
        GTEST_SKIP() << "the first line's set-up and the start did not meet";
}

/* An allocation that fails, made by a wrapped csound->Calloc once, on an
   allocation of the size given: memdie's jump. */
std::atomic<size_t> failingSize{0};
void *(*callocThatWorks)(CSOUND *, size_t);

void *failOnce(CSOUND *csound, size_t size)
{
    if (size == failingSize.load()) {
        failingSize = 0;
        csound->ErrorMsg(csound, "memory allocate failure for %zu (a test's)\n",
                         size);
        csound->LongJmp(csound, CSOUND_MEMORY);
    }
    return callocThatWorks(csound, size);
}

enum Allocation { PFields, Reader, BlockBuffer };

struct FailingAllocation {
    const char *what;
    Allocation allocation;
    size_t size;
    const char *line;
};

const FailingAllocation failingAllocations[] = {
    {"PFields", PFields, sizeof(cs_float) * (PMAX + 1), "i 1 0 0\n"},
    {"Reader", Reader, sizeof(EVT_CB_FUNC), "i 1 0 0\n"},
    {"BlockBuffer", BlockBuffer, 1048576,
     "{\ninstr 100\nchnset 1, \"block\"\nendin\n}\ni 100 0 0\ni 1 0 0\n"},
};

/* An allocation that fails while a host's line goes into the line buffer,
   under line_event_spinlock, loses that line and lets the lock go, and the
   next line plays: the first line's p-field array, its line reader, and an
   orchestra block's buffer. The jump left through the held lock, and after
   the start into the start's spent frame; and with the reader's allocation
   failed, the set-up was taken for done, and no line played again. */
class HostLineAllocationFailures
    : public ::testing::TestWithParam<FailingAllocation> {};

TEST_P(HostLineAllocationFailures, LoseTheLineAndLetTheLockGo)
{
    const FailingAllocation &failing = GetParam();
    Host host;
    ASSERT_NO_FATAL_FAILURE(host.start());
    callocThatWorks = host.csound->Calloc;
    host.csound->Calloc = failOnce;
    failingSize = failing.size;
    host.csound->InputMessage(host.csound, failing.line);
    host.csound->Calloc = callocThatWorks;
    EXPECT_EQ(0u, failingSize.load()) << "the allocation was not made";
    ASSERT_EQ(CSOUND_SUCCESS, csoundSpinTryLock(&host.csound->line_event_spinlock))
        << host.said();
    csoundSpinUnLock(&host.csound->line_event_spinlock);
    /* the failed allocation left its part unmade, for the next line */
    const auto &lines = host.csound->lineventStatics;
    if (failing.allocation == BlockBuffer) {
        EXPECT_EQ(nullptr, lines.orchestrab);
    }
    else {
        EXPECT_EQ(nullptr, lines.Linep);
    }
    host.csound->InputMessage(host.csound, failing.line);
    host.perform(1);
    EXPECT_EQ(1, host.played()) << host.said();
    if (failing.allocation == BlockBuffer) {
        EXPECT_EQ(1, channel(host, "block")) << host.said();
    }
}

INSTANTIATE_TEST_SUITE_P(
    Allocations, HostLineAllocationFailures,
    ::testing::ValuesIn(failingAllocations),
    [](const ::testing::TestParamInfo<FailingAllocation> &info) {
        return std::string(info.param.what);
    });

/* The same failure as the start opens -L fails the start, leaves the lock
   free and gives the -L input up: the jump went to the start's landing with
   the lock held. */
TEST(HostLineEvents, AnAllocationThatFailsAsTheStartOpensLineInputFailsIt)
{
    const TempFile file("csound-failing-start-", ".sco");
    std::ofstream(file.name).close(); /* -L reads nothing from it */
    Host host;
    csoundSetOption(host.csound, ("-L\"" + file.name + "\"").c_str());
    callocThatWorks = host.csound->Calloc;
    host.csound->Calloc = failOnce;
    failingSize = sizeof(cs_float) * (PMAX + 1);
    const int result = csoundStart(host.csound);
    host.csound->Calloc = callocThatWorks;
    EXPECT_EQ(0u, failingSize.load()) << "the allocation was not made";
    EXPECT_NE(CSOUND_SUCCESS, result);
    /* it failed in the line set-up, which -L no longer stands on */
    EXPECT_NE(nullptr, host.csound->lineventStatics.Linebuf);
    EXPECT_EQ(nullptr, host.csound->lineventStatics.pfields);
    EXPECT_EQ(0, host.csound->oparms->Linein);
    ASSERT_EQ(CSOUND_SUCCESS, csoundSpinTryLock(&host.csound->line_event_spinlock));
    csoundSpinUnLock(&host.csound->line_event_spinlock);
}
} // namespace
