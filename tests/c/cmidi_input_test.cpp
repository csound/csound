/*
  cmidi_input_test.cpp:

  Unit test for the CoreMIDI input plugin (InOut/cmidi.c) multiport mapping.

  It creates its own virtual CoreMIDI sources, so no physical MIDI hardware
  or external device is required, then drives Csound through the "coremidi"
  MIDI input module with "-M m" and checks that each source is mapped to its
  own bank of 16 channels (source N -> channels N*16+1 .. N*16+16).

  This test is only compiled on macOS with the CoreMIDI plugin available.
*/

#include "gtest/gtest.h"

#if defined(__APPLE__)

#include <CoreServices/CoreServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <CoreMIDI/CoreMIDI.h>

#include "csound.h"

#include <chrono>
#include <cstring>
#include <string>
#include <thread>

namespace {

class CoreMidiInputTests : public ::testing::Test {
 protected:
  static constexpr int kSourceCount = 2;
  MIDIClientRef client {0};
  MIDIEndpointRef sources[kSourceCount] {};
  CSOUND *csound {nullptr};
  std::string priorOpcodeDir;
  std::string priorOpcodeDir64;
  bool hadOpcodeDir {false};
  bool hadOpcodeDir64 {false};

  static int sourceIndex(MIDIEndpointRef endpoint) {
    ItemCount n = MIDIGetNumberOfSources();
    for (ItemCount i = 0; i < n; i++)
      if (MIDIGetSource(i) == endpoint)
        return static_cast<int>(i);
    return -1;
  }

  void SetUp() override {
#if defined(CSOUND_TEST_COREMIDI_PLUGIN_DIR)
    /* Capture the prior plugin search path first, before any assertion that
       could abort SetUp, so TearDown always restores the original values
       (and never deletes variables it did not set). csoundGetEnv() also sees
       overrides set with csoundSetGlobalEnv(), unlike std::getenv(). */
    const char *value = csoundGetEnv(nullptr, "OPCODE7DIR");
    hadOpcodeDir = value != nullptr;
    if (hadOpcodeDir)
      priorOpcodeDir = value;
    value = csoundGetEnv(nullptr, "OPCODE7DIR64");
    hadOpcodeDir64 = value != nullptr;
    if (hadOpcodeDir64)
      priorOpcodeDir64 = value;
#endif
    ASSERT_EQ(MIDIClientCreate(CFSTR("csound input unittests"), nullptr,
                               nullptr, &client), noErr);
    ASSERT_EQ(MIDISourceCreate(client, CFSTR("csound unit test source A"),
                               &sources[0]), noErr);
    ASSERT_EQ(MIDISourceCreate(client, CFSTR("csound unit test source B"),
                               &sources[1]), noErr);

#if defined(CSOUND_TEST_COREMIDI_PLUGIN_DIR)
    /* Point the plugin search path at the freshly built coremidi plugin
       before Csound loads its default modules. */
    csoundSetGlobalEnv("OPCODE7DIR", CSOUND_TEST_COREMIDI_PLUGIN_DIR);
    csoundSetGlobalEnv("OPCODE7DIR64", CSOUND_TEST_COREMIDI_PLUGIN_DIR);
#endif
  }

  void TearDown() override {
    if (csound != nullptr)
      csoundDestroy(csound);
#if defined(CSOUND_TEST_COREMIDI_PLUGIN_DIR)
    csoundSetGlobalEnv("OPCODE7DIR",
                       hadOpcodeDir ? priorOpcodeDir.c_str() : nullptr);
    csoundSetGlobalEnv("OPCODE7DIR64",
                       hadOpcodeDir64 ? priorOpcodeDir64.c_str() : nullptr);
#endif
    if (client != 0)
      MIDIClientDispose(client);
  }

  static void sendNote(MIDIEndpointRef source, unsigned char status,
                       unsigned char data1, unsigned char data2) {
    Byte buffer[64];
    MIDIPacketList *packetList = reinterpret_cast<MIDIPacketList *>(buffer);
    MIDIPacket *packet = MIDIPacketListInit(packetList);
    unsigned char message[3] = {status, data1, data2};
    packet = MIDIPacketListAdd(packetList, sizeof(buffer), packet, 0, 3,
                               message);
    ASSERT_NE(packet, nullptr);
    MIDIReceived(source, packetList);
  }

  int channel() {
    return static_cast<int>(csoundGetControlChannel(csound, "chan", nullptr));
  }
};

TEST_F(CoreMidiInputTests, MapsSourcesToSeparatePorts) {
#if !defined(CSOUND_TEST_COREMIDI_PLUGIN_DIR)
  GTEST_SKIP() << "The coremidi plugin is not built";
#else
  const int indexA = sourceIndex(sources[0]);
  const int indexB = sourceIndex(sources[1]);
  if (indexA < 0 || indexB < 0)
    GTEST_SKIP() << "virtual CoreMIDI sources are not visible";
  const int channelA = indexA * 16 + 1;
  const int channelB = indexB * 16 + 1;
  ASSERT_NE(channelA, channelB);

  csound = csoundCreate(nullptr, nullptr);
  ASSERT_NE(csound, nullptr);
  csoundCreateMessageBuffer(csound, 0);

  bool haveCoreMidi = false;
  char *name = nullptr, *type = nullptr;
  for (int32_t n = 0; !csoundGetModule(csound, n, &name, &type); n++) {
    if (name != nullptr && type != nullptr && strcmp(name, "coremidi") == 0 &&
        strcmp(type, "midi") == 0) {
      haveCoreMidi = true;
      break;
    }
  }
  if (!haveCoreMidi)
    GTEST_SKIP() << "The coremidi module is not available";

  csoundSetMIDIModule(csound, "coremidi");
  ASSERT_EQ(csoundSetOption(csound, "-n"), 0);
  ASSERT_EQ(csoundSetOption(csound, "-Mm"), 0);

  const char *orc = R"(
    sr = 48000
    ksmps = 16
    nchnls = 1
    massign 0, 0
    instr 1
      /* dummy: absorbs any MIDI-triggered note events */
    endin
    instr 2
      kst, kch, kd1, kd2 midiin
      chnset kch, "chan"
    endin
  )";
  ASSERT_EQ(csoundCompileOrc(csound, orc, 0), 0);
  csoundEventString(csound, "i2 0 10\n", 0);
  ASSERT_EQ(csoundStart(csound), 0);

  /* CoreMIDI delivers input on its own thread, so let it run between
     performance cycles instead of spinning in a tight loop. */
  auto pollForChannel = [&](int expected) {
    for (int i = 0; i < 100 && channel() != expected; i++) {
      ASSERT_EQ(csoundPerformKsmps(csound), 0);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  };

  /* A note on source A must be reported on channel A. */
  sendNote(sources[0], 0x90, 60, 100);
  pollForChannel(channelA);
  EXPECT_EQ(channel(), channelA);

  /* And a note on source B must be reported on channel B. */
  sendNote(sources[1], 0x90, 62, 100);
  pollForChannel(channelB);
  EXPECT_EQ(channel(), channelB);
#endif
}

}  // namespace

#endif  // __APPLE__
