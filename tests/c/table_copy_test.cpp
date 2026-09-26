#include "csound.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

namespace {

class TableCopyTests : public ::testing::TestWithParam<std::tuple<int, int>> {
protected:
    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
        const std::string orc =
            "sr=48000\nksmps=32\nnchnls=1\n"
            "giTable ftgen 1, 0, -" + std::to_string(length()) + ", -2, 0\n";
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), CSOUND_SUCCESS);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    }

    void TearDown() override { csoundDestroy(csound); }
    int length() const { return std::get<0>(GetParam()); }
    int async() const { return std::get<1>(GetParam()); }
    void performCopy()
    {
        if (async()) EXPECT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    }

    CSOUND *csound = nullptr;
};

TEST_P(TableCopyTests, CopiesWholeTableWithDocumentedGuardPointBounds)
{
    cs_float *table = nullptr;
    ASSERT_EQ(csoundGetTable(csound, &table, 1), length());
    std::vector<cs_float> data(static_cast<size_t>(length()) + 2, -123);
    for (int i = 0; i < length(); ++i)
        table[i] = static_cast<cs_float>(i % 1024) / 1024;
    table[length()] = .75;

    csoundTableCopyOut(csound, 1, data.data() + 1, async());
    performCopy();
    EXPECT_TRUE(std::equal(table, table + length(), data.begin() + 1));
    EXPECT_EQ(data.front(), -123);
    EXPECT_EQ(data.back(), -123); // Copy-out excludes the guard point.

    std::fill(data.begin(), data.end(), .25);
    data[length()] = .5;
    data.back() = -123;
    csoundTableCopyIn(csound, 1, data.data(), async());
    performCopy();
    EXPECT_TRUE(std::equal(data.begin(), data.end() - 1, table));
    EXPECT_EQ(table[length()], .5); // Copy-in includes the guard point.
}

TEST_P(TableCopyTests, MissingTableLeavesDestinationUnchanged)
{
    cs_float data[] = { .25, .5 };
    csoundTableCopyOut(csound, 2, data, async());
    performCopy();
    EXPECT_EQ(data[0], .25);
    EXPECT_EQ(data[1], .5);
    csoundTableCopyIn(csound, 2, data, async());
    performCopy();
    EXPECT_EQ(csoundTableLength(csound, 2), -1);
}

INSTANTIATE_TEST_SUITE_P(SyncAndAsync, TableCopyTests,
    ::testing::Combine(::testing::Values(16, 16777216),
                       ::testing::Values(0, 1)));

} // namespace
