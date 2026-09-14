#include "gtest/gtest.h"
#include <set>
#include <string>

#define __BUILDING_LIBCSOUND
#include "csoundCore.h"

namespace {

class FtgentmpTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    }

    void TearDown() override { csoundDestroy(csound); }

    std::string messages()
    {
        std::string text;
        while (csoundGetMessageCnt(csound)) {
            text += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return text;
    }

    void start(const std::string &body, const char *score,
               const std::string &extra = "")
    {
        const std::string orc =
            "sr=8192\nksmps=16\nnchnls=1\n0dbfs=1\ninstr 1\n" + body +
            "\nendin\n" + extra;
        ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), CSOUND_SUCCESS)
            << messages();
        csoundEventString(csound, score, 0);
        ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS) << messages();
    }

    void finish()
    {
        for (int i = 0; i < 64; ++i)
            if (csoundPerformKsmps(csound) != CSOUND_SUCCESS) break;
        const auto text = messages();
        EXPECT_EQ(text.find("Error deleting ftable"), std::string::npos) << text;
        EXPECT_EQ(text.find("deinit error"), std::string::npos) << text;
    }

    bool exists(int number)
    {
        MYFLT *data = nullptr;
        return csoundGetTable(csound, &data, number) > 0;
    }

    int lastTable()
    {
        int error = 0;
        const int number = static_cast<int>(
            csoundGetControlChannel(csound, "table", &error));
        EXPECT_EQ(error, CSOUND_SUCCESS);
        return number;
    }

    const std::string make =
        "ifno ftgentmp p4,0,512,10,1\nchnset ifno,\"table\"\n";
    CSOUND *csound = nullptr;
};

TEST_F(FtgentmpTests, ExplicitTablePersistsWithoutCleanupError)
{
    ASSERT_NO_FATAL_FAILURE(start(make, "i 1 0 .02 500"));
    finish();
    EXPECT_TRUE(exists(500));
}

TEST_F(FtgentmpTests, AutomaticTableIsDeletedAtNoteEnd)
{
    ASSERT_NO_FATAL_FAILURE(start(make, "i 1 0 .02 0"));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    const int number = lastTable();
    ASSERT_GT(number, 0);
    ASSERT_TRUE(exists(number));
    finish();
    EXPECT_FALSE(exists(number));
}

TEST_F(FtgentmpTests, ReusedNoteDoesNotDeleteAnotherNotesTable)
{
    ASSERT_NO_FATAL_FAILURE(start(make,
        "i 1 0 .01 0\ni 2 .015 .08\ni 1 .02 .01 500",
        "instr 2\nifno ftgentmp 0,0,512,10,1\nendin\n"));
    for (int i = 0; i < 24; ++i)
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_TRUE(exists(101));
    EXPECT_TRUE(exists(500));
    finish();
    EXPECT_FALSE(exists(101));
    EXPECT_TRUE(exists(500));
}

TEST_F(FtgentmpTests, ReinitRetainsEveryTemporaryTableUntilNoteEnd)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "Make:\n" + make +
        "rireturn\nkCycle init 0\nkCycle += 1\n"
        "if kCycle < 4 then\nreinit Make\nendif\n", "i 1 0 .04 0"));
    std::set<int> numbers;
    for (int i = 0; i < 6; ++i) {
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        numbers.insert(lastTable());
        for (int number : numbers) EXPECT_TRUE(exists(number));
    }
    ASSERT_GE(numbers.size(), 3u);
    finish();
    for (int number : numbers) EXPECT_FALSE(exists(number)) << number;
    // Include the table created by the first init pass, before reinit.
    EXPECT_FALSE(exists(101));
}

TEST_F(FtgentmpTests, SkippedInitializationDoesNotTryToDeleteTableZero)
{
    ASSERT_NO_FATAL_FAILURE(start("igoto Done\n" + make + "Done:\n",
                                  "i 1 0 .02 0"));
    finish();
}

TEST_F(FtgentmpTests, FailedInitializationDoesNotAddACleanupError)
{
    ASSERT_NO_FATAL_FAILURE(start("ifno ftgentmp 0,0,512,9999,1\n",
                                  "i 1 0 .02"));
    finish();
}

TEST_F(FtgentmpTests, FailedReinitStillDeletesEarlierTemporaryTables)
{
    ASSERT_NO_FATAL_FAILURE(start(
        "iGen init 10\nMake:\nifno ftgentmp 0,0,512,iGen,1\n"
        "iGen = 9999\nrireturn\nkOnce init 0\n"
        "if kOnce == 0 then\nkOnce = 1\nreinit Make\nendif\n",
        "i 1 0 .02"));
    finish();
    EXPECT_FALSE(exists(101));
}

TEST_F(FtgentmpTests, StringArgumentUsesTheSameTableLifetime)
{
    std::string path = __FILE__;
    for (char &c : path) if (c == '\\') c = '/';
    path = path.substr(0, path.find_last_of('/') + 1) +
           "../commandline/gen23.txt";
    ASSERT_NO_FATAL_FAILURE(start(
        "ifno ftgentmp p4,0,0,-23,\"" + path + "\"\n"
        "chnset ifno,\"table\"\n",
        "i 1 0 .01 0\ni 1 .02 .01 500"));
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    const int number = lastTable();
    ASSERT_TRUE(exists(number));
    finish();
    EXPECT_FALSE(exists(number));
    EXPECT_TRUE(exists(500));
}

} // namespace
