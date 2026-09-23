#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "../../Opcodes/uggab.h"
#include "gtest/gtest.h"
#include <cmath>
#include <string>

namespace {
TEST(UserRandomLookup, ContinuousEndpointUsesGuardPoint)
{
  const cs_float table[] = {0, .25, .75, 1};
  cs_float value;
  USER_RAND_LOOKUP(value, table, 3, FL(3.0), 1);
  EXPECT_EQ(1, value);
  USER_RAND_LOOKUP(value, table, 3, FL(2.5), 1);
  EXPECT_EQ(.875, value);
  USER_RAND_LOOKUP(value, table, 3, FL(0.0), 1);
  EXPECT_EQ(0, value);
}

TEST(UserRandomLookup, DiscreteEndpointExcludesGuardPoint)
{
  const cs_float table[] = {2, 4, 6, 99};
  cs_float value;
  USER_RAND_LOOKUP(value, table, 3, FL(3.0), 0);
  EXPECT_EQ(6, value);
  USER_RAND_LOOKUP(value, table, 3, FL(2.5), 0);
  EXPECT_EQ(6, value);
  USER_RAND_LOOKUP(value, table, 3, FL(0.0), 0);
  EXPECT_EQ(2, value);
}

TEST(UserRandomLookup, PositionIsEvaluatedOnce)
{
  const cs_float table[] = {2, 4, 6, 99};
  cs_float value, position = 0;
  USER_RAND_LOOKUP(value, table, 3, position++, 0);
  EXPECT_EQ(1, position);
  EXPECT_EQ(2, value);
}

class UserRandomTests : public ::testing::TestWithParam<const char *> {
protected:
  void run(const std::string &body, cs_float expected,
           const char *error = nullptr, int cycles = 2)
  {
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\nsr=1024\nksmps=8\n"
      "nchnls=1\n0dbfs=1\ngiOne ftgen 1,0,-3,-2,.25,.25,.25\n"
      "giTwo ftgen 2,0,-3,-2,.75,.75,.75\ntableigpw giOne\ntableigpw giTwo\ninstr 1\n" + body +
      "\nendin\n</CsInstruments>\n<CsScore>\ni 1 0 .1\ne\n"
      "</CsScore>\n</CsoundSynthesizer>";
    int status = csoundCompileCSD(csound, csd.c_str(), 1, 0);
    EXPECT_EQ(0, status);
    if (status == 0) status = csoundStart(csound);
    EXPECT_EQ(0, status);
    if (status == 0) {
      int performanceStatus = 0;
      for (int n = 0; n < cycles; ++n) {
        performanceStatus = csoundPerformKsmps(csound);
        if (performanceStatus != 0) break;
      }
      if (error)
        EXPECT_TRUE(performanceStatus != 0 || csound->inerrcnt + csound->perferrcnt > 0);
      else {
        EXPECT_EQ(0, csound->inerrcnt);
        EXPECT_EQ(0, csound->perferrcnt);
        const cs_float *samples = csoundGetSpout(csound);
        for (int n = 0; n < 8; ++n) EXPECT_NEAR(expected, samples[n], 1.e-6) << n;
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

  std::string sample(const std::string &rate, const std::string &table)
  {
    std::string opcode = GetParam();
    return rate + "Value " + opcode + (opcode == "cuserrnd" ? " 2, 6, " : " ") + table +
      (rate == "a" ? "\nout aValue" : "\naValue upsamp " + rate + "Value\nout aValue");
  }

  cs_float first() { return std::string(GetParam()) == "cuserrnd" ? 3 : .25; }
  cs_float second() { return std::string(GetParam()) == "cuserrnd" ? 5 : .75; }
};

TEST_P(UserRandomTests, InitRateConstantTable)
{
  run(sample("i", "1"), first());
}

TEST_P(UserRandomTests, ControlRateConstantTable)
{
  run(sample("k", "1"), first());
}

TEST_P(UserRandomTests, AudioRateConstantTable)
{
  run(sample("a", "1"), first());
}

TEST_P(UserRandomTests, ChangingTableNumber)
{
  run("kCycle init 0\nkCycle+=1\nkTable = (kCycle == 1 ? 1 : 2)\n" +
      sample("a", "kTable"), second());
}

TEST_P(UserRandomTests, MissingTableAtInit)
{
  run(sample("i", "9999"), 0, "Invalid ftable");
}

TEST_P(UserRandomTests, HugeTableNumberAtControlRate)
{
  run(sample("k", "1e20"), 0, "Invalid ftable");
}

TEST_P(UserRandomTests, InvalidTableAtAudioRate)
{
  run(sample("a", "-2"), 0, "Invalid ftable");
}

INSTANTIATE_TEST_SUITE_P(Opcodes, UserRandomTests,
                        ::testing::Values("cuserrnd", "duserrnd", "urd"));

class ContinuousRandomTests : public UserRandomTests {};
TEST_F(ContinuousRandomTests, BuiltinTableAtInit)
{
  run("iValue cuserrnd 2,2,0\naValue upsamp iValue\nout aValue", 2);
}
TEST_F(ContinuousRandomTests, BuiltinTableAtControlRate)
{
  run("kValue cuserrnd 2,2,0\naValue upsamp kValue\nout aValue", 2);
}
TEST_F(ContinuousRandomTests, BuiltinTableAtAudioRate)
{
  run("aValue cuserrnd 2,2,0\nout aValue", 2);
}
}
