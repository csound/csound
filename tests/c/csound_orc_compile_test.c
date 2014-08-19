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
#include "CUnit/Basic.h"

extern int argsRequired(char* arrayName);
extern char** splitArgs(CSOUND* csound, char* argString);

int init_suite1(void) {
    return 0;
}

int clean_suite1(void) {
    return 0;
}

void test_args_required(void) {
    
    CU_ASSERT_EQUAL(1, argsRequired("a"));
    CU_ASSERT_EQUAL(2, argsRequired("ka"));
    CU_ASSERT_EQUAL(3, argsRequired("kak"));
    CU_ASSERT_EQUAL(2, argsRequired("ak"));
    CU_ASSERT_EQUAL(3, argsRequired("a[]ka"));
    CU_ASSERT_EQUAL(4, argsRequired("a[]k[]ka"));
    CU_ASSERT_EQUAL(4, argsRequired("a[][]k[][]ka"));
    CU_ASSERT_EQUAL(0, argsRequired(NULL));
}


void test_split_args(void) {
    CSOUND* csound = csoundCreate(NULL);
    char** results = splitArgs(csound, "kak");
    
    CU_ASSERT_STRING_EQUAL("k", results[0]);
    CU_ASSERT_STRING_EQUAL("a", results[1]);
    CU_ASSERT_STRING_EQUAL("k", results[2]);
    csound->Free(csound, results);
    
    results = splitArgs(csound, "a[]k[]ka");
    
    CU_ASSERT_STRING_EQUAL("[a]", results[0]);
    CU_ASSERT_STRING_EQUAL("[k]", results[1]);
    CU_ASSERT_STRING_EQUAL("k", results[2]);
    CU_ASSERT_STRING_EQUAL("a", results[3]);
    csound->Free(csound, results);
    
    results = splitArgs(csound, "a[][]k[][]ka");
    
    CU_ASSERT_STRING_EQUAL("[[a]", results[0]);
    CU_ASSERT_STRING_EQUAL("[[k]", results[1]);
    CU_ASSERT_STRING_EQUAL("k", results[2]);
    CU_ASSERT_STRING_EQUAL("a", results[3]);
    csound->Free(csound, results);
    
    csoundDestroy(csound);
}

void test_compile(void)
{
    CSOUND  *csound;
    int     result, compile_again=0;
    char  *instrument =
            "instr 1 \n"
            "k1 expon p4, p3, p4*0.001 \n"
            "a1 randi  k1, p5   \n"
            "out  a1   \n"
            "endin \n";

    char  *instrument2 =
            "instr 2 \n"
            "k1 expon p4, p3, p4*0.001 \n"
            "a1 vco2  k1, p5   \n"
            "out  a1   \n"
            "endin \n"
            "event_i \"i\",2, 0.5, 2, 10000, 800 \n";

    csound = csoundCreate(NULL);
    CU_ASSERT_PTR_NOT_NULL(csound);
    csoundSetOption(csound,"-odac");
    result = csoundCompileOrc(csound, instrument);
    CU_ASSERT(result == 0);
    result = csoundReadScore(csound,  "i 1 0  1 10000 5000\n i 1 3 1 10000 1000\n");
    CU_ASSERT(result == 0);
    result = csoundStart(csound);
    CU_ASSERT(result == 0);
    while(!result){
        result = csoundPerformKsmps(csound);
        if(!compile_again){
            /* new compilation */
            csoundCompileOrc(csound, instrument2);
            /* schedule an event on instr2 */
            csoundReadScore(csound, "i2 1 1 10000 110 \n i2 + 1 1000 660");
            compile_again = 1;
        }
    }
    /* delete Csound instance */
    csoundDestroy(csound);
}

void test_reuse(void)
{
    CSOUND  *csound;
    int     result;
    char  *instrument =
            "instr 1 \n"
            "k1 expon p4, p3, p4*0.001 \n"
            "a1 randi  k1, p5   \n"
            "out  a1   \n"
            "endin \n";

    csound = csoundCreate(NULL);
    CU_ASSERT_PTR_NOT_NULL(csound);
    csoundSetOption(csound,"-odac");
    result = csoundCompileOrc(csound, instrument);
    CU_ASSERT(result == 0);
    result = csoundReadScore(csound,  "i 1 0  1 10000 5000\n");
    CU_ASSERT(result == 0);
    result = csoundStart(csound);
    CU_ASSERT(result == 0);
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
    /* delete Csound instance */
    csoundDestroy(csound);
}

void test_linenum(void)
{
    CSOUND  *csound;
    //int     result;
    char  *instrument =
            "instr 1 \n"
            "k1 expon p4, p3, p4*0.001 \n"
            "a1 randi  k1, p5   \n"
            "out  a1   \n"
            "endin \n";

    csound = csoundCreate(NULL);
    csoundSetOption(csound,"-odac");
    TREE *tree = csoundParseOrc(csound, instrument);

    CU_ASSERT_EQUAL(tree->next->line, 0);

    /* delete Csound instance */
    csoundDestroy(csound);
}



int main() {
    CU_pSuite pSuite = NULL;
    
    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();
    
    /* add a suite to the registry */
    pSuite = CU_add_suite("csound_orc_compile function tests", init_suite1, clean_suite1);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "Test argsRequired", test_args_required)) ||
            (NULL == CU_add_test(pSuite, "Test splitArgs", test_split_args)) ||
            (NULL == CU_add_test(pSuite, "Test Compilation", test_compile)) ||
            (NULL == CU_add_test(pSuite, "Test Reuse Instance", test_reuse)) ||
        (NULL == CU_add_test(pSuite, "Test Line Numbers", test_linenum))) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
                                                                                                    
