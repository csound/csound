#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {
void checkExpression(const std::string& expression, cs_float expected,
                     const char* error = nullptr, int mode = 1)
{
    SCOPED_TRACE(expression);
    std::unique_ptr<CSOUND, decltype(&csoundDestroy)> engine(
      csoundCreate(nullptr, nullptr), csoundDestroy);
    CSOUND* csound = engine.get();
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\n"
      "sr=44100\nksmps=32\nnchnls=1\n"
      "instr 1\nchnset p4,\"result\"\nendin\n"
      "</CsInstruments>\n<CsScore>\ni 1 0 .01 [" + expression +
      "]\n</CsScore>\n</CsoundSynthesizer>\n";
    std::string path;
    if (mode == 0) {
#ifdef _WIN32
        int processId = _getpid();
#else
        int processId = getpid();
#endif
        path = ::testing::TempDir() + "csound-score-expression-" +
               std::to_string(processId) + ".csd";
        std::ofstream file(path, std::ios::binary);
        ASSERT_TRUE(file.is_open());
        file << csd;
        file.close();
        ASSERT_TRUE(file.good());
    }
    int result = csoundCompileCSD(csound,
                                 mode == 0 ? path.c_str() : csd.c_str(), mode, 0);
    if (!path.empty())
        EXPECT_EQ(0, std::remove(path.c_str()));
    if (error) {
        EXPECT_NE(CSOUND_SUCCESS, result);
        std::string messages;
        while (csoundGetMessageCnt(csound)) {
            messages += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        EXPECT_NE(std::string::npos, messages.find(error)) << messages;
    }
    else {
        ASSERT_EQ(CSOUND_SUCCESS, result);
        ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));
        ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
        int channelError = 0;
        EXPECT_EQ(expected, csoundGetControlChannel(csound, "result", &channelError));
        EXPECT_EQ(0, channelError);
    }
}

TEST(ScoreExpressionTests, PreservesArithmeticAndGrouping)
{
    for (const auto& item : {
      std::pair<const char*, cs_float>{"110+220", 330},
      {"330-55", 275}, {"44*10", 440}, {"1100/2", 550},
      {"5^4", 625}, {"5660%1000", 660}, {"110&220", 76},
      {"110|220", 254}, {"110#220", 178}, {"8/2*3", 12},
      {"4+3-2+1", 6}, {"4+3*2+1", 11}, {"(4+3)*(2+1)", 21},
      {"2*2&3", 4}, {"3&2*2", 0}, {"4|3*3", 13},
      {"2^3^2", 512}, {"-2+3", 1}, {"1e+2/4", 25},
      {"@11", 16}, {"@@11", 17}, {"[2+3]*[4-1]", 15},
      {"~*0+1", 1}}) {
        checkExpression(item.first, item.second);
    }
}

TEST(ScoreExpressionTests, HandlesFileInputAndScoreErrors)
{
    checkExpression("2+3", 5, nullptr, 0);
    checkExpression("2^", 0, "missing operand before closing bracket", 0);
}

TEST(ScoreExpressionTests, AcceptsNumberBufferBoundary)
{
    checkExpression(std::string(98, '0') + "1", 1);
    checkExpression("-" + std::string(97, '0') + "1", -1);
    checkExpression("1e+" + std::string(95, '0') + "1", 10);
}

TEST(ScoreExpressionTests, RejectsNumbersBeyondBufferCapacity)
{
    for (const auto& number : {
      std::string(99, '0') + "1",
      "0." + std::string(97, '0') + "1",
      "1e" + std::string(97, '0') + "1",
      "1e+" + std::string(96, '0') + "1"}) {
        checkExpression(number, 0, "number too long in score expression");
    }
}

TEST(ScoreExpressionTests, ChecksOperatorStackCapacity)
{
    checkExpression(std::string(29, '(') + "1" + std::string(29, ')'), 1);
    checkExpression(std::string(30, '(') + "1" + std::string(30, ')'), 0,
                    "score expression operator stack full");
    std::string powers = "1";
    for (int i = 0; i < 29; ++i) powers += "^1";
    checkExpression(powers, 1);
    checkExpression(powers + "^1", 0, "score expression operator stack full");
}

TEST(ScoreExpressionTests, BoundsNestedSquareBrackets)
{
    checkExpression(std::string(29, '[') + "1" + std::string(29, ']'), 1);
    checkExpression(std::string(30, '[') + "1" + std::string(30, ']'), 0,
                    "score expression nested too deeply");
}

TEST(ScoreExpressionTests, RejectsMissingPowerOperands)
{
    for (const char* expression : {"^2", "1^^2", "(^2)", "1+(^2)"})
        checkExpression(expression, 0, "missing operand before '^'");
    checkExpression("2^", 0, "missing operand before closing bracket");
}

TEST(ScoreExpressionTests, BoundsPowerOfTwoIntegerAccumulation)
{
    checkExpression("@2147483647", 67108864);
    checkExpression("@@2147483647", FL(67108865.0));
    checkExpression("@2147483648", 0, "integer overflow in @ expression");
    checkExpression("@@2147483648", 0, "integer overflow in @ expression");
}
} // namespace
