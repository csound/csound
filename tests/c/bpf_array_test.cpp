#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <initializer_list>
#include <string>

namespace {
class BpfArrayTests : public ::testing::TestWithParam<const char *> {
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
    void start(const std::string &decl, const std::string &body) {
        if (csound) csoundDestroy(csound);
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        csoundSetOption(csound, "--daemon");
        const std::string orc = "sr=1024\nksmps=16\nnchnls=1\n" +
            decl + "\ninstr 1\n" + body + "\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0) << messages();
        csoundEventString(csound, "i1 0 1", 0);
        ASSERT_EQ(csoundStart(csound), 0) << messages();
    }
    std::string call(const char *out = "gkOut", const char *points = "0,0,1,1,2,0") {
        return std::string(out) + " " + GetParam() + " gkIn," + points + "\n";
    }
    void expectArray(const char *name, std::initializer_list<double> expected) {
        CS_VARIABLE *var = csoundFindVariableWithName(
            csound, csound->engineState.varPool, name);
        ASSERT_NE(var, nullptr);
        auto *array = reinterpret_cast<ARRAYDAT *>(&var->memBlock->value);
        ASSERT_EQ(array->dimensions, 1);
        ASSERT_EQ(array->sizes[0], expected.size());
        int i = 0;
        for (double value : expected) {
            EXPECT_NEAR(array->data[i], value, 1e-6) << i;
            ++i;
        }
    }
    double quarter() { return std::string(GetParam()) == "bpf" ? .25 : .1464466094067262; }
    const std::string input = "gkIn[] fillarray -0.5,0,0.25,0.5,1,1.5,2,3\n";
};

TEST_P(BpfArrayTests, PreservesInterpolationAndInPlaceOutput) {
    for (const char *out : {"gkOut", "gkIn"}) {
        ASSERT_NO_FATAL_FAILURE(start(input+"gkOut[] init 0\n", call(out)));
        ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
        EXPECT_EQ(csound->inerrcnt + csound->perferrcnt, 0) << messages();
        expectArray(out, {0,0,quarter(),.5,1,.5,0,0});
    }
}

TEST_P(BpfArrayTests, StopsBeforeWritingWhenInputOutgrowsTheOutput) {
    ASSERT_NO_FATAL_FAILURE(start(input+"trim_i gkIn,1\ngkOut[] init 0\n",
        call()+"gkIn[0] = 0.25\ntrim gkIn,8\n"));
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectArray("gkOut", {0});
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->perferrcnt, 0);
    EXPECT_NE(messages().find("Array too small"), std::string::npos);
    expectArray("gkOut", {0});
}

TEST_P(BpfArrayTests, ChangesLengthWithinPreallocatedCapacity) {
    ASSERT_NO_FATAL_FAILURE(start(input+"trim_i gkIn,1\ngkOut[] init 8\n",
        call()+"kN = (timeinstk() == 1 ? 8 : 0)\ntrim gkIn,kN\n"));
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectArray("gkOut", {0});
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectArray("gkOut", {0,0,quarter(),.5,1,.5,0,0});
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectArray("gkOut", {});
    EXPECT_EQ(csound->inerrcnt + csound->perferrcnt, 0) << messages();
}

TEST_P(BpfArrayTests, RejectsInputShapeBeforeChangingOutput) {
    ASSERT_NO_FATAL_FAILURE(start("gkIn[][] init 2,2\ngkOut[] fillarray 7,8,9\n",
                                  call()));
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->inerrcnt, 0);
    EXPECT_NE(messages().find("bpf: expected one-dimensional arrays"),
              std::string::npos);
    expectArray("gkOut", {7,8,9});
}

TEST_P(BpfArrayTests, RejectsMultidimensionalOutput) {
    ASSERT_NO_FATAL_FAILURE(start(input+"gkOut[][] init 2,4\n", call()));
    csoundPerformKsmps(csound);
    EXPECT_GT(csound->inerrcnt, 0);
    EXPECT_NE(messages().find("bpf: expected one-dimensional arrays"),
              std::string::npos);
}

TEST_P(BpfArrayTests, RejectsInvalidPointCountsBeforeChangingOutput) {
    for (const char *points : {"0,0", "0,0,1"}) {
        ASSERT_NO_FATAL_FAILURE(start(input+"gkOut[] fillarray 7,8,9\n",
                                      call("gkOut", points)));
        csoundPerformKsmps(csound);
        EXPECT_GT(csound->inerrcnt, 0);
        const auto text = messages();
        const char *expected = std::string(points) == "0,0"
            ? "At least two pairs are needed, got 1"
            : "bpf: data length should be even";
        EXPECT_NE(text.find(expected), std::string::npos) << text;
        expectArray("gkOut", {7,8,9});
    }
}

INSTANTIATE_TEST_SUITE_P(LinearAndCosine, BpfArrayTests,
                         ::testing::Values("bpf", "bpfcos"));
} // namespace
