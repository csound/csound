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
        csoundSetGlobalEnv ("OPCODE6DIR64", "../../");
        csound = csoundCreate (0);
        csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "-odac --logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundCleanup (csound);
        csoundDestroyMessageBuffer (csound);
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

extern "C" {
    extern int argsRequired (char* arrayName);
    extern char** splitArgs (CSOUND* csound, char* argString);
}

TEST_F (OrcCompileTests, testArgsRequired)
{
    ASSERT_EQ (1, argsRequired("a"));
    ASSERT_EQ (2, argsRequired("ka"));
    ASSERT_EQ (3, argsRequired("kak"));
    ASSERT_EQ (2, argsRequired("ak"));
    ASSERT_EQ (3, argsRequired("a[]ka"));
    ASSERT_EQ (4, argsRequired("a[]k[]ka"));
    ASSERT_EQ (4, argsRequired("a[][]k[][]ka"));
    ASSERT_EQ (0, argsRequired(NULL));
}

TEST_F (OrcCompileTests, testSplitArgs)
{
    char** results = splitArgs(csound, "kak");
    
    ASSERT_STREQ ("k", results[0]);
    ASSERT_STREQ ("a", results[1]);
    ASSERT_STREQ ("k", results[2]);
    csound->Free(csound, results);
    
    results = splitArgs(csound, "a[]k[]ka");
    
    ASSERT_STREQ ("[a]", results[0]);
    ASSERT_STREQ ("[k]", results[1]);
    ASSERT_STREQ ("k", results[2]);
    ASSERT_STREQ ("a", results[3]);
    csound->Free(csound, results);
    
    results = splitArgs(csound, "a[][]k[][]ka");
    
    ASSERT_STREQ ("[[a]", results[0]);
    ASSERT_STREQ ("[[k]", results[1]);
    ASSERT_STREQ ("k", results[2]);
    ASSERT_STREQ ("a", results[3]);
    csound->Free(csound, results);
}

TEST_F (OrcCompileTests, testCompile)
{
    int result, compile_again = 0;
    char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    char* instrument2 =
        "instr 2 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 vco2  k1, p5   \n"
        "out  a1   \n"
        "endin \n"
        "event_i \"i\",2, 0.5, 2, 10000, 800 \n";

    result = csoundCompileOrc(csound, instrument);
    ASSERT_TRUE (result == 0);
    result = csoundReadScore(csound,  "i 1 0  1 10000 5000\n i 1 3 1 10000 1000\n");
    ASSERT_TRUE (result == 0);
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
    int result;
    char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    result = csoundCompileOrc(csound, instrument);
    ASSERT_TRUE(result == 0);
    result = csoundReadScore(csound,  "i 1 0  1 10000 5000\n");
    ASSERT_TRUE(result == 0);
    result = csoundStart(csound);
    ASSERT_TRUE(result == 0);
    csoundPerform(csound);
    csoundReset(csound);
    result = csoundCompileOrc(csound, instrument);
    csoundRewindScore(csound);
    result = csoundReadScore(csound,  "i 1 0  1 10000 5000\n");

    csoundPerform(csound);
    csoundReset(csound);
    result = csoundCompileOrc(csound, instrument);
    csoundRewindScore(csound);
    result = csoundReadScore(csound,  "i 1 0  1 10000 5000\n i 1 3 1 10000 1000\n");

    csoundPerform(csound);
}

TEST_F (OrcCompileTests, testLineNumber)
{
    char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    TREE *tree = csoundParseOrc(csound, instrument);
    ASSERT_EQ (tree->next->line, 0);
}
