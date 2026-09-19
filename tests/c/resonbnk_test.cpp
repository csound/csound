#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cmath>
#include <string>
#include <vector>

namespace {
class ResonBankTests : public ::testing::Test {
protected:
    CSOUND *csound;
    struct Sample { MYFLT actual, expected; };
    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        csoundSetOption(csound, "--sample-accurate");
    }
    void TearDown() override { csoundDestroy(csound); }
    std::vector<Sample> render(const std::string &body,
                              const char *score = "i1 0 .512\nf0 1",
                              int blocks = 64) {
        const std::string orc =
            "sr=1000\nksmps=8\nnchnls=2\n0dbfs=1\n"
            "instr 1\nkInput init .1\naInput upsamp kInput\n" + body +
            "\nouts aResult,aReference\nendin";
        std::vector<Sample> result;
        EXPECT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0);
        csoundEventString(csound, score, 0);
        EXPECT_EQ(csoundStart(csound), 0);
        for (int i = 0; i < blocks; ++i) {
            if (csoundPerformKsmps(csound) != 0) break;
            const MYFLT *out = csoundGetSpout(csound);
            for (int n = 0; n < 8; ++n)
                result.push_back({out[2*n], out[2*n+1]});
        }
        return result;
    }
    void expectMatch(const std::vector<Sample> &samples) {
        ASSERT_FALSE(samples.empty());
        EXPECT_EQ(csound->inerrcnt, 0);
        EXPECT_EQ(csound->perferrcnt, 0);
        for (size_t i = 0; i < samples.size(); ++i) {
            ASSERT_TRUE(std::isfinite(samples[i].actual)) << i;
            EXPECT_NEAR(samples[i].actual, samples[i].expected, 1e-5) << i;
        }
    }
};

class ResonBankScaleTests : public ResonBankTests,
                           public ::testing::WithParamInterface<int> {};
TEST_P(ResonBankScaleTests, MatchesResonFromTheFirstSample) {
    const std::string scale = std::to_string(GetParam());
    expectMatch(render("kParams[] fillarray 100,20\n"
        "aResult resonbnk aInput,kParams,0,500,16,1," + scale +
        "\naReference reson aInput,100,20," + scale));
}
INSTANTIATE_TEST_SUITE_P(Scaling, ResonBankScaleTests, ::testing::Values(0,1,2));

TEST_F(ResonBankTests, ExcludesParallelFilters) {
    expectMatch(render("kParams[] fillarray 100,20,300,30\n"
        "aResult resonbnk aInput,kParams,200,400,16,1\n"
        "aReference reson aInput,300,30"));
}
TEST_F(ResonBankTests, ExcludesSerialFilters) {
    expectMatch(render("kParams[] fillarray 100,20,300,30\n"
        "aResult resonbnk aInput,kParams,200,400,16,0\n"
        "aReference reson aInput,300,30"));
}
TEST_F(ResonBankTests, DisablingAndEnablingFiltersResetsTheirHistory) {
    const auto samples = render("kParams[] fillarray 100,20\n"
        "kCycle timeinstk\nkMin = (kCycle >= 3 && kCycle < 5 ? 200 : 0)\n"
        "aResult resonbnk aInput,kParams,kMin,500,8,1,1\n"
        "aReference init 0", "i1 0 .512\nf0 1", 6);
    ASSERT_EQ(samples.size(), 48u);
    for (int n = 16; n < 32; ++n) EXPECT_EQ(samples[n].actual, 0);
    for (int n = 0; n < 16; ++n)
        EXPECT_NEAR(samples[n].actual, samples[n+32].actual, 1e-6);
}
TEST_F(ResonBankTests, ParallelOutputCanReuseItsInput) {
    expectMatch(render("kParams[] fillarray 100,20,200,30\n"
        "aFirst reson aInput,100,20\naSecond reson aInput,200,30\n"
        "aReference = aFirst+aSecond\n"
        "aInput resonbnk aInput,kParams,0,500,16,1\naResult = aInput"));
}
TEST_F(ResonBankTests, ProcessesOnlyActiveSamples) {
    expectMatch(render("kParams[] fillarray 100,20\n"
        "aResult resonbnk aInput,kParams,0,500,16,1,1\n"
        "aReference reson aInput,100,20,1", "i1 .003 .020\nf0 1", 4));
}
TEST_F(ResonBankTests, AOneSamplePeriodUsesTheCurrentParameters) {
    expectMatch(render("kParams[] fillarray 100,20\n"
        "kCycle timeinstk\nkFreq = (kCycle < 3 ? 100 : 200)\nkParams[0] = kFreq\n"
        "aResult resonbnk aInput,kParams,0,500,1,1,1\n"
        "aReference reson aInput,kFreq,20,1"));
}
TEST_F(ResonBankTests, InterpolationReachesTheTargetAtTheEndOfThePeriod) {
    const auto samples = render("kParams[] fillarray 100,20\n"
        "kCycle timeinstk\nkParams[0] = (kCycle == 1 ? 100 : 200)\n"
        "aResult resonbnk aInput,kParams,0,500,8,1\naReference init 0",
        "i1 0 .512\nf0 1", 4);
    ASSERT_EQ(samples.size(), 32u);
    const double pi = std::acos(-1.0);
    const double c3 = std::exp(-2*pi*20/1000);
    const double oldC2 = 4*c3*std::cos(2*pi*100/1000)/(1+c3);
    const double newC2 = 4*c3*std::cos(2*pi*200/1000)/(1+c3);
    double y1=0, y2=0;
    for (int n=0; n<32; ++n) {
        const double fraction = n<8 ? 0 : (n<16 ? (n-7)/8.0 : 1);
        const double c2 = oldC2 + (newC2-oldC2)*fraction;
        const double y = .1+c2*y1-c3*y2;
        EXPECT_NEAR(samples[n].actual,y,1e-5) << n;
        y2=y1; y1=y;
    }
}
TEST_F(ResonBankTests, SkipInitializationPreservesStateAndInterpolation) {
    expectMatch(render("kParams[] fillarray 100,20\n"
        "kCycle timeinstk\nkParams[0] = (kCycle < 3 ? 100 : 200)\n"
        "aReference resonbnk aInput,kParams,0,500,13,1,1\n"
        "if kCycle == 4 then\nreinit AGAIN\nendif\n"
        "AGAIN:\naResult resonbnk aInput,kParams,0,500,13,1,1,1\nrireturn"));
}
TEST_F(ResonBankTests, VanishingPoleRadiusHasFiniteUnityGain) {
    expectMatch(render("kParams[] fillarray 100,1e6\n"
        "aResult resonbnk aInput,kParams,0,500,16,1,1\naReference = aInput"));
}

