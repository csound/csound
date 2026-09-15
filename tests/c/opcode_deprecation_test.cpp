#include "gtest/gtest.h"
#include <cstring>
#include <string>

#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "opcode_deprecation.h"

namespace {

TEST(OpcodeDeprecationMetadata, ExactNamesAndOverloadSuffixes)
{
    const auto *info = csoundOpcodeDeprecation("hrtfer.a");
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(info->replacement, "hrtfstat");
    EXPECT_EQ(info->policy, CS_DEPRECATION_FROZEN);
    EXPECT_EQ(csoundOpcodeDeprecation("hrtf"), nullptr);
    EXPECT_EQ(csoundOpcodeDeprecation("hrtfer2"), nullptr);
    EXPECT_EQ(csoundOpcodeDeprecation(""), nullptr);
    EXPECT_EQ(csoundOpcodeDeprecation(nullptr), nullptr);
    EXPECT_EQ(csoundOpcodeDeprecation("Hrtfer"), nullptr);
    ASSERT_NE(csoundOpcodeDeprecation("MixerSetLevel_i"), nullptr);
    EXPECT_STREQ(csoundOpcodeDeprecation("MixerSetLevel_i")->replacement,
                 "mixersetleveli");
}

TEST(OpcodeDeprecationMetadata, CatalogIsSortedAndSearchable)
{
    const char *previous = "";
#define CSOUND_OPCODE_DEPRECATION(name, replacement, policy, note) \
    EXPECT_LT(std::strcmp(previous, name), 0); \
    ASSERT_NE(csoundOpcodeDeprecation(name), nullptr); \
    previous = name;
#include "opcode_deprecations.def"
#undef CSOUND_OPCODE_DEPRECATION
}

TEST(OpcodeDeprecationMetadata, NoInventedReplacementAndPluginFallback)
{
    char message[512];
    csoundOpcodeDeprecationMessage("spectrum.a", 0, message, sizeof(message));
    EXPECT_NE(std::string(message).find("no direct replacement"), std::string::npos);
    csoundOpcodeDeprecationMessage("thirdparty", 0, message, sizeof(message));
    EXPECT_STREQ(message, "opcode thirdparty is deprecated");
    csoundOpcodeDeprecationMessage("Old_Name", 1, message, sizeof(message));
    EXPECT_NE(std::string(message).find("has been renamed"), std::string::npos);
}

class OpcodeDeprecationCompiler : public ::testing::Test {
protected:
    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
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
    int compile(const char *body)
    {
        std::string code = "instr 1\n" + std::string(body) + "\nendin\n";
        return csoundCompileOrc(csound, code.c_str(), 0);
    }
    CSOUND *csound = nullptr;
    const char *legacy = "kValues[] fillarray 1,2\nkTotal sumtab kValues";
};

TEST_F(OpcodeDeprecationCompiler, LegacyFlagGivesOneReplacementWarning)
{
    ASSERT_EQ(compile(legacy), CSOUND_SUCCESS);
    const auto text = messages();
    const std::string warning = "opcode sumtab is deprecated; use sumarray instead";
    const auto first = text.find(warning);
    ASSERT_NE(first, std::string::npos) << text;
    EXPECT_EQ(text.find("sumtab", first + warning.size()), std::string::npos) << text;
}

TEST_F(OpcodeDeprecationCompiler, ErrorOptionIncludesLegacyFlags)
{
    ASSERT_EQ(csoundSetOption(csound, "--error-deprecated"), CSOUND_SUCCESS);
    EXPECT_NE(compile(legacy), CSOUND_SUCCESS);
    EXPECT_NE(messages().find("use sumarray instead"), std::string::npos);
}

TEST_F(OpcodeDeprecationCompiler, QuietWarningsDoNotOverrideExplicitErrors)
{
    ASSERT_EQ(csoundSetOption(csound, "-m1024"), CSOUND_SUCCESS);
    ASSERT_EQ(compile(legacy), CSOUND_SUCCESS);
    EXPECT_EQ(messages().find("opcode sumtab is deprecated"), std::string::npos);
    ASSERT_EQ(csoundSetOption(csound, "--error-deprecated"), CSOUND_SUCCESS);
    EXPECT_NE(compile(legacy), CSOUND_SUCCESS);
    EXPECT_NE(messages().find("use sumarray instead"), std::string::npos);
}

TEST_F(OpcodeDeprecationCompiler, ExistingStatusFlagGivesReplacement)
{
    ASSERT_EQ(compile("aSig init 0\nouts aSig,aSig"), CSOUND_SUCCESS);
    EXPECT_NE(messages().find("opcode outs is deprecated; use out instead"),
              std::string::npos);
}

TEST_F(OpcodeDeprecationCompiler, RenamedAliasGivesExactReplacement)
{
    ASSERT_EQ(compile("kValues[] genarray_i 0,1"), CSOUND_SUCCESS);
    EXPECT_NE(messages().find("opcode genarray_i has been renamed; use genarrayi instead"),
              std::string::npos);
}

TEST_F(OpcodeDeprecationCompiler, SupportedOpcodeHasNoDeprecationWarning)
{
    ASSERT_EQ(compile("kValues[] fillarray 1,2\nkTotal sumarray kValues"), CSOUND_SUCCESS);
    EXPECT_EQ(messages().find("deprecated"), std::string::npos);
}

TEST_F(OpcodeDeprecationCompiler, FinArrayGetsReplacementAndStrictError)
{
    const char *body = "aChannels[] init 2\nfin \"unused.wav\",0,0,aChannels";
    ASSERT_EQ(compile(body), CSOUND_SUCCESS) << messages();
    EXPECT_NE(messages().find("opcode fin is deprecated; use diskin2 instead"),
              std::string::npos);
    ASSERT_EQ(csoundSetOption(csound, "--error-deprecated"), CSOUND_SUCCESS);
    EXPECT_NE(compile(body), CSOUND_SUCCESS);
    EXPECT_NE(messages().find("use diskin2 instead"), std::string::npos);
}

TEST_F(OpcodeDeprecationCompiler, AudioArrayConversionRemainsSupported)
{
    ASSERT_EQ(csoundSetOption(csound, "--error-deprecated"), CSOUND_SUCCESS);
    ASSERT_EQ(compile("aSig init 0\nkSamples[] = array(aSig)"), CSOUND_SUCCESS)
        << messages();
    EXPECT_EQ(messages().find("deprecated"), std::string::npos);
}

static int32_t customInit(CSOUND *, void *) { return CSOUND_SUCCESS; }

TEST_F(OpcodeDeprecationCompiler, CatalogDoesNotDeprecateUnmarkedOverload)
{
    ASSERT_EQ(compile("aSig init 0"), CSOUND_SUCCESS);
    messages();
    ASSERT_EQ(csound->AppendOpcode(csound, "sumtab.custom", sizeof(OPDS), 0,
                                   "i", "", customInit, nullptr, nullptr),
              CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "--error-deprecated"), CSOUND_SUCCESS);
    ASSERT_EQ(compile("iValue = sumtab()"), CSOUND_SUCCESS) << messages();
    EXPECT_EQ(messages().find("deprecated"), std::string::npos);
}

TEST_F(OpcodeDeprecationCompiler, FunctionalCallGetsReplacement)
{
    ASSERT_EQ(compile("kValues[] fillarray 1,2\nkTotal = sumtab(kValues)"),
              CSOUND_SUCCESS);
    EXPECT_NE(messages().find("use sumarray instead"), std::string::npos);
}

TEST_F(OpcodeDeprecationCompiler, OpcodeListReportsLegacyFlags)
{
    ASSERT_EQ(compile("aSig init 0"), CSOUND_SUCCESS);
    opcodeListEntry *list = nullptr;
    const int count = csoundNewOpcodeList(csound, &list);
    ASSERT_GT(count, 0);
    bool found = false;
    for (int i = 0; i < count; ++i) {
        if (!std::strcmp(list[i].opname, "sumtab")) {
            found = true;
            EXPECT_EQ(list[i].deprecated, 1);
        }
    }
    csoundDisposeOpcodeList(csound, list);
    EXPECT_TRUE(found);
}

} // namespace
