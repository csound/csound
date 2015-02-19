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
#include "CUnit/Basic.h"

extern OENTRIES* find_opcode2(CSOUND* csound, char* opname);
extern OENTRY* resolve_opcode(CSOUND*, OENTRIES* entries, char* outArgTypes, char* inArgTypes);
extern OENTRY* find_opcode_new(CSOUND* csound, char* opname, char* outArgsFound, char* inArgsFound);

extern bool check_in_arg(char* found, char* required);
extern bool check_in_args(CSOUND* csound, char* outArgsFound, char* opOutArgs);
extern bool check_out_arg(char* found, char* required);
extern bool check_out_args(CSOUND* csound, char* outArgsFound, char* opOutArgs);
extern char* get_struct_expr_string(CSOUND* csound, TREE* structTree);

int init_suite1(void)
{
    return 0;
}

int clean_suite1(void)
{
    return 0;
}

void test_find_opcode2(void) {
//    int i;
    CSOUND* csound = csoundCreate(NULL);

    OENTRIES* entries = find_opcode2(csound, "=");
//    printf("Found entries: %d\n", entries->count);
    
//    for (i = 0; i < entries->count; i++) {
//        printf("%d) %s\t%s\t%s\n", i,
//               entries->entries[i]->opname,
//               entries->entries[i]->outypes,
//               entries->entries[i]->intypes);
//    }

    
//    CU_ASSERT_EQUAL(7, entries->count);
//    csound->Free(csound, entries);
    
    entries = find_opcode2(csound, "vco2");
    CU_ASSERT_EQUAL(1, entries->count);
    csound->Free(csound, entries);
    
}

void test_resolve_opcode(void) {
    CSOUND* csound = csoundCreate(NULL);
    
    OENTRIES* entries = find_opcode2(csound, "=");
//    CU_ASSERT_EQUAL(7, entries->count);
    
    OENTRY* opc = resolve_opcode(csound, entries, "k", "k");
    CU_ASSERT_PTR_NOT_NULL(opc);
    csound->Free(csound, entries);
    
    
    entries = find_opcode2(csound, "vco2");
    CU_ASSERT_EQUAL(1, entries->count);
    
    opc = resolve_opcode(csound, entries, "a", "cc");
    CU_ASSERT_PTR_NOT_NULL(opc);
    csound->Free(csound, entries);
    
    
    entries = find_opcode2(csound, "passign");
    CU_ASSERT_EQUAL(1, entries->count);
//    int i;
//    for (i = 0; i < entries->count; i++) {
//        printf("%d) %s\t%s\t%s\n", i,
//               entries->entries[i]->opname,
//               entries->entries[i]->outypes,
//               entries->entries[i]->intypes);
//    }
    
    opc = resolve_opcode(csound, entries, "iiiiS", NULL);
    CU_ASSERT_PTR_NOT_NULL(opc);
    csound->Free(csound, entries);

    
    entries = find_opcode2(csound, "pcauchy");
    
    opc = resolve_opcode(csound, entries, "i", "k");
    CU_ASSERT_PTR_NOT_NULL(opc->iopadr);
    
    opc = resolve_opcode(csound, entries, "k", "k");
    CU_ASSERT_PTR_NOT_NULL(opc->kopadr);
    
    opc = resolve_opcode(csound, entries, "a", "k");
    CU_ASSERT_PTR_NOT_NULL(opc->aopadr);
    
    
    csound->Free(csound, entries);
}

void test_find_opcode_new(void) {
    CSOUND* csound = csoundCreate(NULL);
    CU_ASSERT_PTR_NOT_NULL(find_opcode_new(csound, "##error", "i", "i"));
    CU_ASSERT_PTR_NULL(find_opcode_new(csound, "##error", NULL, "i"));
    CU_ASSERT_PTR_NOT_NULL(find_opcode_new(csound, "xin", "i", NULL));
    CU_ASSERT_PTR_NOT_NULL(find_opcode_new(csound, "##userOpcode", NULL, NULL));
    CU_ASSERT_PTR_NOT_NULL(find_opcode_new(csound, "##array_set", NULL, "k[]k"));
    CU_ASSERT_PTR_NOT_NULL(find_opcode_new(csound, ">=", "B", "kc"));
    

}

