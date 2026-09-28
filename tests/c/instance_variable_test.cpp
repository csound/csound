#define __BUILDING_LIBCSOUND
#include "ugen_internal.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

namespace {

constexpr const char *stateName = "test.instance.value";

struct Creation {
    INSDS *owner;
    cs_float *storage;
    cs_float previousValue;
};

struct Observations {
    std::vector<Creation> creations;
    std::vector<cs_float> reads;
    std::vector<cs_float> deinits;
};

struct StoreOpcode {
    OPDS h;
    cs_float *value;
    cs_float *fail;
};

struct ReadOpcode {
    OPDS h;
    cs_float *out;
    cs_float *cached;
};

// These test opcodes use the same function pointers as an external plugin.
int32_t storeInit(CSOUND *csound, void *data)
{
    auto *p = static_cast<StoreOpcode *>(data);
    INSDS *owner = p->h.insdshead;
    auto *state = static_cast<cs_float *>(
        csound->QueryInstanceVariable(csound, owner, stateName));
    if (state == nullptr) {
        int32_t result = csound->CreateInstanceVariable(
            csound, owner, stateName, sizeof(cs_float));
        if (result != CSOUND_SUCCESS) return result;
        state = static_cast<cs_float *>(
            csound->QueryInstanceVariable(csound, owner, stateName));
    }
    auto *observed = static_cast<Observations *>(csoundGetHostData(csound));
    observed->creations.push_back({owner, state, *state});
    *state = *p->value;
    if (*p->fail != 0)
        return csound->InitError(csound, "test requested an init error");
    return CSOUND_SUCCESS;
}

int32_t storeDeinit(CSOUND *csound, void *data)
{
    auto *p = static_cast<StoreOpcode *>(data);
    auto *state = static_cast<cs_float *>(csound->QueryInstanceVariable(
        csound, p->h.insdshead, stateName));
    if (state == nullptr) return CSOUND_ERROR;
    auto *observed = static_cast<Observations *>(csoundGetHostData(csound));
    observed->deinits.push_back(*state);
    return CSOUND_SUCCESS;
}

int32_t readInit(CSOUND *csound, void *data)
{
    auto *p = static_cast<ReadOpcode *>(data);
    p->cached = static_cast<cs_float *>(csound->QueryInstanceVariable(
        csound, p->h.insdshead, stateName));
    if (p->cached == nullptr)
        return csound->InitError(csound, "test state is missing");
    auto *observed = static_cast<Observations *>(csoundGetHostData(csound));
    observed->reads.push_back(*p->cached);
    return CSOUND_SUCCESS;
}

int32_t readPerf(CSOUND *, void *data)
{
    auto *p = static_cast<ReadOpcode *>(data);
    *p->out = *p->cached;
    return CSOUND_SUCCESS;
}

class InstanceVariableTests : public ::testing::Test {
protected:
    Observations observed;
    CSOUND *csound = nullptr;
    UGEN_FACTORY *factory = nullptr;
    UGEN_CONTEXT *context = nullptr;

