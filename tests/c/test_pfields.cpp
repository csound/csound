/*
 * File:   test_pfields.cpp
 * Author: vlazzarini
 *
 * Created on May 10 25
 */

#include <stdio.h>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "csound.h"

class PfieldTests : public ::testing::Test {
public:
    PfieldTests ()
    {
    }

    virtual ~PfieldTests ()
    {
    }

    virtual void SetUp ()
    {
        csound = csoundCreate (NULL,NULL);
        csoundSetOption(csound, "-odac");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

std::string *long_event(int32_t pcnt, const char* type) {
  int i;
  std::string *event = new std::string(type);
  event->append(" 1 0 0 2");
  for(i = 0; i < pcnt-4; i++)
    event->append(" 1");
  return event;
}

const char *orc = "0dbfs = 1\n"
        "instr 1 \n"
        "ipfields[] passign 1\n"
        "printarray ipfields \n"
        "endin \n";
  
TEST_F (PfieldTests, testScoreStringEvent)
{
  std::string *sco = long_event(4050, "i");
  csoundEventString(csound, sco->c_str(), 0);
  int32_t result = csoundCompileOrc(csound,orc, 0);
  ASSERT_TRUE (result == 0);
  result = csoundStart(csound);
  ASSERT_TRUE (result == 0);
  result = csoundPerformKsmps(csound);
  ASSERT_TRUE (result != 0);
  delete sco;
}

TEST_F (PfieldTests, testRTStringEvent)
{
  std::string *sco = long_event(4050, "i");
  int32_t result = csoundCompileOrc(csound,orc, 0);
  ASSERT_TRUE (result == 0);
  result = csoundStart(csound);
  csoundEventString(csound, sco->c_str(), 0);
  ASSERT_TRUE (result == 0);
  result = csoundPerformKsmps(csound);
  ASSERT_TRUE (result == 0);
  delete sco;
}

TEST_F (PfieldTests, testScoreFTStringEvent)
{
  std::string *sco = long_event(4050, "f");
  int32_t result = csoundCompileOrc(csound,orc, 0);
  ASSERT_TRUE (result == 0);
  result = csoundStart(csound);
  csoundEventString(csound, sco->c_str(), 0);
  ASSERT_TRUE (result == 0);
  result = csoundPerformKsmps(csound);
  ASSERT_TRUE (result == 0);
  delete sco;
}

TEST_F (PfieldTests, testScoreEvent)
{
  MYFLT pfields[4050] = {0};
  pfields[0] = 1.0;
  pfields[3] = 1.0;
  int32_t result = csoundCompileOrc(csound,orc, 0);
  ASSERT_TRUE (result == 0);
  result = csoundStart(csound);
  csoundEvent(csound, 0, pfields, 4050, 0);
  ASSERT_TRUE (result == 0);
  result = csoundPerformKsmps(csound);
  ASSERT_TRUE (result == 0);
}