TEST_F(ResonBankTests, ReusedNotesResetTheirFilters) {
    expectMatch(render("kParams[] fillarray 100,20\n"
        "aResult resonbnk aInput,kParams,0,500,13,1,1\n"
        "aReference reson aInput,100,20,1",
        "i1 0 .128\ni1 .2 .128\nf0 1", 42));
}
TEST_F(ResonBankTests, ChangedBankSizeAllocatesStateEvenWithSkipEnabled) {
    expectMatch(render("kParams[] init p4\n"
        "kParams[0]=100\nkParams[1]=20\n"
        "if p4 == 4 then\nkParams[2]=100\nkParams[3]=20\nendif\n"
        "aResult resonbnk aInput,kParams,0,500,16,1,1,1\n"
        "aRes reson aInput,100,20,1\naReference = aRes*(p4/2)",
        "i1 0 .128 2\ni1 .2 .128 4\nf0 1", 42));
}
TEST_F(ResonBankTests, ReportsArraySizeChangesDuringPerformance) {
    render("kParams[] fillarray 100,20,200,30\n"
        "kCycle timeinstk\nif kCycle == 3 then\ntrim kParams,2\nendif\n"
        "aResult resonbnk aInput,kParams,0,500,16\naReference init 0",
        "i1 0 .512\nf0 1", 4);
    EXPECT_GT(csound->perferrcnt,0);
}

class ResonBankInvalidTests : public ResonBankTests,
                             public ::testing::WithParamInterface<const char *> {};
TEST_P(ResonBankInvalidTests, RejectsInvalidParameters) {
    render(std::string(GetParam()) + "\naReference init 0", "i1 0 .512\nf0 1", 2);
    EXPECT_GT(csound->inerrcnt,0);
}
INSTANTIATE_TEST_SUITE_P(InvalidParameters, ResonBankInvalidTests, ::testing::Values(
    "kParams[] fillarray 100,20,200\naResult resonbnk aInput,kParams,0,500,16",
    "kParams[][] init 2,2\naResult resonbnk aInput,kParams,0,500,16",
    "kParams[] fillarray 100,20\naResult resonbnk aInput,kParams,0,500,0",
    "kParams[] fillarray 100,20\naResult resonbnk aInput,kParams,0,500,-1",
    "kParams[] fillarray 100,20\naResult resonbnk aInput,kParams,0,500,1.5",
    "kParams[] fillarray 100,20\naResult resonbnk aInput,kParams,0,500,1e30",
    "kParams[] fillarray 100,20\naResult resonbnk aInput,kParams,0,500,16,1,1e30"));
} // namespace
