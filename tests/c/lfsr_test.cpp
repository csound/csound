#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cstdlib>
#include <limits>

#define lfsr_init_modules test_lfsr_init_modules
#include "../../Opcodes/lfsr.cpp"

namespace {
int32_t ignoreInitError(CSOUND *, const char *, ...) { return NOTOK; }

struct Instance {
    LFSR opcode = {};
    OPTXT text = {};
    MYFLT args[3] = {8, 255, -1};
    MYFLT output = 0;
    explicit Instance(CSOUND *engine) {
        opcode.csound = reinterpret_cast<csnd::Csound *>(engine);
        text.t.inArgCount = 3;
        opcode.optext = &text;
        for (int i = 0; i < 3; ++i) opcode.inargs.begin()[i] = &args[i];
        opcode.outargs.begin()[0] = &output;
    }
};

class LfsrTests : public ::testing::Test {
protected:
    CSOUND *engine;
    void SetUp() override {
        engine = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(engine, 0);
        engine->InitError = ignoreInitError;
    }
    void TearDown() override { csoundDestroy(engine); }
};

TEST_F(LfsrTests, SeedBitPatternsAndDefaults) {
    Instance instance(engine);
    const struct { MYFLT seed; uint32_t expected; } cases[] = {
        {-1, UINT32_MAX}, {-2, UINT32_MAX - 1}, {-1.75, UINT32_MAX},
        {-.5, 0}, {0, 0}, {1, 1}, {4294967296.0, 0},
        {-4294967296.0, 0}, {4294967040.0, 0xffffff00u}
    };
    for (const auto &test : cases) {
        instance.args[2] = test.seed;
        ASSERT_EQ(instance.opcode.init(), OK);
        EXPECT_EQ(instance.opcode.shift_register_, test.expected);
    }
    instance.text.t.inArgCount = 2;
    ASSERT_EQ(instance.opcode.init(), OK);
    EXPECT_EQ(instance.opcode.shift_register_, UINT32_MAX);
}

TEST_F(LfsrTests, RejectInvalidLengthsProbabilitiesAndSeeds) {
    Instance instance(engine);
    const MYFLT nan = std::numeric_limits<MYFLT>::quiet_NaN();
    const MYFLT inf = std::numeric_limits<MYFLT>::infinity();
    for (MYFLT length : {MYFLT(-1), MYFLT(0), MYFLT(32), MYFLT(256), nan, inf}) {
        instance.args[0] = length;
        EXPECT_EQ(instance.opcode.init(), NOTOK);
    }
    instance.args[0] = 8;
    for (MYFLT probability : {MYFLT(-1), MYFLT(0), MYFLT(256), nan, inf}) {
        instance.args[1] = probability;
        EXPECT_EQ(instance.opcode.init(), NOTOK);
    }
    instance.args[1] = 255;
    for (MYFLT seed : {nan, inf, -inf}) {
        instance.args[2] = seed;
        EXPECT_EQ(instance.opcode.init(), NOTOK);
    }
}

TEST_F(LfsrTests, ValidRegistersStayWithinTheirOutputRange) {
    Instance instance(engine);
    for (int length = 1; length <= 31; ++length) {
        for (int probability : {1, 128, 255}) {
            instance.args[0] = length;
            instance.args[1] = probability;
            instance.args[2] = 0;
            ASSERT_EQ(instance.opcode.init(), OK);
            for (int i = 0; i < 256; ++i) {
                ASSERT_EQ(instance.opcode.kperf(), OK);
                EXPECT_GE(instance.output, 0);
                // MYFLT may round the largest 31-bit integer upward.
                EXPECT_LE(instance.output, static_cast<MYFLT>((1u << length) - 1));
            }
        }
    }
}

TEST_F(LfsrTests, InstancesDoNotReseedOrShareRandomState) {
    Instance first(engine), second(engine), other(engine);
    first.args[1] = second.args[1] = other.args[1] = 128;
    *engine->RandSeed31(engine) = 12345;
    ASSERT_EQ(first.opcode.init(), OK);
    *engine->RandSeed31(engine) = 12345;
    ASSERT_EQ(second.opcode.init(), OK);
    std::srand(42);
    int expected = std::rand();
    std::srand(42);
    ASSERT_EQ(other.opcode.init(), OK);
    for (int i = 0; i < 256; ++i) {
        ASSERT_EQ(first.opcode.kperf(), OK);
        ASSERT_EQ(other.opcode.kperf(), OK);
        ASSERT_EQ(second.opcode.kperf(), OK);
        EXPECT_EQ(first.output, second.output);
    }
    EXPECT_EQ(std::rand(), expected);
}
} // namespace
