/*
  rtmidi_jack_module_test.cpp:

  Regression test for using the JACK MIDI module without JACK audio, e.g.
  "-+rtaudio=null -+rtmidi=jack" (GitHub issue #3418).

  InOut/rtjack.c used to return early from csoundModuleInit() unless JACK
  audio was selected, so the JACK MIDI callbacks were never installed. As a
  result "-+rtmidi=jack" failed to open MIDI with:

      error: -+rtmidi='jack': unknown module
      *** error opening MIDI in device: -1 (Unknown MIDI error)

  Only module selection is exercised here, so a running JACK server is not
  required: Csound is started with dummy audio and the JACK MIDI module, and
  the test checks that the JACK MIDI callbacks replaced the defaults.

  The test is only compiled when the rtjack plugin target exists.
*/

#include "gtest/gtest.h"

#if defined(CSOUND_TEST_JACK_PLUGIN_DIR)

#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csound.h"

#include <string>

/* Default MIDI input callback installed by the core (Top/csound_rtio.c).
   The JACK module must replace it when "-+rtmidi=jack" is selected. */
extern "C" int32_t DummyMidiInOpen(CSOUND *csound, void **userData,
                                   const char *devName);

namespace {

class RtMidiJackModuleTests : public ::testing::Test {
 protected:
  CSOUND *csound {nullptr};
  std::string priorOpcodeDir;
  std::string priorOpcodeDir64;
  bool hadOpcodeDir {false};
  bool hadOpcodeDir64 {false};

  void SetUp() override {
    /* Point the plugin search path at a directory holding only the freshly
       built JACK plugin before Csound loads its default modules. */
    const char *value = csoundGetEnv(nullptr, "OPCODE7DIR");
    hadOpcodeDir = value != nullptr;
    if (hadOpcodeDir)
      priorOpcodeDir = value;
    value = csoundGetEnv(nullptr, "OPCODE7DIR64");
    hadOpcodeDir64 = value != nullptr;
    if (hadOpcodeDir64)
      priorOpcodeDir64 = value;

    ASSERT_EQ(csoundSetGlobalEnv("OPCODE7DIR", CSOUND_TEST_JACK_PLUGIN_DIR),
              CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetGlobalEnv("OPCODE7DIR64", CSOUND_TEST_JACK_PLUGIN_DIR),
              CSOUND_SUCCESS);

    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
  }

  void TearDown() override {
    if (csound != nullptr)
      csoundDestroy(csound);
    csoundSetGlobalEnv("OPCODE7DIR",
                       hadOpcodeDir ? priorOpcodeDir.c_str() : nullptr);
    csoundSetGlobalEnv("OPCODE7DIR64",
                       hadOpcodeDir64 ? priorOpcodeDir64.c_str() : nullptr);
  }

  bool sawMessage(const char *text) {
    bool found = false;
    while (csoundGetMessageCnt(csound)) {
      const std::string message = csoundGetFirstMessage(csound);
      if (message.find(text) != std::string::npos)
        found = true;
      csoundPopFirstMessage(csound);
    }
    return found;
  }
};

/* Selecting the JACK MIDI module while audio is dummy must install the JACK
   MIDI callbacks. Before the fix the callbacks were left at their defaults. */
TEST_F(RtMidiJackModuleTests, MidiModuleSelectedWithoutJackAudio) {
  ASSERT_EQ(csoundSetOption(csound, "-+rtaudio=null"), 0);
  ASSERT_EQ(csoundSetOption(csound, "-+rtmidi=jack"), 0);
  ASSERT_EQ(csoundSetOption(csound, "-n"), 0);
  ASSERT_EQ(csoundSetOption(csound, "-d"), 0);
  ASSERT_EQ(csoundCompileOrc(csound, "sr = 48000\nksmps = 16\nnchnls = 1\n", 0),
            0);

  ASSERT_EQ(csoundStart(csound), 0);

  EXPECT_NE((void *)csound->midiGlobals->MidiInOpenCallback,
            (void *)DummyMidiInOpen)
      << "the JACK MIDI callbacks were not installed";
  EXPECT_FALSE(sawMessage("unknown module"));
}

/* The fix must not break the normal case where JACK provides both audio and
   MIDI. Audio selection no longer short-circuits before the MIDI setup. */
TEST_F(RtMidiJackModuleTests, MidiModuleSelectedWithJackAudio) {
  ASSERT_EQ(csoundSetOption(csound, "-+rtaudio=jack"), 0);
  ASSERT_EQ(csoundSetOption(csound, "-+rtmidi=jack"), 0);
  ASSERT_EQ(csoundSetOption(csound, "-n"), 0);
  ASSERT_EQ(csoundSetOption(csound, "-d"), 0);
  ASSERT_EQ(csoundCompileOrc(csound, "sr = 48000\nksmps = 16\nnchnls = 1\n", 0),
            0);

  ASSERT_EQ(csoundStart(csound), 0);

  EXPECT_NE((void *)csound->midiGlobals->MidiInOpenCallback,
            (void *)DummyMidiInOpen)
      << "the JACK MIDI callbacks were not installed";
}

}  // namespace

#endif  // CSOUND_TEST_JACK_PLUGIN_DIR
