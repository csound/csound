#define __BUILDING_LIBCSOUND

#include <stdio.h>
#include <string.h>
#include "gtest/gtest.h"
#include "csound.h"
#include "csoundCore.h"

extern "C" {
    extern CORFIL *corfile_create_r(CSOUND *csound, const char *text);
    extern void corfile_rm(CSOUND *csound, CORFIL **ff);
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
