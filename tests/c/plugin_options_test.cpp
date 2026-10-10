#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>

#ifdef CSOUND_TEST_HAS_CONFIGURATION_PLUGIN
#include "configuration_plugin_path.hpp"

class PluginOptionTests : public ::testing::Test {
protected:
  CSOUND *csound = nullptr;

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    setPluginDirectory();
  }

  void TearDown() override
  {
    csoundDestroy(csound);
  }

  void setPluginDirectory()
  {
    const std::string option = std::string(sizeof(cs_float) == sizeof(float)
        ? "--env:OPCODE7DIR=" : "--env:OPCODE7DIR64=") +
        CSOUND_TEST_CONFIGURATION_PLUGIN_DIR;
    ASSERT_EQ(csoundSetOption(csound, option.c_str()), CSOUND_SUCCESS);
  }

  std::string messages()
  {
    std::string result;
    while (csoundGetMessageCnt(csound) > 0) {
      result += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    return result;
  }

  void expectClient(const char *value)
  {
    csCfgVariable_t *option =
        csoundQueryConfigurationVariable(csound, "test_plugin_client");
    ASSERT_NE(option, nullptr) << messages();
    EXPECT_STREQ(option->s.p, value);
    EXPECT_EQ(messages().find("invalid variable name"), std::string::npos);
  }

  void expectEnabled(int32_t value)
  {
    csCfgVariable_t *option =
        csoundQueryConfigurationVariable(csound, "test_plugin_enabled");
    ASSERT_NE(option, nullptr) << messages();
    EXPECT_EQ(*option->b.p, value);
  }
};

TEST_F(PluginOptionTests, StringOptionLoadsItsDefinition)
{
  ASSERT_EQ(csoundSetOption(csound, "-+test_plugin_client=Csound"), 0);
  expectClient("Csound");
  ASSERT_EQ(csoundSetOption(csound, "-+test_plugin_client=Changed"), 0);
  expectClient("Changed");
}

TEST_F(PluginOptionTests, BooleanOptionLoadsItsDefinition)
{
  ASSERT_EQ(csoundSetOption(csound, "-+test_plugin_enabled"), 0);
  expectEnabled(1);
}

TEST_F(PluginOptionTests, NegatedBooleanOptionLoadsItsDefinition)
{
  ASSERT_EQ(csoundSetOption(csound, "-+no-test_plugin_enabled"), 0);
  expectEnabled(0);
}

TEST_F(PluginOptionTests, ExplicitBooleanOptionLoadsItsDefinition)
{
  ASSERT_EQ(csoundSetOption(csound, "-+test_plugin_enabled=no"), 0);
  expectEnabled(0);
}

TEST_F(PluginOptionTests, HelpListsPluginOptions)
{
  const char *argv[] = {"csound", "--help"};
  csoundCompile(csound, 2, argv);
  const std::string output = messages();
  EXPECT_NE(output.find("-+test_plugin_client=<string>"), std::string::npos);
  EXPECT_NE(output.find("-+test_plugin_enabled=<boolean>"), std::string::npos);
}

TEST_F(PluginOptionTests, CoreOptionsAndVersionKeepPluginsUnloaded)
{
  ASSERT_EQ(csoundSetOption(csound,
      "-+rtaudio=null -+rtmidi=null -+msg_color -+no-msg_color"), 0);
  const char *argv[] = {"csound", "--version"};
  csoundCompile(csound, 2, argv);
  EXPECT_EQ(csoundQueryConfigurationVariable(csound, "test_plugin_client"),
            nullptr);
  EXPECT_EQ(csound->default_modules_loaded, 0);
}

TEST_F(PluginOptionTests, NoArgumentsKeepPluginsUnloaded)
{
  const char *argv[] = {"csound"};
  EXPECT_NE(csoundCompile(csound, 1, argv), CSOUND_SUCCESS);
  EXPECT_EQ(csoundQueryConfigurationVariable(csound, "test_plugin_client"),
            nullptr);
  EXPECT_EQ(csound->default_modules_loaded, 0);
}

TEST_F(PluginOptionTests, ResetReloadsPluginOptions)
{
  ASSERT_EQ(csoundSetOption(csound, "-+test_plugin_client=BeforeReset"), 0);
  expectClient("BeforeReset");
  csoundReset(csound);
  setPluginDirectory();
  ASSERT_EQ(csoundSetOption(csound, "-+test_plugin_client=AfterReset"), 0);
  expectClient("AfterReset");
}

TEST_F(PluginOptionTests, CsdOptionsLoadPluginDefinitions)
{
  const char *csd =
      "<CsoundSynthesizer>\n"
      "<CsOptions>\n"
      "-n -d -+test_plugin_client=FromCsd\n"
      "</CsOptions>\n"
      "<CsInstruments>\n"
      "sr = 48000\nksmps = 32\nnchnls = 1\n0dbfs = 1\n"
      "instr 1\nendin\n"
      "</CsInstruments>\n"
      "<CsScore>\nf0 0.01\n</CsScore>\n"
      "</CsoundSynthesizer>\n";
  ASSERT_EQ(csoundCompileCSD(csound, csd, 1, 0), CSOUND_SUCCESS);
  expectClient("FromCsd");
}

TEST_F(PluginOptionTests, UnknownOptionStillReportsAnError)
{
  csoundSetOption(csound, "-+test_plugin_missing=value");
  EXPECT_NE(messages().find("invalid variable name"), std::string::npos);
}
#endif
