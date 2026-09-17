#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <array>
#include <string>

namespace {
class FtsetTests : public ::testing::TestWithParam<bool> {
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
            "giA ftgen 101,0,-8,-2,1,2,3,4,5,6,7,8\n"
            "giB ftgen 102,0,-8,-2,1,2,3,4,5,6,7,8\n"
            "instr 1\n" + body + "\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0) << messages();
        csoundEventString(csound, "i1 0 1", 0);
        ASSERT_EQ(csoundStart(csound), 0) << messages();
    }

    std::string call(const std::string &value, const std::string &slice) {
        if (GetParam())
            return "kValue init " + value + "\nftset 101,kValue" + slice;
        return "ftset 101," + value + slice;
    }

    void expectTable(const std::array<MYFLT,8> &expected, int number = 101) {
        MYFLT *table = nullptr;
        ASSERT_EQ(csoundGetTable(csound, &table, number), 8);
        for (int i = 0; i < 8; ++i) EXPECT_EQ(table[i], expected[i]) << i;
    }
};

TEST_P(FtsetTests, WritesSlicesAndLeavesGuardPointAlone) {
    struct Case { const char *slice; std::array<MYFLT,8> expected; };
    const Case cases[] = {
        {"", {9,9,9,9,9,9,9,9}},
        {",2,6,2", {1,2,9,4,9,6,7,8}},
        {",1,-1,2", {1,9,3,9,5,9,7,8}},
        {",2,100,2", {1,2,9,4,9,6,9,8}},
        {",2.9,6.9,2.9", {1,2,9,4,9,6,7,8}},
        {",3,8,2147483520", {1,2,3,9,5,6,7,8}}
    };
    for (const auto &test : cases) {
        SCOPED_TRACE(test.slice);
        ASSERT_NO_FATAL_FAILURE(start(call("9", test.slice)));
        MYFLT *table = nullptr;
        ASSERT_EQ(csoundGetTable(csound, &table, 101), 8);
        table[8] = 123;
        ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
        EXPECT_EQ(csound->inerrcnt + csound->perferrcnt, 0) << messages();
        expectTable(test.expected);
        EXPECT_EQ(table[8], 123);
    }
}

TEST_P(FtsetTests, ClearsValidRangesAndTreatsEmptyRangesConsistently) {
    for (const char *value : {"0", "9"}) {
        for (const char *slice : {",6,2", ",3,3", ",8,0", ",0,-8"}) {
            SCOPED_TRACE(std::string(value) + slice);
            ASSERT_NO_FATAL_FAILURE(start(call(value, slice)));
            ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
            EXPECT_EQ(csound->inerrcnt + csound->perferrcnt, 0) << messages();
            expectTable({1,2,3,4,5,6,7,8});
        }
    }
    ASSERT_NO_FATAL_FAILURE(start(call("0", ",2,-1")));
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    expectTable({1,2,0,0,0,0,0,8});
}

TEST_P(FtsetTests, RejectsInvalidSlicesBeforeWriting) {
    for (const char *slice : {",-1", ",9", ",0,-9", ",0,8,0",
                              ",0,8,-1", ",0,8,0.5", ",0,8,1e30",
                              ",1e30", ",0,1e30", ",0,8,sqrt(-1)"}) {
        SCOPED_TRACE(slice);
        ASSERT_NO_FATAL_FAILURE(start(call("0", slice)));
        csoundPerformKsmps(csound);
        EXPECT_GT(csound->inerrcnt + csound->perferrcnt, 0);
        EXPECT_NE(messages().find("ftset: invalid slice bounds or step"),
                  std::string::npos);
        expectTable({1,2,3,4,5,6,7,8});
    }
}

TEST_P(FtsetTests, RejectsUnrepresentableTableNumbers) {
    for (const char *number : {"1e30", "sqrt(-1)"}) {
        std::string body = GetParam() ? "kNumber init " : "iNumber = ";
        body += number;
        body += GetParam() ? "\nftset kNumber,9" : "\nftset iNumber,9";
        ASSERT_NO_FATAL_FAILURE(start(body));
        csoundPerformKsmps(csound);
        EXPECT_GT(csound->inerrcnt + csound->perferrcnt, 0);
        EXPECT_NE(messages().find("ftset: table number out of range"),
                  std::string::npos);
        expectTable({1,2,3,4,5,6,7,8});
    }
}

TEST_P(FtsetTests, LooksUpBuiltinTableOnFirstCall) {
    const std::string body = GetParam() ?
        "kValue init 9\nftset -1,kValue,0,1\n" : "ftset -1,9,0,1\n";
    ASSERT_NO_FATAL_FAILURE(start(body));
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    EXPECT_EQ(csound->inerrcnt + csound->perferrcnt, 0) << messages();
    ASSERT_NE(csound->sinetable, nullptr);
    EXPECT_EQ(csound->sinetable->ftable[0], 9);
}

TEST_P(FtsetTests, RefreshesTheTableCacheOnRepeatedInitAndRoundedNumberChanges) {
    // The two values straddle the lookup's rounding boundary on platforms
    // where MYFLT2LONG rounds; use that same public conversion for expectations.
    const int first = MYFLT2LONG(MYFLT(101.25));
    const int second = MYFLT2LONG(MYFLT(101.75));
    const std::string body = GetParam() ?
        "kCycle init 0\nkCycle += 1\nkNumber = (kCycle == 1 ? 101.25 : 101.75)\n"
        "ftset kNumber,kCycle,0,1\n" :
        "iNumber = 101.25\niCount = 0\nwrite:\nftset iNumber,9,0,1\n"
        "iNumber = 101.75\niCount += 1\nif iCount < 2 igoto write\n";
    ASSERT_NO_FATAL_FAILURE(start(body));
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
    if (GetParam()) {
        expectTable({MYFLT(first == second ? 2 : 1),2,3,4,5,6,7,8}, first);
        expectTable({2,2,3,4,5,6,7,8}, second);
    } else {
        expectTable({9,2,3,4,5,6,7,8}, first);
        expectTable({9,2,3,4,5,6,7,8}, second);
    }
}

INSTANTIATE_TEST_SUITE_P(InitAndControl, FtsetTests, ::testing::Bool());
} // namespace
