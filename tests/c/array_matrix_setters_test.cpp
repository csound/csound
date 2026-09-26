#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

namespace {
class MatrixSettersTests : public ::testing::Test {
protected:
  void run(const std::string &body, const std::vector<cs_float> &expected = {},
           const char *error = nullptr, int cycles = 2)
  {
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\nsr=1024\nksmps=1\n"
      "nchnls=1\n0dbfs=1\ninstr 1\n" + body +
      "\nendin\n</CsInstruments>\n<CsScore>\ni 1 0 .02\ne\n"
      "</CsScore>\n</CsoundSynthesizer>";
    int result = csoundCompileCSD(csound, csd.c_str(), 1, 0);
    EXPECT_EQ(0, result);
    if (result == 0) result = csoundStart(csound);
    EXPECT_EQ(0, result);
    if (result == 0) {
      for (int n = 0; n < cycles; ++n) csoundPerformKsmps(csound);
      if (error) EXPECT_GT(csound->inerrcnt + csound->perferrcnt, 0);
      else {
        EXPECT_EQ(0, csound->inerrcnt);
        EXPECT_EQ(0, csound->perferrcnt);
        for (size_t i = 0; i < expected.size(); ++i) {
          std::string name = "value" + std::to_string(i);
          EXPECT_EQ(expected[i], csoundGetControlChannel(csound, name.c_str(), nullptr)) << i;
        }
      }
    }
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
      messages += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    if (error) EXPECT_NE(std::string::npos, messages.find(error));
    if (HasFailure()) ADD_FAILURE() << messages;
    csoundDestroy(csound);
  }

