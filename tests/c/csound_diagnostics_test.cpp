#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "filesys.h"
#include "gtest/gtest.h"
#include <string>

extern "C" {
CORFIL *corfile_create_r(CSOUND *, const char *);
void corfile_rm(CSOUND *, CORFIL **);
}

class DiagnosticLineTests : public ::testing::Test {
protected:
  CSOUND *csound = nullptr;

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
  }

  void TearDown() override { csoundDestroy(csound); }

  void expectLine(const char *text)
  {
    bool found = false;
    while (csoundGetMessageCnt(csound)) {
      const std::string message = csoundGetFirstMessage(csound);
      if (message.find(text) != std::string::npos) {
        found = true;
        EXPECT_EQ(message.back(), '\n') << message;
      }
      csoundPopFirstMessage(csound);
    }
    EXPECT_TRUE(found) << text;
  }
};

TEST_F(DiagnosticLineTests, UnknownOutputFormat)
{
  EXPECT_NE(csoundSetOption(csound, "--format=invalid-test-format"), 0);
  expectLine("unknown output format: 'invalid-test-format'");
}

TEST_F(DiagnosticLineTests, InvalidOptionLines)
{
  for (int32_t inCsd : {0, 1}) {
    SCOPED_TRACE(inCsd);
    CORFIL *source = corfile_create_r(csound, "invalid-option\n</CsOptions>\n");
    read_options(csound, source, inCsd);
    corfile_rm(csound, &source);
    expectLine(inCsd ? "Invalid arguments in <CsOptions>:" :
                      "Invalid arguments in .csound7rc or -@ file:");
  }
}

TEST_F(DiagnosticLineTests, InvalidFileArguments)
{
  FILE *stream = nullptr;
  EXPECT_EQ(csoundFileOpen(csound, &stream, -1, "unused", (void *) "r",
                          nullptr, CSFTYPE_OTHER_TEXT, 0), nullptr);
  expectLine("csoundFileOpen(): invalid type:");
  EXPECT_EQ(csoundCreateFileHandle(csound, &stream, -1, "unused"), nullptr);
  expectLine("csoundCreateFileHandle(): invalid type:");
  EXPECT_EQ(csoundFileClose(csound, nullptr, 1U << 8), CSOUND_ERROR);
  expectLine("csoundFileClose: invalid close flags:");
}

TEST_F(DiagnosticLineTests, InvalidAudioModules)
{
  csoundSetRTAudioModule(csound, "null");
  const auto play = csound->playopen_callback;
  const auto record = csound->recopen_callback;
  csRtAudioParams params = {};
  params.sampleRate = 48000;
  params.nChannels = 1;
  for (const char *module : {"", "missing-module"}) {
    SCOPED_TRACE(module);
    csoundSetRTAudioModule(csound, module);
    for (auto open : {play, record}) {
      EXPECT_EQ(open(csound, &params), CSOUND_SUCCESS);
      expectLine(module[0] ? "unknown rtaudio module:" :
                             "rtaudio module set to empty string");
    }
  }
}

TEST_F(DiagnosticLineTests, InvalidMidiModules)
{
  csoundSetMIDIModule(csound, "null");
  const auto input = csound->midiGlobals->MidiInOpenCallback;
  const auto output = csound->midiGlobals->MidiOutOpenCallback;
  for (const char *module : {"", "missing-module"}) {
    SCOPED_TRACE(module);
    csoundSetMIDIModule(csound, module);
    for (auto open : {input, output}) {
      void *data = nullptr;
      EXPECT_NE(open(csound, &data, "0"), 0);
      expectLine("error: -+rtmidi");
    }
  }
}

// Both callback APIs must receive exactly the caller's line endings.
TEST_F(DiagnosticLineTests, MessageHelpersPreserveLineEndings)
{
  for (bool stringCallback : {false, true}) {
    SCOPED_TRACE(stringCallback);
    std::string output;
    if (stringCallback) {
      csoundSetHostData(csound, &output);
      csoundSetMessageStringCallback(csound,
        [](CSOUND *cs, int32_t, const char *message) {
          *static_cast<std::string *>(csoundGetHostData(cs)) += message;
        });
    }
    csoundSetDebug(csound, 99);
    for (const char *text : {"message", "message\n", "message\n\n", ""}) {
      SCOPED_TRACE(text);
      output.clear();
      csoundMessage(csound, "%s", text);
      csoundMessageS(csound, CSOUNDMSG_ERROR, "%s", text);
      csoundErrorMsg(csound, "%s", text);
      csoundErrorMsgS(csound, 0, "%s", text);
      csoundWarning(csound, "%s", text);
      csoundDebugMsg(csound, "%s", text);
      csoundSetDebug(csound, 0);
      synterr(csound, "%s", text); // Also exercises ErrMsgV.
      csoundSetDebug(csound, 99);
      if (!stringCallback) {
        while (csoundGetMessageCnt(csound)) {
          output += csoundGetFirstMessage(csound);
          csoundPopFirstMessage(csound);
        }
      }
      const std::string t = text;
      EXPECT_EQ(output, t + t + t + t + "warning: " + t + t +
                        "\nsyntax error, " + t);
    }
    if (stringCallback) {
      csoundSetMessageCallback(csound, nullptr);
      csoundSetHostData(csound, nullptr);
    }
  }
}

