#include "gtest/gtest.h"

extern "C" int csoundTestPlainCompareExchange(long, long, long, long *);

TEST(AtomicFallbackTests, MatchingValueAdvancesAndReportsSuccess)
{
    long after = -1;
    EXPECT_EQ(csoundTestPlainCompareExchange(7, 7, 8, &after), 0);
    EXPECT_EQ(after, 8);
}

TEST(AtomicFallbackTests, StaleExpectedValueDoesNotChangeStorage)
{
    long after = -1;
    EXPECT_NE(csoundTestPlainCompareExchange(8, 7, 9, &after), 0);
    EXPECT_EQ(after, 8);
}