    void SetUp() override
    {
        csound = csoundCreate(&observed, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
        ASSERT_EQ(csoundAppendOpcode(csound, "test_state_store",
            sizeof(StoreOpcode), 0, "", "io", storeInit, nullptr, storeDeinit),
            CSOUND_SUCCESS);
        ASSERT_EQ(csoundAppendOpcode(csound, "test_state_read",
            sizeof(ReadOpcode), 0, "k", "", readInit, readPerf, nullptr),
            CSOUND_SUCCESS);
        factory = csoundUgenFactoryNew(csound);
        context = csoundUgenContextNew(factory);
    }

    void TearDown() override
    {
        csoundUgenContextDelete(context);
        csoundUgenFactoryDelete(factory);
        csoundDestroy(csound);
    }

    std::string messages()
    {
        std::string text;
        while (csoundGetMessageCnt(csound)) {
            text += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return text;
    }

    void start(const char *orchestra, const char *score)
    {
        const std::string csd =
            "<CsoundSynthesizer>\n<CsInstruments>\n"
            "sr=1000\nksmps=10\nnchnls=1\n0dbfs=1\n" +
            std::string(orchestra) + "\n</CsInstruments>\n<CsScore>\n" +
            score + "\n</CsScore>\n</CsoundSynthesizer>\n";
        ASSERT_EQ(csoundCompileCSD(csound, csd.c_str(), 1, 0), CSOUND_SUCCESS)
            << messages();
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS) << messages();
    }

    void performBlocks(int count)
    {
        for (int i = 0; i < count; ++i)
            ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS) << messages();
    }
};

TEST_F(InstanceVariableTests, NamesAndOwnersAreIndependent)
{
    INSDS *first = factory->insds;
    INSDS *second = context->insds;
    char name[] = "family.first";
    ASSERT_EQ(csound->CreateInstanceVariable(
        csound, first, name, sizeof(cs_double)), CSOUND_SUCCESS);
    auto *value = static_cast<cs_double *>(
        csound->QueryInstanceVariable(csound, first, name));
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 0);
    *value = 7;

    // The entry owns its name and adding another entry leaves its pointer valid.
    name[0] = 'x';
    ASSERT_EQ(csound->CreateInstanceVariable(csound, first, "family.second", 8), 0);
    EXPECT_EQ(csound->QueryInstanceVariable(csound, first, "family.first"), value);
    EXPECT_EQ(*value, 7);
    EXPECT_EQ(csound->QueryInstanceVariable(csound, second, "family.first"), nullptr);
    ASSERT_EQ(csound->CreateInstanceVariable(
        csound, second, "family.first", sizeof(cs_double)), CSOUND_SUCCESS);
    auto *other = static_cast<cs_double *>(
        csound->QueryInstanceVariable(csound, second, "family.first"));
    ASSERT_NE(other, nullptr);
    EXPECT_NE(other, value);
    EXPECT_EQ(*other, 0);
}

TEST_F(InstanceVariableTests, RejectsInvalidRequestsWithoutChangingExistingData)
{
    INSDS *owner = factory->insds;
    ASSERT_EQ(csound->CreateInstanceVariable(
        csound, owner, stateName, sizeof(int)), CSOUND_SUCCESS);
    auto *value = static_cast<int *>(
        csound->QueryInstanceVariable(csound, owner, stateName));
    ASSERT_NE(value, nullptr);
    *value = 42;
    EXPECT_EQ(csound->CreateInstanceVariable(csound, owner, stateName, 64), CSOUND_ERROR);
    EXPECT_EQ(csound->CreateInstanceVariable(csound, owner, "empty", 0), CSOUND_ERROR);
    EXPECT_EQ(csound->CreateInstanceVariable(csound, owner, "large", SIZE_MAX), CSOUND_ERROR);
    const char *invalidNames[] = {nullptr, ""};
    for (const char *name : invalidNames) {
        EXPECT_EQ(csound->CreateInstanceVariable(csound, owner, name, 8), CSOUND_ERROR);
        EXPECT_EQ(csound->QueryInstanceVariable(csound, owner, name), nullptr);
    }
    EXPECT_EQ(csound->CreateInstanceVariable(csound, nullptr, "owner", 8), CSOUND_ERROR);
    EXPECT_EQ(csound->QueryInstanceVariable(csound, nullptr, "owner"), nullptr);
    EXPECT_EQ(csound->CreateInstanceVariable(nullptr, owner, "engine", 8), CSOUND_ERROR);
    EXPECT_EQ(csound->QueryInstanceVariable(nullptr, owner, stateName), nullptr);
    CSOUND *otherEngine = csoundCreate(nullptr, nullptr);
    EXPECT_EQ(csound->CreateInstanceVariable(otherEngine, owner, "engine", 8), CSOUND_ERROR);
    EXPECT_EQ(csound->QueryInstanceVariable(otherEngine, owner, stateName), nullptr);
    csoundDestroy(otherEngine);
    EXPECT_EQ(*value, 42);
}

