/*
  cmidi_input_test.cpp:

  Unit test for the CoreMIDI input plugin (InOut/cmidi.c): multiport
  mapping, a source opened by its number, and the devices it refuses.

  It creates its own virtual CoreMIDI sources, so no physical MIDI hardware
  or external device is required, then drives Csound through the "coremidi"
  MIDI input module with "-M m" and checks that each source is mapped to its
  own bank of 16 channels (source N -> channels N*16+1 .. N*16+16). It also
  checks that a source's number opens that source alone, and that a number
  past the last source, or a name, stops Csound at its start.

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

/* Reports the channel of each message midiin reads in "chan". */
const char *const kOrc = R"(
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

  /* CoreMIDI delivers input on its own thread, so let it run between
     performance cycles instead of spinning in a tight loop. */
  void performUntil(int expected) {
    for (int i = 0; i < 100 && channel() != expected; i++) {
      ASSERT_EQ(csoundPerformKsmps(csound), 0);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  /* True, with the coremidi module chosen, if `csound` has it. */
  bool useCoreMidi() {
    char *name = nullptr, *type = nullptr;
    for (int32_t n = 0; !csoundGetModule(csound, n, &name, &type); n++) {
      if (name != nullptr && type != nullptr &&
          strcmp(name, "coremidi") == 0 && strcmp(type, "midi") == 0) {
        csoundSetMIDIModule(csound, "coremidi");
        return true;
      }
    }
    return false;
  }

  /* Everything Csound said, emptying its message buffer. */
  std::string messages() {
    std::string text;
    while (csoundGetMessageCnt(csound) > 0) {
      text += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    return text;
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
  if (!useCoreMidi())
    GTEST_SKIP() << "The coremidi module is not available";

  ASSERT_EQ(csoundSetOption(csound, "-n"), 0);
  ASSERT_EQ(csoundSetOption(csound, "-Mm"), 0);
  ASSERT_EQ(csoundCompileOrc(csound, kOrc, 0), 0);
  csoundEventString(csound, "i2 0 10\n", 0);
  ASSERT_EQ(csoundStart(csound), 0);

  /* A note on source A must be reported on channel A. */
  sendNote(sources[0], 0x90, 60, 100);
  performUntil(channelA);
  EXPECT_EQ(channel(), channelA);

  /* And a note on source B must be reported on channel B. */
  sendNote(sources[1], 0x90, 62, 100);
  performUntil(channelB);
  EXPECT_EQ(channel(), channelB);
#endif
}

TEST_F(CoreMidiInputTests, OpensASourceByItsNumber) {
#if !defined(CSOUND_TEST_COREMIDI_PLUGIN_DIR)
  GTEST_SKIP() << "The coremidi plugin is not built";
#else
  const int indexA = sourceIndex(sources[0]);
  if (indexA < 0)
    GTEST_SKIP() << "virtual CoreMIDI sources are not visible";

  csound = csoundCreate(nullptr, nullptr);
  ASSERT_NE(csound, nullptr);
  csoundCreateMessageBuffer(csound, 0);
  if (!useCoreMidi())
    GTEST_SKIP() << "The coremidi module is not available";

  ASSERT_EQ(csoundSetOption(csound, "-n"), 0);
  ASSERT_EQ(csoundSetOption(csound, ("-M" + std::to_string(indexA)).c_str()),
            0);
  ASSERT_EQ(csoundCompileOrc(csound, kOrc, 0), 0);
  csoundEventString(csound, "i2 0 10\n", 0);
  ASSERT_EQ(csoundStart(csound), 0);

  /* Source A's note arrives, unmapped, on its own channel... */
  sendNote(sources[0], 0x90, 60, 100);
  performUntil(1);
  EXPECT_EQ(channel(), 1);

  /* ...and source B, which is not open, is not heard. */
  sendNote(sources[1], 0x91, 62, 100);
  performUntil(2);
  EXPECT_EQ(channel(), 1);
#endif
}

TEST_F(CoreMidiInputTests, RefusesADeviceItCannotOpen) {
#if !defined(CSOUND_TEST_COREMIDI_PLUGIN_DIR)
  GTEST_SKIP() << "The coremidi plugin is not built";
#else
  /* One past the last source, and names: until they were refused, the
     number connected nothing, "foo" source 0 and "1foo" source 1, and
     Csound started. */
  const ItemCount count = MIDIGetNumberOfSources();
  const std::string past = std::to_string(count);
  const std::string devices[][2] = {
      {past, count > 0 ? "MIDI input device " + past + " is out of range"
                       : "no MIDI input sources available"},
      {"foo", "invalid MIDI input device 'foo'"},
      {"1foo", "invalid MIDI input device '1foo'"}};
  for (const auto &device : devices) {
    if (csound != nullptr)
      csoundDestroy(csound);
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    if (!useCoreMidi())
      GTEST_SKIP() << "The coremidi module is not available";
    ASSERT_EQ(csoundSetOption(csound, "-n"), 0);
    ASSERT_EQ(csoundSetOption(csound, ("-M" + device[0]).c_str()), 0);
    ASSERT_EQ(csoundCompileOrc(csound, kOrc, 0), 0);
    EXPECT_NE(csoundStart(csound), 0) << device[0];
    EXPECT_NE(messages().find(device[1]), std::string::npos) << device[0];
  }
#endif
}

}  // namespace

#endif  // __APPLE__
