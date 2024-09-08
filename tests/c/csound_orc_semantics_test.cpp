/*
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#define __BUILDING_LIBCSOUND

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "csoundCore.h"
#include "csound_orc.h"
#include "gtest/gtest.h"



extern "C" {
    extern OENTRIES* find_opcode2 (CSOUND* csound, const char* opname);
    extern OENTRY* resolve_opcode (CSOUND*, OENTRIES* entries,  const char* outArgTypes, const char* inArgTypes);
  //extern OENTRY* find_opcode_new (CSOUND* csound,  const char* opname, const char* outArgsFound, const char* inArgsFound);

    extern bool check_in_arg (const char* found, const char* required);
    extern bool check_in_args (CSOUND* csound, const char* outArgsFound, const char* opOutArgs);
    extern bool check_out_arg (const char* found, const char* required);
    extern bool check_out_args (CSOUND* csound,const  char* outArgsFound, const char* opOutArgs);
}

class OrcSemanticsTest : public ::testing::Test {
public:
    OrcSemanticsTest ()
    {
    }

    virtual ~OrcSemanticsTest ()
    {
    }

    virtual void SetUp ()
    {
      csound = csoundCreate (NULL,NULL);
      csoundCreateMessageBuffer (csound, 0);
      csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }


    CSOUND* csound {nullptr};
};

TEST_F (OrcSemanticsTest, FindOpcode2Test)
{
    OENTRIES* entries = find_opcode2(csound, "=");
    entries = find_opcode2(csound, "vco2");
    ASSERT_EQ (1, entries->count);
    csound->Free(csound, entries);
}

TEST_F (OrcSemanticsTest, ResolveOpcodeTest)
{
  OENTRIES* entries = find_opcode2(csound, (char *) "=");

    OENTRY* opc = resolve_opcode(csound, entries, (char *)  "k", (char *) "k");
    ASSERT_TRUE (opc != NULL);
    csound->Free(csound, entries);

    entries = find_opcode2(csound,  "vco2");
    ASSERT_EQ (1, entries->count);

    opc = resolve_opcode(csound, entries, "a", (char *) "cc");
    ASSERT_TRUE (opc != NULL);
    csound->Free(csound, entries);

    entries = find_opcode2(csound, "passign");
    ASSERT_EQ (3, entries->count);

    opc = resolve_opcode(csound, entries, "iiiiS", NULL);
    ASSERT_TRUE (opc != NULL);
    csound->Free(csound, entries);

    entries = find_opcode2(csound, "pcauchy");
    opc = resolve_opcode(csound, entries, "i", "k");
    ASSERT_TRUE (opc->init != NULL);

    opc = resolve_opcode(csound, entries, "k", "k");
    ASSERT_TRUE (opc->perf != NULL);

    // TODO this test is failing
    // opc = resolve_opcode(csound, entries, "a", "k");
    // ASSERT_TRUE (opc->aperf != NULL);

    csound->Free(csound, entries);
}
 
TEST_F (OrcSemanticsTest, FindOpcodeNewTest)
{
  ASSERT_TRUE (find_opcode_new(csound, (char *) "##error", (char *)  "i", (char *)  "i") != NULL);
    ASSERT_TRUE (find_opcode_new(csound,  (char *)  "##error", NULL, (char *)  "i") == NULL);
    // TODO this assertion is failing
    // ASSERT_TRUE (find_opcode_new(csound, "##xin256", "i", NULL) != NULL);
    ASSERT_TRUE (find_opcode_new(csound, (char *)  "##userOpcode", NULL, NULL) != NULL);
    ASSERT_TRUE (find_opcode_new(csound,(char *)  "##array_set", NULL,(char *)  "k[]k") != NULL);
    ASSERT_TRUE (find_opcode_new(csound,(char *)  ">=",(char *)  "B",(char *)  "kc") != NULL);
}

TEST_F (OrcSemanticsTest, CheckInArgsTest)
{
    ASSERT_FALSE (check_in_arg(NULL, NULL));
    ASSERT_FALSE (check_in_arg("a", NULL));
    ASSERT_FALSE (check_in_arg(NULL, "a"));
    ASSERT_TRUE (check_in_arg("a", "a"));
    ASSERT_FALSE (check_in_arg("a", "k"));
    ASSERT_TRUE (check_in_arg("c", "i"));
    ASSERT_TRUE (check_in_arg("i", "k"));

    // checking union types
    ASSERT_TRUE (check_in_arg("k", "x"));
    ASSERT_TRUE (check_in_arg("a", "x"));
    ASSERT_TRUE (check_in_arg("S", "T"));
    ASSERT_TRUE (check_in_arg("i", "T"));
    ASSERT_FALSE (check_in_arg("k", "T"));
    ASSERT_TRUE (check_in_arg("S", "U"));
    ASSERT_TRUE (check_in_arg("i", "U"));
    ASSERT_TRUE (check_in_arg("k", "U"));
    ASSERT_TRUE (check_in_arg("i", "k"));
    ASSERT_TRUE (check_in_arg("p", "k"));
    ASSERT_TRUE (check_in_arg("c", "k"));
    ASSERT_TRUE (check_in_arg("r", "k"));
    ASSERT_TRUE (check_in_arg("c", "i"));
    ASSERT_TRUE (check_in_arg("r", "k"));
    ASSERT_TRUE (check_in_arg("p", "k"));

    // checking var-arg types
    ASSERT_FALSE (check_in_arg("a", "m"));
    ASSERT_TRUE (check_in_arg("i", "m"));
    ASSERT_TRUE (check_in_arg("i", "M"));
    ASSERT_TRUE (check_in_arg("k", "M"));
    ASSERT_TRUE (check_in_arg("a", "M"));
    ASSERT_TRUE (check_in_arg("a", "N"));
    ASSERT_TRUE (check_in_arg("k", "N"));
    ASSERT_TRUE (check_in_arg("i", "N"));
    ASSERT_TRUE (check_in_arg("S", "N"));

    ASSERT_TRUE (check_in_arg("i", "n"));
    ASSERT_TRUE (check_in_arg("a", "y"));
    ASSERT_TRUE (check_in_arg("k", "z"));
    ASSERT_TRUE (check_in_arg("k", "Z"));
    ASSERT_TRUE (check_in_arg("a", "Z"));

    ASSERT_TRUE (check_in_arg("a", "."));
    ASSERT_TRUE (check_in_arg("k", "."));
    ASSERT_TRUE (check_in_arg("i", "."));

    ASSERT_TRUE (check_in_arg("a", "?"));
    ASSERT_TRUE (check_in_arg("k", "?"));
    ASSERT_TRUE (check_in_arg("i", "?"));

    ASSERT_TRUE (check_in_arg("a", "*"));
    ASSERT_TRUE (check_in_arg("k", "*"));
    ASSERT_TRUE (check_in_arg("i", "*"));

    //array
    ASSERT_FALSE (check_in_arg("a", "a[]"));
    ASSERT_FALSE (check_in_arg("a[]", "a"));
    ASSERT_TRUE (check_in_arg("a[]", "a[]"));
    ASSERT_FALSE (check_in_arg("k[]", "a[]"));
    ASSERT_TRUE (check_in_arg("a[]", "?[]"));
    ASSERT_TRUE (check_in_arg("k[]", "?[]"));
}

TEST_F (OrcSemanticsTest, CheckInArgs2Test)
{
    ASSERT_TRUE (check_in_args(csound, NULL, ""));
    ASSERT_TRUE (check_in_args(csound, "", NULL));
    ASSERT_TRUE (check_in_args(csound, NULL, NULL));
    ASSERT_TRUE (check_in_args(csound, "", ""));

    ASSERT_TRUE (check_in_args(csound, "akiSakiS", "N"));
    ASSERT_TRUE (check_in_args(csound, "akiSakiS", "aN"));
    ASSERT_FALSE (check_in_args(csound, "akiSakiS", "akiSakiSa"));

    ASSERT_TRUE (check_in_args(csound, "cc", "kkoM"));
    ASSERT_TRUE (check_in_args(csound, "k[]kk", ".[].M"));
    ASSERT_TRUE (check_in_args(csound, "a", "az"));
}

TEST_F (OrcSemanticsTest, CheckOutArgTest)
{
    ASSERT_FALSE (check_out_arg(NULL, NULL));
    ASSERT_FALSE (check_out_arg("a", NULL));
    ASSERT_FALSE (check_out_arg(NULL, "a"));
    ASSERT_TRUE (check_out_arg("a", "a"));
    ASSERT_FALSE (check_out_arg("a", "k"));
    ASSERT_FALSE (check_out_arg("i", "k"));

    ASSERT_FALSE (check_out_arg("c", "i"));

    // checking union types
    ASSERT_TRUE (check_out_arg("k", "s"));
    ASSERT_TRUE (check_out_arg("a", "s"));
    ASSERT_TRUE (check_out_arg("p", "i"));

    // checking var-arg types
    ASSERT_TRUE (check_out_arg("a", "m"));
    ASSERT_TRUE (check_out_arg("k", "z"));
    ASSERT_TRUE (check_out_arg("i", "I"));
    ASSERT_TRUE (check_out_arg("a", "X"));
    ASSERT_TRUE (check_out_arg("k", "X"));
    ASSERT_TRUE (check_out_arg("i", "X"));
    ASSERT_FALSE (check_out_arg("S", "X"));
    ASSERT_TRUE (check_out_arg("a", "N"));
    ASSERT_TRUE (check_out_arg("k", "N"));
    ASSERT_TRUE (check_out_arg("i", "N"));
    ASSERT_TRUE (check_out_arg("S", "N"));
    ASSERT_TRUE (check_out_arg("f", "F"));

    //array
    ASSERT_FALSE (check_out_arg("a", "[a]"));
    ASSERT_FALSE (check_out_arg("a[]", "a"));
    ASSERT_TRUE (check_out_arg("a[]", "a[]"));
    ASSERT_FALSE (check_out_arg("k[]", "a[]"));
    ASSERT_TRUE (check_out_arg("a[]", ".[]"));
}

TEST_F (OrcSemanticsTest, CheckOutArgs2Test)
{
    ASSERT_TRUE (check_out_args(csound, NULL, ""));
    ASSERT_TRUE (check_out_args(csound, "", NULL));
    ASSERT_TRUE (check_out_args(csound, NULL, NULL));
    ASSERT_TRUE (check_out_args(csound, "", ""));

    ASSERT_TRUE (check_out_args(csound, "akiSakiS", "N"));
    ASSERT_TRUE (check_out_args(csound, "akiSakiS", "aN"));
    ASSERT_FALSE (check_out_args(csound, "akiSakiS", "akiSakiSa"));

    ASSERT_TRUE (check_out_args(csound, "a", "aX"));
}
