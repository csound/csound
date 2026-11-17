#include "csoundCore.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <optional>
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
    /* A count in a macro's text may come after line ends and with a plus
       sign, as develop's strtoll took it there, and one written in place may
       not, as at develop: these are still refused, as is a macro's text with
       no count in it, blank or empty, whatever follows the call. */
    {"{ +2 N\ni 1 0 0\n}", "invalid repeat count"},
    {"{\n2 N\ni 1 0 0\n}", "invalid repeat count"},
    {"#define C #\n0#\n{ $C N\ni 1 0 0\n}", "invalid repeat count"},
    {"#define C #\n-2#\n{ $C N\ni 1 0 0\n}", "invalid repeat count"},
    {"#define C #+x#\n{ $C N\ni 1 0 0\n}", "invalid repeat count"},
    {"#define C #+ 2#\n{ $C N\ni 1 0 0\n}", "invalid repeat count"},
    {"#define C #\n+4294967298#\n{ $C N\ni 1 0 0\n}", "invalid repeat count"},
    {"#define C #\n#\n{ $C 2 N\ni 1 0 0\n}", "invalid repeat count"},
    {"#define C ##\n{ $C 2 N\ni 1 0 0\n}", "invalid repeat count"},
    /* What may follow line ends is a number, not a [] or a macro. */
    {"#define C #\n[2]#\n{ $C N\ni 1 0 0\n}", "invalid repeat count"},
    {"#define A #2#\n#define C #\n$A#\n{ $C N\ni 1 0 0\n}", "invalid repeat count"},
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
    /* A call with too few arguments ends a [] count, as other errors in a []
       do: it read the next argument to the end of the text, which an
       earlier version of this change stopped short of, and then went on. */
    {"#define M(a'b) #$a$b#\n{ [$M(1)+2] x\ni 1 0 0\n}", "Too few arguments"},
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
    /* A count in a macro's text may come after line ends, LF or CRLF, and
       with a plus sign, as develop's strtoll took them in a macro without
       arguments; an early version of this change refused them. A macro with
       arguments, whose counts develop could not read, takes the same. */
    {"#define C #\n2\n#\n{ $C N\ni 1 0 $N\n}", "", 2},
    {"#define C #\r\n2\r\n#\r\n{ $C N\r\ni 1 0 $N\r\n}\r\n", "", 2},
    {"#define C #+2#\n{ $C N\ni 1 0 $N\n}", "", 2},
    {"#define C # \n +2#\n{ $C N\ni 1 0 $N\n}", "", 2},
    {"#define C(x) #\n+2#\n{ $C(1) N\ni 1 0 $N\n}", "", 2},
    {"#define B #\n+2#\n#define C #$B#\n{ $C N\ni 1 0 $N\n}", "", 2},
    /* An empty macro in the text is passed over, as its expansion is. */
    {"#define B ##\n#define C #$B 2#\n{ $C N\ni 1 0 $N\n}", "", 2},
    /* A { that ends a macro's text takes its count from after the call. */
    {"#define L #{#\n#define C(x) #2#\n$L $C(1) N\ni 1 0 0\n}", "", 2},
    /* And a count that ends it, its counter. These stopped the score. */
    {"#define L #{ 2#\n$L N\ni 1 0 $N\n}", "", 2},
    {"#define C(x) #$x#\n#define L #{ $C(2)#\n$L N\ni 1 0 $N\n}", "", 2},
    {"#define L #{ [1+1]#\n$L N\ni 1 0 $N\n}", "", 2},
    {"#define L #{ 2 #\n$L N\ni 1 0 $N\n}", "", 2},
    /* And its counter's name, the line it read on to past the text's end:
       the rest of the call's line is the rest of the { line, as written in
       place. */
    {"#define L #{ 2 N#\n$L\ni 1 0 $N\n}", "", 2},
    {"#define L #{ 2 N#\n$L i 1 0 0\ni 1 0 $N\n}", "", 2},
    /* A note against its } had each repeat's first line written onto it. */
    {"{ 3 N\ni 1 0 0}\n", "", 3},
    /* And an s against it, whose letter took the } with it. */
    {"{ 2 N\ni 1 0 0\ns}\n", "", 2},
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

/* An #include in a score prints nothing on stdout, past the host's messages:
   it printed "stack pointer = 0". */
