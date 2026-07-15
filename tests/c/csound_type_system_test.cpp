/* 
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#define __BUILDING_LIBCSOUND

#include <stdio.h>
#include <stdlib.h>
#include <atomic>
#include <thread>
#include <vector>
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "arrays_internal.h"
#include "csoundCore.h"
#include "arrays.h"
#include "gtest/gtest.h"

class TypeSystemTests : public ::testing::Test {
public:
    TypeSystemTests ()
    {
    }

    virtual ~TypeSystemTests ()
    {
    }

    virtual void SetUp ()
    {
      csound = csoundCreate (0, 0);
      csoundCreateMessageBuffer (csound, 0);
      csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundDestroyMessageBuffer (csound);
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

TEST_F (TypeSystemTests, testTypeSystem)
{
  TYPE_POOL* pool = csound->typePool;
  CS_VAR_POOL* varPool = csound->engineState.varPool;
  
  CS_VARIABLE* var = csoundCreateVariable(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_A,
                                          const_cast<char*>("a1"), NULL);
  ASSERT_TRUE (var != NULL);
  
  csoundAddVariable(csound, varPool, var);
  
  CS_VARIABLE* var2 = csoundFindVariableWithName(csound, varPool, "a1");
  ASSERT_TRUE (var2 != NULL);
  ASSERT_STREQ (var2->varType->varTypeName, "a");
  ASSERT_STREQ (var2->varName, "a1");
  
  ASSERT_TRUE (csoundFindVariableWithName(csound, varPool, "a2") == NULL);
}

TEST_F (TypeSystemTests, testGetVarSimpleName)
{
    ASSERT_STREQ ("a1", csoundGetVarSimpleName(csound, "a1"));
    ASSERT_STREQ ("a1", csoundGetVarSimpleName(csound, "[a]1"));
    ASSERT_STREQ ("StestString", csoundGetVarSimpleName(csound, "StestString"));
    ASSERT_STREQ ("StestString", csoundGetVarSimpleName(csound, "[S]testString"));
}

TEST_F (TypeSystemTests, testArrayCopyPreservesDestinationCapacity)
{
    int32_t sourceSize = 2;
    MYFLT sourceData[] = {FL(11.0), FL(22.0)};
    ARRAYDAT source{};
    ARRAYDAT destination{};

    source.dimensions = 1;
    source.sizes = &sourceSize;
    source.arrayMemberSize = sizeof(MYFLT);
    source.arrayType = &CS_VAR_TYPE_I;
    source.data = sourceData;
    source.allocated = sizeof(sourceData);

    destination.dimensions = 1;
    destination.sizes = static_cast<int32_t *>(
      csound->Calloc(csound, sizeof(int32_t)));
    destination.sizes[0] = 8;
    destination.arrayMemberSize = sizeof(MYFLT);
    destination.arrayType = &CS_VAR_TYPE_I;
    destination.allocated = sizeof(MYFLT) * 8;
    destination.data = static_cast<MYFLT *>(
      csound->Calloc(csound, destination.allocated));
    MYFLT *originalAllocation = destination.data;

    ASSERT_EQ(OK, csound_array_copy_independent(
                    csound, &destination, &source, nullptr,
                    CSOUND_ARRAY_COPY_ALLOW_ALLOCATION));
    EXPECT_EQ(originalAllocation, destination.data);
    EXPECT_EQ(sizeof(MYFLT) * 8, destination.allocated);
    ASSERT_EQ(1, destination.dimensions);
    ASSERT_NE(nullptr, destination.sizes);
    EXPECT_EQ(2, destination.sizes[0]);
    EXPECT_EQ(FL(11.0), destination.data[0]);
    EXPECT_EQ(FL(22.0), destination.data[1]);

    csound_free_array_storage(csound, &destination);
}

TEST_F (TypeSystemTests, testIndependentArrayCopyReportsTypeMismatch)
{
    int32_t sourceSize = 1;
    MYFLT sourceData[] = {FL(1.0)};
    ARRAYDAT source{};
    ARRAYDAT destination{};

    source.dimensions = 1;
    source.sizes = &sourceSize;
    source.arrayMemberSize = sizeof(MYFLT);
    source.arrayType = &CS_VAR_TYPE_I;
    source.data = sourceData;
    source.allocated = sizeof(sourceData);
    destination.arrayType = &CS_VAR_TYPE_S;

    EXPECT_EQ(NOTOK, csound_array_copy_independent(
                       csound, &destination, &source, nullptr,
                       CSOUND_ARRAY_COPY_ALLOW_ALLOCATION));
    EXPECT_EQ(nullptr, destination.data);
    EXPECT_EQ(0u, destination.allocated);
}

TEST_F (TypeSystemTests, testConcurrentStructuredArrayCopiesShareOneStorage)
{
    constexpr int32_t readerCount = 8;
    CS_TYPE elementType = CS_VAR_TYPE_I;
    ARRAYDAT source{};
    std::vector<ARRAYDAT> destinations(readerCount);
    std::vector<std::thread> readers;
    std::atomic<int32_t> ready{0};
    std::atomic<bool> start{false};

    elementType.userDefinedType = 1;
    source.dimensions = 1;
    source.sizes = static_cast<int32_t *>(
      csound->Calloc(csound, sizeof(int32_t)));
    source.sizes[0] = 1;
    source.arrayMemberSize = sizeof(MYFLT);
    source.arrayType = &elementType;
    source.data = static_cast<MYFLT *>(
      csound->Calloc(csound, sizeof(MYFLT)));
    source.data[0] = FL(17.0);
    source.allocated = sizeof(MYFLT);

    for (ARRAYDAT &destination : destinations) {
        destination.arrayType = &elementType;
        readers.emplace_back([&, destinationPtr = &destination]() {
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            CS_VAR_TYPE_ARRAY.copyValue(csound, &CS_VAR_TYPE_ARRAY,
                                        destinationPtr, &source, nullptr);
        });
    }
    while (ready.load(std::memory_order_acquire) != readerCount) {
        std::this_thread::yield();
    }
    start.store(true, std::memory_order_release);
    for (std::thread &reader : readers) {
        reader.join();
    }

    ASSERT_NE(nullptr, source.storage);
    for (ARRAYDAT &destination : destinations) {
        EXPECT_EQ(source.storage, destination.storage);
        EXPECT_EQ(source.data, destination.data);
        ASSERT_NE(nullptr, destination.data);
        EXPECT_EQ(FL(17.0), destination.data[0]);
        csound_free_array_storage(csound, &destination);
    }
    csound_free_array_storage(csound, &source);
}

TEST_F (TypeSystemTests, testStructuredArrayWritePreparationUsesExplicitPolicy)
{
    CS_TYPE elementType = CS_VAR_TYPE_I;
    ARRAYDAT source{};
    ARRAYDAT shared{};

    elementType.userDefinedType = 1;
    source.dimensions = 1;
    source.sizes = static_cast<int32_t *>(
      csound->Calloc(csound, sizeof(int32_t)));
    source.sizes[0] = 1;
    source.arrayMemberSize = sizeof(MYFLT);
    source.arrayType = &elementType;
    source.data = static_cast<MYFLT *>(
      csound->Calloc(csound, sizeof(MYFLT)));
    source.data[0] = FL(23.0);
    source.allocated = sizeof(MYFLT);
    shared.arrayType = &elementType;

    CS_VAR_TYPE_ARRAY.copyValue(csound, &CS_VAR_TYPE_ARRAY,
                                &shared, &source, nullptr);
    ASSERT_NE(nullptr, source.storage);
    ASSERT_EQ(source.storage, shared.storage);
    MYFLT *const originalData = source.data;
    auto *const originalStorage = source.storage;

    EXPECT_EQ(NOTOK, csound_array_try_prepare_write(
                       csound, &source, nullptr));
    EXPECT_EQ(originalStorage, source.storage);
    EXPECT_EQ(originalData, source.data);
    EXPECT_EQ(originalStorage, shared.storage);

    EXPECT_EQ(OK, csound_array_prepare_write(
                    csound, &source, nullptr));
    EXPECT_EQ(nullptr, source.storage);
    EXPECT_NE(originalData, source.data);
    EXPECT_EQ(sizeof(MYFLT), source.allocated);
    EXPECT_EQ(FL(23.0), source.data[0]);
    EXPECT_EQ(originalStorage, shared.storage);
    EXPECT_EQ(originalData, shared.data);

    EXPECT_EQ(OK, csound_array_try_prepare_write(
                    csound, &shared, nullptr));
    EXPECT_EQ(nullptr, shared.storage);
    EXPECT_EQ(originalData, shared.data);

    csound_free_array_storage(csound, &source);
    csound_free_array_storage(csound, &shared);
}

//void test_array_name_variable_clashing(void)
//{
//    CSOUND* csound = csoundCreate(NULL);
//    
//    TYPE_POOL* pool = csound->typePool;
//    CS_VAR_POOL* varPool = csound->engineState.varPool;
//
//    csoundAddStandardTypes(csound, pool);
//    
//    CS_VARIABLE* var = csoundCreateVariable(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_A, "a1", NULL);
//    CU_ASSERT_PTR_NOT_NULL(var);
//    //printf("Var type created: %s\n", var->varType->varTypeName);
//
//    csoundAddVariable(varPool, var);
//    
//    CS_VARIABLE* var2 = csoundFindVariableWithName(csound, varPool, "a1");
//    CU_ASSERT_PTR_EQUAL(var, var2);
//    // should return "a1", as "[a;1" is originally a1[]
//    var2 = csoundFindVariableWithName(csound, varPool, "[a;1");
//    CU_ASSERT_PTR_EQUAL(var, var2);
//    
//}
