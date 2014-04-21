/*
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#include "csound.h"
#include "pthread.h"
#include "CUnit/Basic.h"


int init_suite1(void)
{
    return 0;
}

int clean_suite1(void)
{
    return 0;
}

void test_read_write(void) {
    int i;
    CSOUND* csound = csoundCreate(NULL);
    void *rb = csoundCreateCircularBuffer(csound, 512, sizeof(float));
    CU_ASSERT_PTR_NOT_NULL(rb);
    int written;
    for (i = 0 ; i <256; i++) {
        float val = i;
        written = csoundWriteCircularBuffer(csound, rb, &val, 1);
        if (written != 1) {
            break;
        }
    }
    CU_ASSERT_EQUAL(written, 1);
    float invals[256];
    int read = csoundReadCircularBuffer(csound, rb, invals, 256);
    CU_ASSERT_EQUAL(read, 256);
    for (i = 0 ; i <256; i++) {
        CU_ASSERT_EQUAL(invals[i], i);
    }
    csoundDestroyCircularBuffer(csound, rb);
    csoundDestroy(csound);
}

void test_read_write_diff_size(void) {
    int i, j;
    CSOUND* csound = csoundCreate(NULL);
    void *rb = csoundCreateCircularBuffer(csound, 512, sizeof(float));
    CU_ASSERT_PTR_NOT_NULL(rb);
    for (i = 0 ; i <256; i++) {
        float val = i;
        csoundWriteCircularBuffer(csound, rb, &val, 1);
    }
    float invals[512];
    float outvals[512];
    for (i = 256 ; i <512; i++) {
        invals[i] = i;
    }
    int writeindex = 0;
    int readindex = 0;
    for (i = 1 ; i < 16; i++) {
      int read = csoundReadCircularBuffer(csound, rb, outvals, 17 - i);
        CU_ASSERT_EQUAL(read, 17-i);
        for (j = 0; j < read; j++) {
            CU_ASSERT_EQUAL(outvals[j], readindex++);
        }
        int written = csoundWriteCircularBuffer(csound, rb, &(invals[writeindex]), i );
        CU_ASSERT_EQUAL(written, i);
        writeindex += i;
    }
    csoundDestroyCircularBuffer(csound, rb);
    csoundDestroy(csound);
}

void test_peek(void) {
    int i, j;
    CSOUND* csound = csoundCreate(NULL);
    void *rb = csoundCreateCircularBuffer(csound, 512, sizeof(float));
    CU_ASSERT_PTR_NOT_NULL(rb);
    for (i = 0 ; i <256; i++) {
        float val = i;
        csoundWriteCircularBuffer(csound, rb, &val, 1);
    }
    float invals[512];
    float outvals[512];
    for (i = 256 ; i <512; i++) {
        invals[i] = i;
    }
    int writeindex = 0;
    int readindex = 0;
    for (i = 1 ; i < 16; i++) {
        int read = csoundPeekCircularBuffer(csound, rb, outvals, 17 - i);
        CU_ASSERT_EQUAL(read, 17-i);
        for (j = 0; j < read; j++) {
            CU_ASSERT_EQUAL(outvals[j], readindex++);
        }
        readindex -= read;
        read = csoundReadCircularBuffer(csound, rb, outvals, 17 - i);
        CU_ASSERT_EQUAL(read, 17-i);
        for (j = 0; j < read; j++) {
            CU_ASSERT_EQUAL(outvals[j], readindex++);
        }
        int written = csoundWriteCircularBuffer(csound, rb, &(invals[writeindex]), i );
        CU_ASSERT_EQUAL(written, i);
        writeindex += i;
    }
    csoundDestroyCircularBuffer(csound, rb);
    csoundDestroy(csound);
}

void test_wrap(void) {
    int i;
    CSOUND* csound = csoundCreate(NULL);
    void *rb = csoundCreateCircularBuffer(csound, 32, sizeof(float));
    CU_ASSERT_PTR_NOT_NULL(rb);
    for (i = 0 ; i <16; i++) {
        float val = i;
        csoundWriteCircularBuffer(csound, rb, &val, 1);
    }
    for (i = 0 ; i < 65; i++) {
        float val;
        int read = csoundReadCircularBuffer(csound, rb, &val, 1);
        CU_ASSERT_EQUAL(read, 1);
        CU_ASSERT_EQUAL(val, i);
        val = i + 16;
        int written = csoundWriteCircularBuffer(csound, rb, &val, 1);
        CU_ASSERT_EQUAL(written, 1);
    }
    csoundDestroyCircularBuffer(csound, rb);
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
    if ((NULL == CU_add_test(pSuite, "Test read and write", test_read_write))
            || (NULL == CU_add_test(pSuite, "Test read and write diff sizes", test_read_write_diff_size))
            || (NULL == CU_add_test(pSuite, "Test peek", test_peek))
            || (NULL == CU_add_test(pSuite, "Test wrap", test_wrap))
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

