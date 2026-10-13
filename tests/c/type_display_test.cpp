#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>

namespace {
class TypeDisplayTests : public ::testing::Test {
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

  int compile(const std::string &body)
  {
    const std::string orchestra =
      "sr=8192\nksmps=16\nnchnls=1\n0dbfs=1\n"
      "struct Point x:i, y:i\n" + body;
    return csoundCompileOrc(csound, orchestra.c_str(), 0);
  }

  void perform()
  {
    ASSERT_EQ(0, csoundStart(csound)) << messages();
    csoundEventString(csound, "i1 0 .01", 0);
    for (int block = 0; block < 8; ++block)
      csoundPerformKsmps(csound);
  }
};

TEST_F(TypeDisplayTests, PrintsStructAndArrayNames)
{
  ASSERT_EQ(0, compile(R"(
instr 1
  point:Point init 1, 2
  points:Point[][] init 2, 3
  values:k[][] init 2, 3
  label:S = "hello"
  printtype point
  printtype points
  printtype values
  printtype label
  prints "types: %s, %s, %s, %s\n", typeof(point), typeof(points), typeof(values), typeof(label)
endin
)")) << messages();
  ASSERT_NO_FATAL_FAILURE(perform());
  const std::string output = messages();
  EXPECT_NE(std::string::npos, output.find(
    "Variable Type: Point\nVariable Type: Point[][]\n"
    "Variable Type: k[][]\nVariable Type: S\n")) << output;
  EXPECT_NE(std::string::npos,
            output.find("types: Point, Point[][], k[][], S\n")) << output;
  EXPECT_EQ(std::string::npos, output.find(":Point;"));
}

TEST_F(TypeDisplayTests, ReportsScalarAssignmentTypes)
{
  ASSERT_EQ(0, compile(R"(
instr 1
  point:Point init 1, 2
  value:i = point
endin
)")) << messages();
  ASSERT_NO_FATAL_FAILURE(perform());
  const std::string output = messages();
  EXPECT_NE(std::string::npos, output.find(
    "Opcode given variables with two different types: i : Point\n")) << output;
}

TEST_F(TypeDisplayTests, ReportsArrayAssignmentTypes)
{
  ASSERT_EQ(0, compile(R"(
instr 1
  points:Point[][] init 2, 3
  value:i = points
endin
)")) << messages();
  ASSERT_NO_FATAL_FAILURE(perform());
  const std::string output = messages();
  EXPECT_NE(std::string::npos, output.find(
    "Opcode given variables with two different types: i : Point[][]\n")) << output;
}

TEST_F(TypeDisplayTests, ReportsOpcodeInputsAndCandidates)
{
  EXPECT_NE(0, compile(R"(
opcode ReadPoints(points:Point[], other:Point):i
  xout other.x
endop
instr 1
  point:Point init 1, 2
  points:Point [][] init 2, 3
  result:i ReadPoints point, points
endin
)"));
  const std::string output = messages();
  EXPECT_NE(std::string::npos,
            output.find("Found:\n  i ReadPoints Point, Point[][]\n")) << output;
  EXPECT_NE(std::string::npos,
            output.find("Candidates:\n  i ReadPoints Point[], Point\n")) << output;
  EXPECT_EQ(std::string::npos, output.find(":Point;"));
}

TEST_F(TypeDisplayTests, ReportsOpcodeOutputs)
{
  EXPECT_NE(0, compile(R"(
instr 1
  point:Point init "wrong"
endin
)"));
  const std::string output = messages();
  EXPECT_NE(std::string::npos,
            output.find("Found:\n  Point init S\n")) << output;
  EXPECT_NE(std::string::npos, output.find("  Point init i, i\n")) << output;
  EXPECT_EQ(std::string::npos, output.find(":Point;"));
}

TEST_F(TypeDisplayTests, ReportsExpressionTypes)
{
  EXPECT_NE(0, compile(R"(
instr 1
  point:Point init 1, 2
  value:i = point + 1
endin
)"));
  const std::string output = messages();
  EXPECT_NE(std::string::npos,
            output.find("expression with arg types Point, c not found")) << output;
  EXPECT_EQ(std::string::npos, output.find(":Point;"));
}

TEST_F(TypeDisplayTests, ReportsFunctionArgumentTypes)
{
  EXPECT_NE(0, compile(R"(
instr 1
  point:Point init 1, 2
  value:i = sqrt(point)
endin
)"));
  const std::string output = messages();
  EXPECT_NE(std::string::npos,
            output.find("Found:\n  i sqrt Point\n")) << output;
  EXPECT_EQ(std::string::npos, output.find(":Point;"));
}

TEST_F(TypeDisplayTests, ReportsConditionalTypes)
{
  EXPECT_NE(0, compile(R"(
instr 1
  point:Point init 1, 2
  value:i = (1 == 1 ? point : 0)
endin
)"));
  const std::string output = messages();
  EXPECT_NE(std::string::npos,
            output.find("types 'b ? Point : c'")) << output;
  EXPECT_EQ(std::string::npos, output.find(":Point;"));
}

TEST_F(TypeDisplayTests, ReportsShadowedStructTypes)
{
  EXPECT_NE(0, compile(R"(
struct Other value:i
instr 1
  point:Point init 1, 2
  point@global:Other init 3
endin
)"));
  const std::string output = messages();
  EXPECT_NE(std::string::npos, output.find(
    "global variable point:Other cannot shadow local variable point:Point, line"))
    << output;
  EXPECT_EQ(std::string::npos, output.find(":Point;"));
  EXPECT_EQ(std::string::npos, output.find(":Other;"));
}

TEST_F(TypeDisplayTests, ReportsBooleanOperandTypes)
{
  EXPECT_NE(0, compile(R"(
instr 1
  point:Point init 1, 2
  if point == 1 then
  endif
endin
)"));
  const std::string output = messages();
  EXPECT_NE(std::string::npos, output.find(
    "boolean expression '==' with arg types Point, c not found")) << output;
  EXPECT_EQ(std::string::npos, output.find(":Point;"));
}

TEST_F(TypeDisplayTests, ReportsArrayRedeclarationDimensions)
{
  EXPECT_NE(0, compile(R"(
instr 1
  points:k [][] init 2, 3
  points:Point [][] init 2, 3
endin
)"));
  const std::string output = messages();
  EXPECT_NE(std::string::npos, output.find(
    "points:Point[][] -- type mismatch for existing array variable points:k[][]"))
    << output;
  EXPECT_EQ(std::string::npos, output.find(":Point;"));
}

TEST_F(TypeDisplayTests, TypeofKeepsLongNamesWhenReusingAString)
{
  const std::string name = "Point" + std::string(80, 'x');
  ASSERT_EQ(0, compile(
    "struct " + name + " x:i\n"
    "instr 1\n"
    "  point:" + name + " init 1\n"
    "  label:S = \"short\"\n"
    "  label typeof point\n"
    "  prints \"type=%s\\n\", label\n"
    "  label typeof 1\n"
    "  prints \"type=%s\\n\", label\n"
    "endin\n")) << messages();
  ASSERT_NO_FATAL_FAILURE(perform());
  const std::string output = messages();
  EXPECT_NE(std::string::npos,
            output.find("type=" + name + "\n")) << output;
  EXPECT_NE(std::string::npos, output.find("type=c\n")) << output;
}
}