void test_check_in_arg(void) {
    CU_ASSERT_FALSE(check_in_arg(NULL, NULL));
    CU_ASSERT_FALSE(check_in_arg("a", NULL));
    CU_ASSERT_FALSE(check_in_arg(NULL, "a"));
    CU_ASSERT_TRUE(check_in_arg("a", "a"));
    CU_ASSERT_FALSE(check_in_arg("a", "k"));
    CU_ASSERT_TRUE(check_in_arg("c", "i"));
    CU_ASSERT_TRUE(check_in_arg("i", "k"));    
    
    // checking union types
    CU_ASSERT_TRUE(check_in_arg("k", "x"));
    CU_ASSERT_TRUE(check_in_arg("a", "x"));
    CU_ASSERT_TRUE(check_in_arg("S", "T"));
    CU_ASSERT_TRUE(check_in_arg("i", "T"));
    CU_ASSERT_FALSE(check_in_arg("k", "T"));
    CU_ASSERT_TRUE(check_in_arg("S", "U"));
    CU_ASSERT_TRUE(check_in_arg("i", "U"));
    CU_ASSERT_TRUE(check_in_arg("k", "U"));
    CU_ASSERT_TRUE(check_in_arg("i", "k"));
    CU_ASSERT_TRUE(check_in_arg("p", "k"));
    CU_ASSERT_TRUE(check_in_arg("c", "k"));
    CU_ASSERT_TRUE(check_in_arg("r", "k"));
    CU_ASSERT_TRUE(check_in_arg("c", "i"));
    CU_ASSERT_TRUE(check_in_arg("r", "k"));
    CU_ASSERT_TRUE(check_in_arg("p", "k"));


    
    // checking var-arg types
    CU_ASSERT_FALSE(check_in_arg("a", "m"));
    CU_ASSERT_TRUE(check_in_arg("i", "m"));
    CU_ASSERT_TRUE(check_in_arg("i", "M"));    
    CU_ASSERT_TRUE(check_in_arg("k", "M"));
    CU_ASSERT_TRUE(check_in_arg("a", "M"));
    CU_ASSERT_TRUE(check_in_arg("a", "N"));
    CU_ASSERT_TRUE(check_in_arg("k", "N"));
    CU_ASSERT_TRUE(check_in_arg("i", "N"));
    CU_ASSERT_TRUE(check_in_arg("S", "N"));
    
    CU_ASSERT_TRUE(check_in_arg("i", "n"));
    CU_ASSERT_TRUE(check_in_arg("a", "y"));
    CU_ASSERT_TRUE(check_in_arg("k", "z"));
    CU_ASSERT_TRUE(check_in_arg("k", "Z"));
    CU_ASSERT_TRUE(check_in_arg("a", "Z"));
    
    CU_ASSERT_TRUE(check_in_arg("a", "."));
    CU_ASSERT_TRUE(check_in_arg("k", "."));
    CU_ASSERT_TRUE(check_in_arg("i", "."));

    CU_ASSERT_TRUE(check_in_arg("a", "?"));
    CU_ASSERT_TRUE(check_in_arg("k", "?"));
    CU_ASSERT_TRUE(check_in_arg("i", "?"));
    
    CU_ASSERT_TRUE(check_in_arg("a", "*"));
    CU_ASSERT_TRUE(check_in_arg("k", "*"));
    CU_ASSERT_TRUE(check_in_arg("i", "*"));
    
    //array
    CU_ASSERT_FALSE(check_in_arg("a", "a[]"));
    CU_ASSERT_FALSE(check_in_arg("a[]", "a"));
    CU_ASSERT_TRUE(check_in_arg("a[]", "a[]"));
    CU_ASSERT_FALSE(check_in_arg("k[]", "a[]"));
    CU_ASSERT_TRUE(check_in_arg("a[]", "?[]"));
    CU_ASSERT_TRUE(check_in_arg("k[]", "?[]"));
}

void test_check_in_args(void) {
    CSOUND* csound = csoundCreate(NULL);
    
    CU_ASSERT_TRUE(check_in_args(csound, NULL, ""));
    CU_ASSERT_TRUE(check_in_args(csound, "", NULL));
    CU_ASSERT_TRUE(check_in_args(csound, NULL, NULL));
    CU_ASSERT_TRUE(check_in_args(csound, "", ""));
    
    CU_ASSERT_TRUE(check_in_args(csound, "akiSakiS", "N"));
    CU_ASSERT_TRUE(check_in_args(csound, "akiSakiS", "aN"));
    CU_ASSERT_FALSE(check_in_args(csound, "akiSakiS", "akiSakiSa"));
    
    CU_ASSERT_TRUE(check_in_args(csound, "cc", "kkoM"));
    CU_ASSERT_TRUE(check_in_args(csound, "k[]kk", ".[].M"));
    CU_ASSERT_TRUE(check_in_args(csound, "a", "az"));
    
}