  std::string values(const std::vector<std::string> &expressions)
  {
    std::string result;
    for (size_t i = 0; i < expressions.size(); ++i)
      result += "\nchnset " + expressions[i] + ", \"value" + std::to_string(i) + "\"\n";
    return result;
  }
};

TEST_F(MatrixSettersTests, NumericFirstRowAtInit)
{
  run("iIn[] init 2, 3\niIn[0][0]=1\niIn[0][1]=2\niIn[0][2]=3\n"
      "iOut[] init 1, 3\niOut setrow iIn, 0\n" +
      values({"iOut[0][0]", "iOut[0][1]", "iOut[0][2]"}), {1,2,3});
}

TEST_F(MatrixSettersTests, NumericFirstRowAtControlRate)
{
  run("kIn[] init 2, 3\nkIn[0][0]=1\nkIn[0][1]=2\nkIn[0][2]=3\n"
      "kOut[] init 1, 3\nkOut setrow kIn, 0\n" +
      values({"kOut[0][0]", "kOut[0][1]", "kOut[0][2]"}), {1,2,3});
}

TEST_F(MatrixSettersTests, ColumnAcceptsMatrixFirstRow)
{
  run("iIn[] init 2, 3\niIn[0][0]=1\niIn[0][1]=2\niIn[0][2]=3\n"
      "iOut[] setcol iIn, 0\n" +
      values({"iOut[0][0]", "iOut[1][0]", "iOut[2][0]"}), {1,2,3});
}

TEST_F(MatrixSettersTests, StringRowKeepsInputAndOtherRows)
{
  run("SIn[] fillarray \"one\", \"two\"\nSOut[] init 2, 2\n"
      "SOut[0][0] = \"keep\"\nSOut setrow SIn, 1\n"
      "kA strcmpk SOut[1][0], \"one\"\nkB strcmpk SOut[1][1], \"two\"\n"
      "kC strcmpk SIn[0], \"one\"\nkD strcmpk SOut[0][0], \"keep\"\n" +
      values({"kA", "kB", "kC", "kD"}), {0,0,0,0});
}

TEST_F(MatrixSettersTests, StringColumnUsesStringCopies)
{
  run("SIn[] fillarray \"one\", \"two\"\nSOut[] setcol SIn, 1\n"
      "kA strcmpk SOut[0][1], \"one\"\nkB strcmpk SOut[1][1], \"two\"\n"
      "kC strcmpk SIn[0], \"one\"\n" + values({"kA", "kB", "kC"}), {0,0,0});
}

TEST_F(MatrixSettersTests, StringMatrixFirstRow)
{
  run("SIn[] init 2, 3\nSIn[0][0]=\"one\"\nSIn[0][1]=\"two\"\nSIn[0][2]=\"three\"\n"
      "SOut[] setrow SIn, 0\n"
      "kA strcmpk SOut[0][2], \"three\"\n" + values({"kA"}), {0});
}

TEST_F(MatrixSettersTests, GrowingWidthPreservesRowCoordinates)
{
  run("iOut[] init 2, 2\niOut[0][0]=1\niOut[0][1]=2\niOut[1][0]=3\niOut[1][1]=4\n"
      "iIn[] fillarray 5, 6, 7\niOut setrow iIn, 2\n" +
      values({"iOut[0][0]", "iOut[0][1]", "iOut[0][2]", "iOut[1][0]",
              "iOut[1][1]", "iOut[1][2]", "iOut[2][2]"}), {1,2,0,3,4,0,7});
}

TEST_F(MatrixSettersTests, GrowingColumnPreservesRows)
{
  run("iOut[] init 2, 2\niOut[0][0]=1\niOut[0][1]=2\niOut[1][0]=3\niOut[1][1]=4\n"
      "iIn[] fillarray 5, 6\niOut setcol iIn, 2\n" +
      values({"iOut[0][0]", "iOut[0][1]", "iOut[0][2]", "iOut[1][0]",
              "iOut[1][1]", "iOut[1][2]"}), {1,2,5,3,4,6});
}

TEST_F(MatrixSettersTests, GrowingStringsPreservesOwnership)
{
  run("SOut[] init 2, 2\nSOut[0][0]=\"a\"\nSOut[1][0]=\"b\"\n"
      "SIn[] fillarray \"one\", \"two\", \"three\"\nSOut setrow SIn, 2\n"
      "kA strcmpk SOut[0][0], \"a\"\nkB strcmpk SOut[1][0], \"b\"\n"
      "kC strcmpk SOut[2][2], \"three\"\n" + values({"kA", "kB", "kC"}), {0,0,0});
}

TEST_F(MatrixSettersTests, InPlaceColumnPreservesUnreadValues)
{
  run("iOut[] init 3, 3\niOut[0][0]=1\niOut[0][1]=2\niOut[0][2]=3\n"
      "iOut setcol iOut, 1\n" +
      values({"iOut[0][1]", "iOut[1][1]", "iOut[2][1]"}), {1,2,3});
}

TEST_F(MatrixSettersTests, NegativeIndexRejectedBeforeAllocation)
{
  run("iIn[] fillarray 1\niOut[] setrow iIn, -1", {}, "index out of range");
}

TEST_F(MatrixSettersTests, HugeIndexRejectedBeforeConversion)
{
  run("iIn[] fillarray 1\niOut[] setcol iIn, 1e20", {}, "index out of range");
}

TEST_F(MatrixSettersTests, OneDimensionalOutputRejected)
{
  run("iIn[] fillarray 1\niOut[] init 2\niOut setrow iIn, 0", {}, "invalid array dimensions");
}

TEST_F(MatrixSettersTests, ControlIndexCannotGrowOutput)
{
  run("kIn[] fillarray 1\nkIndex init 0\nkIndex=3\nkOut[] setrow kIn, kIndex",
      {}, "index out of range");
}

TEST_F(MatrixSettersTests, ChangedRowLengthCannotOverwriteNextRow)
{
  run("kIn[] init 2\nkLong[] fillarray 1,2,3\nkOut[] init 2,2\n"
      "kOut setrow kIn, 0\nkIn=kLong", {}, "input does not fit output");
}

TEST_F(MatrixSettersTests, ShortenedColumnInputReportsError)
{
  run("kIn[] init 2\nkShort[] fillarray 1\nkOut[] setcol kIn, 0\n"
      "kIn=kShort", {}, "input does not fit output");
}

TEST_F(MatrixSettersTests, LongerStringColumnStopsAtOutputRows)
{
  run("SIn[] fillarray \"one\", \"two\"\n"
      "SLong[] fillarray \"one\", \"two\", \"three\"\n"
      "SOut[] setcol SIn, 0\nSIn=SLong\n"
      "kA strcmpk SOut[0][0], \"one\"\nkB strcmpk SOut[1][0], \"two\"\n"
      "kRows lenarray SOut, 1\n" + values({"kA", "kB", "kRows"}), {0,0,2});
}

TEST_F(MatrixSettersTests, EmptyRow)
{
  run("iIn[] init 0\niOut[] setrow iIn, 0\niRows lenarray iOut, 1\n"
      "iCols lenarray iOut, 2\n" + values({"iRows", "iCols"}), {1,0});
}
}
