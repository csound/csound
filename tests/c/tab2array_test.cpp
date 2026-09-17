#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <initializer_list>
#include <string>

namespace {
class Tab2arrayTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    void TearDown() override { if (csound) csoundDestroy(csound); }

    std::string messages() {
        std::string result;
        while (csoundGetMessageCnt(csound)) {
            result += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return result;
    }

    void start(const std::string &body, const std::string &declaration) {
        if (csound) csoundDestroy(csound);
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        csoundSetOption(csound, "--daemon");
        const std::string orc =
            "sr=1024\nksmps=16\nnchnls=1\n"
            "giTable ftgen 101,0,-9,-2,1,2,3,4,5,6,7,8,9\n" +
            declaration + "\ninstr 1\n" + body + "\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0) << messages();
        csoundEventString(csound, "i1 0 1", 0);
        ASSERT_EQ(csoundStart(csound), 0) << messages();
    }

    ARRAYDAT *array(const char *name) {
        CS_VARIABLE *var = csoundFindVariableWithName(
            csound, csound->engineState.varPool, name);
        return var ? reinterpret_cast<ARRAYDAT *>(&var->memBlock->value) : nullptr;
    }

    void expectArray(const char *name, std::initializer_list<MYFLT> expected) {
        ARRAYDAT *out = array(name);
        ASSERT_NE(out, nullptr);
        ASSERT_EQ(out->dimensions, 1);
        ASSERT_EQ(out->sizes[0], expected.size());
        int i = 0;
        for (MYFLT value : expected) { EXPECT_EQ(out->data[i], value) << i; ++i; }
    }
};

class Tab2arrayRateTests : public Tab2arrayTests,
                          public ::testing::WithParamInterface<bool> {
protected:
    const char *name() { return GetParam() ? "gkOut" : "giOut"; }
    void slice(const std::string &args) {
        start(std::string(name()) + " tab2array " + args,
              std::string(name()) + "[] init 0");
    }
};

TEST_P(Tab2arrayRateTests, CopiesSlicesAndUpdatesTheLogicalLength) {
    struct Case { const char *args; std::initializer_list<MYFLT> expected; };
    const Case cases[] = {
        {"101", {1,2,3,4,5,6,7,8,9}},
        {"101,1,8,3", {2,5,8}},
        {"101,1,7,3", {2,5}},
        {"101,2,100,2", {3,5,7,9}},
        {"101,2,-1,2", {3,5,7,9}},
        {"101,2.9,8.9,2.9", {3,5,7}},
        {"101,3,9,2147483520", {4}},
        {"101,8,2", {}}, {"101,3,3", {}}, {"101,9,0", {}}
    };
    for (const auto &test : cases) {
        SCOPED_TRACE(test.args);
        ASSERT_NO_FATAL_FAILURE(slice(test.args));
        ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
        EXPECT_EQ(csound->inerrcnt + csound->perferrcnt, 0) << messages();
        expectArray(name(), test.expected);
    }
}

TEST_P(Tab2arrayRateTests, RejectsInvalidSlicesAtInitialization) {
    for (const char *args : {"101,-1", "101,10", "101,0,9,0", "101,0,9,-1",
                             "101,0,9,0.5", "101,0,9,1e30", "101,1e30",
                             "101,0,1e30", "101,0,9,sqrt(-1)"}) {
        SCOPED_TRACE(args);
        ASSERT_NO_FATAL_FAILURE(slice(args));
        csoundPerformKsmps(csound);
        EXPECT_GT(csound->inerrcnt, 0);
        EXPECT_NE(messages().find("tab2array: invalid slice bounds or step"),
                  std::string::npos);
    }
}

TEST_P(Tab2arrayRateTests, RejectsInvalidTableNumbers) {
    for (const char *number : {"1e30", "sqrt(-1)"}) {
        ASSERT_NO_FATAL_FAILURE(slice(number));
        csoundPerformKsmps(csound);
        EXPECT_GT(csound->inerrcnt, 0);
        EXPECT_NE(messages().find("tab2array: table number out of range"),
                  std::string::npos);
    }
}

TEST_P(Tab2arrayRateTests, RejectsMultidimensionalOutputs) {
    ASSERT_NO_FATAL_FAILURE(start(std::string(name()) + " tab2array 101",
                                  std::string(name()) + "[][] init 3,3"));
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->inerrcnt, 0);
    EXPECT_NE(messages().find("tab2array: expected a one-dimensional output"),
              std::string::npos);
}

TEST_F(Tab2arrayTests, StopsBeforeWritingWhenTheSliceOutgrowsCapacity) {
    ASSERT_NO_FATAL_FAILURE(start(
        "kEnd init 2\ngkOut tab2array 101,0,kEnd\nkEnd = 9",
        "gkOut[] init 0"));
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectArray("gkOut", {1,2});
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->perferrcnt, 0);
    EXPECT_NE(messages().find("Array too small"), std::string::npos);
    expectArray("gkOut", {1,2});
}

TEST_F(Tab2arrayTests, ShrinksAndGrowsWithinPreallocatedCapacity) {
    ASSERT_NO_FATAL_FAILURE(start(
        "kEnd init 2\ngkOut tab2array 101,0,kEnd\nkEnd = 9",
        "gkOut[] init 9"));
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectArray("gkOut", {1,2});
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectArray("gkOut", {1,2,3,4,5,6,7,8,9});
    EXPECT_EQ(csound->inerrcnt + csound->perferrcnt, 0) << messages();
}

TEST_F(Tab2arrayTests, RejectsAnInvalidControlStepBeforeWriting) {
    ASSERT_NO_FATAL_FAILURE(start(
        "kStep init 2\ngkOut tab2array 101,0,0,kStep\nkStep = 0",
        "gkOut[] init 9"));
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectArray("gkOut", {1,3,5,7,9});
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->perferrcnt, 0);
    EXPECT_NE(messages().find("tab2array: invalid slice bounds or step"),
              std::string::npos);
    expectArray("gkOut", {1,3,5,7,9});
}

INSTANTIATE_TEST_SUITE_P(InitAndControl, Tab2arrayRateTests, ::testing::Bool());
} // namespace
