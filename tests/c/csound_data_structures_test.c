/*
 * File:   csound_data_structures_test.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#define __BUILDING_LIBCSOUND


#include <stdio.h>
#include <stdlib.h>
#include "csoundCore.h"
#include "CUnit/Basic.h"


int init_suite1(void) {
    return 0;
}

int clean_suite1(void) {
    return 0;
}

void test_cs_cons(void) {
    CSOUND* csound = csoundCreate(NULL);
    CONS_CELL* head = NULL;
    char *a = "A", *b = "B", *c = "C";
    
    CU_ASSERT_EQUAL(cs_cons_length(head), 0);

    head = cs_cons(csound, a, head);
    CU_ASSERT_EQUAL(cs_cons_length(head), 1);
    CU_ASSERT_STRING_EQUAL((char*)head->value, "A");
    CU_ASSERT_PTR_EQUAL(head->value, a);

    head = cs_cons(csound, b, head);
    CU_ASSERT_EQUAL(cs_cons_length(head), 2);
    CU_ASSERT_STRING_EQUAL((char*)head->value, "B");
    CU_ASSERT_PTR_EQUAL(head->value, b);
    CU_ASSERT_PTR_EQUAL(head->next->value, a);

    
    head = cs_cons(csound, c, head);
    CU_ASSERT_EQUAL(cs_cons_length(head), 3);
    CU_ASSERT_STRING_EQUAL((char*)head->value, "C");
    CU_ASSERT_PTR_EQUAL(head->value, c);
    
    csoundDestroy(csound);
}


void test_cs_cons_append(void) {
    CSOUND* csound = csoundCreate(NULL);
    CONS_CELL *list1 = NULL, *list2 = NULL;
    
    list1 = cs_cons(csound, "a", list1);
    list1 = cs_cons(csound, "b", list1);
    list1 = cs_cons(csound, "c", list1);

    list2 = cs_cons(csound, "d", list2);
    list2 = cs_cons(csound, "e", list2);
    list2 = cs_cons(csound, "f", list2);
    
    CU_ASSERT_EQUAL(cs_cons_length(list1), 3);
    CU_ASSERT_EQUAL(cs_cons_length(list2), 3);
    
    list1 = cs_cons_append(list1, list2);
    
    CU_ASSERT_EQUAL(cs_cons_length(list1), 6);
    CU_ASSERT_EQUAL(cs_cons_length(list2), 3);

    
    csoundDestroy(csound);
}

void test_cs_hash_table(void) {
    CSOUND* csound = csoundCreate(NULL);
    char* testValue = "test";
    
    CS_HASH_TABLE* hashTable = cs_hash_table_create(csound);
    
    cs_hash_table_put(csound, hashTable, "a", "1");
    cs_hash_table_put(csound, hashTable, "b", "2");
    cs_hash_table_put(csound, hashTable, "c", "3");
    
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "a"), "1");
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "b"), "2");
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "c"), "3");
    
    cs_hash_table_remove(csound, hashTable, "c");
    
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "a"), "1");
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "b"), "2");
    CU_ASSERT_PTR_NULL((char*)cs_hash_table_get(csound, hashTable, "c"));
    
    cs_hash_table_put(csound, hashTable, testValue, NULL);
    
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get_key(csound, hashTable, "test"), "test");
    CU_ASSERT_PTR_NOT_EQUAL(cs_hash_table_get_key(csound, hashTable, "test"), testValue);
    
    csoundDestroy(csound);
}

void test_cs_hash_table_merge(void) {
    CSOUND* csound = csoundCreate(NULL);
    
    CS_HASH_TABLE* hashTable = cs_hash_table_create(csound);
    CS_HASH_TABLE* hashTable2 = cs_hash_table_create(csound);
    
    cs_hash_table_put(csound, hashTable, "a", "1");
    cs_hash_table_put(csound, hashTable, "b", "2");
    cs_hash_table_put(csound, hashTable, "c", "3");
    
    cs_hash_table_put(csound, hashTable2, "b", "4");
    cs_hash_table_put(csound, hashTable2, "c", "5");
    cs_hash_table_put(csound, hashTable2, "d", "6");
    
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "a"), "1");
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "b"), "2");
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "c"), "3");

    cs_hash_table_merge(csound, hashTable, hashTable2);
    
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "a"), "1");
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "b"), "4");
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "c"), "5");
    CU_ASSERT_STRING_EQUAL((char*)cs_hash_table_get(csound, hashTable, "d"), "6");    
        
    csoundDestroy(csound);
}

void test_cs_hash_table_get_put_key(void) {
    CSOUND* csound = csoundCreate(NULL);
    char *a, *b, *c, *d;
    
    CS_HASH_TABLE* hashTable = cs_hash_table_create(csound);
    a = cs_hash_table_put_key(csound, hashTable, "test");
    b = cs_hash_table_put_key(csound, hashTable, "test");
    c = cs_hash_table_get_key(csound, hashTable, "test");
    d = cs_hash_table_get_key(csound, hashTable, "some other value");
    
    CU_ASSERT_PTR_EQUAL(a, b);
    CU_ASSERT_PTR_EQUAL(b, c);
    CU_ASSERT_PTR_NULL(d);
    
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
    if ((NULL == CU_add_test(pSuite, "Test cs_cons()", test_cs_cons)) ||
        (NULL == CU_add_test(pSuite, "Test cs_cons_append()", test_cs_cons_append)) ||
        (NULL == CU_add_test(pSuite, "Test cs_hash_table()", test_cs_hash_table)) ||
        (NULL == CU_add_test(pSuite, "Test cs_hash_table_merge()", test_cs_hash_table_merge)) ||
        (NULL == CU_add_test(pSuite, "Test cs_hash_table_get_put_key()", test_cs_hash_table_get_put_key))) {
        
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
                                                                                                    
