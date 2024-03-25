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
#include "gtest/gtest.h"

class CsoundDataStructuresTests : public ::testing::Test {
public:
    CsoundDataStructuresTests ()
    {   
    }

    virtual ~CsoundDataStructuresTests ()
    {
    }

    virtual void SetUp ()
    {
        csoundSetGlobalEnv ("OPCODE6DIR64", "../../");
        csound = csoundCreate (0);
        csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundCleanup (csound);
        csoundDestroyMessageBuffer (csound);
        csoundDestroy (csound);
    }

    CSOUND* csound {nullptr};
};

TEST_F (CsoundDataStructuresTests, testCsCons)
{
    CONS_CELL* head = NULL;
    char *a = (char *) "A", *b = (char *) "B", *c = (char *) "C";
    
    ASSERT_EQ (cs_cons_length(head), 0);

    head = cs_cons(csound, a, head);
    ASSERT_EQ (cs_cons_length(head), 1);
    ASSERT_STREQ ((char*)head->value, "A");
    ASSERT_EQ (head->value, a);

    head = cs_cons(csound, b, head);
    ASSERT_EQ (cs_cons_length(head), 2);
    ASSERT_STREQ ((char*)head->value, "B");
    ASSERT_EQ (head->value, b);
    ASSERT_EQ (head->next->value, a);
    
    head = cs_cons(csound, c, head);
    ASSERT_EQ (cs_cons_length(head), 3);
    ASSERT_STREQ ((char*)head->value, "C");
    ASSERT_EQ (head->value, c);
}

TEST_F (CsoundDataStructuresTests, testCsConsAppend)
{
    CONS_CELL *list1 = NULL, *list2 = NULL;
    
    list1 = cs_cons(csound, const_cast<char*>("a"), list1);
    list1 = cs_cons(csound, const_cast<char*>("b"), list1);
    list1 = cs_cons(csound, const_cast<char*>("c"), list1);

    list2 = cs_cons(csound, const_cast<char*>("d"), list2);
    list2 = cs_cons(csound, const_cast<char*>("e"), list2);
    list2 = cs_cons(csound, const_cast<char*>("f"), list2);
    
    ASSERT_EQ (cs_cons_length(list1), 3);
    ASSERT_EQ (cs_cons_length(list2), 3);
    
    list1 = cs_cons_append(list1, list2);
    
    ASSERT_EQ (cs_cons_length(list1), 6);
    ASSERT_EQ (cs_cons_length(list2), 3);
}

TEST_F (CsoundDataStructuresTests, testCsHashTable)
{
    char* testValue = (char *) "test";
    
    CS_HASH_TABLE* hashTable = cs_hash_table_create(csound);
    
    cs_hash_table_put(csound, hashTable, (char *) "a", const_cast<char*>("1"));
    cs_hash_table_put(csound, hashTable, (char *) "b", const_cast<char*>("2"));
    cs_hash_table_put(csound, hashTable, (char *) "c", const_cast<char*>("3"));
    
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "a"), "1");
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "b"), "2");
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "c"), "3");
    
    cs_hash_table_remove(csound, hashTable, (char *) "c");
    
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "a"), "1");
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "b"), "2");
    ASSERT_TRUE ((char*)cs_hash_table_get(csound, hashTable, (char *) "c") == NULL);
    
    cs_hash_table_put(csound, hashTable, testValue, NULL);
    
    ASSERT_STREQ ((char*)cs_hash_table_get_key(csound, hashTable, (char *) "test"), "test");
    ASSERT_TRUE (cs_hash_table_get_key(csound, hashTable, (char *) "test") != testValue);
}

TEST_F (CsoundDataStructuresTests, testCsHasTableMerge)
{
    CS_HASH_TABLE* hashTable = cs_hash_table_create(csound);
    CS_HASH_TABLE* hashTable2 = cs_hash_table_create(csound);
    
    cs_hash_table_put(csound, hashTable, (char *) "a", const_cast<char*>("1"));
    cs_hash_table_put(csound, hashTable, (char *) "b", const_cast<char*>("2"));
    cs_hash_table_put(csound, hashTable, (char *) "c", const_cast<char*>("3"));
    
    cs_hash_table_put(csound, hashTable2, (char *) "b", const_cast<char*>("4"));
    cs_hash_table_put(csound, hashTable2, (char *) "c", const_cast<char*>("5"));
    cs_hash_table_put(csound, hashTable2, (char *) "d", const_cast<char*>("6"));
    
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "a"), "1");
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "b"), "2");
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "c"), "3");

    cs_hash_table_merge(csound, hashTable, hashTable2);
    
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "a"), "1");
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "b"), "4");
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "c"), "5");
    ASSERT_STREQ ((char*)cs_hash_table_get(csound, hashTable, (char *) "d"), "6");    
}

TEST_F (CsoundDataStructuresTests, testCsHashTableGetPutKey)
{
    CS_HASH_TABLE* hashTable = cs_hash_table_create(csound);
    auto a = cs_hash_table_put_key(csound, hashTable, (char *) "test");
    auto b = cs_hash_table_put_key(csound, hashTable, (char *) "test");
    auto c = cs_hash_table_get_key(csound, hashTable, (char *) "test");
    auto d = cs_hash_table_get_key(csound, hashTable, (char *) "some other value");
    
    ASSERT_EQ (a, b);
    ASSERT_EQ (b, c);
    ASSERT_TRUE (d == NULL);
}