TEST(HostScoreReads, IncludePrintsNothingOnStdout)
{
    const std::string file = ::testing::TempDir() + "csound-included.sco";
    std::ofstream(file, std::ios::binary) << "i 1 0 0\n";
    Host host;
    host.start();
    testing::internal::CaptureStdout();
    EXPECT_EQ(CSOUND_SUCCESS,
              host.csound->ReadScore(host.csound,
                                     ("#include \"" + file + "\"\n").c_str()));
    const std::string printed = testing::internal::GetCapturedStdout();
    std::remove(file.c_str());
    EXPECT_EQ(std::string::npos, printed.find("stack pointer")) << printed;
    host.perform();
    EXPECT_EQ(1, host.played()) << host.said();
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

/* What csoundCompile says of files it is given, written in the temporary
   directory: all of them, or those named. */
std::string compiling(const std::vector<std::pair<std::string, std::string>> &files,
                      std::vector<std::string> named = {})
{
    if (named.empty())
        for (const auto &file : files)
            named.push_back(file.first);
    for (const auto &[name, text] : files)
        std::ofstream(::testing::TempDir() + name, std::ios::binary) << text;
    std::vector<std::string> paths;
    for (const auto &name : named)
        paths.push_back(::testing::TempDir() + name);
    std::vector<const char *> arguments = {"csound", "-n", "-d"};
    for (const auto &path : paths)
        arguments.push_back(path.c_str());
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundCompile(csound, (int32_t) arguments.size(), arguments.data());
    std::string said;
    while (csoundGetMessageCnt(csound)) {
        said += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    csoundDestroy(csound);
    for (const auto &file : files)
        std::remove((::testing::TempDir() + file.first).c_str());
    return said;
}

/* An error on the second line of a score, the lexer's or the reader's, and
   what it says of its line, in a file or "the score". */
const char *lineErrors[] = {"#define 1", "i 1 0 2e"};
const char *lineErrorNames[] = {"Lexer", "Reader"};

std::string saidOfLine(int error, int line, const std::string &file)
{
    return error == 0
        ? "Unexpected character 1(31) line " + std::to_string(line) + "\n"
        : ":  at position 7\n  in line " + std::to_string(line) + " of " + file +
              "\n";
}

/* What a host says after reading a text, on an instance of its own. */
std::string reading(const std::string &text)
{
    Host host;
    host.start();
    host.csound->ReadScore(host.csound, text.c_str());
    return host.said();
}

/* The lexer's errors and the reader's name the line they are on, in a .csd, a
   score file and a host's text alike. A score file's text began with a line
   end of its own, which the reader counted, and the lexer took the line before
   the score for its first. One instance each: an instance takes seconds under
   a sanitizer. */
enum LineForm { InACsd, InAScoreFile, InAHostsText };
const char *lineFormNames[] = {"InACsd", "InAScoreFile", "InAHostsText"};

class HostScoreLines
    : public ::testing::TestWithParam<std::tuple<int, LineForm>> {};

TEST_P(HostScoreLines, NameTheLineTheyAreOn)
{
    const int error = std::get<0>(GetParam());
    const LineForm form = std::get<1>(GetParam());
    const std::string name = std::string("csound-lines-") +
                             lineErrorNames[error] + "-" + lineFormNames[form];
    const std::string line = lineErrors[error];
    std::string said;
    switch (form) {
    case InACsd:
        said = compiling(
            {{name + ".csd",
              "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\nendin\n"
              "</CsInstruments>\n<CsScore>\ni 1 0 0\n" + line +
                  "\n</CsScore>\n</CsoundSynthesizer>\n"}});
        EXPECT_NE(std::string::npos,
                  said.find(saidOfLine(error, 8, ::testing::TempDir() + name +
                                                     ".csd")))
            << said;
        break;
    case InAScoreFile:
        said = compiling({{name + ".orc", "instr 1\nendin\n"},
                          {name + ".sco", "i 1 0 0\n" + line + "\n"}});
        EXPECT_NE(std::string::npos, said.find(saidOfLine(error, 2, "the score")))
            << said;
        break;
    case InAHostsText:
        said = reading("i 1 0 0\n" + line);
        EXPECT_NE(std::string::npos, said.find(saidOfLine(error, 2, "the score")))
            << said;
        break;
    }
}

std::string lineFormName(
    const ::testing::TestParamInfo<HostScoreLines::ParamType> &info)
{
    return std::string(lineErrorNames[std::get<0>(info.param)]) + "_" +
           lineFormNames[std::get<1>(info.param)];
}

INSTANTIATE_TEST_SUITE_P(Lines, HostScoreLines,
                         ::testing::Combine(::testing::Values(0, 1),
                                            ::testing::Values(InACsd,
                                                              InAScoreFile,
                                                              InAHostsText)),
                         lineFormName);

/* A host's text read after a .csd, and a score file a .csd's options name,
   have no lines of the .csd's before them, and are not the .csd: their errors
   named the .csd, and a host's line was counted after the .csd's score's. */
enum OwnForm { AHostsTextAfterACsd, AScoreFileACsdNames };
const char *ownFormNames[] = {"AHostsTextAfterACsd", "AScoreFileACsdNames"};

class HostScoreOwnLines
    : public ::testing::TestWithParam<std::tuple<int, OwnForm>> {};

TEST_P(HostScoreOwnLines, NameTheirOwnLinesBesideACsd)
{
    const int error = std::get<0>(GetParam());
    const OwnForm form = std::get<1>(GetParam());
    const std::string name = std::string("csound-own-lines-") +
                             lineErrorNames[error] + "-" + ownFormNames[form];
    const std::string line = lineErrors[error];
    const std::string expected = saidOfLine(error, 2, "the score");
    if (form == AHostsTextAfterACsd) {
        const std::string csd = ::testing::TempDir() + name + ".csd";
        std::ofstream(csd, std::ios::binary)
            << "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\nendin\n"
               "</CsInstruments>\n<CsScore>\ni 1 0 0\n</CsScore>\n"
               "</CsoundSynthesizer>\n";
        CSOUND *csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        csoundSetOption(csound, "-d");
        EXPECT_EQ(CSOUND_SUCCESS, csoundCompileCSD(csound, csd.c_str(), 0, 0));
        EXPECT_EQ(CSOUND_SUCCESS, csoundStart(csound));
        csound->ReadScore(csound, ("i 1 0 0\n" + line).c_str());
        std::string said;
        while (csoundGetMessageCnt(csound)) {
            said += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        csoundDestroy(csound);
        std::remove(csd.c_str());
        EXPECT_NE(std::string::npos, said.find(expected)) << said;
    }
    else {
        const std::string said = compiling(
            {{name + ".orc", "instr 1\nendin\n"},
             {name + ".sco", "i 1 0 0\n" + line + "\n"},
             {name + ".csd",
              "<CsoundSynthesizer>\n<CsOptions>\n-d \"" + ::testing::TempDir() +
                  name + ".orc\" \"" + ::testing::TempDir() + name +
                  ".sco\"\n</CsOptions>\n</CsoundSynthesizer>\n"}},
            {name + ".csd"});
        EXPECT_NE(std::string::npos, said.find(expected)) << said;
    }
}

std::string ownFormName(
    const ::testing::TestParamInfo<HostScoreOwnLines::ParamType> &info)
{
    return std::string(lineErrorNames[std::get<0>(info.param)]) + "_" +
           ownFormNames[std::get<1>(info.param)];
}

INSTANTIATE_TEST_SUITE_P(
    Lines, HostScoreOwnLines,
    ::testing::Combine(::testing::Values(0, 1),
                       ::testing::Values(AHostsTextAfterACsd, AScoreFileACsdNames)),
    ownFormName);

/* The lexer counts the line ends that { and an r section's s read with their
   own line, and not the two an r puts back to end the section before it: each
   of these left the lines after it one fewer, or one more. The rest of an s or
   e line is read as any line is, its line end too, which the lexer counts and
   writes once: the lexer's lines after one were one fewer, and the reader's
   after one with blanks or a carriage return one more. Each text is followed
   by the lexer's error (0), or the reader's (1). */
struct LineAfter {
    const char *name;
    const char *text;
    int error;
};

const LineAfter statements[] = {
    {"LexerAfterARepeat", "{ 2 N\ni 1 0 0\n}", 0},
    {"LexerAfterAnRSection", "r 2\ni 1 0 0\ns", 0},
    {"LexerAfterAnRStartedAgain", "r 2\ni 1 0 0\nr 2\ni 1 0 0\ns", 0},
    {"LexerAfterS", "s", 0},
    {"LexerAfterSWithANumber", "s 2", 0},
    {"LexerAfterSWithABlank", "s ", 0},
    {"LexerAfterSWithACarriageReturn", "s\r", 0},
    {"LexerAfterE", "e", 0},
    {"LexerAfterEWithANumber", "e 2", 0},
    {"ReaderAfterS", "s", 1},
    {"ReaderAfterSWithANumber", "s 2", 1},
    {"ReaderAfterSWithABlank", "s ", 1},
    {"ReaderAfterSWithACarriageReturn", "s\r", 1},
};

class HostScoreStatementLines : public ::testing::TestWithParam<LineAfter> {};

TEST_P(HostScoreStatementLines, CountTheLinesOfAStatement)
{
    const LineAfter &after = GetParam();
    const std::string score =
        std::string(after.text) + "\n" + lineErrors[after.error] + "\n";
    const auto line = (int) std::count(score.begin(), score.end(), '\n');
    const std::string said = reading(score);
    EXPECT_NE(std::string::npos,
              said.find(saidOfLine(after.error, line, "the score"))) << said;
}

std::string lineAfterName(const ::testing::TestParamInfo<LineAfter> &info)
{
    return info.param.name;
}

INSTANTIATE_TEST_SUITE_P(Lines, HostScoreStatementLines,
                         ::testing::ValuesIn(statements), lineAfterName);

#ifndef _WIN32
/* A score a program writes, <CsScore bin>, has the program's lines, not the
   .csd's: an error on its line 5 named line 11 of this .csd, which has 10.
   With two score blocks the first is the score, the second read after its
   #exit: the lines are the first one's, whichever kind it is. A .csd whose
   compile fails leaves its score's text, which the next one writes over from
   its start: that one's block begins the score. */
enum ProgramCase {
    AProgramsScore,
    AProgramsScoreBeforeABlock,
    ABlockBeforeAProgramsScore,
    ABlockAfterAFailedCompile,
    AProgramsScoreAfterAFailedCompile
};
const char *programCaseNames[] = {
    "AProgramsScore", "AProgramsScoreBeforeABlock", "ABlockBeforeAProgramsScore",
    "ABlockAfterAFailedCompile", "AProgramsScoreAfterAFailedCompile"};

class HostScoreProgramLines : public ::testing::TestWithParam<ProgramCase> {};

TEST_P(HostScoreProgramLines, NameTheLinesOfAProgramsScore)
{
    const std::string name =
        std::string("csound-program-") + programCaseNames[GetParam()] + ".csd";
    const std::string csd = ::testing::TempDir() + name;
    const std::string top = "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\n"
                            "endin\n</CsInstruments>\n";
    const std::string program =
        "<CsScore bin=\"sh -c '(echo; echo; echo; cat $0) > $1'\">\n";
    const std::string inTheProgramsScore =
        ":  at position 7\n  in line 5 of the score\n";
    const char *failing = "<CsoundSynthesizer>\n<CsScore>\ni 1 0 0\n"
                          "</CsScore>\n</CsoundSynthesizer>\n";
    std::string said;
    switch (GetParam()) {
    case AProgramsScore:
        said = compiling({{name, top + program + "i 1 0 0\ni 1 0 2e\n</CsScore>\n"
                                                 "</CsoundSynthesizer>\n"}});
        EXPECT_NE(std::string::npos, said.find(inTheProgramsScore)) << said;
        break;
    case AProgramsScoreBeforeABlock:
        said = compiling({{name, top + program + "i 1 0 0\ni 1 0 2e\n</CsScore>\n"
                                                 "<CsScore>\ni 1 0 0\n</CsScore>\n"
                                                 "</CsoundSynthesizer>\n"}});
        EXPECT_NE(std::string::npos, said.find(inTheProgramsScore)) << said;
        break;
    case ABlockBeforeAProgramsScore:
        said = compiling({{name, top + "<CsScore>\ni 1 0 0\ni 1 0 2e\n</CsScore>\n" +
                                     program + "i 1 0 0\n</CsScore>\n"
                                     "</CsoundSynthesizer>\n"}});
        EXPECT_NE(std::string::npos,
                  said.find(":  at position 7\n  in line 8 of " + csd + "\n"))
            << said;
        break;
    case ABlockAfterAFailedCompile:
    case AProgramsScoreAfterAFailedCompile: {
        const bool block = GetParam() == ABlockAfterAFailedCompile;
        Host host;
        host.start();
        EXPECT_NE(CSOUND_SUCCESS, csoundCompileCSD(host.csound, failing, 1, 0));
        csoundCompileCSD(host.csound,
                         (top +
                          (block ? "<CsScore>\ni 1 0 0\ni 1 0 2e\n</CsScore>\n"
                                 : program + "i 1 0 0\ni 1 0 2e\n</CsScore>\n") +
                          "</CsoundSynthesizer>\n")
                             .c_str(),
                         1, 0);
        said = host.said();
        EXPECT_NE(std::string::npos,
                  said.find(block ? ":  at position 7\n  in line 8 of "
                                  : inTheProgramsScore))
            << said;
        break;
    }
    }
}

std::string programCaseName(const ::testing::TestParamInfo<ProgramCase> &info)
{
    return programCaseNames[info.param];
}

INSTANTIATE_TEST_SUITE_P(
    Lines, HostScoreProgramLines,
    ::testing::Values(AProgramsScore, AProgramsScoreBeforeABlock,
                      ABlockBeforeAProgramsScore, ABlockAfterAFailedCompile,
                      AProgramsScoreAfterAFailedCompile),
    programCaseName);
#endif

/* With --csd-line-nums=0 a .csd's score counts its lines from its first, the
   lexer as the reader does: the lexer counted the .csd's. */
TEST(HostScoreErrors, CountFromTheScoreWithCsdLineNumsOff)
{
    for (int error = 0; error < 2; error++) {
        SCOPED_TRACE(lineErrors[error]);
        const std::string said = compiling(
            {{"csound-score-lines.csd",
              "<CsoundSynthesizer>\n<CsOptions>\n--csd-line-nums=0\n"
              "</CsOptions>\n<CsInstruments>\ninstr 1\nendin\n"
              "</CsInstruments>\n<CsScore>\ni 1 0 0\n" +
                  std::string(lineErrors[error]) +
                  "\n</CsScore>\n</CsoundSynthesizer>\n"}});
        EXPECT_NE(std::string::npos, said.find(saidOfLine(error, 2, "the score")))
            << said;
    }
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

/* Nothing after an e plays: the manual says the lines after it are ignored.
   A timed e is written as an f 0, with no e line to stop the performance's
   reader, and the lines after it played, from 0 again: a note at 0.01 after
   an e at 0.05 played at 0.05. Read before the start and on a started
   engine. */
TEST(HostScoreReads, NothingAfterATimedEPlays)
{
    for (const Path path : {BeforeStart, Blocking}) {
        SCOPED_TRACE(pathNames[path]);
        Host host;
        host.send("i 1 0 0\ne 0.05\ni 1 0.01 0", path);
        int cycles = 0;
        while (cycles < 1000 && csoundPerformKsmps(host.csound) == 0)
            cycles++;
        EXPECT_EQ(1, host.played()) << host.said();
        EXPECT_EQ(75, cycles); /* 0.05 s */
    }
}

/* Only one t in a section is meaningful, the manual says: the first is used.
   The sort put two t lines in no order of their own, used the one it put
   first (the last written), and wrote the other into the sorted score,
   where the performance's reader refused it ("illegal opcode t") and read
   every line after it unwarped: a note's p2 and p3, each followed by its
   warped value, gave it two p-fields more. */
TEST(HostScoreReads, OnlyTheFirstTempoInASectionIsUsed)
{
    Host host;
    host.send("t 0 120\ni 3 1 0 7\nt 0 30", BeforeStart);
    int cycles = 0;
    while (cycles < 4000 && csoundPerformKsmps(host.csound) == 0)
        cycles++;
    EXPECT_EQ(4, csoundGetControlChannel(host.csound, "fields", nullptr))
        << host.said();
    EXPECT_EQ(7, csoundGetControlChannel(host.csound, "last", nullptr));
    EXPECT_EQ(750, cycles); /* beat 1 at 120 a minute is 0.5 s */
    EXPECT_EQ(std::string::npos, host.said().find("illegal opcode"));
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
} // namespace