TEST_F(InstanceVariableTests, OverlappingNotesKeepTheirOwnValues)
{
    ASSERT_NO_FATAL_FAILURE(start(R"(
instr 1
 test_state_store p4
 kValue test_state_read
 chnset kValue, "first"
endin
instr 2
 test_state_store p4
 kValue test_state_read
 chnset kValue, "second"
endin
)", "i 1 0 .03 11\ni 2 0 .06 22\nf 0 .1"));
    ASSERT_NO_FATAL_FAILURE(performBlocks(1));
    EXPECT_EQ(csoundGetControlChannel(csound, "first", nullptr), 11);
    EXPECT_EQ(csoundGetControlChannel(csound, "second", nullptr), 22);
    ASSERT_NO_FATAL_FAILURE(performBlocks(4));
    EXPECT_EQ(csoundGetControlChannel(csound, "second", nullptr), 22);
    ASSERT_NO_FATAL_FAILURE(performBlocks(2));
    EXPECT_EQ(observed.deinits, (std::vector<cs_float>{11, 22}));
}

TEST_F(InstanceVariableTests, NoteReuseStartsEmptyButReusesStorage)
{
    ASSERT_NO_FATAL_FAILURE(start(R"(
instr 1
 test_state_store p4
 kValue test_state_read
endin
)", "i 1 0 .02 11\ni 1 .04 .02 22\nf 0 .1"));
    ASSERT_NO_FATAL_FAILURE(performBlocks(3));
    ASSERT_EQ(observed.creations.size(), 1u);
    INSDS *owner = observed.creations[0].owner;
    EXPECT_EQ(csound->QueryInstanceVariable(csound, owner, stateName), nullptr);
    ASSERT_NO_FATAL_FAILURE(performBlocks(4));
    ASSERT_EQ(observed.creations.size(), 2u);
    EXPECT_EQ(observed.creations[1].owner, owner);
    EXPECT_EQ(observed.creations[1].storage, observed.creations[0].storage);
    EXPECT_EQ(observed.creations[0].previousValue, 0);
    EXPECT_EQ(observed.creations[1].previousValue, 0);
    EXPECT_EQ(observed.deinits, (std::vector<cs_float>{11, 22}));
}

TEST_F(InstanceVariableTests, ReaderOnlyReinitKeepsTheSource)
{
    ASSERT_NO_FATAL_FAILURE(start(R"(
instr 1
 test_state_store 17
 kPass timeinstk
 if kPass == 2 then
  reinit ReadAgain
 endif
ReadAgain:
 kValue test_state_read
 rireturn
 chnset kValue, "value"
endin
)", "i 1 0 .05\nf 0 .1"));
    ASSERT_NO_FATAL_FAILURE(performBlocks(6));
    ASSERT_EQ(observed.creations.size(), 1u);
    EXPECT_EQ(observed.reads, (std::vector<cs_float>{17, 17}));
    EXPECT_EQ(csoundGetControlChannel(csound, "value", nullptr), 17);
    EXPECT_EQ(observed.deinits, (std::vector<cs_float>{17}));
}

TEST_F(InstanceVariableTests, TiedNotesKeepTheEntry)
{
    ASSERT_NO_FATAL_FAILURE(start(R"(
instr 1
 test_state_store p4
 kValue test_state_read
endin
)", "i 1 0 -.03 11\ni 1 .03 .03 22\nf 0 .1"));
    ASSERT_NO_FATAL_FAILURE(performBlocks(7));
    ASSERT_EQ(observed.creations.size(), 2u);
    EXPECT_EQ(observed.creations[0].previousValue, 0);
    EXPECT_EQ(observed.creations[1].previousValue, 11);
    EXPECT_EQ(observed.creations[0].storage, observed.creations[1].storage);
    EXPECT_EQ(observed.deinits, (std::vector<cs_float>{22}));
}

TEST_F(InstanceVariableTests, UgensShareOnlyTheirChosenContext)
{
    ASSERT_NO_FATAL_FAILURE(start("instr 1\nendin", "f 0 .1"));
    UGEN *source = csoundUgenNew(factory, const_cast<char *>("test_state_store"),
                               const_cast<char *>(""), const_cast<char *>("io"));
    UGEN *reader = csoundUgenNew(factory, const_cast<char *>("test_state_read"),
                               const_cast<char *>("k"), const_cast<char *>(""));
    ASSERT_NE(source, nullptr);
    ASSERT_NE(reader, nullptr);
    ASSERT_TRUE(csoundUgenSetContext(source, context));
    ASSERT_TRUE(csoundUgenSetContext(reader, context));
    csoundUgenVarSetValue(csoundUgenGetInVar(source, 0), 23);
    ASSERT_EQ(csoundUgenInit(source), CSOUND_SUCCESS);
    ASSERT_EQ(csoundUgenInit(reader), CSOUND_SUCCESS);
    ASSERT_EQ(csoundUgenPerform(reader), CSOUND_SUCCESS);
    EXPECT_EQ(csoundUgenVarGetValue(csoundUgenGetOutVar(reader, 0)), 23);
    EXPECT_EQ(csound->QueryInstanceVariable(csound, factory->insds, stateName), nullptr);

    // The context must outlive the UGENs so their deinit can still query it.
    EXPECT_TRUE(csoundUgenDelete(reader));
    EXPECT_TRUE(csoundUgenDelete(source));
    EXPECT_EQ(observed.deinits, (std::vector<cs_float>{23}));
    EXPECT_NE(csound->QueryInstanceVariable(csound, context->insds, stateName), nullptr);
}

TEST_F(InstanceVariableTests, NestedUdosAndSubinstrKeepSeparateScopes)
{
    ASSERT_NO_FATAL_FAILURE(start(R"(
opcode Inner, k, 0
 test_state_store 33
 kValue test_state_read
 xout kValue
endop
opcode Outer, k, 0
 test_state_store 22
 kInner Inner
 kOuter test_state_read
 xout kOuter + kInner
endop
instr 1
 test_state_store 11
 kNested Outer
 aSub subinstr 2
 kParent test_state_read
 chnset kParent + kNested + k(aSub), "total"
endin
instr 2
 test_state_store 44
 kValue test_state_read
 aValue = kValue
 out aValue
endin
)", "i 1 0 .03\nf 0 .1"));
    ASSERT_NO_FATAL_FAILURE(performBlocks(1));
    EXPECT_EQ(csoundGetControlChannel(csound, "total", nullptr), 110);
    ASSERT_EQ(observed.creations.size(), 4u);
    for (size_t i = 0; i < observed.creations.size(); ++i)
        for (size_t j = 0; j < i; ++j)
            EXPECT_NE(observed.creations[i].owner, observed.creations[j].owner);
    ASSERT_NO_FATAL_FAILURE(performBlocks(3));
    EXPECT_EQ(observed.deinits.size(), 4u);
}

TEST_F(InstanceVariableTests, FailedInitStillDeinitializesAndDiscardsState)
{
    ASSERT_NO_FATAL_FAILURE(start(R"(
instr 1
 test_state_store 99, 1
endin
)", "i 1 0 .03\nf 0 .1"));
    csoundPerformKsmps(csound);
    ASSERT_EQ(observed.creations.size(), 1u);
    EXPECT_NE(messages().find("test requested an init error"), std::string::npos);
    EXPECT_EQ(observed.deinits, (std::vector<cs_float>{99}));
    EXPECT_EQ(csound->QueryInstanceVariable(csound, observed.creations[0].owner,
                                          stateName), nullptr);
}

} // namespace