void test_check_out_arg(void) {
    CU_ASSERT_FALSE(check_out_arg(NULL, NULL));
    CU_ASSERT_FALSE(check_out_arg("a", NULL));
    CU_ASSERT_FALSE(check_out_arg(NULL, "a"));
    CU_ASSERT_TRUE(check_out_arg("a", "a"));
    CU_ASSERT_FALSE(check_out_arg("a", "k"));
    CU_ASSERT_FALSE(check_out_arg("i", "k"));

    CU_ASSERT_FALSE(check_out_arg("c", "i"));
    
    // checking union types
    CU_ASSERT_TRUE(check_out_arg("k", "s"));
    CU_ASSERT_TRUE(check_out_arg("a", "s"));
    CU_ASSERT_TRUE(check_out_arg("p", "i"));    
    
    // checking var-arg types
    CU_ASSERT_TRUE(check_out_arg("a", "m"));
    CU_ASSERT_TRUE(check_out_arg("k", "z"));
    CU_ASSERT_TRUE(check_out_arg("i", "I"));
    CU_ASSERT_TRUE(check_out_arg("a", "X"));
    CU_ASSERT_TRUE(check_out_arg("k", "X"));
    CU_ASSERT_TRUE(check_out_arg("i", "X"));
    CU_ASSERT_FALSE(check_out_arg("S", "X"));
    CU_ASSERT_TRUE(check_out_arg("a", "N"));
    CU_ASSERT_TRUE(check_out_arg("k", "N"));
    CU_ASSERT_TRUE(check_out_arg("i", "N"));
    CU_ASSERT_TRUE(check_out_arg("S", "N"));
    CU_ASSERT_TRUE(check_out_arg("f", "F"));

    //array
    CU_ASSERT_FALSE(check_out_arg("a", "[a]"));
    CU_ASSERT_FALSE(check_out_arg("a[]", "a"));
    CU_ASSERT_TRUE(check_out_arg("a[]", "a[]"));
    CU_ASSERT_FALSE(check_out_arg("k[]", "a[]"));
    CU_ASSERT_TRUE(check_out_arg("a[]", ".[]"));

}

void test_check_out_args(void) {
    CSOUND* csound = csoundCreate(NULL);

    CU_ASSERT_TRUE(check_out_args(csound, NULL, ""));
    CU_ASSERT_TRUE(check_out_args(csound, "", NULL));
    CU_ASSERT_TRUE(check_out_args(csound, NULL, NULL));
    CU_ASSERT_TRUE(check_out_args(csound, "", ""));
    
    CU_ASSERT_TRUE(check_out_args(csound, "akiSakiS", "N"));
    CU_ASSERT_TRUE(check_out_args(csound, "akiSakiS", "aN"));
    CU_ASSERT_FALSE(check_out_args(csound, "akiSakiS", "akiSakiSa"));
    
    CU_ASSERT_TRUE(check_out_args(csound, "a", "aX"));    
}

void test_get_struct_expr_string(void) {
    CSOUND* csound = csoundCreate(NULL);
    TREE* top = csound->Calloc(csound, sizeof(TREE));
    TREE* left = csound->Calloc(csound, sizeof(TREE));
    TREE* right = csound->Calloc(csound, sizeof(TREE));
    TREE* next = csound->Calloc(csound, sizeof(TREE));
    
    top->left = left;
    top->right = right;
    
    left->value = csound->Calloc(csound, sizeof(ORCTOKEN));
    right->value = csound->Calloc(csound, sizeof(ORCTOKEN));
    
    left->value->lexeme = "test";
    right->value->lexeme = "member";
    
    CU_ASSERT_STRING_EQUAL(get_struct_expr_string(csound, top), "test.member");
   
    top->markup = NULL;
    next->value = csound->Calloc(csound, sizeof(ORCTOKEN));
    next->value->lexeme = "submember";
    
    right->next = next;
    
    
    CU_ASSERT_STRING_EQUAL(get_struct_expr_string(csound, top), "test.member.submember");

    csoundDestroy(csound);
}

int main()
{
    CU_pSuite pSuite = NULL;
    
    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();
    
    /* add a suite to the registry */
    pSuite = CU_add_suite("csound_orc_semantics function tests", init_suite1, clean_suite1);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "Test find_opcode2()", test_find_opcode2))
        || (NULL == CU_add_test(pSuite, "Test resolve_opcode()", test_resolve_opcode))
        || (NULL == CU_add_test(pSuite, "Test find_opcode_new()", test_find_opcode_new))
        || (NULL == CU_add_test(pSuite, "Test check_out_arg()", test_check_out_arg))
        || (NULL == CU_add_test(pSuite, "Test check_out_args()", test_check_out_args))
        || (NULL == CU_add_test(pSuite, "Test check_in_arg()", test_check_in_arg))
        || (NULL == CU_add_test(pSuite, "Test check_in_args()", test_check_in_args))
        || (NULL == CU_add_test(pSuite, "Test get_struct_expr_string()", test_get_struct_expr_string))
        )
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

