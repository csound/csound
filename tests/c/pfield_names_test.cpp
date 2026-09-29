/* Reserved p-field names. SPDX-License-Identifier: LGPL-2.1-or-later */
#include "gtest/gtest.h"
#include <string>
#include <tuple>
#include "csound.h"

namespace {
class PfieldNames : public ::testing::Test {
protected:
  CSOUND *csound = nullptr;
  void SetUp() override {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
  }
  void TearDown() override { csoundDestroy(csound); }
  std::string messages() {
    std::string text;
    while (csoundGetMessageCnt(csound)) {
      text += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    return text;
  }
  void perform(const std::string &code) {
    ASSERT_EQ(csoundCompileOrc(csound, code.c_str(), 0), 0) << messages();
    csoundEventString(csound, "i 1 0 0.01 440\ne\n", 0);
    ASSERT_EQ(csoundStart(csound), 0) << messages();
    int result = 0;
    for (int i = 0; i < 100 && result == 0; ++i)
      result = csoundPerformKsmps(csound);
    ASSERT_GT(result, 0) << messages();
    EXPECT_EQ(csoundErrCnt(csound), 0) << messages();
  }
};

using Declaration = std::tuple<const char *, const char *>;
class ReservedPfieldNames : public PfieldNames,
                           public ::testing::WithParamInterface<Declaration> {};

TEST_P(ReservedPfieldNames, RejectsVariableDeclarations) {
  const auto &name = std::get<0>(GetParam());
  const auto &code = std::get<1>(GetParam());
  EXPECT_NE(csoundCompileOrc(csound, code, 0), 0);
  const std::string error = messages();
  EXPECT_NE(error.find(std::string("'") + name + "' is reserved for p-fields"),
            std::string::npos) << error;
}

INSTANTIATE_TEST_SUITE_P(Declarations, ReservedPfieldNames, ::testing::Values(
  Declaration{"p1", "struct OscParams freq:i, amp:i\ninstr 1\np1:OscParams init 440, 0.8\nprints \"%g\", p1.freq\nendin\n"},
  Declaration{"P1", "struct OscParams freq:i\ninstr 1\nP1:OscParams init 440\nendin\n"},
  Declaration{"p4", "instr 1\np4:i = 440\nendin\n"},
  Declaration{"P4", "instr 1\nP4:k = 440\nendin\n"},
  Declaration{"p0", "instr 1\np0:S = \"text\"\nendin\n"},
  Declaration{"p01", "instr 1\np01:i[] init 2\nendin\n"},
  Declaration{"P02", "instr 1\nP02:i[] init 2\nendin\n"},
  Declaration{"p5", "instr 1\np5[] init 2\nendin\n"},
  Declaration{"p12", "p12@global:i init 1\n"},
  Declaration{"P12", "opcode example(P12:i):i\nxout P12\nendop\n"},
  Declaration{"p6", "instr 1\nfor p6 in [1, 2] do\nprints \"%g\", p6\nod\nendin\n"},
  Declaration{"p999999999999999999999", "instr 1\np999999999999999999999:i init 1\nendin\n"}
));

TEST_F(PfieldNames, AllowsNamesWithLettersAfterTheDigits) {
  perform(R"cs(
struct Params value:i
instr 1
  p1n:Params init 440
  P1n:Params init 880
  chnset p1n.value, "lower"
  chnset P1n.value, "upper"
endin
)cs");
  EXPECT_EQ(csoundGetControlChannel(csound, "lower", nullptr), 440);
  EXPECT_EQ(csoundGetControlChannel(csound, "upper", nullptr), 880);
}

TEST_F(PfieldNames, KeepsPfieldReadsAndAssignments) {
  perform(R"cs(
instr 1
  chnset p4, "lower"
  chnset P4, "upper"
  p4 = p4 + 1
  chnset p4, "written"
endin
)cs");
  EXPECT_EQ(csoundGetControlChannel(csound, "lower", nullptr), 440);
  EXPECT_EQ(csoundGetControlChannel(csound, "upper", nullptr), 440);
  EXPECT_EQ(csoundGetControlChannel(csound, "written", nullptr), 441);
}
} // namespace
