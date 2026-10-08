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

TEST_F(DiagnosticLineTests, CompleteLinesKeepTheirNewlines)
{
  for (const char *text : {"message", "message\n", "message\n\n", ""}) {
    SCOPED_TRACE(text);
    csoundMessageLine(csound, CSOUNDMSG_ERROR, "%s", text);
    std::string expected = text;
    if (expected.empty() || expected.back() != '\n') expected += '\n';
    ASSERT_EQ(csoundGetMessageCnt(csound), 1);
    EXPECT_EQ(std::string(csoundGetFirstMessage(csound)), expected);
    EXPECT_EQ(csoundGetFirstMessageAttr(csound), CSOUNDMSG_ERROR);
    csoundPopFirstMessage(csound);
  }
}

TEST_F(DiagnosticLineTests, TranslatedTextIsTerminatedAfterFormatting)
{
  // The caller supplies the gettext result, without changing its lookup key.
  csoundMessageLine(csound, CSOUNDMSG_ERROR,
                    "Le chargement différé de '%s' a échoué", "piano.wav");
  ASSERT_EQ(csoundGetMessageCnt(csound), 1);
  EXPECT_STREQ(csoundGetFirstMessage(csound),
               "Le chargement différé de 'piano.wav' a échoué\n");
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

TEST_F(DiagnosticLineTests, WarningAndSyntaxErrorsDoNotAddBlankLines)
{
  for (const char *text : {"diagnostic", "diagnostic\n"}) {
    csoundWarning(csound, "%s", text);
    synterr(csound, "%s", text);
    std::string output;
    while (csoundGetMessageCnt(csound)) {
      output += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    EXPECT_EQ(output, "warning: diagnostic\n\nsyntax error, diagnostic\n");
  }
}

TEST_F(DiagnosticLineTests, LongLinesReachBothCallbackTypes)
{
  const std::string text(4096, 'x');
  csoundMessageLine(csound, CSOUNDMSG_ERROR, "%s", text.c_str());
  ASSERT_EQ(csoundGetMessageCnt(csound), 1);
  EXPECT_EQ(std::string(csoundGetFirstMessage(csound)), text + "\n");
  csoundPopFirstMessage(csound);

  std::string output;
  csoundSetHostData(csound, &output);
  csoundSetMessageStringCallback(csound,
    [](CSOUND *cs, int32_t, const char *message) {
      *static_cast<std::string *>(csoundGetHostData(cs)) += message;
    });
  csoundMessageLine(csound, CSOUNDMSG_ERROR, "%s", text.c_str());
  EXPECT_EQ(output, text + "\n");
  csoundSetMessageCallback(csound, nullptr);
  csoundSetHostData(csound, nullptr);
}

TEST_F(DiagnosticLineTests, SilentModeSuppressesCompleteDiagnostics)
{
  csoundSetMessageLevel(csound, CS_NOMSG);
  csoundMessageLine(csound, CSOUNDMSG_ERROR, "hidden");
  EXPECT_EQ(csoundGetMessageCnt(csound), 0);
}