TEST_F(DiagnosticLineTests, PublicMessageCallsPreserveFragments)
{
  csoundMessage(csound, "left");
  csoundMessageS(csound, CSOUNDMSG_ERROR, " middle");
  csoundErrorMsg(csound, " right");
  csoundErrorMsgS(csound, 0, "\n");
  std::string output;
  while (csoundGetMessageCnt(csound)) {
    output += csoundGetFirstMessage(csound);
    csoundPopFirstMessage(csound);
  }
  EXPECT_EQ(output, "left middle right\n");
}

class MacroDiagnosticLineTests : public DiagnosticLineTests,
    public ::testing::WithParamInterface<const char *> {};

TEST_P(MacroDiagnosticLineTests, ErrorAfterMacroKeepsSourceLine)
{
  std::string source = "<CsoundSynthesizer>\n<CsInstruments>\n";
  source += GetParam();
  source += "instr 1\n";
  int line = 1;
  for (size_t i = 0; i < source.size(); ++i) {
    if (source[i] == '\n' ||
        (source[i] == '\r' && source[i + 1] != '\n'))
      ++line;
  }
  source += "iAmp = iErrorTest\nendin\n</CsInstruments>\n"
            "<CsScore>\ne\n</CsScore>\n</CsoundSynthesizer>\n";
  ASSERT_EQ(0, csoundSetOption(csound, "-n -d -m0 --csd-line-nums=1"));
  EXPECT_NE(0, csoundCompileCSD(csound, source.c_str(), 1, 0));
  const auto expected = "Variable 'iErrorTest' used before defined, line " +
                        std::to_string(line) + "\n";
  expectLine(expected.c_str());
}

INSTANTIATE_TEST_SUITE_P(MacroSources, MacroDiagnosticLineTests,
  ::testing::Values(
    "",
    "#define TEST # 1 #\n",
    "#define TEST\n# 1 #\n",
    "#define TEST(x)\n\n# $x #\n",
    "#define TEST ; comment\n# 1 #\n",
    "#define TEST(x) // comment\n# $x #\n",
    "#define TEST /* comment\ncontinued */\n# 1 #\n",
    "#define TEST(x) /* comment\ncontinued */\n# $x #\n",
    "#define TEST # iTmp = 1 #\n$TEST\n",
    "#define TEST(x) # iTmp = $x #\n$TEST(1)\n",
    "#define TEST #\niTmp = 1#\n$TEST\n",
    "#define TEST #\niTmp = 1\n#\n$TEST\n",
    "#define TEST(x) #\niTmp = $x\n#\n$TEST(1)\n",
    "#define TEST(x)\n#\niTmp = $x\n#\n$TEST(1)\n",
    "#define TEST\r\n#\r\niTmp = 1\r\n#\r\n$TEST\r\n",
    "#define TEST(x)\r\n#\r\niTmp = $x\r\n#\r\n$TEST(1)\r\n",
    "#define INNER #\niTmp = 1\n#\n"
      "#define OUTER #\n$INNER\n#\n$OUTER\n",
    "#define TEST(x) #iTmp = ($x)#\n$TEST(\n1\n)\n",
    "#define TEST(x) #iTmp = ($x)#\r\n$TEST(\r\n1\r\n)\r\n",
    "#define TEST(x) #iTmp = ($x)#\r$TEST(\r1\r)\r",
    "#define TEST(x) #iTmp = ($x + $x)#\n$TEST(\n1\n)\n",
    "#define VALUE #1#\n#define TEST(x) #iTmp = ($x)#\n"
      "$TEST(\n$VALUE\n)\n",
    "#define VALUE #1#\r\n#define TEST(x) #iTmp = ($x)#\r\n"
      "$TEST(\r\n$VALUE\r\n)\r\n",
    "#define TEST(x) #iTmp = ($x)#\n$TEST(\\\n1\\\n)\n",
    "#define TEST(x) #iTmp = ($x)#\r\n$TEST(\\\r\n1\\\r\n)\r\n"));
