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
#include "csound_orc_structs.h"
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

namespace {
const CS_TYPE* constructorType = nullptr;
const void* constructorTypeArg = nullptr;
INSDS* constructorContext = nullptr;

CS_VARIABLE* createConstructorProbe(void* cs, const CS_TYPE* type,
                                    const void* typeArg, INSDS* ctx)
{
    CSOUND* csound = static_cast<CSOUND*>(cs);
    constructorType = type;
    constructorTypeArg = typeArg;
    constructorContext = ctx;
    CS_VARIABLE* var = static_cast<CS_VARIABLE*>(
      csound->Calloc(csound, sizeof(CS_VARIABLE)));
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(MYFLT));
    return var;
}
}

TEST_F (TypeSystemTests, testCreateVariableForTypeSeparatesArguments)
{
    CS_TYPE probeType{
      const_cast<char*>("ConstructorProbe"),
      const_cast<char*>("constructor callback probe"),
      CS_ARG_TYPE_BOTH,
      createConstructorProbe,
      nullptr,
      nullptr,
      nullptr,
      0
    };
    int32_t typeArg = 42;
    INSDS context{};

    constructorType = nullptr;
    constructorTypeArg = nullptr;
    constructorContext = nullptr;
    CS_VARIABLE* var = csoundCreateVariableForType(
      csound, &probeType, &typeArg, &context);

    ASSERT_NE(nullptr, var);
    EXPECT_EQ(&probeType, constructorType);
    EXPECT_EQ(&typeArg, constructorTypeArg);
    EXPECT_EQ(&context, constructorContext);
    EXPECT_EQ(&probeType, var->varType);
    csound->Free(csound, var);
}

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
    EXPECT_NE(originalStorage, shared.storage);
    EXPECT_TRUE(csound_array_storage_matches(csound, &shared));
    EXPECT_EQ(originalData, shared.data);

    csound_free_array_storage(csound, &source);
    csound_free_array_storage(csound, &shared);
}

TEST_F (TypeSystemTests, testStructuredArrayCopyAndWriteClaimAreSerialized)
{
    constexpr int32_t iterationCount = 500;
    const CS_TYPE *elementType;

    ASSERT_EQ(CSOUND_SUCCESS,
              csoundCompileOrc(csound,
                               "struct ConcurrentValue value:i\n", 0));
    elementType = csoundGetTypeWithVarTypeName(
      csound->typePool, ":ConcurrentValue;");
    ASSERT_NE(nullptr, elementType);
    ASSERT_TRUE(elementType->userDefinedType);

    for (int32_t iteration = 0; iteration < iterationCount; iteration++) {
        ARRAYDAT source{};
        ARRAYDAT initialView{};
        ARRAYDAT destination{};
        CS_VARIABLE *elementVariable;
        CS_STRUCT_VAR *sourceValue;
        CS_STRUCT_VAR *destinationValue;
        std::atomic<int32_t> ready{0};
        std::atomic<bool> start{false};
        std::atomic<int32_t> writeResult{NOTOK};

        source.dimensions = 1;
        source.sizes = static_cast<int32_t *>(
          csound->Calloc(csound, sizeof(int32_t)));
        source.sizes[0] = 1;
        source.arrayType = elementType;
        elementVariable = csoundCreateVariableForType(
          csound, elementType, nullptr, nullptr);
        ASSERT_NE(nullptr, elementVariable);
        ASSERT_NE(nullptr, elementVariable->initializeVariableMemory);
        source.arrayMemberSize = elementVariable->memBlockSize;
        source.data = static_cast<MYFLT *>(
          csound->Calloc(csound, (size_t)source.arrayMemberSize));
        elementVariable->initializeVariableMemory(
          csound, elementVariable, source.data);
        csound->Free(csound, elementVariable);
        source.allocated = (size_t)source.arrayMemberSize;
        sourceValue = reinterpret_cast<CS_STRUCT_VAR *>(source.data);
        ASSERT_NE(nullptr, sourceValue->members);
        sourceValue->members[0]->value = FL(31.0);
        initialView.arrayType = elementType;
        destination.arrayType = elementType;

        CS_VAR_TYPE_ARRAY.copyValue(csound, &CS_VAR_TYPE_ARRAY,
                                    &initialView, &source, nullptr);
        ASSERT_NE(nullptr, source.storage);
        csound_free_array_storage(csound, &initialView);

        std::thread copier([&]() {
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            CS_VAR_TYPE_ARRAY.copyValue(csound, &CS_VAR_TYPE_ARRAY,
                                        &destination, &source, nullptr);
        });
        std::thread writer([&]() {
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            writeResult.store(csound_array_try_prepare_write(
                                csound, &source, nullptr),
                              std::memory_order_release);
        });

        while (ready.load(std::memory_order_acquire) != 2) {
            std::this_thread::yield();
        }
        start.store(true, std::memory_order_release);
        copier.join();
        writer.join();

        ASSERT_NE(nullptr, source.data);
        ASSERT_NE(nullptr, destination.data);
        sourceValue = reinterpret_cast<CS_STRUCT_VAR *>(source.data);
        destinationValue = reinterpret_cast<CS_STRUCT_VAR *>(
          destination.data);
        ASSERT_NE(nullptr, sourceValue->members);
        ASSERT_NE(nullptr, destinationValue->members);
        EXPECT_EQ(FL(31.0), sourceValue->members[0]->value);
        EXPECT_EQ(FL(31.0), destinationValue->members[0]->value);
        EXPECT_TRUE(csound_array_storage_matches(csound, &source));
        EXPECT_TRUE(csound_array_storage_matches(csound, &destination));
        if (writeResult.load(std::memory_order_acquire) == OK) {
            EXPECT_NE(source.data, destination.data);
        }
        else {
            EXPECT_EQ(NOTOK, writeResult.load(std::memory_order_acquire));
            EXPECT_EQ(source.data, destination.data);
            EXPECT_EQ(source.storage, destination.storage);
        }

        csound_free_array_storage(csound, &source);
        csound_free_array_storage(csound, &destination);
    }
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
