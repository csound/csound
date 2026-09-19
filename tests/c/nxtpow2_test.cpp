#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cmath>
#include <limits>
#include <string>

namespace {
struct NextPowerCase {
  MYFLT input;
  MYFLT expected;
  bool error = false;
};

class NextPowerTests : public ::testing::TestWithParam<NextPowerCase> {};

TEST_P(NextPowerTests, ReturnsPowerOrReportsInitError)
{
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  csoundCreateMessageBuffer(csound, 0);
  csoundSetOption(csound, "-n -d -m0");
  const char *csd = R"(<CsoundSynthesizer>
<CsInstruments>
sr=1024
ksmps=1
nchnls=1
0dbfs=1
instr 1
iInput chnget "input"
iOutput nxtpow2 iInput
chnset iOutput, "output"
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>)";
  int status = csoundCompileCSD(csound, csd, 1, 0);
  EXPECT_EQ(0, status);
  if (status == 0) status = csoundStart(csound);
  EXPECT_EQ(0, status);
  if (status == 0) {
    csoundSetControlChannel(csound, "input", GetParam().input);
    csoundPerformKsmps(csound);
    if (GetParam().error) EXPECT_GT(csound->inerrcnt, 0);
    else {
      EXPECT_EQ(0, csound->inerrcnt);
      EXPECT_EQ(0, csound->perferrcnt);
      EXPECT_EQ(GetParam().expected, csoundGetControlChannel(csound, "output", nullptr));
    }
  }
  std::string messages;
  while (csoundGetMessageCnt(csound)) {
    messages += csoundGetFirstMessage(csound);
    csoundPopFirstMessage(csound);
  }
  if (GetParam().error)
    EXPECT_NE(std::string::npos, messages.find("nxtpow2: input outside 32-bit integer range"));
  if (HasFailure()) ADD_FAILURE() << messages;
  csoundDestroy(csound);
}

INSTANTIATE_TEST_SUITE_P(Inputs, NextPowerTests, ::testing::Values(
  NextPowerCase{0, 2},
  NextPowerCase{1, 2},
  NextPowerCase{2, 2},
  NextPowerCase{3, 4},
  NextPowerCase{8, 8},
  NextPowerCase{FL(8.9), 8},
  NextPowerCase{9, 16},
  NextPowerCase{FL(-5.8), 2},
  NextPowerCase{FL(-2147483648.0), 2},
  NextPowerCase{FL(1073741824.0), FL(1073741824.0)},
  NextPowerCase{FL(1500000000.0), FL(2147483648.0)},
  NextPowerCase{std::nextafter(FL(2147483648.0), FL(0.0)), FL(2147483648.0)},
  NextPowerCase{FL(2147483648.0), 0, true},
  NextPowerCase{FL(-4294967296.0), 0, true},
  NextPowerCase{std::numeric_limits<MYFLT>::infinity(), 0, true},
  NextPowerCase{-std::numeric_limits<MYFLT>::infinity(), 0, true},
  NextPowerCase{std::numeric_limits<MYFLT>::quiet_NaN(), 0, true}
));
}
