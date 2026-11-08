#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>

class AssertionRateTests : public ::testing::TestWithParam<const char *> {
protected:
  CSOUND *csound = nullptr;
  void SetUp() override {
    csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
  }
  void TearDown() override { csoundDestroy(csound); }

  void run(bool boolean, bool enabled, bool controlRate) {
    if (enabled) ASSERT_EQ(0, csoundSetOption(csound, "--run-unit-tests"));
    const std::string name = GetParam();
    const bool inverted = name.find("false") != std::string::npos;
    std::string body;
    if (controlRate) {
      body = "kCycle init 0\nkCycle += 1\n";
      const std::string condition = inverted ? "kCycle > 2" : "kCycle <= 2";
      if (boolean) body += name + "(" + condition + ")\n";
      else body += "kValue = (" + condition + " ? 1 : 0)\n" + name + "(kValue)\n";
    } else {
      const std::string value = boolean ? (inverted ? "1 == 0" : "1 == 1")
                                        : (inverted ? "0" : "0.5");
      body = name + "(" + value + ")\n";
    }
    const std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\n"
      "sr=1000\nksmps=1\nnchnls=1\ninstr 1\n" + body +
      "endin\n</CsInstruments>\n<CsScore>\ni1 0 .01\n"
      "</CsScore>\n</CsoundSynthesizer>";
    ASSERT_EQ(0, csoundCompileCSD(csound, csd.c_str(), 1, 0));
    ASSERT_EQ(0, csoundStart(csound));
    for (int cycle = 1; cycle <= 6; cycle++) {
      ASSERT_EQ(0, csoundPerformKsmps(csound));
      EXPECT_EQ(enabled ? (controlRate ? cycle : 1) : 0, csound->total_assert_cnt);
      EXPECT_EQ(enabled && controlRate && cycle > 2 ? cycle - 2 : 0,
                csound->perferrcnt);
    }
  }
};

TEST_P(AssertionRateTests, BooleanRunsEachControlCycle) { run(true, true, true); }
TEST_P(AssertionRateTests, NumericRunsEachControlCycle) { run(false, true, true); }
TEST_P(AssertionRateTests, DisabledAssertionsStayDisabled) { run(true, false, true); }
TEST_P(AssertionRateTests, InitBooleanRunsOnce) { run(true, true, false); }
TEST_P(AssertionRateTests, InitNumericRunsOnce) { run(false, true, false); }

INSTANTIATE_TEST_SUITE_P(Aliases, AssertionRateTests,
  ::testing::Values("assert", "assert_true", "assert_false", "asserttrue", "assertfalse"));
