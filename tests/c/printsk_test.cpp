#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>
#include <filesystem>

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

  // Keep the orchestra readable in standalone CSDs. The host checks exact
  // spaces/newlines and output from individual cycles, not just substrings.
  void start(const char *fixture, const char *caseNumber = nullptr)
  {
    const std::string opcode = std::string("--omacro:PRINT_OPCODE=") + GetParam();
    ASSERT_EQ(0, csoundSetOption(csound, opcode.c_str()));
    if (caseNumber) {
      const std::string option = std::string("--smacro:CASE=") + caseNumber;
      ASSERT_EQ(0, csoundSetOption(csound, option.c_str()));
    }
    const auto path = std::filesystem::path(__FILE__).parent_path() / "fixtures" / fixture;
    ASSERT_EQ(0, csoundCompileCSD(csound, path.string().c_str(), 0, 0)) << messages();
    ASSERT_EQ(0, csoundStart(csound)) << messages();
    messages();
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
  ASSERT_NO_FATAL_FAILURE(start("printsk_long_literal.csd"));
  EXPECT_EQ(literal + ending(), step());
}

TEST_P(PrintskTests, MultipleWideFields)
{
  ASSERT_NO_FATAL_FAILURE(start("printsk_wide_fields.csd"));
  const std::string expected = "prefix:" + std::string(2999, ' ') + "7:" +
    std::string(2996, ' ') + "2.50:end" + ending();
  EXPECT_EQ(expected, step());
  EXPECT_EQ(expected, step());
}

TEST_P(PrintskTests, LongStringAndLiteralSegments)
{
  const std::string value(5000, 'v'), prefix(3000, 'p'), suffix(3000, 's');
  ASSERT_NO_FATAL_FAILURE(start("printsk_long_segments.csd"));
  EXPECT_EQ(prefix + value + ":8" + suffix + ending(), step());
}

TEST_P(PrintskTests, ChangingFormatLength)
{
  const std::string longer(4000, 'L');
  ASSERT_NO_FATAL_FAILURE(start("printsk_changing_format.csd"));
  EXPECT_EQ("1" + ending(), step());
  EXPECT_EQ(longer + "2" + ending(), step());
  EXPECT_EQ("[3]" + ending(), step());
}

TEST_P(PrintskTests, ConversionTypesAndEscapedPercent)
{
  ASSERT_NO_FATAL_FAILURE(start("printsk_conversions.csd"));
  EXPECT_EQ("-3 4 f 1.25 ok %" + ending(), step());
}

TEST_P(PrintskTests, ReusedNote)
{
  ASSERT_NO_FATAL_FAILURE(start("printsk_reused_note.csd"));
  EXPECT_EQ(std::string(2999, ' ') + "1" + ending(), step());
  step();
  EXPECT_EQ(std::string(2999, ' ') + "2" + ending(), step());
}

TEST_P(PrintskTests, RejectsInvalidFormatsAndTypes)
{
  // Instrument numbers in printsk_invalid_format.csd name each rejection.
  for (const char *caseNumber : {"1", "2", "3", "4", "5", "6"}) {
    SCOPED_TRACE(caseNumber);
    csoundReset(csound);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    ASSERT_NO_FATAL_FAILURE(start("printsk_invalid_format.csd", caseNumber));
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->perferrcnt, 0) << messages();
  }
}

INSTANTIATE_TEST_SUITE_P(PrintOpcodes, PrintskTests,
                        ::testing::Values("printsk", "println"),
                        [](const ::testing::TestParamInfo<const char *> &info) {
                          return info.param;
                        });
}
