#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

namespace {
class DelayControlTests : public ::testing::Test {
protected:
    CSOUND *csound;
    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
    }
    void TearDown() override { csoundDestroy(csound); }
    std::vector<cs_float> render(const std::string &body, int cycles = 8,
                             const char *score = "i1 0 1\nf0 1") {
        const std::string orc =
            "sr=1000\nksmps=1\nnchnls=1\n0dbfs=1\n"
            "instr 1\nkInput timeinstk\n" + body +
            "\nchnset kResult,\"result\"\nendin";
        EXPECT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0);
        csoundEventString(csound, score, 0);
        EXPECT_EQ(csoundStart(csound), 0);
        std::vector<cs_float> values;
        for (int n=0; n<cycles; ++n) {
            if (csoundPerformKsmps(csound) != 0) break;
            values.push_back(csoundGetControlChannel(csound,"result",nullptr));
        }
        return values;
    }
    void expectSuccess() {
        EXPECT_EQ(csound->inerrcnt,0);
        EXPECT_EQ(csound->perferrcnt,0);
    }
};

class DelayControlHoldTests : public DelayControlTests,
                             public ::testing::WithParamInterface<const char *> {};
TEST_P(DelayControlHoldTests, HoldsTheFirstValueDuringStartup) {
    EXPECT_EQ(render(GetParam()), (std::vector<cs_float>{1,1,1,1,2,3,4,5}));
    expectSuccess();
}
INSTANTIATE_TEST_SUITE_P(InitialHold, DelayControlHoldTests, ::testing::Values(
    "kResult delayk kInput,.003,2",
    "kResult vdel_k kInput,.003,.003,2"));

TEST_F(DelayControlTests, DefaultStartupIsSilent) {
    EXPECT_EQ(render("kResult delayk kInput,.003"),
              (std::vector<cs_float>{0,0,0,1,2,3,4,5}));
    expectSuccess();
}
TEST_F(DelayControlTests, ZeroDelayIsImmediate) {
    EXPECT_EQ(render("kFixed delayk kInput,0,2\n"
                     "kVariable vdel_k kInput,0,0,2\n"
                     "kResult = kFixed+kVariable"),
              (std::vector<cs_float>{2,4,6,8,10,12,14,16}));
    expectSuccess();
}
TEST_F(DelayControlTests, FixedAndVariableDelaysRoundConsistently) {
    const auto values = render(
        "kFixed delayk kInput,.0025\n"
        "kVariable vdel_k kInput,.0025,.01\n"
        "kResult = kVariable+kFixed");
    EXPECT_EQ(values, (std::vector<cs_float>{0,0,0,2,4,6,8,10}));
    expectSuccess();
}
TEST_F(DelayControlTests, ChangingDelayUsesTheAvailableHistory) {
    EXPECT_EQ(render("kDelay = (kInput < 3 ? 0 : .003)\n"
                     "kResult vdel_k kInput,kDelay,.003,2"),
              (std::vector<cs_float>{1,2,1,1,2,3,4,5}));
    expectSuccess();
}
TEST_F(DelayControlTests, InPlaceOutputPreservesTheFirstInput) {
    EXPECT_EQ(render("kInput delayk kInput,.003,2\nkResult = kInput"),
              (std::vector<cs_float>{1,1,1,1,2,3,4,5}));
    expectSuccess();
}
TEST_F(DelayControlTests, ReusedNotesHaveTheirOwnInitialValue) {
    const auto values = render("kInput = kInput+p4\n"
        "kResult delayk kInput,.003,2", 18,
        "i1 0 .008 0\ni1 .01 .008 10\nf0 1");
    ASSERT_EQ(values.size(),18u);
    EXPECT_EQ(std::vector<cs_float>(values.begin()+10,values.end()),
              (std::vector<cs_float>{11,11,11,11,12,13,14,15}));
    expectSuccess();
}
TEST_F(DelayControlTests, SkipInitializationKeepsFixedDelayHistory) {
    EXPECT_EQ(render("iMode init 2\nif kInput == 3 then\nreinit AGAIN\nendif\n"
        "AGAIN:\n"
        "kResult delayk kInput,.003,iMode\niMode = 3\nrireturn"),
        (std::vector<cs_float>{1,1,1,1,2,3,4,5}));
    expectSuccess();
}
TEST_F(DelayControlTests, SkipInitializationKeepsVariableDelayHistory) {
    EXPECT_EQ(render("iMode init 2\nif kInput == 3 then\nreinit AGAIN\nendif\n"
        "AGAIN:\n"
        "kResult vdel_k kInput,.003,.003,iMode\niMode = 3\nrireturn"),
        (std::vector<cs_float>{1,1,1,1,2,3,4,5}));
    expectSuccess();
}

class DelayControlInitErrorTests : public DelayControlTests,
                                  public ::testing::WithParamInterface<const char *> {};
TEST_P(DelayControlInitErrorTests, RejectsInvalidBufferSizes) {
    render(GetParam(),2);
    EXPECT_GT(csound->inerrcnt,0);
}
INSTANTIATE_TEST_SUITE_P(InvalidSizes, DelayControlInitErrorTests, ::testing::Values(
    "kResult delayk kInput,-.0001",
    "kResult delayk kInput,1e30",
    "kResult vdel_k kInput,0,-.0001",
    "kResult vdel_k kInput,0,1e30"));

class DelayControlPerfErrorTests : public DelayControlTests,
                                  public ::testing::WithParamInterface<const char *> {};
TEST_P(DelayControlPerfErrorTests, RejectsDelayOutsideBuffer) {
    render(std::string("kResult vdel_k kInput,")+GetParam()+",.003",2);
    EXPECT_GT(csound->perferrcnt,0);
}
INSTANTIATE_TEST_SUITE_P(InvalidDelays, DelayControlPerfErrorTests,
                        ::testing::Values("-.0001",".004","1e30"));
} // namespace
