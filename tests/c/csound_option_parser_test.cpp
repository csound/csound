#define __BUILDING_LIBCSOUND

#include <stdio.h>
#include <string.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include "gtest/gtest.h"
#include "csound.h"
#include "csoundCore.h"

extern "C" {
    extern CORFIL *corfile_create_r(CSOUND *csound, const char *text);
    extern void corfile_rm(CSOUND *csound, CORFIL **ff);
}

/* Engine/memalloc.c's block header: a Debug build defines BETA, which turns
   MEMDEBUG on. */
struct TrackedBlock {
#if defined(BETA) || defined(MEMDEBUG)
    int32_t magic;
    void *ptr;
#endif
    TrackedBlock *prv, *nxt;
};

/* The blocks Csound holds until a reset. */
static size_t trackedBlocks (const CSOUND *csound)
{
    size_t n = 0;
    for (auto *b = static_cast<TrackedBlock *> (csound->memalloc_db); b;
         b = b->nxt)
        n++;
    return n;
}

class OptionParserTests : public ::testing::Test {
public:
    OptionParserTests ()
    {
    }

    virtual ~OptionParserTests ()
    {
    }

    virtual void SetUp ()
    {
        csoundSetGlobalEnv ("OPCODE6DIR64", "../../");
        csound = csoundCreate (NULL, NULL);
        csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

TEST_F (OptionParserTests, testParseSimpleEnvVar)
{
    const char option_str[] = "--env:FOO1=bar1 --env:FOO2=bar2\n";
    const char *value;
    CORFIL *cf = corfile_create_r(csound, option_str);
    read_options(csound, cf, 0);

    value = csoundGetEnv(csound, "FOO1");
    ASSERT_TRUE(value != NULL);
    ASSERT_TRUE(strcmp(value, "bar1") == 0);

    value = csoundGetEnv(csound, "FOO2");
    ASSERT_TRUE(value != NULL);
    ASSERT_TRUE(strcmp(value, "bar2") == 0);

    corfile_rm(csound, &cf);
}

TEST_F (OptionParserTests, testParseQuotedEnvVar)
{
    const char option_str[] = "--env:FOO1=\"bar baz\"\n";
    const char *value;
    CORFIL *cf = corfile_create_r(csound, option_str);
    read_options(csound, cf, 0);

    value = csoundGetEnv(csound, "FOO1");
    ASSERT_TRUE(value != NULL);
    ASSERT_TRUE(strcmp(value, "bar baz") == 0);

    corfile_rm(csound, &cf);
}

TEST_F (OptionParserTests, testEmptyOptionSetsNothing)
{
    /* Three calls, as argdecode() takes two names and dies of the third. */
    EXPECT_EQ (csoundSetOption (csound, ""), 0);
    EXPECT_EQ (csoundSetOption (csound, "   "), 0);
    EXPECT_EQ (csoundSetOption (csound, ""), 0);
    EXPECT_EQ (csound->orchname, nullptr);
    EXPECT_EQ (csound->scorename, nullptr);
}

TEST_F (OptionParserTests, testEmptyOptionLeavesRcFilesUnread)
{
    /* The fixture's own option has read them, so a new instance. */
    CSOUND *fresh = csoundCreate (NULL, NULL);
    csoundCreateMessageBuffer (fresh, 0);
    EXPECT_EQ (csoundSetOption (fresh, ""), 0);
    EXPECT_EQ (fresh->options_checked, 0);
    csoundDestroy (fresh);
}

TEST_F (OptionParserTests, testQuotedFirstArgumentKeepsItsText)
{
    EXPECT_EQ (csoundSetOption (csound, "\"my piece.csd\""), 0);
    EXPECT_STREQ (csound->orchname, "my piece.csd");
}

TEST_F (OptionParserTests, testQuotedFlagIsAFlag)
{
    EXPECT_EQ (csoundSetOption (csound, "\"-n\" -d"), 0);
    EXPECT_EQ (csound->oparms->sfwrite, 0);
    EXPECT_EQ (csound->oparms->displays, 0);
    EXPECT_EQ (csound->orchname, nullptr);
}

TEST_F (OptionParserTests, testQuotedArgumentAfterASpaceIsOneArgument)
{
    EXPECT_EQ (csoundSetOption (csound, "-o \"a b.wav\""), 0);
    EXPECT_STREQ (csound->oparms->outfilename, "a b.wav");
    EXPECT_EQ (csound->orchname, nullptr);
}

TEST_F (OptionParserTests, testOptionAfterAQuotedArgumentIsItsOwn)
{
    EXPECT_EQ (csoundSetOption (csound, "-o \"a.wav\" -d"), 0);
    EXPECT_STREQ (csound->oparms->outfilename, "a.wav");
    EXPECT_EQ (csound->oparms->displays, 0);
}

TEST_F (OptionParserTests, testUnquotedAndAttachedFormsSplitAsBefore)
{
    EXPECT_EQ (csoundSetOption (csound, "  -o\"a b.wav\"   -d  "), 0);
    EXPECT_STREQ (csound->oparms->outfilename, "a b.wav");
    EXPECT_EQ (csound->oparms->displays, 0);
    EXPECT_EQ (csoundSetOption (csound, "--env:FOO3=\"bar baz\" orc sco"), 0);
    EXPECT_STREQ (csoundGetEnv (csound, "FOO3"), "bar baz");
    EXPECT_STREQ (csound->orchname, "orc");
    EXPECT_STREQ (csound->scorename, "sco");
}

TEST_F (OptionParserTests, testNullOptionIsRefused)
{
    EXPECT_NE (csoundSetOption (csound, NULL), 0);
}

TEST_F (OptionParserTests, testClusterEndingInADashOrPlusStaysInItsArgument)
{
    /* Each is the last argument, so a read past its end leaves the copy
       argdecode() makes, which AddressSanitizer reports. */
    EXPECT_EQ (csoundSetOption (csound, "-d-"), 0);
    EXPECT_EQ (csound->oparms->displays, 0);
    EXPECT_EQ (csoundSetOption (csound, "-n+"), 0);
    EXPECT_EQ (csound->oparms->sfwrite, 0);
}

TEST_F (OptionParserTests, testOptionKeepsOnlyTheDecodedArguments)
{
    /* One block: argdecode()'s copy of the arguments, which the options it
       sets point into. */
    for (const char *option : {"-d", "-d -m0 -n"}) {
        const size_t before = trackedBlocks (csound);
        EXPECT_EQ (csoundSetOption (csound, option), 0);
        EXPECT_EQ (trackedBlocks (csound) - before, 1u) << option;
    }
}

TEST_F (OptionParserTests, testRefusedOptionKeepsOnlyTheDecodedArguments)
{
    /* argdecode() refuses -% and leaves by a long jump. */
    const size_t before = trackedBlocks (csound);
    EXPECT_NE (csoundSetOption (csound, "-d -m0 -%"), 0);
    EXPECT_EQ (trackedBlocks (csound) - before, 1u);
}

TEST_F (OptionParserTests, testJumpAfterTheDecodeFreesNothingTwice)
{
    /* -z lists the opcodes once the options are decoded, and a .wasm library
       that does not load stops that with csoundDie(), a long jump. */
    EXPECT_NE (csoundSetOption (csound, "--opcode-lib=missing.wasm -z"), 0);
    std::string messages;
    for (; csoundGetMessageCnt (csound) > 0; csoundPopFirstMessage (csound))
        messages += csoundGetFirstMessage (csound);
    EXPECT_NE (messages.find ("error loading 'missing.wasm'"),
               std::string::npos);
}

TEST_F (OptionParserTests, testIndirectionFileOptionsAreRead)
{
    namespace fs = std::filesystem;
    const fs::path file = fs::temp_directory_path () /
        ("csound-options-" + std::to_string (std::random_device {} ()));
    std::ofstream (file) << "--env:FROM_AT_FILE=1\n";
    const std::string option = "-@ \"" + file.string () + "\"";
    EXPECT_EQ (csoundSetOption (csound, option.c_str ()), 0);
    fs::remove (file);
    EXPECT_STREQ (csoundGetEnv (csound, "FROM_AT_FILE"), "1");
}

TEST_F (OptionParserTests, testIndirectionFileThatNamesItselfIsRefused)
{
    namespace fs = std::filesystem;
    const fs::path file = fs::temp_directory_path () /
        ("csound-options-" + std::to_string (std::random_device {} ()));
    const std::string option = "-@ \"" + file.string () + "\"";
    std::ofstream (file) << option << "\n";
    EXPECT_NE (csoundSetOption (csound, option.c_str ()), 0);
    std::string messages;
    for (; csoundGetMessageCnt (csound) > 0; csoundPopFirstMessage (csound))
        messages += csoundGetFirstMessage (csound);
    EXPECT_NE (messages.find ("is already being read"), std::string::npos);
    fs::remove (file);
}

TEST_F (OptionParserTests, testRefusedIndirectionFileIsClosed)
{
    namespace fs = std::filesystem;
    const fs::path file = fs::temp_directory_path () /
        ("csound-options-" + std::to_string (std::random_device {} ()));
    const std::string option = "-@ \"" + file.string () + "\"";
    std::ofstream (file) << "-%\n";
    EXPECT_NE (csoundSetOption (csound, option.c_str ()), 0);
    /* Corrected, the same file is read again. */
    std::ofstream (file) << "--env:FROM_AT_FILE_AGAIN=1\n";
    EXPECT_EQ (csoundSetOption (csound, option.c_str ()), 0);
    fs::remove (file);
    EXPECT_STREQ (csoundGetEnv (csound, "FROM_AT_FILE_AGAIN"), "1");
}

TEST_F (OptionParserTests, testMessageColoursTakeEitherSpelling)
{
    EXPECT_EQ (csoundSetOption (csound, "--m-colors=1"), 0);
    EXPECT_EQ (csound->oparms->msglevel & 96, 96);
    EXPECT_EQ (csoundSetOption (csound, "--m-colors=10"), 0);
    EXPECT_EQ (csound->oparms->msglevel & 96, 96);
    EXPECT_EQ (csoundSetOption (csound, "--m-colours=0"), 0);
    EXPECT_EQ (csound->oparms->msglevel & 96, 0);
}

TEST_F (OptionParserTests, testCodeAndEventsTakeTheTextAfterTheirEqualsSign)
{
    /* Without the =, the text was read from one past the option's end: the
       next argument here, and past the arguments for the last one. */
    EXPECT_EQ (csoundSetOption (csound, "--code -d"), 0);
    EXPECT_EQ (csound->orchname, nullptr);
    EXPECT_EQ (csound->oparms->displays, 0);
    EXPECT_EQ (csoundSetOption (csound, "--events"), 0);
    EXPECT_EQ (csoundSetOption (csound, "--code=;"), 0);
    EXPECT_STREQ (csound->orchname, "cmd-string");
}
