#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <initializer_list>
#include <string>
#include <tuple>

namespace {
class LinlinArrayTestBase : public ::testing::Test {
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
    void start(const std::string &declarations, const std::string &body) {
        if (csound) csoundDestroy(csound);
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        csoundSetOption(csound, "--daemon");
        const std::string orc = "sr=1024\nksmps=16\nnchnls=1\n" +
            declarations + "\ninstr 1\n" + body + "\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0) << messages();
        csoundEventString(csound, "i1 0 1", 0);
        ASSERT_EQ(csoundStart(csound), 0) << messages();
    }
    void expectArray(const std::string &name,
                     std::initializer_list<MYFLT> expected) {
        CS_VARIABLE *var = csoundFindVariableWithName(
            csound, csound->engineState.varPool, name.c_str());
        ASSERT_NE(var, nullptr);
        auto *array = reinterpret_cast<ARRAYDAT *>(&var->memBlock->value);
        ASSERT_EQ(array->dimensions, 1);
        ASSERT_EQ(array->sizes[0], expected.size());
        int i = 0;
        for (MYFLT value : expected) { EXPECT_EQ(array->data[i], value) << i; ++i; }
    }
};

class LinlinArrayTests : public LinlinArrayTestBase,
                        public ::testing::WithParamInterface<std::tuple<bool,bool>> {
protected:
    bool control() { return std::get<0>(GetParam()); }
    bool blend() { return std::get<1>(GetParam()); }
    std::string prefix() { return control() ? "gk" : "gi"; }
    std::string declarations() {
        return prefix() + "A[] fillarray 0,0.5,1\n" +
               prefix() + "B[] fillarray 10,20,30,40\n" +
               prefix() + "Out[] fillarray 7,8,9\n";
    }
    std::string call(const std::string &out = "Out") {
        return prefix() + out + " linlin " +
            (blend() ? "0.5," + prefix() + "A," + prefix() + "B"
                     : prefix() + "A,10,20");
    }
};

TEST_P(LinlinArrayTests, PreservesMappingAndShorterBlendLength) {
    ASSERT_NO_FATAL_FAILURE(start(declarations(), call()));
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    EXPECT_EQ(csound->inerrcnt + csound->perferrcnt, 0) << messages();
    if (blend()) expectArray(prefix()+"Out", {5,10.25,15.5});
    else expectArray(prefix()+"Out", {10,15,20});
}

TEST_P(LinlinArrayTests, SupportsInPlaceOutput) {
    for (const char *out : {"A", "B"}) {
        if (!blend() && std::string(out) == "B") continue;
        ASSERT_NO_FATAL_FAILURE(start(declarations(), call(out)));
        ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
        EXPECT_EQ(csound->inerrcnt + csound->perferrcnt, 0) << messages();
        if (blend()) expectArray(prefix()+out, {5,10.25,15.5});
        else expectArray(prefix()+out, {10,15,20});
    }
}

TEST_P(LinlinArrayTests, RejectsInvalidInputShapeBeforeChangingOutput) {
    for (const char *input : {"A", "B"}) {
        if (!blend() && std::string(input) == "B") continue;
        const std::string decl = prefix()+"A"+
            (std::string(input)=="A" ? "[][] init 2,2\n" : "[] init 3\n") +
            prefix()+"B"+
            (std::string(input)=="B" ? "[][] init 2,2\n" : "[] init 3\n") +
            prefix()+"Out[] fillarray 7,8,9\n";
        ASSERT_NO_FATAL_FAILURE(start(decl, call()));
        csoundPerformKsmps(csound);
        EXPECT_GT(csound->inerrcnt, 0);
        EXPECT_NE(messages().find("linlin: expected one-dimensional arrays"),
                  std::string::npos);
        expectArray(prefix()+"Out", {7,8,9});
    }
}

TEST_P(LinlinArrayTests, ReportsZeroRangeAtTheCorrectRate) {
    ASSERT_NO_FATAL_FAILURE(start(declarations(), call()+",1,1"));
    csoundPerformKsmps(csound);
    if (control()) EXPECT_GT(csound->perferrcnt, 0);
    else EXPECT_GT(csound->inerrcnt, 0);
    const auto text = messages();
    EXPECT_NE(text.find("linlin: Division by zero"), std::string::npos);
    EXPECT_EQ(text.find("PerfError in wrong mode"), std::string::npos);
    expectArray(prefix()+"Out", {7,8,9});
}

TEST_F(LinlinArrayTestBase, StopsWhenEitherArrayFormOutgrowsItsOutput) {
    for (bool blend : {false, true}) {
        const std::string decl =
            "gkA[] fillarray 0,0.5,1\ngkB[] fillarray 10,20,30\n"
            "trim_i gkA,1\ntrim_i gkB,1\ngkOut[] init 0\n";
        const std::string op = blend ? "gkOut linlin 0.5,gkA,gkB\n"
                                     : "gkOut linlin gkA,10,20\n";
        ASSERT_NO_FATAL_FAILURE(start(decl, op+"trim gkA,3\ntrim gkB,3\n"));
        ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
        if (blend) expectArray("gkOut", {5});
        else expectArray("gkOut", {10});
        csoundPerformKsmps(csound);
        EXPECT_GT(csound->perferrcnt, 0);
        EXPECT_NE(messages().find("Array too small"), std::string::npos);
        if (blend) expectArray("gkOut", {5});
        else expectArray("gkOut", {10});
    }
}

TEST_F(LinlinArrayTestBase, TracksChangingLengthsWithinPreallocatedCapacity) {
    for (bool blend : {false, true}) {
        const std::string decl =
            "gkA[] fillarray 0,0.5,1\ngkB[] fillarray 10,20,30\n"
            "trim_i gkA,1\ntrim_i gkB,1\ngkOut[] init 3\n";
        const std::string op = blend ? "gkOut linlin 0.5,gkA,gkB\n"
                                     : "gkOut linlin gkA,10,20\n";
        ASSERT_NO_FATAL_FAILURE(start(decl, op+
            "kN = (timeinstk() == 1 ? 3 : 0)\ntrim gkA,kN\ntrim gkB,kN\n"));
        ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
        if (blend) expectArray("gkOut", {5});
        else expectArray("gkOut", {10});
        ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
        if (blend) expectArray("gkOut", {5,10.25,15.5});
        else expectArray("gkOut", {10,15,20});
        ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
        expectArray("gkOut", {});
        EXPECT_EQ(csound->inerrcnt + csound->perferrcnt, 0) << messages();
    }
}

INSTANTIATE_TEST_SUITE_P(RatesAndForms, LinlinArrayTests,
    ::testing::Combine(::testing::Bool(), ::testing::Bool()));
} // namespace
