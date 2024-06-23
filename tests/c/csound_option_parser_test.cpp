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
        csound = csoundCreate (0);
        csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundCleanup (csound);
        csoundDestroyMessageBuffer (csound);
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
    readOptions(csound, cf, 0);

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
    readOptions(csound, cf, 0);

    value = csoundGetEnv(csound, "FOO1");
    ASSERT_TRUE(value != NULL);
    ASSERT_TRUE(strcmp(value, "bar baz") == 0);

    corfile_rm(csound, &cf);
}
