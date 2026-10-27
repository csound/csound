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
#include <string>
#include "csoundCore.h"
#include "csound_orc.h"
#include "gtest/gtest.h"



extern "C" {
    extern OENTRIES* find_opcode2 (CSOUND* csound, const char* opname);
    extern OENTRY* resolve_opcode (CSOUND*, OENTRIES* entries,  const char* outArgTypes, const char* inArgTypes);
    extern OENTRY* resolve_opcode_exact (CSOUND*, OENTRIES* entries, const char* outArgTypes, const char* inArgTypes);
    extern OENTRY* find_opcode_new (CSOUND* csound,  const char* opname, const char* outArgsFound, const char* inArgsFound);
    extern int32_t check_in_arg (const char* found, const char* required);
    extern int32_t check_in_args (CSOUND* csound, const char* outArgsFound, const char* opOutArgs);
    extern int32_t check_out_arg (const char* found, const char* required);
    extern int32_t check_out_args (CSOUND* csound,const char* outArgsFound, const char* opOutArgs);
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

    opc = resolve_opcode(csound, entries, (char *) "i", (char *) "b");
    ASSERT_STREQ("=.ib", opc->opname);
    csound->Free(csound, entries);

    entries = find_opcode2(csound, "##userOpcode");
    opc = resolve_opcode_exact(csound, entries, NULL, NULL);
    ASSERT_TRUE(opc != NULL);
    opc = resolve_opcode_exact(csound, entries, "0", "0");
    ASSERT_TRUE(opc != NULL);
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

TEST_F (OrcSemanticsTest, NativeArrayRanksKeepLegacyWildcard)
{
    OENTRY legacy{}, explicitAny{}, matrix{};
    char name[] = "ranktest", output[] = "i";
    char legacyInput[] = "i[]", anyInput[] = "i[*]", matrixInput[] = "i[][]";
    legacy.opname = explicitAny.opname = matrix.opname = name;
    legacy.outypes = explicitAny.outypes = matrix.outypes = output;
    legacy.intypes = legacyInput;
    explicitAny.intypes = anyInput;
    matrix.intypes = matrixInput;
    auto *entries = static_cast<OENTRIES *>(
        csound->Calloc(csound, sizeof(OENTRIES) + 3 * sizeof(OENTRY *)));
    entries->count = 3;
    entries->entries[0] = &legacy;
    entries->entries[1] = &explicitAny;
    entries->entries[2] = &matrix;

    EXPECT_EQ(&matrix, resolve_opcode(csound, entries, "i", "i[][]"));
    EXPECT_EQ(&legacy, resolve_opcode(csound, entries, "i", "i[][][]"));
    EXPECT_EQ(&explicitAny, resolve_opcode(csound, entries, "i", "i[*]"));
    EXPECT_EQ(nullptr, resolve_opcode(csound, entries, "i", "S[]"));
    entries->count = 1;
    entries->entries[0] = &matrix;
    EXPECT_EQ(nullptr, resolve_opcode(csound, entries, "i", "i[]"));
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

    // K type (k-rate with initialization) - accepts k, i, c, p, r
    ASSERT_TRUE (check_in_arg("k", "K"));
    ASSERT_TRUE (check_in_arg("i", "K"));
    ASSERT_TRUE (check_in_arg("c", "K"));
    ASSERT_TRUE (check_in_arg("p", "K"));
    ASSERT_TRUE (check_in_arg("r", "K"));
    ASSERT_FALSE (check_in_arg("a", "K"));
    ASSERT_FALSE (check_in_arg("S", "K"));

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
    ASSERT_FALSE (check_in_args(csound, "S", ""));

    ASSERT_TRUE (check_in_args(csound, "akiSakiS", "N"));
    ASSERT_TRUE (check_in_args(csound, "akiSakiS", "aN"));
    ASSERT_FALSE (check_in_args(csound, "akiSakiS", "akiSakiSa"));

    ASSERT_TRUE (check_in_args(csound, "cc", "kkoM"));
    ASSERT_TRUE (check_in_args(csound, "k[]kk", ".[].M"));
    ASSERT_TRUE (check_in_args(csound, "a", "az"));
}

TEST_F(OrcSemanticsTest, InputSignatureOptionalVariadicAndComplexTypes)
{
    const struct {
        const char *found;
        const char *required;
        int32_t expected;
    } cases[] = {
        {"cc", "kkjo", 1},
        {"kc", "kkjo", 1},
        {"k", "kkjo", 0},
        {"kkic", "kkjo", 1},
        {"kkicc", "kkjo", 0},
        {"kkS", "kkjo", 0},
        {"", "jo", 1},
        {"", "ko", 0},
        {"", "z", 1},
        {"", "kz", 0},
        {"k", "kz", 1},
        {"kkk", "kz", 1},
        {"kkS", "kz", 0},
        {"akiS", "N", 1},
        {"cc", "kkoM", 1},
        {"k", "aN", 0},
        {"k[]kk", ".[].M", 1},
        {"[k]k", "k[]k", 1},
        {"i[]i", "k[]k", 1},
        {"a[]k", "k[]k", 0},
        {":Point;i", ":Point;k", 1},
        {":Other;i", ":Point;k", 0},
        {":Point;[]k", ":Point;[]k", 1},
    };
    for (const auto &test : cases) {
        EXPECT_EQ(test.expected, check_in_args(csound, test.found, test.required))
            << "found=" << test.found << " required=" << test.required;
    }
    std::string largest(VARGMAX - 1, 'k');
    EXPECT_EQ(1, check_in_args(csound, largest.c_str(), "z"));
    largest += 'k';
    EXPECT_EQ(-1, check_in_args(csound, largest.c_str(), "z"));
}

TEST_F(OrcSemanticsTest, ConcreteSignaturePrecedesCompatibleAndVariadicEntries)
{
    OENTRY exact = {};
    OENTRY compatible = {};
    OENTRY variadic = {};
    exact.outypes = compatible.outypes = variadic.outypes = (char *) "k";
    exact.intypes = (char *) "ii";
    compatible.intypes = (char *) "kk";
    variadic.intypes = (char *) "M";
    OENTRIES *entries = (OENTRIES *) csound->Calloc(
        csound, sizeof(OENTRIES) + 3 * sizeof(OENTRY *));
    entries->count = 3;
    entries->entries[0] = &variadic;
    entries->entries[1] = &compatible;
    entries->entries[2] = &exact;
    EXPECT_EQ(&exact, resolve_opcode(csound, entries, "k", "ii"));
    EXPECT_EQ(&compatible, resolve_opcode(csound, entries, "k", "kk"));
    EXPECT_EQ(&variadic, resolve_opcode(csound, entries, "k", "ci"));
    csound->Free(csound, entries);
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
    ASSERT_FALSE (check_out_args(csound, "S", ""));

    ASSERT_TRUE (check_out_args(csound, "akiSakiS", "N"));
    ASSERT_TRUE (check_out_args(csound, "akiSakiS", "aN"));
    ASSERT_FALSE (check_out_args(csound, "akiSakiS", "akiSakiSa"));

    ASSERT_TRUE (check_out_args(csound, "a", "aX"));
}

TEST_F (OrcSemanticsTest, NDescriptorOddArgumentCountTest)
{
    // Test 'n' descriptor with various argument counts
    ASSERT_TRUE (check_in_args(csound, "i", "n"));           // 1 arg (odd)
    ASSERT_TRUE (check_in_args(csound, "ic", "n"));          // 2 args (even)
    ASSERT_TRUE (check_in_args(csound, "ici", "n"));         // 3 args (odd)
    ASSERT_TRUE (check_in_args(csound, "icic", "n"));        // 4 args (even)
    ASSERT_TRUE (check_in_args(csound, "icici", "n"));       // 5 args (odd)

    // Test with different valid types for 'n' descriptor (i, c, r, p, b)
    ASSERT_TRUE (check_in_args(csound, "cr", "n"));          // 2 args (even)
    ASSERT_TRUE (check_in_args(csound, "crp", "n"));         // 3 args (odd)
    ASSERT_TRUE (check_in_args(csound, "pb", "n"));          // 2 args (even)
    ASSERT_TRUE (check_in_args(csound, "pbr", "n"));         // 3 args (odd)
}

TEST_F (OrcSemanticsTest, DotMarkerConstraintTest)
{
    // Test that '.' markers are treated as constraints, not selectable types
    // The '.' in "icrpb." should be ignored when checking for valid types

    // These should all pass because 'i', 'c', 'r', 'p', 'b' are valid types for 'n'
    ASSERT_TRUE (check_in_arg("i", "n"));
    ASSERT_TRUE (check_in_arg("c", "n"));
    ASSERT_TRUE (check_in_arg("r", "n"));
    ASSERT_TRUE (check_in_arg("p", "n"));
    ASSERT_TRUE (check_in_arg("b", "n"));

    // These should all fail because 'a', 'k', 'S' are not in the "icrpb." type string
    ASSERT_FALSE (check_in_arg("a", "n"));
    ASSERT_FALSE (check_in_arg("k", "n"));
    ASSERT_FALSE (check_in_arg("S", "n"));

    // Test that '.' itself is not a selectable type
    ASSERT_FALSE (check_in_arg(".", "n"));

    // Test other var-arg types to ensure '.' markers are properly stripped
    // Note: Currently only 'n' has '.' marker, but test framework should handle others
    ASSERT_TRUE (check_in_arg("i", "m"));  // m: "icrpb" - no dot
    ASSERT_TRUE (check_in_arg("a", "M"));  // M: "icrpkabB" - no dot
    ASSERT_TRUE (check_in_arg("S", "N"));  // N: "icrpkaSbB" - no dot
}
