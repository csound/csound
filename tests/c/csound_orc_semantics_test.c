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

extern char* convertArrayName(CSOUND* csound, char* arrayName);
extern char* addDimensionToArrayName(CSOUND* csound, char* arrayName);
extern OENTRIES* find_opcode2(CSOUND* csound, OENTRY* opcodeList, OENTRY* endOpcode, char* opname);
extern OENTRY* resolve_opcode(OENTRIES* entries, char* outArgTypes, char* inArgTypes);


int init_suite1(void)
{
    return 0;
}

int clean_suite1(void)
{
    return 0;
}

void test_convertArrayName(void)
{
    CSOUND* csound = csoundCreate(NULL);
    
    char* result = convertArrayName(csound, NULL);
    CU_ASSERT_PTR_NULL(result);

    result = convertArrayName(csound, "");
    CU_ASSERT_PTR_NULL(result);
    
    result = convertArrayName(csound, "aSignals");
    CU_ASSERT_STRING_EQUAL(result, "[a;Signals");

    result = convertArrayName(csound, "gkSignals");
    CU_ASSERT_STRING_EQUAL(result, "g[k;Signals");
    
}

void test_addDimensionToArrayName(void)
{
    CSOUND* csound = csoundCreate(NULL);
    
    char* result = addDimensionToArrayName(csound, NULL);
    CU_ASSERT_PTR_NULL(result);
    
    result = addDimensionToArrayName(csound, "");
    CU_ASSERT_PTR_NULL(result);
    
    result = addDimensionToArrayName(csound, "[a;Signals");
    CU_ASSERT_STRING_EQUAL(result, "[[a;Signals");
        
    result = addDimensionToArrayName(csound, "g[k;Signals");
    CU_ASSERT_STRING_EQUAL(result, "g[[k;Signals");
    
    result = convertArrayName(csound, "kArr");
    result = addDimensionToArrayName(csound, result);
    CU_ASSERT_STRING_EQUAL(result, "[[k;Arr");
}

void test_find_opcode2(void) {
    int i;
    CSOUND* csound = csoundCreate(NULL);

    OENTRIES* entries = find_opcode2(csound, csound->opcodlst, csound->oplstend, "=");
    printf("Found entries: %d\n", entries->count);
    
    for (i = 0; i < entries->count; i++) {
        printf("%d) %s\t%s\t%s\n", i,
               entries->entries[i]->opname,
               entries->entries[i]->outypes,
               entries->entries[i]->intypes);
    }

    
    CU_ASSERT_EQUAL(7, entries->count);
    csound->Free(csound, entries);
    
    entries = find_opcode2(csound, csound->opcodlst, csound->oplstend, "vco2");
    CU_ASSERT_EQUAL(1, entries->count);
    csound->Free(csound, entries);
}

void test_resolve_opcode(void) {
    int i;
    CSOUND* csound = csoundCreate(NULL);
    
    OENTRIES* entries = find_opcode2(csound, csound->opcodlst, csound->oplstend, "=");
    CU_ASSERT_EQUAL(7, entries->count);
    
    OENTRY* opc = resolve_opcode(entries, "k", "k");
    CU_ASSERT_PTR_NOT_NULL(opc);
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
    if ((NULL == CU_add_test(pSuite, "Test convertArrayName()", test_convertArrayName))
        || (NULL == CU_add_test(pSuite, "Test addDimensionToArrayName()", test_addDimensionToArrayName))
        || (NULL == CU_add_test(pSuite, "Test find_opcode2()", test_find_opcode2))
        || (NULL == CU_add_test(pSuite, "Test resolve_opcode()", test_resolve_opcode))
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

