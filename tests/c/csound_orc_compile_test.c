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
    CU_ASSERT_EQUAL(3, argsRequired("[a;ka"));
    CU_ASSERT_EQUAL(4, argsRequired("[a;[k;ka"));
    CU_ASSERT_EQUAL(4, argsRequired("[[a;[[k;ka"));
    CU_ASSERT_EQUAL(0, argsRequired(NULL));
}


void test_split_args(void) {
    CSOUND* csound = csoundCreate(NULL);
    char** results = splitArgs(csound, "kak");
    
    CU_ASSERT_STRING_EQUAL("k", results[0]);
    CU_ASSERT_STRING_EQUAL("a", results[1]);
    CU_ASSERT_STRING_EQUAL("k", results[2]);
    csound->Free(csound, results);
    
    results = splitArgs(csound, "[a;[k;ka");
    
    CU_ASSERT_STRING_EQUAL("[a", results[0]);
    CU_ASSERT_STRING_EQUAL("[k", results[1]);
    CU_ASSERT_STRING_EQUAL("k", results[2]);
    CU_ASSERT_STRING_EQUAL("a", results[3]);
    csound->Free(csound, results);
    
    results = splitArgs(csound, "[[a;[[k;ka");
    
    CU_ASSERT_STRING_EQUAL("[[a", results[0]);
    CU_ASSERT_STRING_EQUAL("[[k", results[1]);
    CU_ASSERT_STRING_EQUAL("k", results[2]);
    CU_ASSERT_STRING_EQUAL("a", results[3]);
    csound->Free(csound, results);
    
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
        (NULL == CU_add_test(pSuite, "Test splitArgs", test_split_args))) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
                                                                                                    
