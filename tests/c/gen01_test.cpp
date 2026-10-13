#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <tuple>
#include <vector>

namespace {

class DeferredGen01Tests : public ::testing::Test {
protected:
    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        reference = csoundCreate(nullptr, nullptr);
        for (CSOUND *cs : {csound, reference}) {
            csoundCreateMessageBuffer(cs, 0);
            ASSERT_EQ(csoundSetOption(cs, "-n -d -m0"), CSOUND_SUCCESS);
        }
        ASSERT_EQ(csoundSetOption(csound, "--defer-gen1"), CSOUND_SUCCESS);
        path = __FILE__;
        std::replace(path.begin(), path.end(), '\\', '/');
        path = path.substr(0, path.find_last_of('/') + 1) + "../soak/flute.aiff";
    }

    void TearDown() override
    {
        csoundDestroy(csound);
        csoundDestroy(reference);
    }

    void start(CSOUND *cs, const std::string &size, int gen, bool numeric)
    {
        const std::string quoted = "\"" + path + "\"";
        const std::string orc =
            "sr=44100\nksmps=32\nnchnls=1\n0dbfs=1\nstrset 201, " + quoted +
            "\ngiSample ftgen 1, 0, " + size + ", " + std::to_string(gen) +
            ", " + (numeric ? "201" : quoted) + ", .01, 0, 1\n"
            "instr 1\naSample poscil .1, 1, giSample\nout aSample\nendin\n";
        ASSERT_EQ(csoundCompileOrc(cs, orc.c_str(), 0), CSOUND_SUCCESS);
        ASSERT_EQ(csoundStart(cs), CSOUND_SUCCESS);
    }

    CSOUND *csound = nullptr;
    CSOUND *reference = nullptr;
    std::string path;
};

class DeferredGen01DataTests : public DeferredGen01Tests,
    public ::testing::WithParamInterface<std::tuple<const char *, int, bool>> {};

TEST_P(DeferredGen01DataTests, MatchesImmediateLoadAndRetainsArguments)
{
    const auto [size, gen, numeric] = GetParam();
    ASSERT_NO_FATAL_FAILURE(start(reference, size, gen, numeric));
    ASSERT_NO_FATAL_FAILURE(start(csound, size, gen, numeric));
    ASSERT_EQ(csound->flist[1]->flen, 0u);

    cs_float *args = nullptr;
    const int count = csoundGetTableArgs(csound, &args, 1);
    ASSERT_EQ(count, 5);
    const std::vector<cs_float> originalArgs(args, args + count);
    cs_float *expected = nullptr, *actual = nullptr;
    const int length = csoundGetTable(reference, &expected, 1);
    ASSERT_GT(length, 0);
    ASSERT_EQ(csoundGetTable(csound, &actual, 1), length);
    ASSERT_NE(actual, nullptr);
    EXPECT_TRUE(std::equal(expected, expected + length + 1, actual));
    EXPECT_EQ(csound->flist[1]->flenfrms, reference->flist[1]->flenfrms);
    EXPECT_EQ(csound->flist[1]->nchanls, reference->flist[1]->nchanls);
    EXPECT_EQ(csound->flist[1]->lobits, reference->flist[1]->lobits);
    ASSERT_EQ(csoundGetTableArgs(csound, &args, 1), count);
    EXPECT_EQ(std::memcmp(args, originalArgs.data(), count * sizeof(cs_float)), 0);
    cs_float *again = nullptr;
    EXPECT_EQ(csoundGetTable(csound, &again, 1), length);
    EXPECT_EQ(again, actual);
}

INSTANTIATE_TEST_SUITE_P(TableSizesAndFilenames, DeferredGen01DataTests,
    ::testing::Combine(::testing::Values("0", "1024", "1025", "-1000"),
                       ::testing::Values(1, -1), ::testing::Bool()));

TEST_F(DeferredGen01Tests, OpcodeFirstUseLoadsTheTable)
{
    ASSERT_NO_FATAL_FAILURE(start(csound, "0", 1, false));
    ASSERT_EQ(csound->flist[1]->flen, 0u);
    csoundEventString(csound, "i 1 0 .01", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_GT(csound->flist[1]->flen, 0u);
    EXPECT_EQ(csoundErrCnt(csound), 0);
}

TEST_F(DeferredGen01Tests, MissingFileReportsFailureOnFirstUse)
{
    path += ".missing";
    ASSERT_NO_FATAL_FAILURE(start(csound, "0", 1, false));
    cs_float *table = nullptr;
    EXPECT_EQ(csoundGetTable(csound, &table, 1), -1);
    EXPECT_EQ(table, nullptr);
    cs_float *args = nullptr;
    EXPECT_EQ(csoundGetTableArgs(csound, &args, 1), 5);
}

} // namespace
