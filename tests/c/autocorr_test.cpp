#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

namespace {
class AutocorrTests : public ::testing::Test {
protected:
  void run(const std::string &input, const std::vector<cs_float> &expected,
           const std::string &after = "", bool alias = false,
           const char *error = nullptr, int cycles = 1)
  {
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    std::string output = alias ? "kIn" : "kOut";
    std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\nsr=1024\nksmps=1\n"
      "nchnls=1\n0dbfs=1\ninstr 1\n" + input + "\n" + output +
      "[] autocorr kIn\n" + after + "\nkLength lenarray " + output +
      "\nchnset kLength, \"length\"\nkIndex = 0\n"
      "while kIndex < kLength do\n"
      "Sname sprintfk \"lag%d\", kIndex\nchnset " + output +
      "[kIndex], Sname\nkIndex += 1\nod\nendin\n"
      "</CsInstruments>\n<CsScore>\ni 1 0 .01\ne\n"
      "</CsScore>\n</CsoundSynthesizer>";
    int status = csoundCompileCSD(csound, csd.c_str(), 1, 0);
    EXPECT_EQ(0, status);
    if (status == 0) status = csoundStart(csound);
    EXPECT_EQ(0, status);
    if (status == 0) {
      for (int n = 0; n < cycles; ++n) csoundPerformKsmps(csound);
      if (error == nullptr) {
        EXPECT_EQ(0, csound->inerrcnt);
        EXPECT_EQ(0, csound->perferrcnt);
        EXPECT_EQ(expected.size(), csoundGetControlChannel(csound, "length", nullptr));
        for (size_t n = 0; n < expected.size(); ++n) {
          std::string name = "lag" + std::to_string(n);
          EXPECT_NEAR(expected[n], csoundGetControlChannel(csound, name.c_str(), nullptr),
                      1.e-4) << n;
        }
      } else {
        EXPECT_GT(csound->inerrcnt + csound->perferrcnt, 0);
      }
    }
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
      messages += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    if (error != nullptr) EXPECT_NE(std::string::npos, messages.find(error));
    if (HasFailure()) ADD_FAILURE() << messages;
    csoundDestroy(csound);
  }
};

TEST_F(AutocorrTests, NonPowerOfTwoInput)
{
  run("kIn[] fillarray 1, 2, 3", {14, 8, 3});
}

TEST_F(AutocorrTests, Singleton)
{
  run("kIn[] fillarray 3", {9});
}

TEST_F(AutocorrTests, EmptyInput)
{
  run("kIn[] init 0", {});
}

TEST_F(AutocorrTests, InPlace)
{
  run("kIn[] fillarray 1, 2, 3", {14, 8, 3}, "", true);
}

TEST_F(AutocorrTests, ShorterInputUsesCurrentLength)
{
  run("kIn[] init 8\nkSmall[] fillarray 2, 3", {13, 6}, "kIn = kSmall", false, nullptr, 2);
}

TEST_F(AutocorrTests, GrowthWithinAllocatedBuffers)
{
  run("kIn[] init 3\nkOut[] init 8\nkLarge[] fillarray 1, 2, 3, 4",
      {30, 20, 11, 4}, "kIn = kLarge", false, nullptr, 2);
}

TEST_F(AutocorrTests, GrowthBeyondOutputCapacityReportsError)
{
  run("kIn[] init 3\nkLarge[] fillarray 1, 2, 3, 4", {},
      "kIn = kLarge", false, "Array too small");
}

TEST_F(AutocorrTests, InputBecomesEmpty)
{
  run("kIn[] init 8\nkEmpty[] init 0", {}, "kIn = kEmpty");
}

TEST_F(AutocorrTests, GrowthBeyondWorkspaceReportsError)
{
  run("kIn[] init 2\nkLarge[] fillarray 1, 2, 3, 4", {},
      "kIn = kLarge", false, "input array exceeds FFT workspace");
}

TEST_F(AutocorrTests, MultidimensionalInputReportsError)
{
  run("kIn[] init 2, 2", {}, "", false, "expected one-dimensional arrays");
}

TEST_F(AutocorrTests, OutputShapeReportsError)
{
  run("kIn[] fillarray 1, 2\nkOut[] init 2, 2", {}, "", false,
      "expected one-dimensional arrays");
}

TEST_F(AutocorrTests, ShorterOutputRestoresRequiredLength)
{
  run("kIn[] fillarray 1, 2, 3\nkSmall[] fillarray 0", {14, 8, 3},
      "kNever init 0\nif kNever == 1 then\nkOut = kSmall\nendif");
}
}
