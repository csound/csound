#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>

namespace {
class PrintskTests : public ::testing::TestWithParam<const char *> {
protected:
  CSOUND *csound = nullptr;

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
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

  void start(const std::string &body,
             const std::string &score = "i 1 0 .003\nf 0 .004\n")
  {
    const std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\n"
      "sr=1000\nksmps=1\nnchnls=1\ninstr 1\n" + body +
      "\nendin\n</CsInstruments>\n<CsScore>\n" + score +
      "</CsScore>\n</CsoundSynthesizer>";
    ASSERT_EQ(0, csoundCompileCSD(csound, csd.c_str(), 1, 0)) << messages();
    ASSERT_EQ(0, csoundStart(csound)) << messages();
    messages();
  }

  std::string line(const std::string &arguments)
  {
    return std::string(GetParam()) + " " + arguments + "\n";
  }

  std::string ending()
  {
    return std::string(GetParam()) == "println" ? "\n" : "";
  }

  std::string step()
  {
    csoundPerformKsmps(csound);
    EXPECT_EQ(0, csound->perferrcnt) << messages();
    return messages();
  }
};

TEST_P(PrintskTests, LongLiteral)
{
  const std::string literal(8192, 'a');
  start(line("\"" + literal + "\""));
  EXPECT_EQ(literal + ending(), step());
}

TEST_P(PrintskTests, MultipleWideFields)
{
  start(line("\"prefix:%3000d:%3000.2f:end\", 7, 2.5"));
  const std::string expected = "prefix:" + std::string(2999, ' ') + "7:" +
    std::string(2996, ' ') + "2.50:end" + ending();
  EXPECT_EQ(expected, step());
  EXPECT_EQ(expected, step());
}

TEST_P(PrintskTests, LongStringAndLiteralSegments)
{
  const std::string value(5000, 'v'), prefix(3000, 'p'), suffix(3000, 's');
  start(line("\"" + prefix + "%s:%d" + suffix + "\", \"" + value + "\", 8"));
  EXPECT_EQ(prefix + value + ":8" + suffix + ending(), step());
}

TEST_P(PrintskTests, ChangingFormatLength)
{
  const std::string longer(4000, 'L');
  start("kCycle init 0\nkCycle += 1\nSfmt init \"%d\"\n"
        "if kCycle == 1 then\nSfmt strcpyk \"%d\"\n"
        "elseif kCycle == 2 then\nSfmt strcpyk \"" + longer + "%d\"\n"
        "elseif kCycle == 3 then\nSfmt strcpyk \"[%d]\"\nendif\n" +
        line("Sfmt, kCycle"));
  EXPECT_EQ("1" + ending(), step());
  EXPECT_EQ(longer + "2" + ending(), step());
  EXPECT_EQ("[3]" + ending(), step());
}

TEST_P(PrintskTests, ConversionTypesAndEscapedPercent)
{
  start(line("\"%d %u %x %.2f %s %%\", -3, 4, 15, 1.25, \"ok\""));
  EXPECT_EQ("-3 4 f 1.25 ok %" + ending(), step());
}

TEST_P(PrintskTests, ReusedNote)
{
  start(line("\"%3000d\", p4"), "i 1 0 .001 1\ni 1 .002 .001 2\nf 0 .004\n");
  EXPECT_EQ(std::string(2999, ' ') + "1" + ending(), step());
  step();
  EXPECT_EQ(std::string(2999, ' ') + "2" + ending(), step());
}

TEST_P(PrintskTests, RejectsInvalidFormatsAndTypes)
{
  for (const char *arguments : {"\"%*f\", 1", "\"%ld\", 1",
                               "\"%s\", 1", "\"%f\", \"text\"",
                               "\"%d %d\", 1", "\"%d\", 1, 2"}) {
    csoundReset(csound);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    start(line(arguments));
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->perferrcnt, 0) << arguments << messages();
  }
}

INSTANTIATE_TEST_SUITE_P(PrintOpcodes, PrintskTests,
                        ::testing::Values("printsk", "println"));
}
