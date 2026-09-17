#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <initializer_list>
#include <string>

namespace {
// Init-only, control-rate with fixed tables, and control-rate with k tables.
class FtsliceTests : public ::testing::TestWithParam<int> {
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

    void start(const std::string &body) {
        if (csound) csoundDestroy(csound);
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        csoundSetOption(csound, "--daemon");
        const std::string orc =
            "sr=1024\nksmps=16\nnchnls=1\n"
            "giA ftgen 101,0,-9,-2,1,2,3,4,5,6,7,8,9\n"
            "giB ftgen 102,0,-9,-2,-1,-2,-3,-4,-5,-6,-7,-8,-9\n"
            "giC ftgen 103,0,-3,-2,-1,-2,-3\n"
            "giD ftgen 104,0,-129,-2,0\n"
            "instr 1\n" + body + "\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0) << messages();
        csoundEventString(csound, "i1 0 1", 0);
        ASSERT_EQ(csoundStart(csound), 0) << messages();
    }

    std::string call(const std::string &slice, const std::string &src = "101",
                     const std::string &dst = "102") {
        if (GetParam() == 0) return "ftslicei " + src + "," + dst + slice;
        if (GetParam() == 1) return "ftslice " + src + "," + dst + slice;
        return "kSrc init " + src + "\nkDst init " + dst +
               "\nftslice kSrc,kDst" + slice;
    }

    void expectTable(std::initializer_list<MYFLT> expected, int number = 102) {
        MYFLT *table = nullptr;
        ASSERT_EQ(csoundGetTable(csound, &table, number), expected.size());
        int i = 0;
        for (MYFLT value : expected) { EXPECT_EQ(table[i], value) << i; ++i; }
    }
};

TEST_P(FtsliceTests, CopiesSlicesAndPreservesTheTailAndGuard) {
    struct Case { const char *slice; std::initializer_list<MYFLT> expected; };
    const Case cases[] = {
        {"", {1,2,3,4,5,6,7,8,9}},
        {",1,8,3", {2,5,8,-4,-5,-6,-7,-8,-9}},
        {",1,7,3", {2,5,-3,-4,-5,-6,-7,-8,-9}},
        {",2,100,2", {3,5,7,9,-5,-6,-7,-8,-9}},
        {",2,-1,2", {3,5,7,9,-5,-6,-7,-8,-9}},
        {",2.9,8.9,2.9", {3,5,7,-4,-5,-6,-7,-8,-9}},
        {",3,9,2147483520", {4,-2,-3,-4,-5,-6,-7,-8,-9}},
        {",8,2", {-1,-2,-3,-4,-5,-6,-7,-8,-9}},
        {",3,3", {-1,-2,-3,-4,-5,-6,-7,-8,-9}},
        {",9,0", {-1,-2,-3,-4,-5,-6,-7,-8,-9}}
    };
    for (const auto &test : cases) {
        SCOPED_TRACE(test.slice);
        ASSERT_NO_FATAL_FAILURE(start(call(test.slice)));
        MYFLT *table = nullptr;
        ASSERT_EQ(csoundGetTable(csound, &table, 102), 9);
        table[9] = 123;
        ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
        EXPECT_EQ(csound->inerrcnt + csound->perferrcnt, 0) << messages();
        expectTable(test.expected);
        EXPECT_EQ(table[9], 123);
    }
}

TEST_P(FtsliceTests, LimitsCopiesToTheDestinationLength) {
    ASSERT_NO_FATAL_FAILURE(start(call("", "101", "103")));
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectTable({1,2,3}, 103);
}

TEST_P(FtsliceTests, SupportsCopyingWithinTheSameTable) {
    ASSERT_NO_FATAL_FAILURE(start(call(",2,9,2", "101", "101")));
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectTable({3,5,7,9,5,6,7,8,9}, 101);
}

TEST_P(FtsliceTests, HandlesALargeFinalStepWithoutIndexOverflow) {
    ASSERT_NO_FATAL_FAILURE(start(call(",128,129,2147483520", "104")));
    MYFLT *source = nullptr;
    ASSERT_EQ(csoundGetTable(csound, &source, 104), 129);
    source[128] = 42;
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectTable({42,-2,-3,-4,-5,-6,-7,-8,-9});
}

TEST_P(FtsliceTests, RejectsInvalidSlicesBeforeCopying) {
    for (const char *slice : {",-1", ",10", ",0,9,0", ",0,9,-1",
                              ",0,9,0.5", ",0,9,1e30", ",1e30",
                              ",0,1e30", ",0,9,sqrt(-1)"}) {
        SCOPED_TRACE(slice);
        ASSERT_NO_FATAL_FAILURE(start(call(slice)));
        csoundPerformKsmps(csound);
        EXPECT_GT(csound->inerrcnt + csound->perferrcnt, 0);
        EXPECT_NE(messages().find("ftslice: invalid slice bounds or step"),
                  std::string::npos);
        expectTable({-1,-2,-3,-4,-5,-6,-7,-8,-9});
    }
}

TEST_P(FtsliceTests, RejectsUnrepresentableTableNumbers) {
    for (const char *number : {"1e30", "sqrt(-1)"}) {
        for (bool source : {false, true}) {
            ASSERT_NO_FATAL_FAILURE(start(call("", source ? number : "101",
                                                 source ? "102" : number)));
            csoundPerformKsmps(csound);
            EXPECT_GT(csound->inerrcnt + csound->perferrcnt, 0);
            EXPECT_NE(messages().find("ftslice: table number out of range"),
                      std::string::npos);
            expectTable({-1,-2,-3,-4,-5,-6,-7,-8,-9});
        }
    }
}

TEST_P(FtsliceTests, PropagatesMissingTableErrors) {
    for (bool source : {false, true}) {
        ASSERT_NO_FATAL_FAILURE(start(call("", source ? "999" : "101",
                                             source ? "102" : "999")));
        int result = csoundPerformKsmps(csound);
        EXPECT_TRUE(result != 0 || csound->inerrcnt + csound->perferrcnt > 0);
        EXPECT_NE(messages().find("Invalid ftable no."), std::string::npos);
        expectTable({-1,-2,-3,-4,-5,-6,-7,-8,-9});
    }
}

INSTANTIATE_TEST_SUITE_P(AllRates, FtsliceTests, ::testing::Values(0,1,2));
} // namespace
