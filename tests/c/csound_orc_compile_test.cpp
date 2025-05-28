/*
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#define __BUILDING_LIBCSOUND


#include <stdio.h>
#include <stdlib.h>
#include "csoundCore.h"
#include "gtest/gtest.h"

#define csoundCompileOrc(a,b) csoundCompileOrc(a,b,0)
#define csoundReadScore(a,b) csoundEventString(a,b,0)

class OrcCompileTests : public ::testing::Test {
public:
    OrcCompileTests ()
    {
    }

    virtual ~OrcCompileTests ()
    {
    }

    virtual void SetUp ()
    {
        csound = csoundCreate (NULL,NULL);
        //csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "-odac --logfile=null");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

extern "C" {
    extern int32_t args_required (const char* arrayName);
    extern char** split_args (CSOUND* csound, const char* argString);
}

TEST_F (OrcCompileTests, testArgsRequired)
{
    ASSERT_EQ (1, args_required("a"));
    ASSERT_EQ (2, args_required("ka"));
    ASSERT_EQ (3, args_required("kak"));
    ASSERT_EQ (2, args_required("ak"));
    ASSERT_EQ (3, args_required("a[]ka"));
    ASSERT_EQ (4, args_required("a[]k[]ka"));
    ASSERT_EQ (4, args_required("a[][]k[][]ka"));
    ASSERT_EQ (0, args_required(NULL));
}

TEST_F (OrcCompileTests, testSplitArgs)
{
    char** results = split_args(csound, "kak");

    ASSERT_STREQ ("k", results[0]);
    ASSERT_STREQ ("a", results[1]);
    ASSERT_STREQ ("k", results[2]);
    csound->Free(csound, results);

    results = split_args(csound, "a[]k[]ka");

    ASSERT_STREQ ("[a]", results[0]);
    ASSERT_STREQ ("[k]", results[1]);
    ASSERT_STREQ ("k", results[2]);
    ASSERT_STREQ ("a", results[3]);
    csound->Free(csound, results);

    results = split_args(csound, "a[][]k[][]ka");

    ASSERT_STREQ ("[[a]", results[0]);
    ASSERT_STREQ ("[[k]", results[1]);
    ASSERT_STREQ ("k", results[2]);
    ASSERT_STREQ ("a", results[3]);
    csound->Free(csound, results);
}

TEST_F (OrcCompileTests, testCompile)
{
    int32_t result, compile_again = 0;
    const char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    const char* instrument2 =
        "instr 2 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 vco2  k1, p5   \n"
        "out  a1   \n"
        "endin \n"
        "event_i \"i\",2, 0.5, 2, 10000, 800 \n";

    result = csoundCompileOrc(csound, instrument);
    ASSERT_TRUE (result == 0);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n i 1 3 1 10000 1000\n");
    result = csoundStart(csound);
    ASSERT_TRUE (result == 0);

    while(!result)
    {
        result = csoundPerformKsmps(csound);

        if(!compile_again)
        {
            /* new compilation */
            csoundCompileOrc(csound, instrument2);
            /* schedule an event on instr2 */
            csoundReadScore(csound, "i2 1 1 10000 110 \n i2 + 1 1000 660");
            compile_again = 1;
        }
    }
}

TEST_F (OrcCompileTests, testReuse)
{
    int32_t result;
    const char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    result = csoundCompileOrc(csound, instrument);
    ASSERT_TRUE(result == 0);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n");
    result = csoundStart(csound);
    ASSERT_TRUE(result == 0);
    while(csoundPerformKsmps(csound) == 0);
    csoundReset(csound);
    result = csoundCompileOrc(csound, instrument);
    csoundRewindScore(csound);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n");

    while(csoundPerformKsmps(csound) == 0);
    csoundReset(csound);
    result = csoundCompileOrc(csound, instrument);
    csoundRewindScore(csound);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n i 1 3 1 10000 1000\n");

    while(csoundPerformKsmps(csound) == 0);
}

TEST_F (OrcCompileTests, testLineNumber)
{
    const char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    TREE *tree = csoundParseOrc(csound, instrument);
    ASSERT_TRUE(tree != NULL);
}

TEST_F (OrcCompileTests, testStringsInEvent)
{
    const char* instrument = R"(
instr One
 S1 = p4
 S2 = p5
 i1 strcmp S1, "Three"
 i2 strcmp S2, "Two"
 if i1 != 0 && i2 != 0 then
  exitnow(-1)
 endif
endin
     )";

const char* event = R"(
    i "One" 0 1 "Three" "Two"
   )";
   
   int32_t result = csoundCompileOrc(csound, instrument);
   ASSERT_TRUE(result == 0);
   result = csoundStart(csound);
   ASSERT_TRUE(result == 0);
   csoundEventString(csound, event, 0);
   result = csoundPerformKsmps(csound);
   ASSERT_TRUE(result == 0);
}

TEST_F (OrcCompileTests, testMaxTableSize)
{
    const char* instrument = R"(
instr 1
a1 oscil 0,0,1
endin
     )";

const char* event = R"(
 f 1 0 [2^30-1] 2 0 [2^30-1] 0
 i1 0 1
   )";

   csoundEventString(csound, event, 0);
   int32_t result =
     csoundCompileOrc(csound, instrument);
     ASSERT_TRUE(result == 0);
     result = csoundStart(csound);
     ASSERT_TRUE(result == 0);
     result = csoundPerformKsmps(csound);
    if(sizeof(MYFLT) > 4) {
    ASSERT_TRUE(result == 0);
    ASSERT_TRUE(csoundTableLength(csound,1) == pow(2,30)-1);
   }
   else
    ASSERT_FALSE(result == 0);
}

TEST_F (OrcCompileTests, testReCompileCSD)
{
  const char* instrument = R"(
<CsoundSynthesizer>
<CsInstruments>

instr 1
endin

</CsInstruments>
<CsScore>
i 1 0 1000
</CsScore>
</CsoundSynthesizer>   
     )";

  int32_t result = csoundCompileCSD(csound,instrument,1,0);
  ASSERT_TRUE(result == 0);
  result = csoundStart(csound);
  ASSERT_TRUE(result == 0);
  result = csoundPerformKsmps(csound);
  result = csoundCompileCSD(csound,instrument,1,0);
  ASSERT_TRUE(result == 0);
  result = csoundPerformKsmps(csound);
  ASSERT_TRUE(result == 0);

}

TEST_F (OrcCompileTests, testSampleAccurate)
{
  const char* instrument = R"(
instr 1
a1 oscili 1, 440, -1, 0.25
out a1
endin
schedule(1,6/sr,0.5)
     )";

    
  int32_t result = csoundSetOption(csound, "--sample-accurate");
  ASSERT_TRUE(result == 0);
  result = csoundCompileOrc(csound, instrument);
  ASSERT_TRUE(result == 0);
  result = csoundStart(csound);
  ASSERT_TRUE(result == 0);
  result = csoundPerformKsmps(csound);
  const MYFLT *spout = csoundGetSpout(csound);
  ASSERT_TRUE(spout[5] == 0.0);
  ASSERT_TRUE(spout[6] == 1.0);
 

}


